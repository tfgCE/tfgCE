#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\globalDefinitions.h"
#include "..\..\core\concurrency\spinLock.h"

struct Transform;

namespace Framework
{
	class DoorInRoom;
	class ModulePresence;
	class Room;
	struct DoorInRoomArrayPtr;

	interface_class IPresenceObserver
	{
		FAST_CAST_DECLARE(IPresenceObserver);
		FAST_CAST_END();
	public:
		IPresenceObserver() {}
		virtual ~IPresenceObserver() {}

	public:
		// moved to different room, exit from current room through _exitThrough and entered into new one through _enterThrough
		virtual void on_presence_changed_room(ModulePresence* _presence, Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors) {}
		// we were added to room but not moved from different one (teleported?)
		virtual void on_presence_added_to_room(ModulePresence* _presence, Room* _room, DoorInRoom * _enteredThroughDoor) {}
		// we were removed from room but not moved to different one (teleported? died?)
		virtual void on_presence_removed_from_room(ModulePresence* _presence, Room* _room) {}
		// remember that storing as safe ptr may result in nullptr value when accessing on destruction - it's better to store as a direct link
		virtual void on_presence_destroy(ModulePresence* _presence) = 0;

		Concurrency::SpinLock& access_presence_observer_lock() { return presenceObserverLock; }

	private:
		Concurrency::SpinLock presenceObserverLock;
	};
};
