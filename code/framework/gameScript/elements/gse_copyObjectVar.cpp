#include "gse_copyObjectVar.h"

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

bool CopyObjectVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}
	
	copy.clear();
	for_every(n, _node->all_children())
	{
		Copy c;
		if (n->get_name() != TXT("copy"))
		{
			c.typeId = RegisteredType::get_type_id_by_name(n->get_name().to_char());
			error_loading_xml_on_assert(c.typeId != type_id_none(), result, n, TXT("invalid type"));
		}
		c.from = n->get_name_attribute(TXT("from"), c.from);
		c.from = n->get_name_attribute(TXT("name"), c.from);
		c.to = c.from;
		c.to = n->get_name_attribute(TXT("to"), c.to);
		c.to = n->get_name_attribute(TXT("as"), c.to);
		copy.push_back(c);
	}

	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	return result;
}

ScriptExecutionResult::Type CopyObjectVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			for_every(c, copy)
			{
				TypeId readTypeId;
				void const* src = nullptr;
				if (c->typeId.is_set())
				{
					src = _execution.get_variables().get_raw(c->from, c->typeId.get());
					readTypeId = c->typeId.get();
				}
				else
				{
					_execution.get_variables().get_existing_of_any_type_id(c->from, readTypeId, src);
				}
				if (src && readTypeId != type_id_none())
				{
					void* dest = imo->access_variables().access_raw(c->to, readTypeId);
					if (RegisteredType::is_plain_data(readTypeId))
					{
						memory_copy(dest, src, RegisteredType::get_size_of(readTypeId));
					}
					else
					{
						RegisteredType::copy(readTypeId, dest, src);
					}
				}
			}
		}
	}
	else if (!mayFail)
	{
		warn(TXT("missing object"));
	}

	return ScriptExecutionResult::Continue;
}
