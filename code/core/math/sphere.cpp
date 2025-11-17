#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\collision\checkCollisionContext.h"

#include "..\debug\debugRenderer.h"

Sphere::Sphere()
{
}

void Sphere::setup(Vector3 const & _location, float _radius)
{
	location = _location;
	radius = _radius;
}

bool Sphere::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	bool result = true;
	result &= location.load_from_xml_child_node(_node, TXT("location"));
	if (IO::XML::Node const * radiusNode = _node->first_child_named(TXT("radius")))
	{
		radius = radiusNode->get_float();
	}
	else
	{
		result = false;
	}
	setup(location, radius);
	return result;
}

bool Sphere::load_from_string(String const& _string)
{
	List<String> tokens;
	_string.split(String::space(), tokens);
	int eIdx = 0;
	for_every(token, tokens)
	{
		if (token->is_empty())
		{
			continue;
		}
		float& e = access_element(eIdx);
		e = ParserUtils::parse_float(*token, e);
		++eIdx;
		if (eIdx >= 4)
		{
			return true;
		}
	}
	return false;
}

void Sphere::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill) const
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

	float const step = 30.0f;

	for (float c = step, d = 0.0f; c <= 360.0f; d = c, c += step)
	{
		Vector3 cxy = radius * Vector3(sin_deg(c), cos_deg(c), 0.0f);
		Vector3 dxy = radius * Vector3(sin_deg(d), cos_deg(d), 0.0f);

		debug_draw_line(_frontBorder, colour, location + cxy, location + dxy);
		debug_draw_line(_frontBorder, colour, location + Vector3(cxy.x, 0.0f, cxy.y), location + Vector3(dxy.x, 0.0f, dxy.y));
		debug_draw_line(_frontBorder, colour, location + Vector3(0.0f, cxy.x, cxy.y), location + Vector3(0.0f, dxy.x, dxy.y));
	}

	for (float a = step, b = 0.0f; a <= 90.0f; b = a, a += step)
	{
		for (float c = step, d = 0.0f; c <= 360.0f; d = c, c += step)
		{
			float ar = radius * cos_deg(a);
			float br = radius * cos_deg(b);
			float az = radius * sin_deg(a);
			float bz = radius * sin_deg(b);

			Vector3 cxy = Vector3(sin_deg(c), cos_deg(c), 0.0f);
			Vector3 dxy = Vector3(sin_deg(d), cos_deg(d), 0.0f);

			{	// upper
				Vector3 ca = location + Vector3(cxy.x * ar, cxy.y * ar, az);
				Vector3 cb = location + Vector3(cxy.x * br, cxy.y * br, bz);
				Vector3 da = location + Vector3(dxy.x * ar, dxy.y * ar, az);
				Vector3 db = location + Vector3(dxy.x * br, dxy.y * br, bz);

				debug_draw_triangle(_frontFill, fillColour, ca, cb, db);
				debug_draw_triangle(_frontFill, fillColour, db, da, ca);
			}

			{	// lower
				Vector3 ca = location + Vector3(cxy.x * ar, cxy.y * ar, -az);
				Vector3 cb = location + Vector3(cxy.x * br, cxy.y * br, -bz);
				Vector3 da = location + Vector3(dxy.x * ar, dxy.y * ar, -az);
				Vector3 db = location + Vector3(dxy.x * br, dxy.y * br, -bz);

				debug_draw_triangle(_frontFill, fillColour, ca, cb, db);
				debug_draw_triangle(_frontFill, fillColour, db, da, ca);
			}
		}
	}
#endif
}

void Sphere::log(LogInfoContext & _context) const
{
	_context.log(TXT("sphere"));
#ifdef AN_DEVELOPMENT
	_context.on_last_log_select([this](){highlightDebugDraw = true; }, [this](){highlightDebugDraw = false; });
#endif
	LOG_INDENT(_context);
	_context.log(TXT("+- location : %S"), location.to_string().to_char());
	_context.log(TXT("+- radius : %.3f"), radius);
}

