#pragma once

#include "..\globalDefinitions.h"
#include "..\fastCast.h"
#include "..\memory\safeObject.h"

#include "collisionFlags.h"

namespace Collision
{
	/*
	 *	Owner of collidable shape (might be object or something else)
	 *
	 *	Collision related values are stored inside interface (what kind of interface is it then?! ;) )
	 *	To make it run faster.
	 */
	interface_class ICollidableObject
	: public SafeObject<ICollidableObject>
	{
		FAST_CAST_DECLARE(ICollidableObject);
		FAST_CAST_END();
	public:
		ICollidableObject();
		virtual ~ICollidableObject() {}

		// this doesn't have to reflect shapes' collision flags - this is only to allow objects to have their own (per instance) collision flags
		// for example someone might be changed into ghost - works like a filter
		inline Flags const & get_collision_flags() const { an_assert(!does_require_updating_collision_flags()); return collisionFlags; }

		// this is to define with what does it collide
		inline Flags const & get_collides_with_flags() const { an_assert(!does_require_updating_collides_with_flags()); return collidesWithFlags; }

		// get inverted mass of object (1 / mass) - used when calculating gradient
		inline float get_mass_for_collision() const { return massForCollision; }
		inline float get_inverted_mass_for_collision() const { return invertedMassForCollision; }

	public: // setters
		void set_mass_for_collision(float _mass);

		void set_collision_flags(Flags const & _collisionFlags);
		void set_collides_with_flags(Flags const & _collidesWithFlags);

#ifdef AN_DEVELOPMENT
		bool does_require_updating_collision_flags() const { return requiresUpdatingCollisionFlags; }
		bool does_require_updating_collides_with_flags() const { return requiresUpdatingCollidesWithFlags; }
		void mark_requires_updating_collision_flags() { requiresUpdatingCollisionFlags = true; }
		void mark_requires_updating_collides_with_flags() { requiresUpdatingCollidesWithFlags = true; }
#else
		bool does_require_updating_collision_flags() const { return false; }
		bool does_require_updating_collides_with_flags() const { return false; }
#endif

	public: // other methods
		// calculate mass difference - 1.0 means that "me" will be moving
		inline static float calculate_mass_difference(float _invertedMassOfMe, float _invertedMassOfOther)
		{
			float const sum = _invertedMassOfMe + _invertedMassOfOther;
			return sum != 0.0f ? _invertedMassOfMe / sum : 1.0f;
		}

		inline static float calculate_mass_difference(ICollidableObject const * _me, ICollidableObject const * _other)
		{
			float const invertedMassOfMe = _me ? _me->get_inverted_mass_for_collision() : 0.0f;
			float const invertedMassOfOther = _other ?
				(_me && ! Flags::check(_other->get_collides_with_flags(), _me->get_collision_flags())
					 && Flags::check(_me->get_collides_with_flags(), _other->get_collision_flags())? 0.0f // if other can't collide with us but we collide with it, threat it as a wall
																								   : _other->get_inverted_mass_for_collision() ) // both collide with each other
				: 0.0f;
			return calculate_mass_difference(invertedMassOfMe, invertedMassOfOther);
		}

	private:
		float massForCollision;
		float invertedMassForCollision;

		Flags collisionFlags; // filter layed on top of material (if material present)
		Flags collidesWithFlags;

#ifdef AN_DEVELOPMENT
		bool requiresUpdatingCollisionFlags = false;
		bool requiresUpdatingCollidesWithFlags = false;
#endif
	};
};
