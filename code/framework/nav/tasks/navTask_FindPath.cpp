#include "navTask_FindPath.h"

#include "..\navMesh.h"
#include "..\navNode.h"
#include "..\navPath.h"

#include "..\nodes\navNode_ConvexPolygon.h"

#include "..\..\world\doorInRoom.h"
#include "..\..\world\room.h"

#ifdef AN_DEVELOPMENT
//#define AN_DEBUG_FIND_PATH
#endif

#ifdef AN_DEBUG_FIND_PATH
#include "..\..\..\core\debug\debugVisualiser.h"
#endif

#ifdef AN_LOG_NAV_TASKS
#include "..\..\..\core\other\parserUtils.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;
using namespace Tasks;

//

REGISTER_FOR_FAST_CAST(FindPath);

FindPath* FindPath::new_task(PlacementAtNode const & _start, PlacementAtNode const & _end, PathRequestInfo const & _requestInfo)
{
	return new FindPath(_start, _end, _requestInfo);
}

FindPath::FindPath(PlacementAtNode const & _start, PlacementAtNode const & _end, PathRequestInfo const & _requestInfo)
: base(false)
, requestInfo(_requestInfo)
, start(_start)
, end(_end)
{
	requestInfo.useNavFlags.set_if_not_set(Nav::Flags::all());
}

void FindPath::perform()
{
	CANCEL_NAV_TASK_ON_REQUEST();

	if (!start.node.is_set() || !end.node.is_set())
	{
		end_failed();
		return;
	}

	path = Path::get_one();
	path->setup_with(requestInfo);

	int const maxThroughDoors = 512;
	ARRAY_STACK(DoorInRoom*, throughDoors, maxThroughDoors);

	PlacementAtNode currentNode = start;
	PlacementAtNode currentEnd = end;

	if (start.node->get_room() != end.node->get_room() ||
		start.node->get_group() != end.node->get_group())
	{
		if (!add_through_door(start.node->get_room(), start.node->get_group(), end.node->get_room(), end.node->get_group(), throughDoors))
		{
			end_failed();
			return;
		}
		if (requestInfo.withinSameRoomAndNavigationGroup)
		{
			// get as far as the door
			if (!throughDoors.is_empty())
			{
				currentEnd = PlacementAtNode(throughDoors.get_first()->get_nav_door_node().get());
			}

			an_assert(currentEnd.get_room() == currentNode.get_room());

			currentEnd = currentNode.get_room()->find_nav_location(currentEnd.get_current_placement(), nullptr, currentNode.node->get_nav_mesh()->get_id(), currentNode.node->get_group());
			if (!currentEnd.is_valid())
			{
				end_failed();
				return;
			}

			// stay within room
			throughDoors.clear();
		}
	}

	for_every_ptr(throughDoor, throughDoors)
	{
		if (!add_path(currentNode, PlacementAtNode(throughDoor->get_nav_door_node().get())))
		{
			end_failed();
			return;
		}

		// mark that we will be going through door
		an_assert(path->pathNodes.get_last().placementAtNode.node.is_set());
		an_assert(path->pathNodes.get_last().placementAtNode.node->doorInRoom == throughDoor);
		path->pathNodes.get_last().door = throughDoor;

		currentNode.clear();
		if (auto const * diroos = throughDoor->get_door_on_other_side())
		{
			currentNode.set(diroos->get_nav_door_node().get());
		}
		if (!currentNode.is_valid())
		{
			end_failed();
			return;
		}
	}

	if (!requestInfo.withinSameNavigationGroup &&
		!requestInfo.withinSameRoomAndNavigationGroup &&
		currentNode.get_room() &&
		currentNode.node.get() && currentEnd.node.get() &&
		currentNode.node->get_group() != currentEnd.node->get_group())
	{
		// we're in a different group, try to find the closest point within our group
		auto newEnd = currentNode.get_room()->find_nav_location(currentEnd.get_current_placement(), nullptr, Name::invalid(), currentNode.node->get_group());
		if (newEnd.is_valid())
		{
			currentEnd = newEnd;
		}
	}
	if (!add_path(currentNode, currentEnd))
	{
		end_failed();
		return;
	}

	path->post_process();

	end_success();
}

