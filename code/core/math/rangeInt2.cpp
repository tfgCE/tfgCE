#include "math.h"

#include "..\io\xml.h"

bool RangeInt2::load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr, tchar const * _yAttr)
{
	bool result = true;
	
	result &= x.load_from_xml(_node, _xAttr);
	result &= y.load_from_xml(_node, _yAttr);

	return result;
}

bool RangeInt2::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr)
{
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _xAttr, _yAttr);
	}
	else
	{
		return true;
	}
}

bool RangeInt2::save_to_xml(IO::XML::Node* _node, tchar const* _xAttr, tchar const* _yAttr) const
{
	bool result = true;

	_node->set_attribute(_xAttr, x.to_parsable_string());
	_node->set_attribute(_yAttr, y.to_parsable_string());

	return result;
}

bool RangeInt2::save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childNode, tchar const* _xAttr, tchar const* _yAttr) const
{
	bool result = true;

	if (auto* chnode = _node->add_node(_childNode))
	{
		result &= save_to_xml(chnode, _xAttr, _yAttr);
	}

	return result;
}
