#include "gse_tutStep.h"

#include "..\..\game\game.h"
#include "..\..\tutorials\tutorial.h"
#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

Framework::GameScript::ScriptExecutionResult::Type TutStep::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->on_tutorial_step();
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
