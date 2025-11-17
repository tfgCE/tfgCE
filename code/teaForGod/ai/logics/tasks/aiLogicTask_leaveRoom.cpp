#include "aiLogicTask_leaveRoom.h"

#include "..\..\aiCommon.h"
#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\..\framework\nav\navPath.h"
#include "..\..\..\..\framework\nav\tasks\navTask_PathTask.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
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

// scripted behaviours
DEFINE_STATIC_NAME(leaveRoom);

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::leave_room)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("leave room"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	LATENT_VAR(::Framework::Room*, startedInRoom);
	LATENT_VAR(bool, keepRunning);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("leave room"));
	ai_log_no_colour(mind->get_logic());

	{
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.stop();
		}

		if (auto* presence = imo->get_presence())
		{
			startedInRoom = presence->get_in_room();
		}

		keepRunning = true;

		while (keepRunning)
		{
			if (!pathTask.is_set())
			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
				{
					LATENT_LOG(TXT("go through random door"));
					// try to avoid going through the door we went through
					auto* presence = imo->get_presence();
					Framework::Nav::PathRequestInfo pathRequestInfo(imo);
					if (auto* throughDoor = Framework::RoomUtils::get_random_door(presence->get_in_room(), imo->get_ai()->get_entered_through_door()))
					{
						pathTask = mind->access_navigation().find_path_through(throughDoor, 0.5f, pathRequestInfo);
					}
					else
					{
						warn(TXT("no exit door"));
						pathTask = mind->access_navigation().get_random_path(5.0f, NP, pathRequestInfo);
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
					if (locomotion.has_finished_move(0.5f) &&
						imo->get_presence()->get_in_room() != startedInRoom)
					{
						LATENT_LOG(TXT("left room"));
						locomotion.dont_move();
						goto MOVED;
					}
				}
				LATENT_WAIT(Random::get_float(0.1f, 0.3f));
			}
			goto MOVE;

			MOVED:
			{
				keepRunning = false;
				LATENT_YIELD();
			}
		}
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	Common::unset_scripted_behaviour(scriptedBehaviour, NAME(leaveRoom));

	LATENT_END_CODE();
	LATENT_RETURN();
}

