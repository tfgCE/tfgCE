#include "socketDefinition.h"

#include "pose.h"
#include "skeleton.h"

#include "..\serialisation\serialiser.h"

using namespace Meshes;

SocketDefinition::SocketDefinition()
{
}

Name SocketDefinition::load_name_from_xml(IO::XML::Node const * _node)
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

bool SocketDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("name")))
	{
		name = attr->get_as_name();
		result &= name.is_valid();
	}
	else
	{
		error_loading_xml(_node, TXT("\"name\" node missing"));
		result = false;
	}

	boneName = _node->get_name_attribute_or_from_child(TXT("boneName"), boneName);

	if (IO::XML::Node const * node = _node->first_child_named(TXT("placementMS")))
	{
		placementMS.load_from_xml(node);
	}
	if (IO::XML::Node const * node = _node->first_child_named(TXT("placement")))
	{
		placementMS.load_from_xml(node);
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("relativePlacement")))
	{
		relativePlacement.load_from_xml(node);
		an_assert(!relativePlacement.is_set() || relativePlacement.get().get_orientation().is_normalised());
	}

	return result;
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool SocketDefinition::serialise(Serialiser & _serialiser)
{
	uint8 version = CURRENT_VERSION;
	serialise_data(_serialiser, version);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid socket definition version"));
		return false;
	}
	serialise_data(_serialiser, name);
	serialise_data(_serialiser, placementMS);
	return true;
}

Transform SocketDefinition::calculate_socket_using_ms(Meshes::SocketDefinition const * _socket, Meshes::Pose const* _poseMS, Meshes::Skeleton const* _skeleton)
{
	if (!_socket)
	{
		return Transform::identity;
	}
	int boneIdx;
	Transform relativePlacement;
	get_socket_info(_socket, _skeleton, boneIdx, relativePlacement);
	if (_skeleton && boneIdx != NONE)
	{
		if (_poseMS)
		{
			return _poseMS->get_bone(boneIdx).to_world(relativePlacement);
		}
		else
		{
			return _skeleton->get_bones_default_placement_MS()[boneIdx].to_world(relativePlacement);
		}
	}
	return relativePlacement;
}

Transform SocketDefinition::calculate_socket_using_ls(Meshes::SocketDefinition const * _socket, Meshes::Pose const* _poseLS, Meshes::Skeleton const* _skeleton)
{
	if (!_socket)
	{
		return Transform::identity;
	}
	int boneIdx;
	Transform relativePlacement;
	get_socket_info(_socket, _skeleton, boneIdx, relativePlacement);
	if (_skeleton && boneIdx != NONE)
	{
		if (_poseLS)
		{
			return _poseLS->get_bone_ms_from_ls(boneIdx).to_world(relativePlacement);
		}
		else
		{
			return _skeleton->get_bones_default_placement_MS()[boneIdx].to_world(relativePlacement);
		}
	}
	return relativePlacement;
}

Transform SocketDefinition::calculate_socket_using_ms(Meshes::SocketDefinition const * _socket, Array<Matrix44> const & _matMS, Meshes::Skeleton const* _skeleton)
{
	if (!_socket)
	{
		return Transform::identity;
	}
	int boneIdx;
	Transform relativePlacement;
	get_socket_info(_socket, _skeleton, boneIdx, relativePlacement);
	if (_matMS.is_index_valid(boneIdx))
	{
		return _matMS[boneIdx].to_transform().to_world(relativePlacement);
	}
	return relativePlacement;
}

bool SocketDefinition::get_socket_info(Meshes::SocketDefinition const * _socket, Meshes::Skeleton const* _skeleton, OUT_ int & _boneIdx, OUT_ Transform & _relativePlacement)
{
	_boneIdx = NONE;
	_relativePlacement = Transform::identity;
	if (_socket)
	{
		Optional<Transform> const & placementMS = _socket->get_placement_MS();
		Optional<Transform> const & relativePlacementOpt = _socket->get_relative_placement();
		Transform relativePlacement = relativePlacementOpt.is_set() ? relativePlacementOpt.get() : Transform::identity;
		if (_socket->get_bone_name().is_valid())
		{
			if (_skeleton)
			{
				_boneIdx = _skeleton->find_bone_index(_socket->get_bone_name());
				if (_boneIdx != NONE)
				{
					if (placementMS.is_set())
					{
						_relativePlacement = _skeleton->get_bones_default_placement_MS()[_boneIdx].to_local(placementMS.get()).to_world(relativePlacement);
						return true;
					}
					else
					{
						_relativePlacement = relativePlacement;
						return true;
					}
				}
			}
			return false;
		}
		if (placementMS.is_set())
		{
			_relativePlacement = placementMS.get();
		}
		_relativePlacement = _relativePlacement.to_world(relativePlacement);
		return true;
	}
	return false;
}

void SocketDefinition::apply_to_placement_MS(Matrix44 const & _mat)
{
	if (placementMS.is_set())
	{
		Transform placement = placementMS.get();
		Vector3 newLoc = _mat.location_to_world(placement.get_translation());
		Vector3 newFwd = _mat.vector_to_world(placement.get_axis(Axis::Y));
		Vector3 newUp = _mat.vector_to_world(placement.get_axis(Axis::Z));
		placementMS = look_at_matrix(newLoc, newLoc + newFwd, newUp).to_transform();
	}
}
