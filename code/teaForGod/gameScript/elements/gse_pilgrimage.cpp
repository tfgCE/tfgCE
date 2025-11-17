#include "gse_pilgrimage.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::Pilgrimage::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	pilgrimageInstanceShouldLeaveExitDoorAsIs.load_from_xml(_node, TXT("pilgrimageInstanceShouldLeaveExitDoorAsIs"));
	giveControlToManageExitDoorToPilgrimageInstance.load_from_xml(_node, TXT("giveControlToManageExitDoorToPilgrimageInstance"));
	giveControlToManageEntranceDoorsToTransitionRoomsToPilgrimageInstance.load_from_xml(_node, TXT("giveControlToManageEntranceDoorsToTransitionRoomsToPilgrimageInstance"));
	giveControlToManageEntranceDoorsToTransitionRoomsToPilgrimageInstance.load_from_xml(_node, TXT("giveControlToManageEntranceDoorToTransitionRoomToPilgrimageInstance"));
	keepExitDoorOpen.load_from_xml(_node, TXT("keepExitDoorOpen"));
	triggerGameScriptExecutionTrapOnOpenExitDoor.load_from_xml(_node, TXT("triggerGameScriptExecutionTrapOnOpenExitDoor"));
	closeExitDoor.load_from_xml(_node, TXT("closeExitDoor"));

	setCanCreateNextPilgrimage.load_from_xml(_node, TXT("setCanCreateNextPilgrimage"));
	createCellAt.load_from_xml(_node, TXT("createCellAt"));

	doneObjectiveID.load_from_xml(_node, TXT("doneObjectiveID"));
	doneObjectiveID.load_from_xml(_node, TXT("doneObjective"));
	
	createCellForObjectiveID.load_from_xml(_node, TXT("createCellForObjectiveID"));
	createCellForObjectiveID.load_from_xml(_node, TXT("createCellForObjective"));
	
	if (auto* attr = _node->get_attribute(TXT("setDoorDirectionObjectiveOverride")))
	{
		setDoorDirectionObjectiveOverride = DoorDirectionObjectiveOverride::parse(attr->get_as_string());
	}
	clearDoorDirectionObjectiveOverride.load_from_xml(_node, TXT("clearDoorDirectionObjectiveOverride"));
	
	setEnvironmentRotationCycleLength.load_from_xml(_node, TXT("setEnvironmentRotationCycleLength"));
	for_every(n, _node->children_named(TXT("setEnvironmentRotationCycleLengthFrom")))
	{
		setEnvironmentRotationCycleLengthFrom.speed = n->get_float_attribute_or_from_child(TXT("speed"));
		setEnvironmentRotationCycleLengthFrom.radius = n->get_float_attribute_or_from_child(TXT("radius"));
		setEnvironmentRotationCycleLengthFrom.mulResult = n->get_float_attribute_or_from_child(TXT("mulResult"), setEnvironmentRotationCycleLengthFrom.mulResult);
	}

	markMapKnown.load_from_xml(_node, TXT("markMapKnown"));

	advanceEnvironmentCycleBy.load_from_xml(_node, TXT("advanceEnvironmentCycleBy"));
	
	storeAnyTransitionRoomVar.load_from_xml(_node, TXT("storeAnyTransitionRoomVar"));

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::Pilgrimage::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* pi = PilgrimageInstanceOpenWorld::get())
	{
		if (createCellForObjectiveID.is_set() && createCellForObjectiveID.get().is_valid())
		{
			if (!pi->request_create_cell_for_objective(createCellForObjectiveID.get()))
			{
				return Framework::GameScript::ScriptExecutionResult::Repeat;
			}
			return Framework::GameScript::ScriptExecutionResult::Continue;
		}
		if (createCellAt.is_set())
		{
			if (!pi->issue_create_cell(createCellAt.get(), true, true))
			{
				return Framework::GameScript::ScriptExecutionResult::Repeat;
			}
			return Framework::GameScript::ScriptExecutionResult::Continue;
		}

		//

		if (pilgrimageInstanceShouldLeaveExitDoorAsIs.is_set())
		{
			pi->set_leave_starting_room_exit_door_as_is(pilgrimageInstanceShouldLeaveExitDoorAsIs.get());
		}
		if (giveControlToManageExitDoorToPilgrimageInstance.is_set())
		{
			pi->set_manage_starting_room_exit_door(giveControlToManageExitDoorToPilgrimageInstance.get());
			if (!giveControlToManageExitDoorToPilgrimageInstance.get())
			{
				pi->set_trigger_game_script_execution_trap_on_open_exit_door(Name::invalid());
			}
		}
		if (giveControlToManageEntranceDoorsToTransitionRoomsToPilgrimageInstance.is_set())
		{
			pi->set_manage_transition_room_entrance_door(giveControlToManageEntranceDoorsToTransitionRoomsToPilgrimageInstance.get());
		}
		if (keepExitDoorOpen.is_set())
		{
			pi->keep_starting_room_exit_door_open(keepExitDoorOpen.get());
		}
		if (closeExitDoor.is_set())
		{
			pi->set_starting_door_requested_to_be_closed(closeExitDoor.get());
		}
		if (setCanCreateNextPilgrimage.is_set())
		{
			pi->set_can_create_next_pilgrimage(setCanCreateNextPilgrimage.get());
		}
		if (triggerGameScriptExecutionTrapOnOpenExitDoor.is_set())
		{
			pi->set_trigger_game_script_execution_trap_on_open_exit_door(triggerGameScriptExecutionTrapOnOpenExitDoor.get());
		}
		if (doneObjectiveID.is_set() && doneObjectiveID.get().is_valid())
		{
			pi->done_objective(doneObjectiveID.get());
		}
		if (setDoorDirectionObjectiveOverride.is_set())
		{
			pi->set_door_direction_objective_override(setDoorDirectionObjectiveOverride.get());
		}
		if (clearDoorDirectionObjectiveOverride.get(false))
		{
			pi->set_door_direction_objective_override(NP);
		}
		if (setEnvironmentRotationCycleLength.is_set())
		{
			pi->set_environment_rotation_cycle_length(setEnvironmentRotationCycleLength.get());
		}
		if (setEnvironmentRotationCycleLengthFrom.speed != 0.0f &&
			setEnvironmentRotationCycleLengthFrom.radius != 0.0f)
		{
			/*
				make the environment go round
				(at radius in * 2pi)=length (45,8672432km)
				speed=30*(36/10)=108km/h
				1529s	= ((length(km)/speed(km/h)) * 3600 (per hour)) is for going around
						= ((radius(km)/speed(km/h)) * 3600 (per hour)) is for going straight
				second can be ignored
				result is first * 0.86 (at 60 angle)
			*/
			float circleLength = setEnvironmentRotationCycleLengthFrom.radius * ::pi<float>() * 2.0f;
			float circleLengthKM = circleLength / 1000.0f;
			float speedKMH = setEnvironmentRotationCycleLengthFrom.speed * 36.0f / 10.0f;
			float timeAround = (circleLengthKM / speedKMH) * 3600.0f;
			float result = timeAround * setEnvironmentRotationCycleLengthFrom.mulResult;
			pi->set_environment_rotation_cycle_length(result);
		}
		if (markMapKnown.is_set() && markMapKnown.get())
		{
			pi->mark_all_cells_known();
		}
		if (advanceEnvironmentCycleBy.is_set())
		{
			pi->modify_advance_environment_cycle_by(advanceEnvironmentCycleBy.get());
		}
		if (storeAnyTransitionRoomVar.is_set())
		{
			SafePtr<Framework::Room>& roomPtr = _execution.access_variables().access<SafePtr<Framework::Room>>(storeAnyTransitionRoomVar.get());
			roomPtr = pi->get_any_transition_room();
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
