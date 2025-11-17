#include "relativeToPresencePlacement.h"

#include "pathStringPulling.h"

#include "..\module\modulePresence.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\core.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(RelativeToPresencePlacement);

RelativeToPresencePlacement RelativeToPresencePlacement::s_empty;

RelativeToPresencePlacement::RelativeToPresencePlacement()
: SafeObject(this)
, owner(nullptr)
, isActive(false)
, finalRoom(nullptr)
, target(nullptr)
, placementInFinalRoomWS(NP)
, placementRelative(NP)
{
	SET_EXTRA_DEBUG_INFO(throughDoor, TXT("RelativeToPresencePlacement.throughDoor"));
}

RelativeToPresencePlacement::~RelativeToPresencePlacement()
{
	make_safe_object_unavailable();
	set_owner(nullptr);
	set_target_internal(nullptr, true);
}

RelativeToPresencePlacement::RelativeToPresencePlacement(RelativeToPresencePlacement const & _source)
: SafeObject(this)
, owner(nullptr)
, target(nullptr)
{
	SET_EXTRA_DEBUG_INFO(throughDoor, TXT("RelativeToPresencePlacement.throughDoor"));
	set_owner_internal(_source.owner);
	isActive = _source.isActive;
	finalRoom = _source.finalRoom;
	set_target_internal(_source.target, true);
	placementInFinalRoomWS = _source.placementInFinalRoomWS;
	placementRelative = _source.placementRelative;
	throughDoor = _source.throughDoor;
}

RelativeToPresencePlacement & RelativeToPresencePlacement::operator = (RelativeToPresencePlacement const & _source)
{
	SET_EXTRA_DEBUG_INFO(throughDoor, TXT("RelativeToPresencePlacement.throughDoor"));
	set_owner_internal(_source.owner);
	isActive = _source.isActive;
	finalRoom = _source.finalRoom;
	set_target_internal(_source.target, true);
	placementInFinalRoomWS = _source.placementInFinalRoomWS;
	placementRelative = _source.placementRelative;
	throughDoor = _source.throughDoor;
	return *this;
}

RelativeToPresencePlacement & RelativeToPresencePlacement::operator =(PresencePath const& _path)
{
	scoped_call_stack_info(TXT("RelativeToPresencePlacement::operator ="));
	set_owner_internal(_path.get_owner_presence());
	followIfTargetLost = _path.followIfTargetLost;
	isActive = _path.is_active();
	throughDoor = _path.throughDoor;
	update_final_room();
	placementInFinalRoomWS.clear();
	placementRelative.clear();
	if (auto* t = _path.get_target_presence())
	{
		set_placement_in_final_room(t->get_placement(), t);
	}
	else if (isActive)
	{
		an_assert(false, TXT("active but without target? presence path does not require placement, but we do, make it up?"));
		set_target_internal(nullptr, true);
	}
	return *this;
}

void RelativeToPresencePlacement::copy_just_target_placement_from(RelativeToPresencePlacement const & _source)
{
	set_owner_internal(_source.owner);
	isActive = _source.isActive;
	finalRoom = _source.finalRoom;
	set_target_internal(nullptr, true);
	placementInFinalRoomWS = _source.get_placement_in_final_room();
	placementRelative.clear();
	throughDoor = _source.throughDoor;
}

IModulesOwner* RelativeToPresencePlacement::get_owner() const
{
	return owner ? owner->get_owner() : nullptr;
}

IModulesOwner* RelativeToPresencePlacement::get_target() const
{
	return target ? target->get_owner() : nullptr;
}

void RelativeToPresencePlacement::set_owner(IModulesOwner* _owner)
{
	set_owner_presence(_owner ? _owner->get_presence() : nullptr);
}

void RelativeToPresencePlacement::set_owner_presence(ModulePresence* _owner)
{
	if (owner == _owner)
	{
		return;
	}

	set_owner_internal(_owner);

	clear_path();
}

