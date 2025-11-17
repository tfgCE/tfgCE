#include "aiLogic_inspecter.h"

#include "components\aiLogicComponent_confussion.h"

#include "tasks\aiLogicTask_beingHeld.h"
#include "tasks\aiLogicTask_beingThrown.h"
#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_projectileHit.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\perceptionRequests\aipr_FindClosest.h"
#include "..\perceptionRequests\aipr_FindInRoom.h"
#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\gameplay\equipment\me_armable.h"
#include "..\..\modules\gameplay\equipment\me_mine.h"

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
DEFINE_STATIC_NAME(lootObject);

// movement names
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(thrown);

// cooldowns
DEFINE_STATIC_NAME(changeEnemy);
DEFINE_STATIC_NAME(allowPath);
DEFINE_STATIC_NAME(forceRedoPath);

// gaits
DEFINE_STATIC_NAME(armed);

// room tags
DEFINE_STATIC_NAME(largeRoom);

// emissives
DEFINE_STATIC_NAME(scared);
DEFINE_STATIC_NAME_STR(armedSeeking, TXT("armed seeking"));

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

REGISTER_FOR_FAST_CAST(Inspecter);

Inspecter::Inspecter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Inspecter::~Inspecter()
{
}

void Inspecter::advance(float _deltaTime)
{
	if (!is_armed())
	{
		set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));
	}

	base::advance(_deltaTime);
}

bool Inspecter::is_armed() const
{
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* armable = imo->get_gameplay_as<ModuleEquipments::Armable>())
		{
			if (armable->is_armed())
			{
				return true;
			}
		}
	}
	return false;
}

Framework::IModulesOwner* Inspecter::get_armed_by() const
{
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* armable = imo->get_gameplay_as<ModuleEquipments::Armable>())
		{
			return armable->get_armed_by();
		}
	}
	return nullptr;
}

