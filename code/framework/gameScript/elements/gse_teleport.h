#pragma once

#include "..\gameScriptElement.h"
#include "..\..\library\usedLibraryStored.h"

namespace Framework
{
	class Room;

	namespace GameScript
	{
		namespace Elements
		{
			class Teleport
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("teleport"); }

			protected:
				virtual Room* get_to_room(ScriptExecution& _execution) const;

				virtual void alter_target_placement(Optional<Transform>& _targetPlacement) const {}

			protected:
				Name objectVar;

				Optional<Transform> toPlacement;
				Name toRoomVar;
				Name toPlacementVar;
				Name toObjectVar;
				Name toSocket;
				Name toPOI;

				Optional<Rotator3> absoluteRotation;
				Optional<Vector3> absoluteLocationOffset;
				Optional<Transform> relativePlacement;

				bool toLockerRoom = false;

				Name offsetPlacementRelativelyToParam; // in room

				bool silent = false;
				bool keepVelocityOS = false;
			};
		};
	};
};
