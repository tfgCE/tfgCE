#pragma once

#include "..\gameScriptElement.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class DevPause
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("dev pause"); }
				implement_ Optional<Colour> get_colour() const { return Colour::red; }

			private:
			};
		};
	};
};
