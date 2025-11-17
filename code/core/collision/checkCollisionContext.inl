#ifdef AN_CLANG
#include "collisionFlags.h"
#endif

Collision::CheckCollisionContext::CheckCollisionContext()
{
	SET_EXTRA_DEBUG_INFO(avoids, TXT("Collision::CheckCollisionContext.avoids"));
}

bool Collision::CheckCollisionContext::should_check(Collision::ICollidableObject const * _object) const
{
	if (!avoids.does_contain(_object))
	{
		if (!_object || Collision::Flags::check(_object->get_collision_flags(), flags))
		{
			if (! checkObject || checkObject(_object))
			{
				return true;
			}
		}
	}
	return false;
}

void Collision::CheckCollisionContext::avoid(Collision::ICollidableObject const * _object)
{
	if (_object && avoids.has_place_left() &&
		!avoids.does_contain(_object))
	{
		avoids.push_back(_object);
	}
}

