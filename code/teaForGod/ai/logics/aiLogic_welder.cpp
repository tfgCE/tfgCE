#include "aiLogic_welder.h"

#include "actions\aiAction_lightningStrike.h"

#include "tasks\aiLogicTask_flyTo.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_perception.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\library\library.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\framework\world\presenceLink.h"

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME(remoteControl);

// sounds
DEFINE_STATIC_NAME(attack);
DEFINE_STATIC_NAME(engine);

// gaits
DEFINE_STATIC_NAME(fast);
DEFINE_STATIC_NAME(normal);
DEFINE_STATIC_NAME(keepClose);

// variables
DEFINE_STATIC_NAME(dischargeDamage);
DEFINE_STATIC_NAME(dischargeSocket);

// lightnings
DEFINE_STATIC_NAME(weld);

// gaits
DEFINE_STATIC_NAME(react);

// temporary objects
DEFINE_STATIC_NAME_STR(weldingHit, TXT("welding hit"));

//

REGISTER_FOR_FAST_CAST(Welder);

Welder::Welder(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	shield.set_owner(_mind->get_owner_as_modules_owner());
}

Welder::~Welder()
{
}

void Welder::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	dischargeDamage = Energy::get_from_storage(_parameters, NAME(dischargeDamage), dischargeDamage);
	dischargeSocket = _parameters.get_value<Name>(NAME(dischargeSocket), dischargeSocket);

	shield.learn_from(_parameters);
}

LATENT_FUNCTION(Welder::manage_shield)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("manage shield"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(Random::Generator, rg);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(float, forceShieldDown);

	auto* self = fast_cast<Welder>(mind->get_logic());

	self->shield.advance(LATENT_DELTA_TIME);

	forceShieldDown = max(0.0f, forceShieldDown - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(remoteControl), [framePtr, mind, &forceShieldDown](Framework::AI::Message const& _message)
			{
				// force switch off deflection
				forceShieldDown = max(forceShieldDown, REMOTE_CONTROL_EFFECT_TIME);

				framePtr->end_waiting();
				AI_LATENT_STOP_LONG_RARE_ADVANCE();
			}
		);
	}

	while (true)
	{
		self->shield.set_shield_requested(forceShieldDown <= 0.0f && (enemyPlacement.get_target() || self->shieldRequested));
		LATENT_WAIT(rg.get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	LATENT_ON_END();
	//
	self->shield.recall_shield(false);

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Welder::attack_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(float, timeLeft);
	LATENT_VAR(bool, isOk);
	LATENT_VAR(int, dischargeCount);
	LATENT_VAR(int, dischargeCountLeft);
	LATENT_VAR(Energy, damageLeft);

	LATENT_VAR(Random::Generator, rg);

	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);

	auto* imo = mind->get_owner_as_modules_owner();
#ifdef AN_USE_AI_LOG
	auto* logic = mind->get_logic();
#endif
	auto* self = fast_cast<Welder>(mind->get_logic());

	timeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

	if (!lightningSpawner)
	{
		LATENT_BREAK();
	}

	ai_log(logic, TXT("attack enemy"));

	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_activate(NAME(attack));
		}
	}

	if (!enemyPlacement.is_active())
	{
		LATENT_BREAK();
	}

	if (auto* s = imo->get_sound())
	{
		s->play_sound(NAME(attack));
	}

	perceptionEnemyLock.override_lock(NAME(attack), AI::PerceptionLock::Lock);

	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.move_to_3d(enemyPlacement.get_target(), 0.5f, Framework::MovementParameters().full_speed().gait(NAME(keepClose)));
		locomotion.turn_towards_2d(enemyPlacement, 5.0f);
		locomotion.keep_distance_2d(enemyPlacement.get_target(), Random::get_float(0.4f, 0.9f), 1.2f, 0.3f);
	}

	// no telegraph, just attack

	// extra time if we're too far
	timeLeft = 1.0f;
	isOk = false;
	while (timeLeft > 0.0f && !isOk)
	{
		{
			::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
			if (locomotion.is_done_with_move() &&
				locomotion.is_done_with_turn())
			{
				isOk = true;
			}
		}
		LATENT_YIELD();
	}

	if (enemyPlacement.calculate_string_pulled_distance() < 1.5f)
	{
		isOk = true;
	}

	if (! isOk)
	{
		LATENT_BREAK();
	}

	dischargeCount = 10;
	dischargeCountLeft = dischargeCount;
	damageLeft = self->dischargeDamage;
	while (dischargeCountLeft > 0 && enemyPlacement.get_target())
	{
		{
			Energy damageOne = self->dischargeDamage.div((float)dischargeCount);
			Energy damageNow = dischargeCountLeft == 1? damageLeft : clamp(damageOne, Energy::zero(), damageLeft);
			if (!Actions::do_lightning_strike(mind, enemyPlacement, magic_number 1.8f,
				Actions::LightningStrikeParams().with_angle_limit(60.0f)
												.with_vertical_angle_limit(90.0f)
												.from_socket(self->dischargeSocket)
												.discharge_damage(damageNow)
												.ignore_violence_disallowed(ignoreViolenceDisallowed)))
			{
				dischargeCountLeft = 0;
			}
			damageLeft -= damageNow;
		}
		--dischargeCountLeft;
		LATENT_WAIT(rg.get_float(0.05f, 0.1f));
	}

	LATENT_ON_BREAK();
	//
	{
		//::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		//locomotion.stop();
	}

	LATENT_ON_END();
	//
	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_deactivate(NAME(attack));
		}
	}
	perceptionEnemyLock.override_lock(NAME(attack));

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Welder::weld_something)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("weld something"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Framework::RelativeToPresencePlacement, weldPlacement);
	LATENT_VAR(Vector3, weldOffset);

	LATENT_VAR(float, timeLeft);

	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);
	LATENT_VAR(Random::Generator, rg);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	auto* imo = mind->get_owner_as_modules_owner();
