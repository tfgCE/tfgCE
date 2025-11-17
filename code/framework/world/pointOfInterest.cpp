#include "pointOfInterest.h"

#include "..\library\libraryLoadingContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(delayObjectCreation);
DEFINE_STATIC_NAME(delayObjectCreationPriority);

//

using namespace Framework;

PointOfInterest::PointOfInterest()
: offset(Transform::identity)
{
}

bool PointOfInterest::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	return load_from_xml(_node, _lc, TXT("parameters"), TXT("parametersFromOwner"));
}

bool PointOfInterest::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _parametersNodeName, tchar const * _parametersFromOwnerNodeName)
{
	bool result = true;

#ifdef AN_DEVELOPMENT
	debugThis = _node->get_bool_attribute_or_from_child_presence(TXT("debug"), debugThis);
#endif

	name = _node->get_name_attribute(TXT("name"), name);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name given for point of interest"));
		result = false;
	}
	
	result &= tags.load_from_xml_attribute_or_child_node(_node, TXT("tags"));

	socketName = _node->get_name_attribute(TXT("socket"), socketName);
	offset.load_from_xml_child_node(_node, TXT("offset"));
	offset.load_from_xml_child_node(_node, TXT("placement"));

	for_every(volumeNode, _node->children_named(TXT("volume")))
	{
		ConvexHull hull;
		hull.load_from_xml(volumeNode);
		volumes.push_back(hull);
	}

	if (_node->get_bool_attribute_or_from_child_presence(TXT("delayObjectCreation"), false) ||
		_node->get_bool_attribute_or_from_child_presence(TXT("delayedObjectCreation"), false))
	{
		parameters.access<bool>(NAME(delayObjectCreation)) = true;
		IO::XML::Node const * child = nullptr;
		if (!child)
		{
			child = _node->first_child_named(TXT("delayObjectCreation"));
		}
		if (!child)
		{
			child = _node->first_child_named(TXT("delayedObjectCreation"));
		}
		parameters.access<int>(NAME(delayObjectCreationPriority)) = child? child->get_int_attribute(TXT("priority"), 0) : _node->get_int_attribute(TXT("delayedObjectCreationPriority"));
	}

	if (IO::XML::Node const * child = _node->first_child_named(_parametersNodeName))
	{
		result &= parameters.load_from_xml(child);
		_lc.load_group_into(parameters);
	}

	if (IO::XML::Node const * child = _node->first_child_named(_parametersFromOwnerNodeName))
	{
		result &= parametersFromOwner.load_from_xml(child);
		_lc.load_group_into(parametersFromOwner);
	}

	return result;
}

void PointOfInterest::apply(Transform const & _transform)
{
	offset = _transform.to_world(offset);
	offset.set_orientation(offset.get_orientation().normal());
}

void PointOfInterest::apply(Matrix44 const & _transform)
{
	Vector3 yAxis = _transform.vector_to_world(offset.get_axis(Axis::Y)).normal();
	Vector3 zAxis = _transform.vector_to_world(offset.get_axis(Axis::Z)).normal();
	offset.set_translation(_transform.location_to_world(offset.get_translation()));
	offset.set_orientation(look_matrix33(yAxis, zAxis).to_quat());
}

PointOfInterest* PointOfInterest::create_copy() const
{
	PointOfInterest* copy = new PointOfInterest();
	*copy = *this;
	return copy;
}