void FindPath::cancel_if_related_to(Room* _room)
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

	if (end.node.is_set())
	{
		if (end.node->get_room() == _room)
		{
			cancel();
		}
	}
}

#ifdef AN_DEBUG_FIND_PATH
DebugVisualiserPtr dv;
#endif

bool FindPath::add_path(PlacementAtNode const & _start, PlacementAtNode const & _end)
{
	if (!_start.node.is_set() ||
		_start.node->is_outdated() ||
		!_end.node.is_set() ||
		_end.node->is_outdated())
	{
		return false;
	}

#ifdef AN_DEBUG_FIND_PATH
	dv = new DebugVisualiser(String(TXT("nav find path")));
	dv->activate();
#endif

	ARRAY_STACK(PathNodeWorker, bestWay, _start.node->get_nav_mesh()->get_nodes().get_size());
	ARRAY_STACK(PathNodeWorker, currentWay, _start.node->get_nav_mesh()->get_nodes().get_size());
	currentWay.push_back(PathNodeWorker(_start.node.get()));
	float bestDist = 0.0f;
	float currentDist = 0.0f;
	add_path_worker(bestWay, bestDist, currentWay, currentDist, _end);

	if (bestWay.is_empty())
	{
		return false;
	}
	an_assert(bestWay.get_last().node == _end.node.get());

	for_every(bestWayElement, bestWay)
	{
		if (bestWayElement->node == _start.node.get())
		{
			an_assert(bestWayElement->connectionData == nullptr);
			PathNode pathNode;
			pathNode.placementAtNode = _start;
			if (_start.placementLS.is_set() &&
				!_start.placementLS.get().get_translation().is_almost_zero())
			{
				path->pathNodes.push_back(pathNode);
				pathNode.placementAtNode.placementLS.clear();
			}
			path->pathNodes.push_back(pathNode);
		}
		if (bestWayElement->node == _end.node.get())
		{
			PathNode pathNode;
			pathNode.placementAtNode = _end;
			pathNode.connectionData = bestWayElement->connectionData;
			if (_end.placementLS.is_set() &&
				!_end.placementLS.get().get_translation().is_almost_zero())
			{
				// point leading to end
				pathNode.placementAtNode.placementLS.clear();
				path->pathNodes.push_back(pathNode);
				pathNode = PathNode();
				pathNode.placementAtNode = _end;
			}
			path->pathNodes.push_back(pathNode);
		}
		if (bestWayElement->node != _start.node.get() &&
			bestWayElement->node != _end.node.get())
		{
			PathNode pathNode;
			pathNode.placementAtNode.set(bestWayElement->node);
			pathNode.connectionData = bestWayElement->connectionData;
			path->pathNodes.push_back(pathNode);
		}
	}

	return true;
}

struct ConnectionWithDirAlignment
{
	int connectionIdx;
	float dirAlignment;
	float dist;
	Transform toPlacement;
	ConnectionWithDirAlignment() {}
	ConnectionWithDirAlignment(int _connectionIdx, Transform const & _toPlacement, float _dist, float _dirAlignment) : connectionIdx(_connectionIdx), dirAlignment(_dirAlignment), dist(_dist), toPlacement(_toPlacement) {}

	static int compare(void const * _a, void const * _b)
	{
		ConnectionWithDirAlignment const * a = (ConnectionWithDirAlignment const *)(_a);
		ConnectionWithDirAlignment const * b = (ConnectionWithDirAlignment const *)(_b);
		float diff = a->dist - b->dist;
		return diff == 0.0f? 0 : (diff > 0.0f? -1 : 1); // sort from highest to lowest
	}
};