void RelativeToPresencePlacement::set_owner_internal(ModulePresence* _owner)
{
	if (owner == _owner)
	{
		return;
	}

	if (owner && ! temporarySnapshot)
	{
		owner->remove_presence_observer(this);
	}

	owner = _owner;

	if (owner && !temporarySnapshot)
	{
		owner->add_presence_observer(this);
	}
}

void RelativeToPresencePlacement::set_target_internal(ModulePresence* _target, bool _ignorePlacements)
{
	if (target == _target)
	{
		return;
	}

	if (target && !temporarySnapshot)
	{
		target->remove_presence_observer(this);
	}
	if (!_ignorePlacements && target && placementRelative.is_set())
	{
		placementInFinalRoomWS = target->get_placement().to_world(placementRelative.get());
		placementRelative.clear();
	}
	target = _target;
	if (!_ignorePlacements && target && placementInFinalRoomWS.is_set())
	{
		placementRelative = target->get_placement().to_local(placementInFinalRoomWS.get());
		placementInFinalRoomWS.clear();
	}
	if (target && !temporarySnapshot)
	{
		target->add_presence_observer(this);
	}
}

void RelativeToPresencePlacement::reset()
{
	set_owner_internal(nullptr);
	clear_path();
}

void RelativeToPresencePlacement::clear_path()
{
	scoped_call_stack_info(TXT("clear_path"));
	isActive = false;
	finalRoom = nullptr;
	set_target_internal(nullptr, true);
	placementInFinalRoomWS.clear();
	placementRelative.clear();
	throughDoor.clear();
	update_final_room();
}

void RelativeToPresencePlacement::clear_target()
{
	clear_path();
}

void RelativeToPresencePlacement::auto_clear_target()
{
	if (followIfTargetLost)
	{
		clear_target_but_follow();
	}
	else
	{
		clear_target();
	}
}

void RelativeToPresencePlacement::clear_target_but_follow()
{
	if (target)
	{
		set_placement_in_final_room(target->get_placement()); // will clear target
	}
}

void RelativeToPresencePlacement::update_final_room()
{
	if (! throughDoor.is_empty())
	{
		if (throughDoor.get_last().get())
		{
			finalRoom = throughDoor.get_last()->get_world_active_room_on_other_side();
		}
		else
		{
			warn(TXT("using RelativeToPresencePlacement with missing doors!"));
		}
	}
	else
	{
		finalRoom = owner? owner->get_in_room() : nullptr;
	}
}

void RelativeToPresencePlacement::push_through_door(DoorInRoom const * _door)
{
	scoped_call_stack_info(TXT("push_through_door"));
	if (!throughDoor.is_empty() &&
		throughDoor.get_last() == _door->get_door_on_other_side())
	{
		// we would be going back
		throughDoor.remove_at(throughDoor.get_size() - 1);
		an_assert(check_if_through_door_makes_sense());
	}
	else
	{
		throughDoor.push_back(::SafePtr<DoorInRoom>(cast_to_nonconst(_door)));
		an_assert(check_if_through_door_makes_sense());
	}
	update_final_room();
}

void RelativeToPresencePlacement::pop_through_door()
{
	scoped_call_stack_info(TXT("pop_through_door"));
	throughDoor.pop_back();
	update_final_room();
}

void RelativeToPresencePlacement::set_placement_in_final_room(Transform const & _placementWS, IModulesOwner* _target)
{
	set_placement_in_final_room(_placementWS, _target ? _target->get_presence() : nullptr);
}

