#include "math.h"
#include "..\io\xml.h"

#include "..\containers\arrayStack.h"
#include "..\debug\debugRenderer.h"
#include "..\debug\debugVisualiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

ConvexPolygon::ConvexPolygon()
{
}

ConvexPolygon::ConvexPolygon(ConvexPolygon const & _other)
{
	copy_from(_other);
}

ConvexPolygon::~ConvexPolygon()
{
}

void ConvexPolygon::clear_keep_points()
{
	edges.clear();
}

void ConvexPolygon::clear()
{
	clear_keep_points();
	points.clear();
	range = Range2::empty;
}


ConvexPolygon const & ConvexPolygon::operator = (ConvexPolygon const & _other)
{
	copy_from(_other);
	return *this;
}

void ConvexPolygon::copy_from(ConvexPolygon const & _other)
{
	copy_from_dont_build(_other);
	if (points.get_size() >= 3)
	{
		build();
	}
}

void ConvexPolygon::copy_from_dont_build(ConvexPolygon const & _other)
{
	points = _other.points;
	range = _other.range;
	centre = _other.centre;
	edges.clear();
}

bool ConvexPolygon::is_inside(Vector2 const& _point) const
{
	for_every(e, edges)
	{
		float isIn = Vector2::dot(_point - e->a, e->in);
		if (isIn < EPSILON)
		{
			return false;
		}
	}
	return true;
}

bool ConvexPolygon::does_share_edge_with_triangle(Vector2 const& _a, Vector2 const& _b, Vector2 const& _c, OPTIONAL_ OUT_ Vector2 * _other) const
{
	for_every(e, edges)
	{
		if (e->a == _a && e->b == _c) { assign_optional_out_param(_other, _b); return true; }
		if (e->a == _b && e->b == _a) { assign_optional_out_param(_other, _c); return true; }
		if (e->a == _c && e->b == _b) { assign_optional_out_param(_other, _a); return true; }
	}
	return false;
}

bool ConvexPolygon::can_add_to_remain_convex(Vector2 const& _point) const
{
	for_every(p, points)
	{
		if (*p == _point)
		{
			return true;
		}
	}
	todo_note(TXT("make it not create everything?"));
	ConvexPolygon ifAdded;
	ifAdded.copy_from_dont_build(*this); // dont build edges yet
	ifAdded.add(_point);
	if (!ifAdded.internal_build(true))
	{
		return false;
	}
	for_every(p, points)
	{
		if (ifAdded.is_inside(*p))
		{
			return false;
		}
	}
	return true;
}

void ConvexPolygon::add(Vector2 const & _point)
{
	an_assert(edges.get_size() == 0, TXT("convex polygon should be not build when adding new point"));
	if (!points.does_contain(_point))
	{
		points.push_back(_point);
	}
	range.include(_point);
}

void ConvexPolygon::build()
{
	internal_build(false);
}

bool ConvexPolygon::internal_build(bool _mayFail)
{
	an_assert(edges.get_size() == 0, TXT("convex polygon should be not build when building"));
	an_assert(points.get_size() >= 3, TXT("there should be at least three points in convex polygon"));

	// calculate centre
	centre = Vector2::zero;
	for_every(point, points)
	{
		centre += *point;
	}
	centre /= float(points.get_size());

	// remove points that lay on line (if we assume they are clockwise)
	for (int i = 0; i < points.get_size(); ++i)
	{
		Vector2 p = points[i];
		Vector2 dp;
		{
			int m = mod(i + 1, points.get_size());
			dp = (points[m] - p).normal();
		}
		while (points.get_size() > 3)
		{
			int m = mod(i + 1, points.get_size());
			int n = mod(i + 2, points.get_size());
			Vector2 pm = points[m];
			Vector2 pn = points[n];

			Vector2 dn = (pn - p).normal();
			if (Vector2::dot(dn, dp) < 0.999f)
			{
				// we diverted way too much
				break;
			}
			Vector2 rn = dn.rotated_right();
			float off = Vector2::dot(pm - p, rn);
			float threshold = min(0.0005f, (pn - p).length() * 0.0005f);
			if (abs(off) < threshold && (pn - p).length() > (pm - p).length())
			{
				// try next point, maybe it's co-linear too (we will guard ourselves against curves by checking dn against dp
				points.remove_at(m);
				continue;
			}
			else
			{
				break;
			}
		}
	}

	// find furthest point (as we're sure it will lay outside)
	int furthestIdx = NONE;
	float furthestDistSq = 0.0f;
	for_every(p, points)
	{
		float distSq = (*p - centre).length_squared();
		if (furthestIdx == NONE || distSq > furthestDistSq)
		{
			furthestIdx = for_everys_index(p);
			furthestDistSq = distSq;
		}
	}

	// build edges starting from the last
	// then scan points starting with the following point
	// this way we should always look forward and as we start at the furthest
	// we should eventually reach the furthest to finish the process
	int lastIdx = furthestIdx;
	int tryIdx = 0;
	int tryLimit = min(points.get_size() * points.get_size() * 2, 20000);
	while (true)
	{
		bool wereDone = false;
		++tryIdx;
		for(int i = 1; i < points.get_size(); ++ i)
		{
			int ip = mod(lastIdx + i, points.get_size());
			Vector2* p = &points[ip];
			Vector2* last = &points[lastIdx];
			{
				Edge e;
				e.a = *last;
				e.b = *p;
				e.in = (e.b - e.a).rotated_right();
				if (Vector2::dot(e.in, centre - e.a) < 0.0f)
				{
					continue;
				}
				if (e.in.is_zero())
				{
					continue;
				}

				e.in.normalise();

				e.dir = (e.b - e.a).normal();
				e.length = Vector2::dot(e.dir, e.b - e.a);

				if (e.length <= 0.0f)
				{
					continue;
				}

				bool allIn = true;
				float threshold = -0.0001f;
				/*
				if (tryIdx > points.get_size())
				{
					threshold = -0.001f;
				}
				if (tryIdx > points.get_size() * 2)
				{
					threshold = -0.001f;
				}
				*/
				for_every(p, points)
				{
					if (for_everys_index(p) != ip && for_everys_index(p) != lastIdx)
					{
						float dot = Vector2::dot(e.in, *p - e.a);
						if (dot < threshold)
						{
							allIn = false;
							break;
						}
					}
				}

				if (!allIn)
				{
					if (tryIdx > tryLimit)
					{
						if (_mayFail)
						{
							return false;
						}
#ifdef AN_DEVELOPMENT
						DebugVisualiserPtr dv(new DebugVisualiser(String::printf(TXT("can't build convex polygon"))));
						dv->activate();
						dv->start_gathering_data();
						dv->clear();
						Vector2 prevPoint = points.get_last();
						for_every(p, points)
						{
							Vector2 off1 = Vector2(0.1f, 0.1f);
							Vector2 off2 = Vector2(0.1f, -0.1f);
							dv->add_line(*p - off1, *p + off1, Colour::blue);
							dv->add_line(*p - off2, *p + off2, Colour::blue);
							prevPoint = *p;
						}
						dv->add_line(e.a, e.b, Colour::red);
						Vector2 ec = (e.a + e.b) * 0.5f;
						dv->add_line(ec, ec + e.in, Colour::red);
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
						dv->deactivate();
#endif
					}
					continue;
				}

				tryIdx = 0;
				edges.push_back(e);
				lastIdx = ip;
				wereDone = ip == furthestIdx;
				break;
			}

			if (i == points.get_size() - 1)
			{
				an_assert_log_always(false, TXT("this should not happen"));
				wereDone = true;
				break;
			}
		}

		if (wereDone)
		{
			break;
		}
	}

	return true;
}

