#include "presencePath.h"

#include "pathStringPulling.h"

#include "..\module\modulePresence.h"
#include "..\object\object.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\core.h"

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

using namespace Framework;

REGISTER_FOR_FAST_CAST(PresencePath);

PresencePath PresencePath::s_empty;

PresencePath::PresencePath()
: owner(nullptr)
, target(nullptr)
{
	SET_EXTRA_DEBUG_INFO(throughDoor, TXT("PresencePath.throughDoor"));
}

PresencePath::~PresencePath()
{
	clear_path();
	set_owner_presence(nullptr);
	set_target_presence(nullptr);
}

PresencePath::PresencePath(PresencePath const & _source)
: owner(nullptr)
, target(nullptr)
{
	SET_EXTRA_DEBUG_INFO(throughDoor, TXT("PresencePath.throughDoor"));
	set_owner_presence(_source.owner);
	set_target_presence(_source.target);
	allowEmptyTarget = _source.allowEmptyTarget;
	followIfTargetLost = _source.followIfTargetLost;
	autoFindPathIfLost = _source.autoFindPathIfLost;
	throughDoor = _source.throughDoor;
}

PresencePath & PresencePath::operator = (PresencePath const & _source)
{
	if (&_source != this) // we could do that for kicks
	{
		set_owner_presence(_source.owner);
		set_target_presence(_source.target);
		allowEmptyTarget = _source.allowEmptyTarget;
		followIfTargetLost = _source.followIfTargetLost;
		autoFindPathIfLost = _source.autoFindPathIfLost;
		throughDoor = _source.throughDoor;
	}
	return *this;
}

void PresencePath::reset()
{
	clear_path();
	set_owner_presence(nullptr);
	set_target_presence(nullptr);
}

#ifdef AN_DEVELOPMENT
void PresencePath::set_user_function(tchar const * _userFunction)
{
	userFunction = Name(_userFunction);
}
#endif

bool PresencePath::is_same(PresencePath const & _a, PresencePath const & _b)
{
	if (_a.throughDoor.get_size() == _b.throughDoor.get_size())
	{
		return _a.throughDoor.get_size() == 0 || memory_compare(_a.throughDoor.get_data(), _b.throughDoor.get_data(), _a.throughDoor.get_data_size());
	}
	else
	{
		return false;
	}
}

IModulesOwner* PresencePath::get_owner() const
{
	return owner ? owner->get_owner() : nullptr;
}

IModulesOwner* PresencePath::get_target() const
{
	return target ? target->get_owner() : nullptr;
}

Transform PresencePath::get_placement_in_final_room() const
{
	an_assert(is_active());
	return target->get_placement();
}

Transform PresencePath::get_placement_in_owner_room() const
{
	an_assert(is_active());
	return from_target_to_owner(target->get_placement());
}

Room* PresencePath::get_in_final_room() const
{
	an_assert(is_active());
	return throughDoor.is_empty() || ! throughDoor.get_last().get() ? (owner? owner->get_in_room() : nullptr) : throughDoor.get_last()->get_world_active_room_on_other_side();
}

bool PresencePath::has_target() const
{
	return target != nullptr;
}

void PresencePath::set_owner(IModulesOwner* _owner)
{
	set_owner_presence(_owner ? _owner->get_presence() : nullptr);
}

void PresencePath::set_target(IModulesOwner* _target)
{
	set_target_presence(_target ? _target->get_presence() : nullptr);
}

void PresencePath::set_owner_presence(ModulePresence* _owner)
{
	if (owner == _owner)
	{
		return;
	}

	if (owner && owner != target && !temporarySnapshot)
	{
		owner->remove_presence_observer(this);
	}

	owner = _owner;

	if (owner && owner != target && !temporarySnapshot)
	{
		owner->add_presence_observer(this);
	}
}

