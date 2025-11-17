#include "math.h"
#include "..\io\xml.h"

#include "..\collision\checkCollisionContext.h"
#include "..\debug\debugRenderer.h"

Capsule::Capsule()
{
}

void Capsule::setup(Vector3 const & _locationA, Vector3 const & _locationB, float _radius)
{
	locationA = _locationA;
	locationB = _locationB;
	radius = _radius;

	a2bNormal = (locationB - locationA).normal();
	a2bDist = (locationB - locationA).length();
}

bool Capsule::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	bool result = true;
	Array<IO::XML::Node const*> locationNodes;
	if (_node->get_children_named(locationNodes, TXT("location")) == 2)
	{
		locationA.load_from_xml(locationNodes[0]);
		locationB.load_from_xml(locationNodes[1]);
	}
	else
	{
		result = false;
	}
	if (IO::XML::Node const * radiusNode = _node->first_child_named(TXT("radius")))
	{
		radius = radiusNode->get_float();
	}
	else
	{
		result = false;
	}
	setup(locationA, locationB, radius);
	return result;
}

void Capsule::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const
{
#ifdef AN_DEBUG_RENDERER
	if (a2bDist <= EPSILON)
	{
		Sphere(locationA, radius).debug_draw(_frontBorder, _frontFill, _colour, _alphaFill);
		return;
	}
	Colour colour = _colour;
#ifdef AN_DEVELOPMENT
	if (highlightDebugDraw)
	{
		colour = get_debug_highlight_colour();
	}
#endif

	Vector3 up = Vector3::xAxis;
	if (abs(a2bNormal.x) >= abs(a2bNormal.y))
	{
		if (abs(a2bNormal.y) >= abs(a2bNormal.z))
		{
			up = Vector3::zAxis;
		}
		else
		{
			up = Vector3::yAxis;
		}
	}
	else
	{
		if (abs(a2bNormal.x) >= abs(a2bNormal.z))
		{
			up = Vector3::zAxis;
		}
		else
		{
			up = Vector3::xAxis;
		}
	}

	Matrix44 placementMat = look_matrix(locationA, Vector3::cross(a2bNormal, up).normal(), a2bNormal);
	Transform placement = placementMat.to_transform();
	debug_push_transform(placement);

	Colour fillColour = colour * Colour::alpha(_alphaFill);

	float const step = 10.0f;

	for (float c = step, d = 0.0f; c <= 360.0f; d = c, c += step)
	{
		Vector3 cxy = radius * Vector3(sin_deg(c), cos_deg(c), 0.0f);
		Vector3 dxy = radius * Vector3(sin_deg(d), cos_deg(d), 0.0f);

		debug_draw_line(_frontBorder, colour, Vector3(cxy.x, cxy.y, 0.0f), Vector3(dxy.x, dxy.y, 0.0f));
		debug_draw_line(_frontBorder, colour, Vector3(cxy.x, cxy.y, a2bDist), Vector3(dxy.x, dxy.y, a2bDist));
		if (cxy.y > 0.0f)
		{
			cxy.y += a2bDist;
		}
		if (dxy.y > 0.0f)
		{
			dxy.y += a2bDist;
		}
		debug_draw_line(_frontBorder, colour, Vector3(cxy.x, 0.0f, cxy.y), Vector3(dxy.x, 0.0f, dxy.y));
		debug_draw_line(_frontBorder, colour, Vector3(0.0f, cxy.x, cxy.y), Vector3(0.0f, dxy.x, dxy.y));
	}

	debug_draw_line(_frontBorder, colour, Vector3(-radius, 0.0f, 0.0f), Vector3(-radius, 0.0f, a2bDist));
	debug_draw_line(_frontBorder, colour, Vector3(radius, 0.0f, 0.0f), Vector3(radius, 0.0f, a2bDist));
	debug_draw_line(_frontBorder, colour, Vector3(0.0f, -radius, 0.0f), Vector3(0.0f, -radius, a2bDist));
	debug_draw_line(_frontBorder, colour, Vector3(0.0f, radius, 0.0f), Vector3(0.0f, radius, a2bDist));

	for (float c = step, d = 0.0f; c <= 360.0f; d = c, c += step)
	{
		{
			Vector3 cxyb = radius * Vector3(sin_deg(c), cos_deg(c), 0.0f);
			Vector3 dxyb = radius * Vector3(sin_deg(d), cos_deg(d), 0.0f);
			Vector3 cxyt = cxyb + Vector3::zAxis * a2bDist;
			Vector3 dxyt = dxyb + Vector3::zAxis * a2bDist;

			debug_draw_triangle(_frontFill, fillColour, cxyb, cxyt, dxyt);
			debug_draw_triangle(_frontFill, fillColour, dxyt, dxyb, cxyb);
		}

		for (float a = step, b = 0.0f; a <= 90.0f; b = a, a += step)
		{
			float ar = radius * cos_deg(a);
			float br = radius * cos_deg(b);
			float az = radius * sin_deg(a);
			float bz = radius * sin_deg(b);

			Vector3 cxy = Vector3(sin_deg(c), cos_deg(c), 0.0f);
			Vector3 dxy = Vector3(sin_deg(d), cos_deg(d), 0.0f);

			{	// upper
				Vector3 ca = Vector3(cxy.x * ar, cxy.y * ar, az + a2bDist);
				Vector3 cb = Vector3(cxy.x * br, cxy.y * br, bz + a2bDist);
				Vector3 da = Vector3(dxy.x * ar, dxy.y * ar, az + a2bDist);
				Vector3 db = Vector3(dxy.x * br, dxy.y * br, bz + a2bDist);

				debug_draw_triangle(_frontFill, fillColour, ca, cb, db);
				debug_draw_triangle(_frontFill, fillColour, db, da, ca);
			}

			{	// lower
				Vector3 ca = Vector3(cxy.x * ar, cxy.y * ar, -az);
				Vector3 cb = Vector3(cxy.x * br, cxy.y * br, -bz);
				Vector3 da = Vector3(dxy.x * ar, dxy.y * ar, -az);
				Vector3 db = Vector3(dxy.x * br, dxy.y * br, -bz);

				debug_draw_triangle(_frontFill, fillColour, ca, cb, db);
				debug_draw_triangle(_frontFill, fillColour, db, da, ca);
			}
		}
	}

	debug_pop_transform();
#endif
}

