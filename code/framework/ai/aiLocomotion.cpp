#include "aiLocomotion.h"

#include "aiMindInstance.h"

#include "..\modulesOwner\modulesOwner.h"

#include "..\module\moduleAI.h"
#include "..\module\moduleCollision.h"
#include "..\module\moduleController.h"
#include "..\module\moduleMovement.h"
#include "..\module\modulePresence.h"
#include "..\nav\navPath.h"
#include "..\object\actor.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"

#include "..\..\core\debug\debugRenderer.h"

//#define DEBUG_LOCOMOTION

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// room tags
DEFINE_STATIC_NAME(ceilingCrawling);
DEFINE_STATIC_NAME(wallCrawling);
DEFINE_STATIC_NAME(noWallCrawling);

// logic variables
DEFINE_STATIC_NAME(locomotionShouldFall);

//

using namespace Framework;
using namespace AI;

//

void LocomotionReference::clear()
{
	location.clear();
	aiObject.clear();
	placement.clear();
}

void LocomotionReference::set(Vector3 const & _location, Room* _room)
{
	location = _location;
	room = _room;
	aiObject.clear();
	placement.clear();
}

void LocomotionReference::set(IAIObject * _aiObject)
{
	location.clear();
	placement.clear();
	if (_aiObject)
	{
		aiObject.set(SafePtr<IAIObject>(_aiObject));
	}
	else
	{
		aiObject.clear();
	}
}

void LocomotionReference::set_placement(RelativeToPresencePlacement* _placement)
{
	location.clear();
	aiObject.clear();
	placement.clear();
	if (_placement)
	{
		placement.set(SafePtr<RelativeToPresencePlacement>(_placement));
	}
	else
	{
		placement.clear();
	}
}

IAIObject* LocomotionReference::get_object() const
{
	return aiObject.is_set() ? aiObject.get().get() : nullptr;
}

bool LocomotionReference::get_location(Locomotion const & _locomotion, OUT_ Vector3 & _location, GetLocation _getLocation) const
{
	auto* owner = _locomotion.mind->get_owner_as_modules_owner();
	auto* presence = owner->get_presence();
	if (!presence)
	{
		return false;
	}
	auto* presenceInRoom = presence->get_in_room();

	Vector3 locationInRoom = Vector3::zero;
	bool locationInRoomOurRoom = false; // we assume that throughDoor will be set if required so
	Room* checkRoom = nullptr;
	DoorInRoom* throughDoor = nullptr;
	if (aiObject.is_set() && aiObject.get().is_set())
	{
		// get location in room, using ai (aim at the head?) or just placement
		auto* ai = owner->get_ai();
		if (ai && _getLocation == GetTargetLocation)
		{
			locationInRoom = ai->get_target_placement_for(aiObject.get().get()).get_translation();
		}
		else
		{
			auto* imoTarget = fast_cast<IModulesOwner>(aiObject.get().get());
			if (imoTarget && _getLocation != GetActualLocation)
			{
				locationInRoom = imoTarget->get_presence()->get_centre_of_presence_WS();
			}
			else
			{
				locationInRoom = aiObject.get()->ai_get_placement().get_translation() + Vector3(0.0f, 0.0f, _locomotion.movementParameters.relativeZ.get(0.0f));
			}
		}
		checkRoom = aiObject.get()->ai_get_room();
	}
	else if (placement.is_set() &&
			 placement.get().get() &&
			 placement.get().get()->is_active())
	{
		auto* r2pp = placement.get().get();
		an_assert(r2pp->get_owner_presence() == owner->get_presence());
		Transform placement = r2pp->get_placement_in_owner_room();
		auto* ai = owner->get_ai();
		if (ai && _getLocation == GetTargetLocation)
		{
			locationInRoom = placement.location_to_world(ai->get_target_placement_os_for(ai->get_owner()).get_translation());
		}
		else
		{
			auto* imoTarget = r2pp->get_target();
			if (imoTarget && _getLocation != GetActualLocation)
			{
				locationInRoom = placement.location_to_world(imoTarget->get_presence()->get_centre_of_presence_os().get_translation());
			}
			else
			{
				locationInRoom = placement.get_translation() + Vector3(0.0f, 0.0f, _locomotion.movementParameters.relativeZ.get(0.0f));
			}
		}
		checkRoom = r2pp->get_in_final_room();
		throughDoor = r2pp->get_through_door();
		locationInRoomOurRoom = true;
	}
	else if (location.is_set())
	{
		locationInRoom = location.get();
		checkRoom = room.get();
	}
	else
	{
		return false;
	}
	
	if (presenceInRoom == checkRoom)
	{
		_location = locationInRoom;
		return true;
	}
	else
	{
		bool found = checkRoom == presenceInRoom;
		DoorInRoom const * closestDoor = throughDoor;
		Transform intoCheckRoom = Transform::identity;
		
		if (locationInRoomOurRoom)
		{
			_location = locationInRoom;
			found = true;
		}

		if (! found &&
			_locomotion.movePath.is_set())
		{
			auto const & pathNodes = _locomotion.movePath->get_path_nodes();
			for (int i = _locomotion.nextPathIdx; i < pathNodes.get_size(); ++i)
			{
				if (pathNodes[i].is_outdated())
				{
					break;
				}
				if (auto const * door = pathNodes[i].door)
				{
					if (!door->has_world_active_room_on_other_side())
					{
						break;
					}
					if (!closestDoor)
					{
						closestDoor = door;
					}
					intoCheckRoom = intoCheckRoom.to_world(door->get_other_room_transform());
					if (door->get_world_active_room_on_other_side() == checkRoom)
					{
						_location = intoCheckRoom.location_to_world(locationInRoom);
						found = true;
						break;
					}
				}
			}
		}

		if (!found)
		{
			if (auto * inRoom = presenceInRoom)
			{
				for_every_ptr(door, inRoom->get_doors())
				{
					if (door->is_visible() &&
						door->get_world_active_room_on_other_side() == checkRoom)
					{
						closestDoor = door;
						intoCheckRoom = intoCheckRoom.to_world(door->get_other_room_transform());
						_location = intoCheckRoom.location_to_world(locationInRoom);
						found = true;
						break;
					}
				}
			}
		}

		todo_note(TXT("check doors - second depth?"));

		if (found)
		{
			if (closestDoor)
			{
				// if there is door on the way, we have to be able to see through at least first door (note below)
				if (closestDoor->get_world_active_room_on_other_side() != checkRoom)
				{
					todo_note(TXT("this is sort of a hack - we should consider more doors, imagine such situation:"));
					/*
					 *	lines are doors
					 *						   |
					 *						ME.|..		ENEMY
					 *						   |  .		  .
					 *							  .		  .
					 *							 ---	 ---
					 *							  .		  .
					 *							   .......
					 *
					 * with one door check we may end up thinking we see enemy and should consider keep distance
					 */
					return false;
				}
				Vector3 destLocation = _location;
				if (closestDoor->does_go_through_hole(presence->get_placement().get_translation(), _location, destLocation, -0.01f, 0.3f))
				{
					return true;
				}
			}
			else
			{
				return true;
			}
		}
	}
	return false;
}

bool LocomotionReference::operator ==(LocomotionReference const & _a) const
{
	if (aiObject.is_set() && _a.aiObject.is_set())
	{
		// same object
		return aiObject.get() == _a.aiObject.get();
	}
	if (location.is_set() && _a.location.is_set())
	{
		// same location
		return location.get() == _a.location.get();
	}
	return false;
}

//

Locomotion::Locomotion(MindInstance* _mind)
: mind(_mind)
, moveControl(LocomotionMoveControl::None)
, turnControl(LocomotionTurnControl::None)
, nextPathRequested(_mind->get_owner_as_modules_owner())
, importantPathRequested(_mind->get_owner_as_modules_owner())
{
}

Locomotion::~Locomotion()
{
}

bool Locomotion::has_finished_move(float _left) const
{
	if (movePath.is_set() && movePath->is_transport_path())
	{
		if (mind->get_owner_as_modules_owner()->get_presence()->get_in_room() != movePath->get_final_room())
		{
			return false;
		}
	}
	return moveDistanceLeft.is_set() && moveDistanceLeft.get() < _left;
}

void Locomotion::reset()
{
	dont_control();
}

void Locomotion::stop()
{
	dont_move();
	dont_keep_distance();
	dont_turn();
}

void Locomotion::dont_control()
{
	dont_move();
	dont_keep_distance();
	dont_turn();
	// and clear control
	moveControl = LocomotionMoveControl::None;
	turnControl = LocomotionTurnControl::None;
}

