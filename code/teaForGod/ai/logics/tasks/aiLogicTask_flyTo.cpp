#include "aiLogicTask_flyTo.h"

#include "..\aiLogic_npcBase.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\general\cooldowns.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\nav\navPath.h"
#include "..\..\..\..\framework\nav\navTask.h"
#include "..\..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\..\framework\world\room.h"

//
 
#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;

//

// room tags
DEFINE_STATIC_NAME(freeFlyingRoom);

// cooldowns
DEFINE_STATIC_NAME(redoPath);

// variables
DEFINE_STATIC_NAME(isFreeFlyer);
DEFINE_STATIC_NAME(allowRedoFlyPath);
DEFINE_STATIC_NAME(freeFlyingRoomRelativeSpeed);
DEFINE_STATIC_NAME(nonFreeFlyingRoomRelativeSpeed);

//

LATENT_FUNCTION(Logics::Tasks::fly_to)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("fly to"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM(::Framework::RelativeToPresencePlacement*, flyTo);
	LATENT_PARAM_OPTIONAL(bool, flyJustToRoom, false); // instead of the right location
	LATENT_PARAM_OPTIONAL(bool*, out_reachedTarget, nullptr);
	LATENT_PARAM_OPTIONAL(Name, gait, Name::invalid());
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(::Framework::Room*, inRoom);
	LATENT_VAR(bool, inFreeFlyingRoom);

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	LATENT_VAR(::Framework::Room*, pathForRoom);

	// temp
	LATENT_VAR(float, relativeSpeed);
	LATENT_VAR(float, relativeZ);
	LATENT_VAR(Vector3, holeCentreWS);
	LATENT_VAR(float, distToDoor);

	LATENT_VAR(bool, isFreeFlyer);
	LATENT_VAR(bool, allowRedoFlyPath);
	LATENT_VAR(float, freeFlyingRoomRelativeSpeed);
	LATENT_VAR(float, nonFreeFlyingRoomRelativeSpeed);
	
	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Logics::NPCBase>(mind->get_logic());

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	assign_optional_out_param(out_reachedTarget, false);

	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
		locomotion.turn_in_movement_direction_2d();
	}

	isFreeFlyer = imo->get_variables().get_value<bool>(NAME(isFreeFlyer), false);
	allowRedoFlyPath = imo->get_variables().get_value<bool>(NAME(allowRedoFlyPath), false);
	freeFlyingRoomRelativeSpeed = imo->get_variables().get_value<float>(NAME(freeFlyingRoomRelativeSpeed), 1.0f);
	nonFreeFlyingRoomRelativeSpeed = imo->get_variables().get_value<float>(NAME(nonFreeFlyingRoomRelativeSpeed), 1.0f);

	while (true)
	{
		if (! flyTo->is_active())
		{
			LATENT_BREAK();
		}

		{
			auto* nowInRoom = imo->get_presence()->get_in_room();
			if (inRoom != nowInRoom)
			{
				LATENT_LOG(TXT("changed room"));
				inRoom = nowInRoom;
				if (flyJustToRoom && inRoom == flyTo->get_in_final_room())
				{
					assign_optional_out_param(out_reachedTarget, true);
					LATENT_BREAK();
				}
				relativeSpeed = inFreeFlyingRoom ? freeFlyingRoomRelativeSpeed : nonFreeFlyingRoomRelativeSpeed;
				if (flyTo->get_target())
				{
					relativeZ = flyTo->get_target()->get_presence()->get_centre_of_presence_os().get_translation().z;
				}
				else if (inRoom)
				{
					//FOR_EVERY_DOOR_IN_ROOM(dir, inRoom)
					for_every_ptr(dir, inRoom->get_doors())
					{
						if (!dir->is_visible()) continue;
						relativeZ = dir->get_hole_centre_WS().z;
						break;
					}
				}
			}
		}

		{
			bool nowInFreeFlyingRoom = isFreeFlyer && inRoom ? inRoom->get_tags().get_tag(NAME(freeFlyingRoom)) > 0.0f : false;
			if (nowInFreeFlyingRoom != inFreeFlyingRoom)
			{
				inFreeFlyingRoom = nowInFreeFlyingRoom;
				if (isFreeFlyer)
				{
					LATENT_LOG(TXT("free flyer changed flying room"));
					// if has nav path, check if doesn't need to redo it
					if (path.is_set())
					{
						path.clear();
						if (pathTask.is_set())
						{
							pathTask->cancel();
						}
						// but allow to continue movement on that path
					}
				}
			}
		}

		if (allowRedoFlyPath)
		{
			if (cooldowns.is_available(NAME(redoPath)))
			{
				cooldowns.set(NAME(redoPath), 0.3f);
				auto& locomotion = mind->access_locomotion();
				if (locomotion.is_following_path(path.get()))
				{
					if (path->get_final_room() != flyTo->get_in_final_room() ||
						(path->get_final_node_location() - flyTo->get_placement_in_final_room().get_translation()).length() > 0.5f)
					{
						LATENT_LOG(TXT("redo path"));
						cooldowns.set(NAME(redoPath), 0.6f);
						path.clear();
						if (pathTask.is_set())
						{
							pathTask->cancel();
						}
						// but allow to continue movement on that path
					}
				}
			}
		}

		{
			bool findPath = true;
			if (isFreeFlyer && flyTo->get_through_door())
			{
				holeCentreWS = flyTo->get_through_door()->get_hole_centre_WS();
				distToDoor = (holeCentreWS - imo->get_presence()->get_centre_of_presence_WS()).length();
				if (inFreeFlyingRoom || distToDoor < 0.5f)
				{
					auto& locomotion = mind->access_locomotion();
					Framework::MovementParameters params;
					params.relative_speed(relativeSpeed).relative_z(relativeZ);
					if (gait.is_valid())
					{
						params.gait(gait);
					}
					locomotion.move_to_3d(holeCentreWS + flyTo->get_through_door()->get_outbound_matrix().get_y_axis(), 0.0f, params);
					findPath = false;
				}
			}
			if (findPath)
			{
				bool followNewPath = false;
				// find path right to the end and follow it
				if (!path.is_set() && !pathTask.is_set())
				{
					LATENT_LOG(TXT("find new path"));
					Framework::Nav::PathRequestInfo pathRequestInfo(imo);
					pathRequestInfo.with_dev_info(TXT("Tasks::fly_to"));
					if (self && self->should_stay_in_room())
					{
						pathRequestInfo.within_same_room_and_navigation_group();
					}
					if (isFreeFlyer)
					{
						pathTask = mind->access_navigation().find_path_through(flyTo->get_through_door(), 0.5f, pathRequestInfo);
					}
					else
					{
						pathTask = mind->access_navigation().find_path_to(flyTo->get_in_final_room(), flyTo->get_placement_in_final_room().get_translation(), pathRequestInfo);
					}
				}
				LATENT_NAV_TASK_IF_DONE(pathTask)
				{
					LATENT_LOG(TXT("done find new path"));
					if (pathTask->has_succeed())
					{
						LATENT_LOG(TXT("succeeded"));
						if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
						{
							LATENT_LOG(TXT("new path"));
							path = task->get_path();
							pathForRoom = path.is_set() ? path->get_start_room() : nullptr;
							followNewPath = true;
						}
						else
						{
							path.clear();
						}
					}
					else
					{
						path.clear();
					}
					pathTask.clear();
				}
				if (followNewPath && path.is_set())
				{
					auto & locomotion = mind->access_locomotion();
					Framework::MovementParameters params;
					params.relative_speed(relativeSpeed).relative_z(relativeZ);
					if (gait.is_valid())
					{
						params.gait(gait);
					}
					locomotion.follow_path_3d(path.get(), 0.2f, params);
				}
			}
		}

		if (path.is_set() && ! flyJustToRoom)
		{
			auto& locomotion = mind->access_locomotion();
			if (! locomotion.is_following_path() || locomotion.has_finished_move())
			{
				assign_optional_out_param(out_reachedTarget, true);
				LATENT_BREAK();
			}
		}

		LATENT_WAIT(rg.get_float(0.1f, 0.4f));
	}

	LATENT_ON_BREAK();
	LATENT_ON_END();
	//
	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Logics::Tasks::fly_to_room)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("fly to room"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM(::Framework::InRoomPlacement*, flyToRoom);
	LATENT_PARAM_OPTIONAL(bool, flyJustToRoom, false); // instead of the right location
	LATENT_PARAM_OPTIONAL(bool*, out_reachedTarget, nullptr);
	LATENT_PARAM_OPTIONAL(Name, gait, Name::invalid());
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::RelativeToPresencePlacement, rtpp);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	rtpp.find_path(imo, flyToRoom->inRoom.get(), flyToRoom->placement);

	{
		::Framework::AI::LatentTaskInfoWithParams nextTask;
		nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to));
		ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &rtpp);
		ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, flyJustToRoom);
		ADD_LATENT_TASK_INFO_PARAM(nextTask, bool*, out_reachedTarget);
		ADD_LATENT_TASK_INFO_PARAM(nextTask, Name, gait);

		currentTask.start_latent_task(mind, nextTask);
	}

	while (currentTask.is_running())
	{
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

