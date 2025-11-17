#include "shapeBox.h"
#include "gradientQueryResult.h"
#include "queryPrimitivePoint.h"

using namespace Collision;

Collision::Box::Box()
{
}

void Collision::Box::query_distance_axis(Vector3 const & _diff, Vector3 const & _axis, float const _halfSize, OUT_ float & _offsetOutsideBox)
{
	float const dotDiffAxis = Vector3::dot(_diff, _axis);
	float const absDotDiffAxis = abs(dotDiffAxis);
	_offsetOutsideBox = (dotDiffAxis < 0.0f ? -1.0f : 1.0f) * max(0.0f, absDotDiffAxis - _halfSize);
}

GradientQueryResult Collision::Box::get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square) const
{
	an_assert(! xAxis.is_zero());
	an_assert(! yAxis.is_zero());
	an_assert(! zAxis.is_zero());

	// get relative placement
	Vector3 const rel = _point.location - centre;

	float xOffsetOutsideBox;
	float yOffsetOutsideBox;
	float zOffsetOutsideBox;
	query_distance_axis(rel, xAxis, halfSize.x, OUT_ xOffsetOutsideBox);
	query_distance_axis(rel, yAxis, halfSize.y, OUT_ yOffsetOutsideBox);
	query_distance_axis(rel, zAxis, halfSize.z, OUT_ zOffsetOutsideBox);

	float const distanceSq = sqr(xOffsetOutsideBox) + sqr(yOffsetOutsideBox) + sqr(zOffsetOutsideBox);

	if (distanceSq > 0.0f)
	{
		// normal depends on how far away on each axis we are. imagine cases for just plane, edge and point and it will make sense
		Vector3 const offsetOutside = xOffsetOutsideBox * xAxis + yOffsetOutsideBox * yAxis + zOffsetOutsideBox * zAxis;

		return GradientQueryResult(_square? distanceSq : sqrt(distanceSq), offsetOutside.normal());
	}
	else
	{
		float minHalfSize = min(halfSize.x, min(halfSize.y, halfSize.z));

		if (minHalfSize == 0.0f)
		{
			// degenerated box

			Vector3 const offsetOutside = xOffsetOutsideBox * xAxis + yOffsetOutsideBox * yAxis + zOffsetOutsideBox * zAxis;

			return GradientQueryResult(_square? distanceSq : sqrt(distanceSq), offsetOutside.normal());
		}

		float xOffsetOutsideInternalBox;
		float yOffsetOutsideInternalBox;
		float zOffsetOutsideInternalBox;
		query_distance_axis(rel, xAxis, halfSize.x - minHalfSize, OUT_ xOffsetOutsideInternalBox);
		query_distance_axis(rel, yAxis, halfSize.y - minHalfSize, OUT_ yOffsetOutsideInternalBox);
		query_distance_axis(rel, zAxis, halfSize.z - minHalfSize, OUT_ zOffsetOutsideInternalBox);

		// we calculate how much inside we are
		float const invMinHalfSize = 1.0f / minHalfSize;
		float const inXpt = clamp((minHalfSize - abs(xOffsetOutsideInternalBox)) * invMinHalfSize, 0.0f, 1.0f);
		float const inYpt = clamp((minHalfSize - abs(yOffsetOutsideInternalBox)) * invMinHalfSize, 0.0f, 1.0f);
		float const inZpt = clamp((minHalfSize - abs(zOffsetOutsideInternalBox)) * invMinHalfSize, 0.0f, 1.0f);

		// now we calculate total how much inside (percentage) we are
		// this way, when we multiply, if we move along two components, it will be smaller
		float const inPt = inXpt * inYpt * inZpt;

		float const inDist = inPt * minHalfSize;

		Vector3 const offsetOutsideInternal = xOffsetOutsideInternalBox * xAxis + yOffsetOutsideInternalBox * yAxis + zOffsetOutsideInternalBox * zAxis;

		return GradientQueryResult(_square? -sqr(inDist) : inDist, offsetOutsideInternal.normal());
	}
}

ShapeAgainstPlanes::Type Collision::Box::check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const
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

ShapeAgainstPlanes::Type Collision::Box::check_against_plane(Plane const & _clipPlane, float _threshold) const
{
	// we just need to know if we're inside, outside or intersecting
	float maxDist = -1.0f;
	float minDist = 1.0f;
	Vector3 const * point = points;
	Vector3 const * endPoint = points + 8;
	while (point != endPoint)
	{
		float dist = _clipPlane.get_in_front(*point);
		maxDist = max(dist, maxDist);
		minDist = min(dist, minDist);
		++point;
	}
	if (minDist > _threshold)
	{
		return ShapeAgainstPlanes::Inside;
	}
	else if (maxDist < _threshold)
	{
		return ShapeAgainstPlanes::Outside;
	}
	else
	{
		return ShapeAgainstPlanes::Intersecting;
	}
}
