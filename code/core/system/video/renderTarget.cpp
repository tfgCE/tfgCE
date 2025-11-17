#include "renderTarget.h"

#include "renderTargetUtils.h"
#include "video3d.h"
#include "videoGLExtensions.h"
#include "shaderProgram.h"
#include "camera.h"

#include "..\..\mesh\mesh3d.h"

#include "..\timeStamp.h"
#include "..\systemInfo.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_GL
#define USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
#endif

//#define ELABORATE_DEBUG_LOGGING

//#define ALLOW_FOVEATED_FRAMEBUFFER

//

using namespace ::System;

//

// shader program attrib
DEFINE_STATIC_NAME(inOViewRay);

//

#ifdef DEBUG_RENDER_TARGET_MEMORY
int g_renderTargetMemoryAllocated = 0;

void render_target_info_report_memory()
{
#ifdef AN_GL
	auto& glExtensions = ::System::VideoGLExtensions::get();
	if (glExtensions.NVX_gpu_memory_info)
	{
		GLint dedm = 0;
		glGetIntegerv(GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedm); AN_GL_CHECK_FOR_ERROR
		GLint totalm = 0;
		glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalm); AN_GL_CHECK_FOR_ERROR
		GLint curavm = 0;
		glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &curavm); AN_GL_CHECK_FOR_ERROR
		output(TXT("[render-target-memory] dedicated %iMB, total %iMB, currently avaiable %iMB"), dedm >> 10, totalm >> 10, curavm >> 10);
	}
#endif
	output(TXT("[render-target-memory] render target allocated %10iMB"), g_renderTargetMemoryAllocated >> 20);
}

void render_target_info_allocated_memory(int _type, int _id, VectorInt2 _size, int _bytesPerPixel = 4)
{
	int memsize = _size.x * _size.y * _bytesPerPixel;
	g_renderTargetMemoryAllocated += memsize;
	output(TXT("[render-target-memory] allo + %4ix%4i [%i] = %20i : %5iMB   : %20i : %10iMB"), _size.x, _size.y, _bytesPerPixel, memsize, memsize >> 20, g_renderTargetMemoryAllocated, g_renderTargetMemoryAllocated >> 20);
	gpu_memory_info_allocated(_type, _id, memsize);
}

void render_target_info_free_memory(int _type, int _id, VectorInt2 _size, int _bytesPerPixel = 4)
{
	int memsize = _size.x * _size.y * _bytesPerPixel;
	g_renderTargetMemoryAllocated -= memsize;
	output(TXT("[render-target-memory] free - %4ix%4i [%i] = %20i : %5iMB   : %20i : %10iMB"), _size.x, _size.y, _bytesPerPixel, memsize, memsize >> 20, g_renderTargetMemoryAllocated, g_renderTargetMemoryAllocated >> 20);
	gpu_memory_info_freed(_type, _id);
}
#endif

//

void RenderTargetSetup::force_mip_maps(bool _forceMipMaps)
{
	forceMipMaps = _forceMipMaps;
	for_every(outputTexture, outputs)
	{
		outputTexture->set_with_mip_maps(_forceMipMaps);
	}
}

bool RenderTargetSetup::should_use_srgb_conversion() const
{
	if (forcedSRGBConversion.is_set())
	{
		return forcedSRGBConversion.get();
	}
	if (get_output_texture_count() >= 1)
	{
		auto videoFormat = get_output_texture_definition(0).get_video_format();
		return ::System::VideoFormat::is_srgb(videoFormat);
	}
	return false;
}

//

void RenderTargetSetupProxy::attach()
{
	GLenum frameBufferTarget;
#ifdef AN_GL
	frameBufferTarget = GL_FRAMEBUFFER;
#endif
#ifdef AN_GLES
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	frameBufferTarget = should_use_msaa() ? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
#else
	frameBufferTarget = GL_FRAMEBUFFER;
#endif
#endif
	if (depthStencilTexture != 0)
	{
		RenderTargetUtils::attach_colour_texture_and_depth_stencil_texture_to_frame_buffer(frameBufferObject, colourTexture, get_msaa_samples(), depthStencilTexture, DepthStencilFormat::has_stencil(get_depth_stencil_format()), frameBufferTarget);
	}
	else 
	{
		RenderTargetUtils::attach_colour_texture_and_depth_stencil_render_buffer_to_frame_buffer(frameBufferObject, colourTexture, get_msaa_samples(), depthStencilBufferObject, DepthStencilFormat::has_stencil(get_depth_stencil_format()), frameBufferTarget);
	}
}

//
//	based on: http://www.songho.ca/opengl/gl_fbo.html
//

RenderTargetPtr RenderTarget::boundRenderTarget;
Range2 RenderTarget::wholeUVRange(Range(0.0f, 1.0f), Range(0.0f, 1.0f));

RenderTarget::RenderTarget(RenderTargetSetup const & _setup)
: isProxy(false)
, size(VectorInt2::zero)
, v3dData(new RenderTargetVideo3DData())
{
	assert_rendering_thread();

	an_assert(Video3D::get());
	Video3D::get()->add(this);

	init(_setup);
}

RenderTarget::RenderTarget(RenderTargetSetupProxy const & _setupProxy)
: isProxy(true)
, size(VectorInt2::zero)
, v3dData(new RenderTargetVideo3DData())
{
	RenderTargetSetupProxy setupProxy = _setupProxy;
	setupProxy.attach();
	construct_as_proxy(setupProxy);
}

RenderTarget::RenderTarget(RenderTargetSetupDynamicProxy const& _setupProxy)
: isProxy(true)
, isDynamicProxy(true)
, size(VectorInt2::zero)
, v3dData(new RenderTargetVideo3DData())
{
	construct_as_proxy(_setupProxy);
}

void RenderTarget::construct_as_proxy(RenderTargetSetupProxy const& _setupProxy)
{
	assert_rendering_thread();

	an_assert(Video3D::get());
	Video3D::get()->add(this);

	an_assert(_setupProxy.get_output_texture_count() == 1, TXT("for proxies, there should be just one output texture!"));
	v3dData->dataTexturesData.set_size(1);
	v3dData->dataTexturesData[0].textureID = _setupProxy.colourTexture;
	v3dData->frameBufferObject = _setupProxy.frameBufferObject;
	v3dData->depthStencilTexture = _setupProxy.depthStencilTexture;
	v3dData->depthStencilBufferObject = _setupProxy.depthStencilBufferObject;

	if (_setupProxy.colourTexture == texture_id_none())
	{
		error_stop(TXT("no colour texture provided"));
	}
	if (v3dData->frameBufferObject == 0)
	{
		error_stop(TXT("no frame buffer provided"));
	}
	if (DepthStencilFormat::has_depth(_setupProxy.get_depth_stencil_format()))
	{
		// depth texture is not required, render buffer is enough
		if (v3dData->depthStencilTexture == 0 &&
			v3dData->depthStencilBufferObject == 0)
		{
			error_stop(TXT("depth stencil required but not provided"));
		}
	}
	if (!DepthStencilFormat::has_depth(_setupProxy.get_depth_stencil_format()))
	{
		// depth texture is not required, render buffer is enough
		if (v3dData->depthStencilTexture != 0 ||
			v3dData->depthStencilBufferObject != 0)
		{
			error_stop(TXT("depth stencil not required but provided"));
		}
	}

	init(_setupProxy);

	an_assert(v3dData->dataTexturesData.get_size() == 1 && v3dData->dataTexturesData[0].textureID == _setupProxy.colourTexture);
}

RenderTarget* RenderTarget::create_copy(std::function<void(RenderTargetSetup & _setup)> _alterSetup) const
{
	an_assert(!isProxy, TXT("can't copy a proxy render target!"));
	if (_alterSetup)
	{
		RenderTargetSetup setup = get_setup();
		_alterSetup(setup);
		RENDER_TARGET_CREATE_INFO(TXT("altered copy"));
		return new RenderTarget(setup);
	}
	else
	{
		RENDER_TARGET_CREATE_INFO(TXT("copy"));
		return new RenderTarget(get_setup());
	}
}

