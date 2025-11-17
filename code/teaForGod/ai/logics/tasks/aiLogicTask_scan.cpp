#include "aiLogicTask_scan.h"

#include "..\aiLogic_npcBase.h"

#include "aiLogicTask_lookAt.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\..\framework\world\roomUtils.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Tasks;

//

LATENT_FUNCTION(Tasks::scan)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("scan"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(::Framework::RelativeToPresencePlacement, lookAtPlacement);

	LATENT_VAR(bool, turnNow);
	
	LATENT_VAR(float, scanningFov);
	LATENT_VAR(float, scanningSpeed);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	lookAtPlacement.usageInfo = Name(TXT("scan; lookAtPlacement"));
#endif

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("scan"));
	ai_log_no_colour(mind->get_logic());

	scanningFov = 90.0f;
	scanningSpeed = 40.0f;
	{
		auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
		an_assert(npcBase);
		scanningFov = npcBase->get_scanning_fov();
		scanningSpeed = npcBase->get_scanning_speed();
	}

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &lookAtPlacement);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	lookAtPlacement.set_owner(imo);
	lookAtPlacement.clear_target();
	lookAtPlacement.set_placement_in_final_room(imo->get_presence()->get_placement().to_world(Transform(Vector3::yAxis * 100.0f, Quat::identity)));

	turnNow = false;
	while (true)
	{
		LATENT_CLEAR_LOG();
		if (investigate.is_valid())
		{
			turnNow = false;

			if (investigate.inRoom.get() == imo->get_presence()->get_in_room())
			{
				float scanYaw = imo->get_presence()->get_placement().location_to_local(lookAtPlacement.get_placement_in_owner_room().get_translation()).to_rotator().yaw;
				float reqYaw = imo->get_presence()->get_placement().location_to_local(investigate.placement.get_translation()).to_rotator().yaw;
				turnNow = abs(Rotator3::normalise_axis(reqYaw - scanYaw)) > 10.0f;
				if (lookAtPlacement.get_in_final_room() != imo->get_presence()->get_in_room())
				{
					lookAtPlacement.clear_target();
				}
				LATENT_LOG(TXT("look at investigate in the same room"));
				lookAtPlacement.set_placement_in_final_room(investigate.placement);
			}
			else
			{
				LATENT_LOG(TXT("look at investigate in other room"));
				if (auto* door = Framework::RoomUtils::get_door_towards(imo->get_presence()->get_in_room(), imo, investigate))
				{
					LATENT_LOG(TXT(" (through door)"));
					lookAtPlacement.clear_target();
					Vector3 lookAt = door->get_hole_centre_WS();
					if (auto* npcBase = fast_cast<NPCBase>(mind->get_logic()))
					{
						// drop on flat, so it doesn't look down on door (unless door is further away)
						Transform perceptionSocketOS = npcBase->get_scanning_perception_socket_index() != NONE ? imo->get_appearance()->calculate_socket_os(npcBase->get_scanning_perception_socket_index()) : imo->get_presence()->get_centre_of_presence_os();
						Transform perceptionSocketWS = imo->get_presence()->get_placement().to_world(perceptionSocketOS);

						Vector3 f = perceptionSocketWS.get_translation();

						bool lookFlat = (lookAt - f).length() < 5.0f;

						if (lookFlat)
						{
							Vector3 up = imo->get_presence()->get_placement().get_axis(Axis::Up);
							lookAt = f + (lookAt - f).drop_using(up);
						}
					}
					lookAtPlacement.set_placement_in_final_room(Transform(lookAt, Quat::identity));
				}
			}

			if (turnNow)
			{
				auto & locomotion = mind->access_locomotion();
				LATENT_LOG(TXT("turn towards"));
				locomotion.turn_towards_2d(investigate.placement.get_translation(), 5.0f);
			}
		}
		else
		{
			{
				float scanYaw = imo->get_presence()->get_placement().location_to_local(lookAtPlacement.get_placement_in_owner_room().get_translation()).to_rotator().yaw;
				turnNow = scanYaw >= scanningFov * 0.5f;
				scanYaw += scanningSpeed * LATENT_DELTA_TIME;
				LATENT_LOG(TXT("scan"));
				lookAtPlacement.set_placement_in_final_room(imo->get_presence()->get_placement().to_world(Transform(Vector3::yAxis.rotated_by_yaw(scanYaw) * 100.0f, Quat::identity)));
			}

			if (turnNow)
			{
				auto & locomotion = mind->access_locomotion();
				if (locomotion.has_finished_turn(5.0f))
				{
					// always turn right
					Vector3 relLoc = Vector3::yAxis.rotated_by_yaw(Random::get_float(scanningFov * 0.6f, scanningFov * 1.3f));
					Vector3 loc = imo->get_presence()->get_placement().location_to_world(relLoc);
					locomotion.stop();
					locomotion.turn_towards_2d(loc, 5.0f);
				}
			}
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	lookAtTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
