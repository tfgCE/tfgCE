#pragma once

#include "mesh.h"

namespace Framework
{
	class MeshStatic
	: public Mesh
	{
		FAST_CAST_DECLARE(MeshStatic);
		FAST_CAST_BASE(Mesh);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Mesh base;
	public:
		MeshStatic(Library * _library, LibraryName const & _name);

	public: // LibraryStored
		override_ bool create_from_template(Library* _library, CreateFromTemplate const & _createFromTemplate) const;

	public: // Mesh
		override_ Mesh* create_temporary_hard_copy() const;
	};
};