ConvexPolygon::Edge const * ConvexPolygon::get_closest_edge(Vector2 const & _p, ConvexPolygon::Edge const ** _secondDoClosestEdge) const
{
	Edge const * closest = nullptr;
	{
		float closestDistance;
		for_every(e, edges)
		{
			float dist = e->calculate_distance(_p);
			if (!closest || dist < closestDistance)
			{
				closest = e;
				closestDistance = dist;
			}
		}
	}

	if (_secondDoClosestEdge)
	{
		auto & secondClosest = *_secondDoClosestEdge;
		secondClosest = nullptr;
		{
			float closestDistance;
			for_every(e, edges)
			{
				if (e == closest)
				{
					continue;
				}
				float dist = e->calculate_distance(_p);
				if (!secondClosest || dist < closestDistance)
				{
					secondClosest = e;
					closestDistance = dist;
				}
			}
		}
	}

	return closest;
}

Vector2 ConvexPolygon::move_to_edge(Vector2 const & _p) const
{
	Edge const * closest = get_closest_edge(_p);

	return closest->drop_to_edge(_p);
}

Optional<Vector2> ConvexPolygon::move_inside(Vector2 const & _p, float _radius) const
{
	Vector2 p = _p;
	int triesLeft = 32;
	while (triesLeft)
	{
		bool isInside = true;
		Vector2 move = Vector2::zero;
		for_every(e, edges)
		{
			float isIn = Vector2::dot(p - e->a, e->in) - _radius;
			if (isIn < 0.0f)
			{
				isInside = false;
				move += e->in * (-isIn * 0.5f + 0.05f);
			}
		}

		if (isInside)
		{
			return p;
		}

		p += move;
		--triesLeft;
	}

	return NP;
}

bool ConvexPolygon::check_collides(Vector2 const & _start, Vector2 const & _end) const
{
	Vector2 start2end = _end - _start;
	for_every(e, edges)
	{
		float sIn = Vector2::dot(e->in, _start - e->a);
		float eIn = Vector2::dot(e->in, _end - e->a);

		if (sIn * eIn < 0.0f &&
			sIn != eIn)
		{
			Vector2 c = _start + start2end * (0.0f - sIn) / (eIn - sIn);
			float alongE = Vector2::dot(e->dir, c - e->a);
			if (alongE >= 0.0f && alongE <= e->length)
			{
				return true;
			}
		}
	}

	return false;
}

//

float ConvexPolygon::Edge::calculate_distance(Vector2 const & _p) const
{
	float t = Vector2::dot(dir, _p - a) / length;
	if (t <= 0.0f)
	{
		return (_p - a).length();
	}
	else if (t >= 1.0f)
	{
		return (_p - b).length();
	}
	else
	{
		return abs(Vector2::dot(_p - a, in));
	}
}

Vector2 ConvexPolygon::Edge::get_at_pt(float _pt) const
{
	return a + (b - a) * _pt;
}

Vector2 ConvexPolygon::Edge::drop_to_edge(Vector2 const & _p) const
{
	float t = Vector2::dot(dir, _p - a) / length;
	if (t <= 0.0f)
	{
		return a;
	}
	else if (t >= 1.0f)
	{
		return b;
	}
	else
	{
		return a + dir * length * t;
	}
}
