#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\core\types\hand.h"
#include "..\..\..\core\tags\tagCondition.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Pilgrim
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("pilgrim"); }

			private:
				Name storeInRoomVar;

				Optional<bool> storeLastSetup;
				Optional<bool> clearExtraEXMsInInventory; // clears, so only those that are on us are available
				Optional<bool> updateEXMsInInventoryFromPersistence; // copies from persistence

				Optional<bool> setHandsPoseIdle;
				Optional<bool> showWeaponsBlocked;				

				Optional<Hand::Type> loseHand;

				Optional<Name> copyEXMsToObjectVar;

			};
		};
	};
};
