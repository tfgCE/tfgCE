#include "aiLogic_creeper.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_scanAndWander.h"
#include "tasks\aiLogicTask_shoot.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
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
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
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

// ai message params
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(who);

// sounds / temporary objects
DEFINE_STATIC_NAME_STR(postShoot, TXT("post shoot"));
DEFINE_STATIC_NAME(attack);

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// emissive layers
//DEFINE_STATIC_NAME(attack);

// tasks
DEFINE_STATIC_NAME(attackSequence);
DEFINE_STATIC_NAME(idleSequence);

//

static float aggressiveness_coef(float _perOne = 0.3f)
{
	return max(1.0f, 1.0f + _perOne * (GameSettings::get().ai_agrressiveness_as_float() - 1.0f));
}

//

REGISTER_FOR_FAST_CAST(Creeper);

Creeper::Creeper(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Creeper::~Creeper()
{
}

static LATENT_FUNCTION(attack_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	LATENT_VAR(float, timeToShoot);

	LATENT_VAR(int, flankStrategy);
	LATENT_VAR(bool, updateFlankStrategy);
	LATENT_VAR(float, minDist);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(int, lookAtBlocked);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, attackActionTask);

	LATENT_VAR(bool, startShooting);
	LATENT_VAR(Optional<float>, timeToGiveUp);

	LATENT_VAR(bool, postShootRequired);

	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
	an_assert(npcBase);

	timeToShoot = max(0.0f, timeToShoot - LATENT_DELTA_TIME);

	lookAtBlocked = timeToShoot > 1.0f && !startShooting;

	if (timeToGiveUp.is_set())
	{
		timeToGiveUp = max(0.0f, timeToGiveUp.get() - LATENT_DELTA_TIME);
	}

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("attack enemy (%S)"), enemy.get() ? enemy->ai_get_name().to_char() : TXT("??"));
	ai_log_no_colour(mind->get_logic());

	postShootRequired = false;

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

	updateFlankStrategy = true;
	minDist = 3.0f;

	if (auto* s = imo->get_sound())
	{
		if (s->play_sound(NAME(attack)))
		{
			if (Framework::GameUtils::is_controlled_by_player(enemy.get()))
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
				LATENT_BREAK();
			}
		}

		if (startShooting)
		{
			goto SHOOT;
		}

		if (! pathTask.is_set() && enemyPlacement.is_active())
		{
			auto& locomotion = mind->access_locomotion();
			if (locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
			{
				if (enemyPlacement.get_target())
				{
					// sees enemy
					LATENT_LOG(TXT("I see enemy"));
				}
				else
				{
					// follow enemy
					LATENT_LOG(TXT("not seeing enemy"));
				}

				Framework::Nav::PlacementAtNode ourNavLoc = mind->access_navigation().find_nav_location();
				Framework::Nav::PlacementAtNode toEnemyNavLoc = Framework::Nav::PlacementAtNode::invalid();

				if (enemyPlacement.get_through_doors().is_empty())
				{
					// same room
					LATENT_LOG(TXT("same room - check actual location"));
					toEnemyNavLoc = mind->access_navigation().find_nav_location(enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_final_room());
				}
				else
				{
					// get door through which enemy is available
					if (auto* dir = enemyPlacement.get_through_doors().get_first().get())
					{
						if (auto* nav = dir->get_nav_door_node().get())
						{
							LATENT_LOG(TXT("different room, consider first door from the chain/path"));
							toEnemyNavLoc.set(nav);
						}
						else
						{
							LATENT_LOG(TXT("different room, get close to first door although can't get through"));
							Transform dirPlacement = dir->get_placement();
							Transform inFront(Vector3::yAxis * -0.3f, Quat::identity);
							toEnemyNavLoc = mind->access_navigation().find_nav_location(dir->get_in_room(), dirPlacement.to_world(inFront));
						}
					}
				}

				if (ourNavLoc.is_valid() && toEnemyNavLoc.is_valid())
				{
					LATENT_LOG(TXT("we know where enemy is on navmesh"));
					Framework::Room* goToRoom = imo->get_presence()->get_in_room();
					Framework::DoorInRoom* goThroughDoor = nullptr;
					Vector3 goToLoc = toEnemyNavLoc.get_current_placement().get_translation();
					Vector3 upDir = imo->get_presence()->get_environment_up_dir();
					Vector3 startLoc = imo->get_presence()->get_placement().get_translation();

					bool tryToApproach = false;

					// check if door-through or enemy belong to the same nav group that we are and decide whether to follow to enemy or stay where we are
					if (ourNavLoc.node.get()->get_group() == toEnemyNavLoc.node.get()->get_group())
					{
						LATENT_LOG(TXT("same nav group"));
						// same group
						if (enemyPlacement.get_through_doors().is_empty())
						{
							an_assert(imo->get_appearance());
							if (enemyPlacement.get_target() ||
								!check_clear_ray_cast(CastInfo(), imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index())).get_translation(), imo,
									enemy.get(),
									enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_owner_room()))
							{
								// is within same room we see enemy or we do not see enemy's location
								LATENT_LOG(enemyPlacement.get_target()? TXT("approach enemy!") : TXT("approach to see if enemy is there"));
								tryToApproach = true;
								goToRoom = enemyPlacement.get_in_final_room();
								goToLoc = enemyPlacement.get_placement_in_final_room().get_translation();
							}
							else
							{
								LATENT_LOG(TXT("get closer to enemy"));
								// keep "goTo"s
								if ((goToLoc - startLoc).length() < 1.5f)
								{
									LATENT_LOG(TXT("really close and no enemy in sight"));
									thisTaskHandle->access_result().access(NAME(taskFeedback)).access_as<Name>() = NAME(enemyLost);
									LATENT_BREAK();
								}
							}
						}
						else
						{
							if (enemyPlacement.get_target())
							{
								// we see enemy, approach him wherever he is
								LATENT_LOG(TXT("we see enemy, go to the enemy's room"));
								tryToApproach = true;
								goToRoom = enemyPlacement.get_in_final_room();
								goToLoc = enemyPlacement.get_placement_in_final_room().get_translation();
								startLoc = enemyPlacement.location_from_owner_to_target(startLoc);
							}
							else
							{
								LATENT_LOG(TXT("we do not see enemy, go to the door, on the other side"));
								// we don't see enemy and enemy is in different room, just go to the door and see what will happen
								goThroughDoor = enemyPlacement.get_through_doors().get_first().get();
							}
						}
					}
					else
					{
						LATENT_LOG(TXT("different nav group, try to approach if possible"));
						// remain here, within this group, try to approach enemy
						tryToApproach = true;
						// keep "goTo"s
						todo_note(TXT("check chances if maybe we should try to find a way to the enemy, break this function then and request following enemy"));
					}

					Framework::Nav::PathRequestInfo pathRequestInfo(imo);
					pathRequestInfo.with_dev_info(TXT("creeper ?"));
					if (!Mobility::may_leave_room_when_attacking(imo, imo->get_presence()->get_in_room()))
					{
						pathRequestInfo.within_same_room_and_navigation_group();
					}

					if (goThroughDoor)
					{
						pathTask = mind->access_navigation().find_path_through(goThroughDoor, 0.5f, pathRequestInfo);
					}
					else
					{
						if (tryToApproach)
						{
							Vector3 enemyLoc = goToLoc;
							Vector3 startToEnemy = enemyLoc - startLoc;
							Vector3 right = Vector3::cross(startToEnemy, upDir).normal();
							goToLoc = startLoc + startToEnemy * 0.5f + right * startToEnemy.length() * 0.5f * (float)(flankStrategy);
							Vector3 enemyToGoTo = goToLoc - enemyLoc;
							goToLoc = enemyLoc + enemyToGoTo.normal() * max(minDist, enemyToGoTo.length());
						}
						pathTask = mind->access_navigation().find_path_to(goToRoom, goToLoc, pathRequestInfo);
					}
				}
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
						locomotion.turn_follow_path_2d();
					}
				}
				else
				{
					locomotion.dont_move();
					timeToShoot = 0.0f;
					startShooting = true;
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

			LATENT_WAIT(1.0f / aggressiveness_coef(0.75f));

			{
				::Framework::AI::LatentTaskInfoWithParams taskInfo;
				taskInfo.clear();
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::shoot));
				ADD_LATENT_TASK_INFO_PARAM(taskInfo, int, 3);
				ADD_LATENT_TASK_INFO_PARAM(taskInfo, float, 0.1f);
				attackActionTask.start_latent_task(mind, taskInfo);
			}
			while (attackActionTask.is_running())
			{
				LATENT_YIELD();
			}

			postShootRequired = true;
			LATENT_WAIT(1.0f / aggressiveness_coef(1.25f));
			if (auto* tos = imo->get_temporary_objects())
			{
				postShootRequired = false;
				tos->spawn_all(NAME(postShoot));
			}
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
	if (postShootRequired)
	{
		if (auto* tos = imo->get_temporary_objects())
		{
			postShootRequired = false;
			tos->spawn_all(NAME(postShoot));
		}
	}

	LATENT_ON_END();
	//
	perceptionEnemyLock.override_lock(NAME(attack));
	if (mind)
	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	lookAtTask.stop();
	attackActionTask.stop();
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

LATENT_FUNCTION(Creeper::execute_logic)
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
				//::Framework::AI::LatentTaskInfoWithParams nextTask;
				//nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::attack_or_idle));
				//currentTask.start_latent_task(mind, nextTask);
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
