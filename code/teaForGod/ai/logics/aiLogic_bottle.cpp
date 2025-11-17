#include "aiLogic_bottle.h"

#include "actions\aiAction_findPath.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_deathSequenceSimple.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_runAway.h"
#include "tasks\aiLogicTask_wander.h"
#include "tasks\aiLogicTask_shoot.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\tasks\navTask_PathTask.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomUtils.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME(die);

// ai message params
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(who);

// sounds / temporary objects
DEFINE_STATIC_NAME(attack);

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// emissive layers
//DEFINE_STATIC_NAME(attack);

// tasks
DEFINE_STATIC_NAME(attackSequence);
DEFINE_STATIC_NAME(idleSequence);

// temporary objects
DEFINE_STATIC_NAME(explode);

// variables
DEFINE_STATIC_NAME(shootCount);
DEFINE_STATIC_NAME(shootInterval);
DEFINE_STATIC_NAME(arm_rf_off);
DEFINE_STATIC_NAME(arm_lf_off);
DEFINE_STATIC_NAME(arm_rb_off);
DEFINE_STATIC_NAME(arm_lb_off);

// movement gaits
DEFINE_STATIC_NAME(run);

//


//

REGISTER_FOR_FAST_CAST(Bottle);

Bottle::Bottle(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	movementGaitTimeBased.add_gait(Name::invalid(), Range(5.0f, 20.0f));
	movementGaitTimeBased.add_gait(NAME(run), Range(5.0f, 10.0f));
}

Bottle::~Bottle()
{
}

void Bottle::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	shootCount = _parameters.get_value<int>(NAME(shootCount), shootCount);
	shootInterval = _parameters.get_value<float>(NAME(shootInterval), shootInterval);
}

void Bottle::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	movementGaitTimeBased.advance(_deltaTime);
}

