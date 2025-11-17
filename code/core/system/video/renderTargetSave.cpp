#include "renderTargetSave.h"

#include <time.h>

#include "pixelUtils.h"
#include "renderTarget.h"
#include "video3d.h"

#include "imageSavers\is_tga.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

//#define SAVE_AS_PNG
#define SAVE_AS_TGA

//

using namespace System;

//

#ifdef BUILD_NON_RELEASE
RenderTargetSaveSpecialOptions RenderTargetSaveSpecialOptions::s_renderTargetSaveSpecialOptions;
#endif

//

bool RenderTargetSave::save(String const & fileOut, bool _addExtension)
{
	bool result = false;

	System::Video3D* v3d = System::Video3D::get();
	rt->resolve_forced_unbind();
	v3d->unbind_frame_buffer_bare();
	DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, rt->get_frame_buffer_object_id_to_read()); AN_GL_CHECK_FOR_ERROR
	VectorInt2 screenSize = rt->get_size();
	{
		bool readRGBA = true;
		bool storeRGBA = this->storeRGBA;
		bool asPNG = false;
		bool asTGA = false;
#ifdef SAVE_AS_PNG
		asPNG = true;
#endif
#ifdef SAVE_AS_TGA
		asTGA = true;
		asPNG = false;
#endif

#ifdef BUILD_NON_RELEASE
		if (RenderTargetSaveSpecialOptions::get().transparent)
		{
			storeRGBA = true;
		}
#endif

		bool convertSRGB = rt->get_setup().should_use_srgb_conversion();
		if (auto* v3d = ::System::Video3D::get())
		{
			convertSRGB |= v3d->get_system_implicit_srgb();
		}

		int bytes = readRGBA ? 4 : 3;
		Array<byte> upsideDownPixels;
		upsideDownPixels.set_size(bytes * screenSize.x * screenSize.y);
		{
			DIRECT_GL_CALL_ glReadPixels(0, 0, screenSize.x, screenSize.y, readRGBA ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, upsideDownPixels.get_data()); AN_GL_CHECK_FOR_ERROR

			if (readRGBA && !storeRGBA)
			{
				System::PixelUtils::remove_alpha(screenSize, upsideDownPixels, REF_ bytes);
				readRGBA = false;
			}
			if (!readRGBA && storeRGBA)
			{
				System::PixelUtils::add_alpha(screenSize, upsideDownPixels, REF_ bytes);
				readRGBA = true;
			}

			if (convertSRGB)
			{
				System::PixelUtils::convert_linear_to_srgb(screenSize, upsideDownPixels, bytes);
			}

			bool createScreenShotDir = false;
#ifndef AN_ANDROID
			createScreenShotDir = true;
#else
#ifndef QUEST_TO_MAIN_SCREENSHOTS
			createScreenShotDir = true;
#endif
#endif
			VectorInt2 dropSize = screenSize;
			Array<byte> framedUpsideDownPixels;
			bool dataIsUpsideDown = true;

			{
				RangeInt2 useFrameTo = frameTo;

				if (useFrameTo.is_empty())
				{
					useFrameTo.include(VectorInt2::zero);
					useFrameTo.include(screenSize - VectorInt2::one);
				}

				dropSize = useFrameTo.length();

				if ((dropSize.x % 4) != 0)
				{
					warn(TXT("this is going to break... make screen size x mutliplication of 4"));
				}
				dropSize.x -= (dropSize.x % 4);

				if (dropSize == screenSize)
				{
					framedUpsideDownPixels = upsideDownPixels;
				}
				else
				{
					framedUpsideDownPixels.set_size(bytes * dropSize.x * dropSize.y);
					int srcLineSize = bytes * screenSize.x;
					int dstLineSize = bytes * dropSize.x;
					byte* src = &upsideDownPixels[useFrameTo.y.min * srcLineSize + bytes * useFrameTo.x.min];
					byte* dst = framedUpsideDownPixels.get_data();
					for (int y = 0; y < useFrameTo.y.length(); ++y)
					{
						/*
						output(TXT("%i/%i src:%i/%i +%i  dst:%i/%i +%i"), y, frameTo.y.length()
							, src - upsideDownPixels.get_data(), upsideDownPixels.get_size(), srcLineSize
							, dst - framedUpsideDownPixels.get_data(), framedUpsideDownPixels.get_size(), dstLineSize
							);
						*/
						memory_copy(dst, src, dstLineSize);
						src += srcLineSize;
						dst += dstLineSize;
					}
				}
			}

			String useFileOut = fileOut;
			if (_addExtension)
			{
				if (asTGA)
				{
					useFileOut += TXT(".tga");
				}
				else
				{
					an_assert(asPNG);
					useFileOut += TXT(".png");
				}
			}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("to file %S"), useFileOut.to_char());
#endif

			if (asTGA)
			{
				::System::ImageSaver::TGA tgaSaver;
				tgaSaver.set_bytes_per_pixel(bytes /* RGB */);
				tgaSaver.set_size(dropSize);
				an_assert(framedUpsideDownPixels.get_size() == bytes * dropSize.x * dropSize.y);
				memory_copy(&tgaSaver.access_data()[0], framedUpsideDownPixels.get_data(), framedUpsideDownPixels.get_size());
				tgaSaver.save(useFileOut, dataIsUpsideDown);

				result = true;
			}
			else
			{
				an_assert(asPNG);
				todo_implement(TXT("save as png?"));
			}
		}
	}
	DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR
	v3d->invalidate_bound_frame_buffer_info();
	v3d->unbind_frame_buffer(); // to get back from bare

	return result;
}


