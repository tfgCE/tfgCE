#include "gse_game.h"

#include "..\..\game\game.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::Game::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	savePlayerProfile = _node->get_bool_attribute_or_from_child_presence(TXT("savePlayerProfile"), savePlayerProfile);

	return result;
}

bool Elements::Game::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::Game::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		if (savePlayerProfile)
		{
			g->add_async_save_player_profile();
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