LATENT_FUNCTION(Bottle::attack_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, unableToAttack);
	LATENT_COMMON_VAR(bool, perceptionSightImpaired);
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	LATENT_VAR(float, timeToShoot);

	LATENT_VAR(int, flankStrategy);
	LATENT_VAR(bool, updateFlankStrategy);
	LATENT_VAR(float, minDist);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, attackActionTask);

	LATENT_VAR(bool, startShooting);
	LATENT_VAR(Optional<float>, timeToGiveUp);

	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
	auto* self = fast_cast<Bottle>(mind->get_logic());
	an_assert(npcBase);

	timeToShoot = max(0.0f, timeToShoot - LATENT_DELTA_TIME);

	if (timeToGiveUp.is_set())
	{
		timeToGiveUp = max(0.0f, timeToGiveUp.get() - LATENT_DELTA_TIME);
	}

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("attack enemy (%S)"), enemy.get() ? enemy->ai_get_name().to_char() : TXT("??"));
	ai_log_no_colour(mind->get_logic());

	self->set_movement_gait();

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_activate(NAME(attack));
	}

	updateFlankStrategy = true;
	minDist = 3.0f;

	if (auto* s = imo->get_sound())
	{
		if (s->play_sound(NAME(attack)))
		{
			if (Framework::GameUtils::is_controlled_by_player(enemy.get()) &&
				enemyPlacement.get_target())
			{
				if (!npcBase || npcBase->is_ok_to_play_combat_music(enemyPlacement))
				{
					MusicPlayer::request_combat_auto_indicate_presence();
				}
			}
		}
	}

	timeToShoot = -1.0f;
	startShooting = rg.get_chance(0.8f) || enemyPlacement.calculate_string_pulled_distance() < minDist;
	while (true)
	{
		LATENT_CLEAR_LOG();
		if (timeToShoot <= 0.0f)
		{
			timeToShoot = rg.get_float(4.0f, 8.0f);
			if (enemyPlacement.calculate_string_pulled_distance(imo->get_presence()->get_centre_of_presence_os(), Transform(enemyTargetingOffsetOS, Quat::identity)) < minDist)
			{
				LATENT_LOG(TXT("close, shoot more often"));
				// more often
				timeToShoot = rg.get_float(1.0f, 3.5f);
			}
			timeToShoot *= (0.5f + 0.5f * GameSettings::get().difficulty.aiReactionTime);
			if (rg.get_chance(0.2f))
			{
				updateFlankStrategy = true;
			}
			startShooting = rg.get_chance(0.8f) || enemyPlacement.calculate_string_pulled_distance() < minDist;
		}

		if (updateFlankStrategy)
		{
			updateFlankStrategy = false;
			flankStrategy = rg.get_chance(0.5f) ? (rg.get_chance(0.5f) ? 1 : -1) : 0;
		}

		if (enemyPlacement.get_target())
		{
			timeToGiveUp.clear();
		}
		else
		{
			if (!timeToGiveUp.is_set())
			{
				timeToGiveUp = rg.get_float(10.0f, 30.0f);
			}
			if (timeToGiveUp.get() <= 0.0f)
			{
				ai_log(mind->get_logic(), TXT("didn't see target, gave up"));
				LATENT_BREAK();
			}
		}

		if (startShooting)
		{
			goto SHOOT;
		}

		if (! pathTask.is_set() && enemyPlacement.is_active())
		{
			bool continueTask = Actions::find_path(_frame, mind, pathTask, enemy, enemyPlacement, perceptionSightImpaired,
				[imo, flankStrategy, minDist](REF_ Vector3& goToLoc)
			{
				Vector3 startLoc = imo->get_presence()->get_placement().get_translation();
				Vector3 upDir = imo->get_presence()->get_environment_up_dir();

				Vector3 enemyLoc = goToLoc;
				Vector3 startToEnemy = enemyLoc - startLoc;
				Vector3 right = Vector3::cross(startToEnemy, upDir).normal();
				goToLoc = startLoc + startToEnemy * 0.5f + right * startToEnemy.length() * 0.5f * (float)(flankStrategy);
				Vector3 enemyToGoTo = goToLoc - enemyLoc;
				goToLoc = enemyLoc + enemyToGoTo.normal() * max(minDist, enemyToGoTo.length());
			});

			if (!continueTask)
			{
				ai_log(mind->get_logic(), TXT("really close and no enemy in sight (can't get to enemy)"));
				thisTaskHandle->access_result().access(NAME(taskFeedback)).access_as<Name>() = NAME(enemyLost);
				LATENT_BREAK();
			}
		}

		if (pathTask.is_set())
		{
			LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
			{
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					path = task->get_path();
				}
				else
				{
					path.clear();
				}
			}
			else
			{
				path.clear();
			}
			pathTask = nullptr;
		}

		LATENT_LOG(TXT("move for %.1fs"), timeToShoot);
	MOVE:
		if (auto* s = imo->get_sound())
		{
			s->play_sound(NAME(attack));
		}

		{
			{
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					locomotion.dont_move();
					timeToShoot = 0.0f;
					startShooting = true;
				}
				else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
				{
					if (!locomotion.is_following_path(path.get()))
					{
						locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
					}
				}
				else
				{
					locomotion.dont_move();
					locomotion.turn_towards_2d(enemyPlacement, 5.0f);
					timeToShoot = 0.0f;
					startShooting = true;
				}

				//

				if (locomotion.is_following_path())
				{
					if (enemyPlacement.get_target())
					{
						locomotion.turn_towards_2d(enemyPlacement, 5.0f);
					}
					else
					{
						locomotion.turn_follow_path_2d();
					}
				}
			}

			if (timeToShoot <= 0.0f || startShooting)
			{
				goto SHOOT;
			}

			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.has_finished_move(0.5f))
				{
					locomotion.dont_move();
					locomotion.turn_towards_2d(enemyPlacement, 5.0f);
					goto MOVED;
				}
			}
			LATENT_WAIT(rg.get_float(0.1f, 0.3f));
		}
		goto MOVE;

	SHOOT:
		perceptionEnemyLock.override_lock(NAME(attack), AI::PerceptionLock::Lock);
		todo_note(TXT("hardcoded socket assumed always present"));
		an_assert(imo->get_appearance());
		if (npcBase &&
			enemyPlacement.get_target() &&
			!unableToAttack &&
			(!GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed) &&
			check_clear_ray_cast(CastInfo(), imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index())).get_translation(), imo,
						   enemyPlacement.get_target(),
						   enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_owner_room()))
		{
			LATENT_LOG(TXT("shoot"));
			{
				auto& locomotion = mind->access_locomotion();
				locomotion.stop();
				locomotion.turn_towards_2d(enemyPlacement.get_placement_in_owner_room().get_translation(), 5.0f);
			}

			LATENT_WAIT(1.0f * GameSettings::get().difficulty.aiReactionTime);

			{
				::Framework::AI::LatentTaskInfoWithParams taskInfo;
				taskInfo.clear();
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::shoot));
				ADD_LATENT_TASK_INFO_PARAM(taskInfo, int, self->shootCount);
				ADD_LATENT_TASK_INFO_PARAM(taskInfo, float, self->shootInterval); // interval
				attackActionTask.start_latent_task(mind, taskInfo);
			}
			while (attackActionTask.is_running() && !unableToAttack)
			{
				LATENT_YIELD();
			}
			attackActionTask.stop();

			LATENT_WAIT(0.8f * GameSettings::get().difficulty.aiReactionTime);
		}
		else
		{
			timeToShoot = 0.5f; // try again very soon
		}
		startShooting = false;
		perceptionEnemyLock.override_lock(NAME(attack));

	MOVED:
		{
			LATENT_YIELD();
		}
	}
	// ...

	LATENT_WAIT(rg.get_float(0.1f, 0.2f));

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	perceptionEnemyLock.override_lock(NAME(attack));
	if (mind)
	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	attackActionTask.stop();
	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_deactivate(NAME(attack));
		}
	}
	//
	self->set_movement_gait(NAME(run));

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Bottle::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, announcePresenceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(bool, dieNow);
	LATENT_VAR(bool, runAwayNow);

	LATENT_VAR(Random::Generator, rg);

	aggressiveness = 99.0f;

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Bottle>(mind->get_logic());

	LATENT_BEGIN_CODE();

	messageHandler.use_with(mind);
	{
		auto* framePtr = &_frame;
		messageHandler.set(NAME(die), [framePtr, mind, &dieNow](Framework::AI::Message const& _message)
			{
				dieNow = true;
				framePtr->end_waiting();
				AI_LATENT_STOP_LONG_RARE_ADVANCE();
			}
		);
	}

	self->set_movement_gait(NAME(run));

	if (ShouldTask::announce_presence(mind->get_owner_as_modules_owner()))
	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::announce_presence));
		announcePresenceTask.start_latent_task(mind, taskInfo);
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	if (auto* logicLatent = fast_cast<::Framework::AI::LogicWithLatentTask>(logic))
	{
		logicLatent->register_latent_task(NAME(attackSequence), attack_enemy);
		logicLatent->register_latent_task(NAME(idleSequence), Tasks::wander);
	}

	while (true)
	{
		// check if we should die
		{
			{
				int legsOff = 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rb_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lb_off), 0) ? 1 : 0;
				if (legsOff >= 1)
				{
					runAwayNow = true;
				}
				if (legsOff >= 2)
				{
					dieNow = true;
				}
			}
		}
		{
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			bool specialTask = false;
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::attack_or_idle));
			}
			if (dieNow)
			{
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::death_sequence_simple), 100, NP, false, true))
				{
					ADD_LATENT_TASK_INFO_PARAM(nextTask, SafePtr<::Framework::IModulesOwner>, self->runAwayFrom);
					specialTask = true;
				}
			}
			if (runAwayNow)
			{
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::run_away), 10, NP, NP, true))
				{
					if (!currentTask.is_running(AI_LATENT_TASK_FUNCTION(Tasks::run_away)))
					{
						self->set_movement_gait();
					}
					if (!self->runAwayFrom.is_set())
					{
						if (auto* h = imo->get_custom<CustomModules::Health>())
						{
							self->runAwayFrom = h->get_last_damage_instigator();
						}
					}
					ADD_LATENT_TASK_INFO_PARAM(nextTask, SafePtr<::Framework::IModulesOwner>, self->runAwayFrom);
					ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // keep running away
				}
			}
			if (!specialTask &&
				Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
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
		self->movementGaitTimeBased.set_active(!currentTask.is_running(Tasks::attack_or_idle) || !enemyPlacement.get_target());

		LATENT_WAIT(rg.get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	announcePresenceTask.stop();
	perceptionTask.stop();
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