void RenderTarget::init(RenderTargetSetup const & _setup)
{
	assert_rendering_thread();
	
	Video3D::get()->requires_send_all();

	close(true);

	setup = _setup;
	
	if (setup.get_output_texture_count() == 0)
	{
		error(TXT("no outputs defined"));
		return;
	}

	if (setup.get_size() == VectorInt2::zero)
	{
		error(TXT("size of render target is zero (0,0)"));
		return;
	}

#ifdef DEBUG_RENDER_TARGET_CREATION
	output(TXT("[render-target] create %ix%i"), _setup.get_size().x, _setup.get_size().y);
	output(TXT("[render-target] output %i"), _setup.get_output_texture_count());
	output(TXT("[render-target] depth+stencil %i"), _setup.get_depth_stencil_format());
	output(TXT("[render-target] msaa %i"), _setup.get_msaa_samples());
#endif

#ifdef AN_GLES
	auto& glExtensions = ::System::VideoGLExtensions::get();
	bool const canDoMSAA = glExtensions.glRenderbufferStorageMultisampleEXT && glExtensions.glFramebufferTexture2DMultisampleEXT;
#else
	bool const canDoMSAA = true;
#endif

	if (!canDoMSAA)
	{
		setup.dont_use_msaa();
	}

	size = setup.get_size();

	v3dData->dataTexturesData.set_size(setup.get_output_texture_count());
	mark_all_outputs_required();

	withMipMaps = setup.are_mip_maps_forced();

	Video3D::get()->requires_send_all();

	GLenum frameBufferTarget;
#ifdef AN_GL
	frameBufferTarget = GL_FRAMEBUFFER;
#endif
#ifdef AN_GLES
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	frameBufferTarget = !isProxy && setup.should_use_msaa() ? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
#else
	frameBufferTarget = GL_FRAMEBUFFER;
#endif
#endif

	if (isProxy)
	{
		// we don't check, we just assume it is right
		isValid = true;
	}
	else
	{
#ifdef DEBUG_RENDER_TARGET_CREATION
		output(TXT("[render-target] gen buffer(s)"));
#endif
		// create frame buffer object
		DIRECT_GL_CALL_ glGenFramebuffers(1, &v3dData->frameBufferObject); AN_GL_CHECK_FOR_ERROR
			Video3D::get()->bind_frame_buffer(v3dData->frameBufferObject, frameBufferTarget);

		// we will use it to check rendering
		FrameBufferObjectID setupRenderToFrameBufferObject = v3dData->frameBufferObject;

#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
		if (setup.should_use_msaa())
		{
			// create render to frame buffer object
			DIRECT_GL_CALL_ glGenFramebuffers(1, &v3dData->renderToFrameBufferObject); AN_GL_CHECK_FOR_ERROR
				Video3D::get()->bind_frame_buffer(v3dData->renderToFrameBufferObject, frameBufferTarget);
			setupRenderToFrameBufferObject = v3dData->renderToFrameBufferObject;
		}
#endif

		// unbound for now as we might be binding depth buffer
		Video3D::get()->unbind_frame_buffer(frameBufferTarget);

		// depth/stencil texture
		if (DepthStencilFormat::has_depth(setup.get_depth_stencil_format()))
		{
			if (setup.should_use_msaa())
			{
				// create it as render buffer
				DIRECT_GL_CALL_ glGenRenderbuffers(1, &v3dData->depthStencilBufferObject); AN_GL_CHECK_FOR_ERROR

#ifdef DEBUG_RENDER_TARGET_CREATION
				output(TXT("[render-target] prepare depth stencil render buffer"));
#endif
#ifdef DEBUG_RENDER_TARGET_MEMORY
				render_target_info_report_memory();
#endif
				RenderTargetUtils::prepare_depth_stencil_render_buffer(v3dData->depthStencilBufferObject,
						RenderTargetUtils::PrepareDepthStencilRenderBufferParams()
						.with_size(size)
						.with_stencil(DepthStencilFormat::has_stencil(setup.get_depth_stencil_format()))
						.with_msaa_samples(setup.get_msaa_samples())
					);

#ifdef DEBUG_RENDER_TARGET_MEMORY
				render_target_info_allocated_memory(GPU_MEMORY_INFO_TYPE_RENDER_BUFFER, v3dData->depthStencilBufferObject, size, 4);
#endif
			}
			else
			{
#ifdef DEBUG_RENDER_TARGET_CREATION
				output(TXT("[render-target] prepare depth stencil render texture"));
#endif
				// create it as render texture
				DIRECT_GL_CALL_ glGenTextures(1, &v3dData->depthStencilTexture); AN_GL_CHECK_FOR_ERROR
				Video3D::get()->mark_use_texture(0, v3dData->depthStencilTexture);
				Video3D::get()->force_send_use_texture_slot(0); AN_GL_CHECK_FOR_ERROR
				Video3D::get()->requires_send_use_textures();
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
				// only if we plan to generate mip maps
				//DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)(withMipMaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR)); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
				// initial value is false, but hey
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE); AN_GL_CHECK_FOR_ERROR // no automatic mipmap
#endif
				if (setup.get_mip_maps_limit() != NONE)
				{
					DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, setup.get_mip_maps_limit()); AN_GL_CHECK_FOR_ERROR
				}
#ifdef DEBUG_RENDER_TARGET_MEMORY
				render_target_info_report_memory();
#endif
				if (DepthStencilFormat::has_stencil(setup.get_depth_stencil_format()))
				{
					DIRECT_GL_CALL_ glTexImage2D(GL_TEXTURE_2D, 0, setup.get_depth_stencil_format(), size.x, size.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0); AN_GL_CHECK_FOR_ERROR
				}
				else
				{
					DIRECT_GL_CALL_ glTexImage2D(GL_TEXTURE_2D, 0, setup.get_depth_stencil_format(), size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, 0); AN_GL_CHECK_FOR_ERROR
				}
				Video3D::get()->mark_use_texture(0, texture_id_none());
				Video3D::get()->requires_send_use_textures();

#ifdef DEBUG_RENDER_TARGET_MEMORY
				render_target_info_allocated_memory(GPU_MEMORY_INFO_TYPE_TEXTURE, v3dData->depthStencilTexture, size, 4);
#endif
			}
		}

		// data (colour) texture
		{
#ifdef DEBUG_RENDER_TARGET_CREATION
			output(TXT("[render-target] prepare data (colour) texture(s) (%i)"), v3dData->dataTexturesData.get_size());
#endif
			int dataTextureObjectIndex = 0;
			allocate_stack_var(TextureID, dataTextureObjects, v3dData->dataTexturesData.get_size());
			DIRECT_GL_CALL_ glGenTextures(setup.get_output_texture_count(), dataTextureObjects); AN_GL_CHECK_FOR_ERROR
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
			allocate_stack_var(TextureID, dataTextureMSAAObjects, v3dData->dataTexturesData.get_size());
			if (setup.should_use_msaa())
			{
				DIRECT_GL_CALL_ glGenTextures(setup.get_output_texture_count(), dataTextureMSAAObjects); AN_GL_CHECK_FOR_ERROR
			}
#endif
			for_every(dataTextureData, v3dData->dataTexturesData)
			{
				dataTextureData->textureID = dataTextureObjects[dataTextureObjectIndex];
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
				dataTextureData->renderToTextureID = setup.should_use_msaa() ? dataTextureMSAAObjects[dataTextureObjectIndex] : 0;
#else
				dataTextureData->renderToTextureID = 0;
#endif
#ifdef ELABORATE_DEBUG_LOGGING
				output(TXT("created texture 0x%X, render to 0x%X"), dataTextureData->textureID, dataTextureData->renderToTextureID);
#endif
				OutputTextureDefinition const & dataTextureSetup = setup.get_output_texture_definition(dataTextureObjectIndex);
#ifdef DEBUG_RENDER_TARGET_MEMORY
				dataTextureData->bytesPerPixel = ::System::VideoFormat::get_pixel_size(dataTextureSetup.get_video_format());
#endif
				dataTextureData->magFiltering = TextureFiltering::get_proper_for(dataTextureSetup.get_filtering_mag(), withMipMaps);
				dataTextureData->minFiltering = TextureFiltering::get_proper_for(dataTextureSetup.get_filtering_min(), withMipMaps);

#ifdef DEBUG_RENDER_TARGET_MEMORY
				render_target_info_report_memory();
#endif
				RenderTargetUtils::prepare_render_target_texture(dataTextureData->textureID,
					RenderTargetUtils::PrepareRenderTargetParams()
						.with_wrap_u(dataTextureSetup.get_wrap_u())
						.with_wrap_v(dataTextureSetup.get_wrap_v())
						.with_mag_filtering(dataTextureData->magFiltering)
						.with_min_filtering(dataTextureData->minFiltering)
						.with_mip_maps(withMipMaps? Optional<int>(setup.get_mip_maps_limit()) : NP)
						.with_size(size)
						.with_video_format(dataTextureSetup.get_video_format())
						.with_base_video_format(dataTextureSetup.get_base_video_format())
						.with_video_format_data(dataTextureSetup.get_video_format_data())
					);

#ifdef DEBUG_RENDER_TARGET_MEMORY
				render_target_info_allocated_memory(GPU_MEMORY_INFO_TYPE_TEXTURE, dataTextureData->textureID, size, dataTextureData->bytesPerPixel);
#endif

#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
				if (setup.should_use_msaa())
				{
#ifdef AN_GL
					int textureTarget = GL_TEXTURE_2D_MULTISAMPLE;
#endif
#ifdef AN_GLES
					int textureTarget = GL_TEXTURE_2D;
#endif

					DIRECT_GL_CALL_ glBindTexture(textureTarget, dataTextureData->renderToTextureID); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GLES
					DIRECT_GL_CALL_ glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, TextureFiltering::get_sane_mag(dataTextureData->magFiltering)); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, TextureFiltering::get_sane_min(dataTextureData->minFiltering)); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, dataTextureSetup.get_wrap_u()); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, dataTextureSetup.get_wrap_v()); AN_GL_CHECK_FOR_ERROR