void RelativeToPresencePlacement::set_placement_in_final_room(Transform const & _placementWS, ModulePresence* _target)
{
	isActive = true;
	placementInFinalRoomWS.clear();
	placementRelative.clear();
	set_target_internal(_target, true);
	if (target)
	{
		// make it local
		placementRelative = target->get_placement().to_local(_placementWS);
	}
	else
	{
		placementInFinalRoomWS = _placementWS;
	}
	an_assert(!placementInFinalRoomWS.is_set() || placementInFinalRoomWS.get().get_orientation().is_normalised(), TXT("orientation %S _placementWS's %S target's %S"), placementInFinalRoomWS.get().get_orientation().to_string().to_char(), _placementWS.get_orientation().to_string().to_char(), target? target->get_placement().get_orientation().to_string().to_char() : TXT("??"));
	an_assert(!placementRelative.is_set() || placementRelative.get().get_orientation().is_normalised(), TXT("orientation %S _placementWS's %S target's %S"), placementRelative.get().get_orientation().to_string().to_char(), _placementWS.get_orientation().to_string().to_char(), target? target->get_placement().get_orientation().to_string().to_char() : TXT("??"));
}

Transform RelativeToPresencePlacement::get_placement_in_final_room() const
{
	an_assert(isActive);
	return target ? target->get_placement().to_world(placementRelative.get(Transform::identity)) : placementInFinalRoomWS.get(Transform::identity);
}

Transform RelativeToPresencePlacement::get_placement_in_owner_room() const
{
	return calculate_final_room_transform().to_world(get_placement_in_final_room());
}

Vector3 RelativeToPresencePlacement::get_direction_towards_placement_in_owner_room() const
{
	an_assert(owner);
	an_assert(is_active());
	Transform placementInOwnerRoom = get_placement_in_owner_room();
	return placementInOwnerRoom.get_translation() - owner->get_placement().get_translation();
}
		
void RelativeToPresencePlacement::on_presence_changed_room(ModulePresence* _presence, Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors)
{
	scoped_call_stack_info(TXT("RelativeToPresencePlacement::on_presence_changed_room"));
	if (!isActive)
	{
		return;
	}

	if (update_for_doors_missing())
	{
		return;
	}

	if (_throughDoors && _throughDoors->get_number_of_doors() > 1)
	{
#ifdef AN_ASSERT
		++ checksSuppressed;
#endif
		// move through each door
		Framework::DoorInRoom const ** iThroughDoor = _throughDoors->begin();
		for_count(int, i, _throughDoors->get_number_of_doors())
		{
			on_presence_changed_room(_presence, (*iThroughDoor)->get_world_active_room_on_other_side(), (*iThroughDoor)->get_other_room_transform(), cast_to_nonconst(*iThroughDoor), (*iThroughDoor)->get_door_on_other_side(), nullptr);
			++iThroughDoor;
		}
#ifdef AN_ASSERT
		-- checksSuppressed;
#endif
		an_assert_info(debug_check_if_path_valid());
		return;
	}

	an_assert(_exitThrough && _enterThrough);
	an_assert(_enterThrough->get_in_room() == _intoRoom);
	an_assert(_enterThrough->get_door_on_other_side() == _exitThrough);

	bool updateFinalRoom = false;
	if (_presence == owner)
	{
		if (throughDoor.is_empty() || throughDoor.get_first() != _exitThrough)
		{
			an_assert_info(throughDoor.is_empty() ||
					  (_exitThrough && throughDoor.get_first().get() && _exitThrough->get_in_room() == throughDoor.get_first()->get_in_room()));
			if (throughDoor.get_size() < throughDoor.get_max_size())
			{
				// we were in same room or we moved away from our final room, we need to remember which door leads us to final room
				throughDoor.push_front(::SafePtr<DoorInRoom>(_enterThrough));
				an_assert_info(check_if_through_door_makes_sense());
			}
			else
			{
				warn(TXT("relative to presence placement path exceeded size"));
				todo_hack(TXT("lost it, inform somehow?"));
				auto_clear_target();
				return;
			}
		}
		else
		{
			// we moved towards final room
			throughDoor.pop_front();
			an_assert_info(check_if_through_door_makes_sense());
		}
		updateFinalRoom = true;
		// check it here, as both may move in same time
		an_assert_info(checksSuppressed || throughDoor.is_empty() || (throughDoor.get_first().is_set() && throughDoor.get_first()->get_in_room() == owner->get_in_room()));
	}

	if (target &&
		_presence == target)
	{
		if (throughDoor.is_empty() || throughDoor.get_last() != _enterThrough)
		{
			an_assert_info(throughDoor.is_empty() || _exitThrough->get_in_room() == throughDoor.get_last()->get_world_active_room_on_other_side());
			if (throughDoor.get_size() < throughDoor.get_max_size())
			{
				// we were in same room or we moved away from our starting room, we need to remember through which door we moved to new final room
				throughDoor.push_back(::SafePtr<DoorInRoom>(_exitThrough));
				an_assert(check_if_through_door_makes_sense());
			}
			else
			{
				warn(TXT("relative to presence placement path exceeded size"));
				todo_hack(TXT("lost it, inform somehow?"));
				auto_clear_target();
				return;
			}
		}
		else
		{
			// we moved towards starting room
			throughDoor.pop_back();
			an_assert(check_if_through_door_makes_sense());
		}
		updateFinalRoom = true;
		// check it here, as both may move in same time
		an_assert_info(checksSuppressed || throughDoor.is_empty() || (throughDoor.get_last().is_set() && throughDoor.get_last()->get_world_active_room_on_other_side() == target->get_in_room()));
	}

	if (updateFinalRoom)
	{
		update_final_room();
	}

	an_assert_info(checksSuppressed || debug_check_if_path_valid());
}

