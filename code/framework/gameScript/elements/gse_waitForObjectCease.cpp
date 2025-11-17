#include "gse_waitForObjectCease.h"

#include "..\..\modulesOwner\modulesOwner.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool WaitForObjectToCease::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	var = _node->get_name_attribute(TXT("var"), var);

	return result;
}

ScriptExecutionResult::Type WaitForObjectToCease::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (var.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing< SafePtr<Framework::IModulesOwner> >(var))
		{
			if (auto* imo = exPtr->get())
			{
				// still exists
				return ScriptExecutionResult::Repeat;
			}
		}
	}
	return ScriptExecutionResult::Continue;
}
