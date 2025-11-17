#pragma once

#include "pathSettings.h"

#include "..\interfaces\presenceObserver.h"

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"

namespace Framework
{
	interface_class IModulesOwner;
	struct PresencePath;

	struct RelativeToPresencePlacement
	: public IPresenceObserver
	, public SafeObject<RelativeToPresencePlacement>
	{
		FAST_CAST_DECLARE(RelativeToPresencePlacement);
		FAST_CAST_BASE(IPresenceObserver);
		FAST_CAST_END();
	public:
		static RelativeToPresencePlacement const & empty() { return s_empty; }

		RelativeToPresencePlacement();
		virtual ~RelativeToPresencePlacement();

		RelativeToPresencePlacement(RelativeToPresencePlacement const & _source);
		RelativeToPresencePlacement & operator=(RelativeToPresencePlacement const & _source);
		RelativeToPresencePlacement & operator=(PresencePath const& _path);
		void copy_just_target_placement_from(RelativeToPresencePlacement const & _source);

		void be_temporary_snapshot(); // no observers! do it for one frame calls

		void set_owner(IModulesOwner* _owner);
		void set_owner_presence(ModulePresence* _owner);

		void follow_if_target_lost(bool _followIfTargetLost = true) { followIfTargetLost = _followIfTargetLost; }

		bool is_path_valid() const;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool debug_check_if_path_valid() const; // outputs problems
#endif

		void reset();
		void clear_target();
		void clear_target_but_follow();
		void push_through_door(DoorInRoom const * _door);
		void pop_through_door();
		void set_placement_in_final_room(Transform const & _placementWS, IModulesOwner* _target); // always provide placement in WS, target/relative to
		void set_placement_in_final_room(Transform const & _placementWS, ModulePresence* _target = nullptr); // always provide placement in WS, target/relative to

		bool is_there_clear_line(Optional<Transform> const & _ownerRelativeOffset = NP, Optional<Transform> const & _targetRelativeOffset = NP, float _maxPtDifference = 0.01f) const;
		void calculate_distances(OUT_ float& _flatDistance, OUT_ float& _stringPulledDistance, Optional<Transform> const & _ownerRelativeOffset = NP, Optional<Transform> const & _targetRelativeOffset = NP) const;
		float calculate_string_pulled_distance(Optional<Transform> const & _ownerRelativeOffset = NP, Optional<Transform> const & _targetRelativeOffset = NP) const;
		float get_straight_length() const;

		bool is_active() const { return isActive && finalRoom.is_set(); }
		Room* get_in_final_room() const { an_assert(isActive); return finalRoom.get(); }
		Transform calculate_final_room_transform() const;
		Transform get_placement_in_final_room() const;
		Transform get_placement_in_owner_room() const;
		Vector3 get_direction_towards_placement_in_owner_room() const;
		ModulePresence* get_target_presence() const { return target; }
		IModulesOwner* get_target() const;
		ModulePresence* get_owner_presence() const { return owner; }
		IModulesOwner* get_owner() const;

		DoorInRoom * get_through_door() const { return !throughDoor.is_empty() ? throughDoor.get_first().get() : nullptr; } // first through door, null means - same room
		DoorInRoom* get_through_door_into_final_room() const; // how we ended up in the final room
		ArrayStatic<::SafePtr<DoorInRoom>, PathSettings::MaxDepth> const & get_through_doors() const { return throughDoor; }

		Transform calculate_placement_in_os() const;

		Transform from_owner_to_target(Transform const & _a) const;
		Transform from_target_to_owner(Transform const & _a) const;

		Vector3 location_from_owner_to_target(Vector3 const & _a) const;
		Vector3 location_from_target_to_owner(Vector3 const & _a) const;

		Vector3 get_target_centre_relative_to_owner() const;

		Vector3 vector_from_owner_to_target(Vector3 const & _a) const;
		Vector3 vector_from_target_to_owner(Vector3 const & _a) const;

		bool find_path(IModulesOwner* _owner, IModulesOwner* _target, bool _anyPath /* doesn't have to be sam nav group, useful for general AI perception */ = false, Optional<int> const& _maxDepth = NP);
		bool find_path(IModulesOwner* _owner, Transform const & _placementWS, IModulesOwner* _target = nullptr, bool _anyPath /* doesn't have to be sam nav group, useful for general AI perception */ = false, Optional<int> const& _maxDepth = NP);
		bool find_path(IModulesOwner* _owner, Room* _room, Transform const & _placementWS, bool _anyPath /* doesn't have to be sam nav group, useful for general AI perception */ = false, Optional<int> const& _maxDepth = NP);

		void log(LogInfoContext & _context) const;

	public: // IPresenceObserver
		implement_ void on_presence_changed_room(ModulePresence* _presence, Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors);
		implement_ void on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom * _enteredThroughDoor);
		implement_ void on_presence_removed_from_room(ModulePresence* _presence, Room* _room);
		implement_ void on_presence_destroy(ModulePresence* _presence);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		Name usageInfo;
#endif
	private:
		static RelativeToPresencePlacement s_empty;

#ifdef AN_ASSERT
		int checksSuppressed = 0;
#endif

		ModulePresence* owner;
		bool isActive;
		bool temporarySnapshot = false; // no observing
		::SafePtr<Room> finalRoom;
		ModulePresence* target;
		Optional<Transform> placementInFinalRoomWS;
		Optional<Transform> placementRelative; // relative to target
		ArrayStatic<::SafePtr<DoorInRoom>, PathSettings::MaxDepth> throughDoor;

		bool followIfTargetLost = false; // if we lost target but want to follow to last known location

		inline void clear_path();
		inline void auto_clear_target();

		inline void set_owner_internal(ModulePresence* _owner);
		inline void set_target_internal(ModulePresence* _target, bool _ignorePlacements = false);

		inline void update_final_room();

		void find_path_worker(Room* _current, Room* _end, ArrayStack<DoorInRoom*>& bestWay, ArrayStack<DoorInRoom*>& currentWay, bool _anyPath);

		bool check_if_through_door_makes_sense() const;
		bool update_for_doors_missing(); // returns true if missing and cleared

		friend struct PresencePath;
	};
}

DECLARE_REGISTERED_TYPE(Framework::RelativeToPresencePlacement);
DECLARE_REGISTERED_TYPE(Framework::RelativeToPresencePlacement*);
