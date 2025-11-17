#include "gse_deactivateHub.h"

#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

Framework::GameScript::ScriptExecutionResult::Type DeactivateHub::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* hub = game->get_recent_hub_loader())
		{
			hub->force_deactivate();
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
