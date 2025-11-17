#include "lhs_setupNewPlayerProfile.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\screens\lhc_enterText.h"
#include "..\screens\lhc_message.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameConfig.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// screens
DEFINE_STATIC_NAME(hubBackground);

// localised strings

DEFINE_STATIC_NAME_STR(lsNewProfileName, TXT("hub; new player profile; new profile name"));
DEFINE_STATIC_NAME_STR(lsProvideNonEmptyName, TXT("hub; new player profile; provide non empty name"));
DEFINE_STATIC_NAME_STR(lsNameAlreadyTaken, TXT("hub; new player profile; name already taken"));

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(SetupNewPlayerProfile);

void SetupNewPlayerProfile::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		RefCountObjectPtr<HubScene> keepThis(this);
		game->add_async_world_job_top_priority(TXT("on new profile"),
			[this, keepThis, game]()
			{
				game->save_player_profile(); // save current profile
				
				// and reset, switch to no profile, so we have a clear plate
				game->switch_to_no_player_profile();

				nextStatePending = true;
			});
	}

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	//
}

void SetupNewPlayerProfile::deactivate_screens()
{
	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));

	get_hub()->keep_only_screens(screensToKeep);
}

void SetupNewPlayerProfile::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

	if (nextStatePending)
	{
		// do actual setup
		next_state();
	}
}

void SetupNewPlayerProfile::next_state()
{
	nextStatePending = false;

	deactivate_screens();
	
	state = (State)clamp<int>(state + 1, State::Init + 1, State::Done);

	if (state > State::EnterName)
	{	// store run setup after each state is done
		runSetup.update_using_this();
		//
		auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
		if (auto* gs = ps.access_active_game_slot())
		{
			gs->runSetup = runSetup;
		}
	}

	switch (state)
	{
		case State::EnterName:
		{
			ask_for_name();
		} break;
		case State::Done:
		default:
		{
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				deactivate_screens();

				game->add_async_world_job_top_priority(TXT("save profile"),
				[this, game]()
				{
					game->save_player_profile();
					game->set_meta_state(TeaForGodEmperor::GameMetaState::SetupNewGameSlot); // skip game slot selection, we want to get to the game as soon as possible
					deactivateHub = true;
				});
			}
		} break;
	}
}

void SetupNewPlayerProfile::ask_for_name(String _usingName)
{
	HubScreens::EnterText::show(get_hub(), LOC_STR(NAME(lsNewProfileName)), _usingName,
		[this](String const& _text)
		{
			if (_text.is_empty())
			{
				HubScreens::Message::show(get_hub(), NAME(lsProvideNonEmptyName), [this, _text]()
				{
					ask_for_name(_text);
				});
			}
			else if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				game->update_player_profiles_list(false /* do not select */);

				bool isDuplicated = false;
				for_every(pp, game->get_player_profiles())
				{
					if (_text == *pp)
					{
						isDuplicated = true;
					}
				}
				if (isDuplicated)
				{
					HubScreens::Message::show(get_hub(), LOC_STR(NAME(lsNameAlreadyTaken)), [this, _text]()
					{
						ask_for_name(_text);
					});
				}
				else
				{
					add_new_profile(_text);
				}
			}
		},
		[this]()
		{
			deactivateHub = true;
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				// go back
				game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectPlayerProfile);
			}
		},
		false, true);

}

void SetupNewPlayerProfile::add_new_profile(String const& _usingName)
{
	deactivate_screens();

	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->add_async_world_job_top_priority(TXT("add new profile"),
		[this, game, _usingName]()
		{
			game->use_quick_game_player_profile(false);
			game->choose_player_profile(_usingName, true);
			auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
			ps.have_at_least_one_game_slot(); // required to be setup
			game->save_player_profile(); // save new profile

			// read setup
			runSetup.read_into_this();
			//
			if (auto* gs = ps.get_active_game_slot())
			{
				runSetup = gs->runSetup;
			}

			nextStatePending = true;
		});
	}

}