void RelativeToPresencePlacement::on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom * _enteredThroughDoor)
{
	// we have no idea where we are
	clear_path();
}

void RelativeToPresencePlacement::on_presence_removed_from_room(ModulePresence* _presence, Room* _room)
{
	// we have no idea where we are, where we will be
	clear_path();
}

void RelativeToPresencePlacement::on_presence_destroy(ModulePresence* _presence)
{
	if (owner == _presence)
	{
		clear_path();
		set_owner(nullptr);
	}
	if (target &&
		target == _presence)
	{
		auto_clear_target();
	}
}

Transform RelativeToPresencePlacement::calculate_final_room_transform() const
{
	an_assert(owner);
	an_assert(isActive);
	Transform transform = Transform::identity;
	for_every_ref(door, throughDoor)
	{
		transform = transform.to_world(door->get_other_room_transform());
	}
	return transform;
}

Transform RelativeToPresencePlacement::calculate_placement_in_os() const
{
	an_assert(owner);
	an_assert(isActive);

	// calculate placement in same room as owner is
	Transform placement = get_placement_in_final_room();
	for_every_reverse_ref(door, throughDoor)
	{
		placement = door->get_other_room_transform().to_world(placement);
	}

	// put that placement into owner's reference frame
	return owner->get_placement().to_local(placement);
}

bool RelativeToPresencePlacement::is_there_clear_line(Optional<Transform> const & _ownerRelativeOffset, Optional<Transform> const & _targetRelativeOffset, float _maxPtDifference) const
{
	float flatDistance;
	float stringPulledDistance;
	calculate_distances(flatDistance, stringPulledDistance, _targetRelativeOffset, _ownerRelativeOffset);

	float diff = max(0.0f, stringPulledDistance - flatDistance);
	return diff <= stringPulledDistance * max(EPSILON, _maxPtDifference);
}

void RelativeToPresencePlacement::calculate_distances(OUT_ float& _flatDistance, OUT_ float& _stringPulledDistance, Optional<Transform> const & _ownerRelativeOffset, Optional<Transform> const & _targetRelativeOffset) const
{
	Transform ownerRelativeOffset = _ownerRelativeOffset.is_set() ? _ownerRelativeOffset.get() : (owner ? owner->get_centre_of_presence_os() : Transform::identity);
	Transform targetRelativeOffset = _targetRelativeOffset.is_set() ? _targetRelativeOffset.get() : (target ? target->get_centre_of_presence_os() : Transform::identity);

	Vector3 ownerLocation = owner->get_placement().to_world(ownerRelativeOffset).get_translation();
	Vector3 targetLocation = location_from_target_to_owner(target->get_placement().to_world(targetRelativeOffset).get_translation());

	_flatDistance = (ownerLocation - targetLocation).length();
	_stringPulledDistance = calculate_string_pulled_distance(_ownerRelativeOffset, _targetRelativeOffset);
}

