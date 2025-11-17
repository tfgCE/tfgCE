#include "lhc_tutInGameMenu.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\scenes\lhs_beAtRightPlace.h"
#include "..\scenes\lhs_tutInGameMenu.h"
#include "..\widgets\lhw_button.h"

#include "..\..\..\game\game.h"
#include "..\..\..\tutorials\tutorialSystem.h"

//

// localised strings
DEFINE_STATIC_NAME_STR(lsGoBackToTutorial, TXT("hub; common; back"));
DEFINE_STATIC_NAME_STR(lsEndTutorial, TXT("tutorials; in game menu; end tutorial"));

// screens
DEFINE_STATIC_NAME(tutInGameMenu);

// ids
DEFINE_STATIC_NAME(endTutorial);

//

using namespace Loader;
using namespace HubScreens;

//

REGISTER_FOR_FAST_CAST(TutInGameMenu);

TutInGameMenu* TutInGameMenu::show_for_hub(Hub* _hub)
{
	TutInGameMenu* tigm = new TutInGameMenu(_hub);
	tigm->activate();
	_hub->add_screen(tigm);

	return tigm;
}


TutInGameMenu::TutInGameMenu(Hub* _hub)
: base(_hub, NAME(tutInGameMenu), Vector2(15.0f, 20.0f), _hub->get_current_forward(), _hub->get_radius() * 0.5f, NP, NP, Vector2(10.0f, 10.0f))
{
	be_modal();

	followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

	Array<HubScreenButtonInfo> buttons;
	buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsGoBackToTutorial), [this]() { go_back_to_game(); }).activate_on_trigger_hold());
	buttons.push_back(HubScreenButtonInfo(NAME(endTutorial), NAME(lsEndTutorial), [this]() { end_tutorial(); }));

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()),
		Range2(Range(mainResolutionInPixels.x.min + max(fs.x, fs.y), mainResolutionInPixels.x.max - max(fs.x, fs.y)),
			Range(mainResolutionInPixels.y.min + max(fs.x, fs.y), mainResolutionInPixels.y.max - max(fs.x, fs.y))),
		fs);
}

void TutInGameMenu::go_back_to_game()
{
	if (tutInGameMenuScene)
	{
		if (HubScenes::BeAtRightPlace::is_required(hub))
		{
			hub->set_scene((new HubScenes::BeAtRightPlace())->with_go_back_button());
		}
		else
		{
			hub->show_start_movement_centre(false);
			hub->set_beacon_active(false);
			if (tutInGameMenuScene)
			{
				tutInGameMenuScene->deactivateHub = true;
			}
		}
	}
	else
	{
		deactivate();
	}
}

void TutInGameMenu::end_tutorial()
{
	hub->show_start_movement_centre(false);
	hub->set_beacon_active(false);
	hub->deactivate_all_screens();
	TeaForGodEmperor::TutorialSystem::get()->end_tutorial();
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		// will end mode
		game->request_post_game_summary(TeaForGodEmperor::TutorialSystem::get()->is_success_forced()? TeaForGodEmperor::PostGameSummary::ReachedEnd : TeaForGodEmperor::PostGameSummary::Interrupted);
		if (auto* hub = game->get_recent_hub_loader())
		{
			hub->force_deactivate();
		}
	}
	if (tutInGameMenuScene)
	{
		tutInGameMenuScene->deactivateHub = true;
	}
}

void TutInGameMenu::be_stand_alone(HubScenes::TutInGameMenu* _tutInGameMenuScene)
{
	tutInGameMenuScene = _tutInGameMenuScene;
}

void TutInGameMenu::advance(float _deltaTime, bool _beyondModal)
{
	base::advance(_deltaTime, _beyondModal);

	bool buttonsEnabled = TeaForGodEmperor::TutorialSystem::is_hub_input_allowed();
	{
		for_every_ref(w, widgets)
		{
			if (auto* b = fast_cast<HubWidgets::Button>(w))
			{
				b->enable(buttonsEnabled);
			}
		}
	}
}
