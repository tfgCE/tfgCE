#include "video3d.h"

#include "videoGLExtensions.h"

#include "renderTarget.h"
#include "shaderProgram.h"
#include "shaderProgramCache.h"
#include "vertexFormatUtils.h"

#include "primitivesPipeline.h"

#include "viewFrustum.h"
#include "videoClipPlaneStackImpl.inl"

#include "..\core.h"
#include "..\input.h"
#include "..\recentCapture.h"
#include "..\systemInfo.h"

#include "..\..\concurrency\scopedSpinLock.h"
#include "..\..\performance\performanceUtils.h"
#include "..\..\serialisation\serialiser.h"

#include "..\..\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef WITH_GL_CHECK_FOR_ERROR
#define AN_GL_ERROR_INFO(val) if (glError == val) info = TXT(#val);
void ::System::gl_report_error(int glError, char const* file, int line)
{
	tchar const* info = TXT("[unknown]");
	AN_GL_ERROR_INFO(GL_INVALID_ENUM);
	AN_GL_ERROR_INFO(GL_INVALID_VALUE);
	AN_GL_ERROR_INFO(GL_INVALID_OPERATION);
	AN_GL_ERROR_INFO(GL_OUT_OF_MEMORY);
	AN_GL_ERROR_INFO(GL_INVALID_FRAMEBUFFER_OPERATION);
#ifdef AN_CLANG
	error_dev_ignore(TXT("gl error (0x%04X) (@%s:%i) : %S"), glError, file, line, info);
#else
	String fileName = String::from_char8(file);
	error_dev_investigate(TXT("gl error (0x%04X) (@%S:%i) : %S"), glError, fileName.to_char(), line, info);
#endif
#ifdef BUILD_NON_RELEASE
#ifdef AN_WINDOWS
	// crash!
	int* a = 0;
	a += 1;
	delete a;
#endif
#endif
}
#undef AN_GL_ERROR_INFO
#endif

//

using namespace System;

//

#ifdef WITH_GL_DEBUG_MESSAGES
void
#ifndef AN_CLANG
GLAPIENTRY
#endif
gl_message_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	String info = String::from_char8(message);
	output(TXT("[gl_callback] %S type = 0x%x, severity = 0x%x, message = %S"),
		(type == GL_DEBUG_TYPE_ERROR ? TXT("GL ERROR") : TXT("")),
		type, severity, info.to_char());
}
#endif

//

Video3D* Video3D::s_current = nullptr;

void Video3D::create(VideoConfig const * _vc)
{
	an_assert(s_current == nullptr);
	s_current = new Video3D();
	if (_vc)
	{
		s_current->init(*_vc);
	}
	if (Input::get())
	{
		Input::get()->update_grab();
	}
}

void Video3D::terminate()
{
	delete_and_clear(s_current);
}

Video3D::Video3D()
: glContext(nullptr)
, currentFrameBuffer(0)
, blendEnabled(false)
, currentShaderProgramId(0)
, lazyShaderProgramId(0)
, currentShaderProgram(nullptr)
, lazyShaderProgram(nullptr)
, currentVertexBufferObject(0)
, lazyVertexBufferObject(0)
, lazyVertexBufferObjectInfo(&genericVertexBufferObjectInfo)
, currentVertexArrayObject(0)
, lazyVertexArrayObject(0)
, currentElementBufferObject(0)
, lazyElementBufferObject(0)
, activeTextureSlot(0)
, nearFarPlane(Range(0.1f, 100.0f))
{
	projectionMatrix = Matrix44::identity;

	clipPlaneStack.connect_video_matrix_stack(&modelViewMatrixStack);

	viewportLeftBottom = VectorInt2(0, 0);
	viewportSize = VectorInt2(1, 1);
	activeViewportLeftBottom = viewportLeftBottom;
	activeViewportSize = viewportSize;

	set_fallback_colour();

	invalidate_variables();

	while (currentTexturesID.get_size() < 8)
	{
		currentTexturesID.push_back(0);
		lazyTexturesID.push_back(0);
	}
	::System::Core::on_quick_exit(TXT("close video 3D"), []() {terminate(); });
}

Video3D::~Video3D()
{
	::System::Core::on_quick_exit_no_longer(TXT("close video 3D"));

	close();
}

bool Video3D::init(VideoConfig const & _vc)
{
	assert_rendering_thread();
	
	VideoInitInfo vii;
	vii.openGL = true;

	if (!initialised)
	{
		// TODO better initialization please! read from config?
#ifdef AN_SDL
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); // for now just force core profile only

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
#else
#ifdef AN_ANDROID
		// handled via EGLContext
#else
#error implement for non sdl
#endif
#endif

#ifdef AN_DEVELOPMENT
		todo_note(TXT("add parameter for open gl debug?"));
		/*
#ifdef AN_SDL
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG); 
#else
		#error implement for non sdl
#endif
		*/
#endif

		if (!Video::init(_vc, vii))
		{
			return false;
		}

		skipDisplayBufferForVR = config.skipDisplayBufferForVR;

#ifdef AN_SDL
		glContext = SDL_GL_CreateContext(window);
#else
#ifdef AN_ANDROID
		glContext = new EGLContext();
#else
#error implement
#endif
#endif
		if (!glContext)
		{
			// TODO report
			return false;
		}

		make_gl_context_current();

		{	// info
			output_colour_system();
			GLint majorVersion = 0, minorVersion = 0;
			DIRECT_GL_CALL_ glGetIntegerv(GL_MAJOR_VERSION, &majorVersion); AN_GL_CHECK_FOR_ERROR
			DIRECT_GL_CALL_ glGetIntegerv(GL_MINOR_VERSION, &minorVersion); AN_GL_CHECK_FOR_ERROR
			String openGLVersion = String::from_char8((char8*)glGetString(GL_VERSION));
			String vendor = String::from_char8((char8*)glGetString(GL_VENDOR));
			String renderer = String::from_char8((char8*)glGetString(GL_RENDERER));
			output(TXT("open gl: %S"), openGLVersion.to_char());
			output(TXT("vendor: %S"), vendor.to_char());
			output(TXT("renderer: %S"), renderer.to_char());
			Array<String> integratedGraphics;
			integratedGraphics.push_back(String(TXT("Intel(R) HD Graphics 630")));
			if (integratedGraphics.does_contain(renderer))
			{
				show_critical_message(false, TXT("It seems that you're using an integrated graphics card.\n\nPlease try running the game in performance mode, ie. using performance graphics card."));
			}
			useVec4ArraysInsteadOfMat4Arrays = _vc.should_use_vec4_arrays_instead_of_mat4_arrays(renderer);
			if (useVec4ArraysInsteadOfMat4Arrays)
			{
				output(TXT("(will be using vec4 arrays instead of mat4 arrays)"));
			}
			limitedSupportForIndexingInShaders = _vc.has_limited_support_for_indexing_in_shaders(renderer);
			if (limitedSupportForIndexingInShaders)
			{
				output(TXT("(limited support for indexing in shaders)"));
			}
			output_colour();
		}

#ifdef AN_SDL
		glewInit();
#endif
		VideoGLExtensions::get().init_extensions();

		{	// vsync 1
			// late vsync -1 - check if returns 0 for success or -1 for failure - use vsync(1) then
			// no vsync 0
			if (config.vsync)
			{
#ifdef AN_SDL
				//if (SDL_GL_SetSwapInterval(-1) == -1)
				{
					SDL_GL_SetSwapInterval(1);
				}
#else
#ifdef AN_GLES
				// no vsync
#else
#error implement
#endif
#endif
			}
			else
			{
#ifdef AN_SDL
				SDL_GL_SetSwapInterval(0);
#else
#ifdef AN_GLES
				// no vsync
#else
#error implement
#endif
#endif
			}
		}

#ifdef WITH_GL_DEBUG_MESSAGES
		// During init, enable debug output
		if (VideoGLExtensions::get().glDebugMessageCallback)
		{
			glEnable(GL_DEBUG_OUTPUT);
			VideoGLExtensions::get().glDebugMessageCallback(gl_message_callback, 0);
		}
#endif

		// some defaults for the lazy and current states
		{
			depth_mask();
			colour_mask();
			stencil_mask();
			depth_range(0.0f, 1.0f);
			depth_test();
			depth_clamp(false);
			stencil_test();
			stencil_op();
			front_face_order(FaceOrder::CW);
			face_display(FaceDisplay::Both);
			mark_disable_blend();
			mark_disable_polygon_offset();
		}
		requires_send_state();

		// by default, clear to something similar to black colour
		clear_colour(Colour::blackWarm);
		display_buffer();

		clipPlaneStack.open_clip_planes();

		shaderProgramCache = new ShaderProgramCache(this);

		useShaderProgramCache = true;

		init_fallback_shaders();
	}
	else
	{
		if (!Video::init(_vc, vii))
		{
			return false;
		}

		make_gl_context_current();
	}

	{
		bool useFullScreen = true;
		if (VR::IVR::get() && VR::IVR::get()->is_ok())
		{
			useFullScreen = VR::IVR::get()->get_available_functionality().renderToScreen;
		}
		if (useFullScreen)
		{
			update_full_screen_render_target();
		}
	}

	RecentCapture::initialise_static();

	initialised = true;
	displayedBufferIsClean = false;
	return true;
}

