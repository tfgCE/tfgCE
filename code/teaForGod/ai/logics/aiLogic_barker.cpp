#include "aiLogic_barker.h"

#include "actions\aiAction_lightningStrike.h"

#include "tasks\aiLogicTask_announcePresence.h"
#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_beingHeld.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_stayCloseInFront.h"
#include "tasks\aiLogicTask_stayCloseOrWander.h"
#include "tasks\aiLogicTask_wander.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\damage.h"
#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\custom\mc_switchSidesHandler.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\game\gameConfig.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\tasks\navTask_GetRandomLocation.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\framework\world\presenceLink.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\modules\custom\mc_emissiveControl.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

//#define DEBUG_DRAW_THROWING

//

// ai messages
DEFINE_STATIC_NAME(attract);

// ai message params
DEFINE_STATIC_NAME(who);

// socket
DEFINE_STATIC_NAME(projectile);

// lightnings
DEFINE_STATIC_NAME(idle);
DEFINE_STATIC_NAME(charging);
DEFINE_STATIC_NAME(discharge);

// sounds
DEFINE_STATIC_NAME(attack);

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// variables
DEFINE_STATIC_NAME(dischargeDamage);

// gaits
DEFINE_STATIC_NAME(run);

// cooldowns
DEFINE_STATIC_NAME(runAway);
DEFINE_STATIC_NAME(playAttackSound);

// tasks
DEFINE_STATIC_NAME(attackSequence);
DEFINE_STATIC_NAME(idleSequence);
DEFINE_STATIC_NAME(stayCloseSequence);
DEFINE_STATIC_NAME(wanderSequence);

//

REGISTER_FOR_FAST_CAST(Barker);

Barker::Barker(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	data = fast_cast<BarkerData>(_logicData);
}

Barker::~Barker()
{
}

