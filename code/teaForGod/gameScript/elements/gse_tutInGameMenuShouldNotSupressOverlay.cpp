#include "gse_tutInGameMenuShouldNotSupressOverlay.h"

#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

Framework::GameScript::ScriptExecutionResult::Type TutInGameMenuShouldNotSupressOverlay::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->in_game_menu_should_not_supress_overlay();
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
