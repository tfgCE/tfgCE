#include "lhs_tutorialDone.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\scenes\lhs_beAtRightPlace.h"
#include "..\scenes\lhs_options.h"
#include "..\screens\lhc_handGestures.h"
#include "..\screens\lhc_question.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\tutorials\tutorialSystem.h"

//

// localised strings
DEFINE_STATIC_NAME_STR(lsRepeat, TXT("tutorials; post menu; repeat"));
DEFINE_STATIC_NAME_STR(lsNext, TXT("tutorials; post menu; next"));
DEFINE_STATIC_NAME_STR(lsExit, TXT("tutorials; post menu; exit"));

// screens
DEFINE_STATIC_NAME(tutorialDone);

// fade reasons
DEFINE_STATIC_NAME(backFromTutorialDone);

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(TutorialDone);

void TutorialDone::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	deactivateHub = false;
	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	show_menu();
}

void TutorialDone::show_menu()
{
	get_hub()->keep_only_screen(NAME(tutorialDone));

	// add buttons: go back to game, exit to main menu, quit game
	HubScreen* screen = get_hub()->get_screen(NAME(tutorialDone));
	if (! screen)
	{
		Vector2 size(15.0f, 30.0f);
		Vector2 ppa(20.0f, 20.0f);
		Rotator3 atDir = get_hub()->get_current_forward();
		//atDir.yaw += 30.0f; // when immediate stats are added

		screen = new HubScreen(get_hub(), NAME(tutorialDone), size, atDir, get_hub()->get_radius() * 0.9f, NP, NP, ppa);
		screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

		screen->activate();
		get_hub()->add_screen(screen);

		Array<HubScreenButtonInfo> buttons;
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsRepeat), [this]() { repeat(); }));
		if (auto* game = TeaForGodEmperor::Game::get_as< TeaForGodEmperor::Game>())
		{
			if (!game->is_post_run_tutorial())
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsNext), [this]() { next(); }).activate_on_trigger_hold());
			}
		}
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsExit), [this]() { exit(); }));

		Vector2 fs = HubScreen::s_fontSizeInPixels;
		screen->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
										Range2(Range(screen->mainResolutionInPixels.x.min + max(fs.x, fs.y), screen->mainResolutionInPixels.x.max - max(fs.x, fs.y)),
											   Range(screen->mainResolutionInPixels.y.min + max(fs.x, fs.y), screen->mainResolutionInPixels.y.max - max(fs.x, fs.y))),
										fs);

	}
}

void TutorialDone::exit()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->set_meta_state(game->get_post_tutorial_meta_state());
		deactivateHub = true;
	}
}

void TutorialDone::repeat()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		TeaForGodEmperor::TutorialSystem::get()->repeat_tutorial();
		TeaForGodEmperor::TutorialSystem::get()->pause_tutorial();
		game->set_meta_state(TeaForGodEmperor::GameMetaState::Tutorial);
		deactivateHub = true;
	}
}

void TutorialDone::next()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		TeaForGodEmperor::TutorialSystem::get()->start_tutorial(TeaForGodEmperor::TutorialSystem::get()->get_next_tutorial());
		TeaForGodEmperor::TutorialSystem::get()->pause_tutorial();
		game->set_meta_state(TeaForGodEmperor::GameMetaState::Tutorial);
		deactivateHub = true;
	}
}
