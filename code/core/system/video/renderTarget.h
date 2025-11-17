#pragma once

#include "..\..\functionParamsStruct.h"
#include "..\..\globalDefinitions.h"
#include "..\..\math\math.h"

#include "foveatedRenderingSetup.h"

#include "video.h"

#include "videoFormat.h"
#include "vertexFormat.h"

#include "outputSetup.h"

#include "renderTargetPtr.h"

#include <functional>

//#define DEBUG_RENDER_TARGET_MEMORY
//#define DEBUG_RENDER_TARGET_CREATION

#ifdef DEBUG_RENDER_TARGET_CREATION
	#define RENDER_TARGET_CREATE_INFO(...) output(__VA_ARGS__);
#else
	#define RENDER_TARGET_CREATE_INFO(...)
#endif

namespace System
{
	class Video3D;
	class ShaderProgram;
	class Camera;

	struct RenderDataTexture
	{
		TextureID textureID = texture_id_none();
		TextureID renderToTextureID = texture_id_none();
		bool required = false; // will be changed to true if required
#ifdef DEBUG_RENDER_TARGET_MEMORY
		int bytesPerPixel = 4;
#endif

		// last set values, to change only if required
		TextureFiltering::Type magFiltering = TextureFiltering::nearest;
		TextureFiltering::Type minFiltering = TextureFiltering::nearest;
	};

	struct RenderTargetSetup
	: public OutputSetup<>
	{
	public:
		RenderTargetSetup(SIZE_ VectorInt2 _size,
						  int _msaaSamples = 0,
						  DepthStencilFormat::Type _depthStencilFormat = DepthStencilFormat::None)
		: size(_size)
		, depthStencilFormat(_depthStencilFormat)
		, msaaSamples(_msaaSamples)
		{
		}

		RenderTargetSetup()
		: size(VectorInt2::zero)
		, depthStencilFormat(DepthStencilFormat::None)
		, msaaSamples(0)
		{
		}

		SIZE_ VectorInt2 const & get_size() const { return size; }
		void set_size(SIZE_ VectorInt2 const& _size) { size = _size; }

		float get_aspect_ratio_coef() const { return aspectRatioCoef; }
		void set_aspect_ratio_coef(float _aspectRatioCoef) { aspectRatioCoef = _aspectRatioCoef; }

		void set_depth_stencil_format(DepthStencilFormat::Type _depthStencilFormat) { depthStencilFormat = _depthStencilFormat; }
		DepthStencilFormat::Type get_depth_stencil_format() const { return depthStencilFormat; }

		void dont_use_msaa() { msaaSamples = 0; }
		bool should_use_msaa() const { return msaaSamples > 1; }
		int get_msaa_samples() const { return msaaSamples; }
		void set_msaa_samples(int _msaaSamples) { msaaSamples = _msaaSamples; }

		bool should_use_srgb_conversion() const;
		void set_forced_srgb_conversion(Optional<bool> _forced) { forcedSRGBConversion = _forced; }

		void force_mip_maps(bool _forceMipMaps = true);
		bool are_mip_maps_forced() const { return forceMipMaps; }

		void limit_mip_maps(int _maxLevel) { limitMipMaps = _maxLevel; }
		int get_mip_maps_limit() const { return limitMipMaps; }

		void for_eye_idx(Optional<int> const& _forEyeIdx) { forEyeIdx = forEyeIdx; }
		Optional<int> const& get_for_eye_idx() const { return forEyeIdx; }

		void use_foveated_rendering(Optional<FoveatedRenderingSetup> const& _setup) { foveatedRenderingSetup = _setup; }
		Optional<FoveatedRenderingSetup> const& get_foveated_rendering() const { return foveatedRenderingSetup; }

	private:
		SIZE_ VectorInt2 size;
		float aspectRatioCoef = 1.0f;
		DepthStencilFormat::Type depthStencilFormat;
		int msaaSamples;
		bool forceMipMaps = false;
		int limitMipMaps = NONE;
		Optional<bool> forcedSRGBConversion; // in particular cases (see where it is used, it might be required to force sRGB rendering on or off
		Optional<int> forEyeIdx;
		Optional<FoveatedRenderingSetup> foveatedRenderingSetup;
	};

	struct RenderTargetProxyData
	{
		// this is a very simple setup that uses only single output colour texture + frame buffer object, MSAA is not handled automatically etc
		// when used for proxy it assumes that all of them are bound properly together
		// when being used for dynamic proxy, binds and unbinds them all 
		TextureID colourTexture = 0;
		FrameBufferObjectID frameBufferObject = 0;
		TextureID depthStencilTexture = 0;
		RenderBufferID depthStencilBufferObject = 0;
	};

