#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\..\core\types\hand.h"
#include "..\..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class WaitForMainEquipment
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ void interrupted(Framework::GameScript::ScriptExecution& _execution) const; // on either goto/trap
				implement_ tchar const* get_debug_info() const { return TXT("wait for main equipment"); }

			private:
				Hand::Type hand = Hand::MAX;
				Optional<bool> deployed;
				Optional<bool> heldActively;
				
				Optional<bool> enoughEnergyToShoot;
				
				Optional<float> timeLimit;

				static float get_energy_to_shoot_pt();
			};
		};
	};
};
