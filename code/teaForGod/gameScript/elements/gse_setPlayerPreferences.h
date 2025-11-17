#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\teaEnums.h"
#include "..\..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class SetPlayerPreferences
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public:
				SetPlayerPreferences() {}

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("set player preferences"); }

			private:
				Optional<bool> healthIndicator;
				Optional<float> healthIndicatorIntensity;
				Optional<float> hitIndicatorIntensity;
				Optional<HitIndicatorType::Type> hitIndicatorType;
			};
		};
	};
};
