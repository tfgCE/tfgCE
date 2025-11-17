#pragma once

#include "..\gameScriptElement.h"
#include "..\..\..\core\math\math.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Wait
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("wait"); }

			private:
				Range time = Range::empty;
			};
		};
	};
};
