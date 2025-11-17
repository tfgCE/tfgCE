#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\collision\checkCollisionContext.h"

bool Plane::check_segment(REF_ Segment & _segment, REF_ Vector3 & _hitNormal, Collision::CheckCollisionContext const* _context, float _increaseSize) const
{
	an_assert(! get_normal().is_almost_zero());
	float startInFront = get_in_front(_segment.get_start()) - _increaseSize;
	float endInFront = get_in_front(_segment.get_end()) - _increaseSize;

	if (startInFront * endInFront <= 0.0f)
	{
		Vector3 hitNormal;
		if (_context && _context->should_ignore_reversed_normal())
		{
			hitNormal = get_normal();
		}
		else
		{
			hitNormal = startInFront > 0.0f || (startInFront == 0.0f && endInFront < 0.0f) ? get_normal() : -get_normal();
		}
		if ((!_context || !_context->should_ignore_reversed_normal()) ||
			Vector3::dot(hitNormal, _segment.get_start_to_end()) < 0.0f)
		{
			_hitNormal = hitNormal;
			if (startInFront != endInFront)
			{
				float t = (0.0f - startInFront) / (endInFront - startInFront);
				an_assert(t >= -0.0001f && t <= 1.0001f, TXT("t outside bounds %.6f"), t);
				return _segment.collide_at(clamp(t, 0.0f, 1.0f));
			}
			else
			{
				return _segment.collide_at(_segment.get_start_t());
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		// both points on one side
		return false;
	}
}

float Plane::calc_intersection_t(Segment const & _segment) const
{
	float startInFront = get_in_front(_segment.get_start());
	float endInFront = get_in_front(_segment.get_end());

	if (startInFront * endInFront <= 0.0f)
	{
		if (startInFront != endInFront)
		{
			float t = (0.0f - startInFront) / (endInFront - startInFront);
			an_assert(t >= 0.0f && t <= 1.0f);
			return t;
		}
		else
		{
			return _segment.get_start_t();
		}
	}
	else
	{
		return 2.0f;
	}
}
bool Plane::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	bool result = true;

	normal.x = _node->get_float_attribute(TXT("x"), normal.x);
	normal.y = _node->get_float_attribute(TXT("y"), normal.y);
	normal.z = _node->get_float_attribute(TXT("z"), normal.z);
	if (_node->has_attribute(TXT("d")))
	{
		anchor = -normal * _node->get_float_attribute(TXT("d"), 0.0f);
	}

	if (_node->first_child_named(TXT("normal")) ||
		_node->first_child_named(TXT("location")))
	{
		// zero to load it properly
		Vector3 normal = _node->first_child_named(TXT("normal")) ? Vector3::zero : get_normal();
		Vector3 location = _node->first_child_named(TXT("location")) ? Vector3::zero : get_anchor();

		normal.load_from_xml_child_node(_node, TXT("normal"));
		location.load_from_xml_child_node(_node, TXT("location"));

		if (normal != get_normal() ||
			location != get_anchor())
		{
			set(normal, location);
		}
	}

	int vertexIdx = 0;
	Vector3 vertices[3];
	for_every(vertexNode, _node->children_named(TXT("vertex")))
	{
		if (vertexIdx < 3)
		{
			if (vertices[vertexIdx].load_from_xml(vertexNode))
			{
				++vertexIdx;
			}
			else
			{
				error_loading_xml(vertexNode, TXT("error loading vertex for plane"));
				result = false;
			}
		}
		else
		{
			vertexIdx = 5;
			break;
		}
	}

	if (vertexIdx == 3)
	{
		set(vertices[0], vertices[1], vertices[2]);
	}
	else if (vertexIdx > 0)
	{
		error_loading_xml(_node, TXT("incorrect number of vertices!"));
		result = false;
	}

	return result;
}

bool Plane::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child);
	}
	return false;
}

Vector3 Plane::intersect(Plane const & _a, Plane const & _b, Plane const & _c)
{
	Matrix33 matrix;
	matrix.m00 = _a.normal.x;
	matrix.m01 = _a.normal.y;
	matrix.m02 = _a.normal.z;
	matrix.m10 = _b.normal.x;
	matrix.m11 = _b.normal.y;
	matrix.m12 = _b.normal.z;
	matrix.m20 = _c.normal.x;
	matrix.m21 = _c.normal.y;
	matrix.m22 = _c.normal.z;

	Vector3 d(-_a.calculate_d(), -_b.calculate_d(), -_c.calculate_d());

	an_assert(matrix.det() != 0.0f, TXT("those planes won't intersect!"));

	Vector3 i = matrix.full_inverted().to_local(d);
	return i;
}

bool Plane::can_intersect(Plane const & _a, Plane const & _b, Plane const & _c)
{
	Matrix33 matrix;
	matrix.m00 = _a.normal.x;
	matrix.m01 = _a.normal.y;
	matrix.m02 = _a.normal.z;
	matrix.m10 = _b.normal.x;
	matrix.m11 = _b.normal.y;
	matrix.m12 = _b.normal.z;
	matrix.m20 = _c.normal.x;
	matrix.m21 = _c.normal.y;
	matrix.m22 = _c.normal.z;

	return matrix.det() != 0.0f;
}