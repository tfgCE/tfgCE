#pragma once

#include "..\gameScriptCondition.h"
#include "..\gameScriptElement.h"

#include "..\..\..\core\random\randomNumber.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class GoToRandomLabel
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("go to random label"); }
				implement_ Optional<Colour> get_colour() const { return Colour::c64LightBlue; }

			private:
				struct Choose
				{
					Optional<int> value;
					Name id;
					RefCountObjectPtr<ScriptCondition> ifCondition;
					Optional<int> changeValueOnFailedCondition;
				};
				Array<Choose> choose;

				Random::Number<int> randomInt;
			};
		};
	};
};
