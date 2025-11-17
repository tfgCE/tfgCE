#include "navPath.h"

#include "navNode.h"

#include "nodes\navNode_ConvexPolygon.h"
#include "nodes\navNode_Point.h"

#include "..\module\moduleCollision.h"

#include "..\world\doorInRoom.h"
#include "..\world\room.h"

#include "..\..\core\debug\debugRenderer.h"

#ifdef AN_DEVELOPMENT
//#define AN_DEBUG_STRING_PULL_PATH
#endif

#ifdef AN_DEBUG_STRING_PULL_PATH
#include "..\..\core\debug\debugVisualiser.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;

//

bool PathNode::is_outdated() const
{
	if (placementAtNode.is_valid())
	{
		return placementAtNode.node->is_outdated();
	}
	else
	{
		return true;
	}
}

bool PathNode::can_be_string_pulled() const
{
	if (stringPulled) return true;
	if (auto* cp = fast_cast<Nodes::ConvexPolygon>(placementAtNode.node.get()))
	{
		if (!placementAtNode.placementLS.is_set())
		{
			return true;
		}
	}

	return false;
}

//

PathRequestInfo& PathRequestInfo::collision_radius_for(IModulesOwner const* imo)
{
	if (imo)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		devOwner = imo;
#endif
		if (auto* c = imo->get_collision())
		{
			collisionRadius = c->get_radius_for_gradient();

			return *this;
		}
	}
	warn(TXT("owner not provided for path task"));
	return *this;
}

//

Path::Path()
{
}

void Path::on_get()
{
}

void Path::on_release()
{
	pathNodes.clear();
	collisionRadius = 0.25f;
	transportPath = false;
}

void Path::setup_with(PathRequestInfo& _pathRequestInfo)
{
	collisionRadius = _pathRequestInfo.collisionRadius;
	transportPath = _pathRequestInfo.transportPath;
	throughClosedDoors = _pathRequestInfo.throughClosedDoors;
}

int Path::find_next_node(Room* _room, Vector3 const & _loc, OUT_ OPTIONAL_ Vector3* _onPath) const
{
	Nav::PathNode const * prev = nullptr;
	int closestIdx = NONE;
	float closestDist = 0.0f;
	Vector3 closestLoc = Vector3::zero;
	int closestNodeIdx = NONE;
	float closestNodeDist = 0.0f;
	Vector3 closestNodeLoc = Vector3::zero;
	for_every(pathNode, get_path_nodes())
	{
		if (pathNode->is_outdated())
		{
			continue;
		}

		if (pathNode->placementAtNode.get_room() == _room)
		{
			Vector3 currLoc = pathNode->placementAtNode.get_current_placement().get_translation();
			// node - always
			{
				float dist = (currLoc - _loc).length();
				if (closestNodeIdx == NONE || dist < closestNodeDist)
				{
					closestNodeIdx = for_everys_index(pathNode);
					closestNodeDist = dist;
					closestNodeLoc = currLoc;
				}
			}
			// path leg if not blocked temporarily
			if (prev &&
				prev->placementAtNode.get_room() == pathNode->placementAtNode.get_room())
			{
				if (!pathNode->connectionData.is_set() ||
					!pathNode->connectionData->is_blocked_temporarily())
				{
					Vector3 prevLoc = prev->placementAtNode.get_current_placement().get_translation();
					Segment seg(prevLoc, currLoc);
					Vector3 atSegLoc = seg.get_at_t(seg.get_t(_loc));
					float dist = (atSegLoc - _loc).length();
					if (closestIdx == NONE || dist < closestDist)
					{
						closestIdx = for_everys_index(pathNode);
						closestDist = dist;
						closestLoc = atSegLoc;
					}
				}
			}
		}
		prev = pathNode;
	}

	if (closestIdx == NONE || closestNodeDist < closestDist)
	{
		// get closest node instead of path leg
		closestIdx = closestNodeIdx;
		closestLoc = closestNodeLoc;
	}

	assign_optional_out_param(_onPath, closestLoc);

	return closestIdx;
}

