#ifdef AN_CLANG
#include "..\debug\debugRenderer.h"
#include "queryPrimitivePoint.h"
#endif

//#define USE_SQUARE_FOR_QUERY_PRIMITIVE_SEGMENT
// using square distance may end up with missing small objects
// todo_note - use different approach? adjust parameters? use square for some cases, first few tries only?

#ifdef USE_SQUARE_FOR_QUERY_PRIMITIVE_SEGMENT
#define SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT true
#else
#define SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT false
#endif

// old method is probably slower and less reliable
//#define OLD_METHOD_FOR_QUERY_PRIMITIVE_SEGMENT

template <typename ShapeObject>
Collision::GradientQueryResult Collision::QueryPrimitiveSegment::get_gradient(ShapeObject const & _shape, float _maxDistance) const
{
	debug_push_filter(moduleCollisionQuerries);
	QueryPrimitivePoint const qA(locationA);
	QueryPrimitivePoint const qB(locationB);

	todo_note(TXT("get rid of min distance, we have negative distances"));
	float const delta = 0.01f;
#ifdef USE_SQUARE_FOR_QUERY_PRIMITIVE_SEGMENT
	float const invDelta = 1.0f / sqr(delta);
#else
	float const invDelta = 1.0f / delta;
#endif
	Vector3 const a2b = locationB - locationA;
	Vector3 const a2bd = a2b * delta;
	QueryPrimitivePoint const qdA(locationA + a2bd);
	QueryPrimitivePoint const qdB(locationB - a2bd);

	// use working distance
	// this is really important for segment as both points may be further from the shape than _maxDistance but middle of the segment is close to it
	// but we require info about delta to move along a2b (check dCur calculation)
	float const maxWorkingDistance = max(_maxDistance, a2b.length());

	GradientQueryResult const gqrA = qA.get_gradient(_shape, maxWorkingDistance, SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT);
	GradientQueryResult const gqrB = qB.get_gradient(_shape, maxWorkingDistance, SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT);
	GradientQueryResult const gqrdA = qdA.get_gradient(_shape, maxWorkingDistance, SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT);
	GradientQueryResult const gqrdB = qdB.get_gradient(_shape, maxWorkingDistance, SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT);

	float dA = (gqrdA.distance - gqrA.distance) * invDelta;
	float dB = (gqrdB.distance - gqrB.distance) * invDelta;
	
	QueryPrimitivePoint closest = qA;

	if (gqrA.distance < gqrB.distance && dA > 0.0f)
	{
		// if at A it grows (when heading to centre) and A is closer than B, choose A
		closest = qA;
	}
	else if (gqrB.distance < gqrA.distance && dB > 0.0f)
	{
		// if at B it grows (when heading to centre) and B is closer than A, choose B
		closest = qB;
	}
	else
	{
		// find using derevative
		float curLoc = gqrA.distance < gqrB.distance? 0.0f : 1.0f;

#ifndef OLD_METHOD_FOR_QUERY_PRIMITIVE_SEGMENT
		// but when we get below min distance, try to stop at min distance
		float dStep = curLoc < 0.0f ? 0.1f : -0.1f;
		int triesLeft = 10;
		float prevDist = min(gqrA.distance, gqrB.distance);
		curLoc += dStep;
		while (triesLeft-- > 0)
		{
			Vector3 locCur = locationA + a2b * (curLoc);
			//Vector3 locCur = locationA * (1.0f - curLoc) + locationB * (curLoc);
			QueryPrimitivePoint qCur(locCur);
			// try to use as low distance as possible to avoid unnecessary checks
			float currentWorkingDistance = max(prevDist, _maxDistance);
			GradientQueryResult gqrCur = qCur.get_gradient(_shape, currentWorkingDistance, SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT);
			closest = qCur;
			if (dStep > 0.005f && 
				(curLoc + dStep >= 1.0f ||
				 curLoc + dStep <= 0.0f))
			{
				dStep *= 0.6f;
			}
			if (gqrCur.distance > prevDist)
			{
				dStep *= -0.6f;
			}
			curLoc = clamp(curLoc + dStep, 0.0f, 1.0f);
			prevDist = gqrCur.distance;
		}
#else
		float dStep = 0.5f;

		float epsilon = 0.001f;
		float const invDeltaInvMaxDist = invDelta * a2b.length() / _maxDistance;

		float prevLoc = curLoc;
		float lastDCur = 0.0f;
		int triesLeft = 10;
		while (triesLeft-- > 0)
		{
			Vector3 locCur = locationA + a2b * (curLoc);
			//Vector3 locCur = locationA * (1.0f - curLoc) + locationB * (curLoc);
			QueryPrimitivePoint qCur(locCur);
			GradientQueryResult gqrCur = qCur.get_gradient(_shape, maxWorkingDistance, SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT);
			closest = qCur;
			QueryPrimitivePoint qdCur(locCur + a2bd);
			GradientQueryResult gqrdCur = qdCur.get_gradient(_shape, maxWorkingDistance, SQUARE_PARAM_FOR_QUERY_PRIMITIVE_SEGMENT);
			float dCur = (gqrdCur.distance - gqrCur.distance) * invDeltaInvMaxDist;
			dCur = clamp(dCur, -0.5f, 0.5f);
			prevLoc = curLoc;
			curLoc = clamp(curLoc - dCur * dStep, 0.0f, 1.0f);
			if (lastDCur * dCur < 0.0f)
			{
				dStep *= 0.75f; // slow down to catch smaller ones
			}
			lastDCur = dCur;
			if (abs(dCur) < epsilon)
			{
				break;
			}
		}
#endif
	}
	// calculate gradient
	AN_CLANG_TYPENAME ShapeObject::QueryContext queryContext;
#ifdef AN_DEBUG_RENDERER
	if (debug_is_filter_allowed())
	{
		Range3 shapeRange = _shape.calculate_bounding_box(Transform::identity, nullptr, false); // pose?
		Vector3 shapeCentre = shapeRange.centre();
		debug_draw_arrow(true, Colour::yellow, shapeCentre, closest.location);
	}
#endif
	debug_pop_filter();
	return _shape.get_gradient(closest, REF_ queryContext, _maxDistance);
}