#endif
#ifdef DEBUG_RENDER_TARGET_MEMORY
					render_target_info_report_memory();
#endif
#ifdef AN_GL
					DIRECT_GL_CALL_ glTexImage2DMultisample(textureTarget, setup.get_msaa_samples(), dataTextureSetup.get_video_format(), size.x, size.y, GL_TRUE); AN_GL_CHECK_FOR_ERROR
#endif
#ifdef AN_GLES
					DIRECT_GL_CALL_ glTexImage2D(textureTarget, 0, dataTextureSetup.get_video_format(), size.x, size.y, 0, dataTextureSetup.get_base_video_format(), dataTextureSetup.get_video_format_data(), 0); AN_GL_CHECK_FOR_ERROR
#endif
					DIRECT_GL_CALL_ glBindTexture(textureTarget, 0); AN_GL_CHECK_FOR_ERROR

#ifdef DEBUG_RENDER_TARGET_MEMORY
					render_target_info_allocated_memory(GPU_MEMORY_INFO_TYPE_TEXTURE, dataTextureData->renderToTextureID, size, dataTextureData->bytesPerPixel);
#endif
				}
#endif
				withMipMaps |= dataTextureSetup.is_with_mip_maps();
				++dataTextureObjectIndex;
			}
		}

#ifdef AN_SYSTEM_INFO
		::System::SystemInfo::created_render_target();
#endif

		// foveated rendering setup
		if (!isDynamicProxy)
		{
			// if you want to use foveated rendering for proxy, manage texture on your own
			if (setup.get_foveated_rendering().is_set() &&
				v3dData->dataTexturesData.get_size() > 0)
			{
#ifdef DEBUG_RENDER_TARGET_CREATION
				output(TXT("[render-target] setup foveated rendering"));
#endif
				/*
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
				FrameBufferObjectID frameBufferObjectToTest = !isProxy && setup.should_use_msaa() ? v3dData->renderToFrameBufferObject : v3dData->frameBufferObject;
#else
				FrameBufferObjectID frameBufferObjectToTest = v3dData->frameBufferObject;
#endif
				Video3D::get()->bind_frame_buffer(frameBufferObjectToTest, frameBufferTarget);
				*/
#ifdef ALLOW_FOVEATED_FRAMEBUFFER
				if (setup.should_use_msaa() && glExtensions.QCOM_framebuffer_foveated)
				{
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
					auto frameBufferID = !isProxy && setup.should_use_msaa() ? v3dData->renderToFrameBufferObject : v3dData->frameBufferObject;
#else
					auto frameBufferID = v3dData->frameBufferObject;
#endif
					usingFoveatedRendering = RenderTargetUtils::setup_foveated_rendering_for_framebuffer(setup.get_for_eye_idx(), frameBufferID, setup.get_foveated_rendering().get());
				}
				else
#endif
				{
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
					auto textureID = !isProxy && setup.should_use_msaa() ? v3dData->dataTexturesData[0].renderToTextureID : v3dData->dataTexturesData[0].textureID;
#else
					auto textureID = v3dData->dataTexturesData[0].textureID;
#endif
					usingFoveatedRendering = RenderTargetUtils::setup_foveated_rendering_for_texture(setup.get_for_eye_idx(), textureID, setup.get_foveated_rendering().get());
				}
				/*
				Video3D::get()->unbind_frame_buffer(frameBufferTarget);
				*/
			}
		}

		{
#ifdef DEBUG_RENDER_TARGET_CREATION
			output(TXT("[render-target] attach everything"));
#endif

			// attach textures
			{
				Video3D::get()->bind_frame_buffer(v3dData->frameBufferObject, frameBufferTarget);

				int dataTextureObjectIndex = 0;
				for_every(dataTextureData, v3dData->dataTexturesData)
				{
#ifndef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
					auto& glExtensions = ::System::VideoGLExtensions::get();
					DIRECT_GL_CALL_ glExtensions.glFramebufferTexture2DMultisampleEXT(frameBufferTarget, GL_COLOR_ATTACHMENT0 + dataTextureObjectIndex, GL_TEXTURE_2D, dataTextureData->textureID, 0, setup.get_msaa_samples()); AN_GL_CHECK_FOR_ERROR
#else
					DIRECT_GL_CALL_ glFramebufferTexture2D(frameBufferTarget, GL_COLOR_ATTACHMENT0 + dataTextureObjectIndex, GL_TEXTURE_2D, dataTextureData->textureID, 0); AN_GL_CHECK_FOR_ERROR
#endif
					++dataTextureObjectIndex;
				}
			}
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
			if (setup.should_use_msaa())
			{
				Video3D::get()->bind_frame_buffer(v3dData->renderToFrameBufferObject, frameBufferTarget);

				// attach texture
				{
					int dataTextureObjectIndex = 0;
					for_every(dataTextureData, v3dData->dataTexturesData)
					{
#ifdef AN_GL
						DIRECT_GL_CALL_ glFramebufferTexture2D(frameBufferTarget, GL_COLOR_ATTACHMENT0 + dataTextureObjectIndex, GL_TEXTURE_2D_MULTISAMPLE, dataTextureData->renderToTextureID, 0); AN_GL_CHECK_FOR_ERROR
#endif
#ifdef AN_GLES
							auto& glExtensions = ::System::VideoGLExtensions::get();
						DIRECT_GL_CALL_ glExtensions.glFramebufferTexture2DMultisampleEXT(frameBufferTarget, GL_COLOR_ATTACHMENT0 + dataTextureObjectIndex, GL_TEXTURE_2D, dataTextureData->renderToTextureID, 0, setup.get_msaa_samples()); AN_GL_CHECK_FOR_ERROR
#endif
							++dataTextureObjectIndex;
					}
				}
			}
#endif
			// attach depth+stencil texture/render buffers
			if (DepthStencilFormat::has_depth(setup.get_depth_stencil_format()))
			{
				if (v3dData->depthStencilTexture != 0)
				{
					RenderTargetUtils::attach_depth_stencil_texture_to_frame_buffer(setupRenderToFrameBufferObject, v3dData->depthStencilTexture, DepthStencilFormat::has_stencil(setup.get_depth_stencil_format()), frameBufferTarget);
				}
				else
				{
					RenderTargetUtils::attach_depth_stencil_render_buffer_to_frame_buffer(setupRenderToFrameBufferObject, v3dData->depthStencilBufferObject, DepthStencilFormat::has_stencil(setup.get_depth_stencil_format()), frameBufferTarget);
				}
			}
		}
	}

	// unbind frame buffer object for now
	Video3D::get()->unbind_frame_buffer(frameBufferTarget);

	{
		for_count(int, bufferIdx, 2)
		{
			FrameBufferObjectID frameBufferObjectToTest = bufferIdx == 0 ? v3dData->frameBufferObject : v3dData->renderToFrameBufferObject;
			if (frameBufferObjectToTest != 0)
			{
				Video3D::get()->bind_frame_buffer(frameBufferObjectToTest, frameBufferTarget);
				DIRECT_GL_CALL_ isValid = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE; // no AN_GL_CHECK_FOR_ERROR, we check with error_on_invalid_framebuffer

				if (!isValid)
				{
					error(TXT("error creating render target"));
					RenderTargetUtils::error_on_invalid_framebuffer(frameBufferTarget);
				}
			}
		}

		// unbind frame buffer object for now
		Video3D::get()->unbind_frame_buffer(frameBufferTarget);
	}
	//

	v3dData->vertexFormat = vertex_format().with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float).with_custom(NAME(inOViewRay), ::System::DataFormatValueType::Float, ::System::VertexFormatAttribType::Float, 2);
	v3dData->vertexFormat.no_padding();
	v3dData->vertexFormat.used_for_dynamic_data();
	v3dData->vertexFormat.calculate_stride_and_offsets();
}

