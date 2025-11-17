#include "aiLogicTask_attackOrIdle.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\core\random\random.h"
#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"

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

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// tasks
DEFINE_STATIC_NAME(attackSequence);
DEFINE_STATIC_NAME(idleSequence);

//

LATENT_FUNCTION(Tasks::attack_or_idle)
{
	/**
	*	Uses perception task to look for enemy
	*	If enemy is found and we're aggressive enough, attack enemy
	*	If no enemy or we ignore enemy, idle
	*/
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack or idle"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM_OPTIONAL(LatentFunction, attackTaskParam, nullptr);
	LATENT_PARAM_OPTIONAL(LatentFunction, idleTaskParam, nullptr);

	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, unableToAttack);
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	LATENT_VAR(bool, preferToInvestigate);

	auto* imo = mind->get_owner_as_modules_owner();

	LatentFunction attackTask = attackTaskParam;
	LatentFunction idleTask = idleTaskParam;

	if (auto* logicLatent = fast_cast<::Framework::AI::LogicWithLatentTask>(mind->get_logic()))
	{
		if (!attackTask)
		{
			attackTask = logicLatent->get_latent_task(NAME(attackSequence));
		}
		if (!idleTask)
		{
			idleTask = logicLatent->get_latent_task(NAME(idleSequence));
		}
	}

	LATENT_BEGIN_CODE();

	preferToInvestigate = Random::get_chance(0.5f);

	while (true)
	{
		LATENT_CLEAR_LOG();

		if (auto* taskFeedback = currentTask.get_result().get(NAME(taskFeedback)))
		{
			if (taskFeedback->get_as<Name>() == NAME(enemyLost))
			{
				todo_note(TXT("actually we should pursuit in such case"));
				enemy.clear();
			}
		}

		LATENT_LOG(TXT("%c can attack (! unableToAttack)"), !unableToAttack? '+' : '-');
		LATENT_LOG(TXT("%c enemy.is_set()"), enemy.is_set() ? '+' : '-');
		LATENT_LOG(TXT("%c aggressiveness > 0.0f"), aggressiveness > 0.0f ? '+' : '-');
		LATENT_LOG(TXT("%c enemyPlacement.is_active()"), enemyPlacement.is_active() ? '+' : '-');
		LATENT_LOG(TXT("%c !investigate.is_valid()"), !investigate.is_valid() ? '+' : '-');
		LATENT_LOG(TXT("%c enemyPlacement.get_target()"), enemyPlacement.get_target() ? '+' : '-');
		LATENT_LOG(TXT("%c !preferToInvestigate"), !preferToInvestigate ? '+' : '-');
		LATENT_LOG(TXT("%c Investigate::is_close_and_in_front(imo, 2.0f, 60.0f)"), Investigate::is_close_and_in_front(imo, 2.0f, 60.0f) ? '+' : '-');

		nextTask.clear();
		if (enemy.is_set() && aggressiveness > 0.0f &&
			!unableToAttack &&
			enemyPlacement.is_active() &&
			(!investigate.is_valid() || (enemyPlacement.get_target() || !preferToInvestigate) || (investigate.is_valid() && Investigate::is_close_and_in_front(imo, 2.0f, 60.0f))))
		{
			LATENT_LOG(TXT("choose to attack enemy"));
			nextTask.propose(AI_LATENT_TASK_FUNCTION(attackTask));
		}
		else
		{
			LATENT_LOG(TXT("choose to idle"));
			nextTask.propose(AI_LATENT_TASK_FUNCTION(idleTask));
		}

		if (currentTask.can_start(nextTask))
		{
			ai_log(mind->get_logic(), TXT("attack_or_idle, switch task to %S"), nextTask.taskFunctionInfo.taskFunction == attackTask? TXT("attack") : TXT("idle"));
			currentTask.start_latent_task(mind, nextTask);
		}

		LATENT_WAIT(Random::get_float(0.01f, 0.12f)); // reaction time

		if (Random::get_chance(0.2f))
		{
			// prefer not to look at something poking you but attack
			if (investigate.is_valid() && Investigate::is_close_and_in_front(imo, 2.0f, 60.0f))
			{
				preferToInvestigate = Random::get_chance(0.02f);
			}
			else
			{
				preferToInvestigate = Random::get_chance(0.5f);
			}
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(currentTask.can_start_new());

	LATENT_RETURN();
}