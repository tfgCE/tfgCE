#include "gsc_room.h"

#include "..\..\game\game.h"
#include "..\..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::Room::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	roomVar = _node->get_name_attribute_or_from_child(TXT("roomVar"), roomVar);

	isVisible.load_from_xml(_node, TXT("isVisible"));
	isTagged.load_from_xml_attribute(_node, TXT("isTagged"));
	allDelayedObjectCreationProcessed.load_from_xml(_node, TXT("allDelayedObjectCreationProcessed"));

	return result;
}

bool Conditions::Room::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (roomVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
		{
			if (auto* room = exPtr->get())
			{
				if (isVisible.is_set())
				{
					bool visible = room->was_recently_seen_by_player();
					bool ok = visible == isVisible.get();

					anyOk |= ok;
					anyFailed |= !ok;
				}
				if (! isTagged.is_empty())
				{
					bool ok = isTagged.check(room->get_tags());

					anyOk |= ok;
					anyFailed |= !ok;
				}
				if (allDelayedObjectCreationProcessed.is_set())
				{
					bool anyDOCs = true;
					anyDOCs = Game::get()->has_any_delayed_object_creation_pending(room);
					bool ok = ! anyDOCs;

					anyOk |= ok;
					anyFailed |= !ok;
				}
			}
		}
	}

	return anyOk && !anyFailed;
}
