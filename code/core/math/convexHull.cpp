#include "math.h"
#include "..\io\xml.h"

#include "..\collision\checkCollisionContext.h"
#include "..\containers\arrayStack.h"
#include "..\debug\debugRenderer.h"
#include "..\debug\debugVisualiser.h"

//
 
#define USE_QUICK_HULL
//#define USE_CONV_HULL

#ifdef AN_DEVELOPMENT
//#define DETAILED_CONVEX_HULL_BUILD
#endif

//

#ifdef USE_QUICK_HULL
	#include "quickHull\quickHullImpl.h"
#endif
#ifdef USE_CONV_HULL
	#include "convhull\convhullImpl.h"
#endif

//

// when convex hull breaks, use this to find a culprit

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define AN_INVESTIGATE_IN_FRONT_THRESHOLD
#define AN_INVESTIGATE_MULTI_SHARE_FOR_EDGES
#endif

/*
	cases that require checking if nothing got broken

	ConvexHull ch;
	ch.add(Vector3(17.730000f, 15.611050f, 1.000000f));
	ch.add(Vector3(15.611050f, -17.730000f, 1.000000f));
	ch.add(Vector3(-15.611050f, 17.730000f, 1.000000f));
	ch.add(Vector3(-17.730000f, -15.611050f, 1.000000f));
	ch.add(Vector3(17.730000f, 15.611050f, -18.185059f));
	ch.add(Vector3(15.611050f, -17.730000f, -18.185059f));
	ch.add(Vector3(-15.611050f, 17.730000f, -18.185059f));
	ch.add(Vector3(-17.730000f, -15.611050f, -18.185059f));
	ch.add(Vector3(4.241022f, 6.729456f, -5.135925f));
	ch.add(Vector3(4.241022f, 6.729456f, -9.135925f));
	ch.add(Vector3(-0.504835f, -7.943418f, 3.000000f));
	ch.add(Vector3(-0.504835f, -7.943418f, -1.000000f));
	ch.add(Vector3(-7.331470f, -3.085679f, 0.704895f));
	ch.add(Vector3(-7.331470f, -3.085679f, -3.295105f));
	ch.add(Vector3(7.937399f, 0.724285f, -12.904237f));
	ch.add(Vector3(2.810467f, -7.441312f, -16.981201f));
	ch.add(Vector3(-3.347987f, -7.225696f, -20.185059f));
	ch.build();

 */

//

ConvexHull::ConvexHull()
: bisect(nullptr)
{
}

ConvexHull::ConvexHull(ConvexHull const & _other)
: bisect(nullptr)
{
	copy_from(_other);
}

ConvexHull::~ConvexHull()
{
	delete_and_clear(bisect);
}

void ConvexHull::clear_keep_points()
{
	delete_and_clear(bisect);
	triangles.clear();
	edges.clear();
}

void ConvexHull::clear()
{
	clear_keep_points();
	points.clear();
	range = Range3::empty;
}


ConvexHull const & ConvexHull::operator = (ConvexHull const & _other)
{
	copy_from(_other);
	return *this;
}

void ConvexHull::copy_from(ConvexHull const & _other)
{
	delete_and_clear(bisect);
	bisect = nullptr;
	points = _other.points;
	range = _other.range;
	centre = _other.centre;
	triangles.clear();
	edges.clear();
	if (points.get_size() >= 4)
	{
		build();
	}
}

void ConvexHull::add(Vector3 const & _point)
{
	Vector3 point = _point;
	// this messes up lots of things
	// 2023.07.04 wish I wrote what kind of things did it mess up. game collisions?
	//point.x = round_to(point.x, 0.00001f);
	//point.y = round_to(point.y, 0.00001f);
	//point.z = round_to(point.z, 0.00001f);
	an_assert(triangles.get_size() == 0 && bisect == nullptr, TXT("convex hull should be not build when adding new point"));
	if (!points.does_contain(point))
	{
		points.push_back(point);
		range.include(point);
	}
}

