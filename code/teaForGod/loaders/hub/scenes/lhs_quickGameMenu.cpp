#include "lhs_quickGameMenu.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\scenes\lhs_calibrate.h"
#include "..\scenes\lhs_handyMessages.h"
#include "..\scenes\lhs_options.h"
#include "..\scenes\lhs_options.h"
#include "..\scenes\lhs_playerProfileSelection.h"
#include "..\screens\lhc_handGestures.h"
#include "..\screens\lhc_messageSetBrowser.h"
#include "..\screens\lhc_question.h"
#include "..\screens\settings\lhc_gameModifiers.h"
#include "..\screens\prayStations\lhc_spirographMandala.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_slider.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\library\gameDefinition.h"
#include "..\..\..\library\messageSet.h"

#include "..\..\..\..\core\buildInformation.h"
#include "..\..\..\..\framework\debug\testConfig.h"
#include "..\..\..\..\framework\library\library.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// screens
DEFINE_STATIC_NAME(hubBackground);
DEFINE_STATIC_NAME(quickGameVersionInfo);
DEFINE_STATIC_NAME(quickGameMainPanel);
DEFINE_STATIC_NAME(quickGameMenu);

// ids
DEFINE_STATIC_NAME(idCalibrate);

// localised strings
DEFINE_STATIC_NAME_STR(lsChooseLoadout, TXT("hub; common; choose loadout"));
DEFINE_STATIC_NAME_STR(lsStart, TXT("hub; common; start"));
DEFINE_STATIC_NAME_STR(lsBack, TXT("hub; common; back"));
DEFINE_STATIC_NAME_STR(lsCalibrate, TXT("hub; game menu; calibrate"));
DEFINE_STATIC_NAME_STR(lsOptions, TXT("hub; game menu; options"));

DEFINE_STATIC_NAME_STR(lsCustomDifficultySetup, TXT("hub; setup run; custom difficulty; setup"));
DEFINE_STATIC_NAME_STR(lsQuitGame, TXT("hub; game menu; quit game"));
DEFINE_STATIC_NAME_STR(lsQuitGameQuestion, TXT("hub; question; quit game"));

DEFINE_STATIC_NAME_STR(lsShowHandGestures, TXT("hand gestures; show"));

// language icon
DEFINE_STATIC_NAME(small);

// input
DEFINE_STATIC_NAME(tabLeft);
DEFINE_STATIC_NAME(tabRight);

// difficulty settings
DEFINE_STATIC_NAME(experience);
DEFINE_STATIC_NAME(custom);

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(QuickGameMenu);