float RelativeToPresencePlacement::get_straight_length() const
{
	if (!is_active() || !finalRoom.is_set())
	{
		return 0.0f;
	}

	an_assert(get_owner_presence());

	return (get_placement_in_owner_room().get_translation() - get_owner_presence()->get_placement().get_translation()).length();
}

float RelativeToPresencePlacement::calculate_string_pulled_distance(Optional<Transform> const & _ownerRelativeOffset, Optional<Transform> const & _targetRelativeOffset) const
{
	if (!is_active() || !finalRoom.is_set())
	{
		return INF_FLOAT;
	}

	float distance = 0.0f;

	// this is simplified version, for each door it tries to find place on it - this means that it may get sucked onto one of the edges

	Transform ownerRelativeOffset = _ownerRelativeOffset.is_set() ? _ownerRelativeOffset.get() : (owner ? owner->get_centre_of_presence_os() : Transform::identity);
	Transform targetRelativeOffset = _targetRelativeOffset.is_set() ? _targetRelativeOffset.get() : (target ? target->get_centre_of_presence_os() : Transform::identity);

	// pull owner location towards target through doors
	Vector3 ownerLocation = owner->get_placement().to_world(ownerRelativeOffset).get_translation();
	// target location starts in owner room and moves with each door to end up in target's room
	Transform usePlacementInFinalRoomWS = get_placement_in_final_room();
	Vector3 targetLocation = usePlacementInFinalRoomWS.to_world(targetRelativeOffset).get_translation();

	PathStringPulling stringPulling;
	stringPulling.set_start(owner->get_in_room(), ownerLocation);
	stringPulling.set_end(finalRoom.get(), targetLocation);

	for_every_ref(door, throughDoor)
	{
		if (!door)
		{
			return INF_FLOAT; // something went terribly wrong?
		}
		stringPulling.push_through_door(door);
	}

	stringPulling.string_pull(2); // enough?

	for_every(node, stringPulling.get_path())
	{
		distance += (node->nextNode - node->node).length();
	}

	return distance;
}

bool RelativeToPresencePlacement::find_path(IModulesOwner* _owner, IModulesOwner* _target, bool _anyPath, Optional<int> const & _maxDepth)
{
	return _target? find_path(_owner, _target->get_presence()->get_placement(), _target, _anyPath, _maxDepth) : false;
}

bool RelativeToPresencePlacement::find_path(IModulesOwner* _owner, Room* _room, Transform const & _placementWS, bool _anyPath, Optional<int> const& _maxDepth)
{
	scoped_call_stack_info(TXT("RelativeToPresencePlacement::find_path"));
	PERFORMANCE_GUARD_LIMIT(0.002f, TXT("RelativeToPresencePlacement::find_path"));

	clear_path();
	set_owner(_owner);
	placementInFinalRoomWS = _placementWS;

	bool result = true;

	if (_owner && _owner->get_presence()->get_in_room() &&
		_room &&
		_owner->get_presence()->get_in_room() != _room)
	{
		int const maxSize = clamp(_maxDepth.get(64), 3, 128); // at least 3, prefer what we're suggested and stick it to some reasonable values
		ARRAY_STACK(DoorInRoom*, bestWay, maxSize);
		ARRAY_STACK(DoorInRoom*, currentWay, maxSize);
		{
			scoped_call_stack_info(TXT("find_path_worker"));
			find_path_worker(_owner->get_presence()->get_in_room(), _room, bestWay, currentWay, _anyPath);
		}
		if (!bestWay.is_empty() && (throughDoor.get_max_size() - throughDoor.get_size()) >= bestWay.get_size()) // only if we have enough space
		{
			for_every_ptr(dir, bestWay)
			{
				an_assert_log_always(throughDoor.has_place_left(), TXT("not enough space in throughDoor"));
				if (throughDoor.has_place_left())
				{
					throughDoor.push_back(::SafePtr<DoorInRoom>(dir));
					an_assert(check_if_through_door_makes_sense());
				}
				else
				{
					clear_target();
					result = false;
					break;
				}
			}
		}
		else
		{
			// maybe it's too far?
			clear_target();
			result = false;
		}
	}

	if (result)
	{
		isActive = true;
		update_final_room();
	}

	return result;
}

