#include "gsc_varsEqual.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool VarsEqual::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	varA = _node->get_name_attribute(TXT("a"), varA);
	varB = _node->get_name_attribute(TXT("b"), varB);

	error_loading_xml_on_assert(varA.is_valid(), result, _node, TXT("no \"a\" provided"));
	error_loading_xml_on_assert(varB.is_valid(), result, _node, TXT("no \"b\" provided"));

	return result;
}

bool VarsEqual::check(ScriptExecution const & _execution) const
{
	bool result = false;

	auto* viA = _execution.get_variables().find_any_existing(varA);
	auto* viB = _execution.get_variables().find_any_existing(varB);

	if (viA && viB &&
		viA->type_id() == viB->type_id())
	{
		result = memory_compare(viA->get_raw(), viB->get_raw(), RegisteredType::get_size_of(viA->type_id()));
	}

	return result;
}
