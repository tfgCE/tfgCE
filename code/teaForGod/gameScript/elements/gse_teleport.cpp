#include "gse_teleport.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"

#include "..\..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Teleport::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	toLockerRoom = _node->get_bool_attribute_or_from_child_presence(TXT("toLockerRoom"), toLockerRoom);

	return result;
}

Framework::Room* Teleport::get_to_room(Framework::GameScript::ScriptExecution& _execution) const
{
	if (toLockerRoom)
	{
		if (auto* g = Game::get_as<Game>())
		{
			return g->get_locker_room();
		}
	}
	return base::get_to_room(_execution);
}

void Teleport::alter_target_placement(Optional<Transform>& _targetPlacement) const
{
	base::alter_target_placement(_targetPlacement);

	if (toLockerRoom && !_targetPlacement.is_set())
	{
		_targetPlacement = Transform::identity;
	}
}

