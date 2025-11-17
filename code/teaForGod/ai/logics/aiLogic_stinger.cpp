#include "aiLogic_stinger.h"

#include "actions\aiAction_lightningStrike.h"

#include "tasks\aiLogicTask_beingHeld.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_flyTo.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_perception.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\library\library.h"
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

// sounds
DEFINE_STATIC_NAME(attack);
DEFINE_STATIC_NAME(spit);
DEFINE_STATIC_NAME(engine);

// gaits
DEFINE_STATIC_NAME(fast);
DEFINE_STATIC_NAME(normal);
DEFINE_STATIC_NAME(keepClose);

// variables
DEFINE_STATIC_NAME(dischargeDamage);

// lightnings
DEFINE_STATIC_NAME(idle);
DEFINE_STATIC_NAME(charging);
DEFINE_STATIC_NAME(discharge);

// movement names
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(thrown);

// pause reasons
DEFINE_STATIC_NAME(energyDrained);

// temporary objects
DEFINE_STATIC_NAME_STR(healthInhaled, TXT("health inhaled"));
DEFINE_STATIC_NAME_STR(spitDischarge, TXT("spit discharge"));

//

REGISTER_FOR_FAST_CAST(Stinger);

Stinger::Stinger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	data = fast_cast<StingerData>(_logicData);
}

Stinger::~Stinger()
{
}

void Stinger::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.25f));

	base::advance(_deltaTime);
}