void Video3D::make_gl_context_current()
{
#ifdef AN_SDL
	SDL_GL_MakeCurrent(window, glContext);
#else
#ifdef AN_ANDROID
	glContext->make_current();
#else
	#error implement
#endif
#endif
}

void Video3D::init_fallback_shaders()
{
	RefCountObjectPtr<ShaderSetup> fallbackVertex2DShaderSetup(ShaderSetup::from_source(ShaderType::Vertex, PrimitivesPipeline::get_vertex_shader_for_2D_source()));
	RefCountObjectPtr<ShaderSetup> fallbackVertex3DShaderSetup(ShaderSetup::from_source(ShaderType::Vertex, PrimitivesPipeline::get_vertex_shader_for_3D_source()));
	RefCountObjectPtr<ShaderSetup> fallbackFragmentBShaderSetup(ShaderSetup::from_source(ShaderType::Fragment, PrimitivesPipeline::get_fragment_shader_basic_source()));
	RefCountObjectPtr<ShaderSetup> fallbackFragmentWTShaderSetup(ShaderSetup::from_source(ShaderType::Fragment, PrimitivesPipeline::get_fragment_shader_with_texture_source()));
	RefCountObjectPtr<ShaderSetup> fallbackFragmentWOTShaderSetup(ShaderSetup::from_source(ShaderType::Fragment, PrimitivesPipeline::get_fragment_shader_without_texture_source()));
	RefCountObjectPtr<ShaderSetup> fallbackFragmentWTDBShaderSetup(ShaderSetup::from_source(ShaderType::Fragment, PrimitivesPipeline::get_fragment_shader_with_texture_discards_blending_source()));
	RefCountObjectPtr<ShaderSetup> fallbackFragmentWOTDBShaderSetup(ShaderSetup::from_source(ShaderType::Fragment, PrimitivesPipeline::get_fragment_shader_without_texture_discards_blending_source()));

	fallbackVertex2DShaderSetup->fill_version(PrimitivesPipeline::get_version());
	fallbackVertex3DShaderSetup->fill_version(PrimitivesPipeline::get_version());
	fallbackFragmentBShaderSetup->fill_version(PrimitivesPipeline::get_version());
	fallbackFragmentWTShaderSetup->fill_version(PrimitivesPipeline::get_version());
	fallbackFragmentWOTShaderSetup->fill_version(PrimitivesPipeline::get_version());
	fallbackFragmentWTDBShaderSetup->fill_version(PrimitivesPipeline::get_version());
	fallbackFragmentWOTDBShaderSetup->fill_version(PrimitivesPipeline::get_version());

	Shader* fallbackVertex2DShader = new Shader(*fallbackVertex2DShaderSetup.get());
	Shader* fallbackVertex3DShader = new Shader(*fallbackVertex3DShaderSetup.get());
	Shader* fallbackFragmentBShader = new Shader(*fallbackFragmentBShaderSetup.get());
	Shader* fallbackFragmentWTShader = new Shader(*fallbackFragmentWTShaderSetup.get());
	Shader* fallbackFragmentWOTShader = new Shader(*fallbackFragmentWOTShaderSetup.get());
	Shader* fallbackFragmentWTDBShader = new Shader(*fallbackFragmentWTDBShaderSetup.get());
	Shader* fallbackFragmentWOTDBShader = new Shader(*fallbackFragmentWOTDBShaderSetup.get());

	fallbackFragmentBShader->access_fragment_shader_output_setup().allow_default_output();
	fallbackFragmentWTShader->access_fragment_shader_output_setup().allow_default_output();
	fallbackFragmentWOTShader->access_fragment_shader_output_setup().allow_default_output();
	fallbackFragmentWTDBShader->access_fragment_shader_output_setup().allow_default_output();
	fallbackFragmentWOTDBShader->access_fragment_shader_output_setup().allow_default_output();

	fallbackShaderPrograms.set_size(Video3DFallbackShader::COUNT);
	fallbackShaderPrograms[Video3DFallbackShader::For2D | Video3DFallbackShader::Basic] = new ShaderProgram(fallbackVertex2DShader, fallbackFragmentBShader);
	fallbackShaderPrograms[Video3DFallbackShader::For3D | Video3DFallbackShader::Basic] = new ShaderProgram(fallbackVertex3DShader, fallbackFragmentBShader);
	fallbackShaderPrograms[Video3DFallbackShader::For2D | Video3DFallbackShader::WithTexture] = new ShaderProgram(fallbackVertex2DShader, fallbackFragmentWTShader);
	fallbackShaderPrograms[Video3DFallbackShader::For3D | Video3DFallbackShader::WithTexture] = new ShaderProgram(fallbackVertex3DShader, fallbackFragmentWTShader);
	fallbackShaderPrograms[Video3DFallbackShader::For2D | Video3DFallbackShader::WithoutTexture] = new ShaderProgram(fallbackVertex2DShader, fallbackFragmentWOTShader);
	fallbackShaderPrograms[Video3DFallbackShader::For3D | Video3DFallbackShader::WithoutTexture] = new ShaderProgram(fallbackVertex3DShader, fallbackFragmentWOTShader);
	fallbackShaderPrograms[Video3DFallbackShader::For2D | Video3DFallbackShader::WithTexture    | Video3DFallbackShader::DiscardsBlending ] = new ShaderProgram(fallbackVertex2DShader, fallbackFragmentWTDBShader);
	fallbackShaderPrograms[Video3DFallbackShader::For3D | Video3DFallbackShader::WithTexture    | Video3DFallbackShader::DiscardsBlending ] = new ShaderProgram(fallbackVertex3DShader, fallbackFragmentWTDBShader);
	fallbackShaderPrograms[Video3DFallbackShader::For2D | Video3DFallbackShader::WithoutTexture | Video3DFallbackShader::DiscardsBlending ] = new ShaderProgram(fallbackVertex2DShader, fallbackFragmentWOTDBShader);
	fallbackShaderPrograms[Video3DFallbackShader::For3D | Video3DFallbackShader::WithoutTexture | Video3DFallbackShader::DiscardsBlending ] = new ShaderProgram(fallbackVertex3DShader, fallbackFragmentWOTDBShader);
	//fallbackShaderProgram->use_default_textures_for_missing() ?? from where should we get default texture?

	fallbackShaderProgramInstances.set_size(Video3DFallbackShader::COUNT);
	for_every(fspi, fallbackShaderProgramInstances)
	{
		*fspi = new ShaderProgramInstance();
		(*fspi)->set_shader_program(fallbackShaderPrograms[for_everys_index(fspi)].get());
	}
}

