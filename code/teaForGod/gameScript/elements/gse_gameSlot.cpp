#include "gse_gameSlot.h"

#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::GameSlot::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	udpateUnlockedPilgrimages = _node->get_bool_attribute_or_from_child_presence(TXT("udpateUnlockedPilgrimages"), udpateUnlockedPilgrimages);
	udpateRestartAtPilgrimage = _node->get_bool_attribute_or_from_child_presence(TXT("udpateRestartAtPilgrimage"), udpateRestartAtPilgrimage);
	unlockMissionsGameSlotMode = _node->get_bool_attribute_or_from_child_presence(TXT("unlockMissionsGameSlotMode"), unlockMissionsGameSlotMode);

	clearGameStates = _node->get_bool_attribute_or_from_child_presence(TXT("clearGameStates"), clearGameStates);
	clearMissionState = _node->get_bool_attribute_or_from_child_presence(TXT("clearMissionState"), clearMissionState);

	return result;
}

bool Elements::GameSlot::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::GameSlot::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* g = Game::get_as<Game>())
	{
		if (udpateUnlockedPilgrimages || udpateRestartAtPilgrimage)
		{
			auto urapCopy = udpateRestartAtPilgrimage;
			g->add_immediate_sync_world_job(TXT("update unlocked pilgrimages"), [urapCopy]()
				{
					if (auto* ps = PlayerSetup::access_current_if_exists())
					{
						if (auto* gs = ps->access_active_game_slot())
						{
							gs->sync_update_unlocked_pilgrimages_from_game_definition(*ps, urapCopy);
						}
					}
				});
		}
		if (clearGameStates)
		{
			g->add_immediate_sync_world_job(TXT("clear game states"), []()
				{
					if (auto* ps = PlayerSetup::access_current_if_exists())
					{
						if (auto* gs = ps->access_active_game_slot())
						{
							gs->clear_game_states();
						}
					}
				});
		}
		if (clearMissionState)
		{
			g->add_immediate_sync_world_job(TXT("clear mission state"), []()
				{
					if (auto* ps = PlayerSetup::access_current_if_exists())
					{
						if (auto* gs = ps->access_active_game_slot())
						{
							gs->clear_mission_state();
						}
					}
				});
		}
		if (unlockMissionsGameSlotMode)
		{
			g->add_immediate_sync_world_job(TXT("unlock missions game slot mode"), []()
				{
					if (auto* ps = PlayerSetup::access_current_if_exists())
					{
						if (auto* gs = ps->access_active_game_slot())
						{
							gs->make_game_slot_mode_available(GameSlotMode::Missions);
							gs->set_game_slot_mode(GameSlotMode::Missions);
						}
					}
				});

		}

		g->add_async_save_player_profile();
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
