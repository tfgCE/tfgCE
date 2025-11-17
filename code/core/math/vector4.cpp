#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\types\string.h"
#include "..\containers\list.h"

bool Vector4::load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr, tchar const * _wAttr, bool _keepExistingValues)
{
	if (!_node)
	{
		return false;
	}
	_xAttr = _xAttr ? _xAttr : TXT("x");
	_yAttr = _yAttr ? _yAttr : TXT("y");
	_zAttr = _zAttr ? _zAttr : TXT("z");
	_wAttr = _wAttr ? _wAttr : TXT("w");
	if (!_keepExistingValues &&
		(_node->has_attribute(_xAttr) ||
		 _node->has_attribute(_yAttr) ||
		 _node->has_attribute(_zAttr) ||
		 _node->has_attribute(_wAttr)))
	{
		*this = Vector4::zero;
	}
	x = _node->get_float_attribute(_xAttr, x);
	y = _node->get_float_attribute(_yAttr, y);
	z = _node->get_float_attribute(_zAttr, z);
	w = _node->get_float_attribute(_wAttr, w);
	return true;
}

bool Vector4::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr, tchar const * _wAttr, bool _keepExistingValues)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _xAttr, _yAttr, _zAttr, _wAttr, _keepExistingValues);
	}
	return false;
}

bool Vector4::load_from_string(String const & _string)
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
