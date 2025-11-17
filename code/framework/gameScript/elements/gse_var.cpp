#include "gse_var.h"

#include "..\..\modulesOwner\modulesOwner.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

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

bool Var::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= params.load_from_xml(_node);

	return result;
}

ScriptExecutionResult::Type Var::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	_execution.access_variables().set_from(params);

	return ScriptExecutionResult::Continue;
}