void Video3D::update_full_screen_render_target()
{
	assert_rendering_thread();
	fullScreenRenderTarget.clear();
	unbind_frame_buffer_bare();
	RenderTargetSetup fullScreenRenderTargetSetup(screenSize);
	fullScreenRenderTargetSetup.add_output_texture(::System::OutputTextureDefinition(Name::invalid(), ::System::VideoFormat::rgba8, ::System::BaseVideoFormat::rgba));
	RENDER_TARGET_CREATE_INFO(TXT("full screen render target"));
	fullScreenRenderTarget = new RenderTarget(fullScreenRenderTargetSetup);
	fullScreenRenderTarget->bind();
	clear_colour(Colour::black);
	unbind_frame_buffer_bare();
}

#define DELETE_ALL_FROM(_list) \
{ \
	while (! _list.is_empty()) \
	{ \
		delete _list.get_first(); \
	} \
}

#define DELETE_ALL_FROM_REFS(_list) \
{ \
	while (! _list.is_empty()) \
	{ \
		an_assert(_list.get_first()->get_ref_count() == 1, TXT("ref count should be 1, is %i"), _list.get_first()->get_ref_count()); \
		_list.get_first() = nullptr; /* will delete it and remove it from the list */ \
	} \
}

void Video3D::close()
{
	assert_rendering_thread();

	RecentCapture::close_static();

	if (shaderProgramCache && should_use_shader_program_cache())
	{
		if (!shaderProgramCache->is_empty())
		{
			shaderProgramCache->save_to_file();
		}
		delete_and_clear(shaderProgramCache);
	}

	for_every(fspi, fallbackShaderProgramInstances)
	{
		delete_and_clear(*fspi);
	}
	fallbackShaderProgramInstances.clear();

	// clear as static array doesn't use constructors and destructors
	fallbackShaderPrograms.clear();

	if (::System::Core::is_performing_quick_exit())
	{
		for_every_ptr(sp, shaderPrograms)
		{
			sp->close();
		}
		for_every_ptr(s, shaders)
		{
			s->close();
		}
	}
	else
	{
		an_assert(shaderPrograms.get_size() == 0);
		an_assert(shaders.get_size() == 0);
	}

	fullScreenRenderTarget.clear();

#ifdef AN_DEVELOPMENT
	if (!renderTargets.is_empty())
	{
		debug_list_render_targets();
	}
#endif
	if (::System::Core::is_performing_quick_exit())
	{
		Concurrency::ScopedSpinLock lock(renderTargetsLock);
		for_every_ptr(rt, renderTargets)
		{
			rt->close(true);
		}
	}
	else
	{
		Concurrency::ScopedSpinLock lock(renderTargetsLock);
		an_assert(renderTargets.is_empty()); // we should already have no render targets at this point
	}

	// render target data,  mesh parts
	destroy_pending();

	if (glContext)
	{
#ifdef AN_SDL
		SDL_GL_DeleteContext(glContext);
#else
#ifdef AN_ANDROID
		delete glContext;
#else
		#error implement
#endif
#endif
		glContext = nullptr;
	}

	Video::close();

	initialised = false;
}

bool Video3D::is_ok()
{
	return Video::is_ok() && glContext;
}

#define USE_OFFSET(_pointer, _offset) (void*)(((int8*)_pointer) + _offset)

bool Video3D::BoundBuffersAndVertexFormat::check_if_requires_change(BufferObjectID _vertexBufferObject, BufferObjectID _elementBufferObject, VertexFormat const * _vertexFormat, ShaderProgram * _shaderProgram, void const * _dataPointer, TextureID const & _fallbackTexture, Colour const & _fallbackColour) const
{
	if (!isBound || !vertexFormatValid)
	{
		return true;
	}
	if (vertexBufferObject != _vertexBufferObject ||
		elementBufferObject != _elementBufferObject ||
		shaderProgram != _shaderProgram ||
		dataPointer != _dataPointer ||
		fallbackColour != _fallbackColour ||
		fallbackTexture != _fallbackTexture)
	{
		return true;
	}
	if (_vertexFormat && _vertexFormat->is_used_for_dynamic_data())
	{
		return true;
	}
	if (vertexFormat != _vertexFormat)
	{
		// pointer is different but vertex format may look the same
		if ((vertexFormat != nullptr) ^ (_vertexFormat != nullptr))
		{
			return true;
		}
		if (vertexFormat)
		{
			an_assert(_vertexFormat);
			if (!VertexFormatUtils::do_match(*vertexFormat, *_vertexFormat))
			{
				return true;
			}
		}
		else
		{
			an_assert(!_vertexFormat);
		}
	}
	return false;
}

