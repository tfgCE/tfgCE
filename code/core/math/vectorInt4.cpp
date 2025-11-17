#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\types\string.h"
#include "..\containers\list.h"

bool VectorInt4::load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr, tchar const * _wAttr)
{
	if (!_node)
	{
		return false;
	}
	x = _node->get_int_attribute(_xAttr, x);
	y = _node->get_int_attribute(_yAttr, y);
	z = _node->get_int_attribute(_zAttr, z);
	w = _node->get_int_attribute(_wAttr, w);
	return true;
}

bool VectorInt4::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr, tchar const * _wAttr)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _xAttr, _yAttr, _zAttr, _wAttr);
	}
	return false;
}

bool VectorInt4::load_from_string(String const & _string)
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
		int32 & e = access_element(eIdx);
		e = ParserUtils::parse_int(*token, e);
		++eIdx;
		if (eIdx >= 3)
		{
			return true;
		}
	}
	return false;
}