void PresencePath::set_target_presence(ModulePresence* _target)
{
	if (target == _target)
	{
		return;
	}

	if (target && target != owner && !temporarySnapshot)
	{
		target->remove_presence_observer(this);
	}

	target = _target;
	allowEmptyTarget = false; // we have target, right?

	if (target && target != owner && !temporarySnapshot)
	{
		target->add_presence_observer(this);
	}
}

void PresencePath::allow_no_target_presence(bool _allow)
{
	allowEmptyTarget = _allow;
}

bool PresencePath::find_path(IModulesOwner* _owner, IModulesOwner* _target, bool _anyPath, Optional<int> const & _maxDepth)
{
	scoped_call_stack_info(TXT("PresencePath::find_path"));
	PERFORMANCE_GUARD_LIMIT(0.002f, TXT("PresencePath::find_path"));

	clear_path();
	set_owner(_owner);
	set_target(_target);

	bool result = true;

	if (_owner && _owner->get_presence()->get_in_room() &&
		_target && _target->get_presence()->get_in_room() &&
		_owner->get_presence()->get_in_room() != _target->get_presence()->get_in_room())
	{
		int const maxSize = _maxDepth.get(128);
		ARRAY_STACK(DoorInRoom*, bestWay, maxSize);
		ARRAY_STACK(DoorInRoom*, currentWay, maxSize);
		find_path_worker(_owner->get_presence()->get_in_room(), _target->get_presence()->get_in_room(), bestWay, currentWay, _anyPath);
		if (!bestWay.is_empty() && (throughDoor.get_max_size() - throughDoor.get_size()) >= bestWay.get_size()) // only if we have enough space
		{
			for_every_ptr(dir, bestWay)
			{
				an_assert_log_always(throughDoor.get_size() < PathSettings::MaxDepth);
				if (throughDoor.has_place_left())
				{
					throughDoor.push_back(SafePtr<DoorInRoom>(dir));
				}
				else
				{
					// too far?
					auto_clear_target();
					result = false;
				}
			}
		}
		else
		{
			// maybe it's too far?
			auto_clear_target();
			result = false;
		}
	}
	return result;
}

void PresencePath::find_path_worker(Room* _current, Room* _end, ArrayStack<DoorInRoom*>& bestWay, ArrayStack<DoorInRoom*>& currentWay, bool _anyPath)
{
	// RelativeToPresencePlacement::find_path_worker is a copy of this - maybe use template, common base class?
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
	if (! currentWay.has_place_left())
	{
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
			! dir->has_world_active_room_on_other_side() ||
			! dir->can_move_through() ||
			! dir->is_visible() ||
			! dir->is_relevant_for_movement())
		{
			continue;
		}
		if (! _anyPath &&
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
		an_assert_log_always(currentWay.has_place_left(), TXT("not enough space for currentWay"));
		if (currentWay.has_place_left())
		{
			currentWay.push_back(dir);
			find_path_worker(dir->get_world_active_room_on_other_side(), _end, bestWay, currentWay, _anyPath);
			currentWay.pop_back();
		}
	}
}

void PresencePath::clear_target_but_follow()
{
	set_target(nullptr);
}	

void PresencePath::auto_clear_target()
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

void PresencePath::auto_find_path()
{
	if (autoFindPathIfLost &&
		owner && owner->get_in_room() &&
		target && target->get_in_room())
	{
		find_path(owner->get_owner(), target->get_owner());
	}
}

void PresencePath::clear_path()
{
	throughDoor.clear();
}

void PresencePath::clear_target()
{
	clear_path();
	set_target(nullptr);
}

void PresencePath::push_through_door(DoorInRoom const * _door)
{
	an_assert_log_always(throughDoor.get_size() < PathSettings::MaxDepth);
	if (throughDoor.has_place_left())
	{
		throughDoor.push_back(SafePtr<DoorInRoom>(cast_to_nonconst(_door)));
	}
	else
	{
		// will be invalid!
	}
}

void PresencePath::pop_through_door()
{
	throughDoor.pop_back();
}

