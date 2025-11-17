#include "aiLogicTask_scriptedMoveToPOI.h"

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
#include "..\..\..\..\framework\world\subWorld.h"

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

// object variables
DEFINE_STATIC_NAME(scriptedMoveToPOI); // poi name
DEFINE_STATIC_NAME(scriptedMovedToPOI); // if is at POI
DEFINE_STATIC_NAME(scriptedMoveToPOISpeed);
DEFINE_STATIC_NAME(scriptedMoveToPOIDirectly); // false (default) will use path

//

LATENT_FUNCTION(Tasks::scripted_move_to_poi)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("scripted_move_to_poi"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(SimpleVariableInfo, scriptedMoveToPOI);
	LATENT_VAR(SimpleVariableInfo, scriptedMovedToPOI);
	LATENT_VAR(SimpleVariableInfo, scriptedMoveToPOISpeed);
	LATENT_VAR(SimpleVariableInfo, scriptedMoveToPOIDirectly);
	
	LATENT_VAR(Optional<Vector3>, moveToLocation);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("scripted move to poi"));
	ai_log_no_colour(mind->get_logic());

	{
		MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("setup variables"));
		auto& variables = imo->access_variables();
		scriptedMoveToPOI.set_name(NAME(scriptedMoveToPOI));
		scriptedMoveToPOI.look_up<Name>(variables);
		scriptedMovedToPOI.set_name(NAME(scriptedMovedToPOI));
		scriptedMovedToPOI.look_up<bool>(variables);
		scriptedMoveToPOISpeed.set_name(NAME(scriptedMoveToPOISpeed));
		scriptedMoveToPOISpeed.look_up<float>(variables);
		scriptedMoveToPOIDirectly.set_name(NAME(scriptedMoveToPOIDirectly));
		scriptedMoveToPOIDirectly.look_up<bool>(variables);
	}

	scriptedMovedToPOI.access<bool>() = false;

	{
		{
			auto& locomotion = mind->access_locomotion();
			locomotion.dont_move();
			locomotion.dont_turn();
		}
	}

	while (! scriptedMovedToPOI.get<bool>())
	{
		LATENT_CLEAR_LOG();
		LATENT_LOG(TXT("[%i]"), System::Core::get_frame());

		if (!pathTask.is_set() && ! moveToLocation.is_set())
		{
			LATENT_LOG(TXT("find POI"));

			Framework::PointOfInterestInstance* foundPOI = nullptr;
			Name moveToPOI = scriptedMoveToPOI.get<Name>();
			if (moveToPOI.is_valid())
			{
				if (!foundPOI)
				{
					if (imo->get_presence()->get_in_room()->find_any_point_of_interest(moveToPOI, foundPOI))
					{
						LATENT_LOG(TXT("find POI in the same room"));
						moveToLocation = foundPOI->calculate_placement().get_translation();
					}
				}
				if (!scriptedMoveToPOIDirectly.get<bool>())
				{
					if (!foundPOI)
					{
						if (auto* sw = imo->get_in_sub_world())
						{
							if (sw->find_any_point_of_interest(moveToPOI, foundPOI))
							{
								LATENT_LOG(TXT("find POI in a different room"));
							}
						}
					}

					if (foundPOI)
					{
						LATENT_LOG(TXT("find path"));

						Framework::Nav::PathRequestInfo pathRequestInfo(imo);
						pathTask = mind->access_navigation().find_path_to(foundPOI->get_room(), foundPOI->calculate_placement().get_translation(), pathRequestInfo);
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
			}

			pathTask = nullptr;
		}

		MOVE:
		{
			{
				auto& locomotion = mind->access_locomotion();
				if (scriptedMoveToPOIDirectly.get<bool>() && !moveToLocation.is_set())
				{
					ai_log(mind->get_logic(), TXT("no point to move to"));
					locomotion.dont_move();
					goto MOVED;
				}
				else if (!scriptedMoveToPOIDirectly.get<bool>() && !locomotion.check_if_path_is_ok(path.get()))
				{
					ai_log(mind->get_logic(), TXT("path is not ok"));
					locomotion.dont_move();
					goto MOVED;
				}
				else if (scriptedMoveToPOIDirectly.get<bool>() || (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f)))
				{
					if ((scriptedMoveToPOIDirectly.get<bool>() && !locomotion.is_moving()) ||
						(!scriptedMoveToPOIDirectly.get<bool>() && !locomotion.is_following_path(path.get())))
					{
						LATENT_LOG(TXT("move"));
						Framework::MovementParameters mp;
						float speed = scriptedMoveToPOISpeed.get<float>();
						if (speed > 0.0f)
						{
							mp.absolute_speed(speed);
						}
						else
						{
							mp.full_speed();
						}
						if (auto* npcBase = fast_cast<NPCBase>(mind->get_logic()))
						{
							if (npcBase->get_movement_gait().is_valid())
							{
								mp.gait(npcBase->get_movement_gait());
							}
						}
						if (scriptedMoveToPOIDirectly.get<bool>())
						{
							locomotion.move_to_2d(moveToLocation.get(), 0.1f, mp);
							locomotion.turn_towards_2d(moveToLocation.get(), DEG_ 5.0f);
						}
						else
						{
							locomotion.follow_path_2d(path.get(), NP, mp);
							locomotion.turn_follow_path_2d();
						}
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
					scriptedMovedToPOI.access<bool>() = true;

					ai_log(mind->get_logic(), TXT("reached destination"));
					locomotion.dont_move();
					goto REACHED_TARGET;
				}
			}
			LATENT_YIELD();
		}
		goto MOVE;

		MOVED:
		LATENT_YIELD();

		continue;

		REACHED_TARGET:

		scriptedMovedToPOI.access<bool>() = true;
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	{
		// reset variables after use
		scriptedMoveToPOISpeed.access<float>() = 0.0f;
		scriptedMoveToPOIDirectly.access<bool>() = false;
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

