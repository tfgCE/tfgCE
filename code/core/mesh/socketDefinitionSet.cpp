#include "socketDefinitionSet.h"

#include "..\debug\debugRenderer.h"
#include "..\serialisation\serialiser.h"

using namespace Meshes;

Importer<SocketDefinitionSet, SocketDefinitionSetImportOptions> SocketDefinitionSet::s_importer;

SocketDefinitionSetImportOptions::SocketDefinitionSetImportOptions()
: importBoneMatrix(Matrix33::identity)
, importScale(Vector3::one)
{
}

bool SocketDefinitionSetImportOptions::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("importFromNode")))
	{
		importFromNode = attr->get_as_string();
	}
	if (IO::XML::Node const * node = _node->first_child_named(TXT("importBoneMatrix")))
	{
		result &= importBoneMatrix.load_axes_from_xml(node);
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("importFromPlacement")))
	{
		importFromPlacement.set_if_not_set(Transform::identity);
		importFromPlacement.access().load_from_xml(childNode);
	}
	if (IO::XML::Node const * childNode = _node->first_child_named(TXT("placeAfterImport")))
	{
		placeAfterImport.set_if_not_set(Transform::identity);
		placeAfterImport.access().load_from_xml(childNode);
	}
	{
		float const isDef = -1000.0f;
		float is = _node->get_float_attribute_or_from_child(TXT("importScale"), isDef);
		if (is != isDef)
		{
			importScale = is * Vector3::one;
		}
		importScale.load_from_xml_child_node(_node, TXT("importScale"), TXT("x"), TXT("y"), TXT("z"), TXT("axis"), true);
	}
	socketNodePrefix = _node->get_string_attribute(TXT("socketNodePrefix"), socketNodePrefix);
	return result;
}

//

SocketDefinitionSet* SocketDefinitionSet::create_new()
{
	return new SocketDefinitionSet();
}

SocketDefinitionSet::SocketDefinitionSet()
{
}

bool SocketDefinitionSet::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(socketNode, _node->children_named(TXT("socket")))
	{
		Name socketName = SocketDefinition::load_name_from_xml(socketNode);
		SocketDefinition* socket = find_or_create_socket(socketName);
		result &= socket->load_from_xml(socketNode);
	}

	return result;
}

IO::XML::Node const * SocketDefinitionSet::find_xml_socket_node(IO::XML::Node const * _node, Name const & _socketName)
{
	for_every(socketNode, _node->children_named(TXT("socket")))
	{
		if (SocketDefinition::load_name_from_xml(socketNode) == _socketName)
		{
			return socketNode;
		}
	}
	return nullptr;
}

SocketDefinition* SocketDefinitionSet::find_socket(Name const & _socketName)
{
	for_every(socket, sockets)
	{
		if (socket->get_name() == _socketName)
		{
			return socket;
		}
	}
	return nullptr;
}

SocketDefinition* SocketDefinitionSet::find_or_create_socket(Name const & _socketName)
{
	if (SocketDefinition* socket = find_socket(_socketName))
	{
		return socket;
	}
	SocketDefinition socket;
	socket.set_name(_socketName);
	sockets.push_back(socket);
	return &(sockets.get_last());
}

int SocketDefinitionSet::find_socket_index(Name const & _socketName) const
{
	int index = 0;
	for_every(socket, sockets)
	{
		if (socket->get_name() == _socketName)
		{
			return index;
		}
		++index;
	}
	return NONE;
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool SocketDefinitionSet::serialise(Serialiser & _serialiser)
{
	version = CURRENT_VERSION;
	bool result = SerialisableResource::serialise(_serialiser);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid socket definition set version"));
		return false;
	}
	result &= serialise_data(_serialiser, sockets);
	return result;
}

void SocketDefinitionSet::add_sockets_from(SocketDefinitionSet const & _set, Optional<Transform> const & _placement)
{
	for_every(socket, _set.sockets)
	{
		auto& intoSocket = *find_or_create_socket(socket->get_name());
		intoSocket = *socket;
		if (_placement.is_set() && intoSocket.get_placement_MS().is_set())
		{
			intoSocket.set_placement_MS(_placement.get().to_world(intoSocket.get_placement_MS().get()));
		}
	}
}

void SocketDefinitionSet::debug_draw_pose_MS(Array<Matrix44> const & _matMS, Skeleton* _skeleton) const
{
#ifdef AN_DEBUG_RENDERER
	for_every(socket, sockets)
	{
		Transform socketPlacement = socket->calculate_socket_using_ms(socket, _matMS, _skeleton);
		if (socketPlacement.get_scale() > 0.0f)
		{
			debug_draw_matrix_size(true, socketPlacement.to_matrix(), 0.1f);
			debug_draw_text(true, NP, socketPlacement.get_translation(), NP, true, 0.1f, false, socket->get_name().to_char());
		}
		else
		{
			debug_draw_text(true, Colour::grey, socketPlacement.get_translation(), NP, true, 0.1f, false, socket->get_name().to_char());
		}
	}
#endif
}
