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
			class BlendObjectVar
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("blend object var"); }

			private:
				Name objectVar;

				SimpleVariableStorage params;

				float blendTime = 0.0f;
				Name blendCurve;

				bool mayFail = false;
			};
		};
	};
};
