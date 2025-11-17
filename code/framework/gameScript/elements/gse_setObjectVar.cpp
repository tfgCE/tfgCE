#include "gse_setObjectVar.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\modulesOwner\modulesOwner.h"

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

bool SetObjectVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	result &= params.load_from_xml(_node);

	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	return result;
}

ScriptExecutionResult::Type SetObjectVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			imo->access_variables().set_from(params);
		}
	}
	else if (!mayFail)
	{
		warn(TXT("missing object"));
	}

	return ScriptExecutionResult::Continue;
}
