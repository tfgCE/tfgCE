#pragma once

#include "..\gameScriptCondition.h"
#include "..\gameScriptElement.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class ReturnSub
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("return sub"); }
			};
		};
	};
};
