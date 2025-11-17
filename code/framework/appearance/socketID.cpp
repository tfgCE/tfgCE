#include "socketID.h"

#include "mesh.h"

using namespace Framework;

SocketID::SocketID()
: mesh(nullptr)
{
	invalidate();
}

SocketID::SocketID(Name const & _name)
: name(_name)
, mesh(nullptr)
{
	invalidate();
}

SocketID::SocketID(Name const & _name, Mesh const * _mesh)
: name(_name)
{
	invalidate();
	look_up(_mesh);
}

void SocketID::clear()
{
	invalidate();
	// and clear rest too
	name = Name::invalid();
	mesh = nullptr;
}

bool SocketID::load_from_xml(IO::XML::Node const * _node, tchar const * _attrName)
{
	bool result = true;

	if (_node)
	{
		name = _node->get_name_attribute(_attrName, name);
		invalidate();
	}

	return result;
}

void SocketID::look_up(Mesh const * _mesh, ShouldAllowToFail _allowToFailSilently)
{
	mesh = _mesh;
	index = mesh? mesh->find_socket_index(name) : NONE;
	isValid = index != NONE;

	if (_allowToFailSilently == DisallowToFail &&
		index == NONE && name.is_valid())
	{
		error(TXT("couldn't find socket \"%S\""), name.to_char());
	}
}
