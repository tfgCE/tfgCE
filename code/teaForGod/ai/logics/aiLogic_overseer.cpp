#include "aiLogic_overseer.h"

#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_lookAt.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\modules\gameplay\modulePilgrim.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

//

namespace LookAtBlockedFlag
{
	enum Type
	{
		Wandering = 1,
	};
};

//

REGISTER_FOR_FAST_CAST(Overseer);

Overseer::Overseer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Overseer::~Overseer()
{
}

void Overseer::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

static LATENT_FUNCTION(choose_look_at)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(::Framework::RelativeToPresencePlacement*, lookAtPlacement);
	LATENT_END_PARAMS();

	LATENT_VAR(float, timeLeft);

	timeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeLeft = 0.5f;

	lookAtPlacement->set_owner(mind->get_owner_as_modules_owner());

	while (true)
	{
		if (timeLeft <= 0.0f)
		{
			timeLeft = Random::get_float(2.0f, 7.0f);

			lookAtPlacement->clear_target();

			if (Random::get_chance(0.7f))
			{
				Transform placement = mind->get_owner_as_modules_owner()->get_presence()->get_placement();
				placement = placement.to_world(Transform(Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), 0.0f).normal() * Random::get_float(4.0f, 30.0f), Quat::identity));
				lookAtPlacement->set_placement_in_final_room(placement);
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	if (mind)
	{
		lookAtPlacement->clear_target();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(wander_around)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(int, action);
	LATENT_VAR(float, timeLeft);
	LATENT_VAR(Vector3, turnTarget);
	LATENT_VAR(float, moveRelativeSpeed);
	LATENT_VAR(::Framework::Nav::PlacementAtNode, moveTo);

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, navTask);

	timeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeLeft = 0.5f;

	enum Action
	{
		NoAction,
		Turn,
		Walk,
		ACTION_NUM
	};

	action = NoAction;
	while (true)
	{
		if (timeLeft <= 0.0f)
		{
			timeLeft = Random::get_float(3.0f, 7.0f);
			{
				int lastAction = action;
				while (lastAction == action)
				{
					float ch = Random::get_float(0.0f, 1.0f);
					if (ch < 0.5f) action = Walk; else
					if (ch < 0.8f) action = Turn; else
								   action = NoAction;
				}
			}
			if (action == NoAction)
			{
				timeLeft = Random::get_float(1.0f, 2.0f);
			}
			if (action == Walk)
			{
				moveTo.clear();
				{
					if (auto* presence = mind->get_owner_as_modules_owner()->get_presence())
					{
						Vector3 at = presence->get_placement().location_to_world(Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), 0.0f).normal() * Random::get_float(5.0f, 20.0f));
						moveTo = mind->access_navigation().find_nav_location(presence->get_in_room(), Transform(at, Quat::identity));
					}
				}

				mind->access_locomotion().stop();

				if (Random::get_chance(0.3f))
				{
					moveRelativeSpeed = clamp(Random::get_float(0.4f, 0.8f), 0.0f, 1.0f);
				}
				else
				{
					moveRelativeSpeed = clamp(Random::get_float(0.8f, 1.1f), 0.0f, 1.0f);
				}
				moveRelativeSpeed = clamp(Random::get_float(0.4f, 1.3f), 0.0f, 1.0f);

				if (moveTo.is_valid())
				{
					{
						auto* imo = mind->get_owner_as_modules_owner();
						navTask = mind->access_navigation().find_path_to(moveTo, Framework::Nav::PathRequestInfo(imo));
					}
					LATENT_NAV_TASK_WAIT_IF_SUCCEED(navTask)
					{
						if (auto* findPath = fast_cast<Framework::Nav::Tasks::PathTask>(navTask.get()))
						{
							path = findPath->get_path();
							auto & locomotion = mind->access_locomotion();
							locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().relative_speed(moveRelativeSpeed));
							locomotion.turn_follow_path_2d();
						}
					}
					else
					{
						auto & locomotion = mind->access_locomotion();
						locomotion.stop();
					}
					navTask = nullptr;
				}
				timeLeft = Random::get_float(20.0f, 80.0f); // should be enough to allow reaching destination
			}
			if (action == Turn)
			{
				timeLeft = Random::get_float(1.0f, 2.0f);
				Transform placement = mind->get_owner_as_modules_owner()->get_presence()->get_placement();
				// in front
				turnTarget = placement.location_to_world(Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(0.0f, 1.0f), 0.0f).normal() * 3.0f);
			}
		}
		if (mind)
		{
			::Framework::AI::Locomotion & locomotion = mind->access_locomotion();

			if (action == NoAction)
			{
				locomotion.stop();
			}
			if (action == Turn)
			{
				// we still require some time to finish it (check result from last frmae)
				timeLeft = locomotion.has_finished_turn() ? 0.0f : 1.0f;

				locomotion.stop();
				locomotion.turn_towards_2d(turnTarget, 10.0f);
			}
			if (action == Walk)
			{
				if (path.is_set() && locomotion.is_done_with_move())
				{
					// we're done!
					timeLeft = 0.0f;
				}
				// locomotion order has been issued earlier
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Overseer::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(::Framework::RelativeToPresencePlacement, lookAtPlacement);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(int, lookAtBlocked);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, chooseLookAtTask);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(float, timeToCircle);

	timeToCircle -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeToCircle = Random::get_float(10.0f, 30.0f);
	lookAtBlocked = 0;

	messageHandler.use_with(mind);

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &lookAtBlocked);
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &lookAtPlacement);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams chooseLookAtTaskInfo;
		chooseLookAtTaskInfo.clear();
		chooseLookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(choose_look_at));
		ADD_LATENT_TASK_INFO_PARAM(chooseLookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &lookAtPlacement);
		chooseLookAtTask.start_latent_task(mind, chooseLookAtTaskInfo);
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{

		{
			if (currentTask.can_start_new())
			{
				// choose best action for now
				::Framework::AI::LatentTaskInfoWithParams nextTask;

				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle));
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(wander_around), 5))
				{
				}

				// check if we can start new one, if so, start it
				if (currentTask.can_start(nextTask))
				{
					currentTask.start_latent_task(mind, nextTask);
				}
			}
		}
		LATENT_WAIT(Random::get_float(0.05f, 0.25f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	lookAtTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
