#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\serialisation\serialiser.h"

#include "..\types\string.h"
#include "..\containers\list.h"

bool VectorInt2::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, x);
	result &= serialise_data(_serialiser, y);
	return result;
}

bool VectorInt2::load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr, tchar const * _yAttr)
{
	if (!_node)
	{
		return false;
	}
	x = _node->get_int_attribute(_xAttr, x);
	y = _node->get_int_attribute(_yAttr, y);
	return true;
}

bool VectorInt2::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _xAttr, _yAttr);
	}
	return false;
}

bool VectorInt2::load_from_xml_child_or_attr(IO::XML::Node const * _node, tchar const * _attrOrChild)
{
	if (auto* attr = _node->get_attribute(_attrOrChild))
	{
		return load_from_string(attr->get_as_string());
	}
	else
	{
		return load_from_xml_child_node(_node, _attrOrChild);
	}
}

bool VectorInt2::load_from_string(String const & _string)
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
		if (eIdx >= 2)
		{
			return true;
		}
	}
	return false;
}

bool VectorInt2::save_to_xml(IO::XML::Node * _node, tchar const * _xAttr, tchar const * _yAttr) const
{
	bool result = true;

	_node->set_int_attribute(_xAttr, x);
	_node->set_int_attribute(_yAttr, y);

	return result;
}

bool VectorInt2::save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childNode, tchar const* _xAttr, tchar const* _yAttr) const
{
	return save_to_xml(_node->add_node(_childNode), _xAttr, _yAttr);
}
