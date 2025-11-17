#include "aiLogicTask_stayCloseOrWander.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\modules\custom\mc_switchSidesHandler.h"

#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Tasks;

//

// tasks
DEFINE_STATIC_NAME(stayCloseSequence);
DEFINE_STATIC_NAME(wanderSequence);

//

LATENT_FUNCTION(Tasks::stay_close_or_wander)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("stay close or wander"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM_OPTIONAL(LatentFunction, stayCloseTaskParam, nullptr);
	LATENT_PARAM_OPTIONAL(LatentFunction, wanderTaskParam, nullptr);

	LATENT_END_PARAMS();

	LATENT_VAR(CustomModules::SwitchSidesHandler*, switchSidesHandler);
	LATENT_VAR(::Framework::IModulesOwner*, prevSwitchSidesTo);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	switchSidesHandler = imo->get_custom<CustomModules::SwitchSidesHandler>();

	while (true)
	{
		LATENT_CLEAR_LOG();
		LATENT_LOG(TXT("next task - clear"));
		nextTask.clear();
		{
			LatentFunction stayCloseTask = stayCloseTaskParam;
			LatentFunction wanderTask = wanderTaskParam;

			if (auto* logicLatent = fast_cast<::Framework::AI::LogicWithLatentTask>(mind->get_logic()))
			{
				if (!stayCloseTask)
				{
					stayCloseTask = logicLatent->get_latent_task(NAME(stayCloseSequence));
				}
				if (!wanderTask)
				{
					wanderTask = logicLatent->get_latent_task(NAME(wanderSequence));
				}
			}

			auto* sst = switchSidesHandler? switchSidesHandler->get_switch_sides_to() : nullptr;
			if (sst &&
				(prevSwitchSidesTo == sst || !prevSwitchSidesTo))
			{
				LATENT_LOG(TXT("stay close"));
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(stayCloseTask)))
				{
					ADD_LATENT_TASK_INFO_PARAM(nextTask, SafePtr<::Framework::IModulesOwner>, SafePtr<::Framework::IModulesOwner>(sst));
				}
			}
			else
			{
				LATENT_LOG(TXT("wander"));
				nextTask.propose(AI_LATENT_TASK_FUNCTION(wanderTask));
			}
		}
		LATENT_LOG(TXT("can start?"));
		if (currentTask.can_start(nextTask))
		{
			LATENT_LOG(TXT("start"));
			currentTask.start_latent_task(mind, nextTask);
		}
		LATENT_WAIT(Random::get_float(0.2f, 0.4f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	//

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(currentTask.can_start_new());

	LATENT_RETURN();
}
