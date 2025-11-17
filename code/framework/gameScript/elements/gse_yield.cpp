#include "gse_yield.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool GSEYield::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type GSEYield::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	return ScriptExecutionResult::Yield;
}