void ConvexHull::build()
{
	an_assert(triangles.get_size() == 0 && bisect == nullptr, TXT("convex hull should be not build when building"));
	an_assert(points.get_size() >= 4, TXT("there should be at least four points in convex hull"));

#ifdef AN_DEVELOPMENT
	triangles.clear();
#endif

#ifdef USE_QUICK_HULL
	if (true)
	{
#ifdef AN_DEVELOPMENT
		orgPoints = points;
#endif
		// use quick hull implementation
		// but do a multiple passes as sometimes there are extra points that are removed in the second pass (just in any case do more)
		// if we end up with the same amount of points, keep it
		int bigTriesLeft = 40;
		int triesLeft = 20;
		bool requiresAnotherPass = false;
		Array<Vector3> pointsToRemove;
#ifdef DETAILED_CONVEX_HULL_BUILD
		int passIdx = 0;
#endif
		while (triesLeft > 0 || (requiresAnotherPass && bigTriesLeft > 0))
		{
#ifdef DETAILED_CONVEX_HULL_BUILD
			output(TXT(" + pass %i (%i)"), passIdx, points.get_size());
			++ passIdx;
#endif
			--triesLeft;
			--bigTriesLeft;
			int pointsCount = points.get_size();
			ARRAY_PREALLOC_SIZE(float, pointCloud, points.get_size() * 3);
			for_every(p, points)
			{
				pointCloud.push_back(p->x);
				pointCloud.push_back(p->y);
				pointCloud.push_back(p->z);
			}

			points.clear();
			triangles.clear();
			pointsToRemove.clear();

			QuickHullImpl::perform(pointCloud.get_data(), pointCloud.get_size() / 3,
				[this](float _x, float _y, float _z)
				{
					points.push_back(Vector3(_x, _y, _z));
				},
				[this, &requiresAnotherPass, &pointsToRemove](int _a, int _b, int _c)
				{
					Vector3 p0 = points[_a];
					Vector3 p1 = points[_b];
					Vector3 p2 = points[_c];
					if (p0 == p1 || p1 == p2 || p0 == p2)
					{
#ifdef DETAILED_CONVEX_HULL_BUILD
						warn(TXT("degenerate triangle provided by convex hull algorithm (points double)"));
#endif
						if (p0 == p1 || p1 == p2)
						{
							pointsToRemove.push_back(p1);
#ifdef DETAILED_CONVEX_HULL_BUILD
							warn(TXT("removed point"));
#endif
						}
						if (p0 == p2)
						{
							pointsToRemove.push_back(p2);
#ifdef DETAILED_CONVEX_HULL_BUILD
							warn(TXT("removed point"));
#endif
						}
						requiresAnotherPass = true;
						return;
					}
					if (Triangle::is_degenerate(p0, p1, p2))
					{
#ifdef DETAILED_CONVEX_HULL_BUILD
						warn(TXT("degenerate triangle provided by convex hull algorithm (triangle degenerate)"));
#endif
						if (Vector3::dot(p1 - p0, p2 - p0) < 0.0f)
						{
							pointsToRemove.push_back(p0);
#ifdef DETAILED_CONVEX_HULL_BUILD
							warn(TXT("removed point"));
#endif
						}
						else if (Vector3::dot(p0 - p1, p2 - p1) < 0.0f)
						{
							pointsToRemove.push_back(p1);
#ifdef DETAILED_CONVEX_HULL_BUILD
							warn(TXT("removed point"));
#endif
						}
						else if (Vector3::dot(p0 - p2, p1 - p2) < 0.0f)
						{
							pointsToRemove.push_back(p2);
#ifdef DETAILED_CONVEX_HULL_BUILD
							warn(TXT("removed point"));
#endif
						}

						requiresAnotherPass = true;
						return;
					}
					triangles.push_back(Triangle(p2, p1, p0, true));
				});
			if (!pointsToRemove.is_empty())
			{
				for_every(p, pointsToRemove)
				{
					points.remove_fast(*p);
				}
			}
			else if (pointsCount == points.get_size() && !requiresAnotherPass)
			{
				break;
			}
		}
		// calculate centre (mass centre)
		centre = Vector3::zero;
		for_every(point, points)
		{
			centre += *point;
		}
		centre /= float(points.get_size());

		for_every(t, triangles)
		{
			t->prepare_using_centre(centre);
		}
	}
	else
#endif
#ifdef USE_CONV_HULL
	if (true)
	{
#ifdef AN_DEVELOPMENT
		orgPoints = points;
#endif
		ARRAY_PREALLOC_SIZE(ch_vec3, chInPoints, points.get_size());
		ARRAY_PREALLOC_SIZE(int, chOutFaces, points.get_size() * 3);
		ARRAY_PREALLOC_SIZE(Vector3 pointsUsed, points.get_size());

		int bigTriesLeft = 50;
		int triesLeft = 5;
		bool requiresAnotherPass = false;
		while (triesLeft > 0 || (requiresAnotherPass && bigTriesLeft > 0))
		{
			--triesLeft;
			--bigTriesLeft;

			chInPoints.clear();
			for_every(p, points)
			{
				ch_vec3 v;
				v.x = p->x;
				v.y = p->y;
				v.z = p->z;
				chInPoints.push_back(v);
			}

			int chOutFacesCount;

			chOutFaces.set_size(points.get_size() * 3);

			convhullImpl::build(chInPoints.get_data(), chInPoints.get_size(), chOutFaces.begin(), &chOutFacesCount);

			int pointsCount = points.get_size();

			pointsUsed.clear();
			int const* outFace = chOutFaces.begin();
			for_count(int, fIdx, chOutFacesCount)
			{
				Vector3 p0 = points[*outFace];
				++outFace;
				Vector3 p1 = points[*outFace];
				++outFace;
				Vector3 p2 = points[*outFace];
				++outFace;

				if (p0 == p1 || p1 == p2 || p0 == p2)
				{
#ifdef AN_DEVELOPMENT
					warn(TXT("degenerate triangle provided by convex hull algorithm"));
#endif
					requiresAnotherPass = true;
					continue;
				}
				if (Triangle::is_degenerate(p0, p1, p2))
				{
#ifdef AN_DEVELOPMENT
					warn(TXT("degenerate triangle provided by convex hull algorithm"));
#endif
					requiresAnotherPass = true;
					continue;
				}

				pointsUsed.push_back_unique(p0);
				pointsUsed.push_back_unique(p1);
				pointsUsed.push_back_unique(p2);
				triangles.push_back(Triangle(p2, p1, p0, true));
			}

			points = pointsUsed;
			if (pointsCount == points.get_size() && !requiresAnotherPass)
			{
				break;
			}
		}

		// calculate centre (mass centre)
		centre = Vector3::zero;
		for_every(point, points)
		{
			centre += *point;
		}
		centre /= float(points.get_size());

		for_every(t, triangles)
		{
			t->prepare_using_centre(centre);
		}
	}
	else
#endif
	{
		// remove co-linear points
		for (int a = 0; a < points.get_size(); ++ a)
		{
			Vector3 pa = points[a];
			for (int b = a + 1; b < points.get_size(); ++b)
			{
				bool breakB = false;
				Vector3 pb = points[b];
				if (pb == pa)
				{
					points.remove_fast_at(b);
					--b;
					continue;
				}
				for (int c = b + 1; c < points.get_size(); ++c)
				{
					bool breakC = false;
					Vector3 pc = points[c];
					if (pc == pb)
					{
						points.remove_fast_at(c);
						--c;
						continue;
					}
					float colinearDot = Vector3::dot((pb - pa).normal(), (pc - pa).normal());
					if (abs(colinearDot) > 0.999f)
					{
						float c_ab = Vector3::dot(pc - pa, (pb - pa).normal());
						float t = c_ab / (pb - pa).length();
						if (t < 0.0f) // c-a-b
						{
							points.remove_at(a);
							--a;
							breakB = true;
							breakC = true;
						}
						else if (t > 1.0f) // a-b-c
						{
							points.remove_at(b);
							--b;
							breakC = true;
						}
						else
						{
							points.remove_at(c);
							--c;
						}
					}
					if (breakC) break;
				}
				if (breakB) break;
			}
		}

		// calculate centre (mass centre)
		centre = Vector3::zero;
		for_every(point, points)
		{
			centre += *point;
		}
		centre /= float(points.get_size());

		todo_hack(TXT("this is to help with building convex hulls, it's enough to make it work"));
		{
			float distFurthestDistSq = 0.0f;
			for_every(point, points)
			{
				{
					float distSq = (*point - centre).length_squared();
					distFurthestDistSq = max(distFurthestDistSq, distSq);
				}
				{
					float distSq = point->length_squared();
					distFurthestDistSq = max(distFurthestDistSq, distSq);
				}
			}
			float const roundTo = sqrt(distFurthestDistSq) * 0.000001f;
			for_every(point, points)
			{
				point->x = round_to(point->x, roundTo);
				point->y = round_to(point->y, roundTo);
				point->z = round_to(point->z, roundTo);
			}
		}

		// try furthest point first
		{
			int furthestPoint = NONE;
			float distFurthestDistSq = 0.0f;
			for_every(point, points)
			{
				float distSq = (*point - centre).length_squared();
				if (distFurthestDistSq < distSq ||
					furthestPoint == NONE)
				{
					distFurthestDistSq = distSq;
					furthestPoint = for_everys_index(point);
				}
			}
			add_triangle(furthestPoint);
		}
		if (triangles.is_empty())
		{
			// build triangles first - start with first point, if that fails (ie. it is between other points), try next one
			// if we managed to start with a one, it should cover all
			for_count(int, i, points.get_size())
			{
				add_triangle(i);
				if (! triangles.is_empty())
				{
					break;
				}
			}
		}
	}

	// link all triangles
	link_triangles();

	// build bisect basing on triangles just added
	build_bisect();
}

