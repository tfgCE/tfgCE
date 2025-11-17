#include "navNode_Point.h"

#include "navNode_ConvexPolygon.h"

#include "..\navMesh.h"
#include "..\navSystem.h"

#include "..\..\game\game.h"
#include "..\..\modulesOwner\modulesOwner.h"
#include "..\..\module\modulePresence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Nav;
using namespace Nodes;

//

REGISTER_FOR_FAST_CAST(Point);

void Point::unify_as_open_node()
{
	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());
	an_assert(is_open_node());

	Mesh* mesh = belongsTo;
	if (!mesh)
	{
		return;
	}
	if (!openMoveToFind.is_zero())
	{
		return;
	}
	// if is door in room, can still be unified

	Transform currentPlacement = get_current_placement();
	Vector3 currentLocation = currentPlacement.get_translation();

	for_every_ref(other, mesh->access_nodes())
	{
		if (!openMoveToFind.is_zero() && connections.get_size() > 1)
		{
			// we have enough connections!
			continue;
		}
		if (is_connected_to(other))
		{
			continue;
		}
		if (other->get_door_in_room())
		{
			// can't unify with "door in room" - if we have that, it should be other way around (doorInRoom unified with non door in room)
			// this also makes it impossible to unify two "door in room"s
			continue;
		}
		if (other != this)
		{
			if (auto * otherAsPoint = fast_cast<Point>(other))
			{
				if (otherAsPoint->openMoveToFind.is_zero())
				{
					Transform otherPlacement = otherAsPoint->get_current_placement();
					Vector3 otherLocation = otherPlacement.get_translation();

					if (Vector3::are_almost_equal(currentLocation, otherLocation))
					{
						// steal its connections
						for_every(otherConnection, otherAsPoint->connections)
						{
							if (!otherConnection->is_outdated() &&
								otherConnection->get_to() != this)
							{
								auto* reconnectTo = otherConnection->get_to();
								otherConnection->outdate();
								connect(reconnectTo);
							}
						}

						get_unified_values_from(other);
						otherAsPoint->outdate();
#ifdef AN_DEVELOPMENT
						unified = true;
#endif
					}
				}
			}
		}
	}
}