void RenderTarget::update_foveated_rendering(::System::FoveatedRenderingSetup const& _setup)
{
	if (!isDynamicProxy &&
		usingFoveatedRendering &&
		v3dData->dataTexturesData.get_size() > 0)
	{
#ifdef ALLOW_FOVEATED_FRAMEBUFFER
		auto& glExtensions = ::System::VideoGLExtensions::get();
		if (setup.should_use_msaa() && glExtensions.QCOM_framebuffer_foveated)
		{
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
			auto frameBufferID = !isProxy && setup.should_use_msaa() ? v3dData->renderToFrameBufferObject : v3dData->frameBufferObject;
#else
			auto frameBufferID = v3dData->frameBufferObject;
#endif
			RenderTargetUtils::update_foveated_rendering_for_framebuffer(setup.get_for_eye_idx(), frameBufferID, _setup);
		}
		else
#endif
		{
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
			auto textureID = !isProxy && setup.should_use_msaa() ? v3dData->dataTexturesData[0].renderToTextureID : v3dData->dataTexturesData[0].textureID;
#else
			auto textureID = v3dData->dataTexturesData[0].textureID;
#endif
			RenderTargetUtils::update_foveated_rendering_for_texture(setup.get_for_eye_idx(), textureID, _setup);
		}
	}
}

void RenderTarget::load_vertex_data(RenderTargetVertexBufferUsage& _vbu, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Range2 const & _uvRange, Camera const & _camera)
{
	float fov = _camera.get_fov();
	Vector2 cameraViewCentreOffset = _camera.get_view_centre_offset();
	float aspectRatio = _camera.get_view_aspect_ratio();
	if (! _vbu.loadedWithoutCamera &&
		_vbu.loadedLeftBottom == _leftBottom &&
		_vbu.loadedSize == _size &&
		_vbu.loadedUVRange == _uvRange &&
		_vbu.loadedFOV == fov &&
		_vbu.loadedAspectRatio == aspectRatio &&
		_vbu.loadedCameraViewCentreOffset == cameraViewCentreOffset &&
		_vbu.loadedForScale == currentScale)
	{
		return;
	}
	Vector3 fovCorner;
	fovCorner.z = 1.0f;
	fovCorner.y = fovCorner.z * tan_deg(fov / 2.0f);
	fovCorner.x = fovCorner.y * aspectRatio;
	Vector2 viewCentreOffset = cameraViewCentreOffset * Vector2(fovCorner.x, fovCorner.x);
	// 3 location, 2 uv, 2 view ray
	float vertexData[] = { _leftBottom.x, _leftBottom.y, 0.0f,						_uvRange.x.min * currentScale, _uvRange.y.min * currentScale, -fovCorner.x - viewCentreOffset.x, -fovCorner.y - viewCentreOffset.y,
						   _leftBottom.x, _leftBottom.y + _size.y, 0.0f,			_uvRange.x.min * currentScale, _uvRange.y.max * currentScale, -fovCorner.x - viewCentreOffset.x,  fovCorner.y - viewCentreOffset.y,
						   _leftBottom.x + _size.x, _leftBottom.y + _size.y, 0.0f,	_uvRange.x.max * currentScale, _uvRange.y.max * currentScale,  fovCorner.x - viewCentreOffset.x,  fovCorner.y - viewCentreOffset.y,
						   _leftBottom.x + _size.x, _leftBottom.y, 0.0f,			_uvRange.x.max * currentScale, _uvRange.y.min * currentScale,  fovCorner.x - viewCentreOffset.x, -fovCorner.y - viewCentreOffset.y };
	int dataSize = sizeof(float) * 4 * (3 + 2 + 2);
#ifdef AN_SYSTEM_INFO
	::System::TimeStamp ts;
#endif
	// NOTE: at this point VAO should be already bound but to load data we need to bind buffer explicitly
	Video3D::get()->bind_and_send_vertex_buffer_to_load_data(_vbu.vertexBufferObject);
	if (_vbu.dataLoaded)
	{
		DIRECT_GL_CALL_ glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, vertexData); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		DIRECT_GL_CALL_ glBufferData(GL_ARRAY_BUFFER, dataSize, vertexData, GL_DYNAMIC_DRAW); AN_GL_CHECK_FOR_ERROR
		gpu_memory_info_allocated(GPU_MEMORY_INFO_TYPE_BUFFER, _vbu.vertexBufferObject, dataSize);
		_vbu.dataLoaded = true;
	}
#ifdef AN_SYSTEM_INFO
	System::SystemInfo::loaded_vertex_buffer(dataSize, ts.get_time_since());
#endif
	_vbu.loadedWithoutCamera = false;
	_vbu.loadedLeftBottom = _leftBottom;
	_vbu.loadedSize = _size;
	_vbu.loadedUVRange = _uvRange;
	_vbu.loadedFOV = fov;
	_vbu.loadedAspectRatio = aspectRatio;
	_vbu.loadedCameraViewCentreOffset = cameraViewCentreOffset;
	_vbu.loadedForScale = currentScale;
}

void RenderTarget::load_vertex_data(RenderTargetVertexBufferUsage& _vbu, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Range2 const& _uvRange)
{
	if (_vbu.loadedWithoutCamera &&
		_vbu.loadedLeftBottom == _leftBottom &&
		_vbu.loadedSize == _size &&
		_vbu.loadedUVRange == _uvRange &&
		_vbu.loadedForScale == currentScale)
	{
		return;
	}
	float vertexData[] = { _leftBottom.x, _leftBottom.y, 0.0f,						_uvRange.x.min * currentScale, _uvRange.y.min * currentScale, 0.0f, 0.0f,
						   _leftBottom.x, _leftBottom.y + _size.y, 0.0f,			_uvRange.x.min * currentScale, _uvRange.y.max * currentScale, 0.0f, 0.0f,
						   _leftBottom.x + _size.x, _leftBottom.y + _size.y, 0.0f,	_uvRange.x.max * currentScale, _uvRange.y.max * currentScale, 0.0f, 0.0f,
						   _leftBottom.x + _size.x, _leftBottom.y, 0.0f,			_uvRange.x.max * currentScale, _uvRange.y.min * currentScale, 0.0f, 0.0f };
	int dataSize = sizeof(float) * 4 * (3 + 2 + 2);
#ifdef AN_SYSTEM_INFO
	::System::TimeStamp ts;
#endif
	// NOTE: at this point VAO should be already bound but to load data we need to bind buffer explicitly
	Video3D::get()->bind_and_send_vertex_buffer_to_load_data(_vbu.vertexBufferObject);
	if (_vbu.dataLoaded)
	{
		DIRECT_GL_CALL_ glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, vertexData); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		DIRECT_GL_CALL_ glBufferData(GL_ARRAY_BUFFER, dataSize, vertexData, GL_DYNAMIC_DRAW); AN_GL_CHECK_FOR_ERROR
		gpu_memory_info_allocated(GPU_MEMORY_INFO_TYPE_BUFFER, _vbu.vertexBufferObject, dataSize);
		_vbu.dataLoaded = true;
	}