void QuickGameMenu::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	deactivateHub = false;
	
	runSetup.read_into_this();
	//
	auto& ps = TeaForGodEmperor::PlayerSetup::get_current();
	if (auto* gs = ps.get_active_game_slot())
	{
		runSetup = gs->runSetup;
	}
	//
	runSetup.update_using_this();

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(quickGameVersionInfo));
	screensToKeep.push_back(NAME(quickGameMainPanel));
	screensToKeep.push_back(NAME(quickGameMenu));
	screensToKeep.push_back(NAME(hubBackground));
	//
	get_hub()->keep_only_screens(screensToKeep);

	//HubScreens::SpirographMandala::show(get_hub(), NAME(hubBackground));

	float radius = get_hub()->get_radius() * 0.6f;

	{	// top info
		HubScreen* screen = get_hub()->get_screen(NAME(quickGameVersionInfo));

		if (!screen)
		{
			Vector2 ppa = Vector2(24.0f, 24.0f);

			String text(get_build_version());

			Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();
			atDir.pitch = 26.0f;

			Vector2 sizeInPixels = Vector2(text.get_length() + 0.0f, 1.0f) * HubScreen::s_fontSizeInPixels;

			auto* img = get_hub()->get_graphics(HubGraphics::LogoBigOneLine);

			Vector2 imgScale = Vector2(1.0f, 1.0f);
			Vector2 borderSize = Vector2(3.0f, 3.0f);
			float spacing = 3.0f;
			if (img)
			{
				Vector2 imgSize = img->calc_rect().length().to_vector2();
				sizeInPixels.x = max(sizeInPixels.x, (float)imgSize.x * imgScale.x);
				sizeInPixels.y += (float)imgSize.y * imgScale.y;
				sizeInPixels.y += spacing;
			}

			sizeInPixels += borderSize * 2.0f;

			screen = new HubScreen(get_hub(), NAME(quickGameVersionInfo), sizeInPixels / ppa,
				atDir, get_hub()->get_radius() * 0.5f, true, NP, ppa);

			{
				Range2 at = screen->mainResolutionInPixels;
				at.y.min += borderSize.y;
				auto* w = new HubWidgets::Text(Name::invalid(), at, text);
				w->alignPt.y = 0.0f;
				screen->add_widget(w);
			}

			if (img)
			{
				Range2 at = screen->mainResolutionInPixels;
				at.y.max -= borderSize.y;
				auto* w = new HubWidgets::Image(Name::invalid(), at, img, imgScale, HubColours::text());
				w->alignPt.y = 1.0f;
				screen->add_widget(w);
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
	} 
	

	// main panel
	{
		HubScreen* screen = get_hub()->get_screen(NAME(quickGameMainPanel));
		if (!screen)
		{
			Vector2 size(50.0f, 50.0f);
			Vector2 ppa(30.0f, 30.0f);
			Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();
			atDir.yaw += -10.0f;

			screen = new HubScreens::GameModifiers(get_hub(), false, NAME(quickGameMainPanel), runSetup, nullptr, NP, nullptr,
				atDir, size, radius, ppa);

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}

	// menu
	{
		HubScreen* screen = get_hub()->get_screen(NAME(quickGameMenu));

		if (!screen)
		{
			Vector2 size(15.0f, 35.0f);
			Vector2 ppa(16.0f, 16.0f);
			Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();
			atDir.yaw += 30.0f;

			screen = new HubScreen(get_hub(), NAME(quickGameMenu), size, atDir, radius);

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsStart), [this]() { start_pilgrimage(); }).activate_on_trigger_hold().with_scale(2.0f));
			buttons.push_back(HubScreenButtonInfo(NAME(idCalibrate), NAME(lsCalibrate), [this]() { get_hub()->set_scene(new HubScenes::Calibrate()); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOptions), [this]() { get_hub()->set_scene(new HubScenes::Options(HubScenes::Options::Main)); }));
			if (auto* vr = VR::IVR::get())
			{
				if (vr->is_using_hand_tracking())
				{
					buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsShowHandGestures), [this]() { HubScreens::HandGestures::show(get_hub()); }));
				}
			}
			//buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { go_back_to_player_profile_selection(); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsQuitGame),
				[this]()
				{
					HubScreens::Question::ask(get_hub(), NAME(lsQuitGameQuestion),
						[this]()
						{
							Array<Name> screensToKeep;
							screensToKeep.push_back(NAME(hubBackground));

							get_hub()->keep_only_screens(screensToKeep);

							get_hub()->quit_game();
						},
						[]()
						{
							// we'll just get back
						}
					);
				}));

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
				screen->mainResolutionInPixels.expanded_by(-fs), fs);

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}
}

void QuickGameMenu::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

#ifdef AN_STANDARD_INPUT
	if (System::Input::get()->has_key_been_pressed(System::Key::Space))
	{
		deactivateHub = true;
	}
#endif
	
	if (auto* screen = get_hub()->get_screen(NAME(quickGameMenu)))
	{
		if (auto* w = fast_cast<HubWidgets::Button>(screen->get_widget(NAME(idCalibrate))))
		{
			float doorHeight = TeaForGodEmperor::PlayerPreferences::get_door_height_from_eye_level();

			w->set_highlighted(abs(TeaForGodEmperor::PlayerSetup::get_current().get_preferences().doorHeight - doorHeight) >= HubScenes::Calibrate::max_distance);
		}
	}
}

void QuickGameMenu::store_run_setup()
{
	runSetup.update_using_this();
	//
	auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
	if (auto* gs = ps.access_active_game_slot())
	{
		gs->runSetup = runSetup;
	}
}

void QuickGameMenu::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
}

void QuickGameMenu::on_loader_deactivate()
{
	base::on_loader_deactivate();
}

#define HIGHLIGHT_DIFFICULTY(diffName, diffWidget) \
	if (auto * w = fast_cast<HubWidgets::Button>(diffWidget.get())) \
	{ \
		w->set_highlighted(choseDifficulty == diffName); \
	}

void QuickGameMenu::go_back_to_player_profile_selection()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		//
		get_hub()->keep_only_screens(screensToKeep);

		game->add_async_world_job_top_priority(TXT("exit to player profile selection"),
		[this, game]()
		{
			store_run_setup();
			game->save_player_profile();
			game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPlayerProfile);

			deactivateHub = true;
		});
	}
}

void QuickGameMenu::start_pilgrimage()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		//
		get_hub()->keep_only_screens(screensToKeep);

		game->add_async_world_job_top_priority(TXT("exit to player profile selection"),
			[this, game]()
		{
			store_run_setup();
			game->save_player_profile();
			game->set_meta_state(TeaForGodEmperor::GameMetaState::Pilgrimage);

			deactivateHub = true;
		});
	}
}

void QuickGameMenu::change_difficulty(int _dir)
{
	todo_important(TXT("shouldn't we remove this?"));
}