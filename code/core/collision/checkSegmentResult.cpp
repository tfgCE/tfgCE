#include "checkSegmentResult.h"

#include "collisionFlags.h"
#include "iCollidableObject.h"
#include "physicalMaterial.h"

Collision::CheckSegmentResult::CheckSegmentResult()
: hit(false)
#ifdef AN_DEVELOPMENT
, model(nullptr)
#endif
, shape(nullptr)
, object(nullptr)
{
}

void Collision::CheckSegmentResult::to_local_of(Transform const & _placement)
{
	hitLocation = _placement.location_to_local(hitLocation);
	hitNormal = _placement.vector_to_local(hitNormal);
	if (gravityDir.is_set())
	{
		gravityDir.set(_placement.vector_to_local(gravityDir.get()));
		an_assert(gravityDir.get().is_normalised());
	}
}

void Collision::CheckSegmentResult::to_world_of(Transform const & _placement)
{
	hitLocation = _placement.location_to_world(hitLocation);
	hitNormal = _placement.vector_to_world(hitNormal);
	if (gravityDir.is_set())
	{
		gravityDir.set(_placement.vector_to_world(gravityDir.get()));
		an_assert(gravityDir.get().is_normalised());
	}
}

bool Collision::CheckSegmentResult::has_collision_flags(Flags const & _flagsToTest, bool _defaultResult) const
{
	if (material)
	{
		return Flags::check(material->get_collision_flags(), _flagsToTest);
	}
	if (object)
	{
		return Flags::check(object->get_collision_flags(), _flagsToTest);
	}
	return _defaultResult;
}
