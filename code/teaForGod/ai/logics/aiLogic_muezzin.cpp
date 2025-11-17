#include "aiLogic_muezzin.h"

#include "components\aiLogicComponent_confussion.h"

#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_lookAt.h"

#include "..\perceptionRequests\aipr_FindAnywhere.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\tasks\navTask_GetRandomLocation.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(callForPrayerPoint);
DEFINE_STATIC_NAME(horn);
DEFINE_STATIC_NAME(muezzinCall);

DEFINE_STATIC_NAME(damageSource);
DEFINE_STATIC_NAME(dealtDamage);

//

namespace Action
{
	enum Type
	{
		None,
		StandStill,
		Wander,
		Horn,
		LookAtCollider,
		GoBack,
	};
};

//

REGISTER_FOR_FAST_CAST(Muezzin);

Muezzin::Muezzin(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Muezzin::~Muezzin()
{
}

void Muezzin::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

static LATENT_FUNCTION(go_back)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM(::Framework::Room*, startingRoom);
	LATENT_PARAM(Vector3, startingLocation);
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);
	
	LATENT_BEGIN_CODE();

	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	{
		auto* imo = mind->get_owner_as_modules_owner();
		pathTask = mind->access_navigation().find_path_to(startingRoom, startingLocation, Framework::Nav::PathRequestInfo(imo));
	}

	LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
	{
		if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
		{
			path = task->get_path();
		}
	}
	pathTask = nullptr;

	if (path.is_set())
	{
		{
			auto & locomotion = mind->access_locomotion();
			locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().relative_speed(0.8f));
			locomotion.turn_follow_path_2d();
			locomotion.stop_on_actors();
		}

		while (true)
		{
			if (mind->access_locomotion().is_done_with_move())
			{
				break;
			}
			LATENT_WAIT(0.2f);
		}

		{
			auto & locomotion = mind->access_locomotion();
			locomotion.dont_move();
		}
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

