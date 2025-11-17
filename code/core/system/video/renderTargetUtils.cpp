#include "renderTargetUtils.h"

#include "video3d.h"
#include "video3dPrimitives.h"
#include "videoGLExtensions.h"

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
#include "..\..\concurrency\scopedSpinLock.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define ELABORATE_DEBUG_LOGGING

//#define DEBUG_FOVEATED_RENDERING_SETUP

//

using namespace System;

//

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
struct LastFoveatedRenderingSetup
: public ::System::FoveatedRenderingSetup
{
	Optional<int> forEyeIdx;
	TextureID forTexture = ::System::texture_id_none();
};

::Concurrency::SpinLock g_lastFoveatedRenderingSetupsLock;
ArrayStatic<LastFoveatedRenderingSetup, 2> g_lastFoveatedRenderingSetups;

void RenderTargetUtils::log_last_foveated_rendering_setups(LogInfoContext& _log)
{
	Concurrency::ScopedSpinLock lock(g_lastFoveatedRenderingSetupsLock);
	for_every(lfrs, g_lastFoveatedRenderingSetups)
	{
		_log.log(TXT("eye %i texture %i min density %.4f"), lfrs->forEyeIdx.get(-1), lfrs->forTexture, FoveatedRenderingSetup::adjust_min_density(lfrs->minDensity));
		for_every(fp, lfrs->focalPoints)
		{
			_log.log(TXT("  %i at:%.3fx%.3f g:%.3fx%.3f %.3f"), for_everys_index(fp), fp->at.x, fp->at.y, fp->gain.x, fp->gain.y, fp->foveArea);
		}
	}
}

void RenderTargetUtils::render_last_foveated_rendering_setups(int _forEyeIdx)
{
	auto* v3d = ::System::Video3D::get();
	Concurrency::ScopedSpinLock lock(g_lastFoveatedRenderingSetupsLock);
	for_every(lfrs, g_lastFoveatedRenderingSetups)
	{
		if (lfrs->forEyeIdx.get(-1) == _forEyeIdx)
		{
			auto vs = v3d->get_viewport_size();
			auto vlb = v3d->get_viewport_left_bottom();
			Vector2 vh = vs.to_vector2() / 2.0f;
			Vector2 vc = vlb.to_vector2() + vh;
			for_every(fp, lfrs->focalPoints)
			{
				Colour fpLine = Colour::blue;
				if (for_everys_index(fp) == 1) fpLine = Colour::green;
				if (for_everys_index(fp) == 2) fpLine = Colour::orange;
				if (for_everys_index(fp) == 3) fpLine = Colour::yellow;
				if (for_everys_index(fp) == 4) fpLine = Colour::red;
				if (for_everys_index(fp) == 5) fpLine = Colour::mint;
				if (for_everys_index(fp) >= 6) fpLine = Colour::magenta;

				::System::Video3DPrimitives::line_2d(fpLine, Vector2(vc.x - vh.x, vc.y + vh.y * fp->at.y), Vector2(vc.x + vh.x, vc.y + vh.y * fp->at.y), false);
				::System::Video3DPrimitives::line_2d(fpLine, Vector2(vc.x + vh.x * fp->at.x, vc.y - vh.y), Vector2(vc.x + vh.x * fp->at.x, vc.y + vh.y), false);

				// (x - at.x)^2 * gain.x^2 - fovearea = densitycoef
				// rx^2 * gain.x^2 - fovearea = densitycoef
				// rx^2 = (densitycoef + foverea) / gain.x^2
				float densityTarget = 1.0f;
				while (densityTarget >= FoveatedRenderingSetup::adjust_min_density(lfrs->minDensity) &&
					   densityTarget > 0.05f)
				{
					float densityCoef = 1.0f / densityTarget;
					::System::Video3DPrimitives::circle_2d(fpLine, Vector2(vc.x + vh.x * fp->at.x, vc.y + vh.y * fp->at.y),
						vh * Vector2(sqrt(max(0.0f, (densityCoef + fp->foveArea) / sqr(max(0.0001f, fp->gain.x)))), sqrt(max(0.0f, (densityCoef + fp->foveArea) / sqr(max(0.0001f, fp->gain.y))))), false);
					densityTarget *= 0.5f;
				}
			}
		}
	}
}
#endif

