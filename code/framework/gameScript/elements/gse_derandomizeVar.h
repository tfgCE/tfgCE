#pragma once

#include "gse_copyVar.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class DerandomizeVar
			: public CopyVar
			{
				typedef CopyVar base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("derandomize var"); }

			private:
			};
		};
	};
};
