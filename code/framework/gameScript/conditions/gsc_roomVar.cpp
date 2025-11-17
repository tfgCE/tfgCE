#include "gsc_roomVar.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\world\room.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

DEFINE_STATIC_NAME(self);

//

bool RoomVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	roomVar = _node->get_name_attribute(TXT("roomVar"), roomVar);

	return result;
}

bool RoomVar::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
	{
		if (auto* r = exPtr->get())
		{
			anyOk = true;
		}
		else
		{
			anyFailed = true;
		}
	}
	else
	{
		anyFailed = true;
	}

	return anyOk && ! anyFailed;
}
