#include "math.h"
#include "..\io\xml.h"

bool Matrix33::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (_node->first_child_named(TXT("xAxis")))
	{
		Vector3 axis(m00, m01, m02);
		axis.load_from_xml_child_node(_node, TXT("xAxis"));
		m00 = axis.x;
		m01 = axis.y;
		m02 = axis.z;
	}
	if (_node->first_child_named(TXT("yAxis")))
	{
		Vector3 axis(m10, m11, m12);
		axis.load_from_xml_child_node(_node, TXT("yAxis"));
		m10 = axis.x;
		m11 = axis.y;
		m12 = axis.z;
	}
	if (_node->first_child_named(TXT("zAxis")))
	{
		Vector3 axis(m20, m21, m22);
		axis.load_from_xml_child_node(_node, TXT("zAxis"));
		m20 = axis.x;
		m21 = axis.y;
		m22 = axis.z;
	}
	return true;
}

bool Matrix33::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode)
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

Vector3 const & Matrix33__find_free_axis(Vector3 const & _a, Vector3 const & _b)
{
	int at = 0;
	if (at == 0 &&
		(_a.x != 0.0f ||
		 _b.x != 0.0f)) ++at;
	if (at == 1 &&
		(_a.y != 0.0f ||
		 _b.y != 0.0f)) ++at;
	if (at == 2 &&
		(_a.z != 0.0f ||
		 _b.z != 0.0f)) ++at;
	if (at == 0)
	{
		return Vector3::xAxis;
	}
	else if (at == 1)
	{
		return Vector3::yAxis;
	}
	else if (at == 2)
	{
		return Vector3::zAxis;
	}
	else
	{
		return Vector3::zero;
	}
}

bool Matrix33::load_axes_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	*this = zero;
	// load all provided
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("x")))
	{
		set_x_axis(Vector3::load_axis_from_string(attr->get_as_string()));
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("y")))
	{
		set_y_axis(Vector3::load_axis_from_string(attr->get_as_string()));
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("z")))
	{
		set_z_axis(Vector3::load_axis_from_string(attr->get_as_string()));
	}
	// fill other axes
	if (get_x_axis().is_zero())
	{
		set_x_axis(Matrix33__find_free_axis(get_y_axis(), get_z_axis()));
	}
	if (get_y_axis().is_zero())
	{
		set_y_axis(Matrix33__find_free_axis(get_x_axis(), get_z_axis()));
	}
	if (get_z_axis().is_zero())
	{
		set_z_axis(Vector3::cross(get_y_axis(), get_x_axis()));
	}
	return true;
}

bool Matrix33::build_orthogonal_from_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _forward, Vector3 const & _right, Vector3 const & _up)
{
	an_assert(_firstAxis != _secondAxis);

	Vector3 yAxis = _forward;
	Vector3 xAxis = _right;
	Vector3 zAxis = _up;

	Vector3 & first = _firstAxis == Axis::X ? xAxis : (_firstAxis == Axis::Y ? yAxis : zAxis);
	Vector3 & second = _secondAxis == Axis::X ? xAxis : (_secondAxis == Axis::Y ? yAxis : zAxis);

	return build_orthogonal_from_two_axes(_firstAxis, _secondAxis, first, second);
}

bool Matrix33::build_orthogonal_from_two_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _first, Vector3 const & _second)
{
	an_assert(_firstAxis != _secondAxis);

	if (_first.is_zero() || _second.is_zero())
	{
		*this = Matrix33::identity;
		return false;
	}

	Vector3 yAxis = Vector3::zero;
	Vector3 xAxis = Vector3::zero;
	Vector3 zAxis = Vector3::zero;

	Axis::Type thirdAxis = _firstAxis == Axis::X ? (_secondAxis == Axis::Y ? Axis::Z /*xy->z*/ : Axis::Y /*xz->y*/) :
		(_firstAxis == Axis::Y ? (_secondAxis == Axis::X ? Axis::Z /*yx->z*/ : Axis::X /*yz->x*/) :
		(_secondAxis == Axis::X ? Axis::Y /*zx->y*/ : Axis::X /*zy->x*/));
	Vector3 & first = _firstAxis == Axis::X ? xAxis : (_firstAxis == Axis::Y ? yAxis : zAxis);
	Vector3 & second = _secondAxis == Axis::X ? xAxis : (_secondAxis == Axis::Y ? yAxis : zAxis);
	Vector3 & third = thirdAxis == Axis::X ? xAxis : (thirdAxis == Axis::Y ? yAxis : zAxis);

	first = _first;
	second = _second;

	first.normalise();
	if (thirdAxis == Axis::X)
	{
		third = Vector3::cross(yAxis, zAxis);
	}
	else if (thirdAxis == Axis::Y)
	{
		third = Vector3::cross(zAxis, xAxis);
	}
	else if (thirdAxis == Axis::Z)
	{
		third = Vector3::cross(xAxis, yAxis);
	}
	third.normalise();

	// recalculate second
	if (_secondAxis == Axis::X)
	{
		second = Vector3::cross(yAxis, zAxis);
	}
	else if (_secondAxis == Axis::Y)
	{
		second = Vector3::cross(zAxis, xAxis);
	}
	else if (_secondAxis == Axis::Z)
	{
		second = Vector3::cross(xAxis, yAxis);
	}
	second.normalise();

	set_x_axis(xAxis);
	set_y_axis(yAxis);
	set_z_axis(zAxis);

	return true;
}

void Matrix33::log(LogInfoContext & _log) const
{
	todo_note(TXT("implement_ matrix33::log to show separate scales"));
	_log.log(TXT("orientation : %S"), to_quat().to_rotator().to_string().to_char());
	_log.log(TXT("scale : ??"));
}
