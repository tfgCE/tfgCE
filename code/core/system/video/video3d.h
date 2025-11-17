#pragma once

#include "..\..\functionParamsStruct.h"

#include "..\..\concurrency\threadManager.h" // due to inlined debug

#include "..\..\serialisation\serialiserBasics.h"

#include "video.h"
#include "videoEnums.h"
#include "videoMatrixStack.h"
#include "videoClipPlaneStack.h"
#include "videoGLExtensions.h"

#include "shader.h"

#include "..\timeStamp.h"

// .inl
#include "..\..\types\colour.h"
#include "..\..\debug\debug.h"

#ifdef AN_ANDROID
#include "eglContext.h"
#endif

#ifdef WITH_GL_CHECK_FOR_ERROR
namespace System
{
	void gl_report_error(int glError, char const* file, int line);
};

#define AN_GL_CHECK_FOR_ERROR_ALWAYS \
		{ \
			while (true) \
			{ \
				int glResult = glGetError(); \
				if (glResult == 0) \
				{ \
					break; \
				} \
				else \
				{ \
					::System::gl_report_error(glResult, __FILE__, __LINE__); \
				} \
			} \
		}

#ifdef WITH_GL_CHECK_FOR_ERROR_ALL
	#define AN_GL_CHECK_FOR_ERROR \
		{ \
			while (true) \
			{ \
				int glResult = glGetError(); \
				if (glResult == 0) \
				{ \
					break; \
				} \
				else \
				{ \
					::System::gl_report_error(glResult, __FILE__, __LINE__); \
				} \
			} \
		}
	#define AN_GL_CHECK_FOR_ERROR_IGNORE \
		{ \
			while (true) \
			{ \
				int glResult = glGetError(); \
				if (glResult == 0) \
				{ \
					break; \
				} \
				else \
				{ \
					/* ignore */ \
				} \
			} \
		}
	#endif
#endif

#ifndef AN_GL_CHECK_FOR_ERROR_ALWAYS
	#define AN_GL_CHECK_FOR_ERROR_ALWAYS
#endif

#ifndef AN_GL_CHECK_FOR_ERROR
	#define AN_GL_CHECK_FOR_ERROR
#endif

#ifndef AN_GL_CHECK_FOR_ERROR_IGNORE
	#define AN_GL_CHECK_FOR_ERROR_IGNORE
#endif


#define glFlushComment(...) \
	output(__VA_ARGS__); \
	DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR \
	output(TXT("[flush done]"));

//

class Serialiser;

namespace System
{
	class ShaderProgram;
	class ShaderProgramCache;
	class RenderTarget;
	struct VertexFormat;
	struct ShaderProgramInstance;
	struct RenderTargetVideo3DData;

	namespace Video3DCompareFunc
	{
		enum Type
		{
			None			=	0,
			//
			Never			=	GL_NEVER,
			Less			=	GL_LESS,
			Equal			=	GL_EQUAL,
			LessEqual		=	GL_LEQUAL,
			Greater			=	GL_GREATER,
			NotEqual		=	GL_NOTEQUAL,
			GreaterEqual	=	GL_GEQUAL,
			Always			=	GL_ALWAYS,
		};

		inline bool parse(String const & _string, REF_ Type & _func)
		{
			if (_string == TXT("never")) { _func = Never; return true; }
			if (_string == TXT("less")) { _func = Less; return true; }
			if (_string == TXT("equal")) { _func = Equal; return true; }
			if (_string == TXT("lessEqual")) { _func = LessEqual; return true; }
			if (_string == TXT("greater")) { _func = Greater; return true; }
			if (_string == TXT("notEqual")) { _func = NotEqual; return true; }
			if (_string == TXT("greaterEqual")) { _func = GreaterEqual; return true; }
			if (_string == TXT("always")) { _func = Always; return true; }
			return _string.is_empty();
		}
	};

