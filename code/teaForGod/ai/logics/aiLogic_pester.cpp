#include "aiLogic_pester.h"

#include "actions\aiAction_lightningStrike.h"

#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_beingHeld.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_projectileHit.h"
#include "tasks\aiLogicTask_wander.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"

#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"
#include "..\..\music\musicPlayer.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_energyStorage.h"
#include "..\..\modules\custom\mc_interactiveButtonHandler.h"
#include "..\..\modules\custom\mc_pickup.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\general\cooldowns.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navLatentUtils.h"
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

// variables
DEFINE_STATIC_NAME(dontWalk);
DEFINE_STATIC_NAME(beingThrown);
DEFINE_STATIC_NAME(walkerShouldFall);
DEFINE_STATIC_NAME(locomotionShouldFall);
DEFINE_STATIC_NAME(dischargeDamage);
DEFINE_STATIC_NAME(lootObject);

// sounds / emissives
DEFINE_STATIC_NAME(alert);
DEFINE_STATIC_NAME(attack);

// lightnings
DEFINE_STATIC_NAME(idle);
DEFINE_STATIC_NAME(charging);
DEFINE_STATIC_NAME(discharge);

// movement names
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(thrown);

// task feedback
DEFINE_STATIC_NAME(taskFeedback);
DEFINE_STATIC_NAME(enemyLost);

// cooldowns
DEFINE_STATIC_NAME(runAway);
DEFINE_STATIC_NAME(playAttackSound);

// tasks
DEFINE_STATIC_NAME(attackSequence);
DEFINE_STATIC_NAME(idleSequence);
DEFINE_STATIC_NAME(wanderSequence);

//

REGISTER_FOR_FAST_CAST(Pester);

Pester::Pester(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	data = fast_cast<PesterData>(_logicData);
}

Pester::~Pester()
{
}

void Pester::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

