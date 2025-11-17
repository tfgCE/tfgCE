#include "mesh3dHelper_bezierSurface.h"

#include "..\..\containers\arrayStack.h"

#include "..\builders\mesh3dBuilder_IPU.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Meshes;
using namespace Helpers;

//

BezierSurfacePoint::BezierSurfacePoint()
{
}

BezierSurfacePoint::BezierSurfacePoint(BezierSurface const * _surface, Vector2 const & _coord)
: coord(_coord)
, point(_surface->calculate_at(_coord))
{
}

//

BezierSurfaceEdge::BezierSurfaceEdge()
{
	points.set_size(2);
}

BezierSurfaceEdge::BezierSurfaceEdge(BezierSurfaceHelper const * _helper, int _point0, int _point1, BezierSurfaceEdgeFlags::Type _flags)
{
	SET_EXTRA_DEBUG_INFO(points, TXT("BezierSurfaceEdge.points"));
	SET_EXTRA_DEBUG_INFO(belongsToTriangles, TXT("BezierSurfaceEdge.belongsToTriangles"));
	an_assert(_point0 != NONE);
	an_assert(_point1 != NONE);
	flags = _flags;
	points.set_size(2);
	points[0] = _point0;
	points[1] = _point1;
	segmentCoord.update(_helper->points[_point0].coord, _helper->points[_point1].coord);
	length = (_helper->points[_point0].point - _helper->points[_point1].point).length();
}

bool BezierSurfaceEdge::has_open_side() const
{
	if ((flags & BezierSurfaceEdgeFlags::Border) && belongsToTriangles.get_size() < 1)
	{
		return true;
	}
	if (!(flags & BezierSurfaceEdgeFlags::Border) && belongsToTriangles.get_size() < 2)
	{
		return true;
	}

	return false;
}

bool BezierSurfaceEdge::does_share_point_with(BezierSurfaceEdge const & _edge) const
{
	return _edge.points[0] == points[0] || _edge.points[1] == points[0] ||
		   _edge.points[0] == points[1] || _edge.points[1] == points[1];
}

//

BezierSurfaceTriangle::BezierSurfaceTriangle()
{
	SET_EXTRA_DEBUG_INFO(edges, TXT("BezierSurfaceTriangle.edges"));
	SET_EXTRA_DEBUG_INFO(points, TXT("BezierSurfaceTriangle.points"));
	SET_EXTRA_DEBUG_INFO(centreOnRightSideOfEdge, TXT("BezierSurfaceTriangle.centreOnRightSideOfEdge"));
	edges.set_size(3);
	points.set_size(3);
}

BezierSurfaceTriangle::BezierSurfaceTriangle(BezierSurfaceHelper const * _helper, int _edge0, int _edge1, int _edge2, float _u)
{
	u = _u;
	// store edges
	edges.push_back(_edge0);
	edges.push_back(_edge1);
	edges.push_back(_edge2);

	longestEdgeLength = 0.0f;
	// store points and update max length
	for_every(edge, edges)
	{
		longestEdgeLength = max(longestEdgeLength, _helper->edges[*edge].length);
		for_every(pointIdx, _helper->edges[*edge].points)
		{
			points.push_back_unique(*pointIdx);
		}
	}

	// order points clockwise
	{
		// if point 2 is on the right of segment going from 0 to 1, they are clockwise
		// otherwise swap points 2 and 1 (remember that U is forward, V is right!)
		Vector2 const offset1 = _helper->points[points[1]].coord - _helper->points[points[0]].coord;
		Vector2 const offset2 = _helper->points[points[2]].coord - _helper->points[points[0]].coord;
		Vector2 const right1 = offset1.rotated_right();
		if (Vector2::dot(right1, offset2) > 0.0f)
		{
			swap(points[1], points[2]);
		}
	}

	centreCoord = (_helper->points[points[0]].coord +
				   _helper->points[points[1]].coord +
				   _helper->points[points[2]].coord) / 3.0f;
	for_every(edge, edges)
	{
		centreOnRightSideOfEdge.push_back(_helper->edges[*edge].segmentCoord.is_on_right_side(centreCoord, 0.0f));
	}

}

