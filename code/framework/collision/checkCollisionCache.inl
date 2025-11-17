#ifdef AN_CLANG
#include "collisionTrace.h"
#endif

template <typename CollisionTraceClass>
void Framework::CheckCollisionCache::build_from_presence_links(Room const * _room, Transform const & _placement, Array<CollisionTraceClass> const & _traces)
{
	Range3 boundingBox = Range3::empty;
	for_every(trace, _traces)
	{
		for_every(loc, trace->get_locations())
		{
			boundingBox.include(trace->get_locations_in() == CollisionTraceInSpace::OS? _placement.location_to_world(*loc) : *loc);
		}
	}
	build_from_presence_links(_room, boundingBox);
}

template <typename CollisionTraceClass>
void Framework::CheckCollisionCache::build_from_presence_links(Room const * _room, Transform const & _placement, CollisionTraceClass const & _trace)
{
	Range3 boundingBox = Range3::empty;
	for_every(loc, _trace.get_locations())
	{
		boundingBox.include(_trace.get_locations_in() == CollisionTraceInSpace::OS ? _placement.location_to_world(*loc) : *loc);
	}
	build_from_presence_links(_room, boundingBox);
}