void RenderTargetUtils::error_on_invalid_framebuffer(GLenum _frameBufferTarget)
{
	String problem;
	DIRECT_GL_CALL_ GLenum error = glGetError();
	if (error == GL_OUT_OF_MEMORY)
	{
		error_stop(TXT("out of memory!"));
	}
	DIRECT_GL_CALL_ GLenum status = glCheckFramebufferStatus(_frameBufferTarget);
	if (status == GL_FRAMEBUFFER_UNDEFINED)
	{
		problem = TXT("undefined");
	}
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
	{
		problem = TXT("incomplete attachment");
	}
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
	{
		problem = TXT("incomplete missing attachment");
	}
#ifdef AN_GL
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
	{
		problem = TXT("incomplete draw buffer");
	}
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
	{
		problem = TXT("incomplete read buffer");
	}
#endif
	else if (status == GL_FRAMEBUFFER_UNSUPPORTED)
	{
		problem = TXT("unsupported");
	}
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)
	{
		problem = TXT("incomplete multisample");
	}
#ifdef AN_GL
	else if (status == GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS)
	{
		problem = TXT("incomplete layer targets");
	}
#endif
	else
	{
		problem = String::printf(TXT("?? %i ??"), status);
	}
	error_stop(TXT("could not create render target (%S)"), problem.to_char());
}

void RenderTargetUtils::prepare_render_target_texture(TextureID _textureID, PrepareRenderTargetParams const& _params)
{
	int withMipMapsLimit = _params.withMipMaps.get(0);
	bool withMipMaps = withMipMapsLimit > 0;

	Video3D::get()->mark_use_texture(0, _textureID);
	Video3D::get()->force_send_use_texture_slot(0); AN_GL_CHECK_FOR_ERROR
	Video3D::get()->requires_send_use_textures(); AN_GL_CHECK_FOR_ERROR

#ifdef AN_GL
	// initial value is false, but hey
	DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE); AN_GL_CHECK_FOR_ERROR // no automatic mipmap
#endif
	if (withMipMaps)
	{
		DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, withMipMapsLimit); AN_GL_CHECK_FOR_ERROR
	}

	if (_params.size.is_set() &&
		_params.videoFormat.is_set() &&
		_params.baseVideoFormat.is_set() &&
		_params.videoFormatData.is_set())
	{
		DIRECT_GL_CALL_ glTexImage2D(GL_TEXTURE_2D, 0, _params.videoFormat.get(), _params.size.get().x, _params.size.get().y, 0, _params.baseVideoFormat.get(), _params.videoFormatData.get(), 0); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		an_assert(!_params.size.is_set() &&
			!_params.videoFormat.is_set() &&
			!_params.baseVideoFormat.is_set() &&
			!_params.videoFormatData.is_set(), TXT("either set all or none"));
	}
	/*
	if (_params.size.is_set() &&
		_params.videoFormat.is_set() &&
		_params.baseVideoFormat.is_set() &&
		_params.videoFormatData.is_set())
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _params.size.get().x, _params.size.get().y, 0, GL_RGBA,
				GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	*/

	{
		bool useBorder = false;
		for_count(int, componentIdx, 2)
		{
			int glTextureWrapComponent = componentIdx == 0 ? GL_TEXTURE_WRAP_S : GL_TEXTURE_WRAP_T;
			TextureWrap::Type useWrap = (componentIdx == 0 ? _params.wrapU : _params.wrapV).get(TextureWrap::clamp);
			if (useWrap == TextureWrap::clamp)
			{
				// try to use border if clamping
	#ifdef AN_GLES
				auto& glExtensions = ::System::VideoGLExtensions::get();
				if (glExtensions.EXT_texture_border_clamp)
				{
					DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, glTextureWrapComponent, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR
					useBorder = true;
				}
				else
				{
					// Just clamp to edge. However, this requires manually clearing the border
					// around the layer to clear the edge texels.
					DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, glTextureWrapComponent, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR
				}
	#else
				{
					DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, glTextureWrapComponent, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR
					useBorder = true;
				}
	#endif
			}
			else
			{
				DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, glTextureWrapComponent, useWrap); AN_GL_CHECK_FOR_ERROR
			}
		}

		if (useBorder)
		{
			GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			DIRECT_GL_CALL_ glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor); AN_GL_CHECK_FOR_ERROR
		}

		// mip map linear will be changed for mag to linear
		DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TextureFiltering::get_sane_mag(TextureFiltering::get_proper_for(_params.magFiltering.get(TextureFiltering::linearMipMapLinear), withMipMaps))); AN_GL_CHECK_FOR_ERROR
		DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TextureFiltering::get_sane_min(TextureFiltering::get_proper_for(_params.minFiltering.get(TextureFiltering::linearMipMapLinear), withMipMaps))); AN_GL_CHECK_FOR_ERROR
	}

	Video3D::get()->mark_use_texture(0, texture_id_none());
	Video3D::get()->requires_send_use_textures();

}

