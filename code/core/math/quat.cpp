#include "math.h"
#include "..\io\xml.h"
#include "..\serialisation\serialiser.h"

bool Quat::load_from_xml_as_rotator(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	bool result = true;
	
	Rotator3 rotation = to_rotator();
	result &= rotation.load_from_xml(_node);
	*this = rotation.to_quat();
	
	return result;
}

bool Quat::load_from_xml_child_node_as_rotator(IO::XML::Node const * _node, tchar const * _childNode)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml_as_rotator(child);
	}
	return false;
}

bool Quat::save_to_xml_as_rotator(IO::XML::Node * _node) const
{
	return to_rotator().save_to_xml(_node);
}

bool Quat::save_to_xml_child_node_as_rotator(IO::XML::Node * _node, tchar const * _childNode) const
{
	return save_to_xml_as_rotator(_node->add_node(_childNode));
}

bool Quat::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, x);
	result &= serialise_data(_serialiser, y);
	result &= serialise_data(_serialiser, z);
	result &= serialise_data(_serialiser, w);
	return result;
}