void Video3D::bind_and_send_vertex_buffer_to_load_data(BufferObjectID _vertexBufferObject)
{
	// if loading, always bind buffer properly
	DIRECT_GL_CALL_ glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObject); AN_GL_CHECK_FOR_ERROR
	if (currentVertexArrayObject == 0)
	{
		currentVertexBufferObject = _vertexBufferObject;
	}
	else
	{
		an_assert(!lazyVertexBufferObjectInfo || lazyVertexBufferObjectInfo == &genericVertexBufferObjectInfo || lazyVertexBufferObjectInfo->vertexBufferObjectID == _vertexBufferObject, TXT("we should be using vbo associated with currently bound vao or unbind vao first"));
	}
}

void Video3D::bind_and_send_element_buffer_to_load_data(BufferObjectID _elementBufferObject)
{
	// if loading, always bind buffer properly
	DIRECT_GL_CALL_ glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _elementBufferObject); AN_GL_CHECK_FOR_ERROR
	if (currentVertexArrayObject == 0)
	{
		currentElementBufferObject = _elementBufferObject;
	}
	else
	{
		an_assert(!lazyVertexBufferObjectInfo || lazyVertexBufferObjectInfo == &genericVertexBufferObjectInfo || lazyVertexBufferObjectInfo->elementBufferObjectID == _elementBufferObject, TXT("we should be using ebo associated with currently bound vao or unbind vao first"));
	}
}

void Video3D::bind_and_send_buffers_and_vertex_format(BufferObjectID _vertexBufferObject, BufferObjectID _elementBufferObject, VertexFormat const * _vertexFormat, ShaderProgram * _shaderProgram, void const * _dataPointer)
{
	an_assert(!_vertexFormat || _vertexFormat->is_ok_to_be_used());

	bool forceResetup = (_vertexFormat && _vertexFormat->is_used_for_dynamic_data())
		|| _vertexBufferObject == 0 // always reset if no actual VBO, we're most likely using provided data
		|| !_shaderProgram // always reset if no shader program and using fallback
		;

	if (! forceResetup &&
		! boundBuffersAndVertexFormat.check_if_requires_change(_vertexBufferObject, _elementBufferObject, _vertexFormat, _shaderProgram, _dataPointer, fallbackTexture, fallbackColour))
	{
		if (boundBuffersAndVertexFormat.shaderProgram &&
			boundBuffersAndVertexFormat.usingFallbackShaderProgram)
		{
			// update build in uniforms, we don't need to bind everything else, just have to remember about those for fallback shader program
			boundBuffersAndVertexFormat.shaderProgram->set_build_in_uniforms();
		}
		return;
	}

	unbind_and_send_buffers_and_vertex_format();

	boundBuffersAndVertexFormat.vertexBufferObject = _vertexBufferObject;
	boundBuffersAndVertexFormat.elementBufferObject = _elementBufferObject;
	boundBuffersAndVertexFormat.vertexFormat = _vertexFormat;
	boundBuffersAndVertexFormat.vertexFormatValid = true;
	boundBuffersAndVertexFormat.shaderProgram = _shaderProgram;
	boundBuffersAndVertexFormat.usingFallbackShaderProgram = false;
	boundBuffersAndVertexFormat.dataPointer = _dataPointer;
	boundBuffersAndVertexFormat.fallbackTexture = fallbackTexture;
	boundBuffersAndVertexFormat.fallbackColour = fallbackColour;

	an_assert(!boundBuffersAndVertexFormat.vertexFormat || boundBuffersAndVertexFormat.vertexFormat->is_ok_to_be_used());

	bind_vertex_buffers(boundBuffersAndVertexFormat.vertexBufferObject, boundBuffersAndVertexFormat.elementBufferObject);

	if (boundBuffersAndVertexFormat.vertexFormat)
	{
		if (!boundBuffersAndVertexFormat.shaderProgram)
		{
			int fallbackShaderId = 0;
			fallbackShaderId += boundBuffersAndVertexFormat.vertexFormat->get_location() == VertexFormatLocation::XY ? Video3DFallbackShader::For2D : Video3DFallbackShader::For3D;
			fallbackShaderId += fallbackTexture != texture_id_none() ? Video3DFallbackShader::WithTexture : Video3DFallbackShader::WithoutTexture;
			boundBuffersAndVertexFormat.shaderProgram = fallbackShaderPrograms[fallbackShaderId].get();
			boundBuffersAndVertexFormat.shaderProgram->bind();
			if (fallbackTexture != texture_id_none())
			{
				boundBuffersAndVertexFormat.shaderProgram->set_build_in_uniform_in_texture(fallbackTexture);
			}
			boundBuffersAndVertexFormat.usingFallbackShaderProgram = true;
		}
		if (boundBuffersAndVertexFormat.shaderProgram)
		{
			bool requiresBindingVertexAttrib = true;
			if (!forceResetup &&
				lazyVertexBufferObjectInfo != &genericVertexBufferObjectInfo &&
				lazyVertexBufferObjectInfo->shaderProgram == boundBuffersAndVertexFormat.shaderProgram)
			{
				requiresBindingVertexAttrib = false;
			}
			if (requiresBindingVertexAttrib)
			{
				boundBuffersAndVertexFormat.vertexFormat->bind_vertex_attrib(this, boundBuffersAndVertexFormat.shaderProgram, boundBuffersAndVertexFormat.dataPointer);
				lazyVertexBufferObjectInfo->shaderProgram = boundBuffersAndVertexFormat.shaderProgram;
			}
		}
		else
		{
			error_stop(TXT("no longer supported - always requires a shader"));
		}
	}
	else
	{
		unbind_all_vertex_attribs();
		lazyVertexBufferObjectInfo->shaderProgram = nullptr;
	}

	requires_send_vertex_buffers(RequiresSendVertexBufferParams().force_resetup(forceResetup));

	boundBuffersAndVertexFormat.isBound = true;
}

#undef USE_OFFSET

void Video3D::soft_unbind_buffers_and_vertex_format()
{
	// if we were using fallback shader program, do unbind now!
	// no need to send (hence "soft unbind")
	if (boundBuffersAndVertexFormat.isBound &&
		boundBuffersAndVertexFormat.usingFallbackShaderProgram)
	{
		if (boundBuffersAndVertexFormat.shaderProgram)
		{
			boundBuffersAndVertexFormat.shaderProgram->unbind();
			boundBuffersAndVertexFormat.shaderProgram = nullptr;
		}
		unbind_vertex_buffers();
	} 
}