void Capsule::log(LogInfoContext & _context) const
{
	_context.log(TXT("capsule"));
#ifdef AN_DEVELOPMENT
	_context.on_last_log_select([this](){highlightDebugDraw = true; }, [this](){highlightDebugDraw = false; });
#endif
	LOG_INDENT(_context);
	_context.log(TXT("+- location a : %S"), locationA.to_string().to_char());
	_context.log(TXT("+- location b : %S"), locationB.to_string().to_char());
	_context.log(TXT("+- radius : %.3f"), radius);
}

bool Capsule::check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const * _context, float _increasedSize) const
{
	Vector3 const startT = _segment.get_at_t(_segment.get_start_t());
	Vector3 const end = _segment.get_end();
	Vector3 const a2sT = startT - locationA;
	Vector3 const startTatA = locationA + a2sT.drop_using_normalised(a2bNormal); // at plane perpendicular to a2b direction going through A

	float const radiusIncreased = radius + _increasedSize;

	// find collision of segment and cylinder
	Vector3 const sTa2a = locationA - startTatA;
	float const sTa2aSquare = sTa2a.length_squared();
	float const rSqr = sqr(radiusIncreased);
	if (sTa2aSquare <= rSqr)
	{
		if ((!_context || !_context->should_ignore_reversed_normal()))
		{
			// we start inside cylinder - check where
			float const cylinderAlong = Vector3::dot(startT - locationA, a2bNormal);
			if (cylinderAlong >= 0.0f && cylinderAlong <= a2bDist)
			{
				// between A and B
				_hitNormal = Vector3::zero;
				return _segment.collide_at(_segment.get_start_t());
			}
			else if (cylinderAlong < 0.0f)
			{
				// A end
				return Sphere(locationA, radiusIncreased).check_segment(REF_ _segment, REF_ _hitNormal, _context);
			}
			else
			{
				// B end
				return Sphere(locationB, radiusIncreased).check_segment(REF_ _segment, REF_ _hitNormal, _context);
			}
		}
		else
		{
			// can't
			return false;
		}
	}

	// we're looking from "top" (plane perpendicular to a2b direction)
	//				^
	//          _	 \
	//		   / \	  \
	//		  /   \	   \
	//		 |	A  |	\
	//		  \   /		 \
	//		   \_/		  \ start_to_end_dir
	//					   \
	//						\
	//						stA
	//
	// we want to check if A is within increased radius to check if we could actually cross A
	// first we need to drop start_to_end_dir onto plane (normalise it to have pure dir)
	Vector3 onPlaneStartToEndDir = _segment.get_start_to_end_dir().drop_using_normalised(a2bNormal).normal();
	// next check collision between that direction and circle
	//		   |    A    |
	//			\	:   /								x - closest point to A
	// < - - - - o  x  * - - - - - - - - - stA			* - first collision
	//			  \___/									o - second collision
	// knowing vector(stA -> A) and dir(stA -> x) we can calculate distance(stA -> x)
	// this is our sTa2a_along_dir
	float const sTa2a_along_dir = Vector3::dot(sTa2a, onPlaneStartToEndDir);
	// first we check whether x is not further than r:
	//		A					.
	//		|\					.
	//	   h| \ sTa2a			.
	//		|  \				.
	//		*---stA				.
	//		 sTa2a_along_dir	.
	float const hSqr = sTa2aSquare - sqr(sTa2a_along_dir);
	float const xSqr = rSqr - hSqr;
	if (xSqr < 0.0f)
	{
		// wents too far away - next to it
		return false;
	}

	// next check distance (x) to catch first collision point
	//		A									.
	//		|\									.
	//     h| \r								.
	//		|  \				_________		.
	//		*-x-*--t--stA  x =\/r^2 - h^2'		.
	//		\_________/							.
	//		 sTa2a_along_dir					.
	float x = sqrt(xSqr);
	float t = sTa2a_along_dir - x;
	// compare against dropped length
	Vector3 onPlaneStartToEnd = _segment.get_start_to_end().drop_using_normalised(a2bNormal);
	float onPlaneStartToEndLength = onPlaneStartToEnd.length();
	float actualT = (onPlaneStartToEndLength ? t / onPlaneStartToEndLength : 0.0f) + _segment.get_start_t();

	// find point of collision with cylinder
	Vector3 const cylinderHit = _segment.get_at_t(actualT);

	// we're outside, always hit
	float const cylinderHitAlong = Vector3::dot(cylinderHit - locationA, a2bNormal);
	if (cylinderHitAlong >= 0.0f && cylinderHitAlong <= a2bDist)
	{
		// between A and B
		Vector3 locationA2hit = cylinderHit - locationA;
		_hitNormal = (locationA2hit - a2bNormal * Vector3::dot(locationA2hit, a2bNormal)) / radiusIncreased;
		return _segment.collide_at(actualT);
	}
	else if (cylinderHitAlong < 0.0f)
	{
		// A end
		return Sphere(locationA, radiusIncreased).check_segment(REF_ _segment, REF_ _hitNormal, _context);
	}
	else
	{
		// B end
		return Sphere(locationB, radiusIncreased).check_segment(REF_ _segment, REF_ _hitNormal, _context);
	}
}