#ifdef AN_SYSTEM_INFO
	System::SystemInfo::loaded_vertex_buffer(dataSize, ts.get_time_since());
#endif
	_vbu.loadedWithoutCamera = true;
	_vbu.loadedLeftBottom = _leftBottom;
	_vbu.loadedSize = _size;
	_vbu.loadedUVRange = _uvRange;
	_vbu.loadedForScale = currentScale;
}

RenderTarget::~RenderTarget()
{
	close(false /* we're not sure */);

	if (auto* v3d = Video3D::get()) // may be gone during quick exit
	{
		v3d->remove(this);
	}
}

void RenderTarget::close(bool _fromRenderingThread)
{
#ifdef DEBUG_RENDER_TARGET_CREATION
	v3dData->size = size;
#endif
	v3dData->isProxy = isProxy;
	v3dData->shouldUseMSAA = setup.should_use_msaa();

	if (_fromRenderingThread)
	{
		v3dData->close();
	}
	else
	{
		if (auto* v3d = Video3D::get()) // may be gone during quick exit
		{
			v3d->defer_render_target_data_to_destroy(v3dData.get());
		}
	}
}

// because there is only colour attachment 0-7
#define MAX_USE_COUNT 8

bool RenderTarget::is_bound() const
{
	return boundRenderTarget == this;
}

void RenderTarget::bind()
{
	an_assert(! is_bound());

#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	FrameBufferObjectID useFrameBufferObject = !isProxy && setup.should_use_msaa() ? v3dData->renderToFrameBufferObject : v3dData->frameBufferObject;
#else
	FrameBufferObjectID useFrameBufferObject = v3dData->frameBufferObject;
#endif

	if (isDynamicProxy)
	{
		if (v3dData->depthStencilTexture != 0)
		{
			RenderTargetUtils::attach_colour_texture_and_depth_stencil_texture_to_frame_buffer(v3dData->frameBufferObject, v3dData->dataTexturesData[0].textureID, setup.get_msaa_samples(), v3dData->depthStencilTexture, DepthStencilFormat::has_stencil(setup.get_depth_stencil_format()));
		}
		else
		{
			RenderTargetUtils::attach_colour_texture_and_depth_stencil_render_buffer_to_frame_buffer(v3dData->frameBufferObject, v3dData->dataTexturesData[0].textureID, setup.get_msaa_samples(), v3dData->depthStencilBufferObject, DepthStencilFormat::has_stencil(setup.get_depth_stencil_format()));
		}
	}

#ifdef AN_GL
	// implicit for gles
	Video3D::get()->set_multi_sample(setup.should_use_msaa());
#endif
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	resolved = isProxy || (!setup.should_use_msaa() && !withMipMaps); // will require resolving if using msaa
#else
	resolved = isProxy || !withMipMaps;
#endif
	an_assert(is_valid(), TXT("make sure that render target is valid before using it"));
	Video3D::get()->bind_frame_buffer(useFrameBufferObject);
	Video3D::get()->set_srgb_conversion(setup.should_use_srgb_conversion());
	GLenum buffers[MAX_USE_COUNT];
	GLenum* buffer = buffers;
	int usedCount = 0;
	int index = 0;
	for_every_const(dataTextureData, v3dData->dataTexturesData)
	{
		if (dataTextureData->required)
		{
			an_assert(usedCount < MAX_USE_COUNT);
			*buffer = GL_COLOR_ATTACHMENT0 + index;
			++buffer;
			++usedCount;
		}
		++index;
	}
	an_assert(usedCount < MAX_USE_COUNT);
	DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDrawBuffers(usedCount, buffers); AN_GL_CHECK_FOR_ERROR

	boundRenderTarget = this;
}

#undef MAX_USE_COUNT

void RenderTarget::unbind()
{
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	an_assert_immediate(Video3D::get()->is_frame_buffer_bound(! isProxy && setup.should_use_msaa() ? v3dData->renderToFrameBufferObject : v3dData->frameBufferObject));
#else
	an_assert_immediate(Video3D::get()->is_frame_buffer_bound(v3dData->frameBufferObject));
#endif
	an_assert(is_bound());
	bind_none();
}

void RenderTarget::bind_none()
{
	if (auto* rt = boundRenderTarget.get())
	{
		if (rt->isDynamicProxy)
		{
			if (rt->v3dData->depthStencilTexture != 0)
			{
				RenderTargetUtils::detach_colour_texture_and_depth_stencil_texture_from_frame_buffer(rt->v3dData->frameBufferObject, rt->setup.get_msaa_samples(), DepthStencilFormat::has_stencil(rt->setup.get_depth_stencil_format()));
			}
			else
			{
				RenderTargetUtils::detach_colour_texture_and_depth_stencil_render_buffer_from_frame_buffer(rt->v3dData->frameBufferObject, rt->setup.get_msaa_samples(), DepthStencilFormat::has_stencil(rt->setup.get_depth_stencil_format()));
			}
		}
	}

	Video3D::get()->unbind_frame_buffer();
	boundRenderTarget = nullptr;
}

VectorInt2 RenderTarget::get_full_size_for_aspect_ratio_coef(bool _trueNotScaled) const
{
	VectorInt2 vs = get_size(_trueNotScaled);
	float aspectRatioCoef = get_setup().get_aspect_ratio_coef();
	vs = get_full_for_aspect_ratio_coef(vs, aspectRatioCoef);
	return vs;
}

void RenderTarget::generate_mip_maps()
{
	if (!withMipMaps)
	{
		return;
	}
	assert_rendering_thread();
	an_assert(is_valid(), TXT("make sure that render target is valid before using it"));
	Video3D::get()->requires_send_shader_program(); // to make sure that shader is not bound
	an_assert(!Video3D::get()->is_shader_bound(), TXT("unbind shader and do not resolve when a shader is bound!"));
	for_every(dataTextureData, v3dData->dataTexturesData)
	{
		/*
		 * this didn't work. most likely because it really required setting data texture
		 * moved to direct gl calls
		 * Video3D::get()->mark_use_texture(0, dataTextureData->textureID);
		 * Video3D::get()->force_send_use_texture_slot(0);
		 * Video3D::get()->requires_send_use_textures();
		 */
		DIRECT_GL_CALL_ glActiveTexture(GL_TEXTURE0 + 0); AN_GL_CHECK_FOR_ERROR
		DIRECT_GL_CALL_ glBindTexture(GL_TEXTURE_2D, dataTextureData->textureID); AN_GL_CHECK_FOR_ERROR
		DIRECT_GL_CALL_ glGenerateMipmap(GL_TEXTURE_2D); AN_GL_CHECK_FOR_ERROR
	}

	// synchronise texture id so following calls will not miss texture change
	DIRECT_GL_CALL_ glBindTexture(GL_TEXTURE_2D, texture_id_none()); AN_GL_CHECK_FOR_ERROR
	Video3D::get()->mark_use_texture(0, texture_id_none());
	Video3D::get()->mark_use_no_textures();
	Video3D::get()->requires_send_use_textures();
	// TODO depth stencil - do we need it?
}

void RenderTarget::set_data_for_dynamic_proxy(RenderTargetProxyData const& _proxyData)
{
	if (is_bound())
	{
		error(TXT("can't change data if render target is bound, even to the same (check before changing if bound!)"));
		return;
	}

	if (!isDynamicProxy)
	{
		error(TXT("do not use with non dynamic proxy!"));
		return;
	}

	an_assert(v3dData->dataTexturesData.get_size() == 1);
	v3dData->dataTexturesData[0].textureID = _proxyData.colourTexture;
	v3dData->frameBufferObject = _proxyData.frameBufferObject;
	v3dData->depthStencilTexture = _proxyData.depthStencilTexture;
	v3dData->depthStencilBufferObject = _proxyData.depthStencilBufferObject;
}