	namespace Video3DOp
	{
		enum Type
		{
			Keep			=	GL_KEEP,
			Zero			=	GL_ZERO,
			Replace			=	GL_REPLACE,
			Increase		=	GL_INCR,
			IncreaseWrap	=	GL_INCR_WRAP,
			Decrease		=	GL_DECR,
			DecreaseWrap	=	GL_DECR_WRAP,
			InvertRect		=	GL_INVERT,
		};
	};

	namespace FaceOrder
	{
		enum Type
		{
			CW				=	GL_CW,
			CCW				=	GL_CCW,
		};
	};

	namespace FaceDisplay // display as opposite to cull
	{
		enum Type
		{
			None			=	GL_FRONT_AND_BACK,
			Front			=	GL_BACK,
			Back			=	GL_FRONT,
			Both			=	0,
		};
		
		inline bool parse(String const & _string, REF_ Type & _faceDisplay)
		{
			if (_string == TXT("none")) { _faceDisplay = None; return true; }
			if (_string == TXT("front")) { _faceDisplay = Front; return true; }
			if (_string == TXT("back")) { _faceDisplay = Back; return true; }
			if (_string == TXT("both")) { _faceDisplay = Both; return true; }
			return _string.is_empty();
		}
	};

#ifdef AN_GL
	namespace ShadeModel
	{
		enum Type
		{
			Flat			=	GL_FLAT,
			Smooth			=	GL_SMOOTH,
		};
	};
#endif

	namespace Video3DFallbackShader
	{
		enum Flag
		{
			For3D = 0,
			For2D = 1,
			WithTexture = 2,
			WithoutTexture = 0,
			DiscardsBlending = 4,
			Basic = 8, // 8(3d) + 9(2d)
			COUNT = 10,
		};
		typedef int Flags;
	};

	struct Video3DStateStore
	{
		bool blendStored = false;
		bool blendEnabled;
		BlendOp::Type blendSrcFactor;
		BlendOp::Type blendDestFactor;

		bool depthDisplayStored = false;
		bool depthDisplay;

		bool depthFuncStored = false;
		Video3DCompareFunc::Type depthFunc;

		bool depthClampStored = false;
		bool depthClamp;

		bool polygonOffsetStored = false;
		bool polygonOffsetEnabled;
		float polygonOffsetFactor;
		float polygonOffsetUnits;
	};