#ifdef AN_USE_AI_LOG
	auto* logic = mind->get_logic();
#endif
	auto* self = fast_cast<Welder>(mind->get_logic());

	timeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

	if (!lightningSpawner)
	{
		LATENT_BREAK();
	}

	while (true)
	{
		LATENT_CLEAR_LOG();

		self->shieldRequested = false;

		FIND_TARGET:
		LATENT_LOG(TXT("find something to weld"));
		{
			Vector3 startLocation = imo->get_presence()->get_centre_of_presence_WS();
			Vector3 endLocation = Vector3::zero;
			endLocation.x = rg.get_float(-1.0f, 1.0f);
			endLocation.y = rg.get_float(-1.0f, 1.0f);
			endLocation.z = rg.get_float(-0.8f, 0.8f);
			endLocation = startLocation + endLocation.normal() * 10.0f;

			Framework::CheckCollisionContext checkCollisionContext;
			checkCollisionContext.collision_info_needed();
			checkCollisionContext.use_collision_flags(imo->get_collision()->get_collides_with_flags());
			checkCollisionContext.ignore_temporary_objects();
			checkCollisionContext.ignore_actors();
			checkCollisionContext.ignore_items();

			Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
			collisionTrace.add_location(startLocation);
			collisionTrace.add_location(endLocation);
			Framework::CheckSegmentResult result;
			weldPlacement.reset();
			weldPlacement.set_owner(imo);

			if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInPresenceRoom, checkCollisionContext, &weldPlacement))
			{
				weldOffset = result.hitNormal * 0.5f;
			}
		}

		if (!weldPlacement.is_active())
		{
			LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			goto FIND_TARGET;
		}

		LATENT_LOG(TXT("go to something to weld"));

		{
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to)))
			{
				ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &weldPlacement);
			}
			currentTask.start_latent_task(mind, nextTask);
		}

		timeLeft = rg.get_float(5.0f, 15.0f);
		while (weldPlacement.calculate_string_pulled_distance() > 2.0f && timeLeft > 0.0f)
		{
			/*
			debug_context(imo->get_presence()->get_in_room());
			debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::blue, imo->get_presence()->get_placement().get_translation(),
				weldPlacement.get_placement_in_owner_room().get_translation()));
			debug_no_context();
			*/
			LATENT_WAIT(rg.get_float(0.1f, 0.6f));
		}

		// no longer needed, we will be switching soon to own locomotion
		currentTask.stop();

		if (timeLeft <= 0.0f)
		{
			LATENT_LOG(TXT("didn't get there"));
			goto FIND_TARGET;
		}

		if (imo->get_presence()->get_in_room() != weldPlacement.get_in_final_room())
		{
			LATENT_LOG(TXT("not the same room"));
			goto FIND_TARGET;
		}

		LATENT_LOG(TXT("get really close"));
		{
			::Framework::AI::Locomotion& locomotion = mind->access_locomotion();
			//locomotion.move_to_3d(weldPlacement.get_placement_in_final_room().get_translation() + weldOffset, 0.2f, Framework::MovementParameters().full_speed().gait(NAME(keepClose)));
			locomotion.turn_towards_2d(weldPlacement.get_placement_in_final_room().get_translation(), 5.0f);
			locomotion.keep_distance_2d(weldPlacement.get_placement_in_final_room().get_translation(), 0.3f, NP, 0.2f);
			timeLeft = 5.0f;
		}
		while (timeLeft > 0.0f &&
			   (!mind->access_locomotion().has_finished_move() ||
			    !mind->access_locomotion().has_finished_turn()))
		{
			LATENT_WAIT(rg.get_float(0.1f, 0.6f));
		}

		{
			::Framework::AI::Locomotion& locomotion = mind->access_locomotion();
			locomotion.stop();
			locomotion.turn_towards_2d(weldPlacement.get_placement_in_final_room().get_translation(), 5.0f);
		}

		LATENT_LOG(TXT("weld something"));

		self->shieldRequested = true;
		LATENT_WAIT(0.2f); // wait for shield

		timeLeft = rg.get_float(1.0f, 5.0f);
		while (timeLeft > 0.0f)
		{
			if (!Actions::do_lightning_strike(mind, weldPlacement, magic_number 1.8f,
				Actions::LightningStrikeParams().with_angle_limit(40.0f)
												.with_vertical_angle_limit(90.0f)
												.lightning_spawn_id(NAME(weld))
												.from_socket(self->dischargeSocket)
												.discharge_damage(Energy(0.1f)) // some damage
												.force_discharge()
												.with_extra_distance(1.0f)
												.hit_temporary_object_id(NAME(weldingHit))))
			{
				timeLeft = 0.0f;
			}
			LATENT_WAIT(rg.get_float(0.1f, 0.2f));
		}

		self->shieldRequested = false;

		LATENT_LOG(TXT("welded"));

		LATENT_WAIT(rg.get_float(1.0f, 2.0f));
	}

	LATENT_ON_BREAK();
	//
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_ON_END();
	//
	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_deactivate(NAME(attack));
		}
	}
	currentTask.stop();

	self->shieldRequested = false;

	LATENT_END_CODE();
	LATENT_RETURN();
}

