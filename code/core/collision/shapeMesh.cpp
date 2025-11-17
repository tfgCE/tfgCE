#include "shapeMesh.h"
#include "gradientQueryResult.h"
#include "queryPrimitivePoint.h"
#include "shapeConvexHull.h"

#include "..\containers\arrayStack.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Collision;

/*
float Collision::Mesh::get_distance(Vector3 const & _point, REF_ ClosestPointContext & _closestPointContext) const
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

GradientQueryResult Collision::Mesh::get_gradient(QueryPrimitivePoint const & _point, REF_ QueryContext & _queryContext, float _maxDistance, bool _square) const
{
	GradientQueryResult result(_maxDistance, Vector3::zero);
	for_every(triangle, triangles)
	{
		GradientQueryResult triResult = get_gradient_for_triangle(REF_ *triangle, _point.location, REF_ _queryContext, _maxDistance);
		if (result.distance > triResult.distance)
		{
			result.distance = _square ? sign(triResult.distance) * sqr(triResult.distance) : triResult.distance;
			result.normal = triResult.normal;
		}
	}
	return result;
}

ShapeAgainstPlanes::Type Collision::Mesh::check_against_planes(PlaneSet const & _clipPlanes, float _threshold) const
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

ShapeAgainstPlanes::Type Collision::Mesh::check_against_plane(Plane const & _clipPlane, float _threshold) const
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

GradientQueryResult Collision::Mesh::get_gradient_for_triangle(REF_ Triangle const & _triangle, Vector3 const & _point, REF_ QueryContext & _queryContext, float _maxDistance)
{
	float distance = 0.0f;
	Vector3 normal = Vector3::zero;

	float abcDist = _triangle.abc.get_in_front(_point);
	if (abs(abcDist) > _maxDistance)
	{
		distance = _maxDistance;
	}
	else
	{
		// check if we're inside triangle or beyond one of edges - if we're outside any edge, check distance to that edge
		if (_triangle.abIn.get_in_front(_point) >= 0.0f)
		{
			if (_triangle.bcIn.get_in_front(_point) >= 0.0f)
			{
				if (_triangle.caIn.get_in_front(_point) >= 0.0f)
				{
					// inside triangle
					// no negative distance - we treat meshes as just set of triangles without interior nor exterior
					distance = max(0.0f, abs(abcDist));
					normal = (abcDist >= 0.0f? 1.0f : -1.0f) * _triangle.abc.get_normal();
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
	}
	return GradientQueryResult(distance, normal);
}

void Collision::Mesh::break_into_convex_hulls(Array<Collision::ConvexHull> & _hulls, Array<Collision::Mesh> & _meshes) const
{
	ARRAY_PREALLOC_SIZE(Triangle, tris, triangles.get_size());
	ARRAY_PREALLOC_SIZE(Triangle, chTris, triangles.get_size());
	tris.copy_from(triangles);

	while (!tris.is_empty())
	{
		Collision::ConvexHull ch;
		ch.setup_as(*this);

		// find triangles that would make convex hull
		// they have to touch each other!
		chTris.clear();
		bool tryAgain = true;
		while (tryAgain)
		{
			tryAgain = false;
			Triangle* tri = tris.get_data();
			for (int i = 0; i < tris.get_size(); ++i, ++tri)
			{
				bool shareEdge = false;
				bool wouldBeNonConvex = false;
				bool wouldBeDegenerated = false;
				bool wouldHaveSameEdge = false;
				for_every(chTri, chTris)
				{
					float const tre = 0.0f;
					if (chTri->abc.get_in_front(tri->a) > tre ||
						chTri->abc.get_in_front(tri->b) > tre ||
						chTri->abc.get_in_front(tri->c) > tre)
					{
						wouldBeNonConvex = true;
						continue;
					}
					if (tri->abc.get_in_front(chTri->a) > tre ||
						tri->abc.get_in_front(chTri->b) > tre ||
						tri->abc.get_in_front(chTri->c) > tre)
					{
						wouldBeNonConvex = true;
						continue;
					}
					float const almostEqual = 0.00001f;
					if (Vector3::are_almost_equal(tri->abc.get_normal(), -chTri->abc.get_normal(), almostEqual) &&
						abs(tri->abc.calculate_d() - (-chTri->abc.calculate_d())) <= almostEqual)
					{
						wouldBeDegenerated = true;
						continue;
					}
					if (Vector3::are_almost_equal(tri->abc.get_normal(), chTri->abc.get_normal(), almostEqual) &&
						abs(tri->abc.calculate_d() - chTri->abc.calculate_d()) <= almostEqual)
					{
						if (chTri->does_share_edge_with(*tri) ||
							chTri->has_same_edge_with(*tri))
						{
							// same as already existing one
							wouldBeDegenerated = true;
							continue;
						}
					}
					if (chTri->has_same_edge_with(*tri))
					{
						wouldHaveSameEdge = true;
						continue;
					}
					if (chTri->does_share_edge_with(*tri))
					{
						shareEdge = true;
					}
				}

				if (chTris.is_empty() || (shareEdge && !wouldBeNonConvex && !wouldBeDegenerated && !wouldHaveSameEdge))
				{
					tryAgain = true;
					chTris.push_back(*tri);
					ch.add(tri->a);
					ch.add(tri->b);
					ch.add(tri->c);
					tris.remove_fast_at(i);
					--i;
					--tri;
				}
			}

		}

		if (ch.is_empty())
		{
			break;
		}
		else if (chTris.get_size() > 1)
		{
			// only if multiple triangles added
			ch.build();
			_hulls.push_back(ch);
		}
		else
		{
			// single triangle case
			Collision::Mesh cm;
			cm.setup_as(*this);
			auto const & chTri = chTris.get_first();
			cm.add(chTri.a, chTri.b, chTri.c);
			cm.build();
			_meshes.push_back(cm);
		}
	}
}

bool Collision::Mesh::break_into_convex_meshes(Array<Collision::Mesh> & _meshes) const
{
	Array<Triangle> tris;
	Array<Triangle> cmTris;
	tris.make_space_for(triangles.get_size());
	cmTris.make_space_for(triangles.get_size());
	tris.copy_from(triangles);

	int addedMeshesCount = 0;
	while (!tris.is_empty())
	{
		Collision::Mesh cm;
		cm.setup_as(*this);
		cm.make_space_for_triangles(tris.get_size());

		// find triangles that would make convex mesh
		// they have to touch each other!
		cmTris.clear();
		bool tryAgain = true;
		while (tryAgain)
		{
			tryAgain = false;
			Triangle* tri = tris.get_data();
			for (int i = 0; i < tris.get_size(); ++i, ++tri)
			{
				bool shareEdge = false;
				bool wouldBeNonConvex = false;
				for_every(cmTri, cmTris)
				{
					float const tre = 0.001f; // allow a little bit non convex
					if (cmTri->abc.get_in_front(tri->a) > tre ||
						cmTri->abc.get_in_front(tri->b) > tre ||
						cmTri->abc.get_in_front(tri->c) > tre)
					{
						wouldBeNonConvex = true;
						continue;
					}
					if (tri->abc.get_in_front(cmTri->a) > tre ||
						tri->abc.get_in_front(cmTri->b) > tre ||
						tri->abc.get_in_front(cmTri->c) > tre)
					{
						wouldBeNonConvex = true;
						continue;
					}

					if (cmTri->does_share_edge_with(*tri))
					{
						shareEdge = true;
					}
				}

				if (cmTris.is_empty() || (shareEdge && !wouldBeNonConvex))
				{
					tryAgain = true;
					cmTris.push_back(*tri);
					cm.add_just_triangle(tri->a, tri->b, tri->c);
					tris.remove_fast_at(i);
					--i;
					--tri;
				}
			}

		}

		if (cm.is_empty())
		{
			break;
		}
		else if (cm.triangles.get_size() == triangles.get_size())
		{
			an_assert(addedMeshesCount == 0)
			return false;
		}
		else
		{
			cm.build();
			_meshes.push_back(cm);
			++addedMeshesCount;
		}
	}

	return addedMeshesCount > 0;
}