void Point::connect_as_open_node()
{
	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());
	an_assert(is_open_node());

	Mesh* mesh = belongsTo;
	if (!mesh)
	{
		return;
	}
	Transform currentPlacement = get_current_placement();
	Vector3 currentLocation = currentPlacement.get_translation();
	Vector3 currentOpenMoveToFindDir = currentPlacement.vector_to_world(openMoveToFind);
	Vector3 currentOpenMoveToFindLocation = currentLocation + currentOpenMoveToFindDir;
	
	todo_note(TXT("vertical! for time being checking same owner is enough"));
	Transform newPlacement = currentPlacement;
	IModulesOwner* newOwner = placementOwner.get();
	bool newOnMoveableOwner = onMoveableOwner;
	float bestDist = openMoveToFind.length() * 2.0f;
	bool bestOnSameOwner = false;
	Node* connectOnOpenMoveToFind = nullptr;
	Array<NodePtr> outdateNodes;
	bool setNewPlacement = false;
	for_every_ref(node, mesh->access_nodes())
	{
		if (!openMoveToFind.is_zero() && connections.get_size() > 1)
		{
			// we have enough connections!
			continue;
		}
		if (is_connected_to(node))
		{
			continue;
		}
		if (node != this)
		{
			if (node->is_open_accepting_node())
			{
				bool onSameOwner = node->get_placement_owner() && node->get_placement_owner() == get_placement_owner();
				if (auto* poly = fast_cast<ConvexPolygon>(node))
				{
					if (!openMoveToFind.is_zero() && connections.get_size() == 1)
					{
						if (poly->is_inside(currentLocation) &&
							(onSameOwner || !bestOnSameOwner) &&
							abs(poly->get_current_placement().location_to_local(currentLocation).z) < 1.0f)
						{
							todo_note(TXT("better vertical check!"));
							Vector3 newLocation = currentLocation;
							if (!currentOpenMoveToFindDir.is_zero())
							{
								newLocation += currentOpenMoveToFindDir.normal() * min(currentOpenMoveToFindDir.length(), 0.2f magic_number); // move slightly forward (nav mesh should declare radius of objects?)
								if (!poly->is_inside(newLocation))
								{
									newLocation = currentLocation;
								}
							}
							setNewPlacement = true;
							newPlacement.set_translation(newLocation);
							newOwner = poly->get_placement_owner();
							newOnMoveableOwner = poly->get_on_moveable_owner();
							connectOnOpenMoveToFind = node;
							outdateNodes.clear();
							bestDist = 0.0f;
							bestOnSameOwner = onSameOwner;
						}
						else
						{
							Vector3 intersectionPoint;
							if (poly->does_intersect(Segment(currentLocation, currentOpenMoveToFindLocation), OUT_ intersectionPoint) &&
								abs(poly->get_current_placement().location_to_local(currentLocation).z) < 1.0f)
							{
								todo_note(TXT("better vertical check!"));
								float dist = (intersectionPoint - currentLocation).length();
								if ((dist < bestDist || (onSameOwner && !bestOnSameOwner)) &&
									(onSameOwner || !bestOnSameOwner))
								{
									bestDist = dist;
									bestOnSameOwner = onSameOwner;
									setNewPlacement = true;
									newPlacement.set_translation(intersectionPoint);
									newOwner = poly->get_placement_owner();
									newOnMoveableOwner = poly->get_on_moveable_owner();
									connectOnOpenMoveToFind = node;
									outdateNodes.clear();
								}
							}
						}
					}
					else if (poly->is_inside(currentLocation))
					{
						auto * data = connect(poly);
						data->set_through_open();
						continue;
					}
				}
				if (auto* point = fast_cast<Point>(node))
				{
					Transform pointPlacement = point->get_current_placement();
					bool match = false;
					float dist = 0.0f;
					if (!openMoveToFind.is_zero())
					{
						float dot = Vector3::dot(currentOpenMoveToFindDir.normal(), (pointPlacement.get_translation() - currentLocation).normal());
						if (dot >= 0.95f)
						{
							float inDirFit = 1.0f - (pointPlacement.get_translation() - currentLocation).length() / openMoveToFind.length();
							if (inDirFit > 0.0f)
							{
								dist = (pointPlacement.get_translation() - currentLocation).length();
								if (dist < bestDist)
								{
									match = true;
								}
							}
						}
					}
					if ((pointPlacement.get_translation() - currentLocation).length() < magic_number 0.05f)
					{
						dist = 0.0f;
						match = true;
					}
					if (match &&
						(onSameOwner || !bestOnSameOwner) &&
						abs(pointPlacement.location_to_local(currentLocation).z) < 1.0f)
					{
						todo_note(TXT("better vertical check!"));
						/*
						 *	How about such case?
						 *										[*] door in room
						 *										[o] our open move to find node
						 *		[c] --- [c] --- [c] --- [c] --- [c] corridor's end
						 *		corridor===================
						 *
						 *	And actually we would like to have this:
						 *		[c] --- [c] --- [c] --- [o] --- [*] (1)
						 *	Or at least this:
						 *		                        [o] --- [*] (2)
						 *		[c] --- [c] --- [c] --- [c]
						 *		this one is for case:
						 *								[o] --- [*]
						 *						[*] --- [o]
						 *		where both [o]s are referenced by someone (door in room?)
						 *	We have to outdate two nodes and "replace" them!
						 */
						// check if we have such nodes to replace
						Point* pointInDir = nullptr;
						float bestDot = 0.0f;
						if (!currentOpenMoveToFindDir.is_zero())
						{
							for_every(connection, point->connections)
							{
								if (connection->is_outdated())
								{
									continue;
								}
								if (auto * asPoint = fast_cast<Point>(connection->get_to()))
								{
									if (asPoint->connections.get_size() == 2)
									{
										Vector3 nextLocation = asPoint->get_current_placement().get_translation();
										float dot = Vector3::dot((nextLocation - currentLocation).normal(), currentOpenMoveToFindDir.normal());
										if (dot >= bestDot) // = to allow same loc
										{
											pointInDir = asPoint;
										}
									}
								}
							}
						}
						bool newBest = false;
						if (pointInDir)
						{
							// alright, place our open node where this node is, modify connections, etc
							setNewPlacement = true;
							newPlacement = pointInDir->get_current_placement();
							newOwner = pointInDir->get_placement_owner();
							newOnMoveableOwner = pointInDir->get_on_moveable_owner();
							dist = 0.0f; // we won't get any closer, right?
							if (!pointInDir->is_open_node())
							{
								// case (1)
								// 		[c] --- [c] --- [c] --- [o] --- [*]
								for_every(connection, pointInDir->connections)
								{
									if (connection->get_to() != point)
									{
										newBest = true;
										connectOnOpenMoveToFind = connection->get_to();
										// outdate those points, we will remove them when we're done with building stuff
										outdateNodes.clear();
										outdateNodes.push_back(NodePtr(point));
										outdateNodes.push_back(NodePtr(pointInDir));
										break;
									}
								}
							}
							else
							{
								// case (2)
								// 		                        [o] --- [*]
								// 		[c] --- [c] --- [c] --- [c]
								// if it is open, just place it in same place
								newBest = true;
								connectOnOpenMoveToFind = pointInDir;
								outdateNodes.clear();
								outdateNodes.push_back(NodePtr(point));
							}
						}
						else
						{
							// if not in dir
							// check if we can replace current one
							Node* anyOther = nullptr;
							int validConnections = 0;
							for_every(connection, point->connections)
							{
								if (connection->is_outdated())
								{
									continue;
								}
								++validConnections;
								if (!anyOther || !anyOther->get_door_in_room() ||
									(connection->get_to() && connection->get_to()->get_door_in_room()))
								{
									anyOther = connection->get_to();
								}
							}
							setNewPlacement = true;
							newPlacement = point->get_current_placement();
							newOwner = point->get_placement_owner();
							newOnMoveableOwner = point->get_on_moveable_owner();
							if (validConnections <= 1)
							{
								if (anyOther) // this check is to prevent dropping good point we've already found
								{
									//										[*]						[*]
									// 		[c] --- [c] --- [c] --- [c] --- [o]			 or			[o]
									// although for such cases there should be no door in room
									newBest = true;
									connectOnOpenMoveToFind = anyOther; // that's still fine if none
									outdateNodes.clear();
									outdateNodes.push_back(NodePtr(point));
								}
								else if (point->connections.is_empty())
								{
									// connect to this point as it seems to be not connected to anything
									// maybe we will drop it later
									newBest = true;
									connectOnOpenMoveToFind = node; // that's still fine if none
									outdateNodes.clear();
								}
							}
							else
							{
								//										[*]
								//										[o]
								// 		[c] --- [c] --- [c] --- [c] --- [c] --- something else as there are more than one valid connections
								newBest = true;
								connectOnOpenMoveToFind = point;
								outdateNodes.clear();
							}
						}
						if (newBest)
						{
							bestDist = dist;
							bestOnSameOwner = onSameOwner;
						}
					}
				}
			}
		}
	}

	for_every_ref(outdate, outdateNodes)
	{
		outdate->outdate();
	}
	outdateNodes.clear();

	if (connectOnOpenMoveToFind)
	{
		auto * data = connect(connectOnOpenMoveToFind);
		data->set_through_open();
	}
	if (setNewPlacement)
	{
		place_WS(newPlacement, newOwner, newOnMoveableOwner);
	}
}

