#include "aiLogic_spitter.h"

#include "actions\aiAction_findPath.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_deathSequenceSimple.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_runAway.h"
#include "tasks\aiLogicTask_wander.h"
#include "tasks\aiLogicTask_scanAndWander.h"
#include "tasks\aiLogicTask_shoot.h"
#include "tasks\aiLogicTask_shootContinuously.h"

#include "utils\aiLogicUtil_shootingAccuracy.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
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

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

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

// variables
DEFINE_STATIC_NAME(headless);
DEFINE_STATIC_NAME(shootCount);
DEFINE_STATIC_NAME(shootInterval);
DEFINE_STATIC_NAME(shootSetInterval);
DEFINE_STATIC_NAME(arm_rf_off);
DEFINE_STATIC_NAME(arm_lf_off);
DEFINE_STATIC_NAME(arm_b_off);

// sockets
DEFINE_STATIC_NAME(headshot);

// temporary objects
DEFINE_STATIC_NAME(shootLeft);
DEFINE_STATIC_NAME(shootRight);
DEFINE_STATIC_NAME(shootBelly);
DEFINE_STATIC_NAME(muzzleFlashLeft);
DEFINE_STATIC_NAME(muzzleFlashRight);
DEFINE_STATIC_NAME(muzzleBellyFlash);

// movement gaits
DEFINE_STATIC_NAME(run);

//

REGISTER_FOR_FAST_CAST(Spitter);

Spitter::Spitter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	auto& si = access_shot_infos();
	si.clear();
	si.push_back(NPCBase::ShotInfo(NAME(shootLeft), NAME(muzzleFlashLeft)));
	si.push_back(NPCBase::ShotInfo(NAME(shootRight), NAME(muzzleFlashRight)));

	bellyShotInfos.clear();
	bellyShotInfos.push_back(NPCBase::ShotInfo(NAME(shootBelly), NAME(muzzleBellyFlash)));

	movementGaitTimeBased.add_gait(Name::invalid(), Range(2.0f, 10.0f));
	movementGaitTimeBased.add_gait(NAME(run), Range(2.0f, 10.0f));
}

Spitter::~Spitter()
{
}

void Spitter::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	shootCount = _parameters.get_value<int>(NAME(shootCount), shootCount);
	shootInterval = _parameters.get_value<float>(NAME(shootInterval), shootInterval);
	shootSetInterval = _parameters.get_value<float>(NAME(shootSetInterval), shootSetInterval);
}

void Spitter::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	movementGaitTimeBased.advance(_deltaTime);
}