	// we always attach them when creating render target
	struct RenderTargetSetupProxy
	: public RenderTargetSetup
	, public RenderTargetProxyData
	{
	public:
		RenderTargetSetupProxy(SIZE_ VectorInt2 _size,
						  int _msaaSamples = 0,
						  DepthStencilFormat::Type _depthStencilFormat = DepthStencilFormat::None)
		: RenderTargetSetup(_size, _msaaSamples, _depthStencilFormat)
		{}
		RenderTargetSetupProxy(RenderTargetSetup const & _source)
		: RenderTargetSetup(_source)
		{}

		void attach();
	};
	
	// they don't have to be attached together as we will be ataching them on go
	struct RenderTargetSetupDynamicProxy
	: public RenderTargetSetupProxy
	{
	public:
		RenderTargetSetupDynamicProxy(SIZE_ VectorInt2 _size,
						  int _msaaSamples = 0,
						  DepthStencilFormat::Type _depthStencilFormat = DepthStencilFormat::None)
		: RenderTargetSetupProxy(_size, _msaaSamples, _depthStencilFormat)
		{}
		RenderTargetSetupDynamicProxy(RenderTargetSetup const & _source)
		: RenderTargetSetupProxy(_source)
		{}
	};

	struct RenderTargetVertexBufferUsage
	{
		BufferObjectID vertexBufferObject;
		bool dataLoaded = false;
		bool loadedWithoutCamera;
		LOCATION_ Vector2 loadedLeftBottom;
		SIZE_ Vector2 loadedSize;
		float loadedFOV;
		float loadedAspectRatio;
		Vector2 loadedCameraViewCentreOffset;
		Range2 loadedUVRange;
		float loadedForScale;
	};

	struct RenderTargetVideo3DData
	: public RefCountObject
	{
		// when deleting a render target, we should add it to video3d

#ifdef DEBUG_RENDER_TARGET_CREATION
		CACHED_ VectorInt2 size = VectorInt2::zero;
#endif

		// cached stuff is set before deletion, so the deletion process may go smoothly
		CACHED_ bool isProxy = false;
		CACHED_ bool shouldUseMSAA = false;

		// this one fbo may have attached both textures (render buffers would be faster but we'd like to use texture afterwards)
		FrameBufferObjectID frameBufferObject = 0; // this is where we read from
		FrameBufferObjectID renderToFrameBufferObject = 0; // this is where we write to (may be absent)
		Array<RenderDataTexture> dataTexturesData;
		TextureID depthStencilTexture = 0;
		RenderBufferID depthStencilBufferObject = 0;
		VertexFormat vertexFormat;
		Array<RenderTargetVertexBufferUsage> vertexBufferUsages;

		RenderTargetVideo3DData();

		void close();
	};

	struct ForRenderTargetOrNone
	{
		RenderTarget* rt = nullptr;
		ForRenderTargetOrNone() {}
		ForRenderTargetOrNone(RenderTarget* _rt): rt(_rt) {}

		ForRenderTargetOrNone & bind();
		ForRenderTargetOrNone & unbind();
		ForRenderTargetOrNone & set_viewport();
		ForRenderTargetOrNone & set_viewport(RangeInt2 const & _viewport);
		VectorInt2 get_size();
		float get_full_size_aspect_ratio(); // full size
	};