bool RelativeToPresencePlacement::find_path(IModulesOwner* _owner, Transform const & _placementWS, IModulesOwner* _target, bool _anyPath, Optional<int> const& _maxDepth)
{
	if (find_path(_owner, _target ? _target->get_presence()->get_in_room() : _owner->get_presence()->get_in_room(), _placementWS, _anyPath, _maxDepth))
	{
		if (_target)
		{
			set_target_internal(_target->get_presence()); // will automatically rearrange placements
		}
		return true;
	}
	else
	{
		return false;
	}
}

void RelativeToPresencePlacement::find_path_worker(Room* _current, Room* _end, ArrayStack<DoorInRoom*>& bestWay, ArrayStack<DoorInRoom*>& currentWay, bool _anyPath)
{
	// copy of PresencePath::find_path_worker
	if (!_current)
	{
		return;
	}
	if (_current == _end)
	{
		if (bestWay.is_empty() || currentWay.get_size() < bestWay.get_size())
		{
			bestWay = currentWay;
		}
		return;
	}

	DoorInRoom* throughDoorInCurrent = nullptr;
	if (!currentWay.is_empty())
	{
		throughDoorInCurrent = currentWay.get_last()->get_door_on_other_side();
	}

	for_every_ptr(dir, _current->get_all_doors())
	{
		if (dir == throughDoorInCurrent ||
			!dir->has_world_active_room_on_other_side() ||
			!dir->can_move_through() ||
			!dir->is_visible() ||
			!dir->is_relevant_for_movement())
		{
			continue;
		}
		if (!_anyPath &&
			throughDoorInCurrent &&
			throughDoorInCurrent->get_nav_door_node().is_set() &&
			dir->get_nav_door_node().is_set() &&
			throughDoorInCurrent->get_nav_door_node()->get_group() != dir->get_nav_door_node()->get_group())
		{
			// not connected directly
			continue;
		}
		{
			bool alreadyAdded = false;
			for_every_ptr(currentWayElement, currentWay)
			{
				if (currentWayElement->get_in_room() == dir->get_world_active_room_on_other_side())
				{
					alreadyAdded = true;
					break;
				}
			}
			if (alreadyAdded)
			{
				// skip
				continue;
			}
		}
		{
			bool wouldBeWorse = false;
			for_every_ptr(bestWayElement, bestWay)
			{
				if (bestWayElement->get_in_room() == dir->get_world_active_room_on_other_side())
				{
					todo_note(TXT("check this condition?"));
					if (for_everys_index(bestWayElement) <= currentWay.get_size())
					{
						wouldBeWorse = true;
						break;
					}
				}
			}
			if (wouldBeWorse)
			{
				// skip
				continue;
			}
		}

		//
		if (currentWay.has_place_left())
		{
			currentWay.push_back(dir);
			find_path_worker(dir->get_world_active_room_on_other_side(), _end, bestWay, currentWay, _anyPath);
			currentWay.pop_back();
		}
	}
}

Transform RelativeToPresencePlacement::from_owner_to_target(Transform const & _a) const
{
	an_assert(is_active());
	Transform a = _a;
	for_every_ref(door, throughDoor)
	{
		if (!door)
		{
			return _a;
		}
		a = door->get_other_room_transform().to_local(a);
	}
	return a;
}