void ConvexHull::add_triangle(int32 _a)
{
	Vector3* pa = &points[_a];
	for_every(pb, points)
	{
		if (pb == pa)
		{
			continue;
		}
		if (add_triangle(_a, (int)(pb - points.begin())))
		{
			break;
		}
	}
}

bool ConvexHull::add_triangle_in_gap(int32 _a, int32 _b)
{
	// this is sort of hack
	// sometimes we miss just a single triangle. this is to find that gap and fill it
	Vector3* pa = &points[_a];
	Vector3* pb = &points[_b];
	for_every(triangle, triangles)
	{
		if ((triangle->a == *pa && triangle->b == *pb) ||
			(triangle->b == *pa && triangle->c == *pb) ||
			(triangle->c == *pa && triangle->a == *pb))
		{
			// already there
			return true;
		}
	}

	// find one point that will have just one edge to _a and _b
	for_every(pc, points)
	{
		if (pc == pb ||
			pc == pa)
		{
			continue;
		}
		int c2a = 0;
		int c2b = 0;
		for_every(triangle, triangles)
		{
			if ((triangle->a == *pa && triangle->b == *pc) ||
				(triangle->b == *pa && triangle->c == *pc) ||
				(triangle->c == *pa && triangle->a == *pc) ||
				(triangle->b == *pa && triangle->a == *pc) ||
				(triangle->c == *pa && triangle->b == *pc) ||
				(triangle->a == *pa && triangle->c == *pc))
			{
				++c2a;
			}
			if ((triangle->a == *pb && triangle->b == *pc) ||
				(triangle->b == *pb && triangle->c == *pc) ||
				(triangle->c == *pb && triangle->a == *pc) ||
				(triangle->b == *pb && triangle->a == *pc) ||
				(triangle->c == *pb && triangle->b == *pc) ||
				(triangle->a == *pb && triangle->c == *pc))
			{
				++c2b;
			}
		}

		if (c2a == 1 && c2b == 1)
		{
			// that one triangle missing
			triangles.push_back(Triangle(*pa, *pb, *pc));
			return true;
		}
	}
	return false;
}

bool ConvexHull::add_triangle(int32 _a, int32 _b)
{
	Vector3* pa = &points[_a];
	Vector3* pb = &points[_b];
	Vector3* pc = nullptr;
	Vector3 a = *pa;
	Vector3 b = *pb;
	Vector3 c = b;
	bool valid = false;
	// look for valid third point
	for_every(_pc, points)
	{
		valid = false;
		pc = _pc;
		if (pc == pb ||
			pc == pa)
		{
			continue;
		}
		c = *pc;
		an_assert(c != a);
		an_assert(c != b);
		// this triangle has to face outside
		Plane abc = Plane(a, b, c);
		float abcDist = (b - a).length() + (c - b).length() + (a - c).length();
		float inFrontThreshold = max((abcDist / 30.0f) * 0.000001f, 0.000001f); // adaptive approach to threshold. this may happen due to float inaccuracy
		if (abc.get_in_front(centre) > 0.0f)
		{
			// we'll get to a, c, b
			continue;
		}
		// all other points have to be on back of this triangle
		valid = true;
		for_every(pd, points)
		{
			if (pd == pa ||
				pd == pb ||
				pd == pc)
			{
				continue;
			}
			if (abc.get_in_front(*pd) > inFrontThreshold) // 0.0f may disallow coplanar triangles, very low values too
			{
#ifdef AN_DEVELOPMENT
#ifdef AN_INVESTIGATE_IN_FRONT_THRESHOLD
				float inFront = abc.get_in_front(*pd);
				if (inFront < inFrontThreshold * 10.0f)
				{
					output(TXT("investigate?"));
					output(TXT("abc: n:%.9f, %.9f, %.9f d:%.9f"), abc.get_normal().x, abc.get_normal().y, abc.get_normal().z, abc.d);
					output(TXT("abcDist: %.9f"), abcDist);
					output(TXT("inFront: %.9f"), inFront);
					output(TXT("inFrontThreshold: %.9f"), inFrontThreshold);
					output(TXT("inFront > inFrontThreshold: %S"), inFront > inFrontThreshold? TXT("yes - invalid") : TXT("no - ok"));
					debug_visualise(a, b, c, *pd);
				}
#endif
#endif
				// invalid
				valid = false;
				break;
			}
		}
		// it can't be doubled
		if (valid)
		{
			valid = false;
			if (intersects_coplanar_triangle(a, b, c))
			{
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
				int _c = (int)(pc - points.begin());
#endif
#endif
				continue;
			}
			// it is valid?
			valid = true;
			// check if we won't double an edge
			for_every(triangle, triangles)
			{
				if (triangle->has_edge(a, b) ||
					triangle->has_edge(b, c) ||
					triangle->has_edge(c, a))
				{
					// there is already a triangle that has this edge, look for another
					valid = false;
					break;
				}
			}
			if (!valid)
			{
				continue;
			}
			if (has_triangle(a, b, c))
			{
				continue;
			}
			if (valid)
			{
				break;
			}
		}
	}
	if (!valid)
	{
		// couldn't find triangle, break
		return false;
	}
	// add this triangle to list
	triangles.push_back(Triangle(a, b, c));
	int32 _c = (int32)(pc - points.begin());

	// add triangle on each edge (doubled will bail out)
	bool addedAC = add_triangle(_a, _c);
	bool addedBA = add_triangle(_b, _a);
	bool addedCB = add_triangle(_c, _b);
	for_every(triangle, triangles)
	{
		if (!addedAC && triangle->has_edge(a, c))
		{
			addedAC = true;
		}
		if (!addedBA && triangle->has_edge(b, a))
		{
			addedBA = true;
		}
		if (!addedCB && triangle->has_edge(c, b))
		{
			addedCB = true;
		}
	}
	if (!addedAC)
	{
		addedAC = add_triangle_in_gap(_a, _c);
	}
	if (!addedBA)
	{
		addedAC = add_triangle_in_gap(_b, _a);
	}
	if (!addedCB)
	{
		addedAC = add_triangle_in_gap(_c, _b);
	}
#ifdef AN_DEVELOPMENT
	if (!addedAC || !addedBA || !addedCB)
	{
		debug_visualise(a, b, c);
	}
	an_assert(addedAC);
	an_assert(addedBA);
	an_assert(addedCB);
#endif
	return true;
}

