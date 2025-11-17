#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\fastCast.h"

namespace Framework
{
	namespace CollisionFlag
	{
		enum Type
		{
			Scenery = bit(0), // scenery important for movement
			Wall = bit(1),
			Door = bit(2),
			Invisible = bit(3),
			Item = bit(4),
			Actor = bit(5),
			Projectile = bit(6),
			HoleInDoor = bit(7)
		};
	};

	interface_class ICollidable
	// IDebugableObject,
	// IInteractiveObject
	{
		FAST_CAST_DECLARE(ICollidable);
		FAST_CAST_END();
	public:
		ICollidable();

		/*
		public abstract bool check_collision(ICollidableObject? collider, ref Segment segment, ref HitInfo hitInfo, int colliderFlags, int ignoreCollidersWithFlags = 0);
		public abstract int get_collision_flags(); // flags determining what kind of object it is (what collider it is)
		public virtual bool may_collide_with(ICollidableObject? collider, int colliderFlags, int ignoreCollidersWithFlags = 0)
		{
			int collisionFlags = get_collision_flags();
			return (collisionFlags & colliderFlags) != 0 &&
					(collisionFlags & ignoreCollidersWithFlags) == 0;
		}
		*/
	};

};
