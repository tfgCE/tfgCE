#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\teaEnums.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	namespace Story
	{
		class Scene;
	};

	namespace GameScript
	{
		namespace Elements
		{
			class Pilgrimage
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("pilgrimage"); }

			private:
				// in case it was set to false due to "handleDoorOperation"/"handleExitDoorOperation" custom variable for the room and we want the door to open as soon as possible
				Optional<bool> pilgrimageInstanceShouldLeaveExitDoorAsIs;
				Optional<bool> giveControlToManageExitDoorToPilgrimageInstance;
				Optional<bool> giveControlToManageEntranceDoorsToTransitionRoomsToPilgrimageInstance;
				Optional<bool> keepExitDoorOpen;
				Optional<Name> triggerGameScriptExecutionTrapOnOpenExitDoor;
				Optional<bool> closeExitDoor;

				Optional<bool> setCanCreateNextPilgrimage; // for missions
				Optional<VectorInt2> createCellAt;
				
				Optional<Name> doneObjectiveID;
				Optional<Name> createCellForObjectiveID;

				Optional<DoorDirectionObjectiveOverride::Type> setDoorDirectionObjectiveOverride;
				Optional<bool> clearDoorDirectionObjectiveOverride;

				Optional<float> advanceEnvironmentCycleBy;

				Optional<float> setEnvironmentRotationCycleLength;
				struct setEnvironmentRotationCycleLengthFrom
				{
					float radius = 0.0f;
					float speed = 0.0f;
					float mulResult = 1.0f; // to get angle
				};
				setEnvironmentRotationCycleLengthFrom setEnvironmentRotationCycleLengthFrom;

				Optional<bool> markMapKnown; // only if the map is limited

				Optional<Name> storeAnyTransitionRoomVar; // stores any created transition room, if present
			};
		};
	};
};
