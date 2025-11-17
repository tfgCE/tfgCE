#pragma once

#include "..\gameScriptCondition.h"
#include "..\gameScriptElement.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Trap
			: public ScriptElement
			{
				FAST_CAST_DECLARE(Trap);
				FAST_CAST_BASE(ScriptElement);
				FAST_CAST_END();
				typedef ScriptElement base;
			public:
				Name const& get_name() const { return name; }

				bool check_condition(ScriptExecution& _execution) const;

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("trap"); }
				implement_ Optional<Colour> get_colour() const { return Colour::c64Red; }
				implement_ String get_extra_debug_info() const { return name.to_string(); }

			private:
				Name name;

				RefCountObjectPtr<ScriptCondition> ifCondition;
			};
		};
	};
};