LATENT_FUNCTION(Stinger::fly_freely)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("fly freely"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, flyTask);
	LATENT_VAR(Framework::RelativeToPresencePlacement, flyPlacement);

	LATENT_VAR(float, timeLeftToNewDirection);

	auto* imo = mind->get_owner_as_modules_owner();
	Stinger* self = fast_cast<Stinger>(mind->get_logic());

	timeLeftToNewDirection -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeLeftToNewDirection = 0.0f;

	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
		locomotion.turn_in_movement_direction_2d();
	}

	while (true)
	{
		if (self->should_stay_in_room())
		{
			flyTask.stop();
			bool collisionAhead = false;
			{
				Framework::CheckCollisionContext checkCollisionContext;
				checkCollisionContext.collision_info_needed();
				checkCollisionContext.use_collision_flags(imo->get_collision()->get_collides_with_flags());
				checkCollisionContext.ignore_temporary_objects();
				checkCollisionContext.avoid(imo);
				Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
				Vector3 loc = imo->get_presence()->get_centre_of_presence_WS();
				collisionTrace.add_location(loc);
				collisionTrace.add_location(loc + imo->get_presence()->get_velocity_linear() * 0.8f);
				Framework::CheckSegmentResult result;
				if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, Framework::CollisionTraceFlag::ResultInPresenceRoom, checkCollisionContext))
				{
					// collided
					timeLeftToNewDirection = 0.0f;
					collisionAhead = true;
				}
			}
			if (timeLeftToNewDirection <= 0.0f)
			{
				bool turnAround = collisionAhead || Random::get_chance(0.2f);
				timeLeftToNewDirection = Random::get_float(0.5f, 2.5f);
				Vector3 newDir = (turnAround? -0.3f : 1.0f) * imo->get_presence()->get_velocity_linear() + Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-0.2f, -0.2f)).normal() * max(0.5f, imo->get_presence()->get_velocity_linear().length());
				newDir.z = Random::get_float(-0.3f, 0.3f);
				newDir.normalise();

				if (! imo->get_presence()->get_in_room()->get_volumetric_limit().is_empty())
				{
					Vector3 insideDir;
					if (!imo->get_presence()->get_in_room()->get_volumetric_limit().does_contain_inside(imo->get_presence()->get_placement().get_translation(), &insideDir))
					{
						newDir = insideDir;
					}
				}
				{
					::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
					Name usingGait = NAME(normal);
					locomotion.move_to_3d(imo->get_presence()->get_placement().get_translation() + newDir * 10.0f, 0.0f, Framework::MovementParameters().full_speed().gait(usingGait));
				}
			}
		}
		else
		{
			if (!flyTask.is_running() || timeLeftToNewDirection < 0.0f)
			{
				timeLeftToNewDirection = Random::get_float(5.0f, 20.0f);
				flyTask.stop();
				::Framework::AI::LatentTaskInfoWithParams nextTask;
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to), 0))
				{
					flyPlacement.clear_target();
					flyPlacement.set_owner(imo);
					Framework::DoorInRoom* cameThroughDoor = nullptr;
					Framework::Room* room = imo->get_presence()->get_in_room();
					for_count(int, i, Random::get_int_from_range(2, 6))
					{
						if (room->get_doors().get_size() <= 1 && !cameThroughDoor)
						{
							break;
						}
						ARRAY_STACK(Framework::DoorInRoom*, dirs, room->get_doors().get_size());
						for_every_ptr(dir, room->get_doors())
						{
							if (dir != cameThroughDoor &&
								dir->is_visible())
							{
								dirs.push_back(dir);
							}
						}
						if (dirs.is_empty())
						{
							break;
						}
						Framework::DoorInRoom* throughDoor = dirs[Random::get_int(dirs.get_size())];
						flyPlacement.push_through_door(throughDoor);
						room = throughDoor->get_world_active_room_on_other_side();
						cameThroughDoor = throughDoor->get_door_on_other_side();
					}
					if (cameThroughDoor)
					{
						flyPlacement.set_placement_in_final_room(cameThroughDoor->get_placement().to_world(Transform(Vector3(0.0f, -0.5f, 0.0f), Quat::identity)));
					}
					ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &flyPlacement);
					ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // fly just to room
				}
				flyTask.start_latent_task(mind, nextTask);
			}
		}
		LATENT_WAIT(Random::get_float(0.2f, 0.6f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Stinger::go_investigate)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("go investigate"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(float, timeLeftToNewDirection);

	auto* imo = mind->get_owner_as_modules_owner();

	timeLeftToNewDirection -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeLeftToNewDirection = 0.0f;

	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
		locomotion.turn_in_movement_direction_2d();
	}

	while (investigate.is_valid() && (! enemyPlacement.is_active() || ! enemyPlacement.get_target()))
	{
		if (investigate.inRoom == imo->get_presence()->get_in_room())
		{
			float distance = (investigate.placement.get_translation() - imo->get_presence()->get_centre_of_presence_WS()).length();
			::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
			locomotion.move_to_3d(investigate.placement.get_translation(), 0.2f, Framework::MovementParameters().relative_speed(distance > 5.0f? 1.0f : 0.5f));
			locomotion.turn_towards_2d(investigate.placement.get_translation(), 5.0f);
		}
		else
		{
			LATENT_BREAK();
		}
		LATENT_WAIT(Random::get_float(0.2f, 0.6f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(fly_to_enemy_in_same_room)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("follow enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(float, updateTime);

	auto* imo = mind->get_owner_as_modules_owner();
#ifdef AN_USE_AI_LOG
	auto* logic = mind->get_logic();
#endif

	LATENT_BEGIN_CODE();

	ai_log(logic, TXT("follow enemy"));

	while (true)
	{
		updateTime = Random::get_float(0.1f, 0.3f);
		{
			{
				if (mind && enemyPlacement.is_active() && !enemyPlacement.get_through_door())
				{
					::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
					{
						auto * presence = imo->get_presence();
						Vector3 currentLocation = presence->get_centre_of_presence_WS();
						updateTime = Random::get_float(0.4f, 1.2f);

						Vector3 targetLocation = AI::Targetting::get_enemy_centre_of_presence_in_owner_room(imo);
						Vector3 navPoint = targetLocation;
						// avoid flying in straight lines
						if ((navPoint - currentLocation).length_2d() > 2.0f)
						{
							Vector3 toNavPoint = (navPoint - currentLocation).normal();
							Vector3 offset = Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f));
							offset = offset.drop_using_normalised(toNavPoint).normal();
							offset *= (navPoint - currentLocation).length() * Random::get_float(0.0f, 0.3f);
							navPoint += offset;
						}

						float radius = (currentLocation - targetLocation).length();

						Name usingGait = radius > 9.5f ? NAME(fast) : NAME(normal);

						// we will be exiting this movement when too close
						locomotion.move_to_3d(navPoint, 0.5f, Framework::MovementParameters().full_speed().gait(usingGait));
						locomotion.dont_keep_distance();
						locomotion.turn_in_movement_direction_2d();
					}
				}
			}
		}

		LATENT_WAIT(updateTime);

		if (!enemyPlacement.is_active())
		{
			LATENT_BREAK();
		}
	}

	LATENT_ON_BREAK();
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

enum WhatToDoBeAroundEnemy
{
	Wait,
	Circle
};
LATENT_FUNCTION(Stinger::be_around_enemy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("be around enemy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(int, whatToDo);
	LATENT_VAR(float, updateTime);
	LATENT_VAR(float, timeLeft);

	LATENT_VAR(float, goToSide);
	LATENT_VAR(int, beReallyClose);

	auto* imo = mind->get_owner_as_modules_owner();
#ifdef AN_USE_AI_LOG
	auto* logic = mind->get_logic();
#endif
	auto* self = fast_cast<Stinger>(mind->get_logic());

	timeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	ai_log(logic, TXT("be around enemy"));

	timeLeft = -1.0f;

	while (true)
	{
		updateTime = Random::get_float(0.1f, 0.3f);
		{
			bool startNow = false;
			if (timeLeft < 0.0f)
			{
				startNow = true;
				if (whatToDo == WhatToDoBeAroundEnemy::Wait)
				{
					whatToDo = WhatToDoBeAroundEnemy::Circle;
				}
				else
				{
					whatToDo = WhatToDoBeAroundEnemy::Wait;
				}
			}

			if (whatToDo == WhatToDoBeAroundEnemy::Wait)
			{
				if (startNow)
				{
					timeLeft = Random::get_float(0.5f, 2.0f);
					ai_log(logic, TXT("wait"));
				}
				::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
				locomotion.stop();

				auto * presence = imo->get_presence();
				Vector3 currentLocation = presence->get_centre_of_presence_WS();
				Vector3 targetLocation = (self->should_stay_in_room() && enemyPlacement.get_through_door())? enemyPlacement.get_through_door()->get_hole_centre_WS() : AI::Targetting::get_enemy_centre_of_presence_in_owner_room(imo);
				float radius = (currentLocation - targetLocation).length();

				if (enemyPlacement.get_target())
				{
					locomotion.keep_distance_2d(enemyPlacement.get_target(), max(0.5f, radius - 0.2f), min(3.0f, radius + 0.5f), 0.5f);
					locomotion.turn_towards_2d(enemyPlacement, 1.0f);
				}
				else
				{
					locomotion.keep_distance_2d(targetLocation, max(0.5f, radius - 0.2f), min(3.0f, radius + 0.5f), 0.5f);
					locomotion.turn_towards_2d(targetLocation, 1.0f);
				}
			}
			else if (whatToDo == WhatToDoBeAroundEnemy::Circle)
			{
				if (startNow)
				{
					goToSide = Random::get_chance(0.5f) ? 1.0f : -1.0f;
					beReallyClose = Random::get_chance(0.25f);
					if (beReallyClose)
					{
						timeLeft = Random::get_float(1.0f, 6.0f);
						ai_log(logic, TXT("be really close"));
					}
					else
					{
						timeLeft = Random::get_float(0.5f, 2.0f);
						ai_log(logic, TXT("circle %S"), goToSide > 0.0f ? TXT("right") : TXT("left"));
					}
				}
				if (mind && enemyPlacement.is_active())
				{
					::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
					{
						auto * presence = imo->get_presence();
						Vector3 currentLocation = presence->get_centre_of_presence_WS();
						updateTime = 0.5f;

						bool targetDoor = (self->should_stay_in_room() && enemyPlacement.get_through_door());

						Vector3 targetLocation = targetDoor? enemyPlacement.get_through_door()->get_hole_centre_WS() : AI::Targetting::get_enemy_centre_of_presence_in_owner_room(imo);
						float radius = (currentLocation - targetLocation).length();
						Vector3 towardsTarget = targetLocation - currentLocation;
						Vector3 upDir = presence->get_environment_up_dir();
						Vector3 towardsTargetFlat = towardsTarget.drop_using(upDir);
						Vector3 rightDir = Vector3::cross(towardsTargetFlat, upDir);
						Vector3 navPoint = currentLocation + rightDir * 2.0f * goToSide;
						navPoint += upDir * Random::get_float(-1.5f, 1.5f);

						Name usingGait = radius > 2.5f? NAME(normal) : NAME(keepClose);

						if (beReallyClose)
						{
							if (!targetDoor && enemyPlacement.get_target())
							{
								locomotion.move_to_3d(enemyPlacement.get_target(), 0.5f, Framework::MovementParameters().full_speed().gait(usingGait));
								locomotion.keep_distance_2d(enemyPlacement.get_target(), 0.4f, 0.7f, 0.5f);
							}
							else
							{
								locomotion.move_to_3d(targetLocation, 0.5f, Framework::MovementParameters().full_speed().gait(usingGait));
								locomotion.keep_distance_2d(targetLocation, 0.4f, 0.7f, 0.5f);
							}
						}
						else
						{
							locomotion.move_to_3d(navPoint, 0.4f, Framework::MovementParameters().full_speed().gait(usingGait));
							if (!targetDoor && enemyPlacement.get_target())
							{
								locomotion.keep_distance_2d(enemyPlacement.get_target(), 1.0f, 2.0f, 0.5f);
							}
							else
							{
								locomotion.keep_distance_2d(targetLocation, 1.0f, 2.0f, 0.5f);
							}
						}

						if (!targetDoor && enemyPlacement.get_target())
						{
							locomotion.turn_towards_2d(enemyPlacement, 1.0f);
						}
						else
						{
							locomotion.turn_towards_2d(targetLocation, 1.0f);
						}
					}
				}
				else
				{
					LATENT_BREAK();
				}
			}
		}

		LATENT_WAIT(updateTime);

		if (!enemyPlacement.is_active())
		{
			LATENT_BREAK();
		}
	}

	LATENT_ON_BREAK();
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Stinger::attack_enemy)
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

	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);

	auto* imo = mind->get_owner_as_modules_owner();
#ifdef AN_USE_AI_LOG
	auto* logic = mind->get_logic();
#endif
	auto* self = fast_cast<Stinger>(mind->get_logic());

	timeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

	if (!lightningSpawner)
	{
		LATENT_BREAK();
	}

	ai_log(logic, TXT("attack enemy"));

	if (lightningSpawner)
	{
		lightningSpawner->start(NAME(charging));
	}
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

	LATENT_WAIT(0.5f);

	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.move_to_3d(enemyPlacement.get_target(), 0.5f, Framework::MovementParameters().full_speed().gait(NAME(keepClose)));
		locomotion.turn_towards_2d(enemyPlacement, 5.0f);
		locomotion.keep_distance_2d(enemyPlacement.get_target(), Random::get_float(0.5f, 1.0f), 1.2f, 0.1f);
	}

	// initial telegraph
	timeLeft = 2.0f;
	while (timeLeft > 0.0f)
	{
		LATENT_YIELD();
	}

	// extra time if we're to far
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

	if (Actions::do_lightning_strike(mind, enemyPlacement, magic_number 1.8f, Actions::LightningStrikeParams().discharge_damage(self->dischargeDamage).ignore_violence_disallowed(ignoreViolenceDisallowed)))
	{
		// push back
		imo->get_presence()->add_velocity_impulse(imo->get_presence()->get_placement().vector_to_world(Vector3(Random::get_float(-0.2f, 0.2f), Random::get_float(-1.6f, -1.2f), Random::get_float(0.9f, 1.2f))));
		imo->get_presence()->add_orientation_impulse(Rotator3((360.0f / 0.4f) * 1.1f, Random::get_float(-90.0f, 90.0f), Random::get_float(-90.0f, 90.0f)));
		//
		// push into thrown
		imo->activate_movement(NAME(thrown));
		self->beingThrownTimeLeft = 0.4f;
	}


	LATENT_ON_BREAK();
	//
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_ON_END();
	//
	if (lightningSpawner)
	{
		lightningSpawner->stop(NAME(charging));
	}
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

enum MouthState
{
	Normal,
	Agitated,
	HeldToSpit
};
LATENT_FUNCTION(Stinger::manage_mouth)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("manage mouth"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(MouthState, mouthState);
	LATENT_VAR(MouthState, prevMouthState);
	LATENT_VAR(bool, mouthOpen);
	LATENT_VAR(float, mouthChangeTimeLeft);

	LATENT_VAR(float, blockSpitting);
	LATENT_VAR(float, timeToAutoSpitDischarge);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Stinger>(mind->get_logic());

	mouthChangeTimeLeft -= LATENT_DELTA_TIME;

	blockSpitting = max(0.0f, blockSpitting - LATENT_DELTA_TIME);
	timeToAutoSpitDischarge = max(0.0f, timeToAutoSpitDischarge - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	mouthState = MouthState::Normal;
	mouthOpen = false;
	mouthChangeTimeLeft = 0.0f;
	blockSpitting = 0.0f;
	timeToAutoSpitDischarge = 0.0f;

	while (true)
	{
		if (mouthChangeTimeLeft <= 0.0f || prevMouthState != mouthState)
		{
			prevMouthState = mouthState;
			mouthOpen = !mouthOpen;
			if (mouthState == MouthState::Normal)
			{
				if (mouthOpen)
				{
					mouthChangeTimeLeft = Random::get_float(0.2f, 0.6f);
					self->mouthOpen.blend_to(Random::get_float(0.2f, 0.7f), mouthChangeTimeLeft * 0.5f);
				}
				else
				{
					mouthChangeTimeLeft = Random::get_float(0.4f, 3.6f);
					self->mouthOpen.blend_to(0.0f, Random::get_float(0.1f, 0.3f));
				}
			}
			if (mouthState == MouthState::Agitated)
			{
				mouthChangeTimeLeft = Random::get_float(0.15f, 0.4f);
				self->mouthOpen.blend_to(mouthOpen? Random::get_float(0.5f, 0.9f) : 0.0f, mouthChangeTimeLeft * Random::get_float(0.1f, 0.4f));
			}
			if (mouthState == MouthState::HeldToSpit)
			{
				mouthChangeTimeLeft = Random::get_float(0.1f, 0.2f);
				self->mouthOpen.blend_to(mouthOpen ? Random::get_float(0.8f, 1.0f) : Random::get_float(0.3f, 0.6f), mouthChangeTimeLeft * Random::get_float(0.2f, 0.3f));
			}
		}
		mouthState = self->isAttacking? MouthState::Agitated : MouthState::Normal;
		if (self->beingHeld && self->spitEnergySocket.is_valid())
		{
			if (auto* attached = imo->get_presence()->get_attached_to())
			{
				if (auto* top = attached->get_top_instigator())
				{
					if (auto* pilgrim = top->get_gameplay_as<ModulePilgrim>())
					{
						if (auto* h = imo->get_custom<CustomModules::Health>())
						{
							if (h->get_health() > Energy(0))
							{
								Transform energyInhaleWS = top->get_presence()->get_placement().to_world(top->get_appearance()->calculate_socket_os(pilgrim->get_energy_inhale_socket().get_index()));
								Transform spitEnergyWS = imo->get_presence()->get_placement().to_world(imo->get_appearance()->calculate_socket_os(self->spitEnergySocket.get_index()));

								bool canCheck = true;
								Transform top2imo = Transform::identity;
								if (imo->get_presence()->get_in_room() != top->get_presence()->get_in_room())
								{
									canCheck = false;
									Framework::PresencePath pp;
									pp.be_temporary_snapshot();
									if (pp.find_path(imo, top, false, 3)) // there shouldn't be more than 3 doors on the way
									{
										canCheck = true;
										energyInhaleWS = pp.from_target_to_owner(energyInhaleWS);
									}
								}
								if (canCheck)
								{
									float dist = (energyInhaleWS.get_translation() - spitEnergyWS.get_translation()).length();
									if (dist < 0.2f)
									{
										mouthState = MouthState::HeldToSpit;
									}
									else
									{
										mouthState = MouthState::Agitated;
									}
									if (dist < 0.05f &&
										Vector3::dot(energyInhaleWS.get_axis(Axis::Y), spitEnergyWS.get_axis(Axis::Y)) < -0.8f)
									{
										if (blockSpitting <= 0.0f)
										{
											blockSpitting = Random::get(self->spitEnergyInterval);

											SafePtr<Framework::IModulesOwner> imoSafe;
											imoSafe = imo;
											SafePtr<Framework::IModulesOwner> pilgrimIMOSafe;
											pilgrimIMOSafe = pilgrim->get_owner();

											auto spitEnergyAmount = self->spitEnergyAmount;
											auto spitEnergyAmountMul = self->spitEnergyAmountMul;
											Framework::Game::get()->add_immediate_sync_world_job(TXT("spit energy"),
												[imoSafe, pilgrimIMOSafe, spitEnergyAmount, spitEnergyAmountMul]()
												{
													if (!imoSafe.get() ||
														!pilgrimIMOSafe.get())
													{
														return;
													}
													auto* imo = imoSafe.get();
													auto* pilgrimIMO = pilgrimIMOSafe.get();
													if (auto* pilgrimHealth = pilgrimIMO->get_custom<CustomModules::Health>())
													{
														Energy maxToTake = pilgrimHealth->get_max_total_health() - pilgrimHealth->get_total_health();

														if (maxToTake.is_positive())
														{
															Energy energyToGive = Energy(0);
															Energy energyToUse = Energy(0);

															// calculate how much energy are we going to give and use
															{
																EnergyCoef energyMul = EnergyCoef(1);
																if (auto* h = imo->get_custom<CustomModules::Health>())
																{
																	energyToUse = min(spitEnergyAmount.get_random(), h->get_health());
																	energyMul = spitEnergyAmountMul;
																}
																if (energyMul.is_positive())
																{
																	energyToGive = energyToUse.adjusted(energyMul);
																	if (energyToGive > maxToTake)
																	{
																		energyToGive = maxToTake;
																		energyToUse = energyToGive.adjusted_inv(energyMul);
																	}
																}
																else
																{
																	energyToUse = Energy(0);
																}
															}

															if (energyToGive.is_positive())
															{
																// give to pilgrim
																if (pilgrimHealth)
																{
																	EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
																	etr.energyRequested = energyToGive;
																	pilgrimHealth->handle_health_energy_transfer_request(etr);

																	if (auto* to = pilgrimIMO->get_temporary_objects())
																	{
																		to->spawn_all(NAME(healthInhaled));
																	}
																}

																// damage us
																{
																	if (auto* s = imo->get_sound())
																	{
																		s->play_sound(NAME(spit));
																	}
																	if (auto* h = imo->get_custom<CustomModules::Health>())
																	{
																		Damage damage;
																		damage.damage = energyToUse;
																		DamageInfo damageInfo;
																		damageInfo.damager = imo;
																		damageInfo.source = imo;
																		damageInfo.instigator = imo;
																		// don't adjust_damage_on_hit_with_extra_effects
																		h->deal_damage(damage, damage, damageInfo);
																	}
																}

															}
														}
													}
												});
										}
									}
									else
									{
										if (timeToAutoSpitDischarge <= 0.0f && h->get_health().is_positive())
										{
											timeToAutoSpitDischarge = Random::get(self->spitEnergyDischargeInterval);
											if (auto* to = imo->get_temporary_objects())
											{
												to->spawn_all(NAME(spitDischarge));
											}
										}
									}
								}
							}
						}
					}
					else
					{
						timeToAutoSpitDischarge = 0.0f;
					}
				}
			}
		}
		else
		{
			timeToAutoSpitDischarge = 0.0f;
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

enum BehaveTP // tasks priority
{
	FlyAround = 0,
	FlyAroundEnemy = 0, // same as fly around to allow fly around to take over, when enemy is not there
	Evade = 10,
	Attack = 20,
	GetBackToStayInRoom = 100
};
LATENT_FUNCTION(Stinger::behave)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("behave"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement);
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(Optional<float>, updateTime);

	LATENT_VAR(float, attackChanceCooldown);

	LATENT_VAR(float, timeSinceEnemyInSameRoom);
	LATENT_VAR(float, timeSinceEnemyInSameRoomThreshold);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Stinger>(mind->get_logic());

	if (!currentTask.is_running(attack_enemy))
	{
		attackChanceCooldown -= LATENT_DELTA_TIME;
	}

	timeSinceEnemyInSameRoom += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	attackChanceCooldown = Random::get_float(1.0f, 8.0f);
	timeSinceEnemyInSameRoomThreshold = Random::get_float(7.0f, 20.0f);

	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->start(NAME(idle));
	}
	if (auto * ms = imo->get_sound())
	{
		ms->play_sound(NAME(engine));
	}

	while (true)
	{
		{
			auto * imo = mind->get_owner_as_modules_owner();

			updateTime.clear();

			::Framework::AI::LatentTaskInfoWithParams nextTask;
			// choose best action for now
			nextTask.propose(AI_LATENT_TASK_FUNCTION(fly_freely), BehaveTP::FlyAround);
			if (self->get_to_stay_in_room().is_active() &&
				self->get_to_stay_in_room().get_through_door())
			{
				if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to), BehaveTP::GetBackToStayInRoom))
				{
					ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &self->get_to_stay_in_room());
					ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // fly just to room
				}
			}
			if (enemyPlacement.is_active() &&
				!enemyPlacement.get_through_door() &&
				enemyPlacement.get_target())
			{
				// has to point at actual target
				timeSinceEnemyInSameRoom = 0.0f;
			}
			if (investigate.is_valid() &&
				investigate.inRoom == imo->get_presence()->get_in_room())
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(go_investigate), BehaveTP::FlyAround);
			}
			if (enemyPlacement.is_active())
			{
				if (timeSinceEnemyInSameRoom >= timeSinceEnemyInSameRoomThreshold &&
					((self->should_stay_in_room() && enemyPlacement.get_through_door()) ||
					  !enemyPlacement.get_target()))
				{
					// enemy is long gone, we either don't know where it is or we can't do a thing about it
				}
				else
				{
					if (enemyPlacement.get_through_door())
					{
						if (self->should_stay_in_room())
						{
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(be_around_enemy), BehaveTP::FlyAroundEnemy))
							{
								updateTime.clear();
							}
						}
						else
						{
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::fly_to), BehaveTP::FlyAroundEnemy))
							{
								ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &enemyPlacement);
								ADD_LATENT_TASK_INFO_PARAM(nextTask, bool, true); // fly just to room
							}
						}
					}
					else
					{
						if (enemyPlacement.calculate_string_pulled_distance() < 3.0f)
						{
							if (attackChanceCooldown < 0.0f)
							{
								attackChanceCooldown = Random::get_float(1.0f, 8.0f);
								if (enemyPlacement.get_target() && self->is_ai_aggressive() &&
									(!GameDirector::is_violence_disallowed() || ignoreViolenceDisallowed))
								{
									if (nextTask.propose(AI_LATENT_TASK_FUNCTION(attack_enemy), BehaveTP::Attack))
									{
										updateTime = Random::get_float(1.0f, 2.0f);
									}
								}
							}
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(be_around_enemy), BehaveTP::FlyAroundEnemy))
							{
								updateTime.clear();
							}
						}
						else
						{
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(fly_to_enemy_in_same_room), BehaveTP::FlyAroundEnemy))
							{
								updateTime.clear();
							}
						}
					}
				}
				if (Tasks::is_aiming_at_me(enemyPlacement, imo, 0.2f, 10.0f, 6.0f))
				{
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::evade_aiming_3d), BehaveTP::Evade, NP, false))
					{
						ai_log(self, TXT("evade!"));
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &enemyPlacement); //evadeObject
						updateTime = Random::get_float(1.0f, 2.0f);
					}
				}
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

		LATENT_WAIT(updateTime.get(Random::get_float(0.2f, 0.5f)));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	if (auto * ls = imo->get_custom<::Framework::CustomModules::LightningSpawner>())
	{
		ls->stop(NAME(idle));
	}
	if (auto * ms = imo->get_sound())
	{
		ms->stop_sound(NAME(engine));
	}
	self->isAttacking = false;

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Stinger::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR(PerceptionPause, perceptionPaused);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, mouthTask);

	LATENT_VAR(::Framework::RelativeToPresencePlacement, toStart);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	
	LATENT_VAR(bool, energyDrained);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Stinger>(logic);

	LATENT_BEGIN_CODE();

	energyDrained = true; // to start!

	self->mouthOpen.set_variable<float>(imo, self->data->mouthOpenVar);

	if (self->data->spitEnergySocket.is_set())
	{
		self->spitEnergySocket.set_name(self->data->spitEnergySocket.get(imo->access_variables()));
		self->spitEnergyInterval = self->data->spitEnergyInterval.get(imo->access_variables());;
		self->spitEnergyAmount = self->data->spitEnergyAmount.get(imo->access_variables());
		self->spitEnergyAmountMul = self->data->spitEnergyAmountMul.get(imo->access_variables());
		self->spitEnergyDischargeInterval = self->data->spitEnergyDischargeInterval.get(imo->access_variables());;

		self->spitEnergySocket.look_up(imo->get_appearance()->get_mesh());
	}

	self->dischargeDamage = imo->get_variables().get_value(NAME(dischargeDamage), Energy(magic_number 0.5f));

	self->asPickup = imo->get_custom<CustomModules::Pickup>();

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::perception));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	messageHandler.use_with(mind);
	{
	}

	{	// where are we?
		auto * imo = mind->get_owner_as_modules_owner();
		auto * presence = imo->get_presence();
		toStart.find_path(imo, presence->get_in_room(), presence->get_placement());
	}
	
	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		if (auto* h = imo->get_custom<CustomModules::Health>())
		{
			if (h->get_health() == Energy(0))
			{
				if (!energyDrained)
				{
					perceptionPaused.pause(NAME(energyDrained));
					energyDrained = true;
					mouthTask.stop();
					h->appear_dead(true);
				}
			}
			else if (h->get_health() > Energy(0))
			{
				if (energyDrained)
				{
					perceptionPaused.unpause(NAME(energyDrained));
					energyDrained = false;
					{
						::Framework::AI::LatentTaskInfoWithParams nextTask;
						nextTask.propose(AI_LATENT_TASK_FUNCTION(manage_mouth));
						mouthTask.start_latent_task(mind, nextTask);
					}
					h->appear_dead(false);
				}
			}
		}
		{
			self->mouthOpen.update(LATENT_DELTA_TIME);
		}
		if (!energyDrained)
		{
			// stop being thrown after a while and go back into normal movement
			if (currentTask.is_running(Tasks::being_thrown) ||
				currentTask.is_running(Tasks::being_thrown_and_stay))
			{
				if (self->beingThrownTimeLeft == 0.0f)
				{
					self->beingThrownTimeLeft = Random::get_float(0.6f, 1.0f);
				}
				self->beingThrownTimeLeft -= LATENT_DELTA_TIME;
				if (self->beingThrownTimeLeft < 0.0f)
				{
					self->beingThrownTimeLeft = 0.0f;
					currentTask.stop();
				}
			}
		}
		{
			// choose best action for now
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			bool specialTask = false;

			nextTask.propose(AI_LATENT_TASK_FUNCTION(behave));
			if (ShouldTask::being_held(self->asPickup))
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_held), 20, NP, NP, true);
				specialTask = true;
			}
			else if (energyDrained)
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_thrown_and_stay), 20, NP, NP, true);
				specialTask = true;
			}
			else if (ShouldTask::being_thrown(imo)) // when energy was drained, we default to being thrown
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::being_thrown), 20, NP, NP, true);
				specialTask = true;
			}

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
		{
			self->beingHeld = currentTask.is_running(Tasks::being_held);
		}
		// clear timer afterwards to allow external things to trigger being thrown
		if (! currentTask.is_running(Tasks::being_thrown) &&
			! currentTask.is_running(Tasks::being_thrown_and_stay))
		{
			self->beingThrownTimeLeft = 0.0f;
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

//

REGISTER_FOR_FAST_CAST(StingerData);

StingerData::StingerData()
{
}

StingerData::~StingerData()
{
}

bool StingerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= mouthOpenVar.load_from_xml(_node, TXT("mouthOpenVarID"));

	result &= spitEnergySocket.load_from_xml(_node, TXT("spitEnergySocket"));
	result &= spitEnergyInterval.load_from_xml(_node, TXT("spitEnergyInterval"));
	result &= spitEnergyAmount.load_from_xml(_node, TXT("spitEnergyAmount"));
	result &= spitEnergyAmountMul.load_from_xml(_node, TXT("spitEnergyAmountMul"));
	result &= spitEnergyDischargeInterval.load_from_xml(_node, TXT("spitEnergyDischargeInterval"));

	return result;
}

bool StingerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