LATENT_FUNCTION(Barker::attack_enemy)
{
	scoped_call_stack_info(TXT("barker::attack_enemy"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(int, flankStrategy);
	LATENT_VAR(bool, updateFlankStrategy);
	LATENT_VAR(float, attackDist);

	LATENT_VAR(bool, attackSuccessful);
	
	LATENT_VAR(bool, attackNow);
	LATENT_VAR(bool, updatePath);
	LATENT_VAR(float, timeToUpdatePath);
	LATENT_VAR(Optional<float>, timeToGiveUp);

	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);
	LATENT_VAR(CustomModules::SwitchSidesHandler*, switchSidesHandler);

	LATENT_VAR(float, attackTimeMultiplier);

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);

	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Barker>(mind->get_logic());
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());

	an_assert(npcBase);

	if (timeToGiveUp.is_set())
	{
		timeToGiveUp = max(0.0f, timeToGiveUp.get() - LATENT_DELTA_TIME);
	}

	timeToUpdatePath -= LATENT_DELTA_TIME;

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("attack enemy (%S)"), enemy.get()? enemy->ai_get_name().to_char() : TXT("??"));
	ai_log_no_colour(mind->get_logic());

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();
	switchSidesHandler = imo->get_custom<CustomModules::SwitchSidesHandler>();

	if (!lightningSpawner)
	{
		LATENT_BREAK();
	}

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_activate(NAME(attack));
	}

	updateFlankStrategy = true;
	attackDist = 1.5f;

	LATENT_CLEAR_LOG();

	while (true)
	{
		ai_log(mind->get_logic(), TXT("start attack loop"));
		if (cooldowns.is_available(NAME(playAttackSound)))
		{
			scoped_call_stack_info(TXT("play sound"));
			if (auto* s = imo->get_sound())
			{
				s->play_sound(NAME(attack));
				if (Framework::GameUtils::is_controlled_by_player(enemy.get()) &&
					enemyPlacement.get_target())
				{
					if (!npcBase || npcBase->is_ok_to_play_combat_music(enemyPlacement))
					{
						MusicPlayer::request_combat_auto_indicate_presence();
					}
				}
			}
			cooldowns.set(NAME(playAttackSound), rg.get_float(1.0f, 3.0f));
		}

		LATENT_LOG(TXT("start again"));
		attackSuccessful = false;

		if (rg.get_chance(0.6f))
		{
			updateFlankStrategy = true;
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

	UPDATE_PATH:
		ai_log(mind->get_logic(), TXT("update path"));
		if (enemyPlacement.is_active())
		{
			scoped_call_stack_info(TXT("update path"));
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
								LATENT_LOG(enemyPlacement.get_target() ? TXT("approach enemy!") : TXT("approach to see if enemy is there"));
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
					pathRequestInfo.with_dev_info(TXT("barker approach"));
					//if (!Mobility::may_leave_room_when_attacking(imo, imo->get_presence()->get_in_room()))
					//{
					//	pathRequestInfo.within_same_room_and_navigation_group();
					//}

					ai_log(mind->get_logic(), TXT("find a new path"));

					if (goThroughDoor)
					{
						pathTask = mind->access_navigation().find_path_through(goThroughDoor, 0.5f, pathRequestInfo);
					}
					else
					{
						if (tryToApproach)
						{
							Vector3 enemyLoc = goToLoc;

							startLoc = enemyLoc + (startLoc - enemyLoc).drop_using(upDir); // try to be at the same level

							Vector3 startToEnemy = enemyLoc - startLoc;
							Vector3 right = Vector3::cross(startToEnemy, upDir).normal();
							goToLoc = startLoc + startToEnemy * 0.5f + right * startToEnemy.length() * 0.5f * (float)(flankStrategy);
							Vector3 enemyToGoTo = goToLoc - enemyLoc;
							float dist = enemyToGoTo.length();
							if (dist < attackDist * 2.0f)
							{
								dist = attackDist * 0.5f;
							}
							goToLoc = enemyLoc + enemyToGoTo.normal() * dist;
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
				scoped_call_stack_info(TXT("update path from a finished path task"));
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

		//LATENT_LOG(TXT("move for %.1fs"), timeToShoot);
	MOVE:
		ai_log(mind->get_logic(), TXT("move using path"));
		timeToUpdatePath = rg.get_float(3.0f, 7.0f);
		while (true)
		{
			if (cooldowns.is_available(NAME(playAttackSound)))
			{
				scoped_call_stack_info(TXT("move, attack sound"));
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
				cooldowns.set(NAME(playAttackSound), rg.get_float(1.0f, 3.0f));
			}

			updatePath = timeToUpdatePath < 0.0f;
			attackNow = false;
			{
				scoped_call_stack_info(TXT("move, locomotion"));
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					locomotion.dont_move();
					updatePath = true;
				}
				else if (path.get() && ! path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
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
					updatePath = true;
				}
			}
			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.has_finished_move(0.5f))
				{
					updatePath = true;
				}
			}
			if (enemyPlacement.get_target())
			{
				scoped_call_stack_info(TXT("move, run or walk"));
				float flatDistance;
				float stringPulledDistance;
				enemyPlacement.calculate_distances(flatDistance, stringPulledDistance);
				if (flatDistance > stringPulledDistance * 0.7f && flatDistance <= attackDist) // straight enough
				{
					attackNow = true;
				}
				{
					auto& locomotion = mind->access_locomotion();
					if (stringPulledDistance > 2.0f)
					{
						locomotion.access_movement_parameters().gait(NAME(run));
					}
					else
					{
						locomotion.access_movement_parameters().default_gait();
					}
				}
			}

			if (attackNow)
			{
				goto ATTACK;
			}
			if (updatePath)
			{
				LATENT_YIELD();
				goto UPDATE_PATH;
			}
			LATENT_WAIT(rg.get_float(0.1f, 0.3f));
		}
		goto MOVE;

	ATTACK:
		ai_log(mind->get_logic(), TXT("perform attack"));
		perceptionEnemyLock.override_lock(NAME(attack), AI::PerceptionLock::Lock);
		todo_note(TXT("hardcoded socket assumed always present"));
		an_assert(imo->get_appearance());
		if (npcBase &&
			enemyPlacement.get_target() &&
			(!GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed) &&
			check_clear_ray_cast(CastInfo(), imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index())).get_translation(), imo,
				enemyPlacement.get_target(),
				enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_owner_room()))
		{
			LATENT_LOG(TXT("attack"));
			{
				auto& locomotion = mind->access_locomotion();
				locomotion.stop();
				locomotion.turn_towards_2d(enemyPlacement, 5.0f);
			}

			LATENT_WAIT(0.6f);

			lightningSpawner->stop(NAME(idle));

			thisTaskHandle->allow_to_interrupt(false);

			LATENT_LOG(TXT("charging"));

			if (Framework::GameUtils::is_controlled_by_player(enemy.get()))
			{
				if (enemyPlacement.get_target())
				{
					MusicPlayer::request_combat_auto();
				}
				else
				{
					MusicPlayer::request_combat_auto_indicate_presence();
				}
			}

			self->give_out_enemy_location(enemyPlacement);

			attackTimeMultiplier = switchSidesHandler && switchSidesHandler->get_switch_sides_to() ? 0.4f : 1.0f;
			attackTimeMultiplier *= GameSettings::get().difficulty.aiReactionTime;

			lightningSpawner->stop(NAME(charging));
			cooldowns.add(NAME(charging), 2.5f * attackTimeMultiplier);
			if (auto* s = imo->get_sound())
			{
				s->play_sound(NAME(charging));
			}
			while (!cooldowns.is_available(NAME(charging)))
			{
				lightningSpawner->add(NAME(charging));
				LATENT_WAIT(0.5f * attackTimeMultiplier);
			}
			lightningSpawner->stop(NAME(charging));
			if (auto* s = imo->get_sound())
			{
				s->stop_sound(NAME(charging));
			}
			LATENT_LOG(TXT("done charging, wait"));
			LATENT_WAIT(0.5f * sqr(attackTimeMultiplier));
			if (npcBase &&
				enemyPlacement.get_target())
			{
				{
					scoped_call_stack_info(TXT("attack, lightning strike and so on"));

					if (Actions::do_lightning_strike(mind, enemyPlacement, magic_number 3.0f, Actions::LightningStrikeParams().discharge_damage(self->dischargeDamage).ignore_violence_disallowed(ignoreViolenceDisallowed)))
					{
						// push back
						imo->get_presence()->add_velocity_impulse(imo->get_presence()->get_placement().vector_to_world(Vector3::yAxis * magic_number (-1.2f)));
						//
						attackSuccessful = true;
					}
					{
						auto& locomotion = mind->access_locomotion();
						locomotion.stop();
					}
				}
				LATENT_WAIT(1.0f);
			}
			else
			{
				LATENT_LOG(TXT("would miss"));
			}

			thisTaskHandle->allow_to_interrupt(true);

			lightningSpawner->start(NAME(idle));
		}
		perceptionEnemyLock.override_lock(NAME(attack));
		goto POST_ATTACK;

	POST_ATTACK:
		ai_log(mind->get_logic(), TXT("post attack"));
		if (attackSuccessful)
		{
			ai_log(mind->get_logic(), TXT("find some path and run away"));
			pathTask = mind->access_navigation().get_random_path(5.0f, enemyPlacement.is_active()? Optional<Vector3>(enemyPlacement.get_direction_towards_placement_in_owner_room()) : NP, Framework::Nav::PathRequestInfo(imo));

			path.clear();
			LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
			{
				scoped_call_stack_info(TXT("post attack, get path"));
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					path = task->get_path();
				}
				else
				{
					path.clear();
				}
			}

			ai_log(mind->get_logic(), TXT("path available, run?"));

			{
				scoped_call_stack_info(TXT("post attack, run away"));

				float timeMultiplier = switchSidesHandler && switchSidesHandler->get_switch_sides_to() ? 0.6f : 1.0f;

				cooldowns.set(NAME(runAway), rg.get_float(4.0f, 6.0f) * timeMultiplier);
			}

			while (path.is_set() && !cooldowns.is_available(NAME(runAway)))
			{
				attackNow = false;
				{
					scoped_call_stack_info(TXT("post attack, run away, follow path"));
					auto& locomotion = mind->access_locomotion();
					if (! locomotion.check_if_path_is_ok(path.get()))
					{
						locomotion.dont_move();
						updatePath = true;
					}
					else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
					{
						if (!locomotion.is_following_path(path.get()))
						{
							locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed().gait(NAME(run)));
							locomotion.turn_follow_path_2d();
						}
					}
					else
					{
						locomotion.dont_move();
						path.clear();
					}
				}
				{
					scoped_call_stack_info(TXT("post attack, run away, follow path ended?"));
					auto& locomotion = mind->access_locomotion();
					if (locomotion.has_finished_move(0.5f))
					{
						locomotion.dont_move();
						path.clear();
					}
				}
				LATENT_WAIT(rg.get_float(0.1f, 0.3f));
			}

		}

	//DONE:
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
	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			em->emissive_deactivate(NAME(attack));
		}
	}
	if (lightningSpawner)
	{
		lightningSpawner->stop(NAME(charging));
		lightningSpawner->start(NAME(idle));
	}
	if (auto* s = imo->get_sound())
	{
		s->stop_sound(NAME(charging));
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(attack_or_idle)
{
	scoped_call_stack_info(TXT("barkers attack or idle"));
	/**
	 *	Uses perception task to look for enemy
	 *	If enemy is found and we're aggressive enough, attack enemy
	 *	If no enemy or we ignore enemy, idle
	 */
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack or idle (barker's)"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	
	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->start(NAME(idle));
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::attack_or_idle));
		currentTask.start_latent_task(mind, taskInfo);
	}

	while (currentTask.is_running())
	{
		LATENT_YIELD();
	}
	
	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->stop(NAME(idle));
	}

	LATENT_END_CODE();
	//

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(currentTask.can_start_new());

	LATENT_RETURN();
}

