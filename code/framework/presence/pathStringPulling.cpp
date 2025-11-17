#include "pathStringPulling.h"

#include "..\world\doorInRoom.h"
#include "..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

PathStringPulling::PathStringPulling()
{
	SET_EXTRA_DEBUG_INFO(path, TXT("PathStringPulling.path"));
	clear();
}

void PathStringPulling::clear()
{
	startingRoom = nullptr;
	endingRoom = nullptr;
	path.clear();
	path.set_size(1);
}

void PathStringPulling::set_start(Room * _startingRoom, Vector3 const & _loc)
{
	startingRoom = _startingRoom;
	startingLocation = _loc;
}

void PathStringPulling::set_end(Room * _endingRoom, Vector3 const & _loc)
{
	endingRoom = _endingRoom;
	endingLocation = _loc;
}

void PathStringPulling::push_through_door(DoorInRoom const * _door)
{
	path.grow_size(1);
	path.get_last().enteredThroughDoor = _door->get_door_on_other_side();
}

void PathStringPulling::string_pull(int _iterations)
{
	an_assert(startingRoom);
	an_assert(endingRoom);
#ifdef AN_DEVELOPMENT
	// check if we have valid path of doors
	Room const* room = startingRoom;
	for_every(node, path)
	{
		an_assert_info(node->enteredThroughDoor || for_everys_index(node) == 0);
		an_assert_info(!node->enteredThroughDoor || node->enteredThroughDoor->get_world_active_room_on_other_side() == room);
		if (node->enteredThroughDoor)
		{
			room = node->enteredThroughDoor->get_in_room();
		}
	}
	an_assert_info(endingRoom == room);
#endif

	/**
	 *	How does it work?
	 *	We first setup base path and then with iterations we try to pull it to have nice tight path.
	 *	First iteration, initial one, to setup base path:
	 *	1. end to start to update next node's location for each node (to point to end location or node location in next node)
	 *	2. start to end to update node's location (we already have end updated, during this pass we update prev node or fill it with starting location)
	 *	Following iterations just go from start to end and update node and then in prev they update next node, this way after each iteration everything is already connected properly
	 */

	// setup initial path
	path.get_last().nextNode = endingLocation;
	update_next_nodes_end_to_start(true);
	// do very first string pulling
	string_pull_nodes_start_to_end(true);

	for_count(int, iteration, _iterations)
	{
		string_pull_nodes_start_to_end();
	}
}

void PathStringPulling::update_next_nodes_end_to_start(bool _initial)
{
	if (_initial)
	{
		Vector3 loc = endingLocation;
		for_every_reverse(node, path)
		{
			node->nextNode = loc;
			if (node->enteredThroughDoor)
			{
				// get into proper room
				loc = node->enteredThroughDoor->get_other_room_transform().location_to_local(loc);
			}
		}
	}
	else
	{
		PathStringPulledNode * next = nullptr;
		for_every_reverse(node, path)
		{
			if (next)
			{
				an_assert(next->enteredThroughDoor);
				node->nextNode = next->enteredThroughDoor->get_other_room_transform().location_to_local(node->node);
			}
			next = node;
		}
	}
}

void PathStringPulling::string_pull_nodes_start_to_end(bool _initial)
{
	Vector3 loc = startingLocation;
	PathStringPulledNode * prev = nullptr;
	for_every(node, path)
	{
		if (node->enteredThroughDoor)
		{
			an_assert(prev);
			an_assert(for_everys_index(node) > 0);
			node->prevNode = node->enteredThroughDoor->get_other_room_transform().location_to_world(loc);
			// we should have prevNode and nextNode set
			node->node = node->enteredThroughDoor->find_inside_hole_for_string_pulling(node->prevNode, node->nextNode, ! _initial? Optional<Vector3>(node->node) : NP);
			prev->nextNode = node->enteredThroughDoor->get_other_room_transform().location_to_local(node->node);
		}
		else
		{
			an_assert(! prev);
			an_assert(for_everys_index(node) == 0);
			node->prevNode = loc;
			node->node = loc;
		}

		// in next step we will use node to update prevNode and keep it going
		loc = node->node;

		prev = node;
	}
}