void RenderTargetUtils::prepare_colour_frame_buffer(FrameBufferObjectID _frameBufferID, TextureID _textureID, PrepareColourFrameBufferParams const& _params)
{
	Optional<int> msaaSamples = _params.withMSAASamples;

#ifdef AN_GLES
	auto& glExtensions = ::System::VideoGLExtensions::get();
	if (msaaSamples.is_set() &&
		(!glExtensions.glFramebufferTexture2DMultisampleEXT ||
		 !glExtensions.glRenderbufferStorageMultisampleEXT))
	{
		error(TXT("no gl multisample functions available, not using multisample with directly to vr"));
		msaaSamples.clear();
	}
#endif

	GLenum frameBufferTarget = GL_FRAMEBUFFER;// _params.withMSAASamples.is_set() ? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;

	Video3D::get()->bind_frame_buffer(_frameBufferID, frameBufferTarget);
	if (msaaSamples.is_set())
	{
#ifdef AN_GLES
		DIRECT_GL_CALL_ glExtensions.glFramebufferTexture2DMultisampleEXT(frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textureID, 0, msaaSamples.get()); AN_GL_CHECK_FOR_ERROR
#else
		DIRECT_GL_CALL_ glFramebufferTexture2D(frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, _textureID, 0); AN_GL_CHECK_FOR_ERROR
#endif
	}
	else
	{
		DIRECT_GL_CALL_ glFramebufferTexture2D(frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _textureID, 0); AN_GL_CHECK_FOR_ERROR
	}
	{
		DIRECT_GL_CALL_ bool isValid = glCheckFramebufferStatus(frameBufferTarget) == GL_FRAMEBUFFER_COMPLETE; // no AN_GL_CHECK_FOR_ERROR, we check with error_on_invalid_framebuffer

		if (!isValid)
		{
			error(TXT("error creating render target for msaa"));
			error_on_invalid_framebuffer(frameBufferTarget);
		}
	}
	Video3D::get()->unbind_frame_buffer(frameBufferTarget);
}

void RenderTargetUtils::prepare_depth_stencil_render_buffer(FrameBufferObjectID _frameBufferID, PrepareDepthStencilRenderBufferParams const& _params)
{
	if (_params.size.is_set())
	{
		VectorInt2 size = _params.size.get();
		int msaaSamples = _params.withMSAASamples.get(1);

		::System::Video3D::get()->bind_render_buffer(_frameBufferID);
#ifdef AN_GL
		if (_params.withStencil.get(false))
		{
			DIRECT_GL_CALL_ glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, GL_DEPTH_STENCIL, size.x, size.y); AN_GL_CHECK_FOR_ERROR
		}
		else
		{
			DIRECT_GL_CALL_ glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, GL_DEPTH_COMPONENT, size.x, size.y); AN_GL_CHECK_FOR_ERROR
		}
#endif
#ifdef AN_GLES
		auto& glExtensions = ::System::VideoGLExtensions::get();
		if (_params.withStencil.get(false))
		{
			DIRECT_GL_CALL_ glExtensions.glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, msaaSamples, hardcoded GL_DEPTH24_STENCIL8, size.x, size.y); AN_GL_CHECK_FOR_ERROR
		}
		else
		{
			DIRECT_GL_CALL_ glExtensions.glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, msaaSamples, hardcoded GL_DEPTH_COMPONENT24, size.x, size.y); AN_GL_CHECK_FOR_ERROR
		}
#endif
		::System::Video3D::get()->unbind_render_buffer();
	}
}

