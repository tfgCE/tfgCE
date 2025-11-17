#include "aiLogic_battery.h"

#include "components\aiLogicComponent_confussion.h"

#include "tasks\aiLogicTask_beingHeld.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_projectileHit.h"
#include "tasks\aiLogicTask_wander.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_energyStorage.h"
#include "..\..\modules\custom\mc_interactiveButtonHandler.h"
#include "..\..\modules\custom\mc_pickup.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\general\cooldowns.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_PathTask.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(evade);
DEFINE_STATIC_NAME(projectileHit);
DEFINE_STATIC_NAME(duck);
DEFINE_STATIC_NAME(shake);

// var
DEFINE_STATIC_NAME(dontWalk);
DEFINE_STATIC_NAME(beingHeld);
DEFINE_STATIC_NAME(beingThrown);

// movement names
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(thrown);

// cooldowns
DEFINE_STATIC_NAME(changeEnemy);
DEFINE_STATIC_NAME(allowPath);
DEFINE_STATIC_NAME(forceRedoPath);

// room tags
DEFINE_STATIC_NAME(largeRoom);

// emissives
DEFINE_STATIC_NAME(scared);

//

namespace LookAtBlockedFlag
{
	enum Type
	{
		StopAndDuck = 1,
		Wandering = 2,
	};
};

//

REGISTER_FOR_FAST_CAST(Battery);