bool Video3D::unbind_and_send_buffers_and_vertex_format()
{
	if (boundBuffersAndVertexFormat.isBound)
	{
		// unbind any currently bound
		if (boundBuffersAndVertexFormat.vertexFormat)
		{
			if (boundBuffersAndVertexFormat.shaderProgram &&
				boundBuffersAndVertexFormat.usingFallbackShaderProgram)
			{
				boundBuffersAndVertexFormat.shaderProgram->unbind();
				boundBuffersAndVertexFormat.shaderProgram = nullptr;
			}
		}
		unbind_vertex_buffers();
		boundBuffersAndVertexFormat.isBound = false;

		requires_send_vertex_buffers(RequiresSendVertexBufferParams().just_unbind());

		return true;
	}
	else
	{
		return false;
	}
}

void Video3D::update()
{
	assert_rendering_thread();
}

void Video3D::add(Shader* _s)
{
	shaders.push_back(_s);
}

void Video3D::remove(Shader* _s)
{
	shaders.remove(_s);
}

void Video3D::add(ShaderProgram* _sp)
{
	shaderPrograms.push_back(_sp);
}

void Video3D::remove(ShaderProgram* _sp)
{
	shaderPrograms.remove(_sp);
}

void Video3D::on_close(ShaderProgram* _sp)
{
	an_assert(lazyShaderProgramId != _sp->get_shader_program_id(), TXT("you should always unbind shader program!"));
	if (currentShaderProgramId == _sp->get_shader_program_id())
	{
		requires_send_shader_program();
	}
	if (boundBuffersAndVertexFormat.isBound &&
		boundBuffersAndVertexFormat.shaderProgram == _sp)
	{
		unbind_and_send_buffers_and_vertex_format();
	}
}

#ifdef AN_DEVELOPMENT
void Video3D::debug_list_render_targets(bool _onlyCount)
{
	Concurrency::ScopedSpinLock lock(renderTargetsLock);
	output(TXT("render targets (%i):"), renderTargets.get_size());
	if (!_onlyCount)
	{
		int idx = 0;
		for_every_ptr(rt, renderTargets)
		{
			String text;
			VectorInt2 size = rt->get_size();
			text += String::printf(TXT(" %02i %ix%i outputs:%i"), idx, size.x, size.y, rt->get_setup().get_output_texture_count());
			output(text.to_char());
			++idx;
		}
	}
}
#endif

void Video3D::display_buffer_full_screen_render_target()
{
	MEASURE_PERFORMANCE(displayBuffer_fullScreenRenderTarget);

	unbind_frame_buffer_bare();

	setup_for_2d_display();
	set_viewport(VectorInt2::zero, config.resolutionFull);
	set_near_far_plane(0.02f, 100.0f);
	
	set_2d_projection_matrix_ortho();
	access_model_view_matrix_stack().clear();
	
	clear_colour(Colour::black);

	Vector2 resolutionFull = config.resolutionFull.to_vector2();
	Vector2 upScaledSize = fullScreenRenderTarget->get_size().to_vector2();
	upScaledSize.y *= resolutionFull.x / upScaledSize.x;
	upScaledSize.x = resolutionFull.x;
	if (upScaledSize.y > resolutionFull.y)
	{
		upScaledSize.x *= resolutionFull.y / upScaledSize.y;
		upScaledSize.y = resolutionFull.y;
	}

	Vector2 leftBottom = (config.resolutionFull.to_vector2() - upScaledSize) * 0.5f;
	fullScreenRenderTarget->render(0, this, leftBottom, upScaledSize);
}

void Video3D::bind_render_buffer(FrameBufferObjectID _frameBuffer)
{
	DIRECT_GL_CALL_ glBindRenderbuffer(GL_RENDERBUFFER, _frameBuffer); AN_GL_CHECK_FOR_ERROR
}

void Video3D::unbind_render_buffer()
{
	DIRECT_GL_CALL_ glBindRenderbuffer(GL_RENDERBUFFER, 0); AN_GL_CHECK_FOR_ERROR
}

void Video3D::bind_frame_buffer(FrameBufferObjectID _frameBuffer, Optional<int> _asGLFrameBuffer)
{
	if (!forceFrameBufferBinding && currentFrameBuffer == _frameBuffer)
	{
		// we could be bounding full screen render target, change information about how we're bound
		an_assert_immediate(frameBufferBound || fullScreenFrameBufferBound);
		frameBufferBound = true;
		fullScreenFrameBufferBound = false;
		return;
	}
	currentFrameBuffer = _frameBuffer;
	DIRECT_GL_CALL_ glBindFramebuffer(_asGLFrameBuffer.get(GL_DRAW_FRAMEBUFFER), currentFrameBuffer); AN_GL_CHECK_FOR_ERROR
	frameBufferBound = true;
	fullScreenFrameBufferBound = false;
}

void Video3D::unbind_frame_buffer(Optional<int> _asGLFrameBuffer)
{
	if (config.fullScreen == FullScreen::WindowScaled && fullScreenRenderTarget.get())
	{
		if (!fullScreenRenderTarget->is_bound())
		{
			fullScreenRenderTarget->bind();
		}
		frameBufferBound = false;
		fullScreenFrameBufferBound = true;
		return;
	}
	if (!forceFrameBufferBinding && !currentFrameBuffer)
	{
		an_assert_immediate(! frameBufferBound);
		an_assert_immediate(!fullScreenFrameBufferBound);
		return;
	}
	currentFrameBuffer = 0;
	DIRECT_GL_CALL_ glBindFramebuffer(_asGLFrameBuffer.get(GL_DRAW_FRAMEBUFFER), currentFrameBuffer); AN_GL_CHECK_FOR_ERROR
	frameBufferBound = false;
	fullScreenFrameBufferBound = false;
}

bool Video3D::unbind_frame_buffer_bare(Optional<int> _asGLFrameBuffer)
{
	if (!forceFrameBufferBinding &&
		!currentFrameBuffer)
	{
		an_assert_immediate(!frameBufferBound);
		an_assert_immediate(!fullScreenFrameBufferBound);
		return false;
	}
	FrameBufferObjectID prev = currentFrameBuffer;
	currentFrameBuffer = 0;
	DIRECT_GL_CALL_ glBindFramebuffer(_asGLFrameBuffer.get(GL_DRAW_FRAMEBUFFER), currentFrameBuffer); AN_GL_CHECK_FOR_ERROR
	frameBufferBound = false;
	fullScreenFrameBufferBound = false;
	return fullScreenRenderTarget.get() && prev == fullScreenRenderTarget->get_frame_buffer_object_id_to_write();
}

void Video3D::invalidate_bound_frame_buffer_info()
{
	forceFrameBufferBinding = true;
}

void Video3D::rebuild_custom_build_in_uniforms()
{
	customBuildInUniforms.clear();
	for (int i = 0; i < custom_build_in_uniforms.get_size(); ++i)
	{
		customBuildInUniforms.add_from(custom_build_in_uniforms[i].uniforms);
	}
}