enum BehaveTP // tasks priority
{
	WeldSomething = 0,
	Investigate = 5,
	FlyAroundEnemy = 10, // same as fly around to allow fly around to take over, when enemy is not there
	Attack = 20,
};
LATENT_FUNCTION(Welder::behave)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("behave"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR(int, investigateIdx);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(Optional<float>, updateTime);
	LATENT_VAR(float, investigatingTime);
	LATENT_VAR(float, investigatingTimeThreshold);
	LATENT_VAR(bool, investigatingReached);

	LATENT_VAR(int, isInvestigatingIdx);

	LATENT_VAR(bool, enemyPlacementReached);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Welder>(mind->get_logic());

	investigatingTime += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	investigatingTimeThreshold = 10.0f;

	if (auto * ms = imo->get_sound())
	{
		ms->play_sound(NAME(engine));
	}

	while (true)
	{
		{
			updateTime.clear();

			::Framework::AI::LatentTaskInfoWithParams nextTask;
			// choose best action for now
			nextTask.propose(AI_LATENT_TASK_FUNCTION(weld_something), BehaveTP::WeldSomething);
			if (investigate.is_valid())
			{
				if (isInvestigatingIdx != investigateIdx)
				{
					investigatingReached = false;
					investigatingTime = 0.0f;
				}
				if (investigatingTime < investigatingTimeThreshold && !investigatingReached)
				{
					isInvestigatingIdx = investigateIdx;
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to_room), BehaveTP::Investigate))
					{
						self->shieldRequested = true;
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::InRoomPlacement*, &investigate);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, false);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, bool*, &investigatingReached);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, Name, NAME(react));
					}
				}
			}
			else
			{
				investigatingTime = 0.0f;
			}
			if (enemyPlacement.is_active())
			{
				if (enemyPlacement.calculate_string_pulled_distance() < 2.0f)
				{
					if (enemyPlacement.get_target() &&
						(!GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed))
					{
						enemyPlacementReached = false;
						if (nextTask.propose(AI_LATENT_TASK_FUNCTION(attack_enemy), BehaveTP::Attack))
						{
							self->shieldRequested = true;
							updateTime = Random::get_float(1.0f, 2.0f);
						}
					}
					if (! enemyPlacementReached && nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to), BehaveTP::FlyAroundEnemy))
					{
						self->shieldRequested = true;
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &enemyPlacement);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, false);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, bool*, &enemyPlacementReached);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, Name, NAME(react));
						updateTime.clear();
					}
				}
				else if(! enemyPlacementReached)
				{
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to), BehaveTP::FlyAroundEnemy))
					{
						self->shieldRequested = true;
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &enemyPlacement);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, false);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, bool*, nullptr);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, Name, NAME(react));
						updateTime.clear();
					}
				}
			}
			else
			{
				enemyPlacementReached = false;
			}

			// check if we can start new one, if so, start it
			if (currentTask.can_start(nextTask))
			{
				currentTask.start_latent_task(mind, nextTask);
			}
		}
		{
			self->isAttacking = currentTask.is_running(attack_enemy);
		}

		LATENT_WAIT(updateTime.get(Random::get_float(0.1f, 0.3f)));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	if (auto * ms = imo->get_sound())
	{
		ms->stop_sound(NAME(engine));
	}
	self->isAttacking = false;

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Welder::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, shieldTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	
	LATENT_BEGIN_CODE();

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}
	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(manage_shield));
		shieldTask.start_latent_task(mind, taskInfo);
	}

	messageHandler.use_with(mind);
	{
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	// yeah, behave is actual main state
	while (true)
	{
		{
			// choose best action for now
			::Framework::AI::LatentTaskInfoWithParams nextTask;

			nextTask.propose(AI_LATENT_TASK_FUNCTION(behave));

			if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
			{
				// doing scripted behaviour
			}
			else
			{
				// check if we can start new one, if so, start it
				if (currentTask.can_start(nextTask))
				{
					currentTask.start_latent_task(mind, nextTask);
				}
			}
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
