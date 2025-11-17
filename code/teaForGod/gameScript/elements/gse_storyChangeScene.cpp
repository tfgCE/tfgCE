#include "gse_storyChangeScene.h"

#include "..\..\game\gameDirector.h"
#include "..\..\story\storyExecution.h"
#include "..\..\story\storyScene.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool StoryChangeScene::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= scene.load_from_xml(_node, TXT("scene"), _lc);

	return result;
}

bool StoryChangeScene::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= scene.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type StoryChangeScene::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
	{
		gd->access_story_execution().set_scene(scene.get());
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