void __internal__attach_colour_texture_to_bound_frame_buffer(FrameBufferObjectID _frameBufferTarget, TextureID _attachColourTexture, Optional<int> _msaaSamples)
{
	if (_msaaSamples.get(0) > 1)
	{
#ifdef AN_GLES
		auto& glExtensions = ::System::VideoGLExtensions::get();
		DIRECT_GL_CALL_ glExtensions.glFramebufferTexture2DMultisampleEXT(_frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _attachColourTexture, 0, _msaaSamples.get()); AN_GL_CHECK_FOR_ERROR_ALWAYS
#else
		DIRECT_GL_CALL_ glFramebufferTexture2D(_frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, _attachColourTexture, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
#endif
	}
	else
	{
		DIRECT_GL_CALL_ glFramebufferTexture2D(_frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _attachColourTexture, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
	}
	/*
	DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glFramebufferTexture2D(_frameBufferTarget, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _attachColourTexture, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
	GLenum buffers[1];
	GLenum* buffer = buffers;
	int usedCount = 0;
	int index = 0;
	if (_attachColourTexture)
	{
		*buffer = GL_COLOR_ATTACHMENT0 + index;
		++buffer;
		++usedCount;
		++index;
	}
	DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDrawBuffers(usedCount, buffers); AN_GL_CHECK_FOR_ERROR_ALWAYS
	*/
}

void __internal__attach_depth_stencil_texture_to_bound_frame_buffer(FrameBufferObjectID _frameBufferTarget, TextureID _attachDepthStencilTexture, bool _withStencil)
{
	DIRECT_GL_CALL_ glFramebufferTexture2D(_frameBufferTarget, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _attachDepthStencilTexture, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
	if (_withStencil)
	{
		DIRECT_GL_CALL_ glFramebufferTexture2D(_frameBufferTarget, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, _attachDepthStencilTexture, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
	}
	else
	{
		DIRECT_GL_CALL_ glFramebufferTexture2D(_frameBufferTarget, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
	}
}

void __internal__attach_depth_stencil_render_buffer_to_bound_frame_buffer(FrameBufferObjectID _frameBufferTarget, RenderBufferID _attachDepthStencilFrameRenderBuffer, bool _withStencil)
{
	DIRECT_GL_CALL_ glFramebufferRenderbuffer(_frameBufferTarget, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _attachDepthStencilFrameRenderBuffer); AN_GL_CHECK_FOR_ERROR_ALWAYS
	if (_withStencil)
	{
		DIRECT_GL_CALL_ glFramebufferRenderbuffer(_frameBufferTarget, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _attachDepthStencilFrameRenderBuffer); AN_GL_CHECK_FOR_ERROR_ALWAYS
	}
	else
	{
		DIRECT_GL_CALL_ glFramebufferRenderbuffer(_frameBufferTarget, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0); AN_GL_CHECK_FOR_ERROR_ALWAYS
	}
}

void RenderTargetUtils::attach_colour_texture_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachColourTexture, Optional<int> const& _msaaSamples, Optional<int> _frameBufferTarget)
{
	FrameBufferObjectID frameBufferTarget = _frameBufferTarget.get(GL_DRAW_FRAMEBUFFER);
	// attach, each component on its own it, first bind main frame buffer object
	Video3D::get()->bind_frame_buffer(_attachToFrameBuffer, frameBufferTarget);
	__internal__attach_colour_texture_to_bound_frame_buffer(frameBufferTarget, _attachColourTexture, _msaaSamples);
	Video3D::get()->unbind_frame_buffer(frameBufferTarget);
}

void RenderTargetUtils::attach_depth_stencil_texture_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachDepthStencilTexture, bool _withStencil, Optional<int> _frameBufferTarget)
{
	FrameBufferObjectID frameBufferTarget = _frameBufferTarget.get(GL_DRAW_FRAMEBUFFER);
	// attach, each component on its own it, first bind main frame buffer object
	Video3D::get()->bind_frame_buffer(_attachToFrameBuffer, frameBufferTarget);
	__internal__attach_depth_stencil_texture_to_bound_frame_buffer(frameBufferTarget, _attachDepthStencilTexture, _withStencil);
	Video3D::get()->unbind_frame_buffer(frameBufferTarget);
}

void RenderTargetUtils::attach_depth_stencil_render_buffer_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, RenderBufferID _attachDepthStencilFrameRenderBuffer, bool _withStencil, Optional<int> _frameBufferTarget)
{
	FrameBufferObjectID frameBufferTarget = _frameBufferTarget.get(GL_DRAW_FRAMEBUFFER);
	// attach, each component on its own it, first bind main frame buffer object
	Video3D::get()->bind_frame_buffer(_attachToFrameBuffer, frameBufferTarget);
	__internal__attach_depth_stencil_render_buffer_to_bound_frame_buffer(frameBufferTarget, _attachDepthStencilFrameRenderBuffer, _withStencil);
	Video3D::get()->unbind_frame_buffer(frameBufferTarget);
}

void RenderTargetUtils::attach_colour_texture_and_depth_stencil_texture_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachColourTexture, Optional<int> const& _msaaSamples, TextureID _attachDepthStencilTexture, bool _withStencil, Optional<int> _frameBufferTarget)
{
	FrameBufferObjectID frameBufferTarget = _frameBufferTarget.get(GL_DRAW_FRAMEBUFFER);
	// attach, each component on its own it, first bind main frame buffer object
	Video3D::get()->bind_frame_buffer(_attachToFrameBuffer, frameBufferTarget);
	__internal__attach_colour_texture_to_bound_frame_buffer(frameBufferTarget, _attachColourTexture, _msaaSamples);
	__internal__attach_depth_stencil_texture_to_bound_frame_buffer(frameBufferTarget, _attachDepthStencilTexture, _withStencil);
	Video3D::get()->unbind_frame_buffer(frameBufferTarget);
}

void RenderTargetUtils::attach_colour_texture_and_depth_stencil_render_buffer_to_frame_buffer(FrameBufferObjectID _attachToFrameBuffer, TextureID _attachColourTexture, Optional<int> const & _msaaSamples, RenderBufferID _attachDepthStencilFrameRenderBuffer, bool _withStencil, Optional<int> _frameBufferTarget)
{
	FrameBufferObjectID frameBufferTarget = _frameBufferTarget.get(GL_DRAW_FRAMEBUFFER);
	// attach, each component on its own it, first bind main frame buffer object
	Video3D::get()->bind_frame_buffer(_attachToFrameBuffer, frameBufferTarget);
	__internal__attach_colour_texture_to_bound_frame_buffer(frameBufferTarget, _attachColourTexture, _msaaSamples);
	__internal__attach_depth_stencil_render_buffer_to_bound_frame_buffer(frameBufferTarget, _attachDepthStencilFrameRenderBuffer, _withStencil);
	Video3D::get()->unbind_frame_buffer(frameBufferTarget);
}

bool RenderTargetUtils::setup_foveated_rendering_for_texture(Optional<int> _forEyeIdx, TextureID _textureID, FoveatedRenderingSetup const& _setup)
{
#ifdef ELABORATE_DEBUG_LOGGING
	output_gl(TXT("setup_foveated_rendering_for_texture 0x%X"), _textureID);
#endif
	bool result = false;
	auto& glExtensions = ::System::VideoGLExtensions::get();
	if (_textureID != 0 &&
		glExtensions.QCOM_texture_foveated &&
		glExtensions.glTextureFoveationParametersQCOM)
	{
		// https://www.khronos.org/registry/OpenGL/extensions/QCOM/QCOM_texture_foveated.txt

		::System::Video3D::get()->mark_use_texture(0, _textureID);
		::System::Video3D::get()->force_send_use_texture_slot(0);
		::System::Video3D::get()->requires_send_use_textures();
		GLint supported = -1;
		DIRECT_GL_CALL_ glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_FOVEATED_FEATURE_QUERY_QCOM, &supported); AN_GL_CHECK_FOR_ERROR
		// note: could be working fine with other foveated rendering but they may have some limitations
		if (is_flag_set(supported, GL_FOVEATION_ENABLE_BIT_QCOM) &&
			is_flag_set(supported, GL_FOVEATION_SCALED_BIN_METHOD_BIT_QCOM))
		{
#ifdef ELABORATE_DEBUG_LOGGING
			output(TXT("foveated supported %i"), supported);
#endif
			float actualMinDensity = pow(0.5f, (float)max(1, ::MainConfig::global().get_video().vrFoveatedRenderingMaxDepth) - 1.0f) * 0.95f;
			actualMinDensity = max(actualMinDensity, _setup.minDensity);
			actualMinDensity = FoveatedRenderingSetup::adjust_min_density(actualMinDensity);
			DIRECT_GL_CALL_ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FOVEATED_FEATURE_BITS_QCOM, GL_FOVEATION_ENABLE_BIT_QCOM | GL_FOVEATION_SCALED_BIN_METHOD_BIT_QCOM); AN_GL_CHECK_FOR_ERROR
			DIRECT_GL_CALL_ glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_FOVEATED_MIN_PIXEL_DENSITY_QCOM, actualMinDensity); AN_GL_CHECK_FOR_ERROR

#ifdef DEBUG_FOVEATED_RENDERING_SETUP
			output(TXT("[render-target-utils] fovren density %.3f"), actualMinDensity);
#endif
#ifdef ELABORATE_DEBUG_LOGGING
			{
				GLint setbits = -1;
				DIRECT_GL_CALL_ glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_FOVEATED_FEATURE_BITS_QCOM, &setbits); AN_GL_CHECK_FOR_ERROR
					output(TXT("foveated set bits %i"), setbits);
			}
