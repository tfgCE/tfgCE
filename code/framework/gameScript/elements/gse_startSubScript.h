#pragma once

#include "..\gameScriptCondition.h"
#include "..\gameScriptElement.h"
#include "..\..\library\usedLibraryStored.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class StartSubScript
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("start sub script"); }

				implement_ void load_on_demand_if_required();

			private:
				Name id;
				Framework::UsedLibraryStored<GameScript::Script> script;

				RefCountObjectPtr<ScriptCondition> ifCondition;
			};
		};
	};
};