void Inspecter::update_appear_seeking()
{
	bool shouldAppearSeeking = seeking || seekingArmed;
	if (appearSeeking != shouldAppearSeeking)
	{
		appearSeeking = shouldAppearSeeking;
		if (auto* imo = get_mind()->get_owner_as_modules_owner())
		{
			if (auto* ec = imo->get_custom<CustomModules::EmissiveControl>())
			{
				if (appearSeeking)
				{
					ec->emissive_activate(NAME(armedSeeking));
				}
				else
				{
					ec->emissive_deactivate(NAME(armedSeeking));
				}
			}
		}
	}
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

	LATENT_VAR(int, action);
	LATENT_VAR(float, timeLeft);
	LATENT_VAR(float, lookAtTimeLeft);
	LATENT_VAR(Vector3, turnTarget);
	LATENT_VAR(Vector3, moveDir);
	LATENT_VAR(float, moveRelativeSpeed);

	timeLeft -= LATENT_DELTA_TIME;
	lookAtTimeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeLeft = 0.5f;
	lookAtTimeLeft = 0.0f;

	set_flag(*lookAtBlocked, LookAtBlockedFlag::Wandering);

	enum Action
	{
		NoAction,
		Turn,
		Walk,
		ACTION_NUM
	};

	action = NoAction;
	while (true)
	{
		if (timeLeft <= 0.0f)
		{
			timeLeft = Random::get_float(1.0f, 5.0f);
			int lastAction = action;
			while (action == lastAction)
			{
				action = Random::get_int(ACTION_NUM);
			}
			if (action == NoAction)
			{
			}
			if (action == Walk)
			{
				moveDir = Vector3::yAxis;
				if (Random::get_chance(0.3f))
				{
					moveRelativeSpeed = clamp(Random::get_float(0.4f, 0.8f), 0.0f, 1.0f);
				}
				else
				{
					moveRelativeSpeed = clamp(Random::get_float(0.8f, 1.1f), 0.0f, 1.0f);
				}
				if (Random::get_chance(0.3f))
				{
					moveDir.x = (Random::get_chance(0.5f) ? -1.0f : 1.0f) * Random::get_float(0.05f, 0.3f);
				}
				moveRelativeSpeed = clamp(Random::get_float(0.4f, 1.3f), 0.0f, 1.0f);
			}
			if (action == Turn)
			{
				Transform placement = mind->get_owner_as_modules_owner()->get_presence()->get_placement();
				turnTarget = placement.location_to_world(Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), 0.0f).normal() * 3.0f);
			}
		}
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
		if (mind)
		{
			::Framework::AI::Locomotion & locomotion = mind->access_locomotion();

			if (action == NoAction)
			{
				locomotion.stop();
			}
			if (action == Turn)
			{
				locomotion.stop();
				locomotion.turn_towards_3d(turnTarget, 10.0f);
			}
			if (action == Walk)
			{
				locomotion.stop();
				locomotion.turn_in_movement_direction_3d();
				Transform placement = mind->get_owner_as_modules_owner()->get_presence()->get_placement();
				locomotion.move_to_3d(placement.location_to_world(moveDir), 0.0f, Framework::MovementParameters().relative_speed(moveRelativeSpeed));
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//
	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}
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

LATENT_FUNCTION(Inspecter::act_armed)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("act armed"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM(::Framework::PresencePath*, enemyPath);
	LATENT_PARAM_NOT_USED(int*, lookAtBlocked);
	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<::Framework::IModulesOwner>, forEnemy);

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);

	LATENT_VAR(bool, redoPath);
	LATENT_VAR(bool, newPath);
	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Inspecter>(mind->get_logic());

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	forEnemy = nullptr;
	redoPath = true;
	newPath = false;

	LATENT_WAIT(Random::get_float(0.1f, 0.2f));

	while (true)
	{
		if (auto* t = enemyPath->get_target())
		{
			Vector3 enemyLoc = enemyPath->get_placement_in_owner_room().location_to_world(t->get_presence()->get_centre_of_presence_os().get_translation());
			Transform weAt = imo->get_presence()->get_placement();
			Vector3 enemyRel = weAt.location_to_local(enemyLoc);
			if (enemyRel.length_squared_2d() < sqr(0.3f) &&
				enemyRel.z > 0.5f && enemyRel.z < 3.0f)
			{
				if (auto* mine = imo->get_gameplay_as<ModuleEquipments::Mine>())
				{
					mine->trigger();
				}
			}
		}

		if (enemyPath->get_target() != forEnemy.get())
		{
			forEnemy = enemyPath->get_target();
			redoPath = true;
		}

		{
			auto& locomotion = mind->access_locomotion();
			if (locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
			{
				redoPath = true;
			}
		}

		if (cooldowns.is_available(NAME(forceRedoPath)))
		{
			redoPath = true;
		}

		if (!pathTask.is_set() && redoPath && forEnemy.get() && cooldowns.is_available(NAME(allowPath)))
		{
			Framework::Nav::PlacementAtNode toEnemyNavLoc = mind->access_navigation().find_nav_location(forEnemy.get());

			Framework::Nav::PathRequestInfo pathRequestInfo(imo);
			
			pathTask = mind->access_navigation().find_path_to(toEnemyNavLoc, pathRequestInfo);
		}

		if (pathTask.is_set() && pathTask->is_done())
		{
			if (pathTask->has_succeed())
			{
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					path = task->get_path();
					newPath = true;
				}
				else
				{
					path.clear();
				}
			}
			cooldowns.set(NAME(allowPath), 2.0f);
			cooldowns.set(NAME(forceRedoPath), Random::get_float(4.0f, 7.0f));
			pathTask.clear();
		}

		{
			bool hasInvalidPath = false;
			auto& locomotion = mind->access_locomotion();
			if (! locomotion.check_if_path_is_ok(path.get()))
			{
				locomotion.dont_move();
				hasInvalidPath = true;
			}
			else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
			{
				if (newPath || !locomotion.is_following_path(path.get()))
				{
					locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed().gait(NAME(armed)));
					locomotion.turn_follow_path_2d();
					newPath = false;
				}
			}
			else
			{
				locomotion.dont_move();
			}

			if (self->seekingArmed != hasInvalidPath)
			{
				self->seekingArmed = hasInvalidPath;
				self->update_appear_seeking();
			}
		}
			
		LATENT_WAIT(Random::get_float(0.05f, 0.25f));
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Inspecter::act_normal)
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