bool BezierSurfaceTriangle::is_coord_inside(BezierSurfaceHelper const * _helper, Vector2 const & _coord, float _epsilon) const
{
	int onInsideCount = 0;
	bool const * corsoe = centreOnRightSideOfEdge.get_data();
	for_every(edge, edges)
	{
		bool onRightSide = _helper->edges[*edge].segmentCoord.is_on_right_side(_coord, 0.0f);
		if (! (onRightSide ^ *corsoe))
		{
			++onInsideCount;
		}
		++corsoe;
	}

	return onInsideCount == 3;
}

//

BezierSurfaceHelper::BezierSurfaceHelper(BezierSurface const * _surface)
: surface(_surface)
{
}

void BezierSurfaceHelper::add_inner_grid(int _innerPointsCountU, int _innerPointsCountV, float _u)
{
	if (_innerPointsCountU < 1)
	{
		return;
	}
	int topU = _innerPointsCountU + 1;
	float dividerU = 1.0f / (float)topU;
	float prevU = 0.0f;
	for (int ui = 0; ui < _innerPointsCountU; ++ui)
	{
		int limit = 0;
		int topV = topU;
		if (surface->get_type() == BezierSurfaceType::Curve)
		{
			limit = 1;
		}
		else if (surface->get_type() == BezierSurfaceType::Triangle)
		{
			limit = _innerPointsCountU - ui;
		}
		else if (surface->get_type() == BezierSurfaceType::Quad)
		{
			limit = _innerPointsCountV;
			topV = _innerPointsCountV + 1;
		}

		float dividerV = 1.0f / (float)topV;

		// +1 as we want to have INNER points
		float u = (float)(ui + 1) * dividerU;
		for (int vi = 0; vi < limit; ++vi)
		{
			float v = (float)(vi + 1) * dividerV;
			add_point(Vector2(u, v));
		}

		if (ui > 0)
		{
			if (surface->get_type() == BezierSurfaceType::Triangle)
			{
				for (int vi = 0; vi <= limit - 1; ++vi)
				{
					an_assert(dividerU == dividerV);
					float v = (float)(vi + 1) * dividerV;
					float nextV = (float)(vi + 2) * dividerV;
					/**
					 *		.
					 *
					 *		.b c.2	limit=3 (but this is NEXT v so we should stop earlier)
					 *
					 *		.a d.1	.			a,b,c,d for vi = 1
					 *
					 *		.	.0	.	.
					 *	 prev curr
					 */
					an_assert(prevU >= 0.0f && prevU <= 1.0f);
					an_assert(u >= 0.0f && u <= 1.0f);
					an_assert(v >= 0.0f && v <= 1.0f);
					an_assert(nextV >= 0.0f && nextV <= 1.0f);
					// triangle specific
					an_assert(v <= 1.0f - prevU + SMALL_MARGIN);
					an_assert(nextV <= 1.0f - prevU + SMALL_MARGIN);
					an_assert(v <= 1.0f - u + SMALL_MARGIN);
					int a = add_point(Vector2(prevU, v));
					int b = add_point(Vector2(prevU, nextV));
					int d = add_point(Vector2(u, v));
					if (a != NONE &&
						b != NONE &&
						d != NONE)
					{
						// we actually could touch edge, skip this then!
						if (vi < limit - 1)
						{
							an_assert(nextV <= 1.0f - u + SMALL_MARGIN);
							int c = add_point(Vector2(u, nextV));
							if (c != NONE)
							{
								// we actually could touch edge, skip that point then!
								add_triangle(add_edge(b, c, BezierSurfaceEdgeFlags::Undividable),
											 add_edge(c, d, BezierSurfaceEdgeFlags::Undividable),
											 add_edge(d, b, BezierSurfaceEdgeFlags::Undividable), _u);
							}
						}
						add_triangle(add_edge(a, b, BezierSurfaceEdgeFlags::Undividable),
									 add_edge(b, d, BezierSurfaceEdgeFlags::Undividable),
									 add_edge(d, a, BezierSurfaceEdgeFlags::Undividable), _u);

					}
				}
			}
			if (surface->get_type() == BezierSurfaceType::Quad)
			{
				for (int vi = 0; vi < limit - 1; ++vi)
				{
					float v = (float)(vi + 1) * dividerV;
					float nextV = (float)(vi + 2) * dividerV;
					/**
					 *				limit=4 (but this is NEXT v so we should stop earlier)
					 *		.	.3	.	.
					 *
					 *		.b c.2	.	.
					 *
					 *		.a d.1	.	.		a,b,c,d for vi = 1
					 *
					 *		.	.0	.	.
					 *	 prev curr
					 */
					an_assert(prevU >= 0.0f && prevU <= 1.0f);
					an_assert(u >= 0.0f && u <= 1.0f);
					an_assert(v >= 0.0f && v <= 1.0f);
					an_assert(nextV >= 0.0f && nextV <= 1.0f);
					int a = add_point(Vector2(prevU, v));
					int b = add_point(Vector2(prevU, nextV));
					int c = add_point(Vector2(u, nextV));
					int d = add_point(Vector2(u, v));
					an_assert(a != NONE);
					an_assert(b != NONE);
					an_assert(c != NONE);
					an_assert(d != NONE);
					add_triangle(add_edge(b, c, BezierSurfaceEdgeFlags::Undividable),
								 add_edge(c, d, BezierSurfaceEdgeFlags::Undividable),
								 add_edge(d, b, BezierSurfaceEdgeFlags::Undividable), _u);
					add_triangle(add_edge(a, b, BezierSurfaceEdgeFlags::Undividable),
								 add_edge(b, d, BezierSurfaceEdgeFlags::Undividable),
								 add_edge(d, a, BezierSurfaceEdgeFlags::Undividable), _u);
				}
			}
		}

		prevU = u;
	}
}