#endif

			update_foveated_rendering_for_texture(_forEyeIdx, _textureID, _setup);
			//DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR
			result = true;
		}
		else
		{
			warn(TXT("texture does not support foveation"));
		}
		::System::Video3D::get()->mark_use_texture(0, texture_id_none());
		::System::Video3D::get()->requires_send_use_textures();
	}
	return result;
}

void RenderTargetUtils::update_foveated_rendering_for_texture(Optional<int> _forEyeIdx, TextureID _textureID, FoveatedRenderingSetup const& _setup)
{
#ifdef ELABORATE_DEBUG_LOGGING
	output_gl(TXT("update_foveated_rendering_for_texture 0x%X"), _textureID);
#endif
	auto& glExtensions = ::System::VideoGLExtensions::get();
	if (_textureID != 0 &&
		glExtensions.QCOM_texture_foveated &&
		glExtensions.glTextureFoveationParametersQCOM)
	{
		// https://www.khronos.org/registry/OpenGL/extensions/QCOM/QCOM_texture_foveated.txt

		for_count(int, idx, FoveatedRenderingSetup::MAX_FOCAL_POINTS)
		{
			if (! _setup.focalPoints.is_empty())
			{
				// just repeat the last one
				auto& fp = _setup.focalPoints[min(idx, _setup.focalPoints.get_size() - 1)];
				DIRECT_GL_CALL_ glExtensions.glTextureFoveationParametersQCOM(_textureID, /*layer*/ 0, /*focal point*/idx, fp.at.x, fp.at.y, fp.gain.x, fp.gain.y, fp.foveArea); AN_GL_CHECK_FOR_ERROR
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
				output(TXT("[render-target-utils] fovren fp %i %S %S %.3f"), idx, fp.at.to_string().to_char(), fp.gain.to_string().to_char(), fp.foveArea);
#endif
			}
			else
			{
				// set all to none
				DIRECT_GL_CALL_ glExtensions.glTextureFoveationParametersQCOM(_textureID, /*layer*/ 0, /*focal point*/idx, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); AN_GL_CHECK_FOR_ERROR
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
				output(TXT("[render-target-utils] fovren fp %i none"), idx);
#endif
			}
		}

		//::System::Video3D::get()->mark_use_texture(0, 0);

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
		{
			Concurrency::ScopedSpinLock lock(g_lastFoveatedRenderingSetupsLock);
			if (!g_lastFoveatedRenderingSetups.has_place_left())
			{
				g_lastFoveatedRenderingSetups.pop_front();
			}
			{
				LastFoveatedRenderingSetup lfrs;
				lfrs.minDensity = _setup.minDensity;
				lfrs.focalPoints = _setup.focalPoints;
				lfrs.forEyeIdx = _forEyeIdx;
				lfrs.forTexture = _textureID;

				g_lastFoveatedRenderingSetups.push_back(lfrs);
			}
		}
#endif
	}
}

