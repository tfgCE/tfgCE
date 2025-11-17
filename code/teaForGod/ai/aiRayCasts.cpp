#include "aiRayCasts.h"

#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\moduleCollision.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\world\room.h"

//

//#define DEBUG_DRAW_RAY_CASTS

//

bool TeaForGodEmperor::AI::do_ray_cast(CastInfo const & _castInfo, Vector3 const & _startWS, Framework::IModulesOwner* _owner, Framework::IModulesOwner* _enemy, Framework::Room* _enemyRoom, Transform const & _enemyPlacementInOwnerRoom, OUT_ Framework::CheckSegmentResult & _result, Framework::RelativeToPresencePlacement * _fillRelativeToPresencePlacement)
{
	Vector3 enemyOffsetOS = Vector3::zero;
	if (_enemy)
	{
		if (_castInfo.atTarget.get(false))
		{
			enemyOffsetOS = _owner->get_ai()->get_target_placement_os_for(_enemy).get_translation();
		}
		else if (_castInfo.atCentre.get(false))
		{
			enemyOffsetOS = _enemy->get_presence()->get_centre_of_presence_os().get_translation();
		}
		else if (!_castInfo.atTarget.is_set() && Random::get_chance(0.9f))
		{
			enemyOffsetOS = _owner->get_ai()->get_target_placement_os_for(_enemy).get_translation();
		}
		else
		{
			enemyOffsetOS = _enemy->get_presence()->get_random_point_within_presence_os(0.5f);
		}
	}

	// get location in our room
	Vector3 ePointInOurRoom = _enemyPlacementInOwnerRoom.location_to_world(enemyOffsetOS);

	// if we didn't hit anything else and we ended in right room
	Vector3 startLoc = _startWS;
	Vector3 endLoc = ePointInOurRoom;

	// update enemy room
	//todo_important(TXT("enemy room fix and check if ray lands in the right room"));
	/*
	Framework::Room* enemyRoom = _enemyRoom;
	if (_enemy && !enemyOffsetOS.is_zero())
	{
		// to get proper room where that offest point should be
		Transform toWS = _enemyPlacementInOwnerRoom.to_world(Transform(enemyOffsetOS, Quat::identity));
		_enemyRoom->move_through_doors(_enemyPlacementInOwnerRoom.to_world(_enemy->get_presence()->get_centre_of_presence_os()), toWS, enemyRoom);
	}
	*/

	// raycast
	Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
	collisionTrace.add_location(startLoc);
	collisionTrace.add_location(endLoc);
	int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInFinalRoom | Framework::CollisionTraceFlag::ProvideNonHitResult;
	Framework::CheckCollisionContext checkCollisionContext;
	checkCollisionContext.collision_info_needed();
	if (_castInfo.collisionFlags.is_set())
	{
		checkCollisionContext.use_collision_flags(_castInfo.collisionFlags.get());
	}
	else
	{
		checkCollisionContext.use_collision_flags(Collision::DefinedFlags::get_vision_trace());
	}
	checkCollisionContext.avoid(_owner);

	if (_owner->get_presence()->trace_collision(AgainstCollision::Precise, collisionTrace, _result, collisionTraceFlags, checkCollisionContext, _fillRelativeToPresencePlacement))
	{
		// hit something on the way
		return true;
	}

	return false;
}

