#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\game\energy.h"

#include "..\..\..\core\types\option.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Health
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("health"); }

			private:
				Optional<Option::Type> blockRegen;

				Name var;

				// various
				Optional<bool> beImmortal;
				Optional<bool> clearImmortal;

				// time based
				Energy speed = Energy(10.0f);
				Optional<Energy> drainTo;
				Optional<Energy> drainToTotal;
				Optional<Energy> fillToTotal;

				// just set
				Optional<Energy> setTotal;
				Optional<Energy> setBackup;
				
				// die
				Optional<bool> performDeath;
				Optional<bool> quickDeath; // use for scripted
				Optional<bool> peacefulDeath; // something between death and quick death

				// drop
				Optional<EnergyRange> dropBy;
				Optional<Energy> dropMin;
			};
		};
	};
};