void Locomotion::dont_move()
{
	moveControl = LocomotionMoveControl::DontMove;
	movePath.clear();
#ifdef AN_USE_AI_LOG
	pathLog.clear();
#endif
	nextPathRequested.clear();
	importantPathRequested.clear();
	moveDistanceLeft.set(0.0f);
}

void Locomotion::dont_keep_distance()
{
	keepDistanceTo.clear();
}

void Locomotion::dont_turn()
{
	turnControl = LocomotionTurnControl::DontTurn;
	turnDistanceLeft.set(0.0f);
	turnYawOffset.clear();
}

void Locomotion::allow_any_move()
{
	moveControl = LocomotionMoveControl::None;
	movePath.clear();
#ifdef AN_USE_AI_LOG
	pathLog.clear();
#endif
	nextPathRequested.clear();
	importantPathRequested.clear();
	moveDistanceLeft.set(0.0f);
}

void Locomotion::allow_any_turn()
{
	turnControl = LocomotionTurnControl::None;
	turnDistanceLeft.set(0.0f);
}

void Locomotion::move_to_2d(Vector3 const & _location, float _moveToRadius, MovementParameters const & _movementParameters)
{
	moveControl = LocomotionMoveControl::MoveTo2D;
	movementParameters = _movementParameters;
	moveTo.set(_location, mind->get_owner_as_modules_owner()->get_presence()->get_in_room());
	moveToRadius = _moveToRadius;
	movePath.clear();
#ifdef AN_USE_AI_LOG
	pathLog.clear();
#endif
	moveDistanceLeft.clear();
	nextPathRequested.clear();
	importantPathRequested.clear();
}

void Locomotion::move_to_2d(IAIObject * _aiObject, float _moveToRadius, MovementParameters const & _movementParameters)
{
	moveControl = LocomotionMoveControl::MoveTo2D;
	movementParameters = _movementParameters;
	moveTo.set(_aiObject);
	moveToRadius = _moveToRadius;
	movePath.clear();
#ifdef AN_USE_AI_LOG
	pathLog.clear();
#endif
	moveDistanceLeft.clear();
	nextPathRequested.clear();
	importantPathRequested.clear();
}

void Locomotion::move_to_3d(Vector3 const & _location, float _moveToRadius, MovementParameters const & _movementParameters)
{
	moveControl = LocomotionMoveControl::MoveTo3D;
	movementParameters = _movementParameters;
	moveTo.set(_location, mind->get_owner_as_modules_owner()->get_presence()->get_in_room());
	moveToRadius = _moveToRadius;
	movePath.clear();
#ifdef AN_USE_AI_LOG
	pathLog.clear();
#endif
	moveDistanceLeft.clear();
	nextPathRequested.clear();
	importantPathRequested.clear();
}

void Locomotion::move_to_3d(IAIObject * _aiObject, float _moveToRadius, MovementParameters const & _movementParameters)
{
	moveControl = LocomotionMoveControl::MoveTo3D;
	movementParameters = _movementParameters;
	moveTo.set(_aiObject);
	moveToRadius = _moveToRadius;
	movePath.clear();
#ifdef AN_USE_AI_LOG
	pathLog.clear();
#endif
	moveDistanceLeft.clear();
	nextPathRequested.clear();
	importantPathRequested.clear();
}

void Locomotion::follow_path_2d(Nav::Path * _path, Optional<float> const & _moveToRadius, MovementParameters const & _movementParameters)
{
	follow_path(_path, _moveToRadius, _movementParameters, true);
}

void Locomotion::follow_path_3d(Nav::Path * _path, Optional<float> const & _moveToRadius, MovementParameters const & _movementParameters)
{
	follow_path(_path, _moveToRadius, _movementParameters, false);
}

void Locomotion::follow_path(Nav::Path * _path, Optional<float> const & _moveToRadius, MovementParameters const & _movementParameters, bool _2d)
{
	moveControl = _2d ? LocomotionMoveControl::FollowPath2D : LocomotionMoveControl::FollowPath3D;
	moveToRadius = _moveToRadius.get(0.0f);
	movementParameters = _movementParameters;
	movePath = _path;
#ifdef AN_USE_AI_LOG
	pathLog.clear();
#endif
	nextPathRequested.clear();
	importantPathRequested.clear();
	moveDistanceLeft.clear();

	// find closest point on path
	nextPathIdx = 0;

	IModulesOwner* imo = mind->get_owner_as_modules_owner();
	auto* presence = imo ? imo->get_presence() : nullptr;
	if (!presence)
	{
		return;
	}

	Vector3 imoLoc = presence->get_placement().get_translation();

	Nav::PathNode const * prev = nullptr;
	int closestIdx = NONE;
	float closestDist = 0.0f;
	for_every(pathNode, movePath->get_path_nodes())
	{
		if (pathNode->is_outdated())
		{
			return;
		}

		if (prev)
		{
			if (prev->placementAtNode.get_room() == pathNode->placementAtNode.get_room() &&
				pathNode->placementAtNode.get_room() == presence->get_in_room())
			{
				Vector3 prevLoc = prev->placementAtNode.get_current_placement().get_translation();
				Vector3 currLoc = pathNode->placementAtNode.get_current_placement().get_translation();
				Segment seg(prevLoc, currLoc);
				float dist = seg.get_distance(imoLoc);
				if (closestIdx == NONE || dist < closestDist)
				{
					closestIdx = for_everys_index(pathNode);
					closestDist = dist;
				}
			}
		}
		prev = pathNode;
	}

	if (closestIdx != NONE)
	{
		nextPathIdx = closestIdx;
	}
#ifdef AN_USE_AI_LOG
	pathLog.log(TXT("start with nextPathIdx %i (imo's room: %S)"), nextPathIdx, presence->get_in_room()->get_name().to_char());
#endif
	update_next_path_requested();
}

void Locomotion::keep_distance_2d(Vector3 const & _point, Optional<float> const & _min, Optional<float> const & _max, Optional<float> const & _verticalDistance)
{
	keepDistanceTo.set(_point, mind->get_owner_as_modules_owner()->get_presence()->get_in_room());
	keepDistance2D = true;
	keepDistanceMin = _min;
	keepDistanceMax = _max;
	keepDistance2DVerticalSeparation = _verticalDistance.get(1.0f);
}

void Locomotion::keep_distance_2d(IAIObject * _aiObject, Optional<float> const & _min, Optional<float> const & _max, Optional<float> const & _verticalDistance)
{
	keepDistanceTo.set(_aiObject);
	keepDistance2D = true;
	keepDistanceMin = _min;
	keepDistanceMax = _max;
	keepDistance2DVerticalSeparation = _verticalDistance.get(1.0f);
}

void Locomotion::keep_distance_3d(Vector3 const & _point, Optional<float> const & _min, Optional<float> const & _max)
{
	keepDistanceTo.set(_point, mind->get_owner_as_modules_owner()->get_presence()->get_in_room());
	keepDistance2D = false;
	keepDistanceMin = _min;
	keepDistanceMax = _max;
}

void Locomotion::keep_distance_3d(IAIObject * _aiObject, Optional<float> const & _min, Optional<float> const & _max)
{
	keepDistanceTo.set(_aiObject);
	keepDistance2D = false;
	keepDistanceMin = _min;
	keepDistanceMax = _max;
}

