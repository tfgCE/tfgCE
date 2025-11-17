#include "aipr_FindAnywhere.h"

#include "..\..\..\framework\ai\aiPerception.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#include "..\..\..\core\random\randomUtils.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace PerceptionRequests;

//

REGISTER_FOR_FAST_CAST(FindAnywhere);

FindAnywhere::FindAnywhere(Framework::IModulesOwner* _owner, CheckObject _check_object)
: check_object(_check_object)
, owner(_owner)
{
}

struct Found
{
	Framework::IModulesOwner* found;
	float chance;

	Found() {}
	Found(Framework::IModulesOwner* _found, float _chance) : found(_found), chance(_chance) {}
};

bool FindAnywhere::process()
{
	scoped_call_stack_info(TXT("perception request - find anywhere"));

	if (owner)
	{
		result = nullptr;

		ARRAY_STACK(Found, allFound, 16);

		Random::Generator rg;

		if (auto * room = owner->get_presence()->get_in_room())
		{
			if (auto * world = room->get_in_world())
			{
				for_every_ptr(object, world->get_objects())
				{
					if (object == owner ||
						!Framework::AI::Perception::is_valid_for_perception(object))
					{
						continue;
					}
					float chance = check_object(object);
					if (chance > 0.0f)
					{
						allFound.push_back_or_replace(Found(object, chance), rg);
						result = object;
					}
				}
			}
		}

		if (!allFound.is_empty())
		{
			int tryIdx = RandomUtils::ChooseFromContainer<ArrayStack<Found>, Found>::choose(rg, allFound, [](Found const & _found) { return _found.chance; });
			an_assert(allFound.is_index_valid(tryIdx));
			result = allFound[tryIdx].found;
		}

		if (result)
		{
			if (!pathToResult.find_path(owner, result))
			{
				result = nullptr;
			}
		}
	}

	return base::process();
}

