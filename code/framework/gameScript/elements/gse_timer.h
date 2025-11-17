#pragma once

#include "..\..\library\usedLibraryStored.h"

#include "..\gameScriptElement.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Timer
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("timer (script exec)"); }

			private:
				Name resetTimer;
				Name resetIfNotActive;
				Name removeTimer;
			};
		};
	};
};