void Video3D::add_custom_set_build_in_uniforms(Name const & _id, Array<Name> const& _customIsBuildInUniforms, CustomSetBuildInUniforms _custom_set_build_in_uniforms)
{
	Concurrency::ScopedSpinLock lock(customBuildInUniformsLock);

	CustomBuildInUniform cbiu;
	cbiu.id = _id;
	cbiu.uniforms = _customIsBuildInUniforms;
	cbiu.custom_set_build_in_uniforms = _custom_set_build_in_uniforms;
	custom_build_in_uniforms.push_back(cbiu);
	rebuild_custom_build_in_uniforms();
}

void Video3D::remove_custom_set_build_in_uniforms(Name const& _id)
{
	Concurrency::ScopedSpinLock lock(customBuildInUniformsLock);

	for (int i = 0; i < custom_build_in_uniforms.get_size(); ++i)
	{
		auto& cbiu = custom_build_in_uniforms[i];
		if (cbiu.id == _id)
		{
			custom_build_in_uniforms.remove_at(i);
			--i;
		}
	}
	rebuild_custom_build_in_uniforms();
}

void Video3D::apply_custom_set_build_in_uniforms_for(ShaderProgram* _shaderProgram)
{
	for_count(int, i, custom_build_in_uniforms.get_size())
	{
		custom_build_in_uniforms[i].custom_set_build_in_uniforms(_shaderProgram);
	}
}

bool Video3D::is_custom_build_in_uniform(Name const & _shaderParam) const
{
	return customBuildInUniforms.does_contain(_shaderParam);
}

void Video3D::display_buffer()
{
	MEASURE_PERFORMANCE(displayBuffer);
	assert_rendering_thread();
	bool doDisplay = true;
	if (skipDisplayBufferForVR &&
		VR::IVR::can_be_used())
	{
		doDisplay = false;
	}
	if (doDisplay)
	{
#ifdef AN_GLES
		// no buffer to display, we only handle it through VR
#else
		if (config.fullScreen == FullScreen::WindowScaled && fullScreenRenderTarget.get())
		{
			display_buffer_full_screen_render_target();
		}
		{
			MEASURE_PERFORMANCE(displayBuffer_swapWindow);
#ifdef AN_SDL
			SDL_GL_SwapWindow(window);
#else
			#error implement
#endif
		}
#endif
		float timeSinceLastDisplay = lastDisplayTS.get_time_since();
		lastDisplayTS.reset();
		System::Core::store_display_delta_time(timeSinceLastDisplay);
#ifdef AN_GLES
		// no buffer to display, we only handle it through VR
#else
#ifdef AN_SYSTEM_INFO
		SystemInfo::display_buffer_gl_finish_start();
#endif
		{
			MEASURE_PERFORMANCE(displayBuffer_finish);
			DIRECT_GL_CALL_ glFinish(); AN_GL_CHECK_FOR_ERROR_IGNORE // make sure that all effects are done and only then swap window
			//DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR // make sure that all effects are done and only then swap window
		}
#ifdef AN_SYSTEM_INFO
		SystemInfo::display_buffer_gl_finish_end();
#endif
#endif
		displayedBufferIsClean = false;
	}
	else
	{
#ifdef AN_GLES
		// no buffer to display, we only handle it through VR
#else
		if (!displayedBufferIsClean)
		{
			displayedBufferIsClean = true;

			// clear display buffer
			unbind_frame_buffer_bare();
			
			//setup_for_2d_display();
			//if (config.fullScreen == FullScreen::WindowScaled)
			//{
			//	set_viewport(VectorInt2::zero, config.resolutionFull);
			//}
			//else
			//{
			//	set_viewport(VectorInt2::zero, config.resolution);
			//}
			//set_near_far_plane(0.02f, 100.0f);
			//
			//set_2d_projection_matrix_ortho();
			//access_model_view_matrix_stack().clear();
			clear_colour(Colour::blackWarm);
			{
				MEASURE_PERFORMANCE(displayBuffer_swapWindow);
#ifdef AN_SDL
				SDL_GL_SwapWindow(window);
#else
				#error implement for non sdl
#endif
			}
			float timeSinceLastDisplay = lastDisplayTS.get_time_since();
			lastDisplayTS.reset();
			System::Core::store_display_delta_time(timeSinceLastDisplay);
			{
				MEASURE_PERFORMANCE(displayBuffer_finish);
				DIRECT_GL_CALL_ glFinish(); AN_GL_CHECK_FOR_ERROR
			}
		}
		else
#endif
		{
			float timeSinceLastDisplay = lastDisplayTS.get_time_since();
			lastDisplayTS.reset();
			System::Core::store_display_delta_time(timeSinceLastDisplay);
		}
	}
	bufferIsReadyToDisplay = false;
}

void Video3D::add(RenderTarget* _rt)
{
	Concurrency::ScopedSpinLock lock(renderTargetsLock);
	renderTargets.push_back(_rt);
}

void Video3D::remove(RenderTarget* _rt)
{
	Concurrency::ScopedSpinLock lock(renderTargetsLock);
	renderTargets.remove(_rt);
}

void Video3D::destroy_pending(int _limit)
{
	assert_rendering_thread();

	int destroyed = 0;
	while (_limit == 0 || destroyed < _limit)
	{
		if (buffersToDestroy.get_size() > 0)
		{
			::System::BufferObjectID bufToDestroy;
			{
				Concurrency::ScopedSpinLock lock(toDestroyLock);
				bufToDestroy = buffersToDestroy.get_first();
				buffersToDestroy.pop_front();
			}
			++destroyed;
			assert_rendering_thread();
			DIRECT_GL_CALL_ glDeleteBuffers(1, &bufToDestroy); AN_GL_CHECK_FOR_ERROR
			gpu_memory_info_freed(GPU_MEMORY_INFO_TYPE_BUFFER, bufToDestroy);
			continue;
		}
		if (vertexArraysToDestroy.get_size() > 0)
		{
			::System::VertexArrayObjectID vaoToDestroy;
			{
				Concurrency::ScopedSpinLock lock(toDestroyLock);
				vaoToDestroy = vertexArraysToDestroy.get_first();
				vertexArraysToDestroy.pop_front();
			}
			++destroyed;
			assert_rendering_thread();
			DIRECT_GL_CALL_ glDeleteVertexArrays(1, &vaoToDestroy); AN_GL_CHECK_FOR_ERROR
			continue;
		}
		if (!renderTargetVideo3DDatasToDestroy.is_empty())
		{
			RefCountObjectPtr<RenderTargetVideo3DData> rtvd;
			{
				Concurrency::ScopedSpinLock lock(toDestroyLock);
				rtvd = renderTargetVideo3DDatasToDestroy.get_first();
				renderTargetVideo3DDatasToDestroy.pop_front();
			}
			++destroyed;
			assert_rendering_thread();
			if (auto* r = rtvd.get())
			{
				r->close();
			}
			continue;
		}

		// nothing to destroy
		break;
	}
}

void Video3D::defer_buffer_to_destroy(::System::BufferObjectID _buffer)
{
	Concurrency::ScopedSpinLock lock(toDestroyLock);
	buffersToDestroy.push_back(_buffer);
}

