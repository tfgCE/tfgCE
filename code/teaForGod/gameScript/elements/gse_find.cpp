#include "gse_find.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Find::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	lockerRoom = _node->get_bool_attribute_or_from_child_presence(TXT("lockerRoom"), lockerRoom);
	inLockerRoom = _node->get_bool_attribute_or_from_child_presence(TXT("inLockerRoom"), inLockerRoom);

	inPlayersRoom = _node->get_bool_attribute_or_from_child_presence(TXT("inRoom"), inPlayersRoom);
	inPlayersRoom = _node->get_bool_attribute_or_from_child_presence(TXT("inPlayersRoom"), inPlayersRoom);

	return result;
}

Framework::World* Find::get_world(Framework::GameScript::ScriptExecution& _execution) const
{
	if (auto* g = Game::get_as<Game>())
	{
		return g->get_world();
	}
	return base::get_world(_execution);
}

Framework::Room* Find::get_room_for_find_object(Framework::GameScript::ScriptExecution& _execution) const
{
	if (inPlayersRoom)
	{
#ifdef INVESTIGATE_GAME_SCRIPT
		output(TXT("in player's room"));
#endif
		todo_multiplayer_issue(TXT("we assume it's sp"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				return pa->get_presence()->get_in_room();
			}
		}
		return nullptr;
	}
	else if (inLockerRoom)
	{
#ifdef INVESTIGATE_GAME_SCRIPT
		output(TXT("checking objects in the locker room"));
#endif
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* lr = g->get_locker_room())
			{
				return lr;
			}
		}
		return nullptr;
	}

	return base::get_room_for_find_object(_execution);
}

void Find::find_room(Framework::GameScript::ScriptExecution& _execution, OUT_ Results& _results) const
{
	if (lockerRoom)
	{
		if (auto* g = Game::get_as<Game>())
		{
			_results.foundRoom = g->get_locker_room();
			return;
		}
	}

	base::find_room(_execution, OUT_ _results);
}