void PresencePath::push_through_doors(DoorInRoomArrayPtr const & _throughDoors)
{
	Framework::DoorInRoom const ** iThroughDoor = _throughDoors.begin();
	for_count(int, i, _throughDoors.get_number_of_doors())
	{
		push_through_door(*iThroughDoor);
		++iThroughDoor;
	}
}

void PresencePath::push_through_doors_in_reverse(DoorInRoomArrayPtr const & _throughDoors)
{
	Framework::DoorInRoom const ** iThroughDoor = _throughDoors.last_ptr();
	for_count(int, i, _throughDoors.get_number_of_doors())
	{
		push_through_door((*iThroughDoor)->get_door_on_other_side());
		--iThroughDoor;
	}
}

void PresencePath::set_through_doors(DoorInRoomArrayPtr const & _throughDoors)
{
	clear_path();
	push_through_doors(_throughDoors);
}

bool PresencePath::update_for_doors_missing()
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

bool PresencePath::check_if_through_door_makes_sense() const
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

bool PresencePath::is_path_valid() const
{
	if (!owner || !target)
	{
		return true;
	}
	Optional<bool> result;
	if (throughDoor.is_empty())
	{
		return owner->get_in_room() == target->get_in_room();
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
	// the doors for sure exist
	scoped_call_stack_info(TXT("is path valid [1]"));
	if (throughDoor.get_first()->get_in_room() == owner->get_in_room())
	{
		scoped_call_stack_info(TXT("is path valid [2]"));
		if (throughDoor.get_last()->get_world_active_room_on_other_side() == target->get_in_room())
		{
			scoped_call_stack_info(TXT("is path valid [3]"));
			return true;
		}
	}
	return false;
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool PresencePath::debug_check_if_path_valid() const
{
	{
		if (is_path_valid())
		{
			return true;
		}
	}
	System::Core::sleep(0.001f); // this is only for debugging, sometimes we may have two characters moving at the same time, give some time to make sure they moved and then go back to checking if path is valid
	{
		if (is_path_valid())
		{
			return true;
		}
	}
	output(TXT("invalid path!"));
	output(TXT("OWNER : %010i %S"), owner->get_in_room(), owner->get_in_room()->get_name().to_char());
	for_every(door, throughDoor)
	{
		if (door->is_set())
		{
			output(TXT("   %02i: %010i [%010i] -> [%010i] %010i   %S"), for_everys_index(door),
				door->get()->get_in_room(),
				door->get(),
				door->get()->get_door_on_other_side(),
				door->get()->get_world_active_room_on_other_side(),
				door->get()->get_in_room() ? door->get()->get_in_room()->get_name().to_char() : TXT("--"));
		}
		else
		{
			output(TXT("   %02i missing!"), for_everys_index(door));
		}
	}
	output(TXT("TARGET: %010i %S"), target->get_in_room(), target->get_in_room()->get_name().to_char());
	return false;
}
#endif

void PresencePath::on_presence_changed_room(ModulePresence* _presence, Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors)
{
	scoped_call_stack_info(TXT("PresencePath::on_presence_changed_room"));
	if (!is_active())
	{
		// doesn't make sense when not active
		return;
	}

	if (update_for_doors_missing())
	{
		return;
	}

	if (_throughDoors && _throughDoors->get_number_of_doors() > 1)
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("on_presence_changed_room"));
#ifdef AN_ASSERT
		++ checksSuppressed;
#endif
		// move through each door
		Framework::DoorInRoom const ** iThroughDoor = _throughDoors->begin(); // it's okay, it's a static thing
		for (int i = 0; i < _throughDoors->get_number_of_doors(); ++ i, ++ iThroughDoor)
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(TXT("through door %i of %i"), i, _throughDoors->get_number_of_doors());
			on_presence_changed_room(_presence, (*iThroughDoor)->get_world_active_room_on_other_side(), (*iThroughDoor)->get_other_room_transform(), cast_to_nonconst(*iThroughDoor), (*iThroughDoor)->get_door_on_other_side(), nullptr);
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

	// if owner and target are the same, we still need to follow as we might be following ourselves through other door, aiming at us
	if (_presence == owner)
	{
		if (throughDoor.is_empty() || throughDoor.get_first() != _exitThrough)
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("moving away [owner]"));

			an_assert_info(throughDoor.is_empty() || _exitThrough->get_in_room() == throughDoor.get_first()->get_in_room());
#ifndef AN_CLANG
#ifdef AN_DEVELOPMENT
			tchar const * uf = userFunction.to_char();
#else
			tchar const* uf = TXT("<no uf>");
#endif
#endif
			//an_assert(throughDoor.get_size() < throughDoor.get_max_size(), TXT("went too far away! %S moved %S->%S (uf:%S)"),
			//	_presence && _presence->get_owner_as_object() ? _presence->get_owner_as_object()->get_name().to_char() : TXT("??"),
			//	owner && owner->get_owner_as_object() ? owner->get_owner_as_object()->get_name().to_char() : TXT("??"),
			//	target && target->get_owner_as_object() ? target->get_owner_as_object()->get_name().to_char() : TXT("??"),
			//	uf
			//	);
			if (throughDoor.get_size() == throughDoor.get_max_size())
			{
				warn(TXT("presence path exceeded size"));
				todo_hack(TXT("lost it, inform somehow?"));
				auto_clear_target();
				auto_find_path(); // maybe there is another path?
				return;
			}
			// we were in same room or we moved away from our final room, we need to remember which door leads us to final room
			throughDoor.push_front(SafePtr<DoorInRoom>(_enterThrough));
		}
		else
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("getting closer [owner]"));

			// we moved towards final room
			throughDoor.pop_front();
		}
		// check it here, as both may move in same time
		an_assert_info(checksSuppressed || throughDoor.is_empty() || (throughDoor.get_first().is_set() && throughDoor.get_first()->get_in_room() == owner->get_in_room()));
	}
	// no else! because we might be pathing to our selves through doors
	if (_presence == target)
	{
		if (throughDoor.is_empty() || throughDoor.get_last() != _enterThrough)
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("moving away [target]"));

			an_assert_info(throughDoor.is_empty() || _exitThrough->get_in_room() == throughDoor.get_last()->get_world_active_room_on_other_side());