bool Path::find_forward_location(Room* _room, Vector3 const & _loc, float _distance, OUT_ Vector3 & _found, OUT_ OPTIONAL_ Vector3* _onPath, OUT_ OPTIONAL_ bool* _hitTemporaryBlock) const
{
	int nextPathIdx = find_next_node(_room, _loc, OUT_ _onPath);

	if (nextPathIdx == NONE)
	{
		return false;
	}

	Room* room = _room;
	Transform intoRoom = Transform::identity;
	Vector3 currentLoc = _loc;

	assign_optional_out_param(_hitTemporaryBlock, false);

	{
		float distLeft = _distance;

		int toIdx = nextPathIdx;
		Nav::PathNode const * prev = toIdx > 0? &get_path_nodes()[toIdx - 1] : nullptr;

		while (distLeft > 0.0f)
		{
			Vector3 to = Vector3::zero;
			Nav::PathNode const * pathNode = get_path_nodes().is_index_valid(toIdx)? &get_path_nodes()[toIdx] : nullptr;

			if (pathNode &&
				pathNode->connectionData.is_set() &&
				pathNode->connectionData->is_blocked_temporarily())
			{
				if (distLeft >= _distance - 0.15f)
				{
					assign_optional_out_param(_hitTemporaryBlock, true);
				}
				// we should stop here, just move a tiny bit forward
				distLeft = min(distLeft, 0.05f);
			}

			if (pathNode && pathNode->placementAtNode.get_room() == room)
			{
				if (toIdx == 0)
				{
					if (!pathNode->is_outdated())
					{
						to = pathNode->placementAtNode.get_current_placement().get_translation() - currentLoc;
					}
				}
				else if (pathNode && pathNode->placementAtNode.get_room() == prev->placementAtNode.get_room())
				{
					if (pathNode->placementAtNode.get_room() != room)
					{
						// tough luck, we're in a different room, we should try our luck next time, mark that we should not be trusted
						break;
					}
					to = pathNode->placementAtNode.get_current_placement().get_translation() - currentLoc;
				}
			}
			if (! pathNode)
			{
				// end of path!
				break;
			}

			if (!to.is_zero())
			{
				float length = to.length();
				if (distLeft <= length)
				{
					Vector3 toNormal = to.normal();
					currentLoc = currentLoc + toNormal * distLeft;
					break;
				}
				distLeft -= length;
				currentLoc = currentLoc + to;
			}

			// switch to another room if required
			if (pathNode &&
				pathNode->door &&
				pathNode->door->has_world_active_room_on_other_side())
			{
				room = pathNode->door->get_world_active_room_on_other_side();
				currentLoc = pathNode->door->get_other_room_transform().location_to_local(currentLoc);
				intoRoom = intoRoom.to_world(pathNode->door->get_other_room_transform());
			}
			++toIdx;

			prev = pathNode;
		}
	}

	_found = intoRoom.location_to_world(currentLoc); // get current loc that is in room to be in our room
	return true;
}