void ConvexHull::link_triangles()
{
	for_every(triangle, triangles)
	{
#ifdef AN_DEVELOPMENT
		triangle->abTriangle = nullptr;
		triangle->bcTriangle = nullptr;
		triangle->caTriangle = nullptr;
#endif
		for_every(other, triangles)
		{
			if (other == triangle)
			{
				continue;
			}

			if (other->has_edge(triangle->b, triangle->a))
			{
				triangle->abTriangle = other;
			}
			if (other->has_edge(triangle->c, triangle->b))
			{
				triangle->bcTriangle = other;
			}
			if (other->has_edge(triangle->a, triangle->c))
			{
				triangle->caTriangle = other;
			}
		}
#ifdef AN_DEVELOPMENT
		if (!(triangle->abTriangle != nullptr) ||
			!(triangle->bcTriangle != nullptr) ||
			!(triangle->caTriangle != nullptr))
		{
			debug_visualise(triangle->a, triangle->b, triangle->c);
		}
		an_assert(triangle->abTriangle != nullptr, TXT("triangle %i"), triangle - triangles.begin());
		an_assert(triangle->bcTriangle != nullptr, TXT("triangle %i"), triangle - triangles.begin());
		an_assert(triangle->caTriangle != nullptr, TXT("triangle %i"), triangle - triangles.begin());
#endif
	}
}

bool ConvexHull::has_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c) const
{
	for_every(triangle, triangles)
	{
		if (_a == triangle->a &&
			_b == triangle->b &&
			_c == triangle->c)
		{
			return true;
		}
		if (_a == triangle->b &&
			_b == triangle->c &&
			_c == triangle->a)
		{
			return true;
		}
		if (_a == triangle->c &&
			_b == triangle->a &&
			_c == triangle->b)
		{
			return true;
		}
	}
#ifdef AN_DEVELOPMENT
	for_every(triangle, triangles)
	{
		if (_a == triangle->a ||
			_a == triangle->b ||
			_a == triangle->c)
		{
			if (_b == triangle->a ||
				_b == triangle->b ||
				_b == triangle->c)
			{
				if (_c == triangle->a ||
					_c == triangle->b ||
					_c == triangle->c)
				{
					debug_visualise(_a, _b, _c);
					an_assert(false, TXT("reversed triangle?"));
				}
			}
		}
	}
#endif
	return false;
}

bool ConvexHull::intersects_coplanar_triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c) const
{
	Triangle abc = Triangle(_a, _b, _c);
	for_every(triangle, triangles)
	{
		if (Vector3::dot(abc.abc.get_normal(), triangle->abc.get_normal()) > 0.995f)
		{
			if (! abc.has_on_outside(*triangle) &&
				! triangle->has_on_outside(abc))
			{
				return true;
			}
		}
	}
	return false;
}

int32 ConvexHull::find_point_index(Vector3 const & _a) const
{
	for_every(point, points)
	{
		if (_a == *point)
		{
			return (int32)(point - points.begin());
		}
	}
	return NONE;
}

void ConvexHull::build_bisect()
{
	// build list of edges
	for_every(triangle, triangles)
	{
		// get planes that separate triangles
		triangle->abToTriangle = triangle->get_out_separation_plane(triangle->a, triangle->b, triangle->abIn, triangle->abTriangle);
		triangle->bcToTriangle = triangle->get_out_separation_plane(triangle->b, triangle->c, triangle->bcIn, triangle->bcTriangle);
		triangle->caToTriangle = triangle->get_out_separation_plane(triangle->c, triangle->a, triangle->caIn, triangle->caTriangle);
		add_edge(triangle->a, triangle->b, triangle);
		add_edge(triangle->b, triangle->c, triangle);
		add_edge(triangle->c, triangle->a, triangle);
	}

	if (edges.is_empty())
	{
		return;
	}

	an_assert(bisect == nullptr);

	// use copy of edges to create bisect
	ARRAY_PREALLOC_SIZE(Edge, tempEdges, edges.get_size());
	tempEdges = edges;

	// add first edge
	bisect = new Bisect(tempEdges[0]);
	tempEdges.remove_fast_at(0);

	// add all tempEdges - add most perpendicular
	while (!tempEdges.is_empty())
	{
		Edge const * bestEdge = nullptr;
		float bestEdgePerp = 0.0f;
	
		for_every_const(edge, tempEdges)
		{
			float edgePerp = bisect->calculate_perpendicular(*edge);
			if (edgePerp >= bestEdgePerp)
			{
				bestEdge = edge;
				bestEdgePerp = edgePerp;
			}
		}

		bisect->add(*bestEdge);
		tempEdges.remove_fast(bestEdge);
	}
}

void ConvexHull::add_edge(Vector3 const & _a, Vector3 const & _b, const Triangle * _inside)
{
	for_every(edge, edges)
	{
		if (_a == edge->b &&
			_b == edge->a)
		{
			an_assert(edge->triangleMinus == nullptr, TXT("edge should be shared by two triangles only"));
#ifdef AN_DEVELOPMENT
#ifdef AN_INVESTIGATE_MULTI_SHARE_FOR_EDGES
			if (edge->triangleMinus != nullptr)
			{
				output(TXT("investigate?"));
				debug_visualise(_a, _b);
			}
#endif
#endif
			edge->triangleMinus = _inside;
			return;
		}
	}
	Edge edge;
	edge.a = _a;
	edge.b = _b;
	Plane abcentre = Plane(_a, _b, centre);
	edge.inPlus = abcentre;
	edge.trianglePlus = _inside;
	edges.push_back(edge);
}

ConvexHull::Triangle const * ConvexHull::find_triangle(Vector3 const & _point, Triangle const * _startingTriangle) const
{
	Triangle const * triangle = _startingTriangle ? _startingTriangle : bisect->find_triangle(_point);
	Triangle const * prevTriangle = &triangles[0];
	int32 tryCount = triangles.get_size();
	while (tryCount--)
	{
		todo_note(TXT("at some point in future - make it cleaner"));
		if (!triangle->switch_to_triangle(triangle, prevTriangle, _point))
		{
			return triangle;
		}
		an_assert(triangle);
	}
	an_assert(nullptr);
	return nullptr;
}

