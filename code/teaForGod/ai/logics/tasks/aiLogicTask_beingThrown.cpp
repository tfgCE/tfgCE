#include "aiLogicTask_beingThrown.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// var
DEFINE_STATIC_NAME(dontWalk);
DEFINE_STATIC_NAME(beingThrown);
DEFINE_STATIC_NAME(walkerShouldFall);
DEFINE_STATIC_NAME(locomotionShouldFall);

// movement names
DEFINE_STATIC_NAME(thrown);

//

bool TeaForGodEmperor::AI::Logics::ShouldTask::being_thrown(Framework::IModulesOwner const * _imo)
{
	if (_imo->get_movement() &&
		_imo->get_movement()->get_name() == NAME(thrown))
	{
		return true;
	}
	if (_imo->get_variables().get_value<bool>(NAME(walkerShouldFall), false) ||
		_imo->get_variables().get_value<bool>(NAME(locomotionShouldFall), false))
	{
		return true;
	}
	return false;
}

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::being_thrown)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("being thrown"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(PerceptionPause, perceptionPaused);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(float, timeBeingThrown);

	auto * imo = mind->get_owner_as_modules_owner();

	timeBeingThrown += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeBeingThrown = 0.0f;

	perceptionPaused.pause(NAME(beingThrown));

	// force into being thrown state
	if (!imo->get_movement() || imo->get_movement()->get_name() != NAME(thrown))
	{
		imo->activate_movement(NAME(thrown));
	}

	imo->access_variables().access<float>(NAME(dontWalk)) = 1.0f;
	imo->access_variables().access<float>(NAME(beingThrown)) = 1.0f;

	while (true)
	{
		if (auto* c = imo->get_collision())
		{
			if (!c->get_collided_with().is_empty() &&
				imo->get_presence()->get_velocity_linear().length() < 1.0f)
			{
				if (timeBeingThrown < 0.2f)
				{
					if (imo->get_presence()->get_velocity_rotation().length() < 1.0f)
					{
						Random::Generator rg;
						// to tip it over, so it won't just lay there and stay in falling/not falling loop
						imo->get_presence()->add_orientation_impulse(Rotator3(rg.get_float(-10.0f, 10.0f), rg.get_float(-2.0f, 2.0f), rg.get_float(-10.0f, 10.0f)));
					}
				}
				LATENT_BREAK();
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	imo->activate_default_movement();
	imo->access_variables().access<float>(NAME(dontWalk)) = 0.0f;
	imo->access_variables().access<float>(NAME(beingThrown)) = 0.0f;
	perceptionPaused.unpause(NAME(beingThrown));

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::being_thrown_and_stay)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("being thrown"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(PerceptionPause, perceptionPaused);
	LATENT_COMMON_VAR_END();

	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	perceptionPaused.pause(NAME(beingThrown));

	// force into being thrown state
	if (!imo->get_movement() || imo->get_movement()->get_name() != NAME(thrown))
	{
		imo->activate_movement(NAME(thrown));
	}

	imo->access_variables().access<float>(NAME(dontWalk)) = 1.0f;
	imo->access_variables().access<float>(NAME(beingThrown)) = 1.0f;

	while (true)
	{
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	imo->activate_default_movement();
	imo->access_variables().access<float>(NAME(dontWalk)) = 0.0f;
	imo->access_variables().access<float>(NAME(beingThrown)) = 0.0f;
	perceptionPaused.unpause(NAME(beingThrown));

	LATENT_END_CODE();
	LATENT_RETURN();
}

