#pragma once

#include "..\gameScriptCondition.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Conditions
		{
			class Not
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

				implement_ bool check(ScriptExecution const& _execution) const;

			private:
				RefCountObjectPtr<ScriptCondition> subCondition;
			};

			class LogicOp2
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			protected:
				Array<RefCountObjectPtr<ScriptCondition>> subConditions;
			};

			class Or
			: public LogicOp2
			{
				typedef LogicOp2 base;
			public: // ScriptCondition
				implement_ bool check(ScriptExecution const& _execution) const;
			};

			class And
			: public LogicOp2
			{
				typedef LogicOp2 base;
			public: // ScriptCondition
				implement_ bool check(ScriptExecution const& _execution) const;
			};
		}
	}
}