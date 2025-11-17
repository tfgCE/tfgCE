#include "gse_info.h"

#include "..\..\..\core\io\xml.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

REGISTER_FOR_FAST_CAST(Info);

bool Info::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	info = _node->get_text();

	info = _node->get_string_attribute_or_from_child(TXT("text"), info);
	info = _node->get_string_attribute_or_from_child(TXT("info"), info);

	return result;
}

ScriptExecutionResult::Type Info::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	// we're just a text
	return ScriptExecutionResult::Continue;
}

