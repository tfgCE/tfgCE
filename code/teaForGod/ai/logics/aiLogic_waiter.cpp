#include "aiLogic_waiter.h"

#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_wander.h"

#include "..\aiCommonVariables.h"
#include "..\aiRayCasts.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
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
#include "..\..\..\framework\world\room.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\sound\ssm_glitchStutter.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

//#define DEBUG_DRAW_THROWING

//

// socket
DEFINE_STATIC_NAME(projectile);

// sounds
DEFINE_STATIC_NAME(drink);
DEFINE_STATIC_NAME(throw);

// cooldowns (+ perception locks)
DEFINE_STATIC_NAME(nextIdleWanderTask);
DEFINE_STATIC_NAME(giveDrink);
DEFINE_STATIC_NAME(speakingGlitch);
DEFINE_STATIC_NAME(lockPerceptionForAWhile);

//

REGISTER_FOR_FAST_CAST(Waiter);

Waiter::Waiter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Waiter::~Waiter()
{
}

static LATENT_FUNCTION(give_drink)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("give drink"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(bool, attackNoEnemy);

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	LATENT_VAR(Framework::CooldownsSmall, cooldowns);

	LATENT_VAR(float, minDist);

	LATENT_VAR(float, timeLeft);

	LATENT_VAR(bool, giveDrink);

	LATENT_VAR(Framework::SoundSourcePtr, speaking);
	LATENT_VAR(int, drinksLeft);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());
	an_assert(npcBase);

	timeLeft = max(0.0f, timeLeft - LATENT_DELTA_TIME);

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("give drink"));
	ai_log_no_colour(mind->get_logic());

	perceptionEnemyLock.override_lock(NAME(giveDrink), AI::PerceptionLock::Lock);

	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		//em->activate(NAME(attack));
	}

	minDist = 3.0f;

	{
		LATENT_CLEAR_LOG();

		if (enemyPlacement.get_target())
		{
			LATENT_LOG(TXT("seeing enemy close"));
			giveDrink = enemyPlacement.calculate_string_pulled_distance() < minDist;
		}
		else
		{
			giveDrink = false;
		}

		if (giveDrink)
		{
			goto GIVE_DRINK;
		}

		attackNoEnemy = Random::get_chance(0.2f);

		// try to move to enemy?
		if (! pathTask.is_set() && enemyPlacement.is_active())
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

			Framework::Nav::PathRequestInfo pathRequestInfo(imo);
			if (!Mobility::may_leave_room_when_attacking(imo, imo->get_presence()->get_in_room()))
			{
				pathRequestInfo.within_same_room_and_navigation_group();
			}

			pathTask = mind->access_navigation().find_path_to(enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_final_room().get_translation(), pathRequestInfo);
		}
		else
		{
			attackNoEnemy = true;
		}

		// find path to enemy
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

		if (attackNoEnemy)
		{
			LATENT_LOG(TXT("attack no enemy"));
			goto GIVE_DRINK;
		}

		LATENT_LOG(TXT("move"));
		MOVE:
		{
			{
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					locomotion.dont_move();
					giveDrink = true;
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
					LATENT_LOG(TXT("not moving anywhere?"));
					locomotion.dont_move();
					giveDrink = true;
				}
			}

			if (enemyPlacement.get_target())
			{
				LATENT_LOG(TXT("seeing enemy close"));
				giveDrink = enemyPlacement.calculate_string_pulled_distance() < minDist;
			}

			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.has_finished_move(0.5f))
				{
					LATENT_LOG(TXT("reached desination"));
					locomotion.dont_move();
					giveDrink = true;
				}
			}

			if (giveDrink)
			{
				goto GIVE_DRINK;
			}
			LATENT_WAIT(Random::get_float(0.1f, 0.3f));
		}
		goto MOVE;

		GIVE_DRINK:
		todo_note(TXT("hardcoded socket assumed always present"));
		an_assert(imo->get_appearance());
		if (attackNoEnemy ||
			(npcBase &&
			 enemyPlacement.get_target() &&
				check_clear_ray_cast(CastInfo(), imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index())).get_translation(), imo,
							enemyPlacement.get_target(),
							enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_owner_room())))
		{
			LATENT_LOG(TXT("give drink sequence"));
			{
				{
					auto& locomotion = mind->access_locomotion();
					locomotion.stop();
					if (attackNoEnemy)
					{
						LATENT_LOG(TXT("turn randomly"));
						Vector3 randomOffset(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f));
						locomotion.turn_towards_2d(imo->get_presence()->get_placement().get_translation() + randomOffset.normal() * 20.0f, 25.0f);
					}
					else
					{
						LATENT_LOG(TXT("turn towards enemy"));
						locomotion.turn_towards_2d(enemyPlacement.get_placement_in_owner_room().get_translation(), 5.0f);
					}
				}

				LATENT_SECTION_INFO(TXT("wait for turn to end"));
				timeLeft = 10.0f;
				while (timeLeft > 0.0f)
				{
					if (mind->access_locomotion().is_done_with_turn())
					{
						break;
					}
					LATENT_WAIT(0.1f);
				}

				mind->access_locomotion().stop();
			}


			LATENT_LOG(TXT("propose a drink"));

			if (auto* s = imo->get_sound())
			{
				if (speaking.is_set())
				{
					speaking->stop();
				}
				speaking = s->play_sound(NAME(drink));
				cooldowns.set(NAME(speakingGlitch), Random::get_float(2.0f, 3.0f));
				if (speaking.get())
				{
					new SoundSourceModifiers::GlitchStutter(speaking.get());
				}
			}

			while (speaking.is_set() && speaking->is_active() && ! cooldowns.is_available(NAME(speakingGlitch)))
			{
				LATENT_WAIT(0.05f);
			}
			if (speaking.is_set())
			{
				speaking->stop();
			}

			if (!attackNoEnemy && enemyPlacement.is_active())
			{
				LATENT_LOG(TXT("turn towards enemy"));
				{
					auto& locomotion = mind->access_locomotion();
					locomotion.stop();
					locomotion.turn_towards_2d(enemyPlacement.get_placement_in_owner_room().get_translation(), 5.0f);
				}

				LATENT_SECTION_INFO(TXT("wait for turn to end"));
				timeLeft = 2.0f;
				while (timeLeft > 0.0f)
				{
					if (mind->access_locomotion().is_done_with_turn())
					{
						break;
					}
					LATENT_WAIT(0.1f);
				}

				mind->access_locomotion().stop();
			}

			if (auto* h = imo->get_custom<CustomModules::Health>())
			{
				if (!h->get_health().is_positive())
				{
					LATENT_END();
				}
			}

			drinksLeft = Random::get_int_from_range(1, 4);

			while (drinksLeft > 0)
			{
				--drinksLeft;
				if (auto* tos = imo->get_temporary_objects())
				{
					auto* projectile = tos->spawn(NAME(drink));
					if (projectile)
					{
#ifdef DEBUG_DRAW_THROWING
						debug_context(imo->get_presence()->get_in_room());
#endif
						if (auto* s = imo->get_sound())
						{
							s->play_sound(NAME(throw));
						}
						// just in any case if we would be shooting from inside of a capsule
						if (auto* collision = projectile->get_collision())
						{
							collision->dont_collide_with_up_to_top_instigator(imo, 0.5f);
						}
						Vector3 gravity = projectile->get_movement()->get_use_gravity() * imo->get_presence()->get_gravity();
						Vector3 targetAt = imo->get_presence()->get_placement().location_to_world(Vector3(0.0f, 5.0f, 1.5f));
						if (!attackNoEnemy && enemyPlacement.is_active() && enemyPlacement.get_target())
						{
							targetAt = enemyPlacement.get_placement_in_owner_room().location_to_world(enemyTargetingOffsetOS);
						}
						todo_note(TXT("hardcoded!"));
						Vector3 startAt = imo->get_presence()->get_placement().location_to_world(imo->get_appearance()->calculate_socket_os(NAME(projectile)).get_translation());
#ifdef DEBUG_DRAW_THROWING
						debug_draw_time_based(5.0f, debug_draw_arrow(true, Colour::red, startAt, targetAt));
#endif
						Vector3 distanceToCover = targetAt - startAt;
						Vector3 distanceToCoverFlat = distanceToCover.drop_using(gravity);
						Vector3 forward = distanceToCoverFlat.normal();
						Vector3 up = -gravity.normal();
						float speed = Random::get_float(4.5f, 6.0f);
						/*
						
							^
							|
							|
							|     _,_
							|  ,-'   '-,
							|.'         *
							+------------------>

							dist - knonw
							hDiff - known
							speed - known
							angle - not known

							dist = speed * cos(angle) * t
							t = dist / (speed * cos(angle))

							hDiff = speed * sin(angle) * t - gravity * t^2 / 2
							*/
						float angle = 25.0f;
						{	// use simple iteration method
							float hDiff = Vector3::dot(distanceToCover, up);
							float dist = Vector3::dot(distanceToCover, forward);
							float g = gravity.length();
							for (int it = 0; it < 20; ++it)
							{
								float angleD = 0.01f;
								float sinAngle = sin_deg(angle);
								float cosAngle = cos_deg(angle);
								float sinAngleD = sin_deg(angle + angleD);
								float cosAngleD = cos_deg(angle + angleD);
								float t = dist / (speed * cosAngle);
								float resHDiff = speed * sinAngle * t - g * sqr(t) * 0.5f;
								float tD = dist / (speed * cosAngleD);
								float resHDiffD = speed * sinAngleD * tD - g * sqr(tD) * 0.5f;
								float target = sqr(resHDiff - hDiff);
								float targetD = sqr(resHDiffD - hDiff);
								angle -= (targetD - target) / sqr(angleD); // targetD < target? we go in angleD direction
								angle = clamp(angle, 0.0f, 60.0f);
							}
						}

						todo_note(TXT("hardcoded"));
						Vector3 velocity = (forward * cos_deg(angle) + up * sin_deg(angle)) * speed;
#ifdef DEBUG_DRAW_THROWING
						debug_draw_time_based(5.0f, debug_draw_arrow(true, Colour::green, startAt, startAt + velocity));
#endif
						float dispersionAngleDeg = 10.0f;
						Quat dispersionAngle = Rotator3(0.0f, 0.0f, Random::get_float(-180.0f, 180.0f)).to_quat().to_world(Rotator3(Random::get_float(0.0f, dispersionAngleDeg), 0.0f, 0.0f).to_quat());
						Quat velocityDir = velocity.to_rotator().to_quat();
						velocity = velocityDir.to_world(dispersionAngle).get_y_axis() * velocity.length();
						projectile->on_activate_set_velocity(velocity);
						Rotator3 velocityRotation = Rotator3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-0.7f, 0.7f));
						velocityRotation = velocityRotation.normal() * (760.0f * Random::get_float(0.8f, 1.2f));
						projectile->on_activate_set_velocity_rotation(velocityRotation);
#ifdef DEBUG_DRAW_THROWING
						debug_no_context();
#endif
					}
				}
				if (drinksLeft > 0)
				{
					LATENT_WAIT(Random::get_float(0.3f, 1.5f));
				}
			}
			LATENT_WAIT(Random::get_float(0.9f, 1.2f));
		}
		giveDrink = false;
	}
	// ...

	LATENT_WAIT(Random::get_float(0.1f, 0.2f));

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	if (speaking.is_set())
	{
		speaking->stop();
	}
	perceptionEnemyLock.override_lock(NAME(giveDrink));
	if (mind)
	{
		auto& locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	if (imo)
	{
		if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
		{
			//em->deactivate(NAME(attack));
		}
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Waiter::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	perceptionEnemyLock.set_base_lock(AI::PerceptionLock::LookForNew); // and always look for a new one

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		if (cooldowns.is_available(NAME(lockPerceptionForAWhile)))
		{
			if (perceptionEnemyLock.is_overriden_lock_set(NAME(lockPerceptionForAWhile)))
			{
				perceptionEnemyLock.override_lock(NAME(lockPerceptionForAWhile));
				cooldowns.set(NAME(lockPerceptionForAWhile), Random::get_float(1.0f, 3.0f));
			}
			else
			{
				perceptionEnemyLock.override_lock(NAME(lockPerceptionForAWhile), AI::PerceptionLock::KeepButAllowToChange);
				cooldowns.set(NAME(lockPerceptionForAWhile), Random::get_float(1.5f, 5.0f));
			}
		}

		{
			// think about new task only if current one is breakable
			if (currentTask.can_start_new())
			{
				::Framework::AI::LatentTaskInfoWithParams nextTask;
				// choose best action for now
				if (cooldowns.is_available(NAME(nextIdleWanderTask)) || !currentTask.is_running())
				{
					if (!currentTask.is_running())
					{
						if (Random::get_chance(0.7f))
						{
							nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
						}
						else
						{
							nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle));
						}
						cooldowns.set(NAME(nextIdleWanderTask), Random::get_float(1.0f, 3.0f));
					}
					else if (currentTask.is_running(Tasks::idle))
					{
						nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
						cooldowns.set(NAME(nextIdleWanderTask), Random::get_float(1.0f, 3.0f));
					}
					else if (currentTask.is_running(Tasks::wander))
					{
						if (Random::get_chance(0.7f))
						{
							nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));
						}
						else
						{
							nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle));
						}
						cooldowns.set(NAME(nextIdleWanderTask), Random::get_float(1.0f, 3.0f));
					}
				}

				// if we have target, choose to give a drink to it
				if (cooldowns.is_available(NAME(giveDrink)) && enemy.is_set())
				{
					nextTask.propose(AI_LATENT_TASK_FUNCTION(give_drink), 20);
				}

				// check if we can start new one, if so, start it
				if (currentTask.can_start(nextTask))
				{
					currentTask.start_latent_task(mind, nextTask);
				}
			}

			if (currentTask.is_running(give_drink))
			{
				cooldowns.set_if_longer(NAME(giveDrink), Random::get_float(2.0f, 6.0f));
			}
		}

		LATENT_WAIT(Random::get_float(0.05f, 0.25f));
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
