#include "gse_tutClearHighlight.h"

#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

Framework::GameScript::ScriptExecutionResult::Type TutClearHighlight::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->clear_highlights();
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