void RenderTarget::copy_to(RenderTarget* dest)
{
	auto* src = this;

	::System::Video3D* v3d = ::System::Video3D::get();

#ifdef AN_GL
	// implicit for gles
	v3d->set_multi_sample(false);
#endif
	v3d->set_srgb_conversion(false);

	an_assert(!v3d->is_frame_buffer_bound(), TXT("unbind frame buffer and do not resolve when buffer is bound!"));
	v3d->unbind_frame_buffer_bare();
	an_assert(v3d->is_frame_buffer_unbound_bare(), TXT("unbind frame buffer and do not resolve when buffer is bound!"));
	// has to be unbound as otherwise blit may blit onto frame buffer and we do not want that to happen

	DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, src->get_frame_buffer_object_id_to_read()); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->get_frame_buffer_object_id_to_write()); AN_GL_CHECK_FOR_ERROR

	VectorInt2 srcSize = src->get_size();
	VectorInt2 destSize = dest->get_size();
	for_count(int, dt, src->get_data_texture_count())
	{
		DIRECT_GL_CALL_ glReadBuffer(GL_COLOR_ATTACHMENT0 + dt); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
		DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDrawBuffer(GL_COLOR_ATTACHMENT0 + dt); AN_GL_CHECK_FOR_ERROR
#endif
#ifdef AN_GLES
		{
			GLenum bufs = GL_COLOR_ATTACHMENT0 + dt;
			DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDrawBuffers(1, &bufs); AN_GL_CHECK_FOR_ERROR
		}
#endif
		DRAW_CALL_RENDER_TARGET_DRAW_ DIRECT_GL_CALL_ glBlitFramebuffer(0, 0, srcSize.x, srcSize.y, 0, 0, destSize.x, destSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
	}

	DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR

	v3d->invalidate_bound_frame_buffer_info();
	v3d->unbind_frame_buffer(); // to get back from bare

#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	dest->resolved = dest->isProxy || (!dest->setup.should_use_msaa() && !dest->withMipMaps); // will require resolving if using msaa
#else
	dest->resolved = dest->isProxy || !dest->withMipMaps;
#endif
}

void RenderTarget::render_fit(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const& _leftBottom, SIZE_ Vector2 const& _size, Optional<Vector2> const & _alignPT, Optional<bool> const& _zoomX, Optional<bool> const& _zoomY)
{
	VectorInt2 rtSize = get_full_size_for_aspect_ratio_coef();
	VectorInt2 availableSize = _size.to_vector_int2();
	Vector2 alignPT = _alignPT.get(Vector2::half);
	bool zoomX = _zoomX.get(false);
	bool zoomY = _zoomY.get(false);

	Vector2 leftBottom = _leftBottom;
	Vector2 size = _size;
	if (!zoomX && ! zoomY)
	{
		Vector2 lb;
		Vector2 s;
		RenderTargetUtils::fit_into(rtSize, availableSize, OUT_ lb, OUT_ s);
		size = s;
	}
	else if (!zoomX && zoomY)
	{
		// x has to match, y may be bigger
		Vector2 lb;
		Vector2 s;
		RenderTargetUtils::fit_into(rtSize, VectorInt2(availableSize.x, availableSize.x * rtSize.y / rtSize.x), OUT_ lb, OUT_ s);
		size = s;
	}
	else if (zoomX && ! zoomY)
	{
		// y has to match, x may be bigger
		Vector2 lb;
		Vector2 s;
		RenderTargetUtils::fit_into(rtSize, VectorInt2(availableSize.y * rtSize.x / rtSize.y, availableSize.y), OUT_ lb, OUT_ s);
		size = s;
	}
	else
	{
		// has to fill whole thing
		Vector2 lbx, lby;
		Vector2 sx, sy;
		RenderTargetUtils::fit_into(rtSize, VectorInt2(availableSize.x, availableSize.x * rtSize.y / rtSize.x), OUT_ lbx, OUT_ sx);
		RenderTargetUtils::fit_into(rtSize, VectorInt2(availableSize.y * rtSize.x / rtSize.y, availableSize.y), OUT_ lby, OUT_ sy);

		if (sx.x * sx.y > sy.x * sy.y)
		{
			size = sx;
		}
		else
		{
			size = sy;
		}
	}
	leftBottom = leftBottom + availableSize.to_vector2() * 0.5f - size * 0.5f;

	Range2 uvRange = wholeUVRange;
	Vector2 av_lb = _leftBottom;
	Vector2 av_rt = _leftBottom + _size;
	Vector2 cr_lb = leftBottom;
	Vector2 cr_rt = leftBottom + size;
	if (cr_lb.x < av_lb.x)
	{
		uvRange.x.min += (av_lb.x - cr_lb.x) / (cr_rt.x - cr_lb.x);
	}
	if (cr_rt.x > av_rt.x)
	{
		uvRange.x.max -= (cr_rt.x - av_rt.x) / (cr_rt.x - cr_lb.x);
	}
	if (cr_lb.y < av_lb.y)
	{
		uvRange.y.min += (av_lb.y - cr_lb.y) / (cr_rt.y - cr_lb.y);
	}
	if (cr_rt.y > av_rt.y)
	{
		uvRange.y.max -= (cr_rt.y - av_rt.y) / (cr_rt.y - cr_lb.y);
	}

	Vector2 topRight;

	leftBottom.x = lerp(uvRange.x.min, cr_lb.x, cr_rt.x);
	topRight.x = lerp(uvRange.x.max, cr_lb.x, cr_rt.x);
	leftBottom.y = lerp(uvRange.y.min, cr_lb.y, cr_rt.y);
	topRight.y = lerp(uvRange.y.max, cr_lb.y, cr_rt.y);

	// use alignment if is smaller than the size available
	if (topRight.x - leftBottom.x < _size.x && alignPT.x != 0.5f)
	{
		float minV = 0.0f;
		float maxV = _size.x - (topRight.x - leftBottom.x);
		float off = lerp(alignPT.x, minV, maxV) - (minV + maxV) * 0.5f;
		leftBottom.x += off;
		topRight.x += off;
	}
	if (topRight.y - leftBottom.y < _size.y && alignPT.y != 0.5f)
	{
		float minV = 0.0f;
		float maxV = _size.y - (topRight.y - leftBottom.y);
		float off = lerp(alignPT.y, minV, maxV) - (minV + maxV) * 0.5f;
		leftBottom.y += off;
		topRight.y += off;
	}

	render(_dataTextureIndex, _v3d, leftBottom, topRight - leftBottom, uvRange);
}

void RenderTarget::render(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const& _uvRange)
{
	assert_rendering_thread();
	// disable clip planes
	an_assert(is_valid(), TXT("make sure that render target is valid before using it"));
	an_assert(is_resolved(), TXT("please resolve this render target before using it"));

	if (ShaderProgram* shaderProgram = _v3d->get_fallback_shader_program(::System::Video3DFallbackShader::For3D | ::System::Video3DFallbackShader::WithTexture))
	{
		render(_dataTextureIndex, _v3d, _leftBottom, _size, _uvRange, shaderProgram);
	}
}

void RenderTarget::render(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const& _uvRange, Camera const & _camera, ShaderProgram* _shaderProgram)
{
	assert_rendering_thread();
	// disable clip planes
	an_assert(is_valid(), TXT("make sure that render target is valid before using it"));
	an_assert(is_resolved(), TXT("please resolve this render target before using it"));

	// force to show
	_v3d->setup_for_2d_display();

	_shaderProgram->bind();
	_shaderProgram->set_build_in_uniform_in_texture(get_data_texture_id(_dataTextureIndex));
	_shaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(get_size(true).to_vector2(), _size);

	render_for_shader_program(_v3d, _leftBottom, _size, _uvRange, _camera, _shaderProgram);

	_shaderProgram->unbind();
}

