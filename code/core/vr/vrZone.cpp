#include "vrZone.h"

#include "..\debug\debugRenderer.h"
#include "..\debug\debugVisualiser.h"

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

using namespace VR;

void Zone::clear()
{
	points.clear();
	edges.clear();
}

void Zone::add(Vector2 const & _point)
{
	points.push_back(_point);
	edges.clear();
}

void Zone::build()
{
	edges.clear();
	edges.make_space_for(points.get_size());
	for_every(point, points)
	{
		Vector2 nextPoint = points[(for_everys_index(point) + 1) % points.get_size()];
		Edge edge;
		edge.start = *point;
		edge.end = nextPoint;
		edge.dir = (nextPoint - *point).normal();
		edge.inside = edge.dir.rotated_right();
		edge.length = (nextPoint - *point).length();
		edges.push_back(edge);
	}
}

#ifdef AN_DEBUG_RENDERER
void Zone::debug_render(Colour const & _colour, Transform const & _placement) const
{
	debug_push_transform(_placement);
	for_every(point, points)
	{
		Vector2 nextPoint = points[(for_everys_index(point) + 1) % points.get_size()];
		debug_draw_line(true, _colour, point->to_vector3(), nextPoint.to_vector3());
	}
	debug_pop_transform();
}
#endif

void Zone::debug_visualise(DebugVisualiser* _dv, Colour const & _colour) const
{
	for_every(edge, edges)
	{
		_dv->add_line(edge->start, edge->end, _colour);
	}
}

Zone Zone::to_world_of(Transform const & _placement) const
{
	Zone result = *this;

	for_every(point, result.points)
	{
		*point = _placement.location_to_world(point->to_vector3()).to_vector2();
	}

	result.build();
	return result;
}

Zone Zone::to_local_of(Transform const & _placement) const
{
	Zone result = *this;

	for_every(point, result.points)
	{
		*point = _placement.location_to_world(point->to_vector3()).to_vector2();
	}

	result.build();
	return result;
}

void Zone::be_range(Range2 const& _range)
{
	clear();
	add(Vector2(_range.x.min, _range.y.min));
	add(Vector2(_range.x.min, _range.y.max));
	add(Vector2(_range.x.max, _range.y.max));
	add(Vector2(_range.x.max, _range.y.min));
	build();
}

void Zone::be_rect(Vector2 const & _rectSize, Vector2 const & _offset)
{
	clear();
	Vector2 halfRectSize = _rectSize * 0.5f;
	add(Vector2(-1.0f,  1.0f) * halfRectSize + _offset);
	add(Vector2( 1.0f,  1.0f) * halfRectSize + _offset);
	add(Vector2( 1.0f, -1.0f) * halfRectSize + _offset);
	add(Vector2(-1.0f, -1.0f) * halfRectSize + _offset);
	build();
}

void Zone::place_at(Vector2 const & _at, Vector2 const & _dir)
{
	Matrix44 mat = look_at_matrix(_at.to_vector3(), (_at + _dir).to_vector3(), Vector3::zAxis);
	*this = to_world_of(mat.to_transform());
}

bool Zone::does_intersect_with(Zone const & _zone) const
{
	if (does_contain(_zone, -0.01f) || _zone.does_contain(*this, -0.01f))
	{
		return true;
	}

	for_every(edge, edges)
	{
		Segment2 seg(edge->start, edge->end);
		for_every(oEdge, _zone.edges)
		{
			Segment2 oSeg(oEdge->start, oEdge->end);
			if (seg.does_intersect_with(oSeg))
			{
				return true;
			}
		}
	}

	return false;
}

bool Zone::does_intersect_with(Segment2 const & _seg) const
{
	if (does_contain(_seg.get_start()) ||
		does_contain(_seg.get_end()))
	{
		return true;
	}

	for_every(edge, edges)
	{
		Segment2 seg(edge->start, edge->end);
		if (seg.does_intersect_with(_seg))
		{
			return true;
		}
	}

	return false;
}

bool Zone::does_contain(Zone const & _zone, float _inside) const
{
	for_every(p, _zone.points)
	{
		if (!does_contain(*p, _inside))
		{
			return false;
		}
	}
	return true;
}

bool Zone::does_contain(Vector2 const & _p, float _inside) const
{
	Vector2 corrIntersection = get_inside(_p, _inside);
	return (corrIntersection - _p).is_almost_zero();
}

Vector2 Zone::get_inside(Vector2 const & _p, float _inside) const
{
	int triesLeft = 10;
	Vector2 p = _p;
	while (triesLeft--)
	{
		bool isOk = true;
		for_every(edge, edges)
		{
			float in = Vector2::dot(p - edge->start, edge->inside);
			float shouldBeAt = max(in, _inside);
			float diff = shouldBeAt - in;
			if (diff != 0.0f)
			{
				p += edge->inside * diff;
				isOk = false;
			}
		}
		if (isOk)
		{
			break;
		}
	}
	return p;
}

bool Zone::calc_intersection(Vector2 const & _a, Vector2 const & _dir, Vector2 & _intersection, Optional<float> const & _inside, Optional<float> const& _halfWidth) const
{
	float intersectT = calc_intersection_t(_a, _dir, _intersection, _inside);
	if (_halfWidth.get(0.0f) > 0.0f)
	{
		float const halfWidth = _halfWidth.get(0.0f);
		Vector2 const right = _dir.rotated_right();
		for (int side = -1; side <= 1; side += 2)
		{
			Vector2 intersectionO;
			Vector2 offsetSide = right * (halfWidth * ((float)side));
			float intersectTO = calc_intersection_t(_a + offsetSide, _dir, intersectionO, _inside);
			if (intersectTO < intersectT)
			{
				intersectT = intersectTO;
				_intersection = intersectionO - offsetSide;
			}
		}
	}
	return intersectT >= 0.0f && does_contain(_intersection, _inside.get(0.0f));
}

float Zone::calc_intersection_t(Vector2 const& _a, Vector2 const& _dir, Vector2& _intersection, Optional<float> const& _inside) const
{
	float intersectT = -1.0f;
	float inside = _inside.get(0.0f);
	for_every(edge, edges)
	{
		Vector2 intersection;
		Vector2 edgeStart = edge->start + edge->inside * inside;
		if (Vector2::calc_intersection(edgeStart, edge->dir, _a, _dir, intersection))
		{
			float alongT = Vector2::dot(intersection - _a, _dir);
			if (alongT >= 0.0f && // in right direction
				(intersectT < 0.0f || alongT < intersectT)) // closer than last one
			{
				float alongEdge = Vector2::dot(intersection - edgeStart, edge->dir);
				float const threshold = 0.001f;
				if (alongEdge >= -threshold && alongEdge <= edge->length + threshold) // on the edge
				{
					_intersection = intersection;
					intersectT = alongT;
				}
			}
		}
	}
	return intersectT;
}

Range2 Zone::to_range2() const
{
	Range2 result = Range2::empty;
	for_every(edge, edges)
	{
		result.include(edge->start);
	}
	return result;
}
