#include "shapeConvexHull.h"
#include "gradientQueryResult.h"
#include "queryPrimitivePoint.h"

using namespace Collision;

/*
float Collision::ConvexHull::get_distance(Vector3 const & _point, REF_ ClosestPointContext & _closestPointContext) const
{
	if (_closestPointContext.triangle)
	{
		return _closestPointContext.triangle->get_distance(_point, REF_ _closestPointContext);
	}
	else
	{
		Vector3 const relPoint = _point - centre;
		return find_triangle(relPoint)->get_distance(_point, _closestPointContext);
	}
}
*/

GradientQueryResult Collision::ConvexHull::get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square) const
{
	_queryContext.triangle = find_triangle(_point.location, _queryContext.triangle);
	GradientQueryResult result = get_gradient_for_triangle(REF_ *_queryContext.triangle, _point.location, REF_ _queryContext);
	return GradientQueryResult(_square ? sign(result.distance) * sqr(result.distance) : result.distance, result.normal);
}

ShapeAgainstPlanes::Type Collision::ConvexHull::check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const
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

ShapeAgainstPlanes::Type Collision::ConvexHull::check_against_plane(Plane const & _clipPlane, float _threshold) const
{
	// we just need to know if we're inside, outside or intersecting
	float maxDist = -1.0f;
	float minDist = 1.0f;
	for_every(point, points)
	{
		float dist = _clipPlane.get_in_front(*point);
		maxDist = max(dist, maxDist);
		minDist = min(dist, minDist);
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

static void get_normal_and_distance_to_segment(Vector3 const & _lineStart, Vector3 const & _lineEnd, Vector3 const & _lineDir, float const _lineLength, Vector3 const & _point, OUT_ Vector3 & _normal, OUT_ float & _distance)
{
	float const onLineDistance = Vector3::dot(_point - _lineStart, _lineDir);
	Vector3 const onLinePoint = _lineStart + onLineDistance * _lineDir;
	if (onLineDistance >= 0.0f)
	{
		if (onLineDistance <= _lineLength)
		{
			(_point - onLinePoint).normal_and_length(_normal, _distance);
		}
		else
		{
			(_point - _lineEnd).normal_and_length(_normal, _distance);
		}
	}
	else
	{
		(_point - _lineStart).normal_and_length(_normal, _distance);
	}
}

GradientQueryResult Collision::ConvexHull::get_gradient_for_triangle(REF_ Triangle const & _triangle, Vector3 const & _point, REF_ QueryContext & _queryContext)
{
	todo_note(TXT("queryContext triangle - if we're inside this triangle and store triangle in queryContext"));

	float distance = 0.0f;
	Vector3 normal = Vector3::zero;

	// check if we're inside triangle or beyond one of edges - if we're outside any edge, check distance to that edge
	if (_triangle.abIn.get_in_front(_point) >= 0.0f)
	{
		if (_triangle.bcIn.get_in_front(_point) >= 0.0f)
		{
			if (_triangle.caIn.get_in_front(_point) >= 0.0f)
			{
				// inside triangle
				distance = (_triangle.abc.get_in_front(_point)); // allow negative distance
				normal = _triangle.abc.get_normal();
			}
			else
			{
				get_normal_and_distance_to_segment(_triangle.c, _triangle.a, _triangle.c2a, _triangle.c2aDist, _point, normal, distance);
			}
		}
		else
		{
			get_normal_and_distance_to_segment(_triangle.b, _triangle.c, _triangle.b2c, _triangle.b2cDist, _point, normal, distance);
		}
	}
	else
	{
		get_normal_and_distance_to_segment(_triangle.a, _triangle.b, _triangle.a2b, _triangle.a2bDist, _point, normal, distance);
	}
	return GradientQueryResult(distance, normal);
}

//

Collision::ConvexHull::QueryContext::QueryContext()
: triangle(nullptr)
{
}