void BezierSurfaceHelper::add_edges(Vector2 const & _startingUV, Vector2 const & _endingUV, float _tInterval, BezierSurfaceEdgeFlags::Type _flags)
{
	Vector2 const startToEndUV = _endingUV - _startingUV;
	float t = 0.0f;
	int lastPoint = NONE;
	while (t <= 1.0f)
	{
		int newPoint = add_point(_startingUV + startToEndUV * t);
		if (newPoint != NONE)
		{
			add_edge(lastPoint, newPoint, _flags);
			lastPoint = newPoint;
		}
		if (t < 1.0f)
		{
			t = min(1.0f, t + _tInterval);
		}
		else
		{
			t += _tInterval;
		}
	}
}

void BezierSurfaceHelper::add_border_edge(BezierCurve<Vector3> const & _curve, int _segmentsPerEdgeCount, BezierSurfaceEdgeFlags::Type _flags)
{
	Vector2 startUV;
	Vector2 endUV;
	if (surface->get_uv_for_border(_curve, startUV, endUV))
	{
		add_edges(startUV, endUV, _segmentsPerEdgeCount, _flags);
	}
	else
	{
		an_assert(false, TXT("couldn't find uv for border curve"));
	}
}

void BezierSurfaceHelper::add_all_border_edges(int _segmentsPerEdgeCount, BezierSurfaceEdgeFlags::Type _flags)
{
	if (surface->get_type() == BezierSurfaceType::Triangle)
	{
		add_edges(Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), _segmentsPerEdgeCount, _flags | BezierSurfaceEdgeFlags::Border);
		add_edges(Vector2(0.0f, 0.0f), Vector2(0.0f, 1.0f), _segmentsPerEdgeCount, _flags | BezierSurfaceEdgeFlags::Border);
		add_edges(Vector2(0.0f, 1.0f), Vector2(1.0f, 0.0f), _segmentsPerEdgeCount, _flags | BezierSurfaceEdgeFlags::Border);
	}
	else if (surface->get_type() == BezierSurfaceType::Quad)
	{
		add_edges(Vector2(0.0f, 0.0f), Vector2(1.0f, 0.0f), _segmentsPerEdgeCount, _flags | BezierSurfaceEdgeFlags::Border);
		add_edges(Vector2(0.0f, 0.0f), Vector2(0.0f, 1.0f), _segmentsPerEdgeCount, _flags | BezierSurfaceEdgeFlags::Border);
		add_edges(Vector2(1.0f, 1.0f), Vector2(1.0f, 0.0f), _segmentsPerEdgeCount, _flags | BezierSurfaceEdgeFlags::Border);
		add_edges(Vector2(1.0f, 1.0f), Vector2(0.0f, 1.0f), _segmentsPerEdgeCount, _flags | BezierSurfaceEdgeFlags::Border);
	}
}

