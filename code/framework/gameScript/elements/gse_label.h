#pragma once

#include "..\gameScriptElement.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Label
			: public ScriptElement
			{
				FAST_CAST_DECLARE(Label);
				FAST_CAST_BASE(ScriptElement);
				FAST_CAST_END();
				typedef ScriptElement base;
			public:
				Name const& get_id() const { return id; }

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("label"); }
				implement_ Optional<Colour> get_colour() const { return Colour::c64Cyan; }
				implement_ String get_extra_debug_info() const { return id.to_string(); }

			private:
				Name id;
			};
		};
	};
};
