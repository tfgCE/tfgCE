#include "meshSkinned.h"
#include "..\..\core\serialisation\importerHelper.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

REGISTER_FOR_FAST_CAST(MeshSkinned);
LIBRARY_STORED_DEFINE_TYPE(MeshSkinned, meshSkinned);

struct ::Framework::MeshSkinnedMeshImporter
: public ImporterHelper<Meshes::Mesh3D, Meshes::Mesh3DImportOptions>
{
	typedef ImporterHelper<Meshes::Mesh3D, Meshes::Mesh3DImportOptions> base;

	MeshSkinnedMeshImporter(Framework::MeshSkinned* _mesh)
	: base(_mesh->mesh)
	, mesh(_mesh)
	{}

	void use_skeleton(Meshes::Skeleton* _useSkeleton)
	{
		importOptions.skeleton = _useSkeleton;
	}

protected:
	override_ void create_object()
	{
		object = new Meshes::Mesh3D();
		object->set_usage(mesh->meshUsage);
#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		object->set_debug_mesh_name(String(TXT("Framework::MeshSkinned (import) ")) + mesh->get_name().to_string());
#endif
	}

	override_ void delete_object()
	{
		mesh->defer_mesh_delete();
	}

	override_ bool check_if_object_actual()
	{
		return base::check_if_object_actual() &&
			   object->get_digested_skeleton() == importOptions.skeleton->get_digested_source();
	}

	override_ void import_object()
	{
		an_assert(importOptions.skeleton, TXT("you should defer importing until skeleton is available, otherwise no skinning data can be imported and you should revert to meshStatic"));
		object = Meshes::Mesh3D::importer().import(importFromFileName, importOptions);
		object->set_usage(mesh->meshUsage);
	}

private:
	Framework::MeshSkinned* mesh;
};

//

MeshSkinned::MeshSkinned(Library * _library, LibraryName const & _name)
: Mesh(_library, _name)
, deferredMeshImporter(nullptr)
{
	meshUsage = Meshes::Usage::Skinned;
}

MeshSkinned::~MeshSkinned()
{
	delete_and_clear(deferredMeshImporter);
}

bool MeshSkinned::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	// load skeleton before loading mesh, so when loading mesh we will be able to load mesh with skeleton or defer it
	result &= skeleton.load_from_xml(_node, TXT("skeleton"), _lc);

	// we need to have skeleton at this point
	// if it has to be changed, consider loading stuff and importing in prepare for game
	result &= skeleton.find_or_create(_lc.get_library());

	// load mesh as ususal
	result &= base::load_from_xml(_node, _lc);

	return result;
}

bool MeshSkinned::load_mesh_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	an_assert(!deferredMeshImporter);
	an_assert(skeleton.get());
	deferredMeshImporter = new MeshSkinnedMeshImporter(this);
	deferredMeshImporter->setup_from_xml(_node, _lc.get_library_source() == LibrarySource::Assets);
	deferredMeshImporter->digest_definition(_node);
	if (skeleton->has_skeleton())
	{
		result &= perform_deferred_import(_lc);
	}
	else
	{
		skeleton->defer_mesh_skinned_import(this, _lc);
	}
	return result;
}

bool MeshSkinned::perform_deferred_import(LibraryLoadingContext & _lc)
{
	bool result = true;
	an_assert(skeleton.get());
	an_assert(deferredMeshImporter);
	if (skeleton->has_skeleton())
	{
		deferredMeshImporter->use_skeleton(skeleton->get_skeleton());
		result &= deferredMeshImporter->import();
	}
	else
	{
		result = false;
	}
	delete_and_clear(deferredMeshImporter);
	return result;
}

bool MeshSkinned::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (skeleton.is_name_valid())
	{
		result &= skeleton.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		cache_collisions();
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (deferredMeshImporter)
		{
			error(TXT("mesh \"%S\" was not imported as deferred, because \"%S\" was most probably not imported"), get_name().to_string().to_char(), skeleton.get_name().to_string().to_char());
			result = false;
		}
	}

	return result;
}