bool TeaForGodEmperor::AI::check_clear_ray_cast(CastInfo const & _castInfo, Vector3 const & _startWS, Framework::IModulesOwner* _owner, Framework::IModulesOwner* _enemy, Framework::Room* _enemyRoom, Transform const & _enemyPlacementInOwnerRoom, Optional<Transform> const& _enemyOffsetOS)
{
	Vector3 enemyOffsetOS = Vector3::zero;
	if (_enemyOffsetOS.is_set())
	{
		enemyOffsetOS = _enemyOffsetOS.get().get_translation();
	}
	else if (_enemy)
	{
		if (_castInfo.atTarget.get(false))
		{
			enemyOffsetOS = _owner->get_ai()->get_target_placement_os_for(_enemy).get_translation();
		}
		else if (_castInfo.atCentre.get(false))
		{
			enemyOffsetOS = _enemy->get_presence()->get_centre_of_presence_os().get_translation();
		}
		else if (!_castInfo.atTarget.is_set() && Random::get_chance(0.9f))
		{
			enemyOffsetOS = _owner->get_ai()->get_target_placement_os_for(_enemy).get_translation();
		}
		else
		{
			enemyOffsetOS = _enemy->get_presence()->get_random_point_within_presence_os(0.5f);
		}
	}

	// get location in our room
	Vector3 ePointInOurRoom = _enemyPlacementInOwnerRoom.location_to_world(enemyOffsetOS);

	// if we didn't hit anything else and we ended in right room
	Vector3 startLoc = _startWS;
	Vector3 endLoc = ePointInOurRoom;

	// update enemy room
	Framework::Room* enemyRoom = _enemyRoom;
	if (_enemy && !enemyOffsetOS.is_zero())
	{
		// to get proper room where that offset point should be
		Transform toWS = _enemyPlacementInOwnerRoom.to_world(Transform(enemyOffsetOS, Quat::identity));
		_enemyRoom->move_through_doors(_enemyPlacementInOwnerRoom.to_world(_enemy->get_presence()->get_centre_of_presence_os()), toWS, enemyRoom);
	}

	// raycast
	Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
	collisionTrace.add_location(startLoc);
	collisionTrace.add_location(endLoc);
	int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInFinalRoom | Framework::CollisionTraceFlag::ProvideNonHitResult; // to check if we ended up in the right room
	Framework::CheckCollisionContext checkCollisionContext;
	checkCollisionContext.collision_info_needed();
	if (_castInfo.collisionFlags.is_set())
	{
		checkCollisionContext.use_collision_flags(_castInfo.collisionFlags.get());
	}
	else
	{
		checkCollisionContext.use_collision_flags(Collision::DefinedFlags::get_vision_trace());
	}
	// we want only static stuff to check? we could extend it to actors in future maybe?
	checkCollisionContext.start_with_nothing_to_check();
	checkCollisionContext.check_scenery();
	checkCollisionContext.check_room();
	checkCollisionContext.check_room_scenery();
	checkCollisionContext.avoid(_owner);
	checkCollisionContext.avoid(_enemy);

#ifdef DEBUG_DRAW_RAY_CASTS
	debug_context(_owner->get_presence()->get_in_room());
	debug_subject(_owner);
	debug_draw_arrow(true, Colour::white, startLoc, endLoc);
#endif

	Framework::CheckSegmentResult result;
	if (_owner->get_presence()->trace_collision(AgainstCollision::Precise, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
	{
		if (auto* hitImo = fast_cast<Framework::IModulesOwner>(result.object))
		{
			if (auto* p = hitImo->get_presence())
			{
				if (p->is_attached_at_all_to(_enemy))
				{
					// we assume that the room is the same in such a case, it should be rare to happen in first place
					return true;
				}
			}
		}
		// hit something on the way
#ifdef DEBUG_DRAW_RAY_CASTS
		debug_draw_arrow(true, Colour::red, startLoc, result.intoInRoomTransform.location_to_local(result.hitLocation));
		debug_no_subject();
		debug_no_context();
#endif
		return false;
	}

	if (result.inRoom != enemyRoom)
	{
		// different room
#ifdef DEBUG_DRAW_RAY_CASTS
		if (result.inRoom)
		{
			debug_draw_arrow(true, Colour::yellow, startLoc, endLoc);
			debug_no_context();
			debug_context(result.inRoom);
			debug_draw_sphere(true, false, Colour::yellow, 0.2f, Sphere(result.hitLocation, 0.01f));
		}
		debug_no_subject();
		debug_no_context();
#endif
		return false;
	}

	// we reached location!
#ifdef DEBUG_DRAW_RAY_CASTS
	debug_draw_arrow(true, Colour::green, startLoc, endLoc);
	debug_no_subject();
	debug_no_context();
#endif
	return true;
}

bool TeaForGodEmperor::AI::check_clear_perception_ray_cast(CastInfo const& _castInfo, Transform const& _perceptionSocketWS, Optional<Transform> const& _secondaryPerceptionSocketWS, Range const& _perceptionFOV, Range const& _perceptionVerticalFOV, Framework::IModulesOwner const* _owner, Framework::IModulesOwner const* _enemy, Framework::Room const* _enemyRoom, Transform const& _enemyPlacementInOwnerRoom, Optional<Transform> const& _enemyOffsetOS)
{
	Transform usePerceptionSocketWS = _perceptionSocketWS;
	if (_secondaryPerceptionSocketWS.is_set() &&
		_secondaryPerceptionSocketWS.get().get_scale() > 0.0f)
	{
		Vector3 enemyLoc = _enemyPlacementInOwnerRoom.get_translation();
		float distPS = abs(Rotator3::get_yaw(_perceptionSocketWS.location_to_local(enemyLoc)));
		float distSPS = abs(Rotator3::get_yaw(_secondaryPerceptionSocketWS.get().location_to_local(enemyLoc)));
		if (distSPS < distPS)
		{
			usePerceptionSocketWS = _secondaryPerceptionSocketWS.get();
		}
	}
	return check_clear_perception_ray_cast(_castInfo, usePerceptionSocketWS, _perceptionFOV, _perceptionVerticalFOV, _owner, _enemy, _enemyRoom, _enemyPlacementInOwnerRoom, _enemyOffsetOS);
}

bool TeaForGodEmperor::AI::check_clear_perception_ray_cast(CastInfo const & _castInfo, Transform const & _perceptionSocketWS, Range const & _perceptionFOV, Range const& _perceptionVerticalFOV, Framework::IModulesOwner const * _owner, Framework::IModulesOwner const * _enemy, Framework::Room const * _enemyRoom, Transform const & _enemyPlacementInOwnerRoom, Optional<Transform> const& _enemyOffsetOS)
{
	if (!_owner->get_ai())
	{
		return false;
	}

	Vector3 enemyOffsetOS = Vector3::zero;
	ArrayStatic<Vector3, 3> threePointEnemyOffsetsOS; SET_EXTRA_DEBUG_INFO(threePointEnemyOffsetsOS, TXT("TeaForGodEmperor::AI::check_clear_perception_ray_cast.threePointEnemyOffsetsOS"));
	if (_enemyOffsetOS.is_set())
	{
		enemyOffsetOS = _enemyOffsetOS.get().get_translation();
	}
	else if (_enemy)
	{
		if (_castInfo.atTarget.get(false))
		{
			enemyOffsetOS = _owner->get_ai()->get_target_placement_os_for(_enemy).get_translation();
		}
		else if (_castInfo.atCentre.get(false))
		{
			enemyOffsetOS = _enemy->get_presence()->get_centre_of_presence_os().get_translation();
		}
		else if (!_castInfo.atTarget.is_set() && Random::get_chance(0.5f))
		{
			enemyOffsetOS = _owner->get_ai()->get_target_placement_os_for(_enemy).get_translation();
		}
		else
		{
			enemyOffsetOS = _enemy->get_presence()->get_random_point_within_presence_os(0.5f);
		}
		_enemy->get_presence()->get_three_point_of_presence_os(threePointEnemyOffsetsOS);
	}

	// get location in our room
	Vector3 ePointInOurRoom = _enemyPlacementInOwnerRoom.location_to_world(enemyOffsetOS);
	ArrayStatic<Vector3, 3> threePointInOurRoom; SET_EXTRA_DEBUG_INFO(threePointInOurRoom, TXT("TeaForGodEmperor::AI::check_clear_perception_ray_cast.threePointInOurRoom"));
	for_every(tp, threePointEnemyOffsetsOS)
	{
		threePointInOurRoom.push_back(_enemyPlacementInOwnerRoom.location_to_world(*tp));
	}

	// if we didn't hit anything else and we ended in right room
	Vector3 startLoc = _perceptionSocketWS.get_translation();
	Vector3 endLoc = ePointInOurRoom;
	if (_perceptionSocketWS.get_scale() == 0.0f)
	{
		// can't see with zero scale perception socket
		return false;
	}
	{
		bool anythingInFOV = false;
		Rotator3 rayDirLS = _perceptionSocketWS.vector_to_local((endLoc - startLoc).normal()).to_rotator();
		if (_perceptionFOV.does_contain(rayDirLS.yaw) &&
			_perceptionVerticalFOV.does_contain(rayDirLS.pitch))
		{
			anythingInFOV = true;
		}
		else
		{
			for_every(tp, threePointInOurRoom)
			{
				Rotator3 rayDirLS = _perceptionSocketWS.vector_to_local((*tp - startLoc).normal()).to_rotator();
				if (_perceptionFOV.does_contain(rayDirLS.yaw) &&
					_perceptionVerticalFOV.does_contain(rayDirLS.pitch))
				{
					anythingInFOV = true;
					break;
				}
			}
		}
		if (!anythingInFOV)
		{
			// not in fov
			return false;
		}
	}

	// update enemy room
	Framework::Room * enemyRoom = cast_to_nonconst(_enemyRoom);
	if (_enemy && !enemyOffsetOS.is_zero())
	{
		// to get proper room where that offest point should be
		Transform toWS = _enemyPlacementInOwnerRoom.to_world(Transform(enemyOffsetOS, Quat::identity));
		_enemyRoom->move_through_doors(_enemyPlacementInOwnerRoom.to_world(_enemy->get_presence()->get_centre_of_presence_os()), toWS, enemyRoom);
	}

	// raycast
	Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
	collisionTrace.add_location(startLoc);
	collisionTrace.add_location(endLoc);
	int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom | Framework::CollisionTraceFlag::ProvideNonHitResult;
	Framework::CheckCollisionContext checkCollisionContext;
	checkCollisionContext.collision_info_needed();
	if (_castInfo.collisionFlags.is_set())
	{
		checkCollisionContext.use_collision_flags(_castInfo.collisionFlags.get());
	}
	else
	{
		checkCollisionContext.use_collision_flags(Collision::DefinedFlags::get_vision_trace());
	}
	// we want only static stuff to check? we could extend it to actors in future maybe?
	checkCollisionContext.start_with_nothing_to_check();
	checkCollisionContext.check_scenery();
	checkCollisionContext.check_room();
	checkCollisionContext.check_room_scenery();
	checkCollisionContext.avoid(cast_to_nonconst(_owner));
	checkCollisionContext.avoid(cast_to_nonconst(_enemy));

	Framework::CheckSegmentResult result;
	// use precise as sometimes we may have something we can't move through but we can see through
	if (_owner->get_presence()->trace_collision(AgainstCollision::Precise, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
	{
		if (auto* hitImo = fast_cast<Framework::IModulesOwner>(result.object))
		{
			if (auto* p = hitImo->get_presence())
			{
				if (p->is_attached_at_all_to(_enemy))
				{
					// we assume that the room is the same in such a case, it should be rare to happen in first place
					return true;
				}
			}
		}
		// hit something on the way
		return false;
	}

	if (result.inRoom != enemyRoom)
	{
		// different room
		return false;
	}

	// we reached location!
	return true;
}