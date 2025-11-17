#pragma once

#include "..\gameScriptElement.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class TrapSub
			: public ScriptElement
			{
				FAST_CAST_DECLARE(TrapSub);
				FAST_CAST_BASE(ScriptElement);
				FAST_CAST_END();
				typedef ScriptElement base;
			public:
				Name const& get_name() const { return name; }

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("trap sub"); } // will use returnSub to go back
				implement_ Optional<Colour> get_colour() const { return Colour::c64Red; }
				implement_ String get_extra_debug_info() const { return name.to_string(); }

			private:
				Name name;
			};
		};
	};
};