Transform RelativeToPresencePlacement::from_target_to_owner(Transform const & _a) const
{
	an_assert(is_active());
	Transform a = _a;
	for_every_reverse_ref(door, throughDoor)
	{
		if (!door)
		{
			return _a;
		}
		a = door->get_other_room_transform().to_world(a);
	}
	return a;
}

Vector3 RelativeToPresencePlacement::location_from_owner_to_target(Vector3 const & _a) const
{
	an_assert(is_active());
	Vector3 a = _a;
	for_every_ref(door, throughDoor)
	{
		if (!door)
		{
			return _a;
		}
		a = door->get_other_room_transform().location_to_local(a);
	}
	return a;
}

Vector3 RelativeToPresencePlacement::location_from_target_to_owner(Vector3 const & _a) const
{
	an_assert(is_active());
	Vector3 a = _a;
	for_every_reverse_ref(door, throughDoor)
	{
		if (!door)
		{
			return _a;
		}
		a = door->get_other_room_transform().location_to_world(a);
	}
	return a;
}

Vector3 RelativeToPresencePlacement::get_target_centre_relative_to_owner() const
{
	an_assert(is_active());
	if (target)
	{
		return owner->get_placement().location_to_local(location_from_target_to_owner(target->get_centre_of_presence_WS()));
	}
	else
	{
		return owner->get_placement().location_to_local(location_from_target_to_owner(placementInFinalRoomWS.get(Transform::identity).get_translation()));
	}
}

Vector3 RelativeToPresencePlacement::vector_from_owner_to_target(Vector3 const & _a) const
{
	an_assert(is_active());
	Vector3 a = _a;
	for_every_ref(door, throughDoor)
	{
		if (!door)
		{
			return _a;
		}
		a = door->get_other_room_transform().vector_to_local(a);
	}
	return a;
}

Vector3 RelativeToPresencePlacement::vector_from_target_to_owner(Vector3 const & _a) const
{
	an_assert(is_active());
	Vector3 a = _a;
	for_every_reverse_ref(door, throughDoor)
	{
		if (!door)
		{
			return _a;
		}
		a = door->get_other_room_transform().vector_to_world(a);
	}
	return a;
}

void RelativeToPresencePlacement::log(LogInfoContext & _context) const
{
	if (is_active())
	{
		_context.log(is_active() ? TXT("ACTIVE") : TXT("NOT ACTIVE"));
		_context.log(TXT("path owner: %S -- is in \"%S\""), owner ? owner->get_owner()->ai_get_name().to_char() : TXT("--"), owner ? owner->get_in_room()->get_name().to_char() : TXT("--"));
		_context.log(TXT("path target: %S -- is in \"%S\""), target ? target->get_owner()->ai_get_name().to_char() : TXT("--"), target ? target->get_in_room()->get_name().to_char() : TXT("--"));
		if (placementInFinalRoomWS.is_set())
		{
			_context.log(TXT("placement in final room:"));
			LOG_INDENT(_context);
			placementInFinalRoomWS.get(Transform::identity).log(_context);
		}
		if (placementRelative.is_set())
		{
			_context.log(TXT("placement relative:"));
			LOG_INDENT(_context);
			placementRelative.get(Transform::identity).log(_context);
		}
		_context.log(TXT("placement in final room (actual):"));
		{
			LOG_INDENT(_context);
			if (is_active())
			{
				get_placement_in_final_room().log(_context);
			}
		}
		_context.log(TXT("placement in owner room (actual):"));
		{
			LOG_INDENT(_context);
			if (is_active())
			{
				get_placement_in_owner_room().log(_context);
			}
		}
		_context.log(TXT("through doors:"));
		LOG_INDENT(_context);
		for_every_ref(door, throughDoor)
		{
			if (door)
			{
				_context.log(TXT("\"%S\": \"%S\" -> \"%S\""), door->ai_get_name().to_char(), door->get_in_room()->get_name().to_char(), door->get_world_active_room_on_other_side()->get_name().to_char());
			}
			else
			{
				_context.log(TXT("<missing, safely removed>"));
			}
		}
	}

}