int BezierSurfaceHelper::add_edge(Vector2 const & _pointCoord0, Vector2 const & _pointCoord1, BezierSurfaceEdgeFlags::Type _flags)
{
	int pointsPrevSize = points.get_size();
	int point0 = add_point(_pointCoord0);
	int point1 = add_point(_pointCoord1);
	int edge = add_edge(point0, point1, _flags);
	if (edge == NONE)
	{
		points.set_size(pointsPrevSize);
	}
	return edge;
}

int BezierSurfaceHelper::add_triangle(Vector2 const & _pointCoord0, Vector2 const & _pointCoord1, Vector2 const & _pointCoord2, float _u)
{
	int pointsPrevSize = points.get_size();
	int edgesPrevSize = edges.get_size();
	int point0 = add_point(_pointCoord0);
	int point1 = add_point(_pointCoord1);
	int point2 = add_point(_pointCoord2);
	int edge01 = add_edge(point0, point1);
	int edge12 = add_edge(point1, point2);
	int edge20 = add_edge(point2, point0);
	int triangle = add_triangle(edge01, edge12, edge20, _u);
	if (triangle == NONE)
	{
		points.set_size(pointsPrevSize);
		edges.set_size(edgesPrevSize);
	}
	return triangle;
}

int BezierSurfaceHelper::add_point(Vector2 const & _pointCoord)
{
	for_every(point, points)
	{
		Vector2 diff = point->coord - _pointCoord;
		if (abs(diff.x) < SMALL_MARGIN &&
			abs(diff.y) < SMALL_MARGIN)
		{
			// such point already exist
			return for_everys_index(point);
		}
	}

	// check if won't end up in existing triangle
	for_every(triangle, triangles)
	{
		if (triangle->is_coord_inside(this, _pointCoord))
		{
			return NONE;
		}
	}

	// check if won't be too close to existing edge
	int const pointIdx = points.get_size(); // new index
	for_every(edge, edges)
	{
		if (edge->points[0] != pointIdx &&
			edge->points[1] != pointIdx &&
			edge->segmentCoord.calculate_distance_from(_pointCoord) < SMALL_MARGIN)
		{
			return NONE;
		}
	}
	
	mark_dirty();
	points.push_back(BezierSurfacePoint(surface, _pointCoord));
	return pointIdx;
}

int BezierSurfaceHelper::add_edge(int _point0, int _point1, BezierSurfaceEdgeFlags::Type _flags)
{
	if (_point0 == _point1 ||
		_point0 == NONE ||
		_point1 == NONE)
	{
		return NONE;
	}

	// check if there is such edge already
	for_every(edge, edges)
	{
		if ((edge->points[0] == _point0 && edge->points[1] == _point1) ||
			(edge->points[0] == _point1 && edge->points[1] == _point0))
		{
			return for_everys_index(edge);
		}
	}

	// check if new edge won't cut any existing into two
	BezierSurfaceEdge const newEdge(this, _point0, _point1, _flags);

	for_every(point, points)
	{
		int const pointIdx = for_everys_index(point);
		if (newEdge.points[0] != pointIdx &&
			newEdge.points[1] != pointIdx &&
			newEdge.segmentCoord.calculate_distance_from(point->coord) < MARGIN)
		{
			return NONE;
		}
	}

	for_every(edge, edges)
	{
		if (newEdge.segmentCoord.does_intersect_with(edge->segmentCoord))
		{
			return NONE;
		}
	}

	mark_dirty();
	edges.push_back(newEdge);
	return edges.get_size() - 1;
}

