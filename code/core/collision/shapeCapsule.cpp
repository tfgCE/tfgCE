#include "shapeCapsule.h"
#include "gradientQueryResult.h"
#include "queryPrimitivePoint.h"

using namespace Collision;

Collision::Capsule::Capsule()
{
}

GradientQueryResult Collision::Capsule::get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square) const
{
	Vector3 const point2a = _point.location - locationA;
	float const point2aDist = clamp(Vector3::dot(point2a, a2bNormal), 0.0f, a2bDist);

	Vector3 const closest = locationA + point2aDist * a2bNormal;

	// get relative placement
	Vector3 const rel = _point.location - closest;

	Vector3 normal;
	float distance;
	rel.normal_and_length(normal, distance);

	distance = distance - radius; // allow negative distance

	return GradientQueryResult(_square? sign(distance) * sqr(distance) : distance, normal);
}

ShapeAgainstPlanes::Type Collision::Capsule::check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const
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

ShapeAgainstPlanes::Type Collision::Capsule::check_against_plane(Plane const & _clipPlane, float _threshold) const
{
	float inFrontA = _clipPlane.get_in_front(locationA);
	float inFrontB = _clipPlane.get_in_front(locationB);

	if (min(inFrontA, inFrontB) >= radius + _threshold)
	{
		return ShapeAgainstPlanes::Inside;
	}
	else if (max(inFrontA, inFrontB) <= -radius + _threshold)
	{
		return ShapeAgainstPlanes::Outside;
	}
	else
	{
		return ShapeAgainstPlanes::Intersecting;
	}
}
