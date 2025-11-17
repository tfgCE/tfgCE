#pragma once

#include "foveatedRenderingSetup.h"

#include "video.h"
#include "videoEnums.h"
#include "videoFormat.h"

#include "..\..\functionParamsStruct.h"

namespace System
{
	/**
	 *	A bunch of functions used to create, bind, prepare render target components
	 */
	namespace RenderTargetUtils
	{
		void error_on_invalid_framebuffer(GLenum frameBufferTarget);

		struct PrepareRenderTargetParams
		{
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, int, withMipMaps, with_mip_maps); // NP to off, otherwise limit/amount
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, TextureWrap::Type, wrapU, with_wrap_u);
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, TextureWrap::Type, wrapV, with_wrap_v);
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, TextureFiltering::Type, magFiltering, with_mag_filtering);
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, TextureFiltering::Type, minFiltering, with_min_filtering);

			// all of formats have to be set to be used (this may not need to be called if the texture has been provided by the (vr) system)
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, VectorInt2, size, with_size);
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, VideoFormat::Type, videoFormat, with_video_format);
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, BaseVideoFormat::Type, baseVideoFormat, with_base_video_format);
			ADD_FUNCTION_PARAM(PrepareRenderTargetParams, VideoFormatData::Type, videoFormatData, with_video_format_data);
		};
		void prepare_render_target_texture(TextureID _textureID, PrepareRenderTargetParams const & _params = PrepareRenderTargetParams());

		struct PrepareColourFrameBufferParams
		{
			ADD_FUNCTION_PARAM(PrepareColourFrameBufferParams, int, withMSAASamples, with_msaa_samples); // NP to off, otherwise number of samples
		};
		void prepare_colour_frame_buffer(FrameBufferObjectID _frameBuffer, TextureID _textureID, PrepareColourFrameBufferParams const & _params = PrepareColourFrameBufferParams());

		struct PrepareDepthStencilRenderBufferParams
		{
			// has to be set
			ADD_FUNCTION_PARAM(PrepareDepthStencilRenderBufferParams, VectorInt2, size, with_size);

			// optional but used only if size is set
			ADD_FUNCTION_PARAM(PrepareDepthStencilRenderBufferParams, int, withMSAASamples, with_msaa_samples); // NP to off, otherwise number of samples
			ADD_FUNCTION_PARAM_DEF(PrepareDepthStencilRenderBufferParams, bool, withStencil, with_stencil, true);
		};
		void prepare_depth_stencil_render_buffer(FrameBufferObjectID _frameBufferID, PrepareDepthStencilRenderBufferParams const& _params);

		void attach_colour_texture_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachColourTexture, Optional<int> const& _msaaSamples, Optional<int> _frameBufferTarget = NP);
		void attach_depth_stencil_texture_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachDepthStencilTexture, bool _withStencil = 0, Optional<int> _frameBufferTarget = NP);
		void attach_depth_stencil_render_buffer_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, RenderBufferID _attachDepthStencilFrameRenderBuffer, bool _withStencil = 0, Optional<int> _frameBufferTarget = NP);
		void attach_colour_texture_and_depth_stencil_texture_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachColourTexture, Optional<int> const& _msaaSamples, TextureID _attachDepthStencilTexture, bool _withStencil, Optional<int> _frameBufferTarget = NP);
		void attach_colour_texture_and_depth_stencil_render_buffer_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachColourTexture, Optional<int> const& _msaaSamples, RenderBufferID _attachDepthStencilFrameRenderBuffer, bool _withStencil, Optional<int> _frameBufferTarget = NP);
		inline void detach_colour_texture_and_depth_stencil_texture_from_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, Optional<int> const& _msaaSamples, bool _withStencil) { attach_colour_texture_and_depth_stencil_texture_to_frame_buffer(_attachToFrameBuffer, 0, _msaaSamples, 0, _withStencil); }
		inline void detach_colour_texture_and_depth_stencil_render_buffer_from_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, Optional<int> const& _msaaSamples, bool _withStencil) { attach_colour_texture_and_depth_stencil_render_buffer_to_frame_buffer(_attachToFrameBuffer, 0, _msaaSamples, 0, _withStencil); }

		bool setup_foveated_rendering_for_texture(Optional<int> _forEyeIdx, TextureID _textureID, FoveatedRenderingSetup const& _setup); // returns true if successfully setup
		void update_foveated_rendering_for_texture(Optional<int> _forEyeIdx, TextureID _textureID, FoveatedRenderingSetup const& _setup); // setup just focal points and gain, do not switch foveated on
		bool setup_foveated_rendering_for_framebuffer(Optional<int> _forEyeIdx, FrameBufferObjectID _frameBufferID, FoveatedRenderingSetup const& _setup); // returns true if successfully setup
		void update_foveated_rendering_for_framebuffer(Optional<int> _forEyeIdx, FrameBufferObjectID _frameBufferID, FoveatedRenderingSetup const& _setup); // setup just focal points and gain, do not switch foveated on

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
		void log_last_foveated_rendering_setups(LogInfoContext& _log);
		void render_last_foveated_rendering_setups(int _forEyeIdx);
#endif

		void fit_into(VectorInt2 const& _sourceSize, VectorInt2 const& _destinationSize, OUT_ Vector2& _leftBottom, OUT_ Vector2& _size);
		void fit_into(VectorInt2 const& _sourceSize, VectorInt2 const& _destinationSize, OUT_ Vector2& _size);
		void fit_into(VectorInt2 const& _sourceSize, VectorInt2 const& _destinationSize, OUT_ VectorInt2& _size);
	};
};
