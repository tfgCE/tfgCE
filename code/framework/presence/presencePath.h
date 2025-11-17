#pragma once

#include "pathSettings.h"

#include "..\interfaces\presenceObserver.h"

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"

namespace Framework
{
	interface_class IModulesOwner;
	struct DoorInRoomArrayPtr;
	struct RelativeToPresencePlacement;

	/**
	 *	Simpler version of RelativeToPresencePlacement (well, today it looks like more complex, but requires both owner and target while RTPP does not require target)
	 *	This is path from one presence to another (or just another room)
	 *	If owner presence is changed abruptly (added/removed) path is conserved as it was - target is when this happens to both owner and target
	 */
	struct PresencePath
	: public IPresenceObserver
	{
		FAST_CAST_DECLARE(PresencePath);
		FAST_CAST_BASE(IPresenceObserver);
		FAST_CAST_END();
	public:
		static PresencePath const & empty() { return s_empty; }

		PresencePath();
		virtual ~PresencePath();

		PresencePath(PresencePath const & _source);
		PresencePath & operator=(PresencePath const & _source);

		void be_temporary_snapshot(); // no observers!

#ifdef AN_DEVELOPMENT
		void set_user_function(tchar const * _userFunction);
#endif

		bool is_there_clear_line(Optional<Transform> const & _ownerRelativeOffset = NP, Optional<Transform> const & _targetRelativeOffset = NP, float _maxPtDifference = 0.01f) const;
		void calculate_distances(OUT_ float& _flatDistance, OUT_ float& _stringPulledDistance, Optional<Transform> const & _ownerRelativeOffset = NP, Optional<Transform> const & _targetRelativeOffset = NP) const;
		float calculate_string_pulled_distance(Optional<Transform> const & _ownerRelativeOffset = NP, Optional<Transform> const & _targetRelativeOffset = NP, float _distanceMutliplierOnNotStraight = 1.0f) const;

		bool is_path_valid() const;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool debug_check_if_path_valid() const; // outputs problems
#endif

		IModulesOwner* get_owner() const;
		IModulesOwner* get_target() const;
		ModulePresence* get_owner_presence() const { return owner; }
		ModulePresence* get_target_presence() const { return target; }
		Room* get_in_final_room() const;
		Transform get_placement_in_final_room() const;
		Transform get_placement_in_owner_room() const;
		bool has_target() const;
		void set_owner(IModulesOwner* _owner);
		void set_target(IModulesOwner* _target);
		void set_owner_presence(ModulePresence* _owner);
		void set_target_presence(ModulePresence* _target);
		void allow_no_target_presence(bool _allow = true);
		void follow_if_target_lost(bool _followIfTargetLost = true, bool _autoFindPathIfLost = false) { followIfTargetLost = _followIfTargetLost; autoFindPathIfLost = _autoFindPathIfLost; }
		void clear_target_but_follow(); // when we want to follow to last known location

		bool find_path(IModulesOwner* _owner, IModulesOwner* _target, bool _anyPath /* doesn't have to be sam nav group, useful for general AI perception */ = false, Optional<int> const & _maxDepth = NP);

		void update_simplify(); // simplifies path if both are in the same room
		void update_shortcuts(); // remove doors that lead to same room

		void reset(); // clears owner too
		void clear_target();
		void push_through_door(DoorInRoom const * _door);
		void pop_through_door();
		void push_through_doors(DoorInRoomArrayPtr const & _throughDoors);
		void push_through_doors_in_reverse(DoorInRoomArrayPtr const & _throughDoors); // when we have doors from target to owner
		void set_through_doors(DoorInRoomArrayPtr const & _throughDoors);
		DoorInRoom * get_through_door() const { return !throughDoor.is_empty() ? throughDoor.get_first().get() : nullptr; } // first through door, null means - same room
		ArrayStatic<SafePtr<DoorInRoom>, PathSettings::MaxDepth> const & get_through_doors() const { return throughDoor; }

		bool is_active() const { return owner && (target != nullptr || allowEmptyTarget); } // if follows just one, there is no path
		bool is_empty() const { return throughDoor.is_empty(); } // if follows both but path is empty

		static bool is_same(PresencePath const & _a, PresencePath const & _b);

		Transform from_owner_to_target(Transform const & _a) const;
		Transform from_target_to_owner(Transform const & _a) const;

		Vector3 location_from_owner_to_target(Vector3 const & _a) const;
		Vector3 location_from_target_to_owner(Vector3 const & _a) const;

		Vector3 get_target_centre_relative_to_owner() const;

		Vector3 vector_from_owner_to_target(Vector3 const & _a) const;
		Vector3 vector_from_target_to_owner(Vector3 const & _a) const;

		void log(LogInfoContext & _context) const;

	public: // IPresenceObserver
		implement_ void on_presence_changed_room(ModulePresence* _presence, Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors);
		implement_ void on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom * _enteredThroughDoor);
		implement_ void on_presence_removed_from_room(ModulePresence* _presence, Room* _room);
		implement_ void on_presence_destroy(ModulePresence* _presence);

	private:
		static PresencePath s_empty;

#ifdef AN_ASSERT
		int checksSuppressed = 0;
#endif

#ifdef AN_DEVELOPMENT
		Name userFunction;
#endif
		bool temporarySnapshot = false; // no observing
		bool allowEmptyTarget = false; // to mark that is active even if no target presence, cleared when setting target
		bool followIfTargetLost = false; // if we lost target but want to follow to last known location
		bool autoFindPathIfLost = false; // always try to find path if lost
		ModulePresence* owner;
		ModulePresence* target;
		ArrayStatic<SafePtr<DoorInRoom>, PathSettings::MaxDepth> throughDoor; // from owner to target, using safe pointers as doors may be removed/disconnected

		inline void clear_path();
		inline void auto_clear_target();
		inline void auto_find_path();
		
		void find_path_worker(Room* _current, Room* _end, ArrayStack<DoorInRoom*>& bestWay, ArrayStack<DoorInRoom*>& currentWay, bool _anyPath);

		bool check_if_through_door_makes_sense() const;
		bool update_for_doors_missing(); // returns true if missing and cleared

		friend struct RelativeToPresencePlacement;
	};
}

DECLARE_REGISTERED_TYPE(Framework::PresencePath);
DECLARE_REGISTERED_TYPE(Framework::PresencePath*);
