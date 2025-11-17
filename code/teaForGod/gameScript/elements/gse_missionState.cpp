#include "gse_missionState.h"

#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::MissionState::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	markStarted = _node->get_bool_attribute_or_from_child_presence(TXT("markStarted"), markStarted);
	markCameBack = _node->get_bool_attribute_or_from_child_presence(TXT("markCameBack"), markCameBack);
	markSuccess = _node->get_bool_attribute_or_from_child_presence(TXT("markSuccess"), markSuccess);
	markFailed = _node->get_bool_attribute_or_from_child_presence(TXT("markFailed"), markFailed);
	markDied = _node->get_bool_attribute_or_from_child_presence(TXT("markDied"), markDied);

	return result;
}

bool Elements::MissionState::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::MissionState::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* g = Game::get_as<Game>())
	{
		if (markStarted)
		{
			g->add_immediate_sync_world_job(TXT("mission state: started"), []()
				{
					if (auto* ms = TeaForGodEmperor::MissionState::get_current())
					{
						ms->mark_started();
						ms->use_intel_on_start();
						ms->store_starting_state();
					}
				});
		}
		if (markCameBack)
		{
			g->add_immediate_sync_world_job(TXT("mission state: came back"), []()
				{
					if (auto* ms = TeaForGodEmperor::MissionState::get_current())
					{
						ms->mark_came_back();
					}
				});
		}
		if (markSuccess)
		{
			g->add_immediate_sync_world_job(TXT("mission state: success"), []()
				{
					if (auto* ms = TeaForGodEmperor::MissionState::get_current())
					{
						ms->mark_success();
					}
				});
		}
		if (markFailed)
		{
			g->add_immediate_sync_world_job(TXT("mission state: failed"), []()
				{
					if (auto* ms = TeaForGodEmperor::MissionState::get_current())
					{
						ms->mark_failed();
					}
				});
		}
		if (markDied)
		{
			g->add_immediate_sync_world_job(TXT("mission state: died"), []()
				{
					if (auto* ms = TeaForGodEmperor::MissionState::get_current())
					{
						ms->mark_died();
					}
				});
		}

		g->add_async_save_player_profile();
	}
	return Framework::GameScript::ScriptExecutionResult::Yield;
}
