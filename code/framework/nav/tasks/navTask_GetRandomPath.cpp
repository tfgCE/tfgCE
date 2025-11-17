#include "navTask_GetRandomPath.h"

#include "..\navMesh.h"
#include "..\navNode.h"
#include "..\navPath.h"

#include "..\nodes\navNode_ConvexPolygon.h"

#include "..\..\world\doorInRoom.h"
#include "..\..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;
using namespace Tasks;

//

REGISTER_FOR_FAST_CAST(GetRandomPath);

GetRandomPath* GetRandomPath::new_task(PlacementAtNode const & _start, float _distanceLeft, Optional<Vector3> const & _startInDir, PathRequestInfo const & _requestInfo)
{
	return new GetRandomPath(_start, _distanceLeft, _startInDir, _requestInfo);
}

GetRandomPath::GetRandomPath(PlacementAtNode const & _start, float _distanceLeft, Optional<Vector3> const & _startInDir, PathRequestInfo const & _requestInfo)
: base(false)
, requestInfo(_requestInfo)
, start(_start)
, location(start)
, distanceLeft(_distanceLeft)
, startInDir(_startInDir)
{
}

void GetRandomPath::perform()
{
	CANCEL_NAV_TASK_ON_REQUEST();

	if (!start.node.is_set())
	{
		end_failed();
		return;
	}

	path = Path::get_one();
	path->setup_with(requestInfo);

	PlacementAtNode at = start;
	{
		PathNode pathNode;
		pathNode.placementAtNode = at;
		an_assert(pathNode.placementAtNode.node.is_set());
		path->pathNodes.push_back(pathNode);
	}
	Node* prevNode = nullptr;
	while (distanceLeft > 0.0f)
	{
		if (!at.node.is_set())
		{
			break;
		}
		// when last one was door, get through that door
		if (auto * dir = at.node->get_door_in_room())
		{
			if (auto* door = dir->get_door())
			{
				if (door->is_open() || (requestInfo.throughClosedDoors || door->get_operation() != DoorOperation::StayClosed))
				{
					if (auto * oDir = dir->get_door_on_other_side())
					{
						if (auto* node = oDir->get_nav_door_node().get())
						{
							at.set(node);
							{
								PathNode pathNode;
								pathNode.placementAtNode = at;
								an_assert(pathNode.placementAtNode.node.is_set());
								path->pathNodes.push_back(pathNode);
							}
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					break;
				}
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
		ARRAY_STACK(Node::Connection*, connections, at.node->connections.get_size() + 1); // extra one for current node
		for_every(connection, at.node->connections)
		{
			if (connection->get_to() != prevNode &&
				connection->get_to() &&
				! connection->get_to()->is_outdated())
				// allow blocked temporarily
			{
				if (!Nav::Flags::check(connection->get_to()->get_flags(), requestInfo.useNavFlags.get()))
				{
					continue; // skip because of flags
				}
				if (auto* dir = connection->get_to()->get_door_in_room())
				{
					if (auto* r = dir->get_room_on_other_side())
					{
						if (r->should_be_avoided_to_navigate_into())
						{
							continue; // we shouldn't move there
						}
					}
				}
				if (!requestInfo.withinSameRoomAndNavigationGroup ||
					!connection->get_to()->get_door_in_room())
				{
					bool alreadyMovedThroughNode = false;
					for_every(pn, path->pathNodes)
					{
						if (connection->get_to() == pn->placementAtNode.node.get())
						{
							alreadyMovedThroughNode = true;
							break;
						}
					}
					if (!alreadyMovedThroughNode)
					{
						// check if we won't end with something that will immediately block us, don't bother then
						bool willGoLoops = true;
						if (connection->get_to()->connections.get_size() == 1)
						{
							willGoLoops = false; // but accept dead ends
						}
						else
						{
							for_every(nextConnection, connection->get_to()->connections)
							{
								bool alreadyMovedThroughNode = false;
								for_every(pn, path->pathNodes)
								{
									if (nextConnection->get_to() == pn->placementAtNode.node.get())
									{
										alreadyMovedThroughNode = true;
										break;
									}
								}
								if (!alreadyMovedThroughNode)
								{
									willGoLoops = false;
								}
							}
						}
						if (!willGoLoops)
						{
							connections.push_back(connection);
						}
					}
				}
			}
		}
		if (connections.get_size() == 1)
		{
			if (auto* atCPNode = fast_cast<Nodes::ConvexPolygon>(at.node.get()))
			{
				if (atCPNode->calculate_greater_radius() > 5.0f)
				{
					connections.push_back(nullptr); // current node!
				}
			}
		}
		if (connections.is_empty())
		{
			break;
		}

		Node::Connection* connection = connections[Random::get_int(connections.get_size())];
		if (!prevNode && startInDir.is_set())
		{
			float best = 0.0f;
			connection = nullptr;
			Vector3 againstDir = startInDir.get().normal();
			for_every_ptr(c, connections)
			{
				Vector3 dir = (c->get_to()->get_current_placement().get_translation() - at.get_current_placement().get_translation()).normal();
				float dot = Vector3::dot(dir, againstDir);
				if (dot >= best || !connection)
				{
					best = dot;
					connection = c;
				}
			}
		}
		float dist = 0.0f;
		if (connection)
		{
			dist = (connection->get_to()->get_current_placement().get_translation() - at.get_current_placement().get_translation()).length();
			at.set(connection->get_to());
		}
		else
		{
			dist = Random::get_float(3.0f, 8.0f);
			auto* atCPNode = fast_cast<Nodes::ConvexPolygon>(at.node.get());
			an_assert(atCPNode);
			Vector3 found;
			Transform foundPlacement = at.get_current_placement();
			atCPNode->find_location(foundPlacement.get_translation() + (Vector3::yAxis * dist).rotated_by_yaw(Random::get_float(0.0f, 360.0f)), found, dist);
			foundPlacement.set_translation(found);
			at.set_placement(foundPlacement);
		}
		distanceLeft -= dist;
		todo_note(TXT("handle convex polygons"));
		prevNode = at.node.get();

		{
			PathNode pathNode;
			pathNode.placementAtNode = at;
			if (connection)
			{
				pathNode.connectionData = connection->get_data();
			}
			else
			{
				pathNode.connectionData.clear();
			}
			pathNode.door = at.node->get_door_in_room();
			an_assert(pathNode.placementAtNode.node.is_set());
			path->pathNodes.push_back(pathNode);
		}

		CANCEL_NAV_TASK_ON_REQUEST();
	}

	path->post_process();

	end_success();
}

void GetRandomPath::cancel_if_related_to(Room* _room)
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
