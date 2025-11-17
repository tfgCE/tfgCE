#include "shapeSphere.h"
#include "gradientQueryResult.h"
#include "queryPrimitivePoint.h"

using namespace Collision;

Collision::Sphere::Sphere()
{
}

GradientQueryResult Collision::Sphere::get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square) const
{
	// get relative placement
	Vector3 const rel = _point.location - location;

	Vector3 normal;
	float distance;
	rel.normal_and_length(normal, distance);

	distance = distance - radius; // allow negative distance

	return GradientQueryResult(_square ? sign(distance) * sqr(distance) : distance, normal);
}

ShapeAgainstPlanes::Type Collision::Sphere::check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const
{
	ShapeAgainstPlanes::Type result = ShapeAgainstPlanes::Inside;
	for_every(plane, _clipPlanes.planes)
	{
		ShapeAgainstPlanes::Type againstPlane = check_against_plane(*plane, _threshold);
		if (againstPlane == ShapeAgainstPlanes::Outside)
		{
			result = againstPlane;
			break;
		}
		else if (againstPlane == ShapeAgainstPlanes::Intersecting)
		{
			result = ShapeAgainstPlanes::Intersecting;
		}
	}
	return result;
}

ShapeAgainstPlanes::Type Collision::Sphere::check_against_plane(Plane const & _clipPlane, float _threshold) const
{
	float inFront = _clipPlane.get_in_front(location);

	if (inFront >= radius + _threshold)
	{
		return ShapeAgainstPlanes::Inside;
	}
	else if (inFront <= -radius + _threshold)
	{
		return ShapeAgainstPlanes::Outside;
	}
	else
	{
		return ShapeAgainstPlanes::Intersecting;
	}
}
