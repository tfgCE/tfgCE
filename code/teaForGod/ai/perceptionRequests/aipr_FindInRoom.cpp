#include "aipr_FindInRoom.h"

#include "..\..\..\framework\ai\aiPerception.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace PerceptionRequests;

//

REGISTER_FOR_FAST_CAST(FindInRoom);

FindInRoom::FindInRoom(Framework::IModulesOwner* _owner, CheckObject _check_object)
: check_object(_check_object)
, owner(_owner)
{
}

bool FindInRoom::process()
{
	scoped_call_stack_info(TXT("perception request - find in room"));

	if (owner)
	{
		::Framework::IModulesOwner* result = nullptr;

		if (auto * room = owner->get_presence()->get_in_room())
		{
			for_every_ptr(object, room->get_objects())
			{
				if (object == owner ||
					!Framework::AI::Perception::is_valid_for_perception(object))
				{
					continue;
				}
				if (check_object(object))
				{
					result = object;
				}
			}
		}
		if (result)
		{
			pathToResult.set_owner(owner);
			pathToResult.set_target(result);
		}
		else
		{
			pathToResult.set_target(nullptr);
		}
	}

	return base::process();
}

