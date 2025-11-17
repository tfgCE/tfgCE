#include "gse_returnSub.h"

#include "gse_label.h"

#include "..\gameScript.h"
#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

ScriptExecutionResult::Type ReturnSub::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	_execution.set_at_return_sub();

	return ScriptExecutionResult::SetNextInstruction;
}
