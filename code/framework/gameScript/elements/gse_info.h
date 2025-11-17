#pragma once

#include "..\gameScriptElement.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Info
			: public ScriptElement
			{
				FAST_CAST_DECLARE(Info);
				FAST_CAST_BASE(ScriptElement);
				FAST_CAST_END();
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("info"); }
				implement_ Optional<Colour> get_colour() const { return Colour::white; }
				implement_ String get_extra_debug_info() const { return info; }

			private:
				String info;
			};
		};
	};
};
