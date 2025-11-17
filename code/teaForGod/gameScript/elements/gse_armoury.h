#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

namespace Framework
{
	class ModuleAppearance;
};

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Armoury
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("armoury"); }

			private:
				Name objectVar;

				Optional<bool> addGearToPersistence;
				Optional<bool> allowDuplicates;
			};
		};
	};
};