#ifndef AN_CLANG
#ifdef AN_DEVELOPMENT
			tchar const * uf = userFunction.to_char();
#else
			tchar const * uf = TXT("<no uf>");
#endif
#endif
			//an_assert(throughDoor.get_size() < throughDoor.get_max_size(), TXT("went too far away! %S moved %S->%S (uf:%S)"),
			//	_presence && _presence->get_owner_as_object() ? _presence->get_owner_as_object()->get_name().to_char() : TXT("??"),
			//	owner && owner->get_owner_as_object() ? owner->get_owner_as_object()->get_name().to_char() : TXT("??"),
			//	target && target->get_owner_as_object() ? target->get_owner_as_object()->get_name().to_char() : TXT("??"),
			//	uf
			//	);
			if (throughDoor.get_size() == throughDoor.get_max_size())
			{
				warn(TXT("presence path exceeded size"));
				todo_hack(TXT("lost it, inform somehow?"));
				auto_clear_target();
				auto_find_path(); // maybe there is another path?
				return;
			}
			// we were in same room or we moved away from our starting room, we need to remember through which door we moved to new final room
			throughDoor.push_back(SafePtr<DoorInRoom>(_exitThrough));
		}
		else
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info(TXT("getting closer [target]"));

			// we moved towards starting room
			throughDoor.pop_back();
		}
		// check it here, as both may move in same time
		an_assert_info(checksSuppressed || throughDoor.is_empty() || (throughDoor.get_last().is_set() && throughDoor.get_last()->get_world_active_room_on_other_side() == target->get_in_room()));
	}

	an_assert_info(checksSuppressed || debug_check_if_path_valid());
}

