#include "aiLogicTask_runAway.h"

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

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::run_away)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("run away"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_OPTIONAL(SafePtr<::Framework::IModulesOwner>, runAwayFromIMO, SafePtr<::Framework::IModulesOwner>()); // if not provided will just run away
	LATENT_PARAM_OPTIONAL(bool, neverStopRunningAway, false);
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	LATENT_VAR(bool, keepRunning);
	LATENT_VAR(Framework::RelativeToPresencePlacement, runAwayFrom);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("run away"));
	ai_log_no_colour(mind->get_logic());

	{
		if (runAwayFromIMO.get())
		{
			if (!runAwayFrom.find_path(mind->get_owner_as_modules_owner(), runAwayFromIMO.get(), false, 16))
			{
				// wait a bit and do something else
				LATENT_WAIT(Random::get_float(0.1f, 0.3f));
			}
		}
	}
	
	{
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.stop();
		}

		keepRunning = true;

		while (keepRunning || neverStopRunningAway)
		{
			if (!pathTask.is_set())
			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
				{
					Framework::Nav::PathRequestInfo pathRequestInfo(imo);
					if (!Mobility::may_leave_room_to_wander(imo, imo->get_presence()->get_in_room()))
					{
						pathRequestInfo.within_same_room_and_navigation_group();
					}

					auto* presence = imo->get_presence();
					if (mind->access_navigation().is_at_transport_route())
					{
						LATENT_LOG(TXT("run away -- transport route"));
						if (! pathTask.is_set())
						{
							LATENT_LOG(TXT("go through random door but not the one we've entered"));
							pathTask = mind->access_navigation().find_path_through(Framework::RoomUtils::get_random_door(presence->get_in_room(), imo->get_ai()->get_entered_through_door()), 0.5f, pathRequestInfo);
						}
					}
					else
					{
						if (!pathTask.is_set())
						{
							if (runAwayFrom.is_active())
							{
								LATENT_LOG(TXT("run away from"));
								Vector3 towards = runAwayFrom.get_direction_towards_placement_in_owner_room();
								pathTask = mind->access_navigation().get_random_path(Random::get_float(10.0f, 30.0f), -towards, pathRequestInfo);
							}
							else
							{
								LATENT_LOG(TXT("run away"));
								Vector3 towards = imo->get_presence()->get_placement().get_axis(Axis::Forward);
								pathTask = mind->access_navigation().get_random_path(Random::get_float(10.0f, 30.0f), towards, pathRequestInfo);
							}
						}
					}
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
					}
					else
					{
						LATENT_LOG(TXT("what?"));
						path.clear();
					}
				}
				else
				{
					LATENT_LOG(TXT("failed"));
					path.clear();
				}
				pathTask = nullptr;
			}

			MOVE:
			{
				{
					auto& locomotion = mind->access_locomotion();
					if (! locomotion.check_if_path_is_ok(path.get()))
					{
						locomotion.dont_move();
						goto MOVED;
					}
					else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
					{
						if (!locomotion.is_following_path(path.get()))
						{
							LATENT_LOG(TXT("move"));
							locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
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
					if (locomotion.has_finished_move(0.5f))
					{
						LATENT_LOG(TXT("reached destination"));
						locomotion.dont_move();
						goto MOVED;
					}
				}
				LATENT_WAIT(Random::get_float(0.1f, 0.3f));
			}
			goto MOVE;

			MOVED:
			{
				runAwayFrom.reset();
				keepRunning = false;
				LATENT_YIELD();
			}
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

