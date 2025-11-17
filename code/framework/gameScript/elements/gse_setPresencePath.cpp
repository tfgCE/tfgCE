#include "gse_setPresencePath.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\modulesOwner\modulesOwner.h"
#include "..\..\presence\presencePath.h"

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

bool SetPresencePath::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);
	targetVar = _node->get_name_attribute(TXT("targetVar"), targetVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}
	
	presencePathVarName = _node->get_name_attribute(TXT("presencePathVarName"), presencePathVarName);
	presencePathPtrVarName = _node->get_name_attribute(TXT("presencePathPtrVarName"), presencePathPtrVarName);

	return result;
}

ScriptExecutionResult::Type SetPresencePath::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(targetVar))
			{
				if (auto* targetIMO = exPtr->get())
				{
					Concurrency::ScopedSpinLock lock(imo->access_lock());
					auto& pp = imo->access_variables().access<Framework::PresencePath>(presencePathVarName);
					pp.find_path(imo, targetIMO);
					if (presencePathPtrVarName.is_valid())
					{
						auto& pptr = imo->access_variables().access<Framework::PresencePath*>(presencePathPtrVarName);
						pptr = &pp;
					}
				}
			}
		}
	}
	else
	{
		warn(TXT("missing object"));
	}

	return ScriptExecutionResult::Continue;
}
