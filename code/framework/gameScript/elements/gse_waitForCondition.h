#pragma once

#include "..\gameScriptCondition.h"
#include "..\gameScriptElement.h"
#include "..\..\..\core\types\optional.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class WaitForCondition
			: public ScriptElement
			{
				typedef ScriptElement base;
			public:
				WaitForCondition() {}

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ void interrupted(ScriptExecution& _execution) const; // on either goto/trap
				implement_ tchar const* get_debug_info() const { return TXT("wait for condition"); }

			private:
				Optional<Range> checkInterval;
				Optional<float> timeLimit;
				Name goToLabelOnTimeLimit;

				RefCountObjectPtr<ScriptCondition> ifCondition;
			};
		};
	};
};
