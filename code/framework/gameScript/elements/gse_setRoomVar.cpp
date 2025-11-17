#include "gse_setRoomVar.h"

#include "..\..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool SetRoomVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	roomVar = _node->get_name_attribute(TXT("roomVar"), roomVar);

	result &= params.load_from_xml(_node);

	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	return result;
}

ScriptExecutionResult::Type SetRoomVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
	{
		if (auto* room = exPtr->get())
		{
			// fingers-crossed we're the only to access
			room->access_variables().set_from(params);
		}
	}
	else if (!mayFail)
	{
		warn(TXT("missing room"));
	}

	return ScriptExecutionResult::Continue;
}
