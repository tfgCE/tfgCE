#include "recentCapture.h"

#include "video/renderTarget.h"

#include "../concurrency/scopedSpinLock.h"

//

using namespace System;

//

// output textures
DEFINE_STATIC_NAME(colour);

//

RecentCapture* RecentCapture::s_recentCapture = nullptr;

void RecentCapture::initialise_static()
{
#ifdef WITH_RECENT_CAPTURE
	// works as reinitialise
	if (!s_recentCapture)
	{
		s_recentCapture = new RecentCapture();
	}
#endif
}

void RecentCapture::close_static()
{
#ifdef WITH_RECENT_CAPTURE
	delete_and_clear(s_recentCapture);
#endif
}

void RecentCapture::store(System::RenderTarget* _what, bool _fullInGame)
{
#ifdef WITH_RECENT_CAPTURE
	assert_rendering_thread();

	if (!s_recentCapture)
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(s_recentCapture->lock);

	VectorInt2 resolution = _what->get_size(true);

	int biggerSize = max(resolution.x, resolution.y);
	if (biggerSize > MAX_SIZE)
	{
		resolution = TypeConversions::Normal::f_i_closest(resolution.to_vector2() * (float)MAX_SIZE / (float)biggerSize);
	}

	for_count(int, i, RecentCaptureIndex::MAX)
	{
#ifdef RECENT_CAPTURE_ONLY_FULL_GAME
		if (i != FullInGame)
		{
			continue;
		}
#endif
		if (!_fullInGame && i == FullInGame)
		{
			continue;
		}
		System::RenderTargetPtr& storeIn = s_recentCapture->recentCapture[i];

		if (storeIn.get() && storeIn->get_size(true) != resolution)
		{
			storeIn->close(true);
			storeIn.clear();
		}

		if (!storeIn.get())
		{
			::System::RenderTargetSetup rtSetup(resolution);
			rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
				::System::VideoFormat::rgba8,
				::System::BaseVideoFormat::rgba));


			RENDER_TARGET_CREATE_INFO(TXT("recent capture"));
			storeIn = new ::System::RenderTarget(rtSetup);
		}

		auto* v3d = ::System::Video3D::get();

		v3d->setup_for_2d_display();

		v3d->set_viewport(VectorInt2::zero, storeIn->get_size(true));
		
		v3d->set_near_far_plane(0.02f, 100.0f);

		v3d->set_2d_projection_matrix_ortho();
		v3d->access_model_view_matrix_stack().clear();

		_what->resolve();

		storeIn->bind();
		_what->render(0, v3d, Vector2::zero, storeIn->get_size(true).to_vector2());
		storeIn->unbind();
	}
#endif
}

void RecentCapture::force_next_store_pixel_data()
{
#ifdef WITH_RECENT_CAPTURE
	if (s_recentCapture)
	{
		s_recentCapture->forceStorePixelData = true;
	}
#endif
}

void RecentCapture::store_pixel_data()
{
#ifdef WITH_RECENT_CAPTURE
	assert_rendering_thread();

#ifndef RECENT_CAPTURE_STORE_PIXEL_DATA_EVERY_FRAME
	if (!s_recentCapture || !s_recentCapture->forceStorePixelData)
	{
		return;
	}
#else
	if (!s_recentCapture)
	{
		return;
	}
#endif

	Concurrency::ScopedSpinLock lock(s_recentCapture->lock);

	s_recentCapture->forceStorePixelData = false;

	System::Video3D* v3d = System::Video3D::get();

	for_count(int, i, RecentCaptureIndex::MAX)
	{
		bool const readRGBA = true;
		int bytes = readRGBA ? 4 : 3;
		auto& recentCapture = s_recentCapture->recentCapture[i];
		auto& upsideDownPixels = s_recentCapture->recentCaptureRGBAUpsideDownPixels[i];
		auto& upsideDownPixelsSize = s_recentCapture->recentCaptureRGBAUpsideDownPixelsSize[i];
		if (recentCapture.get())
		{
			VectorInt2 size = recentCapture->get_size(true);
			size.x = size.x - mod(size.x, 4); // align, skip the extra pixels
			upsideDownPixels.set_size(bytes * size.x * size.y);

			{
				recentCapture->resolve_forced_unbind();
				v3d->unbind_frame_buffer_bare();
				DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, recentCapture->get_frame_buffer_object_id_to_read()); AN_GL_CHECK_FOR_ERROR
				//
				DIRECT_GL_CALL_ glReadPixels(0, 0, size.x, size.y, readRGBA ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, upsideDownPixels.get_data()); AN_GL_CHECK_FOR_ERROR
				upsideDownPixelsSize = size;
				//
				DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR
				v3d->invalidate_bound_frame_buffer_info();
				v3d->unbind_frame_buffer(); // to get back from bare
			}
		}
		else
		{
			upsideDownPixels.clear();
			upsideDownPixelsSize = VectorInt2::zero;
		}
	}
#endif
}

bool RecentCapture::read_pixel_data(RecentCaptureIndex _what, OUT_ VectorInt2& _resolution, OUT_ Array<byte>& _upsideDownPixels, OUT_ int& _bytesPerPixel, OUT_ bool& _convertSRGB)
{
#ifdef WITH_RECENT_CAPTURE
	if (!s_recentCapture)
	{
		return false;
	}

	Concurrency::ScopedSpinLock lock(s_recentCapture->lock);

	if (!s_recentCapture->recentCapture[_what].get() ||
		s_recentCapture->recentCaptureRGBAUpsideDownPixels[_what].is_empty() ||
		s_recentCapture->recentCaptureRGBAUpsideDownPixelsSize[_what].is_zero())
	{
		return false;
	}

	_upsideDownPixels = s_recentCapture->recentCaptureRGBAUpsideDownPixels[_what];
	_resolution = s_recentCapture->recentCaptureRGBAUpsideDownPixelsSize[_what];
	_bytesPerPixel = 4 /* RGBA */;
	_convertSRGB = s_recentCapture->recentCapture[_what]->get_setup().should_use_srgb_conversion();
	if (auto* v3d = ::System::Video3D::get())
	{
		_convertSRGB |= v3d->get_system_implicit_srgb();
	}

	return true;
#else
	return false;
#endif
}