int BezierSurfaceHelper::add_triangle(int _edge0, int _edge1, int _edge2, float _u)
{
	if (_u < 0.0f)
	{
		_u = defaultU;
	}

	if (_edge0 == NONE ||
		_edge1 == NONE ||
		_edge2 == NONE)
	{
		return NONE;
	}

	for_every(triangle, triangles)
	{
		int exist = 0;
		exist += triangle->edges.does_contain(_edge0) ? 1 : 0;
		exist += triangle->edges.does_contain(_edge1) ? 1 : 0;
		exist += triangle->edges.does_contain(_edge2) ? 1 : 0;
		if (exist == 3)
		{
			return for_everys_index(triangle);
		}
	}

	// check if newly created triangle won't confine existing points!
	BezierSurfaceTriangle newTriangle(this, _edge0, _edge1, _edge2, _u);
	for_every(point, points)
	{
		if (!newTriangle.points.does_contain(for_everys_index(point)))
		{
			if (newTriangle.is_coord_inside(this, point->coord))
			{
				return NONE;
			}
		}
	}

	int newTriangleIdx = triangles.get_size();
	for_every(edge, newTriangle.edges)
	{
		edges[*edge].belongsToTriangles.push_back_unique(newTriangleIdx);
	}
	
	mark_dirty();
	triangles.push_back(newTriangle);
	return triangles.get_size() - 1;
}

void BezierSurfaceHelper::remove_point(int _point)
{
	ARRAY_STATIC(int, edgesToRemove, 2);
	for_every(edge, edges)
	{
		for_every(pointIdx, edge->points)
		{
			if (*pointIdx == _point)
			{
				edgesToRemove.push_back(for_everys_index(edge));
				break;
			}
			if (*pointIdx > _point)
			{
				*pointIdx = *pointIdx - 1;
			}
		}
	}
	for_every(triangle, triangles)
	{
		for_every(pointIdx, triangle->points)
		{
			if (*pointIdx > _point)
			{
				*pointIdx = *pointIdx - 1;
			}
		}
	}

	for_every_reverse(edgeToRemove, edgesToRemove)
	{
		remove_edge(*edgeToRemove);
	}

	mark_dirty();
	points.remove_at(_point);
}

void BezierSurfaceHelper::remove_edge(int _edge)
{
	ARRAY_STATIC(int, trianglesToRemove, 2);
	for_every(triangle, triangles)
	{
		for_every(edgeIdx, triangle->edges)
		{
			if (*edgeIdx == _edge)
			{
				trianglesToRemove.push_back(for_everys_index(triangle));
				break;
			}
			if (*edgeIdx > _edge)
			{
				*edgeIdx = *edgeIdx - 1;
			}
		}
	}

	for_every_reverse(triangleToRemove, trianglesToRemove)
	{
		remove_triangle(*triangleToRemove);
	}

	mark_dirty();
	edges.remove_at(_edge);
}

void BezierSurfaceHelper::remove_triangle(int _triangle)
{
	mark_dirty();
	triangles.remove_at(_triangle);
	for_every(edge, edges)
	{
		int toRemoveIdx = NONE;
		for_every(belongsToTriangle, edge->belongsToTriangles)
		{
			if (*belongsToTriangle == _triangle)
			{
				toRemoveIdx = for_everys_index(belongsToTriangle);
			}
			else if (*belongsToTriangle > _triangle)
			{
				*belongsToTriangle = *belongsToTriangle - 1;
			}
		}
		if (toRemoveIdx != NONE)
		{
			edge->belongsToTriangles.remove_fast_at(toRemoveIdx);
		}
	}
}

bool BezierSurfaceHelper::check_if_there_are_no_holes()
{
	for_every(edge, edges)
	{
		if (edge->has_open_side())
		{
			return false;
		}
	}

	// no holes!
	return true;
}

void BezierSurfaceHelper::process()
{
	fill();
	divide();
	
	processed = true;
}

