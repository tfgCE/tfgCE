#pragma once

#include "..\gameScriptElement.h"
#include "..\..\..\core\types\option.h"
#include "..\..\..\core\types\optional.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class SetRoomVar
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("set room var"); }

			private:
				Name roomVar;

				SimpleVariableStorage params;

				bool mayFail = false;
			};
		};
	};
};
