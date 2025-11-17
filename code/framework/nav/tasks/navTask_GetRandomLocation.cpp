#include "navTask_GetRandomLocation.h"

#include "..\navMesh.h"
#include "..\navNode.h"
#include "..\navPath.h"

#include "..\nodes\navNode_ConvexPolygon.h"

#include "..\..\world\doorInRoom.h"
#include "..\..\world\room.h"

using namespace Framework;
using namespace Nav;
using namespace Tasks;

//

REGISTER_FOR_FAST_CAST(GetRandomLocation);

GetRandomLocation* GetRandomLocation::new_task(PlacementAtNode const & _start, float _distanceLeft)
{
	return new GetRandomLocation(_start, _distanceLeft);
}

GetRandomLocation::GetRandomLocation(PlacementAtNode const & _start, float _distanceLeft)
: base(false)
, start(_start)
, location(start)
, distanceLeft(_distanceLeft)
{
}

void GetRandomLocation::perform()
{
	CANCEL_NAV_TASK_ON_REQUEST();

	if (!start.node.is_set())
	{
		end_failed();
		return;
	}

	PlacementAtNode at = start;
	Node* prevNode = nullptr;
	while (distanceLeft > 0.0f)
	{
		if (!at.node.is_set())
		{
			break;
		}
		if (auto * dir = at.node->get_door_in_room())
		{
			if (auto * oDir = dir->get_door_on_other_side())
			{
				at.set(oDir->get_nav_door_node().get());
			}
			else
			{
				break;
			}
		}
		// again after door
		if (!at.node.is_set())
		{
			break;
		}
		location = at; // last valid
		ARRAY_STACK(Node::Connection*, connections, at.node->connections.get_size());
		for_every(connection, at.node->connections)
		{
			if (connection->get_to() != prevNode &&
				connection->get_to() &&
				! connection->get_to()->is_outdated())
				// allow blocked temporarily
			{
				connections.push_back(connection);
			}
		}
		if (connections.is_empty())
		{
			break;
		}

		Node::Connection* connection = connections[Random::get_int(connections.get_size())];
		float dist = (connection->get_to()->get_current_placement().get_translation() - at.get_current_placement().get_translation()).length();
		distanceLeft -= dist;
		todo_note(TXT("handle convex polygons"));
		prevNode = at.node.get();
		at.set(connection->get_to());

		CANCEL_NAV_TASK_ON_REQUEST();
	}

	end_success();
}

void GetRandomLocation::cancel_if_related_to(Room* _room)
{
	if (!_room)
	{
		return;
	}

	if (start.node.is_set())
	{
		if (start.node->get_room() == _room)
		{
			cancel();
		}
	}
}