void RelativeToPresencePlacement::be_temporary_snapshot()
{
	if (temporarySnapshot)
	{
		return;
	}

	temporarySnapshot = true;

	if (owner)
	{
		owner->remove_presence_observer(this);
	}

	if (target)
	{
		target->remove_presence_observer(this);
	}
}

bool RelativeToPresencePlacement::update_for_doors_missing()
{
	for_count(int, i, throughDoor.get_size())
	{
		if (!throughDoor[i].get())
		{
			clear_target();
			return true;
		}
	}
	return false;
}

bool RelativeToPresencePlacement::check_if_through_door_makes_sense() const
{
	for_count(int, i, throughDoor.get_size() - 1)
	{
		if (!throughDoor[i].get() ||
			!throughDoor[i + 1].get())
		{
			return false;
		}
		if (throughDoor[i] == throughDoor[i + 1])
		{
			return false;
		}
		if (throughDoor[i]->get_world_active_room_on_other_side() != throughDoor[i + 1]->get_in_room())
		{
			return false;
		}
	}
	return true;
}

bool RelativeToPresencePlacement::is_path_valid() const
{
	if (!owner)
	{
		return true;
	}
	if (target &&
		target->get_in_room() != finalRoom.get())
	{
		return false;
	}
	if (throughDoor.is_empty())
	{
		return owner->get_in_room() == finalRoom.get();
	}
	if (!check_if_through_door_makes_sense())
	{
		return false;
	}
	for_every(door, throughDoor)
	{
		if (!door->is_set())
		{
			return false;
		}
	}
	return  throughDoor.get_first()->get_in_room() == owner->get_in_room() &&
			throughDoor.get_last()->get_world_active_room_on_other_side() == finalRoom.get();
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool RelativeToPresencePlacement::debug_check_if_path_valid() const
{
	if (is_path_valid())
	{
		return true;
	}
	System::Core::sleep(0.001f); // this is only for debugging, sometimes we may have two characters moving at the same time, give some time to make sure they moved and then go back to checking if path is valid
	if (is_path_valid())
	{
		return true;
	}
	output(TXT("invalid path!"));
	if (target &&
		target->get_in_room() != finalRoom.get())
	{
		output(TXT("TARGET: target %010i v final room %010i"), target->get_in_room(), get_in_final_room());
		output(TXT("        final room  \"%S\""), get_in_final_room() ? get_in_final_room()->get_name().to_char() : TXT("--"));
	}
	output(TXT("OWNER : %010i \"%S\""), owner->get_in_room(), owner->get_in_room()? owner->get_in_room()->get_name().to_char() : TXT("--"));
	for_every(door, throughDoor)
	{
		if (door->is_set())
		{
			output(TXT("   %02i: %010i [%010i] -> [%010i] %010i   \"%S\""), for_everys_index(door),
				door->get()->get_in_room(),
				door->get(),
				door->get()->get_door_on_other_side(),
				door->get()->get_world_active_room_on_other_side(),
				door->get()->get_in_room()? door->get()->get_in_room()->get_name().to_char() : TXT("--"));
		}
		else
		{
			output(TXT("   %02i missing!"), for_everys_index(door));
		}
	}
	output(TXT("FINAL: %010i \"%S\""), get_in_final_room(), get_in_final_room()? get_in_final_room()->get_name().to_char() : TXT("--"));
	output(TXT("TARGET: %010i \"%S\""), target? target->get_in_room() : 0, target && target->get_in_room() ? target->get_in_room()->get_name().to_char() : TXT("--"));
	return false;
}
#endif

DoorInRoom* RelativeToPresencePlacement::get_through_door_into_final_room() const
{
	if (!throughDoor.is_empty() && throughDoor.get_last().get())
	{
		return throughDoor.get_last().get()->get_door_on_other_side();
	}
	else
	{
		return nullptr;
	}
}
