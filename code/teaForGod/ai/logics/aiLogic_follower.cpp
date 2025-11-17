#include "aiLogic_follower.h"

#include "components\aiLogicComponent_confussion.h"

#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_runAway.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\perceptionRequests\aipr_FindAnywhere.h"
#include "..\perceptionRequests\aipr_FindInVisibleRooms.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\tasks\navTask_GetRandomLocation.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomUtils.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
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

DEFINE_STATIC_NAME(evade);
DEFINE_STATIC_NAME(projectileHit);
DEFINE_STATIC_NAME(hit);
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRoomLocation);
DEFINE_STATIC_NAME(inRoomNormal);

DEFINE_STATIC_NAME(follow);
DEFINE_STATIC_NAME(stopAndWait);

// ai message names
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(someoneElseDied);

// ai message params
DEFINE_STATIC_NAME(damageSource);
DEFINE_STATIC_NAME(killer);

// tags
DEFINE_STATIC_NAME(interestedInInterestingDevices);
DEFINE_STATIC_NAME(interestingDevice);

//

namespace WhatToDo
{
	enum Type
	{
		StandStill,
		UsualStuff,
		GreetPilgrim,
		RunAway,
	};
};

//

REGISTER_FOR_FAST_CAST(Follower);

Follower::Follower(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Follower::~Follower()
{
}

void Follower::advance(float _deltaTime)
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

	LATENT_VAR(int, action);
	LATENT_VAR(float, timeLeft);
	LATENT_VAR(Vector3, turnTarget);
	LATENT_VAR(float, moveRelativeSpeed);

	LATENT_VAR(::Framework::AI::PerceptionRequestPtr, perceptionRequest); // one perception should be enough

	LATENT_VAR(SafePtr<::Framework::IModulesOwner>, objectOfInterest);
	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, navTask);
	
	LATENT_VAR(::Framework::Nav::PathPtr, nextPath);

	timeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	timeLeft = 0.5f;

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
			if (objectOfInterest.is_set())
			{
				if (action == Turn)
				{
					objectOfInterest.clear();
				}
				else
				{
					action = Turn;
				}
			}
			if (!objectOfInterest.is_set())
			{
				int lastAction = action;
				while (lastAction == action)
				{
					float ch = Random::get_float(0.0f, 1.0f);
					if (ch < 0.5f) action = Walk; else
					if (ch < 0.8f) action = Turn; else
								   action = NoAction;
					if (action == Walk)
					{
						break;
					}
				}
			}
			if (action == NoAction)
			{
				timeLeft = Random::get_float(1.0f, 2.0f);
				auto & locomotion = mind->access_locomotion();
				locomotion.stop();
			}
			if (action == Walk)
			{
				if (Random::get_chance(0.3f))
				{
					// make it really slow
					moveRelativeSpeed = Random::get_float(0.1f, 0.3f);
				}
				else
				{
					moveRelativeSpeed = Random::get_float(0.4f, 0.6f);
				}

				objectOfInterest.clear();
				nextPath.clear();
				if (Random::get_chance(0.8f) &&
					mind->get_owner_as_modules_owner() &&
					mind->get_owner_as_modules_owner()->get_as_object() &&
					mind->get_owner_as_modules_owner()->get_as_object()->get_tags().get_tag(NAME(interestedInInterestingDevices)))
				{
					while (true)
					{
						if (!perceptionRequest.is_set())
						{
							perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindAnywhere(mind->get_owner_as_modules_owner(),
								[](Framework::IModulesOwner const* _object)
								{
									if (_object)
									{
										if (auto* object = _object->get_as_object())
										{
											if (object->get_tags().get_tag(NAME(interestingDevice)))
											{
												return 1.0f;
											}
										}
									}
									return 0.0f;
								}));
						}
						else if (perceptionRequest->is_processed())
						{
							if (auto * request = fast_cast<AI::PerceptionRequests::FindAnywhere>(perceptionRequest.get()))
							{
								if (auto* found = request->get_result())
								{
									auto* imo = mind->get_owner_as_modules_owner();
									objectOfInterest = found;
									navTask = mind->access_navigation().find_path_to(found, Framework::Nav::PathRequestInfo(imo));
								}
							}
							LATENT_NAV_TASK_WAIT_IF_SUCCEED(navTask)
							{
								if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(navTask.get()))
								{
									nextPath = task->get_path();
								}
							}
							navTask = nullptr;
							perceptionRequest = nullptr;
							break;
						}
						LATENT_YIELD();
					}
				}
				if (! nextPath.is_set()) // walk randomly
				{
					{
						auto* imo = mind->get_owner_as_modules_owner();
						objectOfInterest.clear();
						navTask = mind->access_navigation().get_random_path(Random::get_float(2.0f, 80.0f), NP, Framework::Nav::PathRequestInfo(imo)); // to allow covering carts
					}
					LATENT_NAV_TASK_WAIT_IF_SUCCEED(navTask)
					{
						if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(navTask.get()))
						{
							nextPath = task->get_path();
						}
					}
					navTask = nullptr;
				}

				if (nextPath.is_set())
				{
					path = nextPath;
					auto & locomotion = mind->access_locomotion();
					locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().relative_speed(moveRelativeSpeed));
					locomotion.turn_follow_path_2d();
					locomotion.stop_on_actors();
				}
				else
				{
					auto & locomotion = mind->access_locomotion();
					locomotion.stop();
				}

				timeLeft = Random::get_float(10.0f, 20.0f); // should be enough to allow reaching destination
			}
			if (action == Turn)
			{
				if (objectOfInterest.is_set())
				{
					timeLeft = Random::get_float(2.0f, 5.0f);
				}
				else
				{
					timeLeft = Random::get_float(1.0f, 2.0f);
					Transform placement = mind->get_owner_as_modules_owner()->get_presence()->get_placement();
					turnTarget = placement.location_to_world(Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), 0.0f).normal() * 3.0f);
				}
			}
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
				// we still require some time to finish it (check result from last frmae)
				locomotion.dont_move();
				if (objectOfInterest.is_set())
				{
					locomotion.turn_towards_2d(objectOfInterest.get(), 10.0f);
				}
				else
				{
					if (! locomotion.is_turning() || ! locomotion.has_finished_turn(10.0f))
					{
						locomotion.turn_towards_2d(turnTarget, 10.0f);
						timeLeft = 1.0f; // check back in a while until we finish the turn
					}
				}
			}
			if (action == Walk)
			{
				if (path.is_set() && locomotion.is_done_with_move())
				{
					// we're done!
					timeLeft = 0.0f;
				}
				// locomotion order has been issued earlier
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

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(go_to_pilgrim)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("go to pilgrim"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(SafePtr<::Framework::IModulesOwner>, target);
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	LATENT_VAR(bool, requiresNewPath);

	LATENT_BEGIN_CODE();

	while (true)
	{
		requiresNewPath = false;
		if (!target.get() || !target->get_presence())
		{
			LATENT_BREAK();
		}
		if (!path.is_set())
		{
			requiresNewPath = true;
		}
		else
		{
			if (path->get_path_nodes().get_last().placementAtNode.get_room() != target->get_presence()->get_in_room())
			{
				requiresNewPath = true;
			}
			else
			{
				Vector3 moved = target->get_presence()->get_placement().get_translation() - path->get_path_nodes().get_last().placementAtNode.get_current_placement().get_translation();
				if (moved.length() > 0.3f)
				{
					requiresNewPath = true;
				}
			}
		}
		if (requiresNewPath &&
			!pathTask.is_set())
		{
			auto* imo = mind->get_owner_as_modules_owner();
			pathTask = mind->access_navigation().find_path_to(target.get(), Framework::Nav::PathRequestInfo(imo));
		}
		if (pathTask.is_set() &&
			pathTask->is_done())
		{
			if (pathTask->has_succeed())
			{
				if (auto* findPath = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					path = findPath->get_path();
					auto & locomotion = mind->access_locomotion();
					locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
					locomotion.keep_distance_2d(target.get(), 0.8f);
					locomotion.turn_follow_path_2d();
					locomotion.stop_on_actors();
				}
			}
			else
			{
				auto & locomotion = mind->access_locomotion();
				locomotion.stop();
				locomotion.keep_distance_2d(target.get(), 0.5f);
				locomotion.stop_on_actors();
			}
			pathTask = nullptr;
		}

		{
			auto & locomotion = mind->access_locomotion();
			float dist;
			if (locomotion.calc_distance_to_keep_distance(dist) && dist < 1.6f)
			{
				locomotion.turn_towards_2d(target.get(), 1.4f);
			}
			else
			{
				locomotion.turn_follow_path_2d();
			}
		}

		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(stop_and_wait)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("stop and wait"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_PARAM(::Framework::PresencePath*, targetPath);
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
		locomotion.turn_towards_2d(targetPath->get_target(), 5.0f);
	}

	while (true)
	{
		if (!Tasks::is_aiming_at_me(*targetPath, mind->get_owner_as_modules_owner(), 0.15f, 5.0f, 2.5f) || targetPath->calculate_string_pulled_distance() < 1.0f)
		{
			break;
		}

		LATENT_WAIT(Random::get_float(0.2f, 0.4f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Follower::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(WhatToDo::Type, whatToDo);
	LATENT_VAR(float, whatToDoTimeLeft);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(SafePtr<::Framework::IModulesOwner>, target);
	LATENT_VAR(::Framework::PresencePath, targetPath);
	LATENT_VAR(float, keepBeingInterestedInPilgrim);
	LATENT_VAR(float, blockBeingInterestedInPilgrim);
	LATENT_VAR(float, runAwayFor);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(int, lookAtBlocked);

	LATENT_VAR(int, collisionCount);

	LATENT_VAR(::Framework::AI::PerceptionRequestPtr, perceptionRequest); // one perception should be enough

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(Confussion, confussion);

	confussion.advance(LATENT_DELTA_TIME);

	whatToDoTimeLeft -= LATENT_DELTA_TIME;
	blockBeingInterestedInPilgrim = max(0.0f, blockBeingInterestedInPilgrim - LATENT_DELTA_TIME);
	keepBeingInterestedInPilgrim = max(0.0f, keepBeingInterestedInPilgrim - LATENT_DELTA_TIME);
	runAwayFor = max(0.0f, runAwayFor - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	whatToDo = WhatToDo::StandStill;
	whatToDoTimeLeft = 0.0f;

	blockBeingInterestedInPilgrim = Random::get_float(5.0f, 120.0f);
	keepBeingInterestedInPilgrim = 0.0f;

	confussion.setup(mind);

	target.clear();
	
	lookAtBlocked = 0;

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &lookAtBlocked);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::PresencePath*, &targetPath);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	/*
	while (true)
	{
		// !@# test
		{
			auto & locomotion = mind->access_locomotion();
			locomotion.move_to_2d(mind->get_owner_as_modules_owner()->get_presence()->get_placement().location_to_world(Vector3(2.0f, 5.0f, 0.0f)), 1.0f, Framework::MovementParameters().full_speed());
			locomotion.move_to_2d(mind->get_owner_as_modules_owner()->get_presence()->get_placement().location_to_world(Vector3(0.0f, 50.0f, 0.0f)), 1.0f, Framework::MovementParameters().full_speed());
			locomotion.turn_in_movement_direction_2d();
		}
		LATENT_WAIT(2.0f);
	}
	*/

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(dealtDamage), [mind, &target, &targetPath, &whatToDo, &runAwayFor](Framework::AI::Message const& _message)
			{
				if (auto* source = _message.get_param(NAME(damageSource)))
				{
					MEASURE_PERFORMANCE(dealtDamage);
					if (auto* damageInstigator = source->get_as<SafePtr<Framework::IModulesOwner>>().get())
					{
						whatToDo = WhatToDo::RunAway;
						runAwayFor = Random::get_float(20.0f, 40.0f);
						// run away from damage instigator
						target = damageInstigator;
						targetPath.find_path(mind->get_owner_as_modules_owner(), target.get(), true);
					}
				}
			}
		);
		messageHandler.set(NAME(someoneElseDied), [mind, &target, &targetPath, &whatToDo, &runAwayFor](Framework::AI::Message const& _message)
			{
				if (auto* source = _message.get_param(NAME(killer)))
				{
					MEASURE_PERFORMANCE(someoneElseDied);
					if (auto* damageInstigator = source->get_as<SafePtr<Framework::IModulesOwner>>().get())
					{
						whatToDo = WhatToDo::RunAway;
						runAwayFor = Random::get_float(20.0f, 40.0f);
						// run away from damage instigator
						target = damageInstigator;
						targetPath.find_path(mind->get_owner_as_modules_owner(), target.get(), true);
					}
				}
			}
		);
	}

	while (true)
	{
		{
			auto * imo = mind->get_owner_as_modules_owner();
			auto * presence = imo->get_presence();
			if (confussion.is_confused())
			{
				// no enemy while confused
				target.clear();
				targetPath.clear_target();
				perceptionRequest.clear();
				runAwayFor = 0.0f;
				whatToDo = WhatToDo::StandStill;
				if (whatToDoTimeLeft <= 0.0f)
				{
					whatToDoTimeLeft = Random::get_float(0.5f, 1.0f);
				}
				else
				{
					whatToDoTimeLeft = min(1.0f, whatToDoTimeLeft);
				}
			}
			else if (whatToDo != WhatToDo::RunAway)
			{
				// update target via perception requests
				if (target.get() && keepBeingInterestedInPilgrim == 0.0f)
				{
					blockBeingInterestedInPilgrim = Random::get_float(5.0f, 120.0f);
					perceptionRequest.clear();
				}
				else if (blockBeingInterestedInPilgrim > 0.0f)
				{
					target.clear();
					targetPath.clear_target();
					perceptionRequest.clear();
				}
				else
				{
					if (!target.get())
					{
						if (!perceptionRequest.is_set())
						{
							perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindInVisibleRooms(imo,
								[](Framework::IModulesOwner const* _object) { return _object->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>() != nullptr ? 1.0f : 0.0f; }));
						}
						else if (perceptionRequest->is_processed())
						{
							if (auto* request = fast_cast<AI::PerceptionRequests::FindInVisibleRooms>(perceptionRequest.get()))
							{
								if (request->has_found_anything())
								{
									targetPath = request->get_path_to_best();
									target = targetPath.get_target();
									keepBeingInterestedInPilgrim = Random::get_float(5.0f, 120.0f);
								}
								else
								{
									blockBeingInterestedInPilgrim = Random::get_float(5.0f, 60.0f);
								}
							}
							perceptionRequest = nullptr;
						}
					}
					else
					{
						perceptionRequest.clear();
					}
				}

				/*
				if (auto* mc = imo->get_collision())
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- checkCollisions"));
					MEASURE_PERFORMANCE(checkCollisions);
					for_every(oc, mc->get_collided_with())
					{
						if (Framework::IModulesOwner * object = fast_cast<Framework::IModulesOwner>(oc->collidedWithObject.get()))
						{
							if (object->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>() != nullptr)
							{
								target = object;
								targetPath.find_path(mind->get_owner_as_modules_owner(), target.get(), true);
								blockBeingInterestedInPilgrim = 0.0f;
								keepBeingInterestedInPilgrim = Random::get_float(5.0f, 20.0f);
							}
						}
					}
				}
				*/
			}

			targetPath.update_simplify();

			if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
			{
				// doing scripted behaviour
			}
			else
			{
				if (currentTask.can_start_new())
				{
					// choose best action for now
					::Framework::AI::LatentTaskInfoWithParams nextTask;

					nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle));
					if (whatToDo == WhatToDo::RunAway)
					{
						lookAtBlocked = 1;
						if (runAwayFor <= 0.0f || !target.get())
						{
							whatToDo = WhatToDo::StandStill;
							whatToDoTimeLeft = Random::get_float(0.5f, 1.0f);
							nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle), 5);
							blockBeingInterestedInPilgrim = Random::get_float(5.0f, 500.0f);
							keepBeingInterestedInPilgrim = 0.0f;
							target.clear();
							targetPath.clear_target();
						}
						else
						{
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::run_away), 5))
							{
								ADD_LATENT_TASK_INFO_PARAM(nextTask, SafePtr<::Framework::IModulesOwner>, target);
							}
						}
					}
					else
					{
						if (whatToDo == WhatToDo::StandStill)
						{
							nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle), 5);
							if (whatToDoTimeLeft <= 0.0f)
							{
								whatToDo = WhatToDo::UsualStuff;
							}
						}
						if (target.get() && targetPath.get_target_presence())
						{
							{
								Vector3 targetLoc = targetPath.location_from_target_to_owner(targetPath.get_target_presence()->get_centre_of_presence_WS());
								float dist = targetPath.calculate_string_pulled_distance();
								if (dist <= 0.7f)
								{
									whatToDo = WhatToDo::GreetPilgrim;
									whatToDoTimeLeft = Random::get_float(4.0f, 8.0f);
								}
								else if (whatToDo == WhatToDo::UsualStuff)
								{
									float dot = Vector3::dot(presence->get_placement().get_axis(Axis::Y), targetLoc - presence->get_centre_of_presence_WS());
									if ((whatToDo != WhatToDo::StandStill && whatToDoTimeLeft <= 0.0f) &&
										((dist <= 2.0f && dot > 0.4f) ||
											(dist <= 10.0f && dot > 0.8f)) &&
										targetPath.is_there_clear_line())
									{
										whatToDo = WhatToDo::GreetPilgrim;
										whatToDoTimeLeft = Random::get_float(4.0f, 8.0f);
									}
								}
							}
							if (whatToDo == WhatToDo::GreetPilgrim)
							{
								lookAtBlocked = 0;
								if (nextTask.propose(AI_LATENT_TASK_FUNCTION(go_to_pilgrim), 5))
								{
									ADD_LATENT_TASK_INFO_PARAM(nextTask, SafePtr<::Framework::IModulesOwner>, target);
								}
								if (whatToDoTimeLeft < 0.0f)
								{
									whatToDo = WhatToDo::UsualStuff;
									whatToDoTimeLeft = Random::get_float(3.0f, 12.0f);
								}
							}
						}
						if (whatToDo == WhatToDo::UsualStuff)
						{
							lookAtBlocked = 1;
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(wander_around), 5))
							{
							}
						}
						if (auto * presence = mind->get_owner_as_modules_owner()->get_presence())
						{
							// maybe we have hit something? stop then
							bool collidedWithNonScenery = false;
							if (presence->get_velocity_linear().length() > 0.1f)
							{
								for_every(collidedWith, mind->get_owner_as_modules_owner()->get_collision()->get_collided_with())
								{
									if (auto* obj = fast_cast<Framework::Object>(collidedWith->collidedWithObject.get()))
									{
										if (fast_cast<Framework::Scenery>(obj) == nullptr) // skip scenery
										{
											if (auto *op = obj->get_presence())
											{
												if (op->get_in_room() == presence->get_in_room())
												{
													if (Vector3::dot(presence->get_velocity_linear(), op->get_placement().get_translation() - presence->get_placement().get_translation()) > 0.0f)
													{
														collidedWithNonScenery = true;
														break;
													}
												}
											}
										}
									}
								}
							}
							if (collidedWithNonScenery)
							{
								++collisionCount;
								if (collisionCount >= 2)
								{
									whatToDo = WhatToDo::StandStill;
									whatToDoTimeLeft = Random::get_float(0.5f, 1.0f);
									nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle), 5);
								}
							}
							else
							{
								collisionCount = 0;
							}
						}
						if (Tasks::is_aiming_at_me(targetPath, imo, 0.15f, 5.0f, 2.5f) && targetPath.calculate_string_pulled_distance() >= 1.0f)
						{
							if (nextTask.propose(AI_LATENT_TASK_FUNCTION(stop_and_wait), 10, NP, true))
							{
								ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::PresencePath*, &targetPath);
							}
						}
					}

					// check if we can start new one, if so, start it
					if (currentTask.can_start(nextTask))
					{
						currentTask.start_latent_task(mind, nextTask);
					}
				}
			}
		}

		LATENT_WAIT(Random::get_float(0.05f, 0.25f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	lookAtTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
