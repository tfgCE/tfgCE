#include "queryPrimitivePoint.h"

using namespace Collision;

QueryPrimitivePoint::QueryPrimitivePoint(Vector3 const & _location)
: location(_location)
{
}

QueryPrimitivePoint QueryPrimitivePoint::transform_to_world_of(Matrix44 const & _matrix) const
{
	return QueryPrimitivePoint(_matrix.location_to_world(location));
}

QueryPrimitivePoint QueryPrimitivePoint::transform_to_local_of(Matrix44 const & _matrix) const
{
	return QueryPrimitivePoint(_matrix.location_to_local(location));
}

QueryPrimitivePoint QueryPrimitivePoint::transform_to_world_of(Transform const & _transform) const
{
	return QueryPrimitivePoint(_transform.location_to_world(location));
}

QueryPrimitivePoint QueryPrimitivePoint::transform_to_local_of(Transform const & _transform) const
{
	return QueryPrimitivePoint(_transform.location_to_local(location));
}

Range3 QueryPrimitivePoint::calculate_bounding_box(float _radius) const
{
	return Range3(Range(location.x - _radius, location.x + _radius),
				  Range(location.y - _radius, location.y + _radius),
				  Range(location.z - _radius, location.z + _radius));
}
