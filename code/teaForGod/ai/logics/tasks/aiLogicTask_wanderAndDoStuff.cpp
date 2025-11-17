#include "aiLogicTask_wanderAndDoStuff.h"

#include "..\aiLogic_npcBase.h"

#include "aiLogicTask_idle.h"
#include "aiLogicTask_wander.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Tasks;

//

LATENT_FUNCTION(Tasks::wander_and_do_stuff)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("wander and do stuff"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_OPTIONAL(LATENT_FUNCTION_TYPE, doStuff, Tasks::idle);
	LATENT_PARAM_OPTIONAL(Range, doStuffTime, Range::empty); // if empty, it will be done until task ends
	LATENT_PARAM_OPTIONAL(Range, wanderTime, Range(3.0f, 10.0f));
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, encouragedInvestigate);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, task);
	LATENT_VAR(float, timeLeft);

	timeLeft -= LATENT_DELTA_TIME;

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("wander and do stuff"));
	ai_log_no_colour(mind->get_logic());

	if (Random::get_chance(0.5f))
	{
		goto WANDER;
	}

	while (true)
	{
		LATENT_CLEAR_LOG();
		//DO_STUFF:
		{
			if (!task.is_running(doStuff))
			{
				::Framework::AI::LatentTaskInfoWithParams taskInfo;
				taskInfo.clear();
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(doStuff));
				task.start_latent_task(mind, taskInfo);
			}
			if (! doStuffTime.is_empty())
			{
				timeLeft = Random::get(doStuffTime);
				while (timeLeft > 0.0f && task.is_running())
				{
					LATENT_YIELD();
				}
			}
			else
			{
				while (task.is_running())
				{
					LATENT_YIELD();
				}
			}
		}

		WANDER:
		{
			if (Mobility::should_remain_stationary(imo) && !encouragedInvestigate)
			{
				// if to remain stationary, just stay here
			}
			else if (!task.is_running(wander))
			{
				::Framework::AI::LatentTaskInfoWithParams taskInfo;
				taskInfo.clear();
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(wander));
				task.start_latent_task(mind, taskInfo);
			}
			an_assert(!wanderTime.is_empty());
			LATENT_WAIT(Random::get(wanderTime));
		}

	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	task.stop();

	LATENT_END_CODE();

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(task.can_start_new());

	LATENT_RETURN();
}