bool Point::find_location(Vector3 const & _at, OUT_ Vector3 & _found, REF_ float & _dist) const
{
	if (is_outdated())
	{
		return false;
	}
	bool result = false;
	Transform placement = get_current_placement();
	float distToPoint = (placement.get_translation() - _at).length();
	if (distToPoint < _dist)
	{
		_dist = distToPoint;
		_found = placement.get_translation();
		result = true;
	}
	// check connections (up to 1/2 way)
	for_every(connection, connections)
	{
		if (connection->is_outdated() ||
			connection->get_data()->is_blocked_temporarily())
		{
			continue;
		}
		if (fast_cast<Point>(connection->get_to()))
		{
			Transform toPlacement = connection->get_to()->get_current_placement();
			Segment connectionSegment(placement.get_translation(), (toPlacement.get_translation() + placement.get_translation()) * 0.5f);
			float t;
			float distToConnectionSegment = connectionSegment.get_distance(_at, OUT_ t);
			if (distToConnectionSegment < _dist)
			{
				_dist = distToConnectionSegment;
				_found = connectionSegment.get_at_t(t);
				result = true;
			}
		}
		else
		{
			an_assert(fast_cast<ConvexPolygon>(connection->get_to()), TXT("implement_ other types"));
		}
	}
	return result;
}
