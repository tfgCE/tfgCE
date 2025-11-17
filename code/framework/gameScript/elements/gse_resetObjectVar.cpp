#include "gse_resetObjectVar.h"

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

bool ResetObjectVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}
	
	reset.clear();
	for_every(n, _node->all_children())
	{
		Reset c;
		if (n->get_name() != TXT("reset"))
		{
			c.typeId = RegisteredType::get_type_id_by_name(n->get_name().to_char());
			error_loading_xml_on_assert(c.typeId != type_id_none(), result, n, TXT("invalid type"));
		}
		c.name = n->get_name_attribute(TXT("name"), c.name);
		reset.push_back(c);
	}


	return result;
}

ScriptExecutionResult::Type ResetObjectVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			for_every(c, reset)
			{
				TypeId useTypeId = type_id_none();
				if (c->typeId.is_set())
				{
					useTypeId = c->typeId.get();
				}
				else
				{
					imo->access_variables().get_existing_type_id(c->name, OUT_ useTypeId);
				}
				if (useTypeId != type_id_none())
				{
					if (void* value = imo->access_variables().access_existing(c->name, useTypeId))
					{
						if (RegisteredType::is_plain_data(useTypeId))
						{
							memory_zero(value, RegisteredType::get_size_of(useTypeId));
						}
						else
						{
							RegisteredType::destruct(useTypeId, value);
							RegisteredType::construct(useTypeId, value);
						}
					}
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
