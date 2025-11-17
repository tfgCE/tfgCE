#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\teaEnums.h"
#include "..\..\..\core\types\option.h"
#include "..\..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class TutSet
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("tut set"); }

			private:
				bool handDisplaysStateProvided = false;
				Optional<bool> handDisplaysState;

				Optional<Option::Type> energyTransfer;
				Optional<Option::Type> physicalViolence;
				Optional<Option::Type> pickupOrbs;
				
				Optional<Name> navigationTargetVar;
			};
		};
	};
};