void BezierSurfaceHelper::fill()
{
	// first fill all holes
	Array<int> edgeIndicesForbidden;
	edgeIndicesForbidden.make_space_for(edges.get_size());
	Array<BezierSurfacePoint*> pointsForbidden;
	pointsForbidden.make_space_for(points.get_size());
	while (true)
	{
		fill_obvious_holes();

		int bestEdgeIdx = NONE;
		float bestEdgeLength;
		for_every(edge, edges)
		{
			if (edgeIndicesForbidden.does_contain(for_everys_index(edge)))
			{
				// skip forbidden one
				continue;
			}

			if (edge->has_open_side())
			{
				float length = edge->length;
				if (bestEdgeIdx == NONE || length > bestEdgeLength)
				{
					bestEdgeIdx = for_everys_index(edge);
					bestEdgeLength = length;
				}
			}
		}

		if (bestEdgeIdx != NONE)
		{
			BezierSurfaceEdge * bestEdge = &edges[bestEdgeIdx];
			
			// for border we're only interesed int points laying on the same side as centre
			bool leftValid = true;
			bool rightValid = true;
			if (bestEdge->flags & BezierSurfaceEdgeFlags::Border)
			{
				if (bestEdge->segmentCoord.is_on_right_side(surface->get_centre_uv(), 0.0f))
				{
					leftValid = false;
				}
				else
				{
					rightValid = false;
				}
			}

			// check if has already something on either right or left side
			for_every(belongsToTriangle, bestEdge->belongsToTriangles)
			{
				BezierSurfaceTriangle const & triangle = triangles[*belongsToTriangle];
				if (bestEdge->segmentCoord.is_on_right_side(triangle.centreCoord, 0.0f))
				{
					rightValid = false;
				}
				else
				{
					leftValid = false;
				}
			}

			Vector2 segmentCentreCoord = bestEdge->segmentCoord.calculate_centre();

			// find best point, it should be as close as possible to the centre line
			pointsForbidden.clear();
			bool triangleAdded = false;
			while (! triangleAdded)
			{
				BezierSurfacePoint * bestPoint = nullptr;
				int bestPointIdx = NONE;
				float bestOff;
				for_every(point, points)
				{
					if (pointsForbidden.does_contain(point))
					{
						// skip this one, it wasn't valid
						continue;
					}
					bool isOnRightSide = bestEdge->segmentCoord.is_on_right_side(point->coord, 0.0f);
					if ((isOnRightSide && rightValid) ||
						(!isOnRightSide && leftValid))
					{
						Vector2 const offset = point->coord - segmentCentreCoord;
						float offCentre = abs(Vector2::dot(bestEdge->segmentCoord.get_start_to_end_dir(), offset));
						float offDist = abs(Vector2::dot(bestEdge->segmentCoord.get_right_dir(), offset));
						float off = offCentre * 5.0f + offDist; // more important for us it is to keep as close to the centre line as possible
						if (!bestPoint || off < bestOff)
						{
							bestPoint = point;
							bestOff = off;
							bestPointIdx = for_everys_index(point);
						}
					}
				}

				if (bestPoint)
				{
					// remember edges so we could revert without leaving them hanging
					int edgesSoFar = edges.get_size();
					static int tryCount = 0;
					++tryCount;
					int edgeAidx = add_edge(bestPointIdx, edges[bestEdgeIdx].points[0]);
					int edgeBidx = add_edge(bestPointIdx, edges[bestEdgeIdx].points[1]);

					// make sure they don't cut through any other edges (if they do, we get NONE index for them)
					if (edgeAidx != NONE &&
						edgeBidx != NONE)
					{
						int triangleIdx = add_triangle(bestEdgeIdx, edgeAidx, edgeBidx);

						if (triangleIdx != NONE)
						{
							triangleAdded = true;
						}
					}

					if (!triangleAdded)
					{
						edges.set_size(edgesSoFar);
						pointsForbidden.push_back_unique(bestPoint);
					}

					// update bestEdge as edges array could get reallocated
					bestEdge = &edges[bestEdgeIdx];
				}
				else
				{
					edgeIndicesForbidden.push_back_unique(bestEdgeIdx);
					break;
				}
			}
		}
		else
		{
			// all done, or couldn't deal with everything
			break;
		}
	}
}