void Path::post_process()
{
	// string pull path
	// remove doubled polygons, if we don't have actual location on them, we only need one instance of such polygon then
	for (int i = 1; i < pathNodes.get_size(); ++i)
	{
		auto const & prev = pathNodes[i - 1];
		auto const & curr = pathNodes[i];
		if (curr.placementAtNode.node == prev.placementAtNode.node)
		{
			// P - polygon
			bool prevP = fast_cast<Nodes::ConvexPolygon>(prev.placementAtNode.node.get()) != nullptr;
			bool currP = fast_cast<Nodes::ConvexPolygon>(curr.placementAtNode.node.get()) != nullptr;
			// JP - just polygon (no placement provided)
			bool prevJP = prevP && !prev.placementAtNode.placementLS.is_set();
			bool currJP = currP && !curr.placementAtNode.placementLS.is_set();
			if (currJP &&
				prevJP)
			{
				// remove doubled
				pathNodes.remove_at(i);
				--i;
				continue;
			}
		}
	}

	// for time being, just remove singular convex polygons - in future we should check if prev and next are inside polygon
	// this also works if we go from one point (entrance) to the other point (exit) through a polygon - we should then skip the polygon's centre (this is done later

	//for_count(int, iteration, 10)
	{
#ifdef AN_DEBUG_STRING_PULL_PATH
		DebugVisualiserPtr dv;
		dv = new DebugVisualiser(String(TXT("string pull")));
		dv->activate();
#endif
		int lastNonSP = 0;
#ifdef AN_DEBUG_STRING_PULL_PATH
		dv->start_gathering_data();
		dv->clear();
		for (int i = 0; i < pathNodes.get_size() - 1; ++i)
		{
			dv->add_line(pathNodes[i].placementAtNode.get_current_placement().get_translation().to_vector2(),
						 pathNodes[i + 1].placementAtNode.get_current_placement().get_translation().to_vector2(), Colour::red.mul_rgb(0.4f));
		}
		dv->end_gathering_data();
		dv->show_and_wait_for_key();
#endif

		for(int i = 0; i < pathNodes.get_size() - 1; ++i)
		{
			if (pathNodes[i].can_be_string_pulled())
			{
				int nextNonSP = pathNodes.get_size() - 1;
				// find first following that can't be string pulled
				for (int j = i + 1; j < pathNodes.get_size(); ++j)
				{
					if (!pathNodes[j].can_be_string_pulled())
					{
						nextNonSP = j;
						break;
					}
				}

				// look between if we have enough edges
				// we start with this
				// polygon 0	SET LOCATION - NON STRING PULL
				// polygon 0	not set - centre
				// polygon 1	not set - centre
				// polygon 2	not set - centre
				// polygon 2	SET LOCATION - NON STRING PULL
				// we actually should have this:
				// polygon 0	NON STRING PULL
				// polygon 1 on the edge between 0 and 1
				// polygon 2 on the edge between 1 and 2
				// polygon 2	NON STRING PULL
				// we need to make sure we do not double the first polygon (0) and we double the last polygon (2)

				if (pathNodes[lastNonSP + 1].placementAtNode.node
					==
					pathNodes[lastNonSP].placementAtNode.node)
				{
					pathNodes.remove_at(lastNonSP + 1);
					nextNonSP -= 1;
				}
				if (pathNodes[nextNonSP - 1].placementAtNode.node
					!=
					pathNodes[nextNonSP].placementAtNode.node)
				{
					auto& nSP = pathNodes[nextNonSP];
					PathNode pn = nSP;
					pn.placementAtNode.placementLS.clear();
					pathNodes.insert_at(nextNonSP, pn);
					nextNonSP += 1;
				}

				for (int j = lastNonSP + 1; j < nextNonSP; ++j)
				{
					auto& pnj = pathNodes[j];
					if (!pnj.stringPulled)
					{
						pnj.stringPulled = true;
						if (auto* node = pnj.placementAtNode.node.get())
						{
							if (auto* cp = fast_cast<Nodes::ConvexPolygon>(node))
							{
								Optional<int> edgeIdx;
								if (auto* connection = node->get_connect_by_shared_data(pnj.connectionData.get()))
								{
									edgeIdx = connection->get_edge_idx();
								}
								if (edgeIdx.get(NONE) != NONE)
								{
									pnj.stringPulledWindow = cp->calculate_segment_for_edge(edgeIdx.get(), collisionRadius);
								}
							}
						}
					}
				}

#ifdef AN_DEBUG_STRING_PULL_PATH
				{
					dv->start_gathering_data();
					dv->clear();
					for (int i = 0; i < pathNodes.get_size() - 1; ++i)
					{
						dv->add_line(pathNodes[i].placementAtNode.get_current_placement().get_translation().to_vector2(),
							pathNodes[i + 1].placementAtNode.get_current_placement().get_translation().to_vector2(), Colour::red.mul_rgb(0.4f));
					}
					for (int i = 0; i < pathNodes.get_size(); ++i)
					{
						auto& n = pathNodes[i];
						if (n.stringPulledWindow.is_set())
						{
							dv->add_line(n.stringPulledWindow.get().get_start().to_vector2(), n.stringPulledWindow.get().get_end().to_vector2(), Colour::purple.mul_rgb(0.4f));
						}
					}
					dv->end_gathering_data();
					dv->show_and_wait_for_key();
				}
#endif

				Vector3 startLoc = pathNodes[lastNonSP].placementAtNode.get_current_placement().get_translation();
				Vector3 endLoc = pathNodes[nextNonSP].placementAtNode.get_current_placement().get_translation();

				bool keepGoing = true;
				while (keepGoing)
				{
					keepGoing = false;

					// find first corner we have to take
					int nextCorner = nextNonSP;
					int n = lastNonSP;
					int j = lastNonSP + 1;
					Vector3 curLoc = startLoc;
					while (j <= nextCorner)
					{
						if (j == nextNonSP || pathNodes[j].stringPulledCorner)
						{
							Vector3 start = pathNodes[n].placementAtNode.get_current_placement().get_translation();
							Vector3 corner = pathNodes[j].placementAtNode.get_current_placement().get_translation();
							Segment startToCorner(start, corner);

							// string pull from n to j, store only last corner
							++ n;
							int lastCorner = NONE;
							while (n < j)
							{
								if (pathNodes[n].stringPulledWindow.is_set())
								{
									float t = pathNodes[n].stringPulledWindow.get().get_closest_t(startToCorner);
									Vector3 loc = pathNodes[n].stringPulledWindow.get().get_at_t(t);
									pathNodes[n].placementAtNode.set_location(loc);
									if (t <= 0.0f || t >= 1.0f)
									{
										lastCorner = n;
										start = loc;
										startToCorner = Segment(start, corner);
									}
								}
								++n;
							}

							if (lastCorner != NONE)
							{
								pathNodes[lastCorner].placementAtNode.set_location(start);
								pathNodes[lastCorner].stringPulledCorner = true;
								nextCorner = lastCorner;
								keepGoing = true;
							}
							break;
						}
						++j;
					}
				}

				lastNonSP = nextNonSP;
				i = nextNonSP - 1;
			}
			else
			{
				lastNonSP = i;
			}
		}

#ifdef AN_DEBUG_STRING_PULL_PATH
		dv->start_gathering_data();
		dv->clear();
		for (int i = 0; i < pathNodes.get_size() - 1; ++i)
		{
			dv->add_line(pathNodes[i].placementAtNode.get_current_placement().get_translation().to_vector2(),
				pathNodes[i + 1].placementAtNode.get_current_placement().get_translation().to_vector2(), Colour::green.mul_rgb(0.4f));
		}
		dv->end_gathering_data();
		dv->show_and_wait_for_key();
#endif

	}
	
	// remove just polygon that is in the middle
	for (int i = 1; i < pathNodes.get_size() - 1; ++i)
	{
		auto const & prev = pathNodes[i - 1];
		auto const & curr = pathNodes[i];
		auto const & next = pathNodes[i + 1];
		// P - polygon
		bool prevP = fast_cast<Nodes::ConvexPolygon>(prev.placementAtNode.node.get()) != nullptr;
		bool currP = fast_cast<Nodes::ConvexPolygon>(curr.placementAtNode.node.get()) != nullptr;
		bool nextP = fast_cast<Nodes::ConvexPolygon>(next.placementAtNode.node.get()) != nullptr;
		// JP - just polygon (no placement provided)
		bool prevJP = prevP && !prev.placementAtNode.placementLS.is_set();
		bool currJP = currP && !curr.placementAtNode.placementLS.is_set();
		bool nextJP = nextP && !next.placementAtNode.placementLS.is_set();
		if (!prevJP &&
			 currJP &&
			!nextJP)
		{
			// remove middle "just polygon"
			pathNodes.remove_at(i);
			--i;
			continue;
		}
	}
}

