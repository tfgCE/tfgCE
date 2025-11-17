#include "aiLogic_fender.h"

#include "actions\aiAction_findPath.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_lightningRandomDischarge.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_perception.h"
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
#include "..\..\..\framework\ai\aiMessageHandler.h"
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

// ai messages
DEFINE_STATIC_NAME(remoteControl);

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
DEFINE_STATIC_NAME(shootCount);
DEFINE_STATIC_NAME(shootInterval);
DEFINE_STATIC_NAME(shootSetInterval);

// temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME(muzzleFlash);

//

REGISTER_FOR_FAST_CAST(Fender);

Fender::Fender(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	shield.set_owner(_mind->get_owner_as_modules_owner());
}

Fender::~Fender()
{
}

void Fender::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	shootCount = _parameters.get_value<int>(NAME(shootCount), shootCount);
	shootInterval = _parameters.get_value<float>(NAME(shootInterval), shootInterval);
	shootSetInterval = _parameters.get_value<float>(NAME(shootSetInterval), shootSetInterval);

	auto_fill_shot_infos();

	shield.learn_from(_parameters);
}

LATENT_FUNCTION(Fender::manage_shield)
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

	auto* self = fast_cast<Fender>(mind->get_logic());

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

LATENT_FUNCTION(Fender::attack_enemy)
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

	LATENT_VAR(Optional<float>, timeToGiveUp);

	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
	auto* self = fast_cast<Fender>(mind->get_logic());
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
		shootTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::shoot_continuously));
		ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, int, self->shootCount);
		ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, float, self->shootInterval); // interval
		ADD_LATENT_TASK_INFO_PARAM(shootTaskInfo, float, self->shootSetInterval); // set interval
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
		
		{
			{
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					locomotion.dont_move();
					timeToChangeStrategy = 0.0f;
				}
				else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
				{
					if (!locomotion.is_following_path(path.get()))
					{
						locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
						locomotion.turn_follow_path_2d();
					}
				}
				else
				{
					locomotion.dont_move();
					timeToChangeStrategy = 0.0f;
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

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Fender::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, announcePresenceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, shieldTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	aggressiveness = 99.0f;

	LATENT_BEGIN_CODE();

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

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(manage_shield));
		shieldTask.start_latent_task(mind, taskInfo);
	}

	if (auto* logicLatent = fast_cast<::Framework::AI::LogicWithLatentTask>(logic))
	{
		logicLatent->register_latent_task(NAME(attackSequence), attack_enemy);
		logicLatent->register_latent_task(NAME(idleSequence), Tasks::scan_and_wander);
	}

	while (true)
	{
		if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
		{
			// doing scripted behaviour
		}
		else
		{
			if (!currentTask.is_running())
			{
				::Framework::AI::LatentTaskInfoWithParams nextTask;
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::attack_or_idle));
				currentTask.start_latent_task(mind, nextTask);
			}
		}
		LATENT_YIELD();
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
