#include "gse_environmentPlayer.h"

#include "..\..\game\environmentPlayer.h"
#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::EnvironmentPlayer::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	updateEffects.load_from_xml(_node, TXT("updateEffects"));

	return result;
}

bool Elements::EnvironmentPlayer::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::EnvironmentPlayer::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* ep = game->access_environment_player())
		{
			if (updateEffects.get(false))
			{
				ep->update_effects();
			}
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