void Video3D::defer_vertex_array_to_destroy(::System::VertexArrayObjectID _vao)
{
	Concurrency::ScopedSpinLock lock(toDestroyLock);
	vertexArraysToDestroy.push_back(_vao);
}

void Video3D::defer_render_target_data_to_destroy(RenderTargetVideo3DData* _rttd)
{
	Concurrency::ScopedSpinLock lock(toDestroyLock);
	renderTargetVideo3DDatasToDestroy.push_back(RefCountObjectPtr<RenderTargetVideo3DData>(_rttd));
}


void Video3D::bind_vertex_buffers(BufferObjectID _vertexBufferObjectID, BufferObjectID _elementBufferObjectID)
{
	lazyVertexBufferObject = _vertexBufferObjectID;
	lazyElementBufferObject = _elementBufferObjectID;
	if (lazyVertexBufferObject == 0)
	{
		lazyVertexBufferObjectInfo = &genericVertexBufferObjectInfo;
	}
	else
	{
		VertexBufferObjectInfo* emptyAvailable = nullptr;
		bool found = false;
		for_every(vboi, vertexBufferObjectInfos)
		{
			if (vboi->vertexBufferObjectID == lazyVertexBufferObject)
			{
				an_assert(vboi->elementBufferObjectID == _elementBufferObjectID);
				found = true;
				lazyVertexBufferObjectInfo = vboi;
				break;
			}
			else if (!emptyAvailable && vboi->vertexBufferObjectID == 0)
			{
				emptyAvailable = vboi;
			}
		}

		if (!found)
		{
			if (emptyAvailable)
			{
				emptyAvailable->ready_for(_vertexBufferObjectID, _elementBufferObjectID);
				lazyVertexBufferObjectInfo = emptyAvailable;
			}
			else
			{
				vertexBufferObjectInfos.grow_size(1);
				lazyVertexBufferObjectInfo = &vertexBufferObjectInfos.get_last();
				lazyVertexBufferObjectInfo->ready_for(_vertexBufferObjectID, _elementBufferObjectID);
			}
		}
	}

	lazyVertexArrayObject = lazyVertexBufferObjectInfo->vertexArrayObjectID;
}

void Video3D::on_vertex_buffer_destroyed(BufferObjectID _vertexBufferObjectID)
{
	if (lazyVertexBufferObjectInfo && lazyVertexBufferObjectInfo->vertexBufferObjectID == _vertexBufferObjectID)
	{
		lazyVertexBufferObjectInfo = &genericVertexBufferObjectInfo;
	}
	for_every(vboi, vertexBufferObjectInfos)
	{
		if (vboi->vertexBufferObjectID == _vertexBufferObjectID)
		{
			vboi->vertexBufferObjectID = 0;
			if (vboi->vertexArrayObjectID)
			{
				if (is_rendering_thread())
				{
					DIRECT_GL_CALL_ glDeleteVertexArrays(1, &vboi->vertexArrayObjectID); AN_GL_CHECK_FOR_ERROR
				}
				else
				{
					defer_buffer_to_destroy(vboi->vertexArrayObjectID);
				}
				vboi->vertexArrayObjectID = 0;
				vboi->bufferBoundToArray = false;
			}
			break;
		}
	}
}

void Video3D::unbind_vertex_buffers()
{
	bind_vertex_buffers(0, 0);
}

void Video3D::requires_send_vertex_buffers()
{
	requires_send_vertex_buffers(RequiresSendVertexBufferParams());
}

void Video3D::requires_send_vertex_buffers(RequiresSendVertexBufferParams const& _params)
{
	bool alreadyJustBoundVertextBufferObjects = false;
	// vertex array object first as we're switching between them
	if (currentVertexArrayObject != lazyVertexArrayObject)
	{
		currentVertexArrayObject = lazyVertexArrayObject;
		DIRECT_GL_CALL_ glBindVertexArray(currentVertexArrayObject); AN_GL_CHECK_FOR_ERROR

		if (!_params.justUnbind &&
			currentVertexArrayObject &&
			!lazyVertexBufferObjectInfo->bufferBoundToArray)
		{
			DIRECT_GL_CALL_ glBindBuffer(GL_ARRAY_BUFFER, lazyVertexBufferObjectInfo->vertexBufferObjectID); AN_GL_CHECK_FOR_ERROR
			DIRECT_GL_CALL_ glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lazyVertexBufferObjectInfo->elementBufferObjectID); AN_GL_CHECK_FOR_ERROR
			lazyVertexBufferObjectInfo->bufferBoundToArray = true;
			alreadyJustBoundVertextBufferObjects = true;
		}
	}
	if (! currentVertexArrayObject)
	{
		if (currentVertexBufferObject != lazyVertexBufferObject ||
			currentElementBufferObject != lazyElementBufferObject)
		{
			currentVertexBufferObject = lazyVertexBufferObject;
			currentElementBufferObject = lazyElementBufferObject;
			DIRECT_GL_CALL_ glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, currentElementBufferObject); AN_GL_CHECK_FOR_ERROR
			DIRECT_GL_CALL_ glBindBuffer(GL_ARRAY_BUFFER, currentVertexBufferObject); AN_GL_CHECK_FOR_ERROR
				alreadyJustBoundVertextBufferObjects = true;
		}
	}

	if (!_params.justUnbind)
	{
		// always send these, it is especially important when changing raw pointers
		if (lazyVertexBufferObjectInfo)
		{
			lazyVertexBufferObjectInfo->send(
				VertexBufferObjectInfo::SendParams()
				.already_just_bound_vertext_buffer_objects(alreadyJustBoundVertextBufferObjects)
				.force_send(_params.forceResetup));
		}

		// this just works
		genericVertexAttribInfo.send();
	}
}

void Video3D::bind_vertex_attrib(int _attributeId, int _valCount, float const* _val)
{
	genericVertexAttribInfo.bind(_attributeId, _valCount, _val);
}

void Video3D::unbind_all_vertex_attribs()
{
	genericVertexAttribInfo.unbind_all();
	if (lazyVertexBufferObjectInfo)
	{
		lazyVertexBufferObjectInfo->unbind_all();
	}
}

void Video3D::bind_vertex_attrib_array(int _attributeId, int _elementCount, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _stride, void* _pointer)
{
	an_assert(lazyVertexBufferObjectInfo);
	lazyVertexBufferObjectInfo->bind(_attributeId, _elementCount, _dataType, _attribType, _stride, _pointer);
}

//

Video3D::GenericVertexAttribInfo::GenericVertexAttribInfo()
{
	attributes.set_size(attributes.get_max_size());
	attributesSend = 0;
	attributesInUse = 0;
}

void Video3D::GenericVertexAttribInfo::unbind_all()
{
	attributesInUse = 0;
}

