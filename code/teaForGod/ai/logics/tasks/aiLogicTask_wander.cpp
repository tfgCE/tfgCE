#include "aiLogicTask_wander.h"

#include "..\aiLogic_npcBase.h"

#include "aiLogicTask_lookAt.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\core\system\core.h"

#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\nav\navTask.h"
#include "..\..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\roomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Tasks;

//

LATENT_FUNCTION(Tasks::wander)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("wander"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, perceptionStartOff);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR(int, investigateIdx);
	LATENT_COMMON_VAR(bool, investigateGlance);
	LATENT_COMMON_VAR(bool, encouragedInvestigate);
	LATENT_COMMON_VAR(bool, forceInvestigate);
	LATENT_COMMON_VAR(Vector3, defaultTargetingOffsetOS);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(Random::Generator, rg);

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(bool, isInvestigating);
	LATENT_VAR(int, isInvestigatingIdx);
	LATENT_VAR(SafePtr<::Framework::Room>, investigatingFromRoom);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(int, lookAtBlocked);
	LATENT_VAR(bool, glancing);
	LATENT_VAR(::Framework::RelativeToPresencePlacement, rtppGlancing);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("wander"));
	ai_log_no_colour(mind->get_logic());

	// if we're supposed to wait, wait idle
	if (perceptionStartOff)
	{
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.dont_move();
			locomotion.dont_turn();
		}
		while (perceptionStartOff)
		{
			LATENT_WAIT(rg.get_float(0.2f, 0.6f));
		}
	}

	isInvestigating = false;
	isInvestigatingIdx = NONE;

	if (investigateGlance)
	{
		glancing = false;
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.dont_move();
		
			// setup and start look at task
			if (rtppGlancing.find_path(imo, investigate.inRoom.get(), investigate.placement))
			{
				::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
				lookAtTaskInfo.clear();
				lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
				ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &lookAtBlocked);
				ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::PresencePath*, nullptr);
				ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &rtppGlancing);
				ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, Vector3*, &defaultTargetingOffsetOS);
				lookAtTask.start_latent_task(mind, lookAtTaskInfo);
				glancing = true;

				locomotion.turn_towards_2d(rtppGlancing, 10.0f);
			}
		}
		while (glancing && investigateGlance)
		{
			LATENT_WAIT(rg.get_float(0.1f, 0.3f));
		}
		lookAtTask.stop();
	}

	while (true)
	{
		LATENT_CLEAR_LOG();
		LATENT_LOG(TXT("[%i]"), System::Core::get_frame());
		if (!pathTask.is_set())
		{
			auto& locomotion = mind->access_locomotion();
			if (locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
			{
				bool wantsToInvestigate = investigate.is_valid();
				Framework::Nav::PathRequestInfo pathRequestInfo(imo);
				pathRequestInfo.with_dev_info(TXT("Tasks::wander"));
				if (wantsToInvestigate)
				{
					if (!Mobility::may_leave_room_to_investigate(imo, imo->get_presence()->get_in_room()) && ! encouragedInvestigate)
					{
						ai_log(mind->get_logic(), TXT("within same navigation group"));
						pathRequestInfo.within_same_room_and_navigation_group();
					}
					if (forceInvestigate)
					{
						// they should be waiting for the player
						pathRequestInfo.through_closed_doors();
					}
				}
				else
				{
					if (Mobility::should_remain_stationary(imo))
					{
						goto IDLE;
					}
					if (!Mobility::may_leave_room_to_wander(imo, imo->get_presence()->get_in_room()))
					{
						ai_log(mind->get_logic(), TXT("within same navigation group"));
						pathRequestInfo.within_same_room_and_navigation_group();
					}
				}

				auto* presence = imo->get_presence();
				if (mind->access_navigation().is_at_transport_route())
				{
					ai_log(mind->get_logic(), TXT("wander; transport route"));
					LATENT_LOG(TXT("wander; transport route"));
					if (wantsToInvestigate)
					{
						isInvestigating = true;
						isInvestigatingIdx = investigateIdx;
						if (investigate.inRoom == presence->get_in_room())
						{
							ai_log(mind->get_logic(), TXT("investigate, go through that door"));
							LATENT_LOG(TXT("investigate, go through that door"));
							pathTask = mind->access_navigation().find_path_through(Framework::RoomUtils::find_closest_door(investigate.inRoom.get(), investigate.placement.get_translation()), 0.5f, pathRequestInfo);
						}
						else
						{
							ai_log(mind->get_logic(), TXT("investigate, find path to where we want to go [%S]"), investigate.inRoom.get()? investigate.inRoom.get()->get_name().to_char() : TXT("--"));
							LATENT_LOG(TXT("investigate, find path to where we want to go"));
							pathTask = mind->access_navigation().find_path_to(investigate.inRoom.get(), investigate.placement.get_translation(), pathRequestInfo);
						}
					}
					if (! pathTask.is_set())
					{
						isInvestigating = false;
						ai_log(mind->get_logic(), isInvestigating ? TXT("investigate? go through random door but not the one we've entered")
							: TXT("go through random door but not the one we've entered"));
						LATENT_LOG(isInvestigating? TXT("investigate? go through random door but not the one we've entered")
							: TXT("go through random door but not the one we've entered"));
						pathTask = mind->access_navigation().find_path_through(Framework::RoomUtils::get_random_door(presence->get_in_room(), imo->get_ai()->get_entered_through_door()), 0.5f, pathRequestInfo);
					}
				}
				else
				{
					if (wantsToInvestigate)
					{
						ai_log(mind->get_logic(), TXT("investigate, go there [%S]"), investigate.inRoom.get()? investigate.inRoom.get()->get_name().to_char() : TXT("[no room provide]"));
						LATENT_LOG(TXT("investigate, go there (remain within nav group)"));
						isInvestigating = true;
						isInvestigatingIdx = investigateIdx;
						pathTask = mind->access_navigation().find_path_to(investigate.inRoom.get(), investigate.placement.get_translation(), pathRequestInfo);
					}
					if (!pathTask.is_set())
					{
						ai_log(mind->get_logic(), TXT("wander randomly"));
						LATENT_LOG(TXT("wander randomly (remain within nav group)"));
						isInvestigating = false;
						isInvestigatingIdx = investigateIdx;
						bool goForward = false;
						if (auto* npc = fast_cast<NPCBase>(mind->get_logic()))
						{
							goForward = npc->should_wander_forward();
						}
						if (goForward)
						{
							pathTask = mind->access_navigation().get_random_path(rg.get_float(2.0f, 10.0f), imo->get_presence()->get_placement().get_axis(Axis::Forward), pathRequestInfo);
						}
						else
						{
							pathTask = mind->access_navigation().get_random_path(rg.get_float(2.0f, 10.0f), NP, pathRequestInfo);
						}
					}
				}
				if (isInvestigating)
				{
					investigatingFromRoom = imo->get_presence()->get_in_room();
					ai_log(mind->get_logic(), TXT("started investigating in room %S"), investigatingFromRoom->get_name().to_char());
				}
				REMOVE_AS_SOON_AS_POSSIBLE_ ai_log(mind->get_logic(), TXT("path task %p"), pathTask.get());
			}
		}
		
		if (pathTask.is_set())
		{
			LATENT_LOG(TXT("wait for nav task to end"));

			LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
			{
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					LATENT_LOG(TXT("got a path"));
					path = task->get_path();
					ai_log(mind->get_logic(), TXT("got path to wander (%.3f) from %S to %S"),
						path->calculate_length(),
						path->get_start_room()? path->get_start_room()->get_name().to_char() : TXT("--"),
						path->get_final_room()? path->get_final_room()->get_name().to_char() : TXT("--")
						);
				}
				else
				{
					LATENT_LOG(TXT("what?"));
					path.clear();
					ai_log(mind->get_logic(), TXT("no path to wander"));
				}
			}
			else
			{
				LATENT_LOG(TXT("failed"));
				path.clear();
				ai_log(mind->get_logic(), TXT("failed finding path to wander"));
				if (isInvestigating && ! forceInvestigate)
				{
					ai_log(mind->get_logic(), TXT("don't investigate any more"));
					investigate.clear();
					++ investigateIdx;
				}
			}
			pathTask = nullptr;
		}

		MOVE:
		{
			{
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					ai_log(mind->get_logic(), TXT("path is not ok"));
					locomotion.dont_move();
					goto MOVED;
				}
				else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
				{
					if (!locomotion.is_following_path(path.get()))
					{
						LATENT_LOG(TXT("move"));
						Framework::MovementParameters mp;
						mp.full_speed();
						if (auto* npcBase = fast_cast<NPCBase>(mind->get_logic()))
						{
							if (npcBase->get_movement_gait().is_valid())
							{
								mp.gait(npcBase->get_movement_gait());
							}
						}
						locomotion.follow_path_2d(path.get(), NP, mp);
						locomotion.turn_follow_path_2d();
					}
				}
				else
				{
					locomotion.dont_move();
				}
			}

			{
				auto& locomotion = mind->access_locomotion();
				if (isInvestigating && investigateIdx != isInvestigatingIdx)
				{
					ai_log(mind->get_logic(), TXT("should investigate something else %i (%i)"), investigateIdx, isInvestigatingIdx);
					locomotion.dont_move();
					LATENT_END();
				}
				if (!isInvestigating && investigate.is_valid())
				{
					ai_log(mind->get_logic(), TXT("should investigate"));
					locomotion.dont_move();
					LATENT_END();
				}
				if (locomotion.has_finished_move(0.5f))
				{
					ai_log(mind->get_logic(), TXT("reached destination"));
					locomotion.dont_move();
					if (isInvestigating)
					{
						ai_log(mind->get_logic(), TXT("end investigation"));
						// clear investigation
						investigate.clear();
						++investigateIdx;
						isInvestigating = false;
					}
					goto MOVED;
				}
			}
			LATENT_YIELD();
			//LATENT_WAIT(rg.get_float(0.1f, 0.3f));
		}
		goto MOVE;

		MOVED:

		if (!investigate.is_valid())
		{
			LATENT_WAIT(rg.get_float(0.0f, 1.0f));
		}

		if (isInvestigating)
		{
			if (investigate.inRoom == imo->get_presence()->get_in_room())
			{
				ai_log(mind->get_logic(), TXT("was investigating and reached the investigation point in the same room"));
				LATENT_END();
			}
			if (investigatingFromRoom == imo->get_presence()->get_in_room() &&
				investigatingFromRoom != investigate.inRoom)
			{
				ai_log(mind->get_logic(), TXT("was investigating but hasn't went outside the starting room"));
				LATENT_END();
			}
		}

		continue;

		IDLE:
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.dont_move();
		}
		LATENT_WAIT(rg.get_float(0.5f, 1.5f));

		continue;

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

