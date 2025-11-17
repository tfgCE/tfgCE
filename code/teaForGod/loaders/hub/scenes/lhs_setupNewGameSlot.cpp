#include "lhs_setupNewGameSlot.h"

#include "lhs_startingPilgrimageSelection.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\screens\lhc_enterText.h"
#include "..\screens\lhc_message.h"
#include "..\screens\settings\lhc_gameDefinitionSelection.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameConfig.h"

#include "..\..\..\library\library.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// screens
DEFINE_STATIC_NAME(hubBackground);

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(SetupNewGameSlot);

void SetupNewGameSlot::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	//

	setup_state(true);
}

void SetupNewGameSlot::deactivate_screens()
{
	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));

	get_hub()->keep_only_screens(screensToKeep);
}

void SetupNewGameSlot::prev_state()
{
	if (state <= State::SetupGameDefinitionPart1 || TeaForGodEmperor::is_demo())
	{
		if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
		{
			deactivate_screens();
			game->add_async_world_job_top_priority(TXT("delete new game slot"),
				[this, game]()
				{
					auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
					if (gameSlot == ps.get_active_game_slot())
					{
						ps.remove_active_game_slot(true);
					}
					game->supress_saving_config_file(false);
					game->save_config_file();
					game->save_player_profile();
					game->set_meta_state(TeaForGodEmperor::GameMetaState::SelectGameSlot);

					deactivateHub = true;
				});
		}
	}

	state = (State)clamp<int>(state - 1, State::Init + 1, State::Done);

	setup_state(false);
}

void SetupNewGameSlot::next_state()
{
	state = (State)clamp<int>(state + 1, State::Init + 1, State::Done);

	setup_state(true);
}

void SetupNewGameSlot::setup_state(bool _asNext)
{
	deactivate_screens();

	if (startImmediately)
	{
		state = State::Done;
	}
	else
	{
		state = (State)clamp<int>(state, State::Init + 1, State::Done);
	}

	if (!gameSlot.is_set())
	{
		auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
		if (auto* gs = ps.access_active_game_slot())
		{
			gameSlot = gs;
		}
	}

	if (TeaForGodEmperor::is_demo())
	{
		if (state < State::Done)
		{
			state = State::SetupGameDefinitionPart3;
			if (auto* gs = gameSlot.get())
			{
				gs->force_game_definitions_for_demo();
			}
		}
	}

	switch (state)
	{
		case State::SetupGameDefinitionPart1:
		case State::SetupGameDefinitionPart2:
		case State::SetupGameDefinitionPart3:
		{
			HubScreens::GameDefinitionSelection::WhatToShow whatToShow = HubScreens::GameDefinitionSelection::WhatToShow::ShowSummary;
			if (state == SetupGameDefinitionPart1) whatToShow = HubScreens::GameDefinitionSelection::WhatToShow::SelectGameDefinition;
			if (state == SetupGameDefinitionPart2) whatToShow = HubScreens::GameDefinitionSelection::WhatToShow::SelectGameSubDefinition;
			if (state == SetupGameDefinitionPart2)
			{
				//if (firstTimePart2)
				if (_asNext) // if we enter from game definition selection to next only
				{
					firstTimePart2 = false;
					if (auto* gs = gameSlot.get())
					{
						gs->fill_default_game_sub_definitions();
					}
				}
			}
			HubScreens::GameDefinitionSelection::show(get_hub(), Name::invalid(), gameSlot.get(), whatToShow,
				[this]()
				{
					prev_state();
				},
				[this](bool _startImmedatiely)
				{
					startImmediately = _startImmedatiely;
					next_state();
				});
		} break;
		case State::Done:
		default:
		{
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				deactivate_screens();
				game->add_async_world_job_top_priority(TXT("done, setup pilgrimage"),
					[this, game]()
				{
					if (auto* gs = gameSlot.get())
					{
						// behave like we just started
						gs->clear_game_states_and_mission_state_and_pilgrimages();
					}
					game->save_config_file();
					game->save_player_profile();
					game->add_async_save_player_profile(false);
					if (startImmediately)
					{
						bool selectPilgrimage = false;
						if (auto* gs = gameSlot.get())
						{
							if (auto* gd = TeaForGodEmperor::GameDefinition::get_chosen())
							{
								gs->update_unlocked_pilgrimages(TeaForGodEmperor::PlayerSetup::get_current(), gd->get_pilgrimages(), gd->get_conditional_pilgrimages(), true);
							}
							if (!gs->unlockedPilgrimages.is_empty())
							{
								selectPilgrimage = true;
							}
						}
						if (selectPilgrimage)
						{
							game->add_immediate_sync_world_job(TXT("switch scene to pilgrimage selection"),
								[this]()
								{
									get_hub()->set_scene(new StartingPilgrimageSelection());
								});
						}
						else
						{
							game->set_meta_state(TeaForGodEmperor::GameMetaState::Pilgrimage); // play the game!
							deactivateHub = true;
						}
					}
					else
					{
						game->set_meta_state(TeaForGodEmperor::GameMetaState::PilgrimageSetup); // show the menu, allow to customise
						deactivateHub = true;
					}

				});
			}
		} break;
	}
}
