#include "screenShotContext.h"

#include <time.h>

#include "game.h"
#include "gameConfig.h"

#include "..\..\core\io\dir.h"
#include "..\..\core\system\video\pixelUtils.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\renderTargetSave.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define QUEST_TO_MAIN_SCREENSHOTS

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(vrScreenshot);
DEFINE_STATIC_NAME(screenshot);

// input tags
DEFINE_STATIC_NAME(allowVRScreenShots); // using controllers

//

ScreenShotContext* ScreenShotContext::s_active = nullptr;

ScreenShotContext::ScreenShotContext(Game* _game)
: game(_game)
{
	if (!game)
	{
		game = Game::get_as<Game>();
	}
}

ScreenShotContext::~ScreenShotContext()
{
	if (s_active == this)
	{
		s_active = nullptr;
	}
}

void ScreenShotContext::pre_render()
{
	if (auto* input = ::System::Input::get())
	{
		todo_note(TXT("this is done now every frame!"));
		bool allowVRScreenShots = game->allowScreenShots;
		if (game->does_use_vr())
		{
			if (GameConfig const* gc = GameConfig::get_as<GameConfig>())
			{
				if (gc->get_allow_vr_screen_shots() == AllowScreenShots::No)
				{
					allowVRScreenShots = false;
				}
			}
		}
		if (allowVRScreenShots)
		{
			input->set_usage_tag(NAME(allowVRScreenShots));
		}
		else
		{
			input->remove_usage_tag(NAME(allowVRScreenShots));
		}
	}

	doScreenShot = false;
	if (game->allowScreenShots)
	{
		doScreenShot |= game->allTimeControlsInput.has_button_been_pressed(NAME(screenshot));
		if (game->does_use_vr())
		{
			if (GameConfig const * gc = GameConfig::get_as<GameConfig>())
			{
				if (gc->get_allow_vr_screen_shots() != AllowScreenShots::No)
				{
					doScreenShot |= game->allTimeControlsInput.has_button_been_pressed(NAME(vrScreenshot));
				}
			}
		}
	}
#ifdef RENDER_TO_BIG_PANELS
	if (doScreenShot)
	{
		screenShotIdx = 0;
		screenShotCount = 1;
		screenShotViewCentreOffsetX = 0.0f;
		if (::System::Input::get()->is_key_pressed(::System::Key::N1))
		{
			screenShotCount = 2;
		}
		if (::System::Input::get()->is_key_pressed(::System::Key::N2))
		{
			screenShotCount = 4;
		}
		if (::System::Input::get()->is_key_pressed(::System::Key::N3))
		{
			screenShotCount = 4;
			screenShotViewCentreOffsetX = 1.0f;
		}
	}
	if (!doScreenShot && screenShotCount > 0)
	{
		++screenShotIdx;
		doScreenShot = screenShotIdx < screenShotCount;
	}
	if (doScreenShot && screenShotCount > 0)
	{
		Vector2 relViewCentreOffset = Vector2::zero;
		float count = (float)screenShotCount;
		float idx = (float)screenShotIdx;
		/*
		we look at (but view centre is negative to this)
		1		0
		2		-0.5	0.5
		3		-1		0		1
		4		-1.5	-0.5	0.5		1
		but we're dealing with view centre offset so remember it goes -1 to 1 for one screen
		*/
		relViewCentreOffset.x = (count - 1.0f) - idx * 2.0f + screenShotViewCentreOffsetX;
		for_count(int, i, 2)
		{
			screenShotCustomRenderContext[i].relViewCentreOffsetForCamera = relViewCentreOffset;
		}
	}
	else
	{
		doScreenShot = false;
		screenShotCount = 0;
	}
#endif

	useCustomRenderContext = nullptr;
	if (doScreenShot)
	{
		if (!s_active)
		{
			s_active = this;
		}
		output(TXT("screenshot!"));
		if (game->does_use_vr())
		{
			// for vr we control it with option
			screenShotCustomRenderContextIdx = 0;
			if (GameConfig const* gc = GameConfig::get_as<GameConfig>())
			{
				if (gc->get_allow_vr_screen_shots() == AllowScreenShots::Small)
				{
					screenShotCustomRenderContextIdx = 1;
				}
			}
		}
		game->make_sure_screen_shot_custom_render_context_is_valid(screenShotCustomRenderContextIdx);
		useCustomRenderContext = &game->screenShotCustomRenderContext[screenShotCustomRenderContextIdx];
	}
	else
	{
		// here to keep this value when doing multiple screenshots
		screenShotCustomRenderContextIdx = 0;
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift) ||
			::System::Input::get()->is_key_pressed(::System::Key::RightShift))
		{
			screenShotCustomRenderContextIdx = 1;
		}
#endif
	}
}

void ScreenShotContext::store_screen_shot(::System::RenderTarget* rt)
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("store screenshot!"));
#endif

	String fileOut;
	{
		bool createScreenShotDir = false;
#ifndef AN_ANDROID
		createScreenShotDir = true;
#else
#ifndef QUEST_TO_MAIN_SCREENSHOTS
		createScreenShotDir = true;
#endif
#endif
		{
			static int screenShotIdx = 0;
			time_t currentTime = time(0);
			tm currentTimeInfo;
			tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
			pcurrentTimeInfo = localtime(&currentTime);
#else
			localtime_s(&currentTimeInfo, &currentTime);
#endif
			fileOut = IO::get_user_game_data_directory() + String::printf(TXT("_screenshots%S"), IO::get_directory_separator());

#ifdef AN_ANDROID
#ifdef QUEST_TO_MAIN_SCREENSHOTS
			{
				String dataFolder = IO::get_user_game_data_directory();
				int androidAt = dataFolder.find_first_of(String(TXT("Android")));
				if (androidAt != NONE)
				{
					fileOut = dataFolder.get_left(androidAt) + String::printf(TXT("Oculus%SScreenshots%S"), IO::get_directory_separator(), IO::get_directory_separator());
					fileOut += String(TXT("Tea For God "));
				}
				else
				{
					createScreenShotDir = true;
				}
			}
#endif
#endif

			fileOut += String::printf(TXT("%04i-%02i-%02i %02i-%02i-%02i %03i"),
				pcurrentTimeInfo->tm_year + 1900, pcurrentTimeInfo->tm_mon + 1, pcurrentTimeInfo->tm_mday,
				pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec, screenShotIdx);
			++screenShotIdx;
		}

		if (createScreenShotDir)
		{
			IO::Dir::create(IO::get_user_game_data_directory() + String(TXT("_screenshots")));
		}

	}

	{
		::System::RenderTargetSave rts(rt);

#ifdef BUILD_NON_RELEASE
		if (::System::RenderTargetSaveSpecialOptions::get().transparent)
		{
			rts.store_as_rgba();
		}
#endif

#ifdef AN_ALLOW_BULLSHOTS
		if (Framework::BullshotSystem::is_active())
		{
			VectorInt2 screenSize = rt->get_size();
			rts.frame_to(Framework::BullshotSystem::calculate_show_frame_for(screenSize));
		}
#endif


#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT("to file %S"), fileOut.to_char());
#endif

		rts.save(fileOut, true);
	}

	output_colour(1, 1, 0, 1);
	output(TXT("screenshot > %S"), fileOut.to_char());
	output_colour();
}

void ScreenShotContext::post_render()
{
	if (doScreenShot)
	{
		store_screen_shot(useCustomRenderContext->sceneRenderTarget.get());

		game->gameIcons.screenshotIcon.activate(0.5f, true);
	}
}
