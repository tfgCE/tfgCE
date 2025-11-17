#include "gse_tutEnd.h"

#include "..\..\game\game.h"
#include "..\..\tutorials\tutorial.h"
#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool TutEnd::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	justMarkSuccess = _node->get_bool_attribute_or_from_child_presence(TXT("justMarkSuccess"), justMarkSuccess);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TutEnd::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* tutorial = TutorialSystem::get()->get_tutorial())
	{
		PlayerSetup::access_current().mark_tutorial_done(tutorial->get_name());
	}
	if (justMarkSuccess)
	{
		TutorialSystem::get()->force_success();
	}
	else
	{
		TutorialSystem::get()->end_tutorial();
		if (auto* game = Game::get_as<Game>())
		{
			game->request_post_game_summary(PostGameSummary::ReachedEnd);
		}
		if (auto* hub = TutorialSystem::get_active_hub())
		{
			hub->force_deactivate();
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