void RenderTarget::render(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const& _uvRange, ShaderProgram* _shaderProgram)
{
	assert_rendering_thread();
	// disable clip planes?
	an_assert(is_valid(), TXT("make sure that render target is valid before using it"));
	an_assert(is_resolved(), TXT("please resolve this render target before using it"));

	// force to show
	_v3d->setup_for_2d_display();

	_shaderProgram->bind();
	_shaderProgram->set_build_in_uniform_in_texture(get_data_texture_id(_dataTextureIndex));
	_shaderProgram->set_build_in_uniform_in_texture_size_related_uniforms(get_size(true).to_vector2(), _size);

	render_for_shader_program(_v3d, _leftBottom, _size, _uvRange, _shaderProgram);

	_shaderProgram->unbind();
}

RenderTargetVertexBufferUsage& RenderTarget::get_vertex_buffer_usage(int _subIndex)
{
	while (v3dData->vertexBufferUsages.get_size() <= _subIndex)
	{
		RenderTargetVertexBufferUsage vbu;
		vbu.loadedFOV = 0.0f;
		DIRECT_GL_CALL_ glGenBuffers(1, &vbu.vertexBufferObject); AN_GL_CHECK_FOR_ERROR
		vbu.dataLoaded = false;
#ifdef AN_SYSTEM_INFO
		System::SystemInfo::created_vertex_buffer();
#endif
		v3dData->vertexBufferUsages.push_back(vbu);
	}
	return v3dData->vertexBufferUsages[_subIndex];
}

void RenderTarget::render_for_shader_program(Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const & _uvRange, Camera const & _camera, ShaderProgram* _shaderProgram)
{
	assert_rendering_thread();
	an_assert(is_resolved(), TXT("please resolve this render target before using it"));

	RenderTargetVertexBufferUsage& vbu = get_vertex_buffer_usage(_camera.get_eye_idx() + 1);
	_v3d->bind_and_send_buffers_and_vertex_format(vbu.vertexBufferObject, 0, &v3dData->vertexFormat, _shaderProgram, nullptr);
	_v3d->ready_for_rendering();
	load_vertex_data(vbu, _leftBottom, _size, _uvRange.get(wholeUVRange), _camera);
	DRAW_CALL_RENDER_TARGET_ DIRECT_GL_CALL_ glDrawArrays(GL_TRIANGLE_FAN, 0, 4); AN_GL_CHECK_FOR_ERROR
	_v3d->soft_unbind_buffers_and_vertex_format();
}

void RenderTarget::render_for_shader_program(Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const& _uvRange, ShaderProgram* _shaderProgram)
{
	assert_rendering_thread();

	RenderTargetVertexBufferUsage& vbu = get_vertex_buffer_usage(0);
	_v3d->bind_and_send_buffers_and_vertex_format(vbu.vertexBufferObject, 0, &v3dData->vertexFormat, _shaderProgram, nullptr);
	_v3d->ready_for_rendering();
	load_vertex_data(vbu, _leftBottom, _size, _uvRange.get(wholeUVRange));
	DRAW_CALL_RENDER_TARGET_ DIRECT_GL_CALL_ glDrawArrays(GL_TRIANGLE_FAN, 0, 4); AN_GL_CHECK_FOR_ERROR
	_v3d->soft_unbind_buffers_and_vertex_format();
}

void RenderTarget::mark_all_outputs_required()
{
	for_every(dataTextureData, v3dData->dataTexturesData)
	{
		dataTextureData->required = true;
	}
}

void RenderTarget::mark_output_not_required(int _index)
{
	an_assert(v3dData->dataTexturesData.is_index_valid(_index));
	v3dData->dataTexturesData[_index].required = false;
}

int RenderTarget::get_required_outputs_count() const
{
	int count = 0;
	for_every(dtd, v3dData->dataTexturesData)
	{
		if (dtd->required) ++count;
	}
	return count;
}

void RenderTarget::resolve_forced_unbind()
{
	if (!resolved)
	{
		unbind_to_none();
		resolve();
	}
}

void RenderTarget::resolve()
{
	if (resolved)
	{
		return;
	}
	an_assert(!isProxy, TXT("proxies should not require resolving"));

	resolved = true;
#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	if (!isProxy && setup.should_use_msaa())
	{
#ifdef AN_GL
		// implicit for gles
		Video3D::get()->set_multi_sample(false);
#endif
		Video3D::get()->set_srgb_conversion(false); 

		an_assert(!Video3D::get()->is_frame_buffer_bound(), TXT("unbind frame buffer and do not resolve when buffer is bound!"));
		Video3D::get()->unbind_frame_buffer_bare();
		an_assert(Video3D::get()->is_frame_buffer_unbound_bare(), TXT("unbind frame buffer and do not resolve when buffer is bound!"));
		// has to be unbound as otherwise blit may blit onto frame buffer and we do not want that to happen

		DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, v3dData->renderToFrameBufferObject); AN_GL_CHECK_FOR_ERROR
		DIRECT_GL_CALL_ glBindFramebuffer(GL_DRAW_FRAMEBUFFER, v3dData->frameBufferObject); AN_GL_CHECK_FOR_ERROR

		VectorInt2 useSize = get_size();
		for_count(int, dt, get_data_texture_count())
		{
			DIRECT_GL_CALL_ glReadBuffer(GL_COLOR_ATTACHMENT0 + dt); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
			DRAW_CALL_RENDER_TARGET_DRAW_ DIRECT_GL_CALL_ glDrawBuffer(GL_COLOR_ATTACHMENT0 + dt); AN_GL_CHECK_FOR_ERROR
#endif
#ifdef AN_GLES
			{
				GLenum bufs = GL_COLOR_ATTACHMENT0 + dt;
				DRAW_CALL_RENDER_TARGET_DRAW_ DIRECT_GL_CALL_ glDrawBuffers(1, &bufs); AN_GL_CHECK_FOR_ERROR
			}
#endif
			DIRECT_GL_CALL_ glBlitFramebuffer(0, 0, useSize.x, useSize.y, 0, 0, useSize.x, useSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
		}

		DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR
		DIRECT_GL_CALL_ glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR
		
		Video3D::get()->invalidate_bound_frame_buffer_info();
		Video3D::get()->unbind_frame_buffer(); // to get back from bare
	}
#endif

	if (withMipMaps)
	{
		generate_mip_maps();
	}
}

void RenderTarget::copy(RenderTarget* _source, VectorInt2 const & _srcAt, VectorInt2 const & _srcSize, RenderTarget* _dest, Optional<int> _copyFlags, Optional<VectorInt2> _destAt, Optional<VectorInt2> _destSize)
{
	if (!_destAt.is_set())
	{
		_destAt = _srcAt;
	}
	if (!_destSize.is_set())
	{
		_destSize = _srcSize;
	}
	int copyFlags = _copyFlags.get(CopyFlagsDefault);
	an_assert(_source->is_resolved());
	an_assert(!Video3D::get()->is_frame_buffer_bound(), TXT("unbind frame buffer and do not resolve when buffer is bound!"));
	Video3D::get()->unbind_frame_buffer_bare();
	an_assert(Video3D::get()->is_frame_buffer_unbound_bare(), TXT("unbind frame buffer and do not resolve when buffer is bound!"));

	// video3d states are done in resolve

	VectorInt2 slb(_srcAt.x, _srcAt.y);
	VectorInt2 srt(_srcAt.x + _srcSize.x - 1, _srcAt.y + _srcSize.y - 1);
	VectorInt2 dlb(_destAt.get().x, _destAt.get().y);
	VectorInt2 drt(_destAt.get().x + _destSize.get().x - 1, _destAt.get().y + _destSize.get().y - 1);

	DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, _source->get_frame_buffer_object_id_to_read()); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _dest->get_frame_buffer_object_id_to_write()); AN_GL_CHECK_FOR_ERROR
	for_count(int, dt, _source->get_data_texture_count())
	{
		DIRECT_GL_CALL_ glReadBuffer(GL_COLOR_ATTACHMENT0 + dt); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
		DRAW_CALL_RENDER_TARGET_DRAW_ DIRECT_GL_CALL_ glDrawBuffer(GL_COLOR_ATTACHMENT0 + dt); AN_GL_CHECK_FOR_ERROR
#endif
#ifdef AN_GLES
		{
			GLenum bufs = GL_COLOR_ATTACHMENT0 + dt;
			DRAW_CALL_RENDER_TARGET_DRAW_ DIRECT_GL_CALL_ glDrawBuffers(1, &bufs); AN_GL_CHECK_FOR_ERROR
		}
#endif
		DRAW_CALL_RENDER_TARGET_DRAW_ DIRECT_GL_CALL_ glBlitFramebuffer(slb.x, slb.y, srt.x, srt.y, dlb.x, dlb.y, drt.x, drt.y,
			((copyFlags & CopyData) ? GL_COLOR_BUFFER_BIT : 0) |
			(((copyFlags & CopyDepth) && dt == 0)? GL_DEPTH_BUFFER_BIT : 0) |
			(((copyFlags & CopyStencil) && dt == 0)? GL_STENCIL_BUFFER_BIT : 0),
			GL_NEAREST); AN_GL_CHECK_FOR_ERROR
	}
	DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR

	Video3D::get()->invalidate_bound_frame_buffer_info();
	Video3D::get()->unbind_frame_buffer(); // to get back from bare