void FindPath::add_path_worker(ArrayStack<PathNodeWorker> & bestWay, float & bestDist, ArrayStack<PathNodeWorker> & currentWay, float & currentDist, PlacementAtNode const & _end) const
{
#ifdef AN_DEBUG_FIND_PATH
	dv->start_gathering_data();
	dv->clear();
	{
		Optional<Vector2> prev;
		for_every(bestWayElement, bestWay)
		{
			if (bestWayElement->node)
			{
				Vector2 curr = bestWayElement->node->get_current_placement().get_translation().to_vector2();
				if (prev.is_set())
				{
					dv->add_arrow(prev.get(), curr, Colour::red.mul_rgb(0.5f));
				}
				prev = curr;
			}
		}
	}
	{
		Optional<Vector2> prev;
		for_every(currentWayElement, currentWay)
		{
			if (currentWayElement->node)
			{
				Vector2 curr = currentWayElement->node->get_current_placement().get_translation().to_vector2();
				if (prev.is_set())
				{
					dv->add_arrow(prev.get(), curr, Colour::green.mul_rgb(0.5f));
				}
				prev = curr;
			}
		}
	}
	dv->add_circle(_end.get_current_placement().get_translation().to_vector2(), 0.1f, Colour::orange);
	
	dv->end_gathering_data();
	dv->show_and_wait_for_key();
#endif

	Node* currentNode = currentWay.get_last().node;
	if (currentNode == _end.node.get())
	{
		if (bestWay.is_empty() || currentDist < bestDist)
		{
			bestDist = currentDist;
			bestWay = currentWay;
		}
		return;
	}

	if (!bestWay.is_empty() && currentDist > bestDist)
	{
		// doesn't make sense to look for a worse path
		return;
	}

	Transform currentPlacement = currentNode->get_current_placement();
	Transform endPlacement = _end.get_current_placement();
	Vector3 current2endDir = (endPlacement.get_translation() - currentPlacement.get_translation()).normal();

	// check all connections
	ARRAY_STACK(ConnectionWithDirAlignment, connectionsByDirAlignment, currentNode->connections.get_size());
	for_every(connection, currentNode->connections)
	{
		Node* to = connection->get_to();
		if (!Nav::Flags::check(to->get_flags(), requestInfo.useNavFlags.get()))
		{
			continue; // skip because of flags
		}
		{
			bool alreadyAdded = false;
			for_every(currentWayElement, currentWay)
			{
				if (currentWayElement->node == to)
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
			for_every(bestWayElement, bestWay)
			{
				if (bestWayElement->node == to)
				{
					if (bestWayElement->dist < currentDist)
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

		Transform toPlacement = to->get_current_placement();
		Vector3 toTo = (toPlacement.get_translation() - currentPlacement.get_translation());
		float dirAlignment = Vector3::dot(toTo.normal(), current2endDir);
		float length = toTo.length();// use pure length *(1.0f - 0.5f * (1.0f - dirAlignment)); // longer if sideways
		connectionsByDirAlignment.push_back(ConnectionWithDirAlignment(for_everys_index(connection), toPlacement, length, dirAlignment));
	}

	sort(connectionsByDirAlignment);
	
	for_every(cwda, connectionsByDirAlignment)
	{
		auto* connection = &currentNode->connections[cwda->connectionIdx];
		
		//
		Node* to = connection->get_to();
		Transform toPlacement = cwda->toPlacement;

		float storedCurrentDist = currentDist;
		currentDist += cwda->dist;
		currentWay.push_back(PathNodeWorker(to, connection->get_data(), currentDist));
		add_path_worker(bestWay, bestDist, currentWay, currentDist, _end);
		currentWay.pop_back();
		currentDist = storedCurrentDist;
	}
}

bool FindPath::add_through_door(Room* _start, int _startNavGroup, Room* _end, int _endNavGroup, ArrayStack<DoorInRoom*> & throughDoors)
{
	if (!_start ||
		!_end)
	{
		return false;
	}

	if (_start == _end)
	{
		return true;
	}

	Array<ThroughDoorWorker> bestWay;
	Array<ThroughDoorWorker> currentWay;
	float bestDist = 0.0f;
	float currentDist = 0.0f;
	add_through_door_worker(_start, _startNavGroup, _end, _endNavGroup, bestWay, bestDist, currentWay, currentDist);

	if (bestWay.is_empty())
	{
		// if the same room, it is handled above
		// getting here means that we either can't find a path or that there are different nav groups

		// try to just get to the same room, using a different end nav group
		
		// reset results first
		bestWay.clear();
		currentWay.clear();
		bestDist = 0.0f;
		currentDist = 0.0f;
		add_through_door_worker(_start, _startNavGroup, _end, NP, bestWay, bestDist, currentWay, currentDist);
		if (bestWay.is_empty())
		{
			return false;
		}
	}

	for_every(bestWayElement, bestWay)
	{
		throughDoors.push_back(bestWayElement->throughDoor);
	}

	return true;
}

void FindPath::add_through_door_worker(Room* _current, int _currentNavGroup, Room* _end, Optional<int> const & _endNavGroup, Array<ThroughDoorWorker> & bestWay, float & bestDist, Array<ThroughDoorWorker> & currentWay, float & currentDist) const
{
	if (_current == _end &&
		_currentNavGroup == _endNavGroup.get(_currentNavGroup)) // if _endNavGroup is not defined, we're good with getting andy door that matches the room
	{
		if (bestWay.is_empty() || currentDist < bestDist)
		{
			bestDist = currentDist;
			bestWay = currentWay;
		}
		return;
	}

	DoorInRoom* throughDoorInCurrent = nullptr;
	if (!currentWay.is_empty())
	{
		throughDoorInCurrent = currentWay.get_last().throughDoor->get_door_on_other_side();
		an_assert(_currentNavGroup == throughDoorInCurrent->get_nav_door_node()->get_group());
	}

	for_every_ptr(dir, _current->get_all_doors())
	{
		if (dir == throughDoorInCurrent ||
			!dir->can_move_through() ||
			!dir->is_visible() ||
			!dir->is_relevant_for_movement())
		{
			continue;
		}
		if (! requestInfo.throughClosedDoors &&
			dir->get_door()->get_operation() == DoorOperation::StayClosed)
		{
			// won't get this way
			continue;
		}
		if (throughDoorInCurrent &&
			throughDoorInCurrent->get_nav_door_node().is_set() &&
			dir->get_nav_door_node().is_set() &&
			throughDoorInCurrent->get_nav_door_node()->get_group() != dir->get_nav_door_node()->get_group())
		{
			// not connected directly
			continue;
		}
		if (dir->get_nav_door_node().is_set() &&
			_currentNavGroup != dir->get_nav_door_node()->get_group())
		{
			// not connected directly
			continue;
		}
		auto* dirOnOtherSide = dir->get_door_on_other_side();
		if (!dirOnOtherSide || !dirOnOtherSide->get_nav_door_node().is_set())
		{
			// no path there, we skip such door
			continue;
		}
		if (auto* r = dir->get_room_on_other_side())
		{
			if (r->should_be_avoided_to_navigate_into())
			{
				// we shouldn't try to navigate there
				continue;
			}
		}
		{
			bool alreadyAdded = false;
			for_every(currentWayElement, currentWay)
			{
				if (currentWayElement->throughDoor->get_in_room() == dir->get_world_active_room_on_other_side() &&
					currentWayElement->throughDoor->get_nav_door_node().is_set() &&
					currentWayElement->throughDoor->get_nav_door_node()->get_group() == dirOnOtherSide->get_nav_door_node()->get_group())
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
			for_every(bestWayElement, bestWay)
			{
				if (bestWayElement->throughDoor->get_in_room() == dir->get_world_active_room_on_other_side() &&
					bestWayElement->throughDoor->get_nav_door_node().is_set() &&
					bestWayElement->throughDoor->get_nav_door_node()->get_group() == dirOnOtherSide->get_nav_door_node()->get_group())
				{
					if (bestWayElement->dist < currentDist)
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
		if (dir->has_world_active_room_on_other_side())
		{
			float storedCurrentDist = currentDist;
			currentDist += throughDoorInCurrent? (dir->get_placement().get_translation() - throughDoorInCurrent->get_placement().get_translation()).length() : 1.0f;
			currentWay.push_back(ThroughDoorWorker(dir, currentDist));
			add_through_door_worker(dir->get_world_active_room_on_other_side(), dirOnOtherSide->get_nav_door_node()->get_group(), _end, _endNavGroup, bestWay, bestDist, currentWay, currentDist);
			currentWay.pop_back();
			currentDist = storedCurrentDist;
		}
	}
}

#ifdef AN_LOG_NAV_TASKS
void FindPath::log_internal(LogInfoContext& _log) const
{
	base::log_internal(_log);

	/*
	_log.log(TXT("flags %S"), requestInfo.useNavFlags.get(Nav::Flags::none()).to_string().to_char());

	_log.log(TXT("start"));
	start.log(_log);

	_log.log(TXT("end"));
	end.log(_log);
	*/
}
#endif
