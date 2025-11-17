#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\game\energy.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\hand.h"

namespace TeaForGodEmperor
{
	class ModulePilgrim;

	namespace GameScript
	{
		namespace Elements
		{
			class MainEquipment
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("main equipment"); }

			private:
				// just set
				Optional<Energy> setEnergyTotal;

				Optional<Hand::Type> hand;
				Optional<Energy> setStorage;
				Optional<EnergyCoef> setCoreDamaged;
				Optional<EnergyRange> dropStorageBy;

				Optional<bool> blockReleasing;

				bool actionRelease = false;

				void execute_for_hand(ModulePilgrim* pilgrim, Hand::Type _hand) const;
			};
		};
	};
};