bool RenderTargetUtils::setup_foveated_rendering_for_framebuffer(Optional<int> _forEyeIdx, FrameBufferObjectID _frameBufferID, FoveatedRenderingSetup const& _setup)
{
	bool result = false;
	auto& glExtensions = ::System::VideoGLExtensions::get();
	if (_frameBufferID != 0 &&
		glExtensions.QCOM_framebuffer_foveated &&
		glExtensions.glFramebufferFoveationConfigQCOM &&
		glExtensions.glFramebufferFoveationParametersQCOM)
	{
		// https://www.khronos.org/registry/OpenGL/extensions/QCOM/QCOM_framebuffer_foveated.txt

		GLuint providedFeatures;
		glExtensions.glFramebufferFoveationConfigQCOM(_frameBufferID, 1, 1, GL_FOVEATION_ENABLE_BIT_QCOM, &providedFeatures);
		if (is_flag_set(providedFeatures, GL_FOVEATION_ENABLE_BIT_QCOM))
		{
			update_foveated_rendering_for_framebuffer(_forEyeIdx, _frameBufferID, _setup);
			result = true;
		}
		else
		{
			warn(TXT("frame buffer does not support foveation"));
		}
	}
	return result;
}

void RenderTargetUtils::update_foveated_rendering_for_framebuffer(Optional<int> _forEyeIdx, FrameBufferObjectID _frameBufferID, FoveatedRenderingSetup const& _setup)
{
	auto& glExtensions = ::System::VideoGLExtensions::get();
	if (_frameBufferID != 0 &&
		glExtensions.QCOM_framebuffer_foveated && 
		glExtensions.glFramebufferFoveationConfigQCOM &&
		glExtensions.glFramebufferFoveationParametersQCOM)
	{
		// https://www.khronos.org/registry/OpenGL/extensions/QCOM/QCOM_texture_foveated.txt

		for_count(int, idx, FoveatedRenderingSetup::MAX_FOCAL_POINTS)
		{
			if (_setup.focalPoints.get_size() > idx)
			{
				auto& fp = _setup.focalPoints[idx];
				DIRECT_GL_CALL_ glExtensions.glFramebufferFoveationParametersQCOM(_frameBufferID, /*layer*/ 0, /*focal point*/idx, fp.at.x, fp.at.y, fp.gain.x, fp.gain.y, fp.foveArea); AN_GL_CHECK_FOR_ERROR
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
				output(TXT("[render-target-utils] fovren fp %i %S %S %.3f"), idx, fp.at.to_string().to_char(), fp.gain.to_string().to_char(), fp.foveArea);
#endif
			}
			else
			{
				DIRECT_GL_CALL_ glExtensions.glFramebufferFoveationParametersQCOM(_frameBufferID, /*layer*/ 0, /*focal point*/idx, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); AN_GL_CHECK_FOR_ERROR
#ifdef DEBUG_FOVEATED_RENDERING_SETUP
				output(TXT("[render-target-utils] fovren fp %i none"), idx);
#endif
			}
		}
	}
}

