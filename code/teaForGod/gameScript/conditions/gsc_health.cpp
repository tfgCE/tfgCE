#include "gsc_health.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\io\xml.h"

#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

DEFINE_STATIC_NAME(self);

//

bool Health::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	isAlive.load_from_xml(_node, TXT("isAlive"));

	return result;
}

bool Health::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	bool noObject = false;

	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			bool anyCondition = false;
			if (isAlive.is_set())
			{
				anyCondition = true;
				if (auto* h = imo->get_custom<CustomModules::Health>())
				{
					bool alive = h->get_health().is_positive();
					if (isAlive.get() == alive)
					{
						anyOk = true;
					}
					else
					{
						anyFailed = true;
					}
				}
			}
			if (!anyCondition)
			{
				anyOk = true;
			}
		}
		else
		{
			noObject = true;
		}
	}
	else
	{
		noObject = true;
	}

	if (noObject)
	{
		bool anyCondition = false;
		if (isAlive.is_set())
		{
			anyCondition = true;
			if (isAlive.get())
			{
				anyFailed = true;
			}
			else
			{
				anyOk = true;
			}
		}
		if (!anyCondition)
		{
			anyFailed = true;
		}
	}

	return anyOk && ! anyFailed;
}