void Video3D::GenericVertexAttribInfo::bind(int _attributeId, int _valueCount, float const* _values)
{
	Attribute & a = attributes[_attributeId];

	auto& lv = a.lazyValue;

	lv.valueCount = _valueCount;
	memory_copy(lv.value, _values, sizeof(float) * lv.valueCount);

	attributesInUse = max(attributesInUse, _attributeId + 1);
}

void Video3D::GenericVertexAttribInfo::send()
{
	int count = max(attributesInUse, attributesSend);
	auto* a = attributes.get_data();
	for (int i = 0; i < count; ++i, ++a)
	{
		auto& cv = a->currentValue;
		auto& lv = a->lazyValue;
		if (cv.valueCount != lv.valueCount ||
			(cv.valueCount > 0 && ! memory_compare(cv.value, lv.value, sizeof(float) * cv.valueCount)))
		{
			cv.valueCount = lv.valueCount;
			if (cv.valueCount > 0)
			{
				int const attributeId = i;
				memory_copy(cv.value, lv.value, sizeof(float) * cv.valueCount);
				if (cv.valueCount == 2)
				{
					DIRECT_GL_CALL_ glVertexAttrib2f(attributeId, cv.value[0], cv.value[1]); AN_GL_CHECK_FOR_ERROR
				}
				else if (cv.valueCount == 3)
				{
					DIRECT_GL_CALL_ glVertexAttrib3f(attributeId, cv.value[0], cv.value[1], cv.value[2]); AN_GL_CHECK_FOR_ERROR
				}
				else if (cv.valueCount == 4)
				{
					DIRECT_GL_CALL_ glVertexAttrib4f(attributeId, cv.value[0], cv.value[1], cv.value[2], cv.value[3]); AN_GL_CHECK_FOR_ERROR
				}
			}
		}
	}

	attributesSend = attributesInUse;
}

//

Video3D::VertexBufferObjectInfo::VertexBufferObjectInfo()
{
	attributes.set_size(attributes.get_max_size());
	attributesSend = 0;
	attributesInUse = 0;
}

void Video3D::VertexBufferObjectInfo::ready_for(BufferObjectID _vertexBufferObjectID, BufferObjectID _elementBufferObjectID)
{
	an_assert(vertexBufferObjectID == 0);
	an_assert(vertexArrayObjectID == 0);
	vertexBufferObjectID = _vertexBufferObjectID;
	elementBufferObjectID = _elementBufferObjectID;
	shaderProgram = nullptr;
	if (vertexBufferObjectID != 0)
	{
		DIRECT_GL_CALL_ glGenVertexArrays(1, &vertexArrayObjectID); AN_GL_CHECK_FOR_ERROR
	}
	bufferBoundToArray = false;
	attributes.set_size(attributes.get_max_size());
	for_every(a, attributes)
	{
		a->currentValue = Attribute::Value();
		a->lazyValue = Attribute::Value();
	}
	attributesSend = 0;
	attributesInUse = 0;
}

void Video3D::VertexBufferObjectInfo::unbind_all()
{
	int count = max(attributesInUse, attributesSend);
	auto* a = attributes.get_data();
	for (int i = 0; i < count; ++i, ++a)
	{
		a->lazyValue.inUse = false;
	}
	attributesInUse = 0;
}

void Video3D::VertexBufferObjectInfo::bind(int _attributeId, int _elementCount, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _stride, void* _pointer)
{
	Attribute & a = attributes[_attributeId];

	auto& lv = a.lazyValue;

	lv.inUse = true;
	lv.elementCount = _elementCount;
	lv.dataType = _dataType;
	lv.attribType = _attribType;
	lv.stride = _stride;
	lv.pointer = _pointer;

	attributesInUse = max(attributesInUse, _attributeId + 1);
}

void Video3D::VertexBufferObjectInfo::send(SendParams const& _params)
{
	bool requiresSetting = _params.forceSend;
	int count = max(attributesInUse, attributesSend);
	if (! requiresSetting)
	{
		auto* a = attributes.get_data();
		for (int i = 0; i < count; ++i, ++a)
		{
			auto& cv = a->currentValue;
			auto& lv = a->lazyValue;
			if (cv.inUse != lv.inUse)
			{
				requiresSetting = true;
				break;
			}
			if (cv.inUse)
			{
				if (cv.elementCount != lv.elementCount
				 || cv.dataType != lv.dataType
				 || cv.attribType != lv.attribType
				 || cv.stride != lv.stride
				 || cv.pointer != lv.pointer)
				{
					requiresSetting = true;
					break;
				}
			}
		}
	}
	if (requiresSetting)
	{
		// if we're required to change anything, we should change everything
		if (!_params.alreadyJustBoundVertextBufferObjects)
		{
			DIRECT_GL_CALL_ glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectID); AN_GL_CHECK_FOR_ERROR
			DIRECT_GL_CALL_ glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferObjectID); AN_GL_CHECK_FOR_ERROR
		}
		auto* a = attributes.get_data();
		for (int i = 0; i < count; ++i, ++a)
		{
			auto& cv = a->currentValue;
			auto& lv = a->lazyValue;
			// always set
			{
				int const attributeId = i;
				cv.inUse = lv.inUse;
				if (!cv.inUse)
				{
#ifndef AN_GLES
					if (vertexArrayObjectID != 0)
					{
						DIRECT_GL_CALL_ glDisableVertexArrayAttrib(vertexArrayObjectID, attributeId); AN_GL_CHECK_FOR_ERROR
					}
					else
#endif
					{
						DIRECT_GL_CALL_ glDisableVertexAttribArray(attributeId); AN_GL_CHECK_FOR_ERROR
					}
				}
				else
				{
#ifndef AN_GLES
					if (vertexArrayObjectID)
					{
						DIRECT_GL_CALL_ glEnableVertexArrayAttrib(vertexArrayObjectID, attributeId); AN_GL_CHECK_FOR_ERROR
					}
					else
#endif
					{
						DIRECT_GL_CALL_ glEnableVertexAttribArray(attributeId); AN_GL_CHECK_FOR_ERROR
					}
				}
			}
			if (cv.inUse)
			{
				int const attributeId = i;
				cv = lv;
				if (DataFormatValueType::is_integer_value(cv.dataType) && VertexFormatAttribType::should_be_mapped_as_integer(cv.attribType))
				{
					DIRECT_GL_CALL_ glVertexAttribIPointer(attributeId, cv.elementCount, cv.dataType, cv.stride, cv.pointer); AN_GL_CHECK_FOR_ERROR
				}
				else
				{
					DIRECT_GL_CALL_ glVertexAttribPointer(attributeId, cv.elementCount, cv.dataType, DataFormatValueType::is_normalised_data(cv.dataType), cv.stride, cv.pointer); AN_GL_CHECK_FOR_ERROR
				}
			}
		}
	}
	attributesSend = attributesInUse;
}

