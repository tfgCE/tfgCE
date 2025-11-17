#include "viewFrustum.h"

//

#ifdef AN_ALLOW_EXTENSIVE_LOGS
// #define DEBUG_CLIPPING 
#endif

#define for_every_edge_and_prev(edge, prev, edges) \
	if (auto edge = edges.begin()) \
	if (auto end##edge = edges.end()) \
	if (auto prev = end##edge) \
	for (edges.is_empty()? (prev = edges.begin()) : -- prev; edge != end##edge; prev = edge, ++ edge)

//

using namespace System;

//

ViewFrustum::ViewFrustum()
#ifdef AN_ASSERT
:	normalsValid( true )
#endif
{
	SET_EXTRA_DEBUG_INFO(edges, TXT("ViewFrustum.edges"));
}

ViewFrustum::~ViewFrustum()
{
	clear();
}

void ViewFrustum::clear()
{
	edges.clear();
#ifdef AN_ASSERT
	normalsValid = true;
#endif
}
	
void ViewFrustum::add_point(Vector3 const & LOCATION_ _point)
{
	edges.push_back(ViewFrustumEdge(_point));
#ifdef AN_ASSERT
	normalsValid = false;
#endif
}

void ViewFrustum::build()
{
	for_every_edge_and_prev(edge, prev, edges)
	{
		prev->normal = Vector3::cross(prev->point, edge->point).normal();
	}
#ifdef AN_ASSERT
	normalsValid = true;
#endif
}

static inline float compute_intersection(Vector3 const & point, Vector3 const & prev, ViewFrustumEdge const * clipEdge, OUT_ Vector3& outIntersection)
{
	float dotPoint = Vector3::dot(point, clipEdge->normal);
	float dotPrev = Vector3::dot(prev, clipEdge->normal);
	if (dotPoint != dotPrev)
	{
		// find point on edge
		float it = (0.0f - dotPrev) / (dotPoint - dotPrev);
#ifdef DEBUG_CLIPPING
		output(TXT(" it %.3f"), it);
#endif
		outIntersection = prev + (point - prev) * it;
		return it;
	}
	an_assert(false);
	outIntersection = prev;
	return 0.0f;
}

bool ViewFrustum::clip(ViewFrustum const * _subject, ViewFrustum const * _clipper)
{
#ifdef AN_ASSERT
	an_assert(_subject->normalsValid, TXT("call build() on subject after adding points!"));
	an_assert(!_clipper || _clipper->normalsValid, TXT("call build() on clipper after adding points!"));
#endif
	//	based on: http://en.wikipedia.org/wiki/Sutherland-Hodgman_clipping_algorithm

#ifdef DEBUG_CLIPPING
	output(TXT(" clip"));
#endif
	// TODO an_assert(_clipper->is_convex(), TXT("clipping frustum should be convex"));
	clear();

	if (_subject->is_empty())
	{
		return false;
	}

	// allocate two temporary containers to work with
	uint availablePointCount = _subject->edges.get_size() + _clipper->edges.get_size() + 2;
	allocate_stack_var(Vector3, edgesTempA, availablePointCount);
	allocate_stack_var(Vector3, edgesTempB, availablePointCount);

	// setup variables for swapping containers
	Vector3* subject = edgesTempA;
	Vector3* newSubject = edgesTempB;
	uint subjectCount = 0;

	// copy _subject points to subject
	for_every_const(subjectEdge, _subject->edges)
	{
#ifdef DEBUG_CLIPPING
		output(TXT("  subject %S"), subjectEdge->point.to_string().to_char());
#endif
		*subject = subjectEdge->point;
		++ subject;
	}
	subjectCount = (uint)(subject - edgesTempA);
	subject = edgesTempA;

	// for each clipping edge...
	if (_clipper)
	{
		for_every_const(clipEdge, _clipper->edges)
		{
#ifdef DEBUG_CLIPPING
			output(TXT(" clip against %S"), clipEdge->normal.to_string().to_char());
#endif
			// go through all points of current input list
			Vector3 prev = *(subject + subjectCount - 1);
			float const insideDotLimit = -0.001f;
			bool prevInside = Vector3::dot(clipEdge->normal, prev) >= insideDotLimit;
			Vector3* newPoint = newSubject;
			for (Vector3* point = subject; subjectCount; -- subjectCount, ++ point)
			{
#ifdef DEBUG_CLIPPING
				output(TXT(" is inside? %.3f %S"), Vector3::dot(clipEdge->normal, *point), Vector3::dot(clipEdge->normal, *point) >= insideDotLimit? TXT("inside") : TXT("outside"));
#endif
				
				if (Vector3::dot(clipEdge->normal, *point) >= insideDotLimit)
				{
					// current is inside
					if (! prevInside)
					{
						// previous was not - add intersection
						if (compute_intersection(*point, prev, clipEdge, *newPoint) < 1.0f)
						{
#ifdef DEBUG_CLIPPING
							output(TXT("   add intersection !prev %S"), newPoint->to_string().to_char());
#endif
							++ newPoint;
						}
					}
					// add current (it is inside)
					*newPoint = *point;
#ifdef DEBUG_CLIPPING
					output(TXT("   add inside %S"), newPoint->to_string().to_char());
#endif
					++ newPoint;
					prevInside = true;
				}
				else
				{
					// current is outside
					if (prevInside)
					{
						// previous was inside - add intersection
						if (compute_intersection(*point, prev, clipEdge, *newPoint) > 0.0f)
						{
#ifdef DEBUG_CLIPPING
							output(TXT("   add intersection !curr %S"), newPoint->to_string().to_char());
#endif
							++ newPoint;
						}
					}
					prevInside = false;
				}
				// store previous
				prev = *point;
			}
			// how many points do we have now?
			subjectCount = (uint)(newPoint - newSubject);
			an_assert(subjectCount <= availablePointCount, TXT("add more points"));
#ifdef DEBUG_CLIPPING
			output(TXT("   subjectCount %i"), subjectCount);
#endif
			if (! subjectCount)
			{
#ifdef DEBUG_CLIPPING
				output(TXT(" failed"));
#endif
				return false;
			}
			// let's get ready for another pass
			swap(subject, newSubject);
		}
	}

#ifdef DEBUG_CLIPPING
	output(TXT(" result"));
#endif
	// store final subject
	for (Vector3* point = subject; subjectCount; -- subjectCount, ++ point)
	{
#ifdef DEBUG_CLIPPING
		output(TXT("   point %S"), point->to_string().to_char());
#endif
		edges.push_back(ViewFrustumEdge(*point));
	}

#ifdef AN_ASSERT
	Vector3* prevPoint = &subject[subjectCount - 1];
	for (Vector3* point = subject; subjectCount; -- subjectCount, ++ point)
	{
		an_assert(prevPoint != point, TXT("doubled point"));
		prevPoint = point;
	}
#endif

	// calculate normals
	build();

	return true;
}

#ifdef DEBUG_CLIPPING
#undef DEBUG_CLIPPING
#endif

