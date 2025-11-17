#pragma once

#include "..\..\core\globalDefinitions.h"

struct Vector3;

namespace Framework
{
	interface_class IModulesOwner;
	class DoorInRoom;
	class Room;
	struct InRoomPlacement;

	class RoomUtils
	{
	public:
		static DoorInRoom* find_closest_door(Room* _room, Vector3 const & _loc);
		static DoorInRoom* find_furthest_door(Room* _room, Vector3 const & _loc);
		static DoorInRoom* get_random_door(Room* _room, DoorInRoom* _butNotThisOne = nullptr, bool _canBeTheSame = true); // unless it is the only door
		static DoorInRoom* get_door_towards(Room* _room, IModulesOwner* _forOwner, InRoomPlacement const& _inRoomPlacement);
	};
};