LATENT_FUNCTION(Spitter::shoot)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("shoot"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, shootBellyTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, shootHeadTask);

	LATENT_VAR(SimpleVariableInfo, headlessVar);
		
	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Spitter>(mind->get_logic());

	LATENT_BEGIN_CODE();

	headlessVar = imo->access_variables().find<bool>(NAME(headless));

	while (true)
	{
		if (auto* s = imo->get_sound())
		{
			if (s->play_sound(NAME(attack)))
			{
				if (Framework::GameUtils::is_controlled_by_player(enemy.get()))
				{
					if (enemyPlacement.get_target() && enemyPlacement.get_straight_length() < 10.0f)
					{
						MusicPlayer::request_combat_auto();
					}
					else
					{
						MusicPlayer::request_combat_auto_indicate_presence();
					}
				}
			}
		}
		// try to shoot now
		{
			::Framework::AI::LatentTaskInfoWithParams shootTaskInfo;
			shootTaskInfo.clear();
			shootTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::shoot));
			ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, int, 1);
			ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, float, 0.2f); // interval
			ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(shootTaskInfo); // speed
			ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, Array<NPCBase::ShotInfo> const*, &self->bellyShotInfos); // shot infos
			ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, bool, true); // ignore "unableToShoot"
			shootBellyTask.start_latent_task(mind, shootTaskInfo);
		}

		// start continuous shooting
		if (!headlessVar.get<bool>())
		{
			LATENT_WAIT(1.0f);

			{
				::Framework::AI::LatentTaskInfoWithParams shootTaskInfo;
				shootTaskInfo.clear();
				shootTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::shoot_continuously));
				ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, int, self->shootCount);
				ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, float, self->shootInterval); // interval
				ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, float, self->shootSetInterval); // very long, we will be manually interrupting
				ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(shootTaskInfo); // speed
				ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(shootTaskInfo); // shot infos
				ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, bool, true); // use secondary perception
				shootHeadTask.start_latent_task(mind, shootTaskInfo);
			}

			LATENT_WAIT(5.0f);
		}
		else
		{
			LATENT_WAIT(2.0f);
		}

		// end (if still running) and restart

		shootBellyTask.stop();
		shootHeadTask.stop();

		LATENT_YIELD();
	}
	// ...

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	shootBellyTask.stop();
	shootHeadTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Spitter::attack_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, perceptionSightImpaired);
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	LATENT_VAR(float, timeToChangeStrategy);

	LATENT_VAR(int, flankStrategy);
	LATENT_VAR(bool, updateFlankStrategy);
	LATENT_VAR(float, minDist);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(int, lookAtBlocked);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, shootTask);

	LATENT_VAR(bool, startShooting);
	LATENT_VAR(Optional<float>, timeToGiveUp);

	LATENT_VAR(Random::Generator, rg);

	LATENT_VAR(SimpleVariableInfo, headlessVar);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
	auto* self = fast_cast<Spitter>(mind->get_logic());
	an_assert(npcBase);

	timeToChangeStrategy = max(0.0f, timeToChangeStrategy - LATENT_DELTA_TIME);

	lookAtBlocked = false;

	if (timeToGiveUp.is_set())
	{
		timeToGiveUp = max(0.0f, timeToGiveUp.get() - LATENT_DELTA_TIME);
	}

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("attack enemy (%S)"), enemy.get() ? enemy->ai_get_name().to_char() : TXT("??"));
	ai_log_no_colour(mind->get_logic());

	self->set_movement_gait();

	headlessVar = imo->access_variables().find<bool>(NAME(headless));

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_activate(NAME(attack));
	}

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &lookAtBlocked);
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &enemyPlacement);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, Vector3*, &enemyTargetingOffsetOS);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	{
		::Framework::AI::LatentTaskInfoWithParams shootTaskInfo;
		shootTaskInfo.clear();
		shootTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Spitter::shoot));
		shootTask.start_latent_task(mind, shootTaskInfo);
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

	timeToChangeStrategy = -1.0f;
	while (true)
	{
		LATENT_CLEAR_LOG();
		if (timeToChangeStrategy <= 0.0f)
		{
			timeToChangeStrategy = rg.get_float(8.0f, 12.0f);
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

		if (! pathTask.is_set() && enemyPlacement.is_active())
		{
			bool continueTask = Actions::find_path(_frame, mind, pathTask, enemy, enemyPlacement, perceptionSightImpaired,
				[imo, flankStrategy, minDist](REF_ Vector3& goToLoc)
			{
				Vector3 startLoc = imo->get_presence()->get_placement().get_translation();
				Vector3 upDir = imo->get_presence()->get_environment_up_dir();

				Vector3 enemyLoc = goToLoc;
				startLoc = enemyLoc + (startLoc - enemyLoc).drop_using(upDir); // try to be at the same level

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

		LATENT_LOG(TXT("move for %.1fs"), timeToChangeStrategy);
	MOVE:
		
		{
			{
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					locomotion.dont_move();
					timeToChangeStrategy = 0.0f;
					// startShooting = true;
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
					timeToChangeStrategy = 0.0f;
					// startShooting = true;
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


			if (timeToChangeStrategy <= 0.0f)
			{
				goto MOVED;
			}

			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.has_finished_move(0.5f))
				{
					locomotion.dont_move();
					goto MOVED;
				}
			}
			LATENT_WAIT(rg.get_float(0.1f, 0.3f));
		}
		goto MOVE;

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
	if (mind)
	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	lookAtTask.stop();
	shootTask.stop();
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

LATENT_FUNCTION(Spitter::wander_scanning)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, wanderTask);

	LATENT_VAR(Random::Generator, rg);
	LATENT_VAR(float, scanYaw);
	LATENT_VAR(Vector3, locationMS);

	LATENT_VAR(float, scanningSpeed);
	LATENT_VAR(float, scanDir);

	LATENT_BEGIN_CODE();

	locationMS = Vector3(0.0f, 0.0f, 1000.0f);

	{
		auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
		an_assert(npcBase);
		scanningSpeed = npcBase->get_scanning_speed();
	}

	scanDir = rg.get_bool() ? 1.0f : -1.0f;

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
		wanderTask.start_latent_task(mind, taskInfo);
	}

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo); // blocked param, we don't block now
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo);
		ADD_LATENT_TASK_INFO_PARAM_OPTIONAL_NOT_PROVIDED(lookAtTaskInfo); 
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, Vector3*, &locationMS);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	while (true)
	{
		{
			scanYaw += scanDir * scanningSpeed * LATENT_DELTA_TIME;
			scanYaw = Rotator3::normalise_axis(scanYaw);
			locationMS = Rotator3(0.0f, scanYaw, 0.0f).get_forward() * 1000.0f;
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	wanderTask.stop();
	lookAtTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Spitter::execute_logic)
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

	LATENT_VAR(::Framework::AI::LatentTaskHandle, announcePresenceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(bool, dieNow);
	LATENT_VAR(bool, runAwayNow);

	LATENT_VAR(Random::Generator, rg);

	aggressiveness = 99.0f;

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Spitter>(mind->get_logic());

	LATENT_BEGIN_CODE();

	self->use_secondary_perception_for_scanning();

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
		logicLatent->register_latent_task(NAME(idleSequence), wander_scanning);
	}

	self->set_movement_gait(NAME(run));

	while (true)
	{
		// check if we should die
		{
			{
				int legsOff = 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_b_off), 0) ? 1 : 0;
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

		LATENT_WAIT(rg.get_float(0.02f, 0.2f));
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
