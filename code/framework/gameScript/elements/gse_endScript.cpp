#include "gse_endScript.h"

#include "..\gameScript.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

ScriptExecutionResult::Type EndScript::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	_execution.stop();
	return ScriptExecutionResult::End;
}
