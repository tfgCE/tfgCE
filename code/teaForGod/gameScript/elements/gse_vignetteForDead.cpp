#include "gse_vignetteForDead.h"

#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool VignetteForDead::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	at.load_from_xml(_node, TXT("at"));
	target.load_from_xml(_node, TXT("target"));
	blendTime.load_from_xml(_node, TXT("blendTime"));
	clear = _node->get_bool_attribute_or_from_child_presence(TXT("clear"), clear);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type VignetteForDead::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (clear)
		{
			game->set_vignette_for_dead_at(0.0f);
		}
		else
		{
			if (at.is_set())
			{
				game->set_vignette_for_dead_at(at.get());
			}
			if (target.is_set())
			{
				game->set_vignette_for_dead_target(target.get(), blendTime);
			}
			else if (blendTime.is_set())
			{
				game->set_vignette_for_dead_target(game->get_vignette_for_dead_target(), blendTime);
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