void RenderTargetUtils::fit_into(VectorInt2 const& _sourceSize, VectorInt2 const& _destinationSize, OUT_ Vector2& _size)
{
	Vector2 sourceSize = _sourceSize.to_vector2();
	Vector2 destinationSize = _destinationSize.to_vector2();
	sourceSize.y = sourceSize.y * destinationSize.x / sourceSize.x;
	sourceSize.x = destinationSize.x;
	if (sourceSize.y > destinationSize.y)
	{
		sourceSize.x = sourceSize.x * destinationSize.y / sourceSize.y;
		sourceSize.y = destinationSize.y;
	}
	_size.x = (float)TypeConversions::Normal::f_i_closest(sourceSize.x);
	_size.y = (float)TypeConversions::Normal::f_i_closest(sourceSize.y);
}

void RenderTargetUtils::fit_into(VectorInt2 const& _sourceSize, VectorInt2 const& _destinationSize, OUT_ Vector2& _leftBottom, OUT_ Vector2& _size)
{
	fit_into(_sourceSize, _destinationSize, _size);
	_leftBottom.x = (float)TypeConversions::Normal::f_i_closest(((float)_destinationSize.x - _size.x) / 2);
	_leftBottom.y = (float)TypeConversions::Normal::f_i_closest(((float)_destinationSize.y - _size.y) / 2);
}

void RenderTargetUtils::fit_into(VectorInt2 const& _sourceSize, VectorInt2 const& _destinationSize, OUT_ VectorInt2& _size)
{
	Vector2 size;
	fit_into(_sourceSize, _destinationSize, size);
	_size.x = TypeConversions::Normal::f_i_closest(size.x);
	_size.y = TypeConversions::Normal::f_i_closest(size.y);
}
