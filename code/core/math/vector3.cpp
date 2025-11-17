#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\serialisation\serialiser.h"

#include "..\types\string.h"
#include "..\containers\list.h"

bool Vector3::load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr, tchar const * _axisAttr, bool _keepExistingValues)
{
	if (!_node)
	{
		return false;
	}
	_xAttr = _xAttr ? _xAttr : TXT("x");
	_yAttr = _yAttr ? _yAttr : TXT("y");
	_zAttr = _zAttr ? _zAttr : TXT("z");
	if (!_keepExistingValues &&
		(_node->has_attribute(_xAttr) ||
		 _node->has_attribute(_yAttr) ||
		 _node->has_attribute(_zAttr)))
	{
		*this = Vector3::zero;
	}
	x = _node->get_float_attribute(_xAttr, x);
	y = _node->get_float_attribute(_yAttr, y);
	z = _node->get_float_attribute(_zAttr, z);

	String const & axis = _node->get_string_attribute(_axisAttr);
	if (axis == TXT("x")) *this = Vector3::xAxis;
	if (axis == TXT("y")) *this = Vector3::yAxis;
	if (axis == TXT("z")) *this = Vector3::zAxis;
	if (axis == TXT("-x")) *this = -Vector3::xAxis;
	if (axis == TXT("-y")) *this = -Vector3::yAxis;
	if (axis == TXT("-z")) *this = -Vector3::zAxis;

	return true;
}

bool Vector3::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr, tchar const * _axisAttr, bool _keepExistingValues)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _xAttr, _yAttr, _zAttr, _axisAttr, _keepExistingValues);
	}
	return false;
}

bool Vector3::load_from_string(String const & _string)
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
		float & e = access_element(eIdx);
		e = ParserUtils::parse_float(*token, e);
		++eIdx;
		if (eIdx >= 3)
		{
			return true;
		}
	}
	return false;
}

bool Vector3::save_to_xml(IO::XML::Node * _node, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr) const
{
	bool result = true;

	_node->set_float_attribute(_xAttr, x);
	_node->set_float_attribute(_yAttr, y);
	_node->set_float_attribute(_zAttr, z);

	return result;
}

bool Vector3::save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr) const
{
	return save_to_xml(_node->add_node(_childNode), _xAttr, _yAttr, _zAttr);
}

Vector3 Vector3::load_axis_from_string(String const & _string)
{
	Vector3 result = zero;
	float value = _string.has_prefix(TXT("-")) ? -1.0f : 1.0f;
	if (_string.has_suffix(TXT("x")))
	{
		result.x = value;
	}
	else if (_string.has_suffix(TXT("y")))
	{
		result.y = value;
	}
	else if (_string.has_suffix(TXT("z")))
	{
		result.z = value;
	}
	else
	{
		error(TXT("no valid axis defined"));
	}
	return result;
}

bool Vector3::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, x);
	result &= serialise_data(_serialiser, y);
	result &= serialise_data(_serialiser, z);
	return result;
}

Vector3 Vector3::get_any_perpendicular() const
{
	Vector3 normalised = normal();

	Vector3 perpCandidate = abs(normalised.z) > 0.7f ? Vector3::xAxis : Vector3::zAxis;

	Vector3 result = Vector3::cross(normalised, Vector3::cross(perpCandidate, normalised)).normal();
	
	return result;
}