void Pester::advance(float _deltaTime)
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
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, forceInvestigate);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);

	LATENT_VAR(float, timeLeft);
	LATENT_VAR(bool, wasForcedToInvestigate);

	timeLeft -= LATENT_DELTA_TIME;

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	timeLeft = 0.0f;
	wasForcedToInvestigate = false;

	while (true)
	{
		if (cooldowns.is_available(NAME(alert)))
		{
			auto* imo = mind->get_owner_as_modules_owner();

			scoped_call_stack_info(TXT("play sound"));
			if (auto* s = imo->get_sound())
			{
				s->play_sound(NAME(alert));
			}
			cooldowns.set(NAME(alert), Random::get_float(1.5f, 4.0f));
		}

		if (timeLeft <= 0.0f ||
			(!wasForcedToInvestigate && forceInvestigate)) // this is to ensure we go investigating as quickly as we're forced to
		{
			timeLeft = Random::get_float(2.0f, 3.0f);
			if (Random::get_chance(0.7f) || forceInvestigate)
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
			}

			if (nextTask.is_proposed())
			{
				if (currentTask.can_start(nextTask))
				{
					LATENT_LOG(TXT("start wander task"));
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("start_latent_task"));
					currentTask.start_latent_task(mind, nextTask);
				}
			}
			else
			{
				if (currentTask.is_running())
				{
					LATENT_LOG(TXT("stop wander task"));
					currentTask.stop();
				}
			}
			nextTask.clear();

			wasForcedToInvestigate = forceInvestigate;
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Pester::attack_enemy)
{
	scoped_call_stack_info(TXT("pester::attack_enemy"));

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

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);

	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Pester>(mind->get_logic());
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
		LATENT_CLEAR_LOG();
		ai_log(mind->get_logic(), TXT("start attack loop"));
		if (cooldowns.is_available(NAME(playAttackSound)))
		{
			scoped_call_stack_info(TXT("play sound"));
			if (auto* s = imo->get_sound())
			{
				s->play_sound(rg.get_chance(0.7f)? NAME(alert) : NAME(attack));
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
			flankStrategy = rg.get_chance(0.9f) ? (rg.get_chance(0.5f) ? 1 : -1) : 0;
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
							goToLoc = startLoc + startToEnemy * 0.5f + right * startToEnemy.length() * 0.5f * (float)(flankStrategy * (startToEnemy.length() < 3.0f? 1 : 1));
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
		timeToUpdatePath = rg.get_float(2.0f, 5.0f);
		while (true)
		{
			if (cooldowns.is_available(NAME(playAttackSound)))
			{
				scoped_call_stack_info(TXT("move, attack sound"));
				if (auto* s = imo->get_sound())
				{
					if (s->play_sound(rg.get_chance(0.7f) ? NAME(alert) : NAME(attack)))
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
				if (flatDistance > stringPulledDistance * 0.7f && flatDistance <= attackDist * 1.2f) // straight enough
				{
					attackNow = true;
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
			(! GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed) &&
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

			lightningSpawner->stop(NAME(idle));

			{
				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(attack));
				}
			}

			thisTaskHandle->allow_to_interrupt(false);

			if (npcBase &&
				enemyPlacement.get_target())
			{
				{
					scoped_call_stack_info(TXT("attack, lightning strike and so on"));

					if (Actions::do_lightning_strike(mind, enemyPlacement, magic_number 3.0f, Actions::LightningStrikeParams().discharge_damage(self->dischargeDamage).ignore_violence_disallowed(ignoreViolenceDisallowed)))
					{
						// push back
						imo->get_presence()->add_velocity_impulse(imo->get_presence()->get_placement().vector_to_world(Vector3::yAxis * magic_number (-0.5f)));
						//
						attackSuccessful = true;
					}
					{
						auto& locomotion = mind->access_locomotion();
						locomotion.stop();
					}
				}
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
			updateFlankStrategy = true;

			// if reacts fast (mean machine), it is unlikely that it will stand still
			if (rg.get_chance(clamp(0.025f + 0.5f * GameSettings::get().difficulty.aiReactionTime, 0.0f, 1.0f)))
			{
				LATENT_WAIT(rg.get_float(0.5f, 1.0f));
			}
			else
			{
				LATENT_WAIT(0.25f * GameSettings::get().difficulty.aiReactionTime);
				ai_log(mind->get_logic(), TXT("find some path and run away"));
				pathTask = mind->access_navigation().get_random_path(1.0f, enemyPlacement.is_active() ? Optional<Vector3>(enemyPlacement.get_direction_towards_placement_in_owner_room()) : NP, Framework::Nav::PathRequestInfo(imo));

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

					cooldowns.set(NAME(runAway), rg.get_float(0.5f, 1.0f));
				}

				while (path.is_set() && !cooldowns.is_available(NAME(runAway)))
				{
					attackNow = false;
					{
						scoped_call_stack_info(TXT("post attack, run away, follow path"));
						auto& locomotion = mind->access_locomotion();
						if (!locomotion.check_if_path_is_ok(path.get()))
						{
							locomotion.dont_move();
							updatePath = true;
						}
						else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
						{
							if (!locomotion.is_following_path(path.get()))
							{
								locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().relative_speed(0.4f));
								locomotion.turn_towards_2d(enemyPlacement, 5.0f);
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
			{
				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(attack));
				}
			}
			{
				scoped_call_stack_info(TXT("post attack, post run away, stop"));
				auto& locomotion = mind->access_locomotion();
				locomotion.dont_move();
				path.clear();
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
	scoped_call_stack_info(TXT("pester attack or idle"));
	/**
	 *	Uses perception task to look for enemy
	 *	If enemy is found and we're aggressive enough, attack enemy
	 *	If no enemy or we ignore enemy, idle
	 */
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("attack or idle (pester's)"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	auto* imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	if (auto* ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
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
	if (auto* ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->stop(NAME(idle));
	}

	LATENT_END_CODE();
	//

	// update allow to interrupt state
	thisTaskHandle->allow_to_interrupt(currentTask.can_start_new());

	LATENT_RETURN();
}

LATENT_FUNCTION(Pester::execute_logic)
{
	scoped_call_stack_info(TXT("Pester::execute_logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(float, aggressiveness);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(bool, emissiveFriendly);

	LATENT_VAR(float, ignoreEnemiesTimeLeft);

	aggressiveness = 99.0f;

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Pester>(logic);

	ignoreEnemiesTimeLeft = max(0.0f, ignoreEnemiesTimeLeft - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	self->dischargeDamage = imo->get_variables().get_value(NAME(dischargeDamage), Energy(magic_number 0.5f));

	if (auto* logicLatent = fast_cast<::Framework::AI::LogicWithLatentTask>(logic))
	{
		logicLatent->register_latent_task(NAME(attackSequence), Pester::attack_enemy);
		logicLatent->register_latent_task(NAME(idleSequence), wander_around);
	}

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	emissiveFriendly = false;

	LATENT_WAIT(Random::get_float(0.03f, 0.3f));

	if (auto* imo = mind->get_owner_as_modules_owner())
	{
		if (!imo->get_variables().get_value<bool>(NAME(lootObject), false))
		{
			imo->get_presence()->request_on_spawn_random_dir_teleport(4.5f, 0.4f);
		}
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

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
			if (ShouldTask::being_thrown(imo))
			{
				specialTask = true;
				ignoreEnemiesTimeLeft = 0.0f;
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_thrown), 20, NP, NP, true);
			}
			else if (! scriptedBehaviour.is_valid())
			{
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
	perceptionTask.stop();
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(PesterData);

PesterData::PesterData()
{
}

PesterData::~PesterData()
{
}

bool PesterData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool PesterData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
