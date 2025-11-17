#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class WaitForNoVoiceover
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("wait for no voiceover"); }

			private:
			};
		};
	};
};
