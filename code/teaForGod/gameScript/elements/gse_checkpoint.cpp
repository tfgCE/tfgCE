#include "gse_checkpoint.h"

#include "..\..\game\game.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::Checkpoint::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	restartInRoomTagged.load_from_xml_attribute_or_child_node(_node, TXT("restartInRoomTagged"));

	return result;
}

bool Elements::Checkpoint::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::Checkpoint::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		// store only if certain time passed
		if (game->get_time_since_pilgrimage_started() > 10.0f)
		{
			output(TXT("[game script ordered checkpoint]"));
			game->add_async_store_game_state(true, GameStateLevel::Checkpoint, StoreGameStateParams().restart_in_room_tagged(restartInRoomTagged));
		}
		else
		{
			output(TXT("[game script ordered checkpoint ignored, too soon]"));
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