void PresencePath::on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom * _enteredThroughDoor)
{
	// for owner - we have no idea where we are
	// for target - we have no idea where target is, keep track to it (if we allow that)
	if (_presence == owner)
	{
		clear_path();
	}
	auto_clear_target();
	auto_find_path();
}

void PresencePath::on_presence_removed_from_room(ModulePresence* _presence, Room* _room)
{
	// for owner - we have no idea where we are, where we will be
	// for target - we have no idea where target is, keep track to it (if we allow that)
	if (_presence == owner)
	{
		clear_path();
	}
	auto_clear_target();
}

void PresencePath::on_presence_destroy(ModulePresence* _presence)
{
	if (owner == _presence)
	{
		clear_path();
		set_owner(nullptr);
		set_target(nullptr);
	}
	if (target == _presence)
	{
		auto_clear_target();
	}
}

Transform PresencePath::from_owner_to_target(Transform const & _a) const
{
	an_assert(is_active());
	Transform a = _a;
	for_every_ref(door, throughDoor)
	{
		if (! door)
		{
			return _a;
		}
		a = door->get_other_room_transform().to_local(a);
	}
	return a;
}

Transform PresencePath::from_target_to_owner(Transform const & _a) const
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

Vector3 PresencePath::location_from_owner_to_target(Vector3 const & _a) const
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

Vector3 PresencePath::location_from_target_to_owner(Vector3 const & _a) const
{
	an_assert(is_active());
	Vector3 a = _a;
	for_every_reverse_ref(door, throughDoor)
	{
		if (!door || ! door->get_room_on_other_side())
		{
			return _a;
		}
		a = door->get_other_room_transform().location_to_world(a);
	}
	return a;
}

Vector3 PresencePath::get_target_centre_relative_to_owner() const
{
	an_assert(is_active());
	return owner->get_placement().location_to_local(location_from_target_to_owner(target->get_centre_of_presence_WS()));
}

Vector3 PresencePath::vector_from_owner_to_target(Vector3 const & _a) const
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

Vector3 PresencePath::vector_from_target_to_owner(Vector3 const & _a) const
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

bool PresencePath::is_there_clear_line(Optional<Transform> const & _ownerRelativeOffset, Optional<Transform> const & _targetRelativeOffset, float _maxPtDifference) const
{
	float flatDistance;
	float stringPulledDistance;
	calculate_distances(flatDistance, stringPulledDistance, _targetRelativeOffset, _ownerRelativeOffset);

	float diff = max(0.0f, stringPulledDistance - flatDistance);
	return diff <= stringPulledDistance * max(EPSILON, _maxPtDifference);
}

void PresencePath::calculate_distances(OUT_ float& _flatDistance, OUT_ float& _stringPulledDistance, Optional<Transform> const & _ownerRelativeOffset, Optional<Transform> const & _targetRelativeOffset) const
{
	Transform ownerRelativeOffset = _ownerRelativeOffset.is_set() ? _ownerRelativeOffset.get() : (owner ? owner->get_centre_of_presence_os() : Transform::identity);
	Transform targetRelativeOffset = _targetRelativeOffset.is_set() ? _targetRelativeOffset.get() : (target ? target->get_centre_of_presence_os() : Transform::identity);

	Vector3 ownerLocation = owner->get_placement().to_world(ownerRelativeOffset).get_translation();
	Vector3 targetLocation = location_from_target_to_owner(target->get_placement().to_world(targetRelativeOffset).get_translation());

	_flatDistance = (ownerLocation - targetLocation).length();
	_stringPulledDistance = calculate_string_pulled_distance(_ownerRelativeOffset, _targetRelativeOffset);
}

