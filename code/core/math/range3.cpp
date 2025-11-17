#include "math.h"

#include "..\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

String Range3::to_string(int _decimals, tchar const * _suffix) const
{
	return String::printf(TXT("x:%S  y:%S  z:%S"), x.to_string(_decimals, _suffix).to_char(), y.to_string(_decimals, _suffix).to_char(), z.to_string(_decimals, _suffix).to_char());
}

bool Range3::load_from_xml(IO::XML::Node const * _node, tchar const * _attrX, tchar const * _attrY, tchar const * _attrZ)
{
	return x.load_from_xml(_node, _attrX) &&
		   y.load_from_xml(_node, _attrY) &&
		   z.load_from_xml(_node, _attrZ);
}

bool Range3::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		result &= x.load_from_xml(node, _xAttr);
		result &= y.load_from_xml(node, _yAttr);
		result &= z.load_from_xml(node, _zAttr);
	}

	return result;
}

bool Range3::load_from_string(String const & _string)
{
	bool result = true;

	List<String> tokens;
	_string.split(String(TXT(";")), tokens);
	if (tokens.get_size() == 3)
	{
		x.load_from_string(tokens[0]);
		y.load_from_string(tokens[1]);
		z.load_from_string(tokens[2]);
	}
	else if (tokens.get_size() == 1)
	{
		if (!tokens[0].is_empty())
		{
			x.load_from_string(tokens[0]);
			y = x;
			z = x;
		}
	}
	else
	{
		error(TXT("could not parse \"%S\" to Range3"), _string.to_char());
		result = false;
	}

	return result;
}

bool Range3::intersects_triangle(Vector3 const& _a, Vector3 const& _b, Vector3 const& _c) const
{
	if (does_contain(_a) ||
		does_contain(_b) ||
		does_contain(_c))
	{
		return true;
	}

	if ((_a.x < x.min && _b.x < x.min && _c.x < x.min) ||
		(_a.x > x.max && _b.x > x.max && _c.x > x.max) ||
		(_a.y < y.min && _b.y < y.min && _c.y < y.min) ||
		(_a.y > y.max && _b.y > y.max && _c.y > y.max) ||
		(_a.z < z.min && _b.z < z.min && _c.z < z.min) ||
		(_a.z > z.max && _b.z > z.max && _c.z > z.max))
	{
		return false;
	}

	// if any triangle edge will cut through range, they intersect
	if (Segment(_a, _b).check_against_range3(*this) ||
		Segment(_b, _c).check_against_range3(*this) ||
		Segment(_c, _a).check_against_range3(*this))
	{
		return true;
	}

	// if any range side cuts through triangle, they intersect
	Vector3 tempNormal;
	if (Segment(Vector3(x.min, y.min, z.min), Vector3(x.max, y.min, z.min)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.min, y.min, z.max), Vector3(x.max, y.min, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.min, y.max, z.min), Vector3(x.max, y.max, z.min)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.min, y.max, z.max), Vector3(x.max, y.max, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.min, y.min, z.min), Vector3(x.min, y.max, z.min)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.min, y.min, z.max), Vector3(x.min, y.max, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.max, y.min, z.min), Vector3(x.max, y.max, z.min)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.max, y.min, z.max), Vector3(x.max, y.max, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.min, y.min, z.min), Vector3(x.min, y.min, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.min, y.max, z.min), Vector3(x.min, y.max, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.max, y.min, z.min), Vector3(x.max, y.min, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal) ||
		Segment(Vector3(x.max, y.max, z.min), Vector3(x.max, y.max, z.max)).check_against_triangle(_a, _b, _c, OUT_ tempNormal))
	{
		return true;
	}
	return false;
}
