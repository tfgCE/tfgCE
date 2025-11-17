#include "math.h"

#include "..\io\xml.h"

bool RangeInt3::load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr)
{
	bool result = true;
	
	result &= x.load_from_xml(_node, _xAttr);
	result &= y.load_from_xml(_node, _yAttr);
	result &= z.load_from_xml(_node, _zAttr);

	return result;
}

bool RangeInt3::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr, tchar const * _zAttr)
{
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _xAttr, _yAttr, _zAttr);
	}
	else
	{
		return true;
	}
}

bool RangeInt3::save_to_xml(IO::XML::Node* _node, tchar const* _xAttr, tchar const* _yAttr, tchar const* _zAttr) const
{
	bool result = true;

	_node->set_attribute(_xAttr, x.to_parsable_string());
	_node->set_attribute(_yAttr, y.to_parsable_string());
	_node->set_attribute(_zAttr, z.to_parsable_string());

	return result;
}

bool RangeInt3::save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childNode, tchar const* _xAttr, tchar const* _yAttr, tchar const* _zAttr) const
{
	bool result = true;

	if (auto* chnode = _node->add_node(_childNode))
	{
		result &= save_to_xml(chnode, _xAttr, _yAttr, _zAttr);
	}

	return result;
}