LATENT_FUNCTION(Inspecter::execute_logic)
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
	LATENT_VAR(bool, isArmed);
	LATENT_VAR(bool, wasArmed);
	LATENT_VAR(SafePtr<::Framework::IModulesOwner>, armedBy);

	LATENT_VAR(Framework::CooldownsSmall, cooldowns);
	
	LATENT_VAR(Confussion, confussion);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Inspecter>(logic);

	confussion.advance(LATENT_DELTA_TIME);
	
	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	self->asPickup = imo->get_custom<CustomModules::Pickup>();

	confussion.setup(mind);

	lookAtBlocked = 0;

	self->seeking = false;

	isHeldOrThrown = false;

	isArmed = self->is_armed();
	wasArmed = !isArmed;

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &lookAtBlocked);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::PresencePath*, &enemyPath);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	if (auto* imo = mind->get_owner_as_modules_owner())
	{
		if (!imo->get_variables().get_value<bool>(NAME(lootObject), false))
		{
			imo->get_presence()->request_on_spawn_random_dir_teleport(4.5f, 0.3f);
		}
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		isArmed = self->is_armed(); // update
		armedBy = self->get_armed_by();

		{
			auto * imo = mind->get_owner_as_modules_owner();
			// update target via perception requests
			{
				bool seekingNow = false;
				if (!isHeldOrThrown)
				{
					if (wasArmed != isArmed)
					{
						// most likely it is a different enemy
						enemyPath.clear_target();
					}
					if (confussion.is_confused())
					{
						// no enemy while confused
						enemyPath.clear_target();
					}
					else if (!enemyPath.is_active())
					{
						seekingNow = true;
						if (cooldowns.is_available(NAME(changeEnemy)) || wasArmed != isArmed)
						{
							// update social faction
							{
								Optional<Name> armedByFaction;
								if (isArmed && armedBy.get())
								{
									armedByFaction = armedBy->get_ai()->get_mind()->get_social().get_faction();
								}
								if (auto* ai = imo->get_ai())
								{
									auto* mind = ai->get_mind();
									mind->access_social().set_sociopath(armedByFaction.is_set() ? Optional<bool>(false) : NP);
									mind->access_social().set_endearing(armedByFaction.is_set() ? Optional<bool>(false) : NP);
									mind->access_social().set_faction(armedByFaction);
								}
							}
							if (!perceptionRequest.is_set())
							{
								wasArmed = isArmed;
								if (isArmed && armedBy.get())
								{
									bool inLargeRoom = imo->get_presence()->get_in_room()->get_tags().get_tag(NAME(largeRoom)) > 0.0f;
									auto* ai = imo->get_ai();
									perceptionRequest = mind->access_perception().add_request((new AI::PerceptionRequests::FindClosest(imo, 100.0f, 8,
										[ai](Framework::IModulesOwner const* _object, REF_ Optional<Transform> & _offsetOS) { return ai->get_mind()->get_social().is_enemy(_object); }))
										->add_door_penalty(0, inLargeRoom? 20.0f : 0.3f) // to prefer same room
										->add_door_penalty(1, inLargeRoom? 30.0f : 0.3f)
										->add_door_penalty(2, inLargeRoom? 40.0f : 0.3f)
										->only_navigable());
								}
								else
								{
									//perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindInRoom(imo,
									perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindAnywhere(imo,
										[](Framework::IModulesOwner const* _object) { return _object->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>() != nullptr ? 1.0f : 0.0f; }));
								}
							}
						}
					}
				}
			
				if (perceptionRequest.get() && perceptionRequest->is_processed())
				{
					if (auto* request = fast_cast<AI::PerceptionRequests::FindInRoom>(perceptionRequest.get()))
					{
						enemyPath = request->get_path();
					}
					if (auto* request = fast_cast<AI::PerceptionRequests::FindAnywhere>(perceptionRequest.get()))
					{
						enemyPath = request->get_path();
					}
					if (auto* request = fast_cast<AI::PerceptionRequests::FindClosest>(perceptionRequest.get()))
					{
						enemyPath = request->get_path();
					}
					perceptionRequest = nullptr;
					cooldowns.set(NAME(changeEnemy), isArmed ? Random::get_float(3.0f, 6.0f) : Random::get_float(5.0f, 30.0f));
				}

				if (perceptionRequest.get() || ! enemyPath.is_active())
				{
					seekingNow = true;
				}

				if (!isArmed)
				{
					seekingNow = false;
				}

				if (self->seeking != seekingNow)
				{
					self->seeking = seekingNow;
					self->update_appear_seeking();
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
					if (isArmed)
					{
						if (nextTask.propose(AI_LATENT_TASK_FUNCTION(act_armed), 5))
						{
							ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, &enemyPath);
							ADD_LATENT_TASK_INFO_PARAM(nextTask, int*, &lookAtBlocked);
						}
					}
					else
					{
						if (nextTask.propose(AI_LATENT_TASK_FUNCTION(act_normal), 5))
						{
							ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, &enemyPath);
							ADD_LATENT_TASK_INFO_PARAM(nextTask, int*, &lookAtBlocked);
						}
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
