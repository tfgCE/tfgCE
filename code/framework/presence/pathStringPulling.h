#pragma once

#include "pathSettings.h"

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\math\math.h"

namespace Framework
{
	class DoorInRoom;
	class Room;

	struct PathStringPulledNode
	{
		DoorInRoom const * enteredThroughDoor = nullptr; // through which door we have entered into THIS room (all nodes are in this room's space)
		OUT_ Vector3 node; // for first one it is start, for others this is point on "enteredThroughDoor"
		OUT_ Vector3 prevNode; // prev node in this room space if not first one
		OUT_ Vector3 nextNode; // next node in this room space or end if last one
		// node -> nextNode is always valid
	};

	/**
	 *	This struct is just to string pull path, does not observe it or use safe pointers
	 *	It is just for very temporary action, calculating distance etc
	 */
	struct PathStringPulling
	{
	public:
		PathStringPulling();

	public: // setup
		void clear();

		void set_start(Room * _startingRoom, Vector3 const & _loc);
		void set_end(Room* _endingRoom, Vector3 const & _loc);

		void push_through_door(DoorInRoom const * _door); // in direction from start to end

	public:	// string pulling and results
		void string_pull(int _iterations = 5);

		ArrayStatic<PathStringPulledNode, PathSettings::MaxDepth + 1> const & get_path() const { return path; }

		Room* get_starting_room() const { return startingRoom; }

	private:
		Room* startingRoom = nullptr;
		Room* endingRoom = nullptr;
		Vector3 startingLocation;
		Vector3 endingLocation;

		ArrayStatic<PathStringPulledNode, PathSettings::MaxDepth + 1 /*first till door*/> path; // from start to end, at least one element if both are in the same room

		inline void update_next_nodes_end_to_start(bool _initial = false);
		inline void string_pull_nodes_start_to_end(bool _initial = false);

	};
}