bool Path::is_close_to_the_end(Room* _room, Vector3 const& _loc, float _distance, Optional<bool> const& _flat, OUT_ float * _pathDistance) const
{
	bool flat = _flat.get(false);
	bool properRoom = !transportPath || _room == get_final_room();

	float pathDistance = calculate_distance(_room, _loc, flat);
	if (_pathDistance)
	{
		*_pathDistance = pathDistance;
	}
	if (!properRoom && pathDistance > _distance * 0.5f)
	{
		return false;
	}
	return pathDistance <= _distance;
}

float Path::calculate_length(bool _flat) const
{
	if (get_path_nodes().get_size() > 0)
	{
		Room* room = get_path_nodes().get_first().placementAtNode.get_room();
		Vector3 currentLoc = get_path_nodes().get_first().placementAtNode.get_current_placement().get_translation();

		return calculate_distance(room, currentLoc, _flat);
	}
	else
	{
		return 0.0f;
	}
}

float Path::calculate_distance(Room* _room, Vector3 const & _loc, bool _flat) const
{
	int nextPathIdx = find_next_node(_room, _loc);

	if (nextPathIdx == NONE)
	{
		return 0.0f;
	}

	float distance = 0.0f;
	{
		Room* room = _room;
		Vector3 currentLoc = _loc;

		int toIdx = nextPathIdx;
		Nav::PathNode const * prev = toIdx > 0 ? &get_path_nodes()[toIdx - 1] : nullptr;

		while (toIdx < get_path_nodes().get_size())
		{
			Vector3 to = Vector3::zero;
			Nav::PathNode const * pathNode = get_path_nodes().is_index_valid(toIdx) ? &get_path_nodes()[toIdx] : nullptr;

			if (pathNode && pathNode->placementAtNode.get_room() == room)
			{
				if (toIdx == 0)
				{
					if (!pathNode->is_outdated())
					{
						to = pathNode->placementAtNode.get_current_placement().get_translation() - currentLoc;
					}
				}
				else if (pathNode && pathNode->placementAtNode.get_room() == prev->placementAtNode.get_room())
				{
					if (pathNode->placementAtNode.get_room() != room)
					{
						// tough luck, we're in a different room, we should try our luck next time, mark that we should not be trusted
						break;
					}
					to = pathNode->placementAtNode.get_current_placement().get_translation() - currentLoc;
				}
			}
			if (!pathNode)
			{
				// end of path!
				break;
			}

			if (!to.is_zero())
			{
				float length = _flat? to.length_2d() : to.length();
				distance += length;
				/*
				debug_context(room);
				debug_draw_arrow(true, Colour::red, currentLoc, currentLoc + to);
				debug_draw_text(true, Colour::red, currentLoc, NP, true, 1.0f, false, TXT("%.3f"), length);
				debug_no_context();
				*/
				currentLoc += to;
			}

			// switch to another room if required
			if (pathNode &&
				pathNode->door &&
				pathNode->door->has_world_active_room_on_other_side())
			{
				room = pathNode->door->get_world_active_room_on_other_side();
				currentLoc = pathNode->door->get_other_room_transform().location_to_local(currentLoc);
			}
			++toIdx;

			prev = pathNode;
		}
	}

	return distance;
}

