#include "gse_vignetteForMovement.h"

#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool VignetteForMovement::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	forceVignetteForMovement.load_from_xml(_node, TXT("force"));
	clear = _node->get_bool_attribute_or_from_child_presence(TXT("clear"), clear);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type VignetteForMovement::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (forceVignetteForMovement.is_set())
		{
			game->force_vignette_for_movement(forceVignetteForMovement.get());
		}
		else if (clear)
		{
			game->clear_forced_vignette_for_movement();
		}
		else
		{
			warn(TXT("nothing for vignette for movement"));
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
