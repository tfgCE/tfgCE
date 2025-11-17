#include "math.h"
#include "..\io\xml.h"

#include "..\other\parserUtils.h"

bool Rotator3::load_from_xml(IO::XML::Node const * _node, tchar const * _pitchAttr, tchar const * _yawAttr, tchar const * _rollAttr)
{
	if (!_node)
	{
		return false;
	}
	if (_node->has_attribute(_pitchAttr) ||
		_node->has_attribute(_yawAttr) ||
		_node->has_attribute(_rollAttr))
	{
		*this = Rotator3::zero;
	}
	pitch = _node->get_float_attribute(_pitchAttr, pitch);
	yaw = _node->get_float_attribute(_yawAttr, yaw);
	roll = _node->get_float_attribute(_rollAttr, roll);
	return true;
}

bool Rotator3::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _pitchAttr, tchar const * _yawAttr, tchar const * _rollAttr)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _pitchAttr, _yawAttr, _rollAttr);
	}
	return false;
}

bool Rotator3::load_from_string(String const & _string)
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

bool Rotator3::save_to_xml(IO::XML::Node * _node, tchar const * _pitchAttr, tchar const * _yawAttr, tchar const * _rollAttr) const
{
	bool result = true;

	_node->set_float_attribute(_pitchAttr, pitch);
	_node->set_float_attribute(_yawAttr, yaw);
	_node->set_float_attribute(_rollAttr, roll);

	return result;
}

bool Rotator3::save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _pitchAttr, tchar const * _yawAttr, tchar const * _rollAttr) const
{
	return save_to_xml(_node->add_node(_childNode), _pitchAttr, _yawAttr, _rollAttr);
}


