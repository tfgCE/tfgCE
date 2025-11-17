#include "gse_trapSub.h"

#include "..\..\..\core\io\xml.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

REGISTER_FOR_FAST_CAST(TrapSub);

bool TrapSub::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	error_loading_xml_on_assert(name.is_valid(), result, _node, TXT("\"name\" for trapSub required"));

	return result;
}

ScriptExecutionResult::Type TrapSub::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	// we're just a trap
	return ScriptExecutionResult::Continue;
}

