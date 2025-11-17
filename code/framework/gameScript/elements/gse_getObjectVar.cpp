#include "gse_getObjectVar.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\modulesOwner\modulesOwner.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool GetObjectVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);
	var = _node->get_name_attribute(TXT("var"), var);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	result &= params.load_from_xml(_node);

	return result;
}

ScriptExecutionResult::Type GetObjectVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			for_every(param, params.get_all())
			{
				if (auto* src = imo->get_variables().get_raw(param->get_name(), param->type_id()))
				{
					if (auto* dest = _execution.access_variables().access_raw(param->get_name(), param->type_id()))
					{
						RegisteredType::copy(param->type_id(), dest, src);
					}
				}
				else
				{
					error(TXT("missing variable \"%S\" (%S)"), param->get_name().to_char(), RegisteredType::get_name_of(param->type_id()));
				}
			}
			if (var.is_valid())
			{
				void const* src = nullptr;
				TypeId srcType;
				if (imo->get_variables().get_existing_of_any_type_id(var, srcType, src))
				{
					if (auto* dest = _execution.access_variables().access_raw(var, srcType))
					{
						RegisteredType::copy(srcType, dest, src);
					}
				}
				else
				{
					error(TXT("missing variable \"%S\" (any type)"), var.to_char());
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