Battery::Battery(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Battery::~Battery()
{
}

void Battery::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

void Battery::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

static LATENT_FUNCTION(wander_around)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("wander around"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM_NOT_USED(::Framework::PresencePath*, enemyPath);
	LATENT_PARAM(int*, lookAtBlocked);
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	LATENT_VAR(float, timeLeft);
	LATENT_VAR(float, lookAtTimeLeft);

	timeLeft -= LATENT_DELTA_TIME;
	lookAtTimeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeLeft = 0.0f;
	lookAtTimeLeft = 0.0f;

	set_flag(*lookAtBlocked, LookAtBlockedFlag::Wandering);

	while (true)
	{
		if (lookAtTimeLeft <= 0.0f)
		{
			if (Random::get_chance(0.5f))
			{
				set_flag(*lookAtBlocked, LookAtBlockedFlag::Wandering);
			}
			else
			{
				clear_flag(*lookAtBlocked, LookAtBlockedFlag::Wandering);
			}
			lookAtTimeLeft = Random::get_float(0.3f, 1.3f);
		}
		if (timeLeft <= 0.0f)
		{
			timeLeft = Random::get_float(2.0f, 3.0f);
			if (Random::get_chance(0.7f))
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
			}

			if (nextTask.is_proposed())
			{
				if (currentTask.can_start(nextTask))
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("start_latent_task"));
					currentTask.start_latent_task(mind, nextTask);
				}
			}
			else
			{
				if (currentTask.is_running())
				{
					currentTask.stop();
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
	currentTask.stop();
	clear_flag(*lookAtBlocked, LookAtBlockedFlag::Wandering);

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(stop_and_duck)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("stop and duck"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(::Framework::PresencePath*, enemyPath);
	LATENT_PARAM(int*, lookAtBlocked);
	LATENT_PARAM(bool, aimingAtMe);
	LATENT_PARAM(bool, triesToPickup);
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	if (aimingAtMe)
	{
		set_flag(*lookAtBlocked, LookAtBlockedFlag::StopAndDuck);
	}

	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();

		if (Random::get_chance(0.2f))
		{
			if (auto* target = enemyPath->get_target())
			{
				locomotion.turn_towards_3d(target, 1.0f);
			}
		}
	}

	if (auto * imo = mind->get_owner_as_modules_owner())
	{
		imo->access_variables().access<float>(NAME(duck)) = 1.0f;
		imo->access_variables().access<float>(NAME(shake)) = aimingAtMe ? 1.0f : 0.0f;
		if (auto * ec = imo->get_custom<CustomModules::EmissiveControl>())
		{
			ec->emissive_activate(NAME(scared));
		}
	}

	while (true)
	{
		if (aimingAtMe)
		{
			if (! Tasks::is_aiming_at_me(*enemyPath, mind->get_owner_as_modules_owner(), 0.15f, 5.0f, 3.0f))
			{
				break;
			}
		}
		else if (triesToPickup)
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto * p = imo->get_custom<CustomModules::Pickup>())
				{
					if (p->get_interested_picking_up_count() == 0)
					{
						break;
					}
				}
			}
		}
		else
		{
			break;
		}

		LATENT_WAIT(Random::get_float(0.2f, 0.4f));
	}

	LATENT_ON_BREAK();
	//
	if (auto * imo = mind->get_owner_as_modules_owner())
	{
		imo->access_variables().access<float>(NAME(duck)) = 0.0f;
		imo->access_variables().access<float>(NAME(shake)) = 0.0f;
		if (auto * ec = imo->get_custom<CustomModules::EmissiveControl>())
		{
			ec->emissive_deactivate(NAME(scared));
		}
	}
	if (aimingAtMe)
	{
		clear_flag(*lookAtBlocked, LookAtBlockedFlag::StopAndDuck);
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Battery::act_normal)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("act normal"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM(::Framework::PresencePath*, enemyPath);
	LATENT_PARAM(int*, lookAtBlocked);
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(float, timeToCircle);

	timeToCircle -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeToCircle = Random::get_float(10.0f, 30.0f);
	
	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(projectileHit), [mind, framePtr, &nextTask](Framework::AI::Message const & _message)
		{
			if (Tasks::handle_projectile_hit_message(mind, nextTask, _message, 0.6f, Tasks::avoid_projectile_hit_3d, 20))
			{
				framePtr->end_waiting();
				AI_LATENT_STOP_LONG_RARE_ADVANCE();
			}
		}
		);
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{

		{
			auto * imo = mind->get_owner_as_modules_owner();
			// update target via perception requests

			if (currentTask.can_start_new())
			{
				// choose best action for now
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle));
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("propose wander"));
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(wander_around), 5))
					{
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, enemyPath);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, int*, lookAtBlocked);
					}
				}
				if (auto * target = enemyPath->get_target())
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("check if aiming"));
					/*
					if (timeToCircle < 0.0f &&
					!enemyPath.get_through_door() &&
					enemyPath.calculate_string_pulled_distance() < 4.0f)
					{
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(circle_around_enemy), 5))
					{
					ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, &enemyPath);
					ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, Random::get_chance(0.5f));
					}
					timeToCircle = Random::get_float(10.0f, 30.0f);
					}
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(follow_enemy)))
					{
					ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, &enemyPath);
					}
					*/
					if (Tasks::is_aiming_at_me(*enemyPath, imo, 0.15f, 5.0f, 3.0f))
					{
						LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("propose for aiming"));
						if (nextTask.propose(AI_LATENT_TASK_FUNCTION(stop_and_duck), 10, NP, true))
						{
							ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, enemyPath);
							ADD_LATENT_TASK_INFO_PARAM(nextTask, int*, lookAtBlocked);
							ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // aimingAtMe
							ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, false); // triesToPickup
						}
					}
					else
					{
						if (auto* p = imo->get_custom<CustomModules::Pickup>())
						{
							if (p->get_interested_picking_up_count() > 0)
							{
								LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("propose for picking up"));
								if (nextTask.propose(AI_LATENT_TASK_FUNCTION(stop_and_duck), 10, NP, true))
								{
									ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, enemyPath);
									ADD_LATENT_TASK_INFO_PARAM(nextTask, int*, lookAtBlocked);
									ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, false); // aimingAtMe
									ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // triesToPickup
								}
							}
						}
					}
				}

				// check if we can start new one, if so, start it
				if (currentTask.can_start(nextTask))
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("start_latent_task"));
					currentTask.start_latent_task(mind, nextTask);
				}
				nextTask.clear();
			}
		}
		LATENT_WAIT(Random::get_float(0.05f, 0.25f));
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Battery::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);
	LATENT_VAR(::Framework::PresencePath, enemyPath);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(int, lookAtBlocked);
	
	LATENT_VAR(::Framework::AI::PerceptionRequestPtr, perceptionRequest); // one perception should be enough

	LATENT_VAR(bool, isHeldOrThrown);

	LATENT_VAR(Framework::CooldownsSmall, cooldowns);

	LATENT_VAR(SafePtr<Framework::IModulesOwner>, heldBy);
	LATENT_VAR(float, heldByTime);

	LATENT_VAR(Confussion, confussion);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Battery>(logic);

	confussion.advance(LATENT_DELTA_TIME);

	cooldowns.advance(LATENT_DELTA_TIME);
	heldByTime += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	self->asPickup = imo->get_custom<CustomModules::Pickup>();
	self->asInteractiveButtonHandler = imo->get_custom<CustomModules::InteractiveButtonHandler>();
	self->asEnergyStorage = imo->get_custom<CustomModules::EnergyStorage>();

	confussion.setup(mind);

	lookAtBlocked = 0;

	isHeldOrThrown = false;

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &lookAtBlocked);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::PresencePath*, &enemyPath);
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		if (self->asPickup)
		{
			auto* curHeldBy = self->asPickup->get_held_by();
			if (heldBy != curHeldBy)
			{
				heldBy = curHeldBy;
				heldByTime = 0.0f;
			}
		}
		{
			Framework::IModulesOwner* shouldTransferTo = nullptr;
			if (self->asInteractiveButtonHandler &&
				self->asEnergyStorage &&
				self->asPickup &&
				self->asInteractiveButtonHandler->is_on() &&
				heldByTime > 0.5f) // give it a moment to settle down
			{
				shouldTransferTo = self->asPickup->get_held_by(); // hand!
			}
			if (self->transferringEnergyTo != shouldTransferTo)
			{
				if (auto* imo = self->transferringEnergyTo.get())
				{
					self->asEnergyStorage->end_transfer(imo);
					if (auto* ec = imo->get_custom<CustomModules::EmissiveControl>())
					{
						ec->emissive_activate(NAME(scared));
					}
				}
				self->transferringEnergyTo = shouldTransferTo;
				if (auto* imo = self->transferringEnergyTo.get())
				{
					self->asEnergyStorage->begin_transfer(imo);
				}
			}
			if (auto* imo = self->transferringEnergyTo.get())
			{
				self->asEnergyStorage->transfer_energy_to(imo, LATENT_DELTA_TIME);
			}
		}
		{
			auto * imo = mind->get_owner_as_modules_owner();
			// update target via perception requests
			{
				if (!isHeldOrThrown)
				{
					if (confussion.is_confused())
					{
						enemyPath.reset();
						perceptionRequest = nullptr;
					}
					else if (!enemyPath.is_active())
					{
						if (cooldowns.is_available(NAME(changeEnemy)))
						{
							if (!perceptionRequest.is_set())
							{
								//perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindInRoom(imo,
								perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindAnywhere(imo,
									[](Framework::IModulesOwner const* _object) { return _object->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>() != nullptr ? 1.0f : 0.0f; }));
							}
						}
					}
				}
			
				if (perceptionRequest.get() && perceptionRequest->is_processed())
				{
					if (auto* request = fast_cast<AI::PerceptionRequests::FindAnywhere>(perceptionRequest.get()))
					{
						enemyPath = request->get_path();
					}
					perceptionRequest = nullptr;
					cooldowns.set(NAME(changeEnemy), Random::get_float(5.0f, 30.0f));
				}
			}

			// always try to simplify
			enemyPath.update_simplify();

			{
				// choose best action for now
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle));
				isHeldOrThrown = false;
				if (ShouldTask::being_held(self->asPickup))
				{
					nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_held), 20, NP, NP, true);
					isHeldOrThrown = true;
				}
				else if (ShouldTask::being_thrown(imo))
				{
					nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_thrown), 20, NP, NP, true);
					isHeldOrThrown = true;
				}
				if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
				{
					// doing scripted behaviour
				}
				else
				{				
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(act_normal), 5))
					{
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, &enemyPath);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, int*, &lookAtBlocked);
					}

					// check if we can start new one, if so, start it
					if (currentTask.can_start(nextTask))
					{
						currentTask.start_latent_task(mind, nextTask);
					}
				}
				nextTask.clear();
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	lookAtTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
