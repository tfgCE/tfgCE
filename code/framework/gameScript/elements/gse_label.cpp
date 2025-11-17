#include "gse_label.h"

#include "..\..\..\core\io\xml.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

REGISTER_FOR_FAST_CAST(Label);

bool Label::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	id = _node->get_name_attribute_or_from_child(TXT("name"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("\"id\" for label required"));

	return result;
}

ScriptExecutionResult::Type Label::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	// we're just a label
	return ScriptExecutionResult::Continue;
}