void BezierSurfaceHelper::fill_obvious_holes()
{
	// fill holes that have edges and compose triangle
	for_every(edge, edges)
	{
		if (edge->has_open_side())
		{
			bool edgeDone = false;
			// look for pair of edges that share points with our edge and share same other point
			for_every(edgeA, edges)
			{
				if (edgeA != edge &&
					edgeA->has_open_side() &&
					edgeA->does_share_point_with(*edge))
				{
					for_every(edgeB, edges)
					{
						if (edgeB != edge &&
							edgeB != edgeA &&
							edgeB->has_open_side() &&
							edgeB->does_share_point_with(*edge) &&
							edgeB->does_share_point_with(*edgeA))
						{
							// check if all points in edges may create triangle
							// just check if when building array of all points from edges we will have just 3 unique points
							ARRAY_STATIC(int, usedPoints, 6);
							usedPoints.push_back_unique(edge->points[0]);
							usedPoints.push_back_unique(edge->points[1]);
							usedPoints.push_back_unique(edgeA->points[0]);
							usedPoints.push_back_unique(edgeA->points[1]);
							usedPoints.push_back_unique(edgeB->points[0]);
							usedPoints.push_back_unique(edgeB->points[1]);
							if (usedPoints.get_size() == 3)
							{
								// even if not possible, as it confines some point, just keep going
								add_triangle(for_everys_index(edge), for_everys_index(edgeA), for_everys_index(edgeB));
								edgeDone = true;
								break;
							}
						}
					}
				}
				if (edgeDone)
				{
					break;
				}
			}
		}
	}
}

void BezierSurfaceHelper::divide()
{
	if (maxEdgeLength <= 0.0f)
	{
		return;
	}

	bool allFine = false;
	while (! allFine)
	{
		allFine = true;

		int longestEdgeIdx = NONE;
		float longestEdgeLength;
		for_every(edge, edges)
		{
			if (!(edge->flags & BezierSurfaceEdgeFlags::Undividable))
			{
				float const edgeLength = edge->length;
				if (edgeLength > maxEdgeLength &&
					(longestEdgeIdx == NONE || edgeLength > longestEdgeLength))
				{
					// check this edge isn't too short
					// it should be longer than [some part] of longest edge in triangles it belongs to
					float longestEdgeInAdjencentTriangles = edgeLength;
					for_every(belongsToTriangle, edge->belongsToTriangles)
					{
						longestEdgeInAdjencentTriangles = max(longestEdgeInAdjencentTriangles, triangles[*belongsToTriangle].longestEdgeLength);
					}
					if (edgeLength >= 0.667f * longestEdgeInAdjencentTriangles)
					{
						// check if new point won't be part of any existing edge (other than one we're checking), if so, skip this one
						bool validForAddition = true;
						Vector2 newPointCoord = edge->segmentCoord.calculate_centre();
						for_every(checkEdge, edges)
						{
							if (checkEdge != edge &&
								checkEdge->segmentCoord.calculate_distance_from(newPointCoord) < MARGIN)
							{
								validForAddition = false;
							}
						}
						if (validForAddition)
						{
							longestEdgeIdx = for_everys_index(edge);
							longestEdgeLength = edgeLength;
						}
					}
				}
			}
		}

		if (longestEdgeIdx != NONE)
		{
			allFine = false;
			BezierSurfaceEdge const * longestEdge = &edges[longestEdgeIdx];
			BezierSurfaceEdgeFlags::Type flags = longestEdge->flags;
			// store points that belong to this edge
			int pointA = longestEdge->points[0];
			int pointB = longestEdge->points[1];
			// store info about triangles to remove
			int triangleIdx[2];
			triangleIdx[0] = longestEdge->belongsToTriangles.get_size() >= 1 ? longestEdge->belongsToTriangles[0] : NONE;
			triangleIdx[1] = longestEdge->belongsToTriangles.get_size() >= 2 ? longestEdge->belongsToTriangles[1] : NONE;
			float triangleU[2];
			// store points for those triangles that don't belong to this edge
			int triangleOppositePoint[2];
			triangleOppositePoint[0] = NONE;
			triangleOppositePoint[1] = NONE;
			for (int i = 0; i < 2; ++i)
			{
				int triIdx = triangleIdx[i];
				int & triOppositePoint = triangleOppositePoint[i];
				triangleU[i] = defaultU;
				if (triIdx != NONE)
				{
					BezierSurfaceTriangle const * triangle = &triangles[triIdx];
					triangleU[i] = triangle->u;
					for_every(point, triangle->points)
					{
						if (*point != pointA && *point != pointB)
						{
							triOppositePoint = *point;
							break;
						}
					}
				}
			}
			// now we can remove edge (it will remove triangles too!)
			remove_edge(longestEdgeIdx);
			// add point in the middle between A and B
			int pointC = add_point((points[pointA].coord + points[pointB].coord) * 0.5f);
			// add edges AB, BC
			int edgeAC = add_edge(pointA, pointC, flags);
			int edgeBC = add_edge(pointB, pointC, flags);
			for (int i = 0; i < 2; ++i)
			{
				if (triangleIdx[i] != NONE)
				{
					// try to add all edges although it is more to find proper ones and new one will be only CO
					int edgeAO = add_edge(pointA, triangleOppositePoint[i]);
					int edgeBO = add_edge(pointB, triangleOppositePoint[i]);
					int edgeCO = add_edge(pointC, triangleOppositePoint[i]);
					// now divide that triangle into two sub triangles
					add_triangle(edgeAC, edgeAO, edgeCO, triangleU[i]);
					add_triangle(edgeBC, edgeBO, edgeCO, triangleU[i]);
				}
			}
		}
	}
}

