#include "math.h"
#include "..\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

bool Matrix44::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (auto* ch = _node->first_child_named(TXT("asTransform")))
	{
		Transform t = Transform::identity;
		t.load_from_xml(ch);
		*this = t.to_matrix();
	}
	if (_node->first_child_named(TXT("xAxis")))
	{
		Vector4 axis(m00, m01, m02, m03);
		axis.load_from_xml_child_node(_node, TXT("xAxis"));
		m00 = axis.x;
		m01 = axis.y;
		m02 = axis.z;
		m03 = axis.w;
	}
	if (_node->first_child_named(TXT("yAxis")))
	{
		Vector4 axis(m10, m11, m12, m13);
		axis.load_from_xml_child_node(_node, TXT("yAxis"));
		m10 = axis.x;
		m11 = axis.y;
		m12 = axis.z;
		m13 = axis.w;
	}
	if (_node->first_child_named(TXT("zAxis")))
	{
		Vector4 axis(m20, m21, m22, m23);
		axis.load_from_xml_child_node(_node, TXT("zAxis"));
		m20 = axis.x;
		m21 = axis.y;
		m22 = axis.z;
		m23 = axis.w;
	}
	{
		Vector4 axis(m30, m31, m32, m33);
		axis.load_from_xml_child_node(_node, TXT("translation"));
		axis.load_from_xml_child_node(_node, TXT("wAxis"));
		m30 = axis.x;
		m31 = axis.y;
		m32 = axis.z;
		m33 = axis.w;
	}
	if (auto* ch = _node->first_child_named(TXT("scale")))
	{
		if (ch->has_attribute(TXT("x")) ||
			ch->has_attribute(TXT("y")) ||
			ch->has_attribute(TXT("z")))
		{
			Vector3 scale = Vector3::one;
			scale.load_from_xml_child_node(_node, TXT("scale"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
			m00 *= scale.x;
			m01 *= scale.y;
			m02 *= scale.z;
			m10 *= scale.x;
			m11 *= scale.y;
			m12 *= scale.z;
			m20 *= scale.x;
			m21 *= scale.y;
			m22 *= scale.z;
		}
		else
		{
			float scale = ch->get_float(1.0f);
			m00 *= scale;
			m01 *= scale;
			m02 *= scale;
			m10 *= scale;
			m11 *= scale;
			m12 *= scale;
			m20 *= scale;
			m21 *= scale;
			m22 *= scale;
		}
	}
	if (_node->get_bool_attribute_or_from_child_presence(TXT("invert")))
	{
		*this = inverted();
	}
	return true;
}

bool Matrix44::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode)
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

void Matrix44::fix_axes_forward_up_right()
{
	// get axes
	Vector3 x = get_x_axis();
	Vector3 y = get_y_axis();
	Vector3 z = get_z_axis();

	float xLen = x.length();
	float yLen = y.length();
	float zLen = z.length();

	// y/forward
	y = y.normal();

	// z/up
	z = (z - y * Vector3::dot(y, z)).normal();
	
	// x/right
	x = Vector3::cross(y, z);

	// apply scales
	set_x_axis(x * xLen);
	set_y_axis(y * yLen);
	set_z_axis(z * zLen);
}

void Matrix44::log(LogInfoContext & _log) const
{
	todo_note(TXT("implement_ matrix44::log to show separate scales"));
	if (is_orthogonal())
	{
		_log.log(TXT("as transform"));
		LOG_INDENT(_log);
		to_transform().log(_log);
	}
	{
		_log.log(TXT("as matrix"));
		LOG_INDENT(_log);
		for (int i = 0; i < 4; ++i)
		{
			_log.log(TXT(" %7.4f %7.4f %7.4f %7.4f"), m[i][0], m[i][1], m[i][2], m[i][3]);
		}
	}
}
