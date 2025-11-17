#include "aiLogic_item.h"

#include "tasks\aiLogicTask_beingHeld.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_idle.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\..\modules\custom\mc_pickup.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

REGISTER_FOR_FAST_CAST(Item);

Item::Item(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Item::~Item()
{
}

void Item::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

void Item::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

LATENT_FUNCTION(Item::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Item>(logic);

	LATENT_BEGIN_CODE();

	self->asPickup = imo->get_custom<CustomModules::Pickup>();

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		{
			// choose best action for now
			nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle));
			if (ShouldTask::being_held(self->asPickup))
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_held), 20, NP, NP, true);
			}
			else if (ShouldTask::being_thrown(imo))
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_thrown), 20, NP, NP, true);
			}
			{
				// check if we can start new one, if so, start it
				if (currentTask.can_start(nextTask))
				{
					currentTask.start_latent_task(mind, nextTask);
				}
			}
			nextTask.clear();
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}
