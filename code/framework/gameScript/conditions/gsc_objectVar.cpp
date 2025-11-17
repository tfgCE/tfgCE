#include "gsc_objectVar.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\io\xml.h"

#include "..\..\modulesOwner\modulesOwner.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

DEFINE_STATIC_NAME(self);

//

bool ObjectVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	variable.load_from_xml(_node, TXT("variable"));
	variable.load_from_xml(_node, TXT("param"));

	return result;
}

bool ObjectVar::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	bool noObject = false;

	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			bool anyCondition = false;
			if (variable.is_set())
			{
				anyCondition = true;
				TypeId typeId;
				void const* value;
				if (imo->get_variables().get_existing_of_any_type_id(variable.get(), typeId, value))
				{
					bool ok = false;
					if (typeId == type_id<bool>())
					{
						ok = (*(plain_cast<bool>(value)));
					}
					else if (typeId == type_id<float>())
					{
						ok = (*(plain_cast<float>(value))) != 0.0f;
					}
					else
					{
						error(TXT("implement support for variable of type \"%S\""), RegisteredType::get_name_of(typeId));
						anyFailed = true;
					}
					anyOk |= ok;
					anyFailed |= !ok;
				}
				else
				{
					anyFailed = true;
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
		if (!anyCondition)
		{
			anyFailed = true;
		}
	}

	return anyOk && ! anyFailed;
}
