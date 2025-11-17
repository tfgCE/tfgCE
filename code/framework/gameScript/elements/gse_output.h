#pragma once

#include "..\gameScriptElement.h"
#include "..\..\library\usedLibraryStored.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Output
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ Optional<Colour> get_colour() const { return Colour::greyLight; }
				implement_ tchar const* get_debug_info() const { return TXT("output"); }
				implement_ String get_extra_debug_info() const { return text; }

			private:
				String text;
				Name var;
			};
		};
	};
};