void Capsule::apply_transform(Matrix44 const & _transform)
{
	if (_transform == Matrix44::identity)
	{
		return;
	}
	if (!_transform.has_uniform_scale())
	{
		warn(TXT("scaling capsule with non uniform scale, consider using mesh"));
	}
	if (!_transform.is_orthogonal())
	{
		warn(TXT("scaling capsule with non orthogonal scale, consider using mesh"));
	}
	setup(_transform.location_to_world(locationA),
		  _transform.location_to_world(locationB),
		  _transform.extract_min_scale() * radius);
}

Range3 Capsule::calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const
{
	Range3 boundingBox = Sphere(locationA, radius).calculate_bounding_box(_usingPlacement, _quick);
	boundingBox.include(Sphere(locationB, radius).calculate_bounding_box(_usingPlacement, _quick));
	return boundingBox;
}

float Capsule::calculate_outside_distance(Vector3 const& _loc) const
{
	Segment s(locationA, locationB);
	float t = s.get_t(_loc);
	float tClamped = clamp(t, 0.0f, 1.0f);
	Vector3 closest = s.get_at_t(tClamped);
	return (_loc - closest).length() - radius;
}

bool Capsule::does_overlap(Range3 const& _range) const
{
	Vector3 locA = get_location_a();
	Vector3 locB = get_location_b();
	float rad = get_radius();
	float sqrRad = sqr(rad);

	// check if closest point of range is within sphere

	{
		Vector3 rangeClosest(_range.x.clamp(locA.x), _range.y.clamp(locA.y), _range.z.clamp(locA.z));
		if ((rangeClosest - locA).length_squared() < sqrRad)
		{
			return true;
		}
	}
	{
		Vector3 rangeClosest(_range.x.clamp(locB.x), _range.y.clamp(locB.y), _range.z.clamp(locB.z));
		if ((rangeClosest - locB).length_squared() < sqrRad)
		{
			return true;
		}
	}

	Vector3 a2b = (locationB - locationA).normal();
	float t = 0.5f;
	float d = 0.25f;
	for_count(int, iter, 16)
	{
		Vector3 a = locationA * t + (1.0f - t) * locationB;
		Vector3 ad = a2b * 0.001f;
		Vector3 aP = a + ad;
		Vector3 aM = a - ad;

		Vector3 closestRangeAP = Vector3(_range.x.clamp(aP.x), _range.y.clamp(aP.y), _range.z.clamp(aP.z));
		Vector3 closestRangeAM = Vector3(_range.x.clamp(aM.x), _range.y.clamp(aM.y), _range.z.clamp(aM.z));

		float dPsq = (closestRangeAP - aP).length_squared();
		float dMsq = (closestRangeAM - aM).length_squared();

		if (dPsq > dMsq)
		{
			t += d;
		}
		else if (dPsq < dMsq)
		{
			t -= d;
		}

		d *= 0.5f;
	}

	Vector3 closest = locationA * t + (1.0f - t) * locationB;

	Vector3 closestRangeClosest = Vector3(_range.x.clamp(closest.x), _range.y.clamp(closest.y), _range.z.clamp(closest.z));

	return (closestRangeClosest - closest).length_squared() < sqrRad;
}