	class Video3D
	: public Video
	{
	public:
		typedef std::function<void(ShaderProgram* _shaderProgram)> CustomSetBuildInUniforms;

	public:
		static void create(VideoConfig const * _vc = nullptr);
		static void terminate();
		static Video3D* get() { return s_current; }

#ifdef AN_ANDROID
		EGLContext* get_gl_context() const { return glContext; }
#endif

		bool is_ok();

		bool init(VideoConfig const & vc);
		void init_fallback_shaders();
		void close();

		void update();

#ifdef AN_DEVELOPMENT
		void set_load_matrices(bool _loadMatrices) { loadMatrices = _loadMatrices; }
#endif

		bool is_for_3d() const { return isFor3d; }
		bool should_use_vec4_arrays_instead_of_mat4_arrays() const { return useVec4ArraysInsteadOfMat4Arrays; }
		bool has_limited_support_for_indexing_in_shaders() const { return limitedSupportForIndexingInShaders; }

		void skip_display_buffer_for_VR(bool _skip = true) { skipDisplayBufferForVR = _skip; }
		bool should_skip_display_buffer_for_VR() const { return skipDisplayBufferForVR; }

		bool should_use_shader_program_cache() const { return useShaderProgramCache; }
		ShaderProgramCache& access_shader_program_cache() { an_assert(shaderProgramCache); return *shaderProgramCache; }

		void add_custom_set_build_in_uniforms(Name const& _id, Array<Name> const & _customIsBuildInUniforms, CustomSetBuildInUniforms _custom_set_build_in_uniforms);
		void remove_custom_set_build_in_uniforms(Name const& _id);
		void apply_custom_set_build_in_uniforms_for(ShaderProgram* _shaderProgram);
		bool is_custom_build_in_uniform(Name const & _shaderParam) const;

		// individual offsets (to be used by vertex shaders)
		void set_current_individual_offset(float _currentIndividualOffset) { currentIndividualOffset = _currentIndividualOffset; }
		float get_current_individual_offset() const { return currentIndividualOffset; }

		// clearing
		inline void clear_colour(Colour const & _colour);
		inline void clear_depth(float _depth = 1.0f);
		inline void clear_stencil(int32 _stencil = 0);
		inline void clear_colour_depth(Colour const & _colour, float _depth = 1.0f);
		inline void clear_colour_depth_stencil(Colour const & _colour, float _depth = 1.0f, int32 _stencil = 0);
		inline void clear_depth_stencil(float _depth = 1.0f, int32 _stencil = 0);

		inline void set_fallback_texture(TextureID _texture = texture_id_none()) { fallbackTexture = _texture; }

		inline void set_fallback_colour(Colour const & _colour = Colour::full) { fallbackColour = _colour; }
		inline Colour const & get_fallback_colour() const { return fallbackColour; }

		// just for info, to provide for shaders
		inline void set_eye_offset(Vector3 const & _eyeOffset = Vector3::zero) { eyeOffset = _eyeOffset; }
		inline Vector3 const & get_eye_offset() const { return eyeOffset; }

 		// states
		inline void push_state();
		inline void pop_state();

		// bind buffers or no buffers
		void bind_and_send_vertex_buffer_to_load_data(BufferObjectID _vertexBufferObject);
		void bind_and_send_element_buffer_to_load_data(BufferObjectID _elementBufferObject);
		void bind_and_send_buffers_and_vertex_format(BufferObjectID _vertexBufferObject = 0, BufferObjectID _elementBufferObject = 0, VertexFormat const * _vertexFormat = nullptr, ShaderProgram * _shaderProgram = nullptr, void const * _dataPointer = nullptr);
		void soft_unbind_buffers_and_vertex_format(); // does not send
		bool unbind_and_send_buffers_and_vertex_format();
		inline void invalidate_vertex_format(VertexFormat const * _vertexFormat); // when we get rid of vertex format, invalidate it

		// update render state (all lazy stuff), ready for rendering
		inline void requires_send_all();
		inline void ready_for_rendering(); // if something would not be set

		// multi sample, set immediately
		inline void set_multi_sample(bool _enable);

		// conversion rgb->srgb on saving fragment shader to surface/buffer, set immediately
		inline void set_srgb_conversion(bool _enable);

		bool get_system_implicit_srgb() const { return systemImplicitSRGB; }
		void set_system_implicit_srgb(bool _srgb) { systemImplicitSRGB = _srgb; }

		// immediately, just don't double changes
		void bind_render_buffer(FrameBufferObjectID _frameBuffer);
		void unbind_render_buffer();
		void bind_frame_buffer(FrameBufferObjectID _frameBuffer, Optional<int> _asGLFrameBuffer = NP);
		void unbind_frame_buffer(Optional<int> _asGLFrameBuffer = NP); // for FullScreen::WindowScaled will get fullScreenRenderTarget 
		bool unbind_frame_buffer_bare(Optional<int> _asGLFrameBuffer = NP); // unbind to none / returns true if was bound to fullScreenRenderTarget 
		void invalidate_bound_frame_buffer_info(); // in case we were directly messing with bounding frame buffers, will force next binding/unbinding
		inline void forget_all_buffer_data();
		inline void forget_render_buffer_data();
		inline void forget_stencil_buffer_data();
		inline void forget_depth_buffer_data();
		inline void forget_depth_stencil_buffer_data();
		inline bool is_frame_buffer_bound() const { return frameBufferBound; }
		inline bool is_frame_buffer_bound(FrameBufferObjectID _frameBuffer) const { return frameBufferBound && currentFrameBuffer == _frameBuffer; }
		inline bool is_frame_buffer_unbound_bare() const { return currentFrameBuffer == 0; }

		inline void restore(Video3DStateStore const & _store);

		// state
		inline void requires_send_state();
		// blend
		inline void mark_enable_blend(BlendOp::Type _srcFactor = BlendOp::SrcAlpha, BlendOp::Type _destFactor = BlendOp::OneMinusSrcAlpha);
		inline void mark_disable_blend();
		inline void store_blend_mark(Video3DStateStore & _store) const;

		// polygon offset
		inline void mark_enable_polygon_offset(float _factor, float _units);
		inline void mark_disable_polygon_offset();
		inline void store_polygon_offset_mark(Video3DStateStore & _store) const;

		// display (swap) buffer
		void display_buffer();
		inline void mark_buffer_ready_to_display();
		inline void mark_buffer_displayed();
		inline bool is_buffer_ready_to_display() const { return bufferIsReadyToDisplay; }
		inline void display_buffer_if_ready();

		// shader programs
		inline bool is_shader_bound() const { return currentShaderProgramId != 0; }
		inline void bind_shader_program(ShaderProgramID _program, ShaderProgram* _shaderProgram = nullptr);
		inline void unbind_shader_program(ShaderProgramID _program);
		inline void requires_send_shader_program();
		inline ShaderProgram* get_fallback_shader_program(Video3DFallbackShader::Flags _flags) const { an_assert(fallbackShaderPrograms.is_index_valid(_flags));  return fallbackShaderPrograms[_flags].get(); }
		inline ShaderProgramInstance* get_fallback_shader_program_instance(Video3DFallbackShader::Flags _flags) const { an_assert(fallbackShaderProgramInstances.is_index_valid(_flags));  return fallbackShaderProgramInstances[_flags]; }

		// vertex buffer (objects)
		void bind_vertex_buffers(BufferObjectID _vertexBufferObjectID, BufferObjectID _elementBufferObjectID);
		void on_vertex_buffer_destroyed(BufferObjectID _vertexBufferObjectID); // required to be called once vertex buffer is destroyed
		void unbind_vertex_buffers();
		// these are setup once we know which shader program we use to connect attribs to shader program inputs
		void unbind_all_vertex_attribs(); // should be happening when we're setting up vertex attribs so there's no need to unbind everything always, just call this and lazy v current will handle things
		void bind_vertex_attrib(int _attributeId, int _valCount, float const * _val);
		void bind_vertex_attrib(int _attributeId, Vector2 const& _val) { bind_vertex_attrib(_attributeId, 2, &_val.x); }
		void bind_vertex_attrib(int _attributeId, Vector3 const& _val) { bind_vertex_attrib(_attributeId, 3, &_val.x); }
		void bind_vertex_attrib(int _attributeId, Vector4 const& _val) { bind_vertex_attrib(_attributeId, 4, &_val.x); }
		void bind_vertex_attrib(int _attributeId, Colour const& _val) { bind_vertex_attrib(_attributeId, 4, &_val.r); }
		void bind_vertex_attrib_array(int _attributeId, int _elementCount, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _stride, void* _pointer);
		// sends array buffers and vertex attribs
		struct RequiresSendVertexBufferParams
		{
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(RequiresSendVertexBufferParams, bool, forceResetup, force_resetup, false, true);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(RequiresSendVertexBufferParams, bool, justUnbind, just_unbind, false, true);
		};
		void requires_send_vertex_buffers();
		void requires_send_vertex_buffers(RequiresSendVertexBufferParams const & _params);

		// scissors
		inline RangeInt2 const& get_scissors() const; // if empty, not enabled
		inline void set_scissors(RangeInt2 const& _region);
		inline void clear_scissors();

		// viewport (after setting that it might be required to reset projection matrices)
		inline void set_viewport(SIZE_ VectorInt2 const& _size, bool _activate = true) { set_viewport(VectorInt2::zero, _size, _activate); }
		inline void set_viewport(LOCATION_ VectorInt2 const & _leftBottom, SIZE_ VectorInt2 const & _size, bool _activate = true);
		inline void activate_viewport();
		inline void set_default_viewport(bool _activate = true);
		inline LOCATION_ VectorInt2 get_viewport_left_bottom() const { return viewportLeftBottom; }
		inline SIZE_ VectorInt2 get_viewport_size() const { return viewportSize; }

		// setting up matrices (sets also matrix stack mode properly) (after viewport is set, near far planes etc)
		inline void set_3d_projection_matrix(); // x-right y-forward z-up
		inline void set_2d_projection_matrix_ortho(Vector2 const & _size = Vector2::zero); // x-right y-up

		// setting up matrices (sets also matrix stack mode properly)
		inline void set_3d_projection_matrix(Matrix44 const & _mat); // x-right y-forward z-up
		inline void set_2d_projection_matrix(Matrix44 const & _mat); // x-right y-up

		// setup state for 2d display
		inline void setup_for_2d_display();
		inline void setup_for_3d_display();

		// masks (for writing) (part of state)
		inline void depth_mask(bool _display = true);
		inline void colour_mask(bool _display = true);
		inline void stencil_mask(uint32 _mask = 0xffffffff);

		inline void store_depth_mask(Video3DStateStore & _store) const;

		inline void store_depth_test(Video3DStateStore & _store) const;
		
		inline void store_depth_clamp(Video3DStateStore & _store) const;

		// depth
		inline void depth_range(float _min, float _max);
		inline void depth_test(Video3DCompareFunc::Type _depthFunc = Video3DCompareFunc::None);
		inline void depth_clamp(bool _depthClamp);

		// stencil
		inline void stencil_test(Video3DCompareFunc::Type _stencilFunc = Video3DCompareFunc::None, int32 _stencilFuncRef = 0, uint32 _stencilFuncMask = 0xffffffff);
		// in reverse order of tests taken
		inline void stencil_op(Video3DOp::Type _bothPassed = Video3DOp::Keep, Video3DOp::Type _depthTestFails = Video3DOp::Keep, Video3DOp::Type _stencilTestFails = Video3DOp::Keep);
		
		// texture management
		inline void requires_send_use_textures();

		inline void mark_use_texture(int _textureSlot, TextureID _textureID);
		inline void mark_use_texture_if_not_set(int _textureSlot, TextureID _textureID);
		inline void mark_use_no_textures();
		inline void force_send_use_texture_slot(int _textureSlot);

		// face culling
		inline void front_face_order(FaceOrder::Type _type);
		inline void face_display(FaceDisplay::Type _display);

		// near far plane
		inline void set_near_far_plane(float _near, float _far) { nearFarPlane = Range(_near, _far); }
		inline Range const & get_near_far_plane() const { return nearFarPlane; }

		// fov (vertical)
		inline void set_fov(DEG_ float const _fov) { fov = _fov; }
		inline float get_fov() const { return fov; }

		inline VideoMatrixStack& access_model_view_matrix_stack() { return modelViewMatrixStack; }
		inline VideoClipPlaneStack& access_clip_plane_stack() { return clipPlaneStack; }

		inline VideoMatrixStack const & get_model_view_matrix_stack() const { return modelViewMatrixStack; }
		inline VideoClipPlaneStack const & get_clip_plane_stack() const { return clipPlaneStack; }
		inline Matrix44 const & get_projection_matrix() const { return projectionMatrix; }

		inline float get_aspect_ratio() const { return aspect_ratio(viewportSize); }

		inline static void ready_matrix_for_video_x_right_y_forward_z_up(REF_ Matrix44 & _mat);
		inline static void ready_plane_for_video_x_right_y_forward_z_up(REF_ Plane & _plane);

	public:
		void destroy_pending(int _limit = 0);
		void defer_buffer_to_destroy(::System::BufferObjectID _buffer);
		void defer_vertex_array_to_destroy(::System::VertexArrayObjectID _buffer);
		void defer_render_target_data_to_destroy(RenderTargetVideo3DData* _rttd);

	private:
		Concurrency::SpinLock toDestroyLock = Concurrency::SpinLock(TXT("System.Video3D.toDestroyLock"));
		Array<::System::BufferObjectID> buffersToDestroy;
		Array<::System::VertexArrayObjectID> vertexArraysToDestroy;
		List<RefCountObjectPtr<RenderTargetVideo3DData>> renderTargetVideo3DDatasToDestroy;

	public:
#ifdef AN_DEVELOPMENT
		void debug_list_render_targets(bool _onlyCount = false);
#endif

#ifdef AN_GL
	private: friend class VideoMatrixStack;
		inline void load_matrix_for_rendering(VideoMatrixMode::Type _mode, Matrix44 const & _matrix);
#endif

	public:

	private: friend class RenderTarget;
		void add(RenderTarget* _rt);
		void remove(RenderTarget* _rt);

	private: friend class Shader;
		void add(Shader* _s);
		void remove(Shader* _s);

	private: friend class ShaderProgram;
		void add(ShaderProgram* _sp);
		void remove(ShaderProgram* _sp);
		void on_close(ShaderProgram* _sp);

	private:
		static Video3D* s_current;
		
		bool useVec4ArraysInsteadOfMat4Arrays = false;
		bool limitedSupportForIndexingInShaders = false;
		
		bool initialised = false;

		System::TimeStamp lastDisplayTS;

		bool isFor3d = false;

		bool useShaderProgramCache = true;
		ShaderProgramCache* shaderProgramCache = nullptr;

		float currentIndividualOffset = 0.0f;

		struct CustomBuildInUniform
		{
			Name id;
			CustomSetBuildInUniforms custom_set_build_in_uniforms;
			Array<Name> uniforms;
		};
		Concurrency::SpinLock customBuildInUniformsLock;
		ArrayStatic<CustomBuildInUniform, 16> custom_build_in_uniforms;
		Array<Name> customBuildInUniforms; // all for easier lookup

		// replace with Video3DResource base class?
		Concurrency::SpinLock renderTargetsLock = Concurrency::SpinLock(TXT("System.Video3D.renderTargetsLock"));
		List<RenderTarget*> renderTargets;
		List<Shader*> shaders;
		List<ShaderProgram*> shaderPrograms;

#ifdef AN_SDL
		SDL_GLContext glContext;
#else
#ifdef AN_ANDROID
		EGLContext* glContext = nullptr;
#else
		#error implement
#endif
#endif

		Array<RefCountObjectPtr<ShaderProgram>> fallbackShaderPrograms;
		Array<ShaderProgramInstance*> fallbackShaderProgramInstances;
		
		bool bufferIsReadyToDisplay = false;
		bool skipDisplayBufferForVR = false;
		bool displayedBufferIsClean = false; // to display empty buffer

#ifdef AN_GL
		VideoMatrixMode::Type loadMatrixMode = VideoMatrixMode::NotSet;
#endif

		Matrix44 projectionMatrix;
		VideoMatrixStack modelViewMatrixStack;
		VideoClipPlaneStack clipPlaneStack;

		TextureID fallbackTexture = texture_id_none();
		Colour fallbackColour = Colour::full;

		Vector3 eyeOffset = Vector3::zero;

		// using optionals as the default state may be different
		Optional<bool> multiSampleEnabled = false;
		Optional<bool> sRGBConversion = false;

		// we do not turn on SRGB conversion as system expects data to be SRGB
		bool systemImplicitSRGB = false;
		
		bool forceFrameBufferBinding = false; // check invalidate_bound_frame_buffer_info
		FrameBufferObjectID currentFrameBuffer;
		bool frameBufferBound = false;
		RefCountObjectPtr<RenderTarget> fullScreenRenderTarget; // in case we have scaled window
		bool fullScreenFrameBufferBound = false; // this is when not explicitly bound

		bool blendEnabled = false;
		BlendOp::Type blendSrcFactor;
		BlendOp::Type blendDestFactor;

		bool lazyBlendEnabled = false;
		BlendOp::Type lazyBlendSrcFactor;
		BlendOp::Type lazyBlendDestFactor;

		bool polygonOffsetEnabled = false;
		float polygonOffsetFactor;
		float polygonOffsetUnits;

		bool lazyPolygonOffsetEnabled = false;
		float lazyPolygonOffsetFactor;
		float lazyPolygonOffsetUnits;

		ShaderProgramID currentShaderProgramId;
		ShaderProgramID lazyShaderProgramId;
		ShaderProgram* currentShaderProgram;
		ShaderProgram* lazyShaderProgram;

		static int const MAX_ATTRIBUTES_COUNT = 16;

		// this is shared between all 
		struct GenericVertexAttribInfo
		{
			struct Attribute
			{
				struct Value
				{
					int valueCount = 0;
					float value[4];
				};
				Value currentValue;
				Value lazyValue;
			};
			ArrayStatic<Attribute, MAX_ATTRIBUTES_COUNT> attributes;
			int attributesSend = 0; // to unbind/to avoid going too far
			int attributesInUse = 0;

			GenericVertexAttribInfo();

			void unbind_all();

			void bind(int _attributeId, int _valueCount, float const* _values);

			void send();
		} genericVertexAttribInfo;
		// this is per vertex buffer object
		struct VertexBufferObjectInfo
		{
			VertexArrayObjectID vertexArrayObjectID = 0; // created if not present, bound and used if vertexBufferObjectID in use
			BufferObjectID vertexBufferObjectID = 0;
			BufferObjectID elementBufferObjectID = 0;
			ShaderProgram const* shaderProgram = nullptr; // if the same, doesn't have to redo attributes
			bool bufferBoundToArray = false;
			struct Attribute
			{
				struct Value
				{
					bool inUse = false;
					int elementCount = 0;
					DataFormatValueType::Type dataType;
					VertexFormatAttribType::Type attribType;
					int stride;
					void* pointer = nullptr;
				};
				Value currentValue;
				Value lazyValue;
			};
			ArrayStatic<Attribute, MAX_ATTRIBUTES_COUNT> attributes;
			int attributesSend = 0; // to unbind
			int attributesInUse = 0;

			VertexBufferObjectInfo();

			void unbind_all();

			void bind(int _attributeId, int _elementCount, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _stride, void* _pointer);

			void ready_for(BufferObjectID _vertexBufferObjectID, BufferObjectID _elementBufferObjectID); // creates vertex array object
			struct SendParams
			{
				ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(SendParams, bool, alreadyJustBoundVertextBufferObjects, already_just_bound_vertext_buffer_objects, false, true);
				ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(SendParams, bool, forceSend, force_send, false, true);
			};
			void send(SendParams const & _params);
		};
		VertexBufferObjectInfo genericVertexBufferObjectInfo;
		Array<VertexBufferObjectInfo> vertexBufferObjectInfos; // what is bound and where (do not remove, clear within, on resize change lazyVertexBufferObjectInfo)

		BufferObjectID currentVertexBufferObject; // changes only if vertex array object not in use
		BufferObjectID lazyVertexBufferObject;
		VertexBufferObjectInfo* lazyVertexBufferObjectInfo; // setup only when binding to properly setup vertex buffer object attributes
		VertexArrayObjectID currentVertexArrayObject;
		VertexArrayObjectID lazyVertexArrayObject;

		BufferObjectID currentElementBufferObject;
		BufferObjectID lazyElementBufferObject;

		struct BoundBuffersAndVertexFormat
		{
			bool isBound = false;
			BufferObjectID vertexBufferObject;
			BufferObjectID elementBufferObject;
			VertexFormat const * vertexFormat;
			bool vertexFormatValid = false; // if vertexFormat is set but was discarded, this is set to true
			ShaderProgram * shaderProgram;
			bool usingFallbackShaderProgram;
			void const * dataPointer;
			TextureID fallbackTexture = texture_id_none();
			Colour fallbackColour = Colour::full;

			bool check_if_requires_change(BufferObjectID _vertexBufferObject, BufferObjectID _elementBufferObject, VertexFormat const * _vertexFormat, ShaderProgram * _shaderProgram, void const * _dataPointer,
				TextureID const & _fallbackTexture, Colour const & _fallbackColour) const;
		};
		BoundBuffersAndVertexFormat boundBuffersAndVertexFormat;

		Array<TextureID> currentTexturesID;
		int activeTextureSlot;
		Array<TextureID> lazyTexturesID;
		
		struct State
		{
			Range depthRange = Range(0.0f, 1.0f);

			FaceOrder::Type frontFaceOrder;
			FaceDisplay::Type faceDisplay;

#ifdef AN_GL
			ShadeModel::Type shadeModel;
#endif

			bool depthDisplay;
			bool colourDisplay;
			uint32 stencilDisplayMask;

			Video3DCompareFunc::Type depthFunc;
			
			bool depthClamp = false;

			Video3DCompareFunc::Type stencilFunc;
			int32 stencilFuncRef;
			uint32 stencilFuncMask;
			Video3DOp::Type stencilOpBothPassed;
			Video3DOp::Type stencilOpDepthTestFails;
			Video3DOp::Type stencilOpStencilTestFails;
		};
		State currentState;
		State lazyState;
		Array<State> stateStack;

		LOCATION_ VectorInt2 viewportLeftBottom;
		SIZE_ VectorInt2 viewportSize;
		LOCATION_ VectorInt2 activeViewportLeftBottom;
		SIZE_ VectorInt2 activeViewportSize;

		RangeInt2 scissors = RangeInt2::empty;

		Range nearFarPlane;
		DEG_ float fov; // vertical

#ifdef AN_DEVELOPMENT
		bool loadMatrices = false;
#endif

		Video3D();
		~Video3D();

		inline void set_projection_matrix(Matrix44 const & _mat);

		inline void invalidate_variables();
		inline void invalidate_pop_variables();

		inline void send_lazy_depth_range_internal();

		inline void send_lazy_depth_mask_internal();
		inline void send_lazy_colour_mask_internal();
		inline void send_lazy_stencil_mask_internal();

		inline void send_lazy_depth_test_internal();
		inline void send_lazy_depth_clamp_internal();
		inline void send_lazy_stencil_test_internal();
		inline void send_lazy_stencil_op_internal();

		inline void send_lazy_front_face_order_internal();
		inline void send_lazy_face_display_internal();

		inline void send_lazy_enable_blend_internal(BlendOp::Type _srcFactor, BlendOp::Type _destFactor);
		inline void send_lazy_disable_blend_internal();

		inline void send_lazy_polygon_offset_internal();

		inline void send_lazy_textures_internal();
		inline void send_lazy_use_texture_internal(int _textureSlot, TextureID _textureID);

		inline void send_lazy_shader_program_internal();
		inline void send_lazy_bind_shader_program_internal(ShaderProgramID _program, ShaderProgram* _shaderProgram);
		inline void send_lazy_unbind_shader_program_internal(ShaderProgramID _program);

		void update_full_screen_render_target();
		void display_buffer_full_screen_render_target();

	private:
		void make_gl_context_current();

		void rebuild_custom_build_in_uniforms();
	};

	#include "video3d.inl"

};