#ifdef USE_SEPARATE_TEXTURE_TO_RENDER_FOR_MSAA
	_dest->resolved = _dest->isProxy || (!_dest->setup.should_use_msaa() && !_dest->withMipMaps); // will require resolving if using msaa
#else
	_dest->resolved = _dest->isProxy || !_dest->withMipMaps;
#endif
}

void RenderTarget::change_filtering(int _dataTextureIndex, TextureFiltering::Type _mag, TextureFiltering::Type _min)
{
	auto & dataTexture = v3dData->dataTexturesData[_dataTextureIndex];
	_mag = ::System::TextureFiltering::get_proper_for(_mag, get_setup().are_mip_maps_forced());
	_min = ::System::TextureFiltering::get_proper_for(_min, get_setup().are_mip_maps_forced());
	if (dataTexture.magFiltering == _mag &&
		dataTexture.minFiltering == _min)
	{
		return;
	}

	Video3D* v3d = Video3D::get();
	v3d->mark_use_no_textures();
	v3d->mark_use_texture(0, v3dData->dataTexturesData[_dataTextureIndex].textureID); // do it through this, so video 3d can restore it later
	v3d->force_send_use_texture_slot(0);
	v3d->requires_send_use_textures(); // because we will be setting texture filters we have to have textures loaded

	DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ::System::TextureFiltering::get_sane_mag(_mag)); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ::System::TextureFiltering::get_sane_min(_min)); AN_GL_CHECK_FOR_ERROR

	dataTexture.magFiltering = _mag;
	dataTexture.minFiltering = _min;
}

//--

RenderTargetVideo3DData::RenderTargetVideo3DData()
: frameBufferObject(0)
, renderToFrameBufferObject(0)
, depthStencilTexture(texture_id_none())
, depthStencilBufferObject(0)
{
}

void RenderTargetVideo3DData::close()
{
#ifdef DEBUG_RENDER_TARGET_CREATION
	output(TXT("[render-target] close %ix%i"), size.x, size.y);
#endif

	assert_rendering_thread();
	
	if (!isProxy) // proxies have to clean actual render targets somewhere else, they are proxies, right?
	{
		if (!dataTexturesData.is_empty())
		{
			for_every(dataTextureData, dataTexturesData)
			{
				DIRECT_GL_CALL_ glDeleteTextures(1, &dataTextureData->textureID); AN_GL_CHECK_FOR_ERROR
#ifdef DEBUG_RENDER_TARGET_MEMORY
				render_target_info_free_memory(GPU_MEMORY_INFO_TYPE_TEXTURE, dataTextureData->textureID, size, dataTextureData->bytesPerPixel);
#endif
				if (dataTextureData->renderToTextureID != 0)
				{
					DIRECT_GL_CALL_ glDeleteTextures(1, &dataTextureData->renderToTextureID); AN_GL_CHECK_FOR_ERROR

#ifdef DEBUG_RENDER_TARGET_MEMORY
					render_target_info_free_memory(GPU_MEMORY_INFO_TYPE_TEXTURE, dataTextureData->renderToTextureID, size, dataTextureData->bytesPerPixel);
#endif
				}
			}
			dataTexturesData.clear();
		}
		if (depthStencilTexture != texture_id_none())
		{
			DIRECT_GL_CALL_ glDeleteTextures(1, &depthStencilTexture); AN_GL_CHECK_FOR_ERROR
#ifdef DEBUG_RENDER_TARGET_MEMORY
			render_target_info_free_memory(GPU_MEMORY_INFO_TYPE_TEXTURE, depthStencilTexture, size, 4);
#endif
			depthStencilTexture = texture_id_none();
		}
		if (depthStencilBufferObject != 0)
		{
			DIRECT_GL_CALL_ glDeleteRenderbuffers(1, &depthStencilBufferObject); AN_GL_CHECK_FOR_ERROR
#ifdef DEBUG_RENDER_TARGET_MEMORY
			render_target_info_free_memory(GPU_MEMORY_INFO_TYPE_RENDER_BUFFER, depthStencilBufferObject, size, 4);
#endif
			depthStencilBufferObject = 0;
		}
		if (frameBufferObject != 0)
		{
			DIRECT_GL_CALL_ glDeleteFramebuffers(1, &frameBufferObject); AN_GL_CHECK_FOR_ERROR
			frameBufferObject = 0;
#ifdef AN_SYSTEM_INFO
			::System::SystemInfo::destroyed_render_target();
#endif
		}
		if (renderToFrameBufferObject != 0)
		{
			DIRECT_GL_CALL_ glDeleteFramebuffers(1, &renderToFrameBufferObject); AN_GL_CHECK_FOR_ERROR
			renderToFrameBufferObject = 0;
		}
	}
	for_every(vbu, vertexBufferUsages)
	{
		DIRECT_GL_CALL_ glDeleteBuffers(1, &vbu->vertexBufferObject); AN_GL_CHECK_FOR_ERROR
		gpu_memory_info_freed(GPU_MEMORY_INFO_TYPE_BUFFER, vbu->vertexBufferObject);
		if (auto* v3d = Video3D::get())
		{
			v3d->on_vertex_buffer_destroyed(vbu->vertexBufferObject);
		}
#ifdef AN_SYSTEM_INFO
		System::SystemInfo::destroyed_vertex_buffer();
#endif
	}
	vertexBufferUsages.clear();
}

//--

ForRenderTargetOrNone& ForRenderTargetOrNone::bind()
{
	if (rt)
	{
		rt->bind();
	}
	else
	{
		RenderTarget::bind_none();
	}

	return *this;
}

ForRenderTargetOrNone& ForRenderTargetOrNone::unbind()
{
	if (rt)
	{
		rt->unbind();
	}
	else
	{
		RenderTarget::unbind_to_none();
	}

	return *this;
}

ForRenderTargetOrNone & ForRenderTargetOrNone::set_viewport()
{
	if (rt)
	{
		Video3D::get()->set_viewport(rt->get_size());
	}
	else
	{
		Video3D::get()->set_default_viewport();
	}

	return *this;
}

ForRenderTargetOrNone & ForRenderTargetOrNone::set_viewport(RangeInt2 const & _viewport)
{
	if (_viewport.is_empty())
	{
		set_viewport();
	}
	else
	{
		Video3D::get()->set_viewport(_viewport.bottom_left(), _viewport.length());
	}

	return *this;
}

VectorInt2 ForRenderTargetOrNone::get_size()
{
	if (rt)
	{
		return rt->get_size();
	}
	else
	{
		return Video3D::get()->get_screen_size();
	}
}

float ForRenderTargetOrNone::get_full_size_aspect_ratio()
{
	if (rt)
	{
		return aspect_ratio(rt->get_full_size_for_aspect_ratio_coef());
	}
	else
	{
		return aspect_ratio(Video3D::get()->get_screen_size());
	}
}