void BezierSurfaceHelper::into_mesh(::Meshes::Builders::IPU * _builder)
{
	if (!processed)
	{
		process();
	}

	// add points to builder and store "in-mesh" point indices
	Array<int> pointInMeshIdx;
	pointInMeshIdx.make_space_for(points.get_size());
	for_every(point, points)
	{
		pointInMeshIdx.push_back(_builder->add_point(point->point));
	}

	for_every(triangle, triangles)
	{
		_builder->add_triangle(triangle->u,
			pointInMeshIdx[triangle->points[0]],
			pointInMeshIdx[triangle->points[1]],
			pointInMeshIdx[triangle->points[2]]);
	}
}

//

void BezierSurfaceRibbonHelper::turn_into_mesh(BezierCurve<Vector3> const& _curveA, BezierCurve<Vector3> const& _curveB, int _segmentsPerEdgeCount, ::Meshes::Builders::IPU* _builder)
{
	float tInverval = 1.0f / (float)max(1, _segmentsPerEdgeCount);

	Array<Vector3> pA;
	Array<Vector3> pB;

	int predictedPointsCount = _segmentsPerEdgeCount + 1;
	_builder->will_need_more_points(predictedPointsCount * 2);
	_builder->will_need_more_polygons(_segmentsPerEdgeCount);

	pA.make_space_for(predictedPointsCount);
	pB.make_space_for(predictedPointsCount);
	float t = 0.0f;
	while (t <= 1.0f)
	{
		pA.push_back(_curveA.calculate_at(t));
		pB.push_back(_curveB.calculate_at(t));
		if (t < 1.0f)
		{
			t = min(1.0f, t + tInverval);
		}
		else
		{
			t += tInverval;
		}
	}

	//

	int aIPU = _builder->get_point_count();
	for_every(p, pA)
	{
		_builder->add_point(*p);
	}
	int bIPU = _builder->get_point_count();
	for_every(p, pB)
	{
		_builder->add_point(*p);
	}

	//

	int aIdx = aIPU;
	int bIdx = bIPU;
	for_count(int, i, pA.get_size() - 1)
	{
		_builder->add_quad(defaultU, aIdx, bIdx, bIdx + 1, aIdx + 1);
		++aIdx;
		++bIdx;
	}
}