	/*
	 *	Will auto delete on Video3d closing.
	 *
	 *	If in proxy mode, won't clean up after itself, has to be done manually. It is a very simple setup, check above. If there is MSAA it is not resolved automatically, etc.
	 */
	class RenderTarget
	: public RefCountObject
	{
	public:
		RenderTarget(RenderTargetSetup const & _setup);
		RenderTarget(RenderTargetSetupProxy const& _setupProxy);
		RenderTarget(RenderTargetSetupDynamicProxy const& _setupProxy);
		virtual ~RenderTarget();

		RenderTarget* create_copy(std::function<void(RenderTargetSetup & _setup)> _alterSetup = nullptr) const;

		void init(RenderTargetSetup const & _setup);
		void close(bool _fromRenderingThread);

		void mark_all_outputs_required();
		void mark_output_not_required(int _index);
		int get_required_outputs_count() const;

		void update_foveated_rendering(::System::FoveatedRenderingSetup const& _setup); // only if already used and not proxy (proxies don't have foveated rendering managed by render target)

		bool is_bound() const;
		void bind();
		void unbind();
		static void bind_none();
		static void unbind_to_none() { return bind_none(); }
		void resolve(); // for msaa, for others it does nothing
		void resolve_forced_unbind(); // will unbind frame buffer
		bool is_resolved() const { return resolved; }

		void generate_mip_maps();

		float get_scaling() const { return currentScale; }
		void use_scaling(float _scale = 1.0f) { currentScale = clamp(_scale, 0.01f, 1.0f); }

		inline bool is_valid() const { return isValid; }

		inline RenderTargetSetup const & get_setup() const { return setup; }
		inline bool should_use_mip_maps() const { return withMipMaps; }

		void set_data_for_dynamic_proxy(RenderTargetProxyData const& _proxyData);

		inline FrameBufferObjectID get_frame_buffer_object_id_to_read() const { an_assert(resolved); return v3dData->frameBufferObject; }
		inline FrameBufferObjectID get_frame_buffer_object_id_to_write() const { return !isProxy && setup.should_use_msaa() ? v3dData->renderToFrameBufferObject : v3dData->frameBufferObject; }
		inline int get_data_texture_count() const { return v3dData->dataTexturesData.get_size(); }
		inline TextureID get_data_texture_id(int _index) const { an_assert(resolved);  return v3dData->dataTexturesData[_index].textureID; }
		inline TextureID get_data_texture_id_may_be_unresolved(int _index) const { return v3dData->dataTexturesData[_index].textureID; }
		inline VectorInt2 get_size(bool _trueNotScaled = false) const { return currentScale == 1.0f || _trueNotScaled ? size : (size.to_vector2() * currentScale).to_vector_int2_cells(); }
		VectorInt2 get_full_size_for_aspect_ratio_coef(bool _trueNotScaled = false) const;

		void change_filtering(int _dataTextureIndex, TextureFiltering::Type _mag, TextureFiltering::Type _min);

		void copy_to(RenderTarget* dest);

		void render_fit(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Vector2> const& _alignPT = NP, Optional<bool> const & _zoomX = NP, Optional<bool> const& _zoomY = NP); // will try to fit into this space, maintaining aspect ratio,

		void render(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const & _uvRange = NP);
		void render(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const & _uvRange, ShaderProgram* _shaderProgram); // will bind vertex attribs
		void render(int _dataTextureIndex, Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const & _uvRange, Camera const & _camera, ShaderProgram* _shaderProgram); // will bind vertex attribs

		void render_for_shader_program(Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const & _uvRange, ShaderProgram* _shaderProgram); // just pure render will bind vertex attribs
		void render_for_shader_program(Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Optional<Range2> const & _uvRange, Camera const & _camera, ShaderProgram* _shaderProgram); // just pure render will bind vertex attribs

		enum CopyFlags
		{
			CopyData = 1,
			CopyDepth = 2,
			CopyStencil = 4,

			CopyFlagsDefault = CopyData,
			CopyAll = CopyData | CopyDepth | CopyStencil,
		};
		static void copy(RenderTarget* _source, VectorInt2 const & _srcAt, VectorInt2 const & _srcSize, RenderTarget* _dest, Optional<int> _copyFlags = NP, Optional<VectorInt2> _destAt = NP, Optional<VectorInt2> _destSize = NP); // will copy with same params if not provided

	private:
		RenderTargetSetup setup;
		bool isProxy = false; // if proxy, won't release objects, have to be released externally, use with extreme care!
		bool isDynamicProxy = false; // if dynamic proxy, will bind buffers at binding and unbind at unbinding
		bool withMipMaps;
		float currentScale = 1.0f; // this is temporary (glad you described for what it is used)
		VectorInt2 size;
		RefCountObjectPtr<RenderTargetVideo3DData> v3dData;
		bool isValid;
		bool resolved = true;
		bool usingFoveatedRendering = false;

		static RenderTargetPtr boundRenderTarget;
		static Range2 wholeUVRange;

		inline void load_vertex_data(RenderTargetVertexBufferUsage& _vbu, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Range2 const& _uvRange);
		inline void load_vertex_data(RenderTargetVertexBufferUsage& _vbu, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size, Range2 const& _uvRange, Camera const & _camera);
		inline RenderTargetVertexBufferUsage& get_vertex_buffer_usage(int _subIndex);

		inline void construct_as_proxy(RenderTargetSetupProxy const& _setupProxy);
	};

};
