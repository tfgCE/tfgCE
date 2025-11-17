#include "meshStatic.h"

#include "..\library\library.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(MeshStatic);
LIBRARY_STORED_DEFINE_TYPE(MeshStatic, meshStatic);

MeshStatic::MeshStatic(Library * _library, LibraryName const & _name)
: Mesh(_library, _name)
{
	meshUsage = Meshes::Usage::Static;
}

bool MeshStatic::create_from_template(Library* _library, CreateFromTemplate const & _createFromTemplate) const
{
	LibraryName newName = get_name();
	_createFromTemplate.get_renamer().apply_to(newName);
	if (_library->get_meshes_static().find_may_fail(newName))
	{
		error(TXT("mesh static \"%S\" already exists"), newName.to_string().to_char());
		return false;
	}

	MeshStatic* newMesh = _library->get_meshes_static().find_or_create(newName);
	return fill_on_create_from_template(*newMesh, _createFromTemplate);
}

Mesh* MeshStatic::create_temporary_hard_copy() const
{
	MeshStatic* copy = new MeshStatic(get_library(), LibraryName::invalid());
	copy->be_temporary();
	copy->copy_from(this);
	return copy;
}