static LATENT_FUNCTION(stay_close_in_front)
{
	/**
	*	Uses perception task to look for enemy
	*	If enemy is found and we're aggressive enough, attack enemy
	*	If no enemy or we ignore enemy, idle
	*/
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("stay close in front (barker's)"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM_OPTIONAL(SafePtr<::Framework::IModulesOwner>, stayCloseTo, SafePtr<::Framework::IModulesOwner>());

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->start(NAME(idle));
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::attack_or_idle));
		ADD_LATENT_TASK_INFO_PARAM(taskInfo, SafePtr<::Framework::IModulesOwner>, stayCloseTo);
		currentTask.start_latent_task(mind, taskInfo);
	}

	while (currentTask.is_running())
	{
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->stop(NAME(idle));
	}

	LATENT_END_CODE();
	//

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(currentTask.can_start_new());

	LATENT_RETURN();
}

LATENT_FUNCTION(Barker::execute_logic)
{
	scoped_call_stack_info(TXT("Barker::execute_logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR(Range, stayCloseGoFurtherDist);
	LATENT_COMMON_VAR(Range, stayCloseGoFurtherTime);
	LATENT_COMMON_VAR(Range, stayCloseGoFurtherInterval);
	LATENT_COMMON_VAR(Name, stayCloseGait);
	LATENT_COMMON_VAR(float, stayCloseGaitDist);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, announcePresenceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(bool, emissiveFriendly);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(CustomModules::SwitchSidesHandler*, switchSidesHandler);
	LATENT_VAR(bool, forceNewHeel);
	LATENT_VAR(float, heelTimeLeft);
	LATENT_VAR(float, ignoreEnemiesTimeLeft);

	aggressiveness = 99.0f;

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Barker>(logic);

	heelTimeLeft = max(0.0f, heelTimeLeft - LATENT_DELTA_TIME);
	ignoreEnemiesTimeLeft = max(0.0f, ignoreEnemiesTimeLeft - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	stayCloseGoFurtherDist = Range(10.0f, 20.0f);
	stayCloseGoFurtherTime = Range(5.0f, 12.0f);
	stayCloseGoFurtherInterval = Range(5.0f, 10.0f);
	stayCloseGait = NAME(run);
	stayCloseGaitDist = 2.0f;

	self->dischargeDamage = imo->get_variables().get_value(NAME(dischargeDamage), Energy(magic_number 1.0f));

	self->asPickup = imo->get_custom<CustomModules::Pickup>();

	if (auto* logicLatent = fast_cast<::Framework::AI::LogicWithLatentTask>(logic))
	{
		logicLatent->register_latent_task(NAME(attackSequence), Barker::attack_enemy);
		logicLatent->register_latent_task(NAME(idleSequence), Tasks::stay_close_or_wander);
		logicLatent->register_latent_task(NAME(stayCloseSequence), Tasks::stay_close_in_front);
		logicLatent->register_latent_task(NAME(wanderSequence), Tasks::wander);
	}

	if (ShouldTask::announce_presence(imo))
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

	emissiveFriendly = false;

	switchSidesHandler = imo->get_custom<CustomModules::SwitchSidesHandler>();
	forceNewHeel = false;
	heelTimeLeft = 0.0f;
	ignoreEnemiesTimeLeft = 0.0f;
	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(attract), [switchSidesHandler, &forceNewHeel, &heelTimeLeft, &ignoreEnemiesTimeLeft](Framework::AI::Message const& _message)
		{
			if (auto * whoParam = _message.get_param(NAME(who)))
			{
				if (auto * who = whoParam->get_as<SafePtr<Framework::IModulesOwner>>().get())
				{
					who = fast_cast<Framework::IModulesOwner>(cast_to_nonconst(who->ai_instigator()));
					if (switchSidesHandler && switchSidesHandler->get_switch_sides_to() == who)
					{
						forceNewHeel = true;
						heelTimeLeft = Random::get_float(10.0f, 30.0f);
						ignoreEnemiesTimeLeft = 5.0f;
					}
				}
			}
		}
		);
	}
	while (true)
	{
		{
			// choose best action for now
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			bool specialTask = false;

			nextTask.propose(AI_LATENT_TASK_FUNCTION(attack_or_idle));
			if (ignoreEnemiesTimeLeft > 0.0f)
			{
				enemyPlacement.clear_target();
			}
			if (ShouldTask::being_held(self->asPickup))
			{
				specialTask = true;
				heelTimeLeft = 0.0f;
				ignoreEnemiesTimeLeft = 0.0f;
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_held), 20, NP, NP, true);
			}
			else if (ShouldTask::being_thrown(imo))
			{
				specialTask = true;
				heelTimeLeft = 0.0f;
				ignoreEnemiesTimeLeft = 0.0f;
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_thrown), 20, NP, NP, true);
			}
			else if (! scriptedBehaviour.is_valid())
			{
				if (heelTimeLeft > 0.0f && !enemyPlacement.is_active())
				{
					// stop any current task
					if (forceNewHeel && currentTask.can_start_new())
					{
						currentTask.stop();
						forceNewHeel = false;
					}
					auto* sst = switchSidesHandler ? switchSidesHandler->get_switch_sides_to() : nullptr;
					if (sst)
					{
						if (nextTask.propose(AI_LATENT_TASK_FUNCTION(stay_close_in_front)))
						{
							ADD_LATENT_TASK_INFO_PARAM(nextTask, SafePtr<::Framework::IModulesOwner>, SafePtr<::Framework::IModulesOwner>(sst));
						}
					}
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

//

REGISTER_FOR_FAST_CAST(BarkerData);

BarkerData::BarkerData()
{
}

BarkerData::~BarkerData()
{
}

bool BarkerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool BarkerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