void Locomotion::turn_in_movement_direction_2d()
{
	turnControl = LocomotionTurnControl::InMovementDirection2D;
	turnDeadZone = 0.0f;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_in_movement_direction_3d()
{
	turnControl = LocomotionTurnControl::InMovementDirection3D;
	turnDeadZone = 0.0f;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_follow_path_2d()
{
	turnControl = LocomotionTurnControl::FollowPath2D;
	turnDeadZone = 0.0f;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_follow_path_3d()
{
	turnControl = LocomotionTurnControl::FollowPath3D;
	turnDeadZone = 0.0f;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_towards_2d(Vector3 const & _location, float _turnDeadZone)
{
	turnControl = LocomotionTurnControl::TurnTowards2D;
	turnTowards.set(_location, mind->get_owner_as_modules_owner()->get_presence()->get_in_room());
	turnDeadZone = _turnDeadZone;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_towards_2d(IAIObject * _aiObject, float _turnDeadZone)
{
	turnControl = LocomotionTurnControl::TurnTowards2D;
	turnTowards.set(_aiObject);
	turnDeadZone = _turnDeadZone;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_towards_2d(RelativeToPresencePlacement & _placement, float _turnDeadZone)
{
	turnControl = LocomotionTurnControl::TurnTowards2D;
	turnTowards.set_placement(&_placement);
	turnDeadZone = _turnDeadZone;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_towards_3d(Vector3 const & _location, float _turnDeadZone)
{
	turnControl = LocomotionTurnControl::TurnTowards3D;
	turnTowards.set(_location, mind->get_owner_as_modules_owner()->get_presence()->get_in_room());
	turnDeadZone = _turnDeadZone;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_towards_3d(IAIObject * _aiObject, float _turnDeadZone)
{
	turnControl = LocomotionTurnControl::TurnTowards3D;
	turnTowards.set(_aiObject);
	turnDeadZone = _turnDeadZone;
	turnDistanceLeft.clear();
	turnYawOffset.clear();
}

void Locomotion::turn_yaw_offset(Optional<float> const& _offset)
{
	turnYawOffset = _offset;
}

void Locomotion::advance(float _deltaTime)
{
	if (!mind)
	{
		return;
	}

	IModulesOwner* imo = mind->get_owner_as_modules_owner();
	if (!imo)
	{
		return;
	}

	ModuleController* controller = imo->get_controller();
	ModulePresence* presence = imo->get_presence();
	ModuleMovement* movement = imo->get_movement();
	if (!controller || !presence || ! movement)
	{
		warn(TXT("controller%C, presence%C and movement%C required by locomotion (%S)"), controller ? '+' : '-', presence ? '+' : '-', movement ? '+' : '-', imo->ai_get_name().to_char());
		return;
	}
	if (!presence->get_in_room())
	{
		return;
	}

	{
		wallCrawlingYawOffsetTimeLeft -= _deltaTime;
		if (wallCrawlingYawOffsetTimeLeft <= 0.0f)
		{
			Random::Generator rg;
			wallCrawlingYawOffsetTimeLeft = rg.get_float(2.0f, 7.0f);
			if (auto* m = mind->get_mind())
			{
				wallCrawlingYawOffset = m->get_wall_crawling_yaw_offset().get(rg);
			}
		}
	}

	bool allowWallCrawling = false;
	if (auto* m = mind->get_mind())
	{
		allowWallCrawling = m->does_allow_wall_crawling();
	}

	debug_filter(locomotion);
	debug_subject(imo);
	debug_context(presence->get_in_room());

	Vector3 movementDirection = Vector3::zero;

	/*
	 *	In some distant future I'd like to reorganise this code
	 */
	// movement

	if (moveControl == LocomotionMoveControl::DontMove)
	{
		controller->clear_requested_velocity_linear();
		controller->clear_requested_movement_direction();
		controller->clear_distance_to_stop();
		controller->clear_distance_to_slow_down();
		moveDistanceLeft.set(0.0f);
	}

	Vector3 currentLocation = presence->get_placement().get_translation();
	Quat currentOrientation = presence->get_placement().get_orientation();

	moveLost = false;

	{
		bool in2D = false;
		bool shouldStopAndWait = false;
		bool currentMoveToLocationIsIntermediateLocation = false; // this is not our final stop
		Optional<Vector3> currentMoveToLocation;
		Optional<Vector3> nextMoveToLocation;
		Optional<Vector3> finalMoveDestination;
		Range currentMoveToRadius(0.0f, moveToRadius);

		if (LocomotionMoveControl::is_move_to(moveControl))
		{
			Vector3 currentMoveToLocationTemp;
			if (moveTo.get_location(*this, OUT_ currentMoveToLocationTemp))
			{
				in2D = LocomotionMoveControl::is_2d(moveControl);
				currentMoveToLocation = currentMoveToLocationTemp;
				finalMoveDestination = currentMoveToLocation;
			}
		}

		if (LocomotionMoveControl::is_path_following(moveControl) &&
			movePath.is_set())
		{
			in2D = LocomotionMoveControl::is_2d(moveControl);

			Vector3 imoLoc = currentLocation;
			todo_note(TXT("this might be heavy? finding nav location each frame, limited to current room, but still. it isn't now but pay attention right here"));
			Nav::PlacementAtNode atNode = presence->get_in_room()->find_nav_location(presence->get_placement());
			int atGroupId = atNode.node.get() ? atNode.node->get_group() : NONE;

			// here as we update only when moving
			{
				if (auto* r = presence->get_in_room())
				{
					if (wallCeilingCrawlingDoneForRoom != r)
					{
						wallCeilingCrawlingDoneForRoom = r;
						{
							// hardcoded for time being
							wallCeilingCrawlingRoomHeight = 2.0f;
						}
						bool doWallCrawling = false;
						bool doCeilingCrawling = false;
						if (allowWallCrawling)
						{
							doWallCrawling = mind->get_mind()->does_assume_wall_crawling();
							{
								if (r->get_tags().get_tag(NAME(noWallCrawling)))
								{
									doWallCrawling = false;
								}
								if (r->get_tags().get_tag(NAME(wallCrawling)))
								{
									doWallCrawling = true;
								}
								if (r->get_tags().get_tag(NAME(ceilingCrawling)))
								{
									doCeilingCrawling = true;
								}
							}
						}
						wallCrawling = doWallCrawling;
						ceilingCrawling = doCeilingCrawling;
					}
				}
			}

			{
				if (nextPathIdx > 0 && nextPathIdx < movePath->get_path_nodes().get_size())
				{
					// check if maybe something went wrong and we're trying to move backward while we just require to get to next state
					int switchToPathIdx = min(nextPathIdx + 1, movePath->get_path_nodes().get_size() - 1);
					while (switchToPathIdx > 0)
					{
						auto const & curr = movePath->get_path_nodes()[switchToPathIdx];
						if (curr.placementAtNode.get_room() != presence->get_in_room() ||
							curr.placementAtNode.node->get_group() != atGroupId)
						{
							// go back as we should be still in that previous room
							--switchToPathIdx;
						}
						else
						{
							if (nextPathIdx > switchToPathIdx)
							{
#ifdef AN_USE_AI_LOG
								pathLog.log(TXT("switch back (imo's room: %S)"), presence->get_in_room()->get_name().to_char());
								{
									LOG_INDENT(pathLog);
									movePath->log_node(pathLog, nextPathIdx, TXT("from "));
									movePath->log_node(pathLog, switchToPathIdx, TXT("  to "));
								}
#endif

								// we have to redo some stuff
								nextPathIdx = switchToPathIdx;
								update_next_path_requested();
							}
							break;
						}
					}
				}
				if (nextPathIdx == 0)
				{
					// move to first
					auto const & first = movePath->get_path_nodes().get_first();
					if (!first.is_outdated())
					{
						Transform firstPlacement = get_node_placement(first, imoLoc);
						currentMoveToLocation = firstPlacement.get_translation() + Vector3(0.0f, 0.0f, movementParameters.relativeZ.get(0.0f));

						Vector3 localImoLoc = firstPlacement.location_to_local(imoLoc);
						if (first.door)
						{
							if (presence->get_in_room() == first.door->get_world_active_room_on_other_side() &&
								first.door->get_door_on_other_side()->get_nav_door_node().is_set() &&
								atGroupId == first.door->get_door_on_other_side()->get_nav_door_node()->get_group())
							{
								// we're already in that room? splendid
								nextPathIdx = 1;
#ifdef AN_USE_AI_LOG
								pathLog.log(TXT("we're already in that room? nextPathIdx = 1 (imo's room: %S)"), presence->get_in_room()->get_name().to_char());
#endif
								update_next_path_requested();
							}
						}
						else
						{
							if (localImoLoc.length_2d() < 0.05f) magic_number
							{
								nextPathIdx = 1;
#ifdef AN_USE_AI_LOG
								pathLog.log(TXT("?? nextPathIdx = 1 (imo's room: %S)"), presence->get_in_room()->get_name().to_char());
#endif
								update_next_path_requested();
							}
						}
					}
				}
				if (nextPathIdx > 0)
				{
					while (nextPathIdx < movePath->get_path_nodes().get_size())
					{
						auto const & prev = movePath->get_path_nodes()[nextPathIdx - 1];
						auto const & curr = movePath->get_path_nodes()[nextPathIdx];
						if (curr.is_outdated() || prev.is_outdated())
						{
							currentMoveToLocation.clear();
							shouldStopAndWait = true;
							break;
						}
						Vector3 prevLoc = prev.placementAtNode.get_current_placement().get_translation();
						Vector3 currLoc = curr.placementAtNode.get_current_placement().get_translation();
						Vector3 prev2curr = currLoc - prevLoc;
						float prev2currDist = prev2curr.length();
						Vector3 prev2currDir = prev2currDist != 0.0f ? prev2curr / prev2currDist : prev2curr;
						float t = Vector3::dot(imoLoc - prevLoc, prev2currDir);
						if (curr.door && presence->get_in_room() == curr.door->get_world_active_room_on_other_side() &&
							curr.door->get_door_on_other_side() && curr.door->get_door_on_other_side()->get_nav_door_node().is_set() && 
							atGroupId == curr.door->get_door_on_other_side()->get_nav_door_node()->get_group())
						{
							// we're already in that room? splendid
							++nextPathIdx;
#ifdef AN_USE_AI_LOG
							pathLog.log(TXT("moved through doors, advance nextPathIdx to %i (imo's room: %S)"), nextPathIdx, presence->get_in_room()->get_name().to_char());
#endif
							update_next_path_requested();
							continue;
						}
						if (prev.door)
						{
							// we need to be in the same room
							++nextPathIdx;
#ifdef AN_USE_AI_LOG
							pathLog.log(TXT("advance nextPathIdx to %i (previous is in a different room)"), nextPathIdx);
#endif
							update_next_path_requested();
							continue;
						}
						else if (t > prev2currDist - 0.05f) magic_number
						{
							if (curr.door && presence->get_in_room() == curr.door->get_in_room() &&
								curr.door->get_nav_door_node().is_set() &&
								atGroupId == curr.door->get_nav_door_node()->get_group())
							{
								// we still need to go through that door
								break;
							}
							++nextPathIdx;
#ifdef AN_USE_AI_LOG
							pathLog.log(TXT("advance nextPathIdx to %i (imo's room: %S)"), nextPathIdx, presence->get_in_room()->get_name().to_char());
#endif
							update_next_path_requested();
							continue;
						}
						break;
					}
				}
			}

			if (nextPathIdx >= movePath->get_path_nodes().get_size())
			{
				// remain at last point
				auto const & last = movePath->get_path_nodes().get_last();
				if (last.is_outdated())
				{
					currentMoveToLocation.clear();
					shouldStopAndWait = true;
					moveLost = true;
				}
				else if (presence->get_in_room() == last.placementAtNode.get_room() &&
						 atGroupId == last.placementAtNode.node->get_group())
				{
					currentMoveToLocation = get_node_location(last, imoLoc);
					finalMoveDestination = currentMoveToLocation;
				}
				else
				{
					currentMoveToLocation.clear();
					shouldStopAndWait = true;
					moveLost = true;
				}
			}
			else
			{
				currentMoveToLocationIsIntermediateLocation = true;
				auto const & curr = movePath->get_path_nodes()[nextPathIdx];
				if (curr.is_outdated())
				{
					currentMoveToLocation.clear();
					shouldStopAndWait = true;
				}
				else
				{
					bool blockedTemporarily = false;
					if (curr.connectionData.is_set() && curr.connectionData->is_blocked_temporarily())
					{
						blockedTemporarily = true;
					}
					else
					{
						if (movePath->is_path_blocked_temporarily(nextPathIdx, imoLoc))
						{
							blockedTemporarily = true;
						}
					}
					if (blockedTemporarily)
					{
						currentMoveToLocation.clear();
						shouldStopAndWait = true;
					}
					else if (curr.door && curr.door->get_door() && !curr.door->get_door()->is_open())
					{
						currentMoveToLocation.clear();
						shouldStopAndWait = true;
					}
					else if (curr.door)
					{
						if (wallCrawling || ceilingCrawling) // for ceiling crawling we want to get through the door, we might be on a wall above it
						{
							float inFront = curr.door->get_plane().get_in_front(imoLoc);
							if (inFront < 0.0f)
							{
								// first move to be in front of the doors
								currentMoveToLocation = imoLoc + curr.door->get_plane().get_normal() * (-inFront + 0.5f);
							}
							else
							{
								// move through door, use walls, so just find a point inside hole
								currentMoveToLocation = curr.door->find_inside_hole(imoLoc, 0.3f)
									+ curr.door->get_placement().vector_to_world(Vector3::yAxis * 0.1f);
							}
						}
						else
						{
							// move through door
							currentMoveToLocation = get_node_location(curr, imoLoc)
								+ curr.door->get_placement().vector_to_world(Vector3::yAxis * 0.1f);
						}
					}
					else
					{
						if (wallCrawling) // for ceiling crawling we follow the path - it's just upside down
						{
							if (nextPathIdx > 0)
							{
								auto const& prev = movePath->get_path_nodes()[nextPathIdx - 1];
								auto currLoc = get_node_location(curr, imoLoc);
								currentMoveToLocation = imoLoc + (currLoc - get_node_location(prev, imoLoc)).normal();
								currentMoveToLocation = lerp(0.05f, currentMoveToLocation.get(), currLoc);
							}
							else
							{
								currentMoveToLocation = get_node_location(curr, imoLoc);
							}
						}
						else
						{
							// actually it should find point in future
							currentMoveToLocation = get_node_location(curr, imoLoc);
							if (movePath->get_path_nodes().is_index_valid(nextPathIdx + 1))
							{
								auto const& next = movePath->get_path_nodes()[nextPathIdx + 1];
								if (curr.placementAtNode.node.get() &&
									next.placementAtNode.node.get() &&
									curr.placementAtNode.node.get()->get_nav_mesh() == next.placementAtNode.node.get()->get_nav_mesh())
								{
									nextMoveToLocation = get_node_location(next, imoLoc);
								}
							}
						}
					}
				}
				if (currentMoveToLocation.is_set() && nextPathIdx >= movePath->get_path_nodes().get_size() - 1)
				{
					if (curr.placementAtNode.get_room() == presence->get_in_room())
					{
						finalMoveDestination = currentMoveToLocation;
					}
					else
					{
						todo_note(TXT("implement for different rooms"));
					}
				}
			}
		}

		{
			Vector3 keepDistanceToPoint;
			if (keepDistanceTo.get_location(*this, OUT_ keepDistanceToPoint))
			{
				float verticalMovementRequired = 0.0f;
				Vector3 upDir = get_up_dir(presence);
				if (keepDistance2D)
				{
					Vector3 keepDistanceToPointActual;
					if (keepDistanceTo.get_location(*this, OUT_ keepDistanceToPointActual, LocomotionReference::GetTargetLocation))
					{
						Vector3 difference = currentLocation - keepDistanceToPointActual;
						Vector3 differenceVertical = difference.along_normalised(upDir);
						float differenceVerticalAlong = Vector3::dot(differenceVertical, upDir);
						if (differenceVerticalAlong > keepDistance2DVerticalSeparation)
						{
							verticalMovementRequired = keepDistance2DVerticalSeparation - differenceVerticalAlong;
						}
						else if (differenceVerticalAlong < -keepDistance2DVerticalSeparation)
						{
							verticalMovementRequired = -keepDistance2DVerticalSeparation - differenceVerticalAlong;
						}
					}
				}
				{
					if (keepDistanceTo == moveTo && currentMoveToLocation.is_set())
					{
						// we're moving at the same thing, find location on line between us and target
						Vector3 difference = currentLocation - keepDistanceToPoint;
						if (keepDistance2D)
						{
							difference = difference.drop_using_normalised(upDir);
						}
						debug_draw_arrow(true, Colour::orange, currentLocation, currentLocation + difference);
						float distance = difference.length();
						float const orgDistance = distance;
						// adding move to radius will make us stop at actual radius (we do not modify it!)
						if (keepDistanceMin.is_set() &&
							distance < keepDistanceMin.get() + currentMoveToRadius.max)
						{
							distance = keepDistanceMin.get() + currentMoveToRadius.max;
						}
						if (keepDistanceMax.is_set() &&
							distance > keepDistanceMax.get() - currentMoveToRadius.max)
						{
							distance = keepDistanceMax.get() - currentMoveToRadius.max;
						}
						currentMoveToLocation = currentLocation - difference.normal() * (orgDistance - distance) + upDir * verticalMovementRequired;
						if (abs(orgDistance - distance) > 0.01f)
						{
							currentMoveToLocationIsIntermediateLocation = false;
						}
					}
					else
					{
						if (currentMoveToLocation.is_set())
						{	// find point within valid zone
							todo_note(TXT("try to go around invalid zone, ie. find on the edge?"));
							Vector3 difference = currentMoveToLocation.get() - keepDistanceToPoint;
							if (keepDistance2D)
							{
								difference = difference.drop_using_normalised(upDir);
							}
							float distance = difference.length();
							if (distance < keepDistanceMin.get())
							{
								Vector3 prevMoveToLocation = currentMoveToLocation.get();
								// if both points are really close, prefer closer to our current location
								if (keepDistance2D)
								{
									Vector3 diff = currentLocation - keepDistanceToPoint;
									diff = diff.drop_using_normalised(upDir);
									currentMoveToLocation = keepDistanceToPoint + diff.normal() * keepDistanceMin.get() * 0.5f;
									currentMoveToLocation = currentMoveToLocation.get().drop_using_normalised(upDir) + upDir * prevMoveToLocation.along_normalised(upDir);
								}
								else
								{
									currentMoveToLocation = keepDistanceToPoint - (currentLocation - keepDistanceToPoint).normal() * keepDistanceMin.get() * 0.5f;
								}
								currentMoveToLocationIsIntermediateLocation = false;
								// recalculate difference
								difference = currentMoveToLocation.get() - keepDistanceToPoint;
								if (keepDistance2D)
								{
									difference = difference.drop_using_normalised(upDir);
								}
								// and distance
								distance = difference.length();
							}
							float const orgDistance = distance;
							// adjust distances by radiuses, so we keep radius but take it into account when calculating destination point
							todo_note(TXT("we are clearing currentMoveToRadius as we want to keep distance from object!"));
							if (keepDistanceMin.is_set() &&
								distance < keepDistanceMin.get() + currentMoveToRadius.max)
							{
								distance = keepDistanceMin.get() + currentMoveToRadius.max;
							}
							if (keepDistanceMax.is_set() &&
								distance > keepDistanceMax.get() - currentMoveToRadius.max)
							{
								distance = keepDistanceMax.get() - currentMoveToRadius.max;
							}
							currentMoveToLocation = currentMoveToLocation.get() - difference.normal() * (orgDistance - distance); // use current location to maintain alt
							currentMoveToLocation = currentMoveToLocation.get() + upDir * verticalMovementRequired;

							if (abs(orgDistance - distance) > 0.01f)
							{
								currentMoveToLocationIsIntermediateLocation = false;
							}

							if (!currentMoveToLocationIsIntermediateLocation)
							{
								Vector3 difference = currentMoveToLocation.get() - currentLocation;
								if (keepDistance2D)
								{
									Vector3 upDir = get_up_dir(presence);
									difference = difference.drop_using_normalised(upDir);
								}
								if (difference.length() < 0.1f) magic_number
								{
									// we could as well just stop right where we are
									shouldStopAndWait = true;
								}
							}
						}

						{	// always move immediately when within invalid zone
							Vector3 difference = keepDistanceToPoint - currentLocation;
							if (keepDistance2D)
							{
								Vector3 upDir = get_up_dir(presence);
								difference = difference.drop_using_normalised(upDir);
							}
							debug_draw_arrow(true, Colour::orange, currentLocation, currentLocation + difference);
							float distance = difference.length();
							// adjust distances by radiuses, so we keep radius but take it into account when calculating destination point
							todo_note(TXT("we are clearing currentMoveToRadius as we want to keep distance from object!"));
							if (keepDistanceMin.is_set() &&
								distance < keepDistanceMin.get())
							{
								currentMoveToLocation = currentLocation + difference.normal() * (distance - (keepDistanceMin.get()));
								currentMoveToLocation = currentMoveToLocation.get() + upDir * verticalMovementRequired;
								currentMoveToRadius = Range(0.0f);
								currentMoveToLocationIsIntermediateLocation = false;
							}
							if (keepDistanceMax.is_set() &&
								distance > keepDistanceMax.get())
							{
								currentMoveToLocation = currentLocation + difference.normal() * (distance - (keepDistanceMax.get()));
								currentMoveToLocation = currentMoveToLocation.get() + upDir * verticalMovementRequired;
								currentMoveToRadius = Range(0.0f);
								currentMoveToLocationIsIntermediateLocation = false;
							}

							// if we're not moving, make us move up/down if required
							if (!currentMoveToLocation.is_set() && verticalMovementRequired != 0.0f)
							{
								currentMoveToLocation = currentLocation + upDir * verticalMovementRequired;
							}
						}
					}
				}
			}
		}

		if (!shouldStopAndWait && (stopOnActors || stopOnCollision))
		{
			Room* room;
			Transform placementWS;
			todo_note(TXT("different time?"));
			where_I_want_to_be_in(OUT_ room, OUT_ placementWS, 0.5f);

			bool collidedThisFrame = false;
			if (room)
			{
				if (auto* collision = imo->get_collision())
				{
					Collision::Flags collidesWith = collision->get_collides_with_flags();

					for_every_ptr(object, room->get_objects())
					{
						if (object != imo &&
							collision->should_collide_with(object))
						{
							if (Collision::Flags::check(collidesWith, object->get_collision_flags()))
							{
								if (stopOnActors)
								{
									if (auto* actor = fast_cast<Actor>(object))
									{
										if (auto* actorCollision = actor->get_collision())
										{
											Transform actorPlacementWS = actor->get_presence()->get_placement();
											float distance = ModuleCollision::get_distance_between(collision, placementWS, actorCollision, actorPlacementWS);
											if (distance < 0.1f) // leave just a little space
											{
												shouldStopAndWait = true;
												// no need to do any further checks
												break;
											}
										}
									}
								}
								if (stopOnCollision)
								{
									collidedThisFrame = true;
									collisionTime += _deltaTime;
									if (collisionTime > 0.25f)
									{
										shouldStopAndWait = true;
										// no need to do any further checks
										break;
									}
								}
							}
						}
					}
				}
			}
			if (!collidedThisFrame)
			{
				collisionTime = 0.0f;
			}
		}
		else
		{
			collisionTime = 0.0f;
		}

		controller->clear_distance_to_stop();
		controller->clear_distance_to_slow_down();
		if (shouldStopAndWait)
		{
			MovementParameters stopMovementParameters = movementParameters;
			stopMovementParameters.stop();
			controller->set_requested_movement_parameters(stopMovementParameters);
			controller->clear_requested_velocity_linear();
			debug_draw_sphere(true, true, Colour::orange, 0.2f, Sphere(currentLocation, 0.01f));
		}
		else if (currentMoveToLocation.is_set())
		{
			debug_draw_arrow(true, Colour::green, currentLocation, currentMoveToLocation.get());
			debug_draw_sphere(true, true, Colour::green, 0.2f, Sphere(currentLocation, 0.01f));

			Vector3 difference = currentMoveToLocation.get() - currentLocation;
			Optional<Vector3> finalDifference;
			if (finalMoveDestination.is_set())
			{
				finalDifference = finalMoveDestination.get() - currentLocation;
			}
			if (in2D)
			{
				Vector3 upDir = get_up_dir(presence);
				difference = difference.drop_using_normalised(upDir);
				if (finalDifference.is_set())
				{
					finalDifference = finalDifference.get().drop_using_normalised(upDir);
				}
			}
			float distance = difference.length();
			Optional<float> finalDistance;
			if (finalMoveDestination.is_set() && finalDifference.is_set())
			{
				finalDistance = finalDifference.get().length();
				moveDistanceLeft.set(finalDistance);
			}
			else
			{
				moveDistanceLeft.clear();
			}
			{
				Optional<float> slowDownPercentage;
				if (nextMoveToLocation.is_set())
				{
					Vector3 nextDir = (nextMoveToLocation.get() - currentMoveToLocation.get()).normal();
					Vector3 currentDir = (currentMoveToLocation.get() - currentLocation).normal();
					slowDownPercentage = clamp(0.1f + 1.8f * Vector3::dot(currentDir, nextDir) - 0.7f, 0.1f, 1.0f);
				}
				controller->set_distance_to_slow_down(distance, slowDownPercentage);
			}
			if (finalDistance.is_set())
			{
				controller->set_distance_to_stop(finalDistance.get());
			}
			else
			{
				controller->clear_distance_to_stop();
			}
			if (movePath.is_set())
			{
				if (movePath->get_final_room() != presence->get_in_room())
				{
					// we still have to go to a different room
					distance = currentMoveToRadius.max + 0.1f;
				}
				else if (finalDistance.is_set())
				{
					distance = finalDistance.get();
				}
				else
				{
					// no final destination, just go
					distance = currentMoveToRadius.max + 0.1f;
				}
			}
			if (movePath.is_set() && movement->find_allow_movement_direction_difference_of_current_gait() > 0.0f)
			{
				// this code helps moving through doors if we're keen on stopping to turn
				// check if we haven't went through some door, if so, keep going in the same direction
				int checkPathIdx = nextPathIdx;
				int firstInRoomIdx = checkPathIdx;
				int lastInRoomIdx = checkPathIdx;
				int lastPathIdx = movePath->get_path_nodes().get_size() - 1;
				while (checkPathIdx <= lastPathIdx && movePath->get_path_nodes().is_index_valid(checkPathIdx))
				{
					auto const& checkPN = movePath->get_path_nodes()[checkPathIdx];
					if (checkPN.placementAtNode.get_room() == presence->get_in_room())
					{
						// go back as we should be still in that previous room
						lastInRoomIdx = checkPathIdx;
						++checkPathIdx;
					}
					else
					{
						break;
					}
				}
				checkPathIdx = nextPathIdx;
				while (checkPathIdx > 0 && movePath->get_path_nodes().is_index_valid(checkPathIdx))
				{
					auto const& checkPN = movePath->get_path_nodes()[checkPathIdx];
					if (checkPN.placementAtNode.get_room() == presence->get_in_room())
					{
						// go back as we should be still in that previous room
						firstInRoomIdx = checkPathIdx;
						--checkPathIdx;
					}
					else
					{
						break;
					}
				}
				Vector3 forwardDir = currentOrientation.get_axis(Axis::Forward);
				if (firstInRoomIdx > 0 && movePath->get_path_nodes().is_index_valid(firstInRoomIdx))
				{
					auto const & checkPN = movePath->get_path_nodes()[firstInRoomIdx];
					Vector3 checkLoc = get_node_location(checkPN, currentLocation);
					float distToCheckLoc = (currentLocation - checkLoc).length();
					if (distToCheckLoc < 0.05f)
					{
						// keep moving inside
						distance = currentMoveToRadius.max + 0.1f;
						difference = forwardDir;
					}
				}
				if (lastInRoomIdx < lastPathIdx && movePath->get_path_nodes().is_index_valid(lastInRoomIdx))
				{
					auto const & checkPN = movePath->get_path_nodes()[lastInRoomIdx];
					Vector3 checkLoc = get_node_location(checkPN, currentLocation);
					float distToCheckLoc = (currentLocation - checkLoc).length();
					if (distToCheckLoc < 0.05f &&
						Vector3::dot(checkLoc - currentLocation, forwardDir) > 0.0f)
					{
						// keep moving inside
						distance = currentMoveToRadius.max + 0.1f;
						difference = forwardDir;
					}
				}
			}
			if (currentMoveToRadius.does_contain(distance) &&
				(!movePath.is_set() || (!movePath->is_transport_path() || movePath->get_final_room() == presence->get_in_room())))
			{
				controller->set_requested_velocity_linear(Vector3::zero); // stand
				controller->clear_requested_movement_direction();
			}
			else
			{
				if (distance < currentMoveToRadius.min)
				{
					difference = -difference; // move away
				}
				if (wallCrawling && !difference.is_zero() && wallCrawlingYawOffset != 0.0f)
				{
					Vector3 upDir = get_up_dir(presence);
					Matrix33 lookMatrix = look_matrix33(difference.normal(), upDir);
					difference = lookMatrix.to_world(Rotator3(0.0f, wallCrawlingYawOffset, 0.0f).get_forward() * difference.length());
				}

				controller->set_requested_movement_direction(difference);
				controller->set_requested_movement_parameters(movementParameters);
				controller->clear_requested_velocity_linear();
				movementDirection = difference.normal();
			}
		}
		else
		{
			debug_draw_sphere(true, true, Colour::red, 0.2f, Sphere(currentLocation, 0.01f));
		}

#ifdef AN_DEVELOPMENT
		if (moveDistanceLeft.is_set())
		{
			debug_draw_text(true, Colour::green, currentLocation, NP, NP, NP, false, TXT("left: %.3f"), moveDistanceLeft.get());
		}
		else
		{
			debug_draw_text(true, Colour::green, currentLocation, NP, NP, NP, false, TXT("left: --"));

		}
#endif
		debug_draw_arrow(true, Colour::yellow, currentLocation, currentLocation + movementDirection);
	}

	// turning

	if (turnControl == LocomotionTurnControl::DontTurn)
	{
		controller->clear_requested_orientation();
		controller->clear_requested_look_orientation();
		controller->clear_requested_relative_look();
		controller->clear_requested_velocity_orientation();
		controller->clear_snap_yaw_to_look_orientation();
		controller->clear_follow_yaw_to_look_orientation();
		turnDistanceLeft.set(0.0f);
	}

	{
		bool in2D = false;
		Optional<Vector3> turnToPoint;

		if (LocomotionTurnControl::is_turn_towards(turnControl) &&
			turnTowards.is_set())
		{
			Vector3 turnToPointTemp;
			if (turnTowards.get_location(*this, OUT_ turnToPointTemp))
			{
				in2D = LocomotionTurnControl::is_2d(turnControl);
				turnToPoint = turnToPointTemp;
			}
		}
		
		if (turnToPoint.is_set())
		{
			bool turn = false;
			Vector3 difference = turnToPoint.get() - currentLocation;
			Vector3 upDir = get_up_dir(presence);
			if (in2D)
			{
				difference = difference.drop_using_normalised(upDir);
			}
			if (difference.is_zero())
			{
				turnDistanceLeft = 0.0f;
			}
			else
			{
				Matrix33 destMatrix = look_matrix33(difference.normal(), upDir);
				if (turnYawOffset.is_set())
				{
					destMatrix = destMatrix.to_world(Rotator3(0.0f, turnYawOffset.get(0.0f), 0.0f).to_quat().to_matrix_33());
				}
				Quat const destQuat = destMatrix.to_quat();
				Quat const diffQuat = currentOrientation.to_local(destQuat);
				Rotator3 const diffRot = diffQuat.to_rotator();
				float distance = in2D ? abs(diffRot.yaw) : diffRot.length();
				turnDistanceLeft = distance;
				if (distance > turnDeadZone)
				{
					turn = true;
					controller->set_requested_orientation(destQuat);
					controller->clear_requested_velocity_orientation();
				}
			}
			if (!turn)
			{
				controller->set_requested_orientation(currentOrientation);
				controller->clear_requested_velocity_orientation();
			}
		}
	}

	bool turnInMovementDirection = false;

	if (LocomotionTurnControl::is_following_path(turnControl))
	{
		if (movement->find_allow_movement_direction_difference_of_current_gait() > 0.0f)
		{
			// if movement requires to stop, don't do early turns, just follow where we want to face
			turnInMovementDirection = true;
		}
		else
		{
			if (movePath.is_set() && movePath->get_path_nodes().is_index_valid(nextPathIdx) && nextPathIdx > 0)
			{
				bool in2D = LocomotionTurnControl::is_2d(turnControl);
				Vector3 nextPathPoint = movePath->get_path_nodes()[nextPathIdx].placementAtNode.get_current_placement().get_translation();
				Vector3 prevPathPoint = movePath->get_path_nodes()[nextPathIdx - 1].placementAtNode.get_current_placement().get_translation();
				Vector3 turnInDirection = nextPathPoint - currentLocation;
				Vector3 prevToNextDir = nextPathPoint - prevPathPoint;
				if (!prevToNextDir.is_zero())
				{
					float alongP2N = Vector3::dot(-turnInDirection, prevToNextDir);
					if (alongP2N > -0.5f)
					{
						// when close, use pure prev to next dir
						float useP2N = clamp((alongP2N - (-0.5f)) / 0.4f, 0.0f, 1.0f);
						turnInDirection = useP2N * prevToNextDir + (1.0f - useP2N) * turnInDirection.normal();
					}
				}
				Vector3 upDir = get_up_dir(presence);
				if (in2D)
				{
					turnInDirection = turnInDirection.drop_using_normalised(upDir);
				}

				if (!turnInDirection.is_zero())
				{
					// keep same up axis
					Matrix33 movMatrix = look_matrix33(turnInDirection.normal(), upDir);
					if (turnYawOffset.is_set())
					{
						movMatrix = movMatrix.to_world(Rotator3(0.0f, turnYawOffset.get(0.0f), 0.0f).to_quat().to_matrix_33());
					}
					Quat const movQuat = movMatrix.to_quat();
					controller->set_requested_orientation(movQuat);

					Quat const diffQuat = currentOrientation.to_local(movQuat);
					Rotator3 const diffRot = diffQuat.to_rotator();
					float distance = in2D ? abs(diffRot.yaw) : diffRot.length();
					turnDistanceLeft.set(distance);
				}
			}
		}
		controller->clear_requested_velocity_orientation();
		controller->clear_snap_yaw_to_look_orientation();
		controller->clear_follow_yaw_to_look_orientation();
	}

	if (LocomotionTurnControl::is_in_movement_direction(turnControl) || turnInMovementDirection)
	{
		if (movementDirection.is_zero())
		{
			controller->set_requested_orientation(currentOrientation); // keep current orientation
			turnDistanceLeft.set(0.0f);
		}
		else
		{
			bool in2D = LocomotionTurnControl::is_2d(turnControl);
			Vector3 turnInDirection = movementDirection;
			Vector3 upDir = get_up_dir(presence);
			if (in2D)
			{
				turnInDirection = turnInDirection.drop_using_normalised(upDir);
			}
			turnInDirection = turnInDirection.is_zero()? turnInDirection : turnInDirection.normal();
			if (turnInDirection.is_zero() || Vector3::dot(turnInDirection, upDir) > 0.990f)
			{
				controller->set_requested_orientation(currentOrientation); // keep current orientation
				turnDistanceLeft.set(0.0f);
			}
			else
			{
				// keep same up axis
				Matrix33 movMatrix = look_matrix33(turnInDirection, upDir);
				if (turnYawOffset.is_set())
				{
					movMatrix = movMatrix.to_world(Rotator3(0.0f, turnYawOffset.get(0.0f), 0.0f).to_quat().to_matrix_33());
				}
				Quat const movQuat = movMatrix.to_quat();
				controller->set_requested_orientation(movQuat);

				Quat const diffQuat = currentOrientation.to_local(movQuat);
				Rotator3 const diffRot = diffQuat.to_rotator();
				float distance = in2D ? abs(diffRot.yaw) : diffRot.length();
				turnDistanceLeft.set(distance);
			}
		}
		controller->clear_requested_velocity_orientation();
		controller->clear_snap_yaw_to_look_orientation();
		controller->clear_follow_yaw_to_look_orientation();
	}

	if (allowWallCrawling)
	{
		bool shouldFall = false;
		if (is_following_path()) // we don't care what they do when they're not following a path
		{
			if (!wallCrawling && !ceilingCrawling && imo->get_presence()->does_allow_falling())
			{
				//
				Vector3 gravityUpDir = -imo->get_presence()->get_gravity_dir();
				Vector3 upDir = imo->get_presence()->get_placement().get_axis(Axis::Up);
				if (!gravityUpDir.is_zero() &&
					Vector3::dot(gravityUpDir, upDir) < 0.3f)
				{
					shouldFall = true;
				}
			}
		}
		if (fallFromCrawling != shouldFall)
		{
			fallFromCrawling = shouldFall;
			imo->access_variables().access<bool>(NAME(locomotionShouldFall)) = shouldFall;
		}
	}

	debug_no_context();
	debug_no_filter();
	debug_no_subject();
}

void Locomotion::update_next_path_requested()
{
	if (movePath.is_set() && movePath->get_path_nodes().is_index_valid(nextPathIdx))
	{
		nextPathRequested.request(movePath->get_path_nodes()[nextPathIdx].placementAtNode.node.get());
		{
			bool found = false;
			for (int idx = nextPathIdx + 1; idx < movePath->get_path_nodes().get_size(); ++idx)
			{
				auto& pathNode = movePath->get_path_nodes()[idx];
				if (auto* node = pathNode.placementAtNode.node.get())
				{
					if (node->is_important_path_node())
					{
						importantPathRequested.request(node);
						found = true;
						break;
					}
				}
			}
			if (!found)
			{
				importantPathRequested.clear();
			}
		}
	}
	else
	{
		nextPathRequested.clear();
		importantPathRequested.clear();
	}
}

void Locomotion::debug_draw()
{
	debug_filter(locomotionPath);
	debug_subject(mind->get_owner_as_modules_owner());

	if (movePath.is_set())
	{
		Nav::PathNode const * prev = nullptr;
		for_every(pathNode, movePath->get_path_nodes())
		{
			if (prev &&
				prev->placementAtNode.node->get_room() == pathNode->placementAtNode.node->get_room())
			{
				debug_context(pathNode->placementAtNode.get_room());
				Colour colour = Colour::yellow;
				if (! pathNode->is_outdated() && pathNode->connectionData.is_set() && pathNode->connectionData->is_blocked_temporarily())
				{
					colour = Colour::magenta;
				}
				else if (movePath->is_path_blocked_temporarily(for_everys_index(pathNode), prev->placementAtNode.get_current_placement().get_translation()))
				{
					colour = Colour::magenta;
				}
				if (for_everys_index(pathNode) < nextPathIdx)
				{
					colour = colour * 0.5f;
					colour.a *= 0.5f;
				}
				if (for_everys_index(pathNode) == nextPathIdx)
				{
					colour.a *= 0.5f;
					debug_draw_arrow(true, Colour::green, mind->get_owner_as_modules_owner()->get_presence()->get_placement().get_translation(),
						pathNode->placementAtNode.get_current_placement().get_translation());
				}
				debug_draw_arrow(true, colour, prev->placementAtNode.get_current_placement().get_translation(),
					pathNode->placementAtNode.get_current_placement().get_translation());
				debug_no_context();
			}
			prev = pathNode;
		}
	}

	debug_no_subject();
	debug_no_filter();
}

void Locomotion::debug_log(LogInfoContext & _log)
{
	IModulesOwner* imo = mind->get_owner_as_modules_owner();
	_log.log(TXT("locomotion for %S"), imo->ai_get_name().to_char());
	LOG_INDENT(_log);

	_log.log(TXT("move: %S"), LocomotionMoveControl::to_char(moveControl));
	{
		LOG_INDENT(_log);
		if (LocomotionMoveControl::is_path_following(moveControl))
		{
			if (movePath.is_set())
			{
				_log.log(TXT("next path idx: %i (count %i)"), nextPathIdx, movePath->get_path_nodes().get_size());
				for_count(int, idx, movePath->get_path_nodes().get_size())
				{
					movePath->log_node(_log, idx, idx == nextPathIdx ? TXT("> ") : TXT("  "));
				}
			}
			else
			{
				_log.log(TXT("NO PATH!"));
			}
		}
	}
	_log.log(TXT("turn: %S"), LocomotionTurnControl::to_char(turnControl));
	if (keepDistanceTo.is_set())
	{
		LOG_INDENT(_log);
		_log.log(TXT("keep distance %S"), keepDistance2D? TXT("2D") : TXT("3D"));
		if (auto* aio = keepDistanceTo.get_object())
		{
			_log.log(TXT("to object: %S"), aio->ai_get_name().to_char());
		}
		Vector3 keepDistanceToPoint;
		if (keepDistanceTo.get_location(*this, OUT_ keepDistanceToPoint))
		{
			_log.log(TXT("to point: %S"), keepDistanceToPoint.to_string().to_char());
		}
		if (keepDistanceMin.is_set())
		{
			_log.log(TXT("min: %.3f"), keepDistanceMin.get());
		}
		if (keepDistanceMax.is_set())
		{
			_log.log(TXT("max: %.3f"), keepDistanceMax.get());
		}
		if (keepDistance2D)
		{
			_log.log(TXT("vertical separation: %.3f"), keepDistance2DVerticalSeparation);
		}
	}
#ifdef AN_USE_AI_LOG
	_log.log(TXT("path log"));
	{
		LOG_INDENT(_log);
		_log.log(pathLog);
	}
#endif
}

bool Locomotion::calc_distance_to_keep_distance(float & _distance) const
{
	IModulesOwner* imo = mind->get_owner_as_modules_owner();
	auto* presence = imo ? imo->get_presence() : nullptr;
	if (!presence)
	{
		return false;
	}

	Vector3 currentLocation = presence->get_placement().get_translation();

	Vector3 keepDistanceToPoint;
	if (keepDistanceTo.get_location(*this, OUT_ keepDistanceToPoint))
	{
		Vector3 difference = currentLocation - keepDistanceToPoint;
		if (keepDistance2D)
		{
			Vector3 upDir = get_up_dir(presence);
			Vector3 keepDistanceToPointActual;
			if (keepDistanceTo.get_location(*this, OUT_ keepDistanceToPointActual, LocomotionReference::GetPresenceCentre))
			{
				difference = currentLocation - keepDistanceToPointActual;
				Vector3 differenceVertical = difference.along_normalised(upDir);
				if (differenceVertical.length() > keepDistance2DVerticalSeparation)
				{
					return false;
				}
			}
			_distance = difference.drop_using_normalised(upDir).length();
			return true;
		}
		else
		{
			_distance = difference.length();
			return true;
		}
	}
	return false;
}

bool Locomotion::where_I_want_to_be_in(OUT_ Room*& _room, OUT_ Transform& _placementWS, float _time, OPTIONAL_ OUT_ Transform* _intoRoom) const
{
	bool result = true;

	auto* owner = mind->get_owner_as_modules_owner();
	
	auto* presence = owner->get_presence();

	Room* room = presence->get_in_room();
	Transform placementWS = presence->get_placement();
	Transform intoRoom = Transform::identity;

	Vector3 currentLocation = placementWS.get_translation();

	{
		float requestedSpeed = 0.0f;
		if (auto* movement = owner->get_movement())
		{
			owner->get_movement()->calculate_requested_linear_speed(movementParameters, OUT_ &requestedSpeed);
		}
		float distLeft = _time * requestedSpeed;

		if (LocomotionMoveControl::is_path_following(moveControl) &&
			movePath.is_set())
		{
			bool in2D = LocomotionMoveControl::is_2d(moveControl);

			int toIdx = nextPathIdx;

			while (distLeft > 0.0f)
			{
				Vector3 to = Vector3::zero;
				if (toIdx == 0)
				{
					auto const & first = movePath->get_path_nodes().get_first();
					if (!first.is_outdated())
					{
						to = get_node_location(first, currentLocation) - placementWS.get_translation();
					}
				}
				else if (movePath->get_path_nodes().is_index_valid(toIdx))
				{
					auto & pathNode = movePath->get_path_nodes()[toIdx];
					if (pathNode.placementAtNode.get_room() != room)
					{
						// tough luck, we're in a different room, we should try our luck next time, mark that we should not be trusted
						result = false;
						break;
					}
					to = get_node_location(pathNode, currentLocation) - placementWS.get_translation();
				}
				else
				{
					// end of path!
					break;
				}

				if (!to.is_zero())
				{
					Vector3 upDir = get_up_dir(presence);
					if (in2D)
					{
						to = to.drop_using_normalised(upDir);
					}
					float length = to.length();

					if (wallCrawling || ceilingCrawling)
					{
						// if the target is above us and we're wall/ceiling crawling, just move forward
						// this way we should eventually hit a wall and crawl it till we end up closer to the enemy
						float const xyLimit = 1.0f;
						float const zAboveOrMore = 1.0f;
						if (to.length_2d() < xyLimit && to.z > zAboveOrMore)
						{
							// just move forward
							to = presence->get_placement().get_axis(Axis::Forward) * to.z;
						}
					}
					if (wallCrawling && !to.is_zero() && wallCrawlingYawOffset != 0.0f)
					{
						Vector3 upDir = get_up_dir(presence);
						Matrix33 lookMatrix = look_matrix33(to.normal(), upDir);
						to = lookMatrix.to_world(Rotator3(0.0f, wallCrawlingYawOffset, 0.0f).get_forward() * to.length());
					}

					if (distLeft <= length)
					{
						Vector3 toNormal = to.normal();
						placementWS.set_translation(placementWS.get_translation() + toNormal * distLeft);
						if (LocomotionTurnControl::is_in_movement_direction(turnControl))
						{
							todo_note(TXT("uses same 2d as for movement"));
							Matrix33 lookMatrix = look_matrix33(toNormal, upDir);
							if (turnYawOffset.is_set())
							{
								lookMatrix = lookMatrix.to_world(Rotator3(0.0f, turnYawOffset.get(0.0f), 0.0f).to_quat().to_matrix_33());
							}
							placementWS.set_orientation(lookMatrix.to_quat());
						}
						break;
					}
					distLeft -= length;
					placementWS.set_translation(placementWS.get_translation() + to);
				}

				// switch to another room if required
				{
					auto & pathNode = movePath->get_path_nodes()[toIdx];
					if (pathNode.door &&
						pathNode.door->has_world_active_room_on_other_side())
					{
						room = pathNode.door->get_world_active_room_on_other_side();
						placementWS = pathNode.door->get_other_room_transform().to_local(placementWS);
						intoRoom = intoRoom.to_world(pathNode.door->get_other_room_transform());
					}
				}
				++toIdx;
			}
		}

		todo_note(TXT("support keeping distance"));
	}

	{
		Rotator3 rotationRequired = Rotator3::zero;
		Rotator3 rotationMaxSpeed = Rotator3::zero;
		if (auto* movement = owner->get_movement())
		{
			rotationMaxSpeed = owner->get_movement()->calculate_requested_orientation_speed(movementParameters);
		}
		Rotator3 rotationWillCover = rotationMaxSpeed * _time;

		if (LocomotionTurnControl::is_turn_towards(turnControl))
		{
			bool in2D = false;
			Optional<Vector3> turnToPoint; // this is in our current room
			{
				Vector3 turnToPointTemp;
				if (turnTowards.get_location(*this, OUT_ turnToPointTemp))
				{
					in2D = LocomotionTurnControl::is_2d(turnControl);
					turnToPoint = turnToPointTemp;
				}
			}

			if (turnToPoint.is_set())
			{
				Transform predictedPlacement = intoRoom.to_world(placementWS);
				Vector3 predictedLocation = predictedPlacement.get_translation();

				// handle calculations in our current room (to get enviornment up dir)
				Vector3 to = turnToPoint.get() - predictedLocation;
				Vector3 upDir = get_up_dir(presence);
				if (in2D)
				{
					to = to.drop_using_normalised(upDir);
				}

				if (turnYawOffset.is_set())
				{
					Matrix33 lookMatrix = look_matrix33(to.normal(), upDir);
					lookMatrix = lookMatrix.to_world(Rotator3(0.0f, turnYawOffset.get(0.0f), 0.0f).to_quat().to_matrix_33());
					to = lookMatrix.get_y_axis();
				}

				Vector3 requiredLocal = predictedPlacement.vector_to_local(to);
				rotationRequired = requiredLocal.to_rotator();
			}
		}

		if (!rotationRequired.is_zero())
		{
			Rotator3 rotationCovered;
			rotationCovered.yaw = clamp(rotationRequired.yaw, -rotationWillCover.yaw, rotationWillCover.yaw);
			rotationCovered.pitch = clamp(rotationRequired.pitch, -rotationWillCover.pitch, rotationWillCover.pitch);
			rotationCovered.roll = clamp(rotationRequired.roll, -rotationWillCover.roll, rotationWillCover.roll);
			placementWS.set_orientation(placementWS.get_orientation().to_world(rotationRequired.to_quat()));
		}
	}

	_room = room;
	_placementWS = placementWS;
	assign_optional_out_param(_intoRoom, intoRoom);

	return result;
}

bool Locomotion::check_if_path_is_ok(Nav::Path* _path) const
{
	if (!_path)
	{
		return false;
	}
	int idx = 0;
	if (_path == movePath.get())
	{
		idx = max(0, nextPathIdx - 1);
	}
	while (idx < _path->get_path_nodes().get_size())
	{
		auto& pathNode = _path->get_path_nodes()[idx];
		if (auto* node = pathNode.placementAtNode.node.get())
		{
			if (node->is_outdated())
			{
				return false;
			}
			if (!_path->is_through_closed_doors()) // if we didn't mind it then, don't bring it up now
			{
				if (auto* dir = node->get_door_in_room())
				{
					if (auto* door = dir->get_door())
					{
						if (!DoorOperation::is_likely_to_be_open(door->get_operation()))
						{
							// won't open anytime soon
							return false;
						}
					}
				}
			}
		}
		++idx;
	}
	return true;
}

Vector3 Locomotion::get_up_dir(ModulePresence* _presence) const
{
	if (wallCrawling || ceilingCrawling)
	{
		return _presence->get_placement().get_axis(Axis::Up);
	}
	else
	{
		return _presence->get_environment_up_dir();
	}
}

Transform Locomotion::get_node_placement(Nav::PathNode const& _node, Vector3 const& _imoLoc) const
{
	Transform placement = _node.placementAtNode.get_current_placement();
	
	if (wallCrawling)
	{
		Segment s(placement.get_translation(), placement.location_to_world(Vector3(0.0f, 0.0f, wallCeilingCrawlingRoomHeight)));
		float tWithin = s.get_t(_imoLoc);
		Vector3 loc = s.get_at_t(tWithin);
		placement.set_translation(loc);
	}
	else if (ceilingCrawling)
	{
		Vector3 floorLoc = placement.get_translation();
		Vector3 ceilDir = placement.get_axis(Axis::Up);
		float imoUpLoc = Vector3::dot(ceilDir, _imoLoc - floorLoc);
		if (imoUpLoc > wallCeilingCrawlingRoomHeight * 0.5f)
		{
			// move it up a bit, in case the ceiling is a bit higher
			imoUpLoc = max(imoUpLoc + 0.2f, wallCeilingCrawlingRoomHeight);
			placement.set_translation(floorLoc + ceilDir * imoUpLoc);
		}
	}

	return placement;
}

Vector3 Locomotion::get_node_location(Nav::PathNode const& _node, Vector3 const& _imoLoc) const
{
	return get_node_placement(_node, _imoLoc).get_translation() + Vector3(0.0f, 0.0f, movementParameters.relativeZ.get(0.0f));
}
