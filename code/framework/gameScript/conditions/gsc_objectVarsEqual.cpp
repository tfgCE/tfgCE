#include "gsc_objectVarsEqual.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\io\xml.h"

#include "..\..\modulesOwner\modulesOwner.h"

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

bool ObjectVarsEqual::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	varA = _node->get_name_attribute(TXT("a"), varA);
	varB = _node->get_name_attribute(TXT("b"), varB);

	result &= params.load_from_xml(_node);

	error_loading_xml_on_assert((varA.is_valid() && varB.is_valid()) || ! params.is_empty(), result, _node, TXT("no \"a\" or \"b\" or \"params\" provided"));

	return result;
}

bool ObjectVarsEqual::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			
			if (!params.is_empty())
			{
				for_every(param, params.get_all())
				{
					auto* imov = imo->get_variables().find_any_existing(param->get_name());

					if (imov &&
						param->type_id() == imov->type_id())
					{
						if (memory_compare(param->get_raw(), imov->get_raw(), RegisteredType::get_size_of(param->type_id())))
						{
							anyOk = true;
						}
						else
						{
							anyFailed = true;
						}
					}
				}
			}

			{
				auto* viA = imo->get_variables().find_any_existing(varA);
				auto* viB = imo->get_variables().find_any_existing(varB);

				if (viA && viB &&
					viA->type_id() == viB->type_id())
				{
					if (memory_compare(viA->get_raw(), viB->get_raw(), RegisteredType::get_size_of(viA->type_id())))
					{
						anyOk = true;
					}
					else
					{
						anyFailed = true;
					}
				}
			}
		}
	}

	return anyOk && ! anyFailed;
}
