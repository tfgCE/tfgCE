#pragma once

#include "..\gameScriptElement.h"

#include "..\..\..\core\types\option.h"
#include "..\..\..\core\types\optional.h"

#include "..\..\world\doorType.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Door
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ tchar const* get_debug_info() const { return TXT("door"); }

			protected:
				Name doorVar;

				Optional<Framework::DoorOperation::Type> setOperation;
				bool operationForced = true; // by default, script should always override locks
				Optional<bool> setOperationLock; // locks/unlocks post setting operation
				Optional<float> setAutoOperationDistance;

				Optional<bool> setMute;
				Optional<bool> setVisible;

				Optional<Name> placeAtPOI; // synced
				Optional<Name> placeAtPOIInRoomVar; // synced

			};
		};
	};
};
