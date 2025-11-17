#include "gse_storyEndScene.h"

#include "..\..\game\gameDirector.h"
#include "..\..\story\storyExecution.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

Framework::GameScript::ScriptExecutionResult::Type StoryEndScene::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
	{
		gd->access_story_execution().set_scene(nullptr);
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
