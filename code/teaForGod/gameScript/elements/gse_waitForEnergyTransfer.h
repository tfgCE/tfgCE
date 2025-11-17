#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\game\energy.h"
#include "..\..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class WaitForEnergyTransfer
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ void interrupted(Framework::GameScript::ScriptExecution& _execution) const; // on either goto/trap
				implement_ tchar const* get_debug_info() const { return TXT("wait for energy transfer"); }

			private:
				Optional<Energy> energyPoolMin;
				Optional<Energy> energyPoolMax;

				static float calculate_energy_pool_pt(Framework::GameScript::ScriptExecution& _execution, Optional<Energy> const& _energyPoolMin, Optional<Energy> const& _energyPoolMax);
			};
		};
	};
};