static LATENT_FUNCTION(wander)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Transform, prayingPoint);

	LATENT_BEGIN_CODE();

	{	// find praying point and start path there
		auto * imo = mind->get_owner_as_modules_owner();
		auto * presence = imo->get_presence();
		prayingPoint = presence->get_placement();
		prayingPoint.set_orientation(Rotator3(0.0f, Random::get_float(0.0f, 360.0f), 0.0f).to_quat());

		Array<Transform> prayingPoints;
		presence->get_in_room()->for_every_point_of_interest(NAME(callForPrayerPoint),
			[&prayingPoints](Framework::PointOfInterestInstance * _fpoi)
			{
				prayingPoints.push_back(_fpoi->calculate_placement());
			}
		);
		if (!prayingPoints.is_empty())
		{
			prayingPoint = prayingPoints[Random::get_int(prayingPoints.get_size())];
		}

		//pathTask = mind->access_navigation().find_path_to(presence->get_in_room(), prayingPoint.get_translation());
	}

	{
		{
			auto & locomotion = mind->access_locomotion();
			locomotion.move_to_2d(prayingPoint.get_translation(), 0.1f, Framework::MovementParameters().relative_speed(0.8f));
			locomotion.turn_in_movement_direction_2d();
			locomotion.stop_on_actors();
		}

		while (true)
		{
			if (mind->access_locomotion().is_done_with_move())
			{
				break;
			}
			LATENT_WAIT(0.2f);
		}

		{
			auto & locomotion = mind->access_locomotion();
			locomotion.dont_move();
		}
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.2f));

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Muezzin::horn)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM(::Framework::RelativeToPresencePlacement*, lookAtRel);
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::SoundSourcePtr, hornSound);
	LATENT_VAR(Transform, placement);

	LATENT_BEGIN_CODE();

	{
		auto & locomotion = mind->access_locomotion();
		locomotion.dont_move();
	}

	if (auto* presence = mind->get_owner_as_modules_owner()->get_presence())
	{
		placement = presence->get_placement();
		if (auto * room = presence->get_in_room())
		{
			Framework::DoorInRoom* closest = nullptr;
			float closestDist = 0.0f;
			for_every_ptr(door, room->get_all_doors())
			{
				if (!door->is_visible()) continue;
				float dist = (door->get_placement().get_translation() - presence->get_centre_of_presence_WS()).length();
				if (!closest || closestDist > dist)
				{
					closestDist = dist;
					closest = door;
				}
			}
			if (closest)
			{
				Vector3 atDir = closest->get_placement().get_translation() - placement.get_translation();
				atDir = atDir.drop_using(placement.get_axis(Axis::Z));
				placement = look_at_matrix(placement.get_translation(), placement.get_translation() + atDir, placement.get_axis(Axis::Z)).to_transform();
			}
		}
	}

	{
		{
			auto & locomotion = mind->access_locomotion();
			locomotion.turn_towards_2d(placement.location_to_world(Vector3::yAxis * 20.0f), 5.0f);
		}

		while (true)
		{
			if (mind->access_locomotion().has_finished_turn())
			{
				break;
			}
			LATENT_WAIT(0.2f);
		}

		{
			auto & locomotion = mind->access_locomotion();
			locomotion.dont_turn();
		}
	}


	{	// setup and start look at task
		lookAtRel->set_placement_in_final_room(Transform(placement.location_to_world(Vector3(0.0f, 20.0f, 10.0f)), Quat::identity));
	}

	LATENT_WAIT(1.0f);

	{
		auto * imo = mind->get_owner_as_modules_owner();
		if (auto* sound = imo->get_sound())
		{
			hornSound = sound->play_sound(NAME(horn));
		}
	}

	fast_cast<Muezzin>(mind->get_logic())->needsToCall = false;

	while (hornSound.is_set() && hornSound->is_playing())
	{
		LATENT_WAIT(0.2f);
	}

	lookAtRel->clear_target();

	LATENT_WAIT(Random::get_float(0.1f, 0.2f));

	LATENT_ON_BREAK();
	lookAtRel->clear_target();
	if (hornSound.is_set())
	{
		hornSound->stop();
	}
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Muezzin::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Optional<float>, currentTaskTimeLimit);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);
	LATENT_VAR(::Framework::AI::LatentTaskInfoWithParams, nextTask);
	LATENT_VAR(Action::Type, currentTaskAction);
	LATENT_VAR(Action::Type, nextTaskAction);

	LATENT_VAR(::Framework::Room*, startingRoom);
	LATENT_VAR(Vector3, startingLocation);

	LATENT_VAR(::Framework::PresencePath, lookAtTargetPath);
	LATENT_VAR(::Framework::PresencePath, nextLookAtTargetPath);
	LATENT_VAR(::Framework::RelativeToPresencePlacement, lookAtRel);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, lookAtTask);
	LATENT_VAR(int, lookAtBlocked);

	LATENT_VAR(int, collisionCount);
	LATENT_VAR(float, blockLookingAtCollider)

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(Confussion, confussion);

	auto * muezzin = fast_cast<Muezzin>(logic);

	confussion.advance(LATENT_DELTA_TIME);

	if (currentTaskTimeLimit.is_set())
	{
		currentTaskTimeLimit = currentTaskTimeLimit.get() - LATENT_DELTA_TIME;
	}
	blockLookingAtCollider = max(0.0f, blockLookingAtCollider - LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	collisionCount = 0;
	blockLookingAtCollider = 0.0f;
		
	confussion.setup(mind);

	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(dealtDamage), [framePtr, mind, muezzin](Framework::AI::Message const & _message)
		{
			muezzin->hasBeenDamaged = true;
			if (auto * source = _message.get_param(NAME(damageSource)))
			{
				muezzin->damageInstigator = source->get_as<SafePtr<Framework::IModulesOwner>>().get();
				if (muezzin->damageInstigator.is_set())
				{
					muezzin->damageInstigator = muezzin->damageInstigator->get_top_instigator();
				}
			}
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();

		}
		);
		messageHandler.set(NAME(muezzinCall), [framePtr, mind, muezzin](Framework::AI::Message const & _message)
		{
			muezzin->needsToCall = true;
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
	}

	{	// where are we?
		auto * imo = mind->get_owner_as_modules_owner();
		auto * presence = imo->get_presence();
		startingRoom = presence->get_in_room();
		startingLocation = presence->get_placement().get_translation();
	}

	currentTaskAction = Action::None;
	nextTaskAction = Action::None;

	lookAtBlocked = 0;

	{	// setup and start look at task
		::Framework::AI::LatentTaskInfoWithParams lookAtTaskInfo;
		lookAtTaskInfo.clear();
		lookAtTaskInfo.propose(AI_LATENT_TASK_FUNCTION(Tasks::look_at));
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, int*, &lookAtBlocked);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::PresencePath*, &lookAtTargetPath);
		ADD_LATENT_TASK_INFO_PARAM(lookAtTaskInfo, ::Framework::RelativeToPresencePlacement*, &lookAtRel);
		lookAtTask.start_latent_task(mind, lookAtTaskInfo);

		lookAtRel.set_owner(mind->get_owner_as_modules_owner());
	}

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

	while (true)
	{
		{
			auto * imo = mind->get_owner_as_modules_owner();
			auto * presence = imo->get_presence();

			lookAtTargetPath.update_simplify();

			if (currentTask.can_start_new())
			{
				// choose best action for now
				nextTask.clear();
				nextLookAtTargetPath.reset();

				// what to do with confussion?
				// 
				// always prefer to call, if called, switch back
				if (muezzin->needsToCall)
				{
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(horn), 10))
					{
						nextTaskAction = Action::Horn;
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::RelativeToPresencePlacement*, &lookAtRel);
					}
				}
				
				if (currentTaskAction == Action::StandStill)
				{
					todo_note(TXT("turning around"));
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(wander), 5, true))
					{
						nextTaskAction = Action::Wander;
					}
				}
				else
				{
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle), 5, true, NP, NP, Random::get_float(1.0f, 4.0f)))
					{
						nextTaskAction = Action::StandStill;
					}
				}

				// go back to the room if not in it, or stop if we went back
				if (startingRoom == imo->get_presence()->get_in_room())
				{
					if (currentTaskAction == Action::GoBack)
					{
						currentTask.stop();
						currentTaskAction = Action::None;
					}
				}
				else
				{
					if (nextTask.propose(AI_LATENT_TASK_FUNCTION(go_back), 15))
					{
						nextTaskAction = Action::GoBack;
						ADD_LATENT_TASK_INFO_PARAM(nextTask, ::Framework::Room*, startingRoom);
						ADD_LATENT_TASK_INFO_PARAM(nextTask, Vector3, startingLocation);
					}
				}

				if (currentTaskAction == Action::LookAtCollider)
				{
					blockLookingAtCollider = 1.0f;
				}
					
				// maybe we have hit something? look at it
				if ((currentTaskAction != Action::LookAtCollider && blockLookingAtCollider <= 0.0) || muezzin->hasBeenDamaged)
				{
					Framework::IModulesOwner* lookAt = nullptr;

					if (muezzin->hasBeenDamaged)
					{
						lookAt = muezzin->damageInstigator.get();
					}
					else if (! muezzin->needsToCall && currentTaskAction != Action::Horn)
					{
						for_every(collidedWith, imo->get_collision()->get_collided_with())
						{
							if (auto* obj = fast_cast<Framework::Object>(collidedWith->collidedWithObject.get()))
							{
								if (fast_cast<Framework::Scenery>(obj) == nullptr) // skip scenery
								{
									if (auto *op = obj->get_presence())
									{
										if (op->get_in_room() == presence->get_in_room())
										{
											if ((lookAt = op->get_owner()))
											{
												if ((lookAt = lookAt->get_top_instigator()))
												{
													++collisionCount;
													if (collisionCount >= 2)
													{
														break;
													}
													else
													{
														lookAt = nullptr;
													}
												}
											}
										}
									}
								}
							}
						}
					}

					if (lookAt)
					{
						if (nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::idle), 15, NP, NP, NP, Random::get_float(1.5f, 3.0f)))
						{
							nextTaskAction = Action::LookAtCollider;
							nextLookAtTargetPath.find_path(imo, lookAt);
						}
					}
					else
					{
						collisionCount = 0;
					}
				}
			}

			// check if we can start new one, if so, start it
			if (currentTask.can_start(nextTask))
			{
				currentTaskAction = nextTaskAction;
				lookAtTargetPath = nextLookAtTargetPath;
				currentTask.start_latent_task(mind, nextTask);
			}
		}

		// clear for next frame
		muezzin->hasBeenDamaged = false;
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