bool Sphere::check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize) const
{
	Vector3 const startT2location = location - _segment.get_at_t(_segment.get_start_t());
	float const startT2locationSquare = startT2location.length_squared();
	float const radiusIncreased = radius + _increaseSize;
	float const rSqr = sqr(radiusIncreased);
	if (startT2locationSquare <= rSqr + EPSILON)
	{
		// within sphere
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

	// along segment dir
	float const sT2l_along_dir = Vector3::dot(startT2location, _segment.get_start_to_end_dir());
	if (sT2l_along_dir <= 0.0f)
	{
		// not going towards sphere
		return false;
	}

	// we have triangles:
	//	*						*
	//	|\						|\
	// h| \ sT2l			   h| \r
	//	|  \					|  \				_________
	//	*---*					*-x-*--t--*    x =\/r^2 - h^2'
	//	 sT2l_along_dir			\_________/
	//							 sT2l_along_dir
	float const hSqr = startT2locationSquare - sqr(sT2l_along_dir);
	float const xSqr = rSqr - hSqr;
	if (xSqr < 0.0f)
	{
		// wents too far away - next to it
		return false;
	}

	float const x = sqrt(xSqr);
	float const t = sT2l_along_dir - x;

	float const actualT = (_segment.length()? t / _segment.length() : 0.0f) + _segment.get_start_t();

	// we're outside, always hit
	_hitNormal = (_segment.get_at_t(actualT) - location) / radiusIncreased; // we know distance of hit location
	return _segment.collide_at(actualT);
}

void Sphere::apply_transform(Matrix44 const & _transform)
{
	if (!_transform.has_uniform_scale())
	{
		warn(TXT("scaling sphere with non uniform scale, consider using mesh"));
	}
	if (!_transform.is_orthogonal())
	{
		warn(TXT("scaling sphere with non orthogonal scale, consider using mesh"));
	}
	location = _transform.location_to_world(location);
	radius = _transform.extract_min_scale() * radius;
}

Range3 Sphere::calculate_bounding_box(Transform const & _usingPlacement, bool _quick) const
{
	Vector3 t_location = _usingPlacement.location_to_world(location);
	Range3 boundingBox;
	boundingBox.x.min = t_location.x - radius;
	boundingBox.x.max = t_location.x + radius;
	boundingBox.y.min = t_location.y - radius;
	boundingBox.y.max = t_location.y + radius;
	boundingBox.z.min = t_location.z - radius;
	boundingBox.z.max = t_location.z + radius;
	return boundingBox;
}

float Sphere::calc_intersection_t(Segment const & _segment) const
{
	float radiusSq = sqr(radius);

	/*
	 *	line = ls + t * ld;   ls-line start  ld-line direction
	 *	sphere (s) (s.x - c.x)^2 + (s.y - c.y)^2 + (s.z - c.z)^2 = r^2  s.*-on sphere  c.*-sphere's centre
	 *
	 *	(s.x - c.x)^2
	 *  (ls.x + t * ld.x - c.x)^2		ls.x - c.x -> lsc.x
	 *	(lsc.x + t * ld.x)^2
	 *	t^2 * ld.x^2 + 2 * t * ld.x * lsc.x + lsc.x^2
	 *	t^2 * (ld.x^2 + ld.y^2 + ld.z^2) + t * 2 * (ld.x * lsc.x + ld.y * lsc.y + ld.z * lsc.z) + (lsc.x^2 + lsc.y^2 + lsc.z^2) - r^2
	 *
	 *	a = (ld.x^2 + ld.y^2 + ld.z^2) -> ld.length_squared()
	 *	b = 2 * (ld.x * lsc.x + ld.y * lsc.y + ld.z * lsc.z) -> 2.0f * dot(ld, lsc)
	 *	c = (lsc.x^2 + lsc.y^2 + lsc.z^2) - r^2 -> lsc.length_squared() - sqr(r)
	 */
	Vector3 ls = _segment.get_start();
	Vector3 ld = _segment.get_end() - _segment.get_start();
	Vector3 lsc = ls - location;

	float a = ld.length_squared();
	float b = 2.0f * Vector3::dot(ld, lsc);
	float c = lsc.length_squared() - radiusSq;

	float delta = sqr(b) - 4.0f * a * c;

	if (delta < 0.0f)
	{
		return 2.0f;
	}

	float deltaSqrt = sqrt(delta);
	float invA2 = 1.0f / (2.0f * a);
	float t1 = (-b - deltaSqrt) * invA2;
	float t2 = (-b + deltaSqrt) * invA2;

	if (t1 <= t2)
	{
		if (t1 >= 0.0f && t2 <= 1.0f)
		{
			return t1;
		}
		if (t2 >= 0.0f && t2 <= 1.0f)
		{
			return t2;
		}
	}
	else
	{
		if (t2 >= 0.0f && t2 <= 1.0f)
		{
			return t2;
		}
		if (t1 >= 0.0f && t2 <= 1.0f)
		{
			return t1;
		}
	}
	return 2.0f;
}

float Sphere::calculate_outside_distance(Vector3 const& _loc) const
{
	return (_loc - location).length() - radius;
}

bool Sphere::does_overlap(Range3 const& _range) const
{
	Vector3 loc = get_centre();
	float rad = get_radius();

	// check if closest point of range is within sphere

	Vector3 rangeClosest(_range.x.clamp(loc.x), _range.y.clamp(loc.y), _range.z.clamp(loc.z));

	return (rangeClosest - loc).length_squared() < sqr(rad);
}

String Sphere::to_string() const
{
	return String::printf(TXT("at %S radius %.3f"), location.to_string().to_char(), radius);
}
