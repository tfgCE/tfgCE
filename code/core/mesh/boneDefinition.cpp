#include "boneDefinition.h"

#include "..\serialisation\serialiser.h"

using namespace Meshes;

BoneDefinition::BoneDefinition()
: parentIndex(NONE)
, placementMS(Transform::identity)
{
}

Name BoneDefinition::load_name_from_xml(IO::XML::Node const * _node)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("name")))
	{
		return attr->get_as_name();
	}
	else
	{
		return Name::invalid();
	}
}

Name BoneDefinition::load_parent_name_from_xml(IO::XML::Node const * _node)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("parent")))
	{
		return attr->get_as_name();
	}
	else
	{
		return Name::invalid();
	}
}

bool BoneDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	name = load_name_from_xml(_node);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("\"name\" node missing"));
		result = false;
	}

	Name loadedParentName = load_parent_name_from_xml(_node);
	if (loadedParentName.is_valid())
	{
		parentName = loadedParentName;
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("placementMS")))
	{
		placementMS.load_from_xml(node);
	}
	if (IO::XML::Node const * node = _node->first_child_named(TXT("placement")))
	{
		placementMS.load_from_xml(node);
	}

	return result;
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool BoneDefinition::serialise(Serialiser & _serialiser)
{
	uint8 version = CURRENT_VERSION;
	serialise_data(_serialiser, version);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid socket definition version"));
		return false;
	}
	serialise_data(_serialiser, name);
	serialise_data(_serialiser, parentName);
	serialise_data(_serialiser, placementMS);
	if (_serialiser.is_reading())
	{
		parentIndex = NONE;
	}
	return true;
}
