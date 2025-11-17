#include "gse_copyVar.h"

#include "..\..\modulesOwner\modulesOwner.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool CopyVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	toVar = _node->get_name_attribute(TXT("as"), toVar);
	toVar = _node->get_name_attribute(TXT("to"), toVar);
	toVar = _node->get_name_attribute(TXT("toVar"), toVar);

	if (auto* attr = _node->get_attribute(TXT("from")))
	{
		fromVar.push_back(attr->get_as_name());
	}
	if (auto* attr = _node->get_attribute(TXT("fromVar")))
	{
		fromVar.push_back(attr->get_as_name());
	}

	for_every(node, _node->children_named(TXT("from")))
	{
		if (auto* attr = node->get_attribute(TXT("var")))
		{
			fromVar.push_back(attr->get_as_name());
		}
	}

	error_loading_xml_on_assert(toVar.is_valid(), result, _node, TXT(" no \"toVar\" attribute provided"));
	error_loading_xml_on_assert(! fromVar.is_empty(), result, _node, TXT(" no \"fromVar\" attribute nor <from var=\"\"/> provided"));

	return result;
}

ScriptExecutionResult::Type CopyVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (!fromVar.is_empty() && toVar.is_valid())
	{
		Name fromVarName = fromVar[Random::get_int(fromVar.get_size())];
		scoped_call_stack_info(fromVarName.to_char());
		scoped_call_stack_info(toVar.to_char());
		if (auto* vi = _execution.get_variables().find_any_existing(fromVarName))
		{
			void* dest = _execution.access_variables().access_raw(toVar, vi->type_id());
			if (RegisteredType::is_plain_data(vi->type_id()))
			{
				memory_copy(dest, vi->get_raw(), RegisteredType::get_size_of(vi->type_id()));
			}
			else
			{
				RegisteredType::copy(vi->type_id(), dest, vi->get_raw());
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