float PresencePath::calculate_string_pulled_distance(Optional<Transform> const & _ownerRelativeOffset, Optional<Transform> const & _targetRelativeOffset, float _distanceMutliplierOnNotStraight) const
{
	if (!is_active())
	{
		return INF_FLOAT;
	}

	an_assert(owner);
	an_assert(target);

	Transform ownerRelativeOffset = _ownerRelativeOffset.is_set() ? _ownerRelativeOffset.get() : owner->get_centre_of_presence_os();
	Transform targetRelativeOffset = _targetRelativeOffset.is_set() ? _targetRelativeOffset.get() : target->get_centre_of_presence_os();

	// pull owner location towards target through doors
	Vector3 ownerLocation = owner->get_placement().to_world(ownerRelativeOffset).get_translation();
	// target location starts in owner room and moves with each door to end up in target's room
	Vector3 targetLocation = location_from_target_to_owner(target->get_placement().to_world(targetRelativeOffset).get_translation());

	PathStringPulling stringPulling;
	stringPulling.set_start(owner->get_in_room(), ownerLocation);

	Room* targetRoom = owner->get_in_room();
	for_every(doorRef, throughDoor)
	{
		if (!doorRef->is_set() || !doorRef->get())
		{
			return INF_FLOAT; // path was destroyed halfway?
		}
		auto * door = doorRef->get();
		if (door->get_in_room() != targetRoom)
		{
			output(TXT("improper path, doors do not connect correctly"));
			return INF_FLOAT;
		}
		if (! door->get_room_on_other_side())
		{
			//output(TXT("no room on the other side"));
			return INF_FLOAT;
		}
		stringPulling.push_through_door(door);
		targetLocation = door->get_other_room_transform().location_to_local(targetLocation);
		targetRoom = door->get_world_active_room_on_other_side();
	}

	stringPulling.set_end(targetRoom, targetLocation);

	stringPulling.string_pull(2); // enough?

	float distance = 0.0f;
	float distanceMutliplierOnNotStraightSoFar = 1.0f;
	for_every(node, stringPulling.get_path())
	{
		// if we went off the course, modify distance multiplier
		if (_distanceMutliplierOnNotStraight != 1.0f && node->enteredThroughDoor)
		{
			float const dotDiff = Vector3::dot((node->nextNode - node->node), (node->node - node->prevNode));
			if (dotDiff < 0.95f)
			{
				distanceMutliplierOnNotStraightSoFar *= _distanceMutliplierOnNotStraight;
			}
		}
		distance += (node->nextNode - node->node).length() * distanceMutliplierOnNotStraightSoFar;
	}

	return distance;
}

void PresencePath::update_simplify()
{
	if (owner && target && owner->get_in_room() == target->get_in_room())
	{
		clear_path();
	}
}

void PresencePath::update_shortcuts()
{
	if (owner && target)
	{
		for (int i = 0; i < throughDoor.get_size() - 1; ++i)
		{
			if (throughDoor[i].get())
			{
				if (auto* tdiRoom = throughDoor[i]->get_world_active_room_on_other_side())
				{
					for (int n = i + 1; n < throughDoor.get_size(); ++n)
					{
						if (throughDoor[n].get() && tdiRoom == throughDoor[n]->get_world_active_room_on_other_side())
						{
							throughDoor.remove_at(i + 1, n - i);
							--i; // repeat
							break;
						}
					}
				}
				else
				{
					clear_target();
					break;
				}
			}
			else
			{
				clear_target();
				break;
			}
		}
	}
	update_simplify();
}

void PresencePath::log(LogInfoContext & _context) const
{
	if (is_active())
	{
		_context.log(is_active() ? TXT("ACTIVE") : TXT("NOT ACTIVE"));
		_context.log(TXT("path owner: %S -- is in \"%S\""), owner ? owner->get_owner()->ai_get_name().to_char() : TXT("--"), owner ? owner->get_in_room()->get_name().to_char() : TXT("--"));
		_context.log(TXT("path target: %S -- is in \"%S\""), target ? target->get_owner()->ai_get_name().to_char() : TXT("--"), target ? target->get_in_room()->get_name().to_char() : TXT("--"));
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

void PresencePath::be_temporary_snapshot()
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
