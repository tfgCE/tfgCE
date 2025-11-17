#include "gsc_movementScripted.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\game.h"

#include "..\..\..\framework\module\moduleMovementScripted.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::MovementScripted::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	isAtDestination.load_from_xml(_node, TXT("isAtDestination"));

	return result;
}

bool Conditions::MovementScripted::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			if (auto* m = fast_cast<Framework::ModuleMovementScripted>(imo->get_movement()))
			{
				if (isAtDestination.is_set())
				{
					bool ok = isAtDestination.get() == m->is_at_destination();
					anyOk |= ok;
					anyFailed |= !ok;
				}
			}
		}
	}

	return anyOk && !anyFailed;
}
