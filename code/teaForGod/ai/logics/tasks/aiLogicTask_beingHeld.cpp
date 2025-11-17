#include "aiLogicTask_beingHeld.h"

#include "aiLogicTask_beingThrown.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\modules\custom\mc_pickup.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// var
DEFINE_STATIC_NAME(dontWalk);
DEFINE_STATIC_NAME(beingHeld);

// movement names
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(thrown);

//

bool TeaForGodEmperor::AI::Logics::ShouldTask::being_held(CustomModules::Pickup const* _asPickup)
{
	return _asPickup &&
		   (_asPickup->is_held() ||
			_asPickup->is_in_pocket() ||
			_asPickup->is_in_holder());
}

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::being_held)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("being held"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(PerceptionPause, perceptionPaused);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(TeaForGodEmperor::CustomModules::Pickup*, asPickup);

	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	perceptionPaused.pause(NAME(beingHeld));

	asPickup = imo->get_custom<TeaForGodEmperor::CustomModules::Pickup>();

	imo->activate_movement(NAME(held));
	imo->access_variables().access<float>(NAME(dontWalk)) = 1.0f;
	imo->access_variables().access<float>(NAME(beingHeld)) = 1.0f;

	while (asPickup && (asPickup->is_held() || asPickup->is_in_pocket() || asPickup->is_in_holder()))
	{
		LATENT_YIELD();
	}

	{
		Framework::AI::LatentTaskInfo taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::being_thrown));
		thisTaskHandle->switch_latent_task(mind, taskInfo);
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	imo->activate_movement(NAME(thrown));
	imo->access_variables().access<float>(NAME(dontWalk)) = 0.0f;
	imo->access_variables().access<float>(NAME(beingHeld)) = 0.0f;
	perceptionPaused.unpause(NAME(beingHeld));

	LATENT_END_CODE();
	LATENT_RETURN();
}

