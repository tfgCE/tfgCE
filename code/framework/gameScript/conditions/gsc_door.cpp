#include "gsc_door.h"

#include "..\..\world\door.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::Door::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	doorVar = _node->get_name_attribute_or_from_child(TXT("doorVar"), doorVar);

	isClosed.load_from_xml(_node, TXT("isClosed"));
	isOpen.load_from_xml(_node, TXT("isOpen"));

	return result;
}

bool Conditions::Door::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (doorVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Door>>(doorVar))
		{
			if (auto* door = exPtr->get())
			{
				if (isClosed.is_set())
				{
					bool ok = door->get_open_factor() == 0.0f;

					ok = (ok == isClosed.get());

					anyOk |= ok;
					anyFailed |= !ok;
				}
				if (isOpen.is_set())
				{
					bool ok = door->get_open_factor() == 1.0f;

					ok = (ok == isOpen.get());

					anyOk |= ok;
					anyFailed |= !ok;
				}
			}
		}
	}

	return anyOk && !anyFailed;
}