bool Path::is_path_blocked_temporarily(int _nextPathIdx, Vector3 const & imoLoc, float distToCover) const
{
	// check longer distance, not only the current one
	int pathIdx = _nextPathIdx;
	if (_nextPathIdx == 0)
	{
		auto const & curr = get_path_nodes()[_nextPathIdx];
		Vector3 currLoc = curr.placementAtNode.get_current_placement().get_translation();
		distToCover -= (imoLoc - currLoc).length();
		++pathIdx;
	}
	while (pathIdx < get_path_nodes().get_size() && distToCover >= 0.0f)
	{
		auto const & prev = get_path_nodes()[pathIdx - 1];
		auto const & curr = get_path_nodes()[pathIdx];
		if (curr.is_outdated() || prev.is_outdated())
		{
			break;
		}
		if (prev.placementAtNode.node->get_room() == curr.placementAtNode.node->get_room()) // to cover doors
		{
			if (prev.placementAtNode.node->is_blocked_temporarily_stop())
			{
				// do not perform any further checks
				break;
			}
			if (curr.connectionData.is_set() && curr.connectionData->is_blocked_temporarily())
			{
				return true;
			}
			Vector3 prevLoc = prev.placementAtNode.get_current_placement().get_translation();
			Vector3 currLoc = curr.placementAtNode.get_current_placement().get_translation();
			Vector3 prev2curr = currLoc - prevLoc;
			float prev2currDist = prev2curr.length();
			if (pathIdx > _nextPathIdx)
			{
				distToCover -= prev2currDist;
			}
			else
			{
				Vector3 prev2currDir = prev2currDist != 0.0f ? prev2curr / prev2currDist : prev2curr;
				float distToNext = Vector3::dot(currLoc - imoLoc, prev2currDir);
				distToCover -= distToNext;
			}
		}
		++pathIdx;
	}
	return false;
}

void Path::log_node(LogInfoContext & _log, int _idx, tchar const * _prefix) const
{
#ifdef AN_DEVELOPMENT
	if (pathNodes.is_index_valid(_idx))
	{
		auto const & pn = pathNodes[_idx];
		if (pn.door)
		{
			_log.log(TXT("%S%i [D] in %S at %S, door %S (c:%i)"),
				_prefix? _prefix : TXT(""),
				_idx,
				pn.placementAtNode.get_room()? pn.placementAtNode.get_room()->get_name().to_char() : TXT("<no room>"),
				pn.placementAtNode.get_current_placement().get_translation().to_string().to_char(),
				pn.door->ai_get_name().to_char(),
				pn.placementAtNode.node->get_connections_count()
				);
		}
		else
		{
			_log.log(TXT("%S%i [ ] in %S at %S (c:%i)"),
				_prefix ? _prefix : TXT(""),
				_idx,
				pn.placementAtNode.get_room() ? pn.placementAtNode.get_room()->get_name().to_char() : TXT("<no room>"),
				pn.placementAtNode.get_current_placement().get_translation().to_string().to_char(),
				pn.placementAtNode.node->get_connections_count()
				);
		}
	}
#endif
}

Room* Path::get_start_room() const
{
	if (pathNodes.is_empty())
	{
		return nullptr;
	}
	return pathNodes.get_first().placementAtNode.get_room();
}

Room* Path::get_final_room() const
{
	if (pathNodes.is_empty())
	{
		return nullptr;
	}
	return pathNodes.get_last().placementAtNode.get_room();
}

Vector3 Path::get_final_node_location() const
{
	if (pathNodes.is_empty())
	{
		return Vector3::zero;
	}
	return pathNodes.get_last().placementAtNode.get_current_placement().get_translation();
}
