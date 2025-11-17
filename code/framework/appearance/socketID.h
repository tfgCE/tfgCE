#pragma once

#include "..\..\core\math\math.h"

namespace Framework
{
	class Mesh;

	struct SocketID
	{
	public:
		SocketID();
		SocketID(Name const & _name);
		SocketID(Name const & _name, Mesh const * _mesh);

		void clear();

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrName = TXT("socket"));

		void set_name(Name const & _name) { name = _name; invalidate(); }

		void look_up(Mesh const * _mesh, ShouldAllowToFail _allowToFailSilently = DisallowToFail);
		void invalidate() { index = NONE; isValid = false; }

		Name const & get_name() const { return name; }
		int get_index() const { an_assert(isValid); return index; }
		bool is_valid() const { return isValid; } // index valid!
		bool is_name_valid() const { return name.is_valid(); }

		Mesh const* get_mesh() const { return mesh; }

		bool operator==(SocketID const & _other) const { return name == _other.name && mesh == _other.mesh; }

	private:
		Name name;
		Mesh const * mesh;
		int index;
		bool isValid;
	};
};
