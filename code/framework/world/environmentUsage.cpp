#include "environmentUsage.h"

#include "doorInRoom.h"
#include "room.h"
#include "environment.h"

#include "..\..\core\concurrency\threadManager.h"
#include "..\..\core\concurrency\threadSystemUtils.h"

using namespace Framework;

EnvironmentUsage::EnvironmentUsage(Room* _owner)
: owner(_owner)
, environment(nullptr)
, wasActiveLastFrame(false)
, wasActiveAnytime(false)
{
	an_assert(owner);
	threadUsages.set_size(Concurrency::ThreadSystemUtils::get_number_of_cores());
	for_every(threadUsage, threadUsages)
	{
		threadUsage->activationLevel = NOT_ACTIVE;
	}
}

void EnvironmentUsage::ready_for_rendering(float _deltaTime)
{
	wasActiveLastFrame = false;
	timeInactive += _deltaTime;
	for_every(threadUsage, threadUsages)
	{
		if (threadUsage->activationLevel != NOT_ACTIVE)
		{
			lastActivePlacement = threadUsage->placement;
			wasActiveLastFrame = true;
			wasActiveAnytime = true;
			timeInactive = 0.0f;
		}
		threadUsage->activationLevel = NOT_ACTIVE;
	}
}

/**
 *	Activation propagates placement among ALL rooms that share environment.
 *	This is done to have same view in all rooms.
 *	But as we don't have euclidian space, we need at some point to have discontinuity - to get that, we use activationLevel
 */
Transform EnvironmentUsage::activate(int _activationLevel)
{
	an_assert(environment);
	an_assert(owner);
	// if at least one environment user was active last frame, it had to be propagated to other environment users making them all active
	return activate_worker(_activationLevel, nullptr);
}

Transform EnvironmentUsage::activate_worker(int _atActivationLevel, DoorInRoom const * _throughDoorFromOuter)
{
	ThreadUsage & threadUsage = threadUsages[Concurrency::ThreadManager::get_current_thread_id()];
	if (threadUsage.activationLevel != NOT_ACTIVE && threadUsage.activationLevel <= _atActivationLevel)
	{
		// already activated with higher level
		return threadUsage.placement;
	}
	// setup activation
	threadUsage.activationLevel = _atActivationLevel;
	// setup placement
	if (_throughDoorFromOuter)
	{
		// get placement from room from which we came
		an_assert(_throughDoorFromOuter->get_in_room()->get_environment() == environment);
		threadUsage.placement = _throughDoorFromOuter->get_other_room_transform().to_local(_throughDoorFromOuter->get_in_room()->get_environment_usage().get_placement());
		an_assert(threadUsage.placement.get_orientation().is_normalised());
	}
	else
	{
		if (!wasActiveAnytime || (environment->get_time_before_usage_reset() >= 0.0f && environment->get_time_before_usage_reset() < timeInactive))
		{
			// it really doesn't matter as we didn't see it last frame, use identity for placement
			todo_future(TXT("maybe change 'was active last frame' to 'time since last seen'?"));
			threadUsage.placement = requestedPlacement.to_world(environment->get_info().get_default_in_room_placement());
			an_assert(threadUsage.placement.get_orientation().is_normalised());
		}
		else
		{
			threadUsage.placement = lastActivePlacement;
			an_assert(threadUsage.placement.get_orientation().is_normalised());
		}
		// if it was active last frame, keep same placement
	}
	if (!environment->get_info().does_propagate_placement_between_rooms())
	{
		// if we don't want to propagate break now
		return threadUsage.placement;
	}
	todo_future(TXT("with this approach we will end up doing same rooms more than one time. for now that's fine I guess but this better change in future"));
	an_assert(owner);
	int nextActivationLevel = _atActivationLevel + 1;
	for_every_ptr(door, owner->get_doors())
	{
		if (door->is_visible())
		{
			an_assert(door->get_door());
			if (door->get_door()->does_block_environment_propagation())
			{
				continue;
			}
			if (Room * roomOnOtherSide = door->get_world_active_room_on_other_side())
			{
				if (roomOnOtherSide->get_environment() == environment)
				{
					roomOnOtherSide->access_environment_usage().activate_worker(nextActivationLevel, door);
				}
			}
		}
	}

	return threadUsage.placement;
}

int EnvironmentUsage::get_activation_level() const
{
	return threadUsages[Concurrency::ThreadManager::get_current_thread_id()].activationLevel;
}

Transform const & EnvironmentUsage::get_placement() const
{
	return threadUsages[Concurrency::ThreadManager::get_current_thread_id()].placement;
}