int MeshSkinned::get_socket_count() const
{
	int socketCount = base::get_socket_count();
	if (auto const * s = skeleton.get())
	{
		socketCount += s->get_sockets().get_sockets().get_size();
	}
	return socketCount;
}

Meshes::SocketDefinition const * MeshSkinned::get_socket(int _socketIndex) const
{
	int const baseSocketCount = base::get_socket_count();
	if (_socketIndex < baseSocketCount)
	{
		return base::get_socket(_socketIndex);
	}
	// move to better space
	_socketIndex -= baseSocketCount;
	if (auto const * s = skeleton.get())
	{
		return get_socket_in(_socketIndex, s->get_sockets());
	}
	return nullptr;
}

int MeshSkinned::find_socket_index(Name const & _socketName) const
{
	int const baseSocketCount = base::get_socket_count();
	if (auto const * s = skeleton.get())
	{
		int socketIndex = find_socket_index_in(_socketName, s->get_sockets());
		if (socketIndex != NONE)
		{
			return socketIndex += baseSocketCount;
		}
	}
	return base::find_socket_index(_socketName);
}

void MeshSkinned::set_skeleton(Skeleton* _skeleton)
{
	if (skeleton.get() != _skeleton)
	{
		skeleton = _skeleton;
		cache_collisions();
	}
}

Mesh* MeshSkinned::create_temporary_hard_copy() const
{
	MeshSkinned* copy = new MeshSkinned(get_library(), LibraryName::invalid());
	copy->be_temporary();
	copy->copy_from(this);
	return copy;
}

void MeshSkinned::copy_from(Mesh const * _mesh)
{
	base::copy_from(_mesh);
	an_assert(fast_cast<MeshSkinned>(_mesh));
	if (auto* src = fast_cast<MeshSkinned>(_mesh))
	{
		skeleton = src->skeleton->create_temporary_hard_copy();
	}
}

void MeshSkinned::build_remap_bones(Mesh const* _toMeshToUse, Array<int>& _remapBones) const
{
	MeshSkinned const* toMeshToUse = fast_cast<MeshSkinned>(_toMeshToUse);
	if (!toMeshToUse)
	{
		an_assert_log_always(false, TXT("can't build remap bones if other mesh is not skinned"));
		error(TXT("can't build remap bones if other mesh is not skinned"));
		base::build_remap_bones(_toMeshToUse, _remapBones);
		return;
	}

	an_assert_log_always(skeleton.get() && skeleton->get_skeleton(), TXT("skeleton required for \"%S\""), get_name().to_string().to_char());
	an_assert_log_always(toMeshToUse->skeleton.get() && toMeshToUse->skeleton->get_skeleton(), TXT("skeleton required for \"%S\""), toMeshToUse->get_name().to_string().to_char());

#ifdef AN_DEVELOPMENT
	if (!skeleton.get())
	{
		AN_BREAK;
	}
	if (!toMeshToUse->skeleton.get())
	{
		AN_BREAK;
	}
#endif

	auto* outSkeleton = skeleton->get_skeleton();
	auto* toUseSkeleton = toMeshToUse->skeleton->get_skeleton();

	_remapBones.clear();
	for_every(bone, outSkeleton->get_bones())
	{
		Name boneName = bone->get_name();
		_remapBones.push_back(toUseSkeleton->find_bone_index(boneName));
	}

	bool allInOrder = outSkeleton->get_num_bones() == toUseSkeleton->get_num_bones();
	if (allInOrder)
	{
		for_every(bone, _remapBones)
		{
			if (*bone != for_everys_index(bone))
			{
				allInOrder = false;
				break;
			}
		}
	}
	if (allInOrder)
	{
		_remapBones.clear();
	}
}
