#include "iCollidableObject.h"

using namespace Collision;

REGISTER_FOR_FAST_CAST(ICollidableObject);

ICollidableObject::ICollidableObject()
: SafeObject<ICollidableObject>(nullptr)
{
	set_mass_for_collision(0.0f);
	set_collision_flags(Flags::defaults());
	set_collides_with_flags(Flags::defaults());
}

void ICollidableObject::set_mass_for_collision(float _mass)
{
	massForCollision = _mass;
	invertedMassForCollision = _mass > 0.0f? 1.0f / _mass : 0.0f;
}

void ICollidableObject::set_collision_flags(Flags const & _collisionFlags)
{
	collisionFlags = _collisionFlags;
#ifdef AN_DEVELOPMENT
	requiresUpdatingCollisionFlags = false;
#endif
}

void ICollidableObject::set_collides_with_flags(Flags const & _collidesWithFlags)
{
	collidesWithFlags = _collidesWithFlags;
#ifdef AN_DEVELOPMENT
	requiresUpdatingCollidesWithFlags = false;
#endif
}
