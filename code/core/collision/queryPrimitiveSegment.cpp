#include "queryPrimitiveSegment.h"

using namespace Collision;

QueryPrimitiveSegment::QueryPrimitiveSegment(Vector3 const & _locationA, Vector3 const & _locationB)
: locationA(_locationA)
, locationB(_locationB)
{
}

QueryPrimitiveSegment QueryPrimitiveSegment::transform_to_world_of(Matrix44 const & _matrix) const
{
	return QueryPrimitiveSegment(_matrix.location_to_world(locationA), _matrix.location_to_world(locationB));
}

QueryPrimitiveSegment QueryPrimitiveSegment::transform_to_local_of(Matrix44 const & _matrix) const
{
	return QueryPrimitiveSegment(_matrix.location_to_local(locationA), _matrix.location_to_local(locationB));
}

QueryPrimitiveSegment QueryPrimitiveSegment::transform_to_world_of(Transform const & _transform) const
{
	return QueryPrimitiveSegment(_transform.location_to_world(locationA), _transform.location_to_world(locationB));
}

QueryPrimitiveSegment QueryPrimitiveSegment::transform_to_local_of(Transform const & _transform) const
{
	return QueryPrimitiveSegment(_transform.location_to_local(locationA), _transform.location_to_local(locationB));
}

Range3 QueryPrimitiveSegment::calculate_bounding_box(float _radius) const
{
	return Range3(locationA.x < locationB.x ? Range(locationA.x - _radius, locationB.x + _radius) : Range(locationB.x - _radius, locationA.x + _radius),
				  locationA.y < locationB.y ? Range(locationA.y - _radius, locationB.y + _radius) : Range(locationB.y - _radius, locationA.y + _radius),
				  locationA.z < locationB.z ? Range(locationA.z - _radius, locationB.z + _radius) : Range(locationB.z - _radius, locationA.z + _radius));
}