/*
float ConvexHull::get_distance(Vector3 const & _point, REF_ ClosestPointContext & _closestPointContext) const
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

bool ConvexHull::load_from_xml(IO::XML::Node const * _node, bool _allowLessThanFour)
{
	if (!_node)
	{
		return false;
	}
	clear();
	Array<IO::XML::Node const*> cornerNodes;
	if (_node->get_children_named(cornerNodes, TXT("corner")) >= 4 || _allowLessThanFour)
	{
		for_every_ptr(node, cornerNodes)
		{
			Vector3 corner = Vector3::zero;
			if (!corner.load_from_xml(node))
			{
				todo_important(TXT("error"));
				return false;
			}
			add(corner);
		}
		if (cornerNodes.get_size() >= 4)
		{
			build();
		}
		return true;
	}
	return false;
}

bool ConvexHull::does_intersect(ConvexHull const & _a, Transform const & _aPlacement, ConvexHull const & _b, Transform const & _bPlacement)
{
	Transform relativePlacement = _aPlacement.to_local(_bPlacement);
	return _a.does_intersect_with(_b, relativePlacement);
}

bool ConvexHull::does_intersect_with(ConvexHull const & _other, Transform const & _otherRelativePlacement) const
{
	Transform const thisRelativePlacement = _otherRelativePlacement.inverted();
	bool intersects;
	
	// check if we won't intersect for sure - check if all points lay in front of one triangle of convex hull
	// also - if at least one point lays within other - we intersect

	// first - simple axis separation using normal of faces
	todo_note(TXT("think maybe about smarter axis separation that would predict which one to use first?"));
	if (check_intersection_against_points(_otherRelativePlacement, REF_ _other.points, OUT_ intersects))
	{
		return intersects;
	}
	if (_other.check_intersection_against_points(thisRelativePlacement, REF_ points, OUT_ intersects))
	{
		return intersects;
	}

	// check intersection of triangles against edges
	if (check_intersection_against_edges(_otherRelativePlacement, REF_ _other.edges, OUT_ intersects))
	{
		return intersects;
	}
	if (_other.check_intersection_against_edges(thisRelativePlacement, REF_ edges, OUT_ intersects))
	{
		return intersects;
	}

	// check if centre is inside (this means that whole thing intersects)
	if (does_contain_inside(_otherRelativePlacement.location_to_world(_other.centre)) ||
		_other.does_contain_inside(thisRelativePlacement.location_to_world(centre)))
	{
		return true;
	}

	// no intersection then
	return false;
}

bool ConvexHull::does_contain_inside(Vector3 const & _point, OUT_ OPTIONAL_ Vector3 * _normal) const
{
	for_every(triangle, triangles)
	{
		if (!triangle->check_point_inside(_point, _normal))
		{
			return false;
		}
	}
	// inside against all
	return true;
}

bool ConvexHull::check_intersection_against_points(Transform const & _otherRelativePlacement, REF_ Array<Vector3> const & _points, OUT_ bool & _intersects) const
{
	int pointCount = _points.get_size();
	int triangleCount = triangles.get_size();

	// during checks remember if for each point if it was inside - at the end we will check if any point was inside against all triangles - that would mean that point was inside -> intersection
	ARRAY_PREALLOC_SIZE(int, pointsInside, pointCount);
	pointsInside.set_size(pointCount);
	memory_zero(pointsInside.get_data(), sizeof(int) * pointCount);

	// calculate using relative placement
	ARRAY_PREALLOC_SIZE(Vector3, localPoints, _points.get_size());
	localPoints.set_size(_points.get_size());
	Vector3 * localPoint = localPoints.get_data();
	for_every_const(point, _points)
	{
		*localPoint = _otherRelativePlacement.location_to_world(*point);
		++localPoint;
	}

	// if they are separated by axis, this might be the best one to work with
	Triangle const * tryTriangle = find_triangle(localPoints[0], triangles.get_data());
	if (tryTriangle->check_points_inside(localPoints, pointsInside.get_data()) == 0)
	{
		_intersects = false; // all in front - we don't intersect
		return true;
	}

	// check if all points are in front (or at) triangle - if so - we don't intersect for sure
	for_every_const(triangle, triangles)
	{
		if (triangle != tryTriangle)
		{
			if (triangle->check_points_inside(localPoints, pointsInside.get_data()) == 0)
			{
				_intersects = false; // all in front - we don't intersect
				return true;
			}
		}
	}

	for_every_const(pointInside, pointsInside)
	{
		if (*pointInside == triangleCount)
		{
			_intersects = true; // one point is inside hull - we have to intersect
			return true;
		}
	}

	return false;
}

bool ConvexHull::check_intersection_against_edges(Transform const & _otherRelativePlacement, REF_ Array<Edge> const & _edges, OUT_ bool & _intersects) const
{
	// calculate using relative placement
	ARRAY_PREALLOC_SIZE(Edge, localEdges, _edges.get_size());
	localEdges.set_size(_edges.get_size());
	Edge * localEdge = localEdges.get_data();
	for_every_const(edge, _edges)
	{
		localEdge->a = _otherRelativePlacement.location_to_world(edge->a);
		localEdge->b = _otherRelativePlacement.location_to_world(edge->b);
		++localEdge;
	}

	for_every_const(triangle, triangles)
	{
		if (triangle->check_edges_intersect(localEdges))
		{
			_intersects = true;
			return true;
		}
	}

	// check if contains middle of edge (this may happen when edge is fully inside but points are on surfaces)
	for_every(localEdge, localEdges)
	{
		if (does_contain_inside((localEdge->a + localEdge->b) * 0.5f))
		{
			return true;
		}
	}

	return false;
}

void ConvexHull::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const
{
#ifdef AN_DEBUG_RENDERER
	Colour colour = _colour;
#ifdef AN_DEVELOPMENT
	if (highlightDebugDraw)
	{
		colour = get_debug_highlight_colour();
	}
#endif

	Colour fillColour = colour * Colour::alpha(_alphaFill);

	for_every(edge, edges)
	{
		debug_draw_line(_frontBorder, colour, edge->a, edge->b);
	}

	for_every(triangle, triangles)
	{
		debug_draw_triangle(_frontFill, fillColour, triangle->a, triangle->b, triangle->c);
	}
#endif
}

void ConvexHull::log(LogInfoContext & _context) const
{
	_context.log(TXT("convex hull"));
#ifdef AN_DEVELOPMENT
	_context.on_last_log_select([this](){highlightDebugDraw = true; }, [this](){highlightDebugDraw = false; });
#endif
	LOG_INDENT(_context);
	_context.log(TXT("+- points count : %i"), points.get_size());
	_context.log(TXT("+- triangles count : %i"), triangles.get_size());
}

bool ConvexHull::check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increasedSize) const
{
	Vector3 startTLoc = _segment.get_at_t(_segment.get_start_t());
	Triangle const * triangle = find_triangle(startTLoc, nullptr);
	if (triangle->abc.get_in_front(startTLoc) < _increasedSize)
	{
		// starts inside
		if ((!_context || !_context->should_ignore_reversed_normal()))
		{
			_hitNormal = Vector3::zero;
			return _segment.collide_at(_segment.get_start_t());
		}
		else
		{
			return false;
		}
	}
	// find exact hit location
	Triangle const * prevTriangle = triangle;
	int tryCount = triangles.get_size();
	while (tryCount > 0)
	{
		Segment segmentTest = _segment;
		Vector3 segmentHitNormal;
		if (!triangle->abc.check_segment(REF_ segmentTest, REF_ segmentHitNormal, _context, _increasedSize))
		{
			// doesn't touch it? then it doesn't touch any other as we are convex
			return false;
		}
		Vector3 hitLoc = segmentTest.get_hit();
		if (!triangle->switch_to_triangle(triangle, prevTriangle, hitLoc, _increasedSize))
		{
			// couldn't switch, this hit is valid
			_hitNormal = triangle->abc.get_normal();
			return _segment.collide_at(segmentTest.get_end_t());
		}
		--tryCount;
	}
	return false;
}

float ConvexHull::calculate_size_approximated() const
{
	Range3 range = get_range();
	int const parts = 50;
	float const partsAsFloat = (float)parts;
	float const iParts = 1.0f / partsAsFloat;
	float const hParts = 0.5f / partsAsFloat;
	float size = 0.0f;
	Vector3 probe = Vector3::zero;
	float probeSize = (range.x.length() * range.y.length() * range.z.length()) / (partsAsFloat * partsAsFloat * partsAsFloat);
	for (int ix = 0; ix < parts; ++ix)
	{
		probe.x = range.x.get_at((float)ix * iParts + hParts);
		for (int iy = 0; iy < parts; ++iy)
		{
			probe.y = range.y.get_at((float)iy * iParts + hParts);
			for (int iz = 0; iz < parts; ++iz)
			{
				probe.z = range.z.get_at((float)iz * iParts + hParts);
				if (does_contain_inside(probe))
				{
					size += probeSize;
				}
			}
		}
	}
	return size;
}

bool ConvexHull::get_vertex(int _idx, OUT_ Vector3& _v) const
{
	if (!points.is_index_valid(_idx))
	{
		return false;
	}

	_v = points[_idx];

	return true;
}

bool ConvexHull::get_triangle(int _idx, OUT_ Vector3 & _a, OUT_ Vector3 & _b, OUT_ Vector3 & _c) const
{
	if (! triangles.is_index_valid(_idx))
	{
		return false;
	}

	auto const & triangle = triangles[_idx];
	_a = triangle.a;
	_b = triangle.b;
	_c = triangle.c;

	return true;
}

void ConvexHull::apply_transform(Matrix44 const & _transform)
{
	if (_transform == Matrix44::identity)
	{
		return;
	}
	clear_keep_points();
	range = Range3::empty;
	for_every(point, points)
	{
		*point = _transform.location_to_world(*point);
		range.include(*point);
	}
	build();
}

Range3 ConvexHull::calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const
{
	Range3 boundingBox = Range3::empty;
	if (_quick)
	{
		boundingBox.construct_from_placement_and_range3(_usingPlacement, range);
	}
	else
	{
		for_every(point, points)
		{
			boundingBox.include(_usingPlacement.location_to_world(*point));
		}
	}
	return boundingBox;
}

#ifdef AN_DEVELOPMENT
String ConvexHull::debug_build_code() const
{
	String logPoints;
	logPoints = TXT("code:\n");
	logPoints += TXT("using orgPoints:\n");
	logPoints += TXT("  ConvexHull ch;\n");
	for_every(point, orgPoints)
	{
		logPoints += String::printf(TXT("  ch.add(Vector3(%18.14ff,%18.14ff,%18.14ff));\n"), point->x, point->y, point->z);
	}
	logPoints += TXT("  ch.build();\n");
	logPoints += TXT("ends with points:\n");
	logPoints += TXT("  ConvexHull ch;\n");
	for_every(point, points)
	{
		logPoints += String::printf(TXT("  ch.add(Vector3(%18.14ff,%18.14ff,%18.14ff));\n"), point->x, point->y, point->z);
	}
	logPoints += TXT("  ch.build();\n");
	return logPoints;
}

void ConvexHull::debug_visualise(Optional<Vector3> _a, Optional<Vector3> _b, Optional<Vector3> _c, Optional<Vector3> _o) const
{
	if (_a.is_set())
	{
		output(TXT(" Vector3 a = Vector3(%18.14ff, %18.14ff, %18.14ff);"), _a.get().x, _a.get().y, _a.get().z);
	}
	if (_b.is_set())
	{
		output(TXT(" Vector3 b = Vector3(%18.14ff, %18.14ff, %18.14ff);"), _b.get().x, _b.get().y, _b.get().z);
	}
	if (_c.is_set())
	{
		output(TXT(" Vector3 c = Vector3(%18.14ff, %18.14ff, %18.14ff);"), _c.get().x, _c.get().y, _c.get().z);
	}
	if (_o.is_set())
	{
		output(TXT(" Vector3 d = Vector3(%18.14ff, %18.14ff, %18.14ff);"), _o.get().x, _o.get().y, _o.get().z);
	}
	String logPoints = debug_build_code();
	output(logPoints.to_char());
	DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("convex hull issues"))));
	dv->activate();
	dv->start_gathering_data();
	dv->clear();
	float pointSize = 0.2f;
	for_every(point, points)
	{
		dv->add_line_3d(*point - Vector3::xAxis * pointSize, *point + Vector3::xAxis * pointSize, Colour::blue);
		dv->add_line_3d(*point - Vector3::yAxis * pointSize, *point + Vector3::yAxis * pointSize, Colour::blue);
		dv->add_line_3d(*point - Vector3::zAxis * pointSize, *point + Vector3::zAxis * pointSize, Colour::blue);
		dv->add_text_3d(*point, String::printf(TXT("%i"), for_everys_index(point)), Colour::blue);
	}
	if (_o.is_set())
	{
		dv->add_line_3d(_o.get() - Vector3::xAxis * pointSize, _o.get() + Vector3::xAxis * pointSize, Colour::red);
		dv->add_line_3d(_o.get() - Vector3::yAxis * pointSize, _o.get() + Vector3::yAxis * pointSize, Colour::red);
		dv->add_line_3d(_o.get() - Vector3::zAxis * pointSize, _o.get() + Vector3::zAxis * pointSize, Colour::red);
	}
	for_every(edge, edges)
	{
		dv->add_line_3d(edge->a, edge->b, Colour::green);
	}
	for_every(triangle, triangles)
	{
		Vector3 cent = (triangle->a + triangle->b + triangle->c) / 3.0f;
		Vector3 a = cent + (triangle->a - cent) * 0.95f;
		Vector3 b = cent + (triangle->b - cent) * 0.95f;
		Vector3 c = cent + (triangle->c - cent) * 0.95f;
		dv->add_line_3d(a, b, Colour::yellow);
		dv->add_line_3d(b, c, Colour::yellow);
		dv->add_line_3d(c, a, Colour::yellow);
	}
	if (_a.is_set() && _b.is_set() && _c.is_set())
	{
		Vector3 cent = (_a.get() + _b.get() + _c.get()) / 3.0f;
		Vector3 a = cent + (_a.get() - cent) * 0.9f;
		Vector3 b = cent + (_b.get() - cent) * 0.9f;
		Vector3 c = cent + (_c.get() - cent) * 0.9f;
		dv->add_line_3d(a, c, Colour::red);
		dv->add_line_3d(b, a, Colour::green);
		dv->add_line_3d(c, b, Colour::blue);
	}
	else if (_a.is_set() && _b.is_set())
	{
		Vector3 cent = (_a.get() + _b.get()) / 2.0f;
		Vector3 a = cent + (_a.get() - cent) * 1.0f;
		Vector3 b = cent + (_b.get() - cent) * 1.0f;
		dv->add_line_3d(a, b, Colour::red);
	}
	dv->end_gathering_data();
	dv->show_and_wait_for_key();
	dv->deactivate();
}
#endif

//

bool ConvexHull::Triangle::is_degenerate(Vector3 const& a, Vector3 const& b, Vector3 const& c)
{
	if (a == b || b == c || a == c)
	{
		return true;
	}

	Plane abc;
	Plane abIn, bcIn, caIn;

	abc.set(a, b, c);
	Vector3 a2b = (b - a).normal();
	Vector3 a2c = (c - a).normal();
	Vector3 b2a = (a - b).normal();
	Vector3 b2c = (c - b).normal();
	Vector3 c2a = (a - c).normal();
	Vector3 c2b = (b - c).normal();
	// xy->z
	// zx->y
	// calculate normals for each edge towards other point
	Vector3 nab = Vector3::cross(Vector3::cross(a2b, a2c), a2b).normal();
	Vector3 nbc = Vector3::cross(Vector3::cross(b2c, b2a), b2c).normal();
	Vector3 nca = Vector3::cross(Vector3::cross(c2a, c2b), c2a).normal();
	abIn.set(nab, a);
	bcIn.set(nbc, b);
	caIn.set(nca, c);

	if (abIn.get_normal().is_zero() ||
		bcIn.get_normal().is_zero() ||
		caIn.get_normal().is_zero())
	{
		return true;
	}

	return false;
}

ConvexHull::Triangle::Triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
: a(_a)
, b(_b)
, c(_c)
{
	abc.set(a, b, c);
	// xAxis = Vector3::cross(yAxis, zAxis);
	// forward -> from point to point
	// up -> plane's normal
	// right -> in dir
	Vector3 nab = Vector3::cross(b - a, abc.normal).normal();
	Vector3 nbc = Vector3::cross(c - b, abc.normal).normal();
	Vector3 nca = Vector3::cross(a - c, abc.normal).normal();
	abIn.set(nab, a);
	bcIn.set(nbc, b);
	caIn.set(nca, c);
	an_assert(!abIn.get_normal().is_zero());
	an_assert(!bcIn.get_normal().is_zero());
	an_assert(!caIn.get_normal().is_zero());

	prepare_cached();
}

ConvexHull::Triangle::Triangle(Vector3 const & _a, Vector3 const & _b, Vector3 const & _c, bool _justStore)
: a(_a)
, b(_b)
, c(_c)
{

	prepare_cached();
}

void ConvexHull::Triangle::prepare_using_centre(Vector3 const& _hullCentre)
{
	{
		Vector3 ab = b - a;
		Vector3 ac = c - a;
		Vector3 normal = Vector3::cross(ac, ab).normal();

		if (normal.is_zero())
		{
			// if point, we just use normal as point - centre
			if (a == b && b == c)
			{
				normal = (a - _hullCentre).normal();
				abc.set(normal, a);
			}
			else
			{
				// line then, two non equal points are enough
				Vector3 p0 = a;
				Vector3 p1 = a == b ? c : b;
				Vector3 p01 = p1 - p0;
				normal = (a - _hullCentre).normal();
				Vector3 nominalNormal = normal;
				Vector3 right = Vector3::cross(p01, normal).normal();
				normal = Vector3::cross(normal, right).normal();

				if (Vector3::dot(normal, nominalNormal) < 0.0f)
				{
					// it points towards hull centre
					normal = -normal;
				}
				abc.set(normal, a);
			}
		}
		else
		{
			abc.set(a, b, c);
		}
	}
	// xAxis = Vector3::cross(yAxis, zAxis);
	// forward -> from point to point
	// up -> plane's normal
	// right -> in dir
	Vector3 nab = Vector3::cross(b - a, abc.normal).normal();
	Vector3 nbc = Vector3::cross(c - b, abc.normal).normal();
	Vector3 nca = Vector3::cross(a - c, abc.normal).normal();
	abIn.set(nab, a);
	bcIn.set(nbc, b);
	caIn.set(nca, c);
	an_assert(!abIn.get_normal().is_zero());
	an_assert(!bcIn.get_normal().is_zero());
	an_assert(!caIn.get_normal().is_zero());
}

void ConvexHull::Triangle::prepare_cached()
{
	a2bDist = (b - a).length();
	b2cDist = (c - b).length();
	c2aDist = (a - c).length();
	a2b = (b - a).normal();
	b2c = (c - b).normal();
	c2a = (a - c).normal();
}

bool ConvexHull::Triangle::has_on_outside(Triangle const & _t) const
{
	float const tre = 0.005f;
	if (abIn.get_in_front(_t.a) <= tre &&
		abIn.get_in_front(_t.b) <= tre &&
		abIn.get_in_front(_t.c) <= tre)
	{
		return true;
	}
	if (bcIn.get_in_front(_t.a) <= tre &&
		bcIn.get_in_front(_t.b) <= tre &&
		bcIn.get_in_front(_t.c) <= tre)
	{
		return true;
	}
	if (caIn.get_in_front(_t.a) <= tre &&
		caIn.get_in_front(_t.b) <= tre &&
		caIn.get_in_front(_t.c) <= tre)
	{
		return true;
	}
	return false;
}

bool ConvexHull::Triangle::has_edge(Vector3 const & _a, Vector3 const & _b) const
{
	return ((a == _a && b == _b) ||
			(b == _a && c == _b) ||
			(c == _a && a == _b));
}

Plane ConvexHull::Triangle::get_out_separation_plane(Vector3 const & _a, Vector3 const & _b, Plane const & _in, Triangle const * _out) const
{
	Plane invIn = _in.negated();
	Plane otherIn = invIn;
	if (_out->a == _b && _out->b == _a) { otherIn = _out->abIn; }
	if (_out->b == _b && _out->c == _a) { otherIn = _out->bcIn; }
	if (_out->c == _b && _out->a == _a) { otherIn = _out->caIn; }
	// as we need to be exactly between, we can just normalise sum of normals
	return Plane((invIn.get_normal() + otherIn.get_normal()).normal(), _a);
}

bool ConvexHull::Triangle::switch_to_triangle(Triangle const *& _newTriangle, Triangle const *& _prevTriangle, Vector3 const & _point, float _increasedSize) const
{
	float ab = _prevTriangle != abTriangle ? abToTriangle.get_in_front(_point) - _increasedSize : 0.0f;
	float bc = _prevTriangle != bcTriangle ? bcToTriangle.get_in_front(_point) - _increasedSize : 0.0f;
	float ca = _prevTriangle != caTriangle ? caToTriangle.get_in_front(_point) - _increasedSize : 0.0f;
	// find triangle on edge closest
	if (ab > 0.0f && (ab <= bc || bc <= 0.0f) && (ab <= ca || ca <= 0.0f))
	{
		_prevTriangle = this;
		_newTriangle = abTriangle;
		return true;
	}
	else if (bc > 0.0f && (bc <= ab || ab <= 0.0f) && (bc <= ca || ca <= 0.0f))
	{
		_prevTriangle = this;
		_newTriangle = bcTriangle;
		return true;
	}
	else if (ca > 0.0f && (ca <= ab || ab <= 0.0f) && (ca <= bc || bc <= 0.0f))
	{
		_prevTriangle = this;
		_newTriangle = caTriangle;
		return true;
	}
	else
	{
		return false;
	}
}

bool ConvexHull::Triangle::check_point_inside(Vector3 const & _point, OUT_ OPTIONAL_ Vector3 * _normal) const
{
	bool inside = abc.get_in_front(_point) < 0.0f;
	if (!inside && _normal)
	{
		assign_optional_out_param(_normal, -abc.get_normal());
	}
	return inside;
}

int ConvexHull::Triangle::check_points_inside(REF_ Array<Vector3> const & _points, int * _pointsInside) const
{
	int result = 0;
	for_every_const(point, _points)
	{
		if (abc.get_in_front(*point) < 0.0f)
		{
			*_pointsInside = *_pointsInside + 1;
			++result;
		}
		++_pointsInside;
	}
	return result;
}

bool ConvexHull::Triangle::check_edges_intersect(REF_ Array<Edge> const & _edges) const
{
	for_every_const(edge, _edges)
	{
		float a = abc.get_in_front(edge->a);
		float b = abc.get_in_front(edge->b);

		if (a * b > 0.0f || // no point at the triangle
			(a >= 0.0f && b >= 0.0f)) // both in front
		{
			continue;
		}

		// we actually intersect at triangle plane, get intersection point
		float const length = a - b;
		an_assert(length != 0.0f);
		float const t = a / length;
		Vector3 const atTriangle = edge->a * t + (1.0f - t) * edge->b;

		// check if we're inside
		if (abIn.get_in_front(atTriangle) > 0.0f &&
			bcIn.get_in_front(atTriangle) > 0.0f &&
			caIn.get_in_front(atTriangle) > 0.0f)
		{
			// we intersect this edge with triangle
			return true;
		}
	}
	// no edge intersects this triangle
	return false;
}

//

ConvexHull::Bisect::Bisect(Edge const & _edge)
: inPlus(_edge.inPlus)
, trianglePlus(_edge.trianglePlus)
, triangleMinus(_edge.triangleMinus)
, plus(nullptr)
, minus(nullptr)
{
}

ConvexHull::Bisect::~Bisect()
{
	delete_and_clear(plus);
	delete_and_clear(minus);
}

ConvexHull::Triangle const * ConvexHull::Bisect::find_triangle(Vector3 const & _point) const
{
	float inFront = inPlus.get_in_front(_point);
	return inFront >= 0.0f
		? (plus ? plus->find_triangle(_point) : trianglePlus)
		: (minus ? minus->find_triangle(_point) : triangleMinus);
}

void ConvexHull::Bisect::add(Edge const & _edge)
{
	float inFrontA = inPlus.get_in_front(_edge.a);
	float inFrontB = inPlus.get_in_front(_edge.b);
	if (is_almost_zero(inFrontA) && is_almost_zero(inFrontB))
	{
		// ignore
	}
	else if (inFrontA * inFrontB < 0.0f)
	{
		// both sides
		add_at(REF_ plus, _edge);
		add_at(REF_ minus, _edge);
	}
	else if (inFrontA < 0.0f || inFrontB < 0.0f) // there's no other way - one of them is at minus (if we check just inFrontA, it could be 0
	{
		// minus
		add_at(REF_ minus, _edge);
	}
	else
	{
		// plus
		add_at(REF_ plus, _edge);
	}
}

void ConvexHull::Bisect::add_at(Bisect*& _bisect, Edge const & _edge)
{
	if (!_bisect)
	{
		_bisect = new Bisect(_edge);
	}
	else
	{
		_bisect->add(_edge);
	}
}

float ConvexHull::Bisect::calculate_perpendicular(Edge const & _edge) const
{
	float inFrontA = inPlus.get_in_front(_edge.a);
	float inFrontB = inPlus.get_in_front(_edge.b);
	if (is_almost_zero(inFrontA) && is_almost_zero(inFrontB))
	{
		// get it first 
		return 0.0f;
	}
	else if (inFrontA * inFrontB < 0.0f)
	{
		// both sides
		return min(calculate_perpendicular_at(REF_ plus, _edge), calculate_perpendicular_at(REF_ minus, _edge));
	}
	else if (inFrontA < 0.0f || inFrontB < 0.0f) // check above
	{
		// minus
		return calculate_perpendicular_at(REF_ minus, _edge);
	}
	else
	{
		// plus
		return calculate_perpendicular_at(REF_ plus, _edge);
	}
}

float ConvexHull::Bisect::calculate_perpendicular_at(Bisect const * _bisect, Edge const & _edge) const
{
	if (_bisect)
	{
		return _bisect->calculate_perpendicular(_edge);
	}
	else
	{
		return 1.0f - clamp(abs(Vector3::dot(_edge.inPlus.get_normal(), inPlus.get_normal())), 0.0f, 1.0f);
	}
}

//

ConvexHull::Edge::Edge()
: trianglePlus(nullptr)
, triangleMinus(nullptr)
{
}

//
