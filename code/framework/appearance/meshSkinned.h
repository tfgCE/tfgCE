#pragma once

#include "mesh.h"

namespace Framework
{
	class Skeleton;
	struct MeshSkinnedMeshImporter;

	/* skinned and skinned to single bone */
	class MeshSkinned
	: public Mesh
	{
		FAST_CAST_DECLARE(MeshSkinned);
		FAST_CAST_BASE(Mesh);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Mesh base;
	public:
		MeshSkinned(Library * _library, LibraryName const & _name);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected: // Mesh
		override_ Skeleton const * get_skeleton() const { return skeleton.get(); }
		override_ void set_skeleton(Skeleton* _skeleton);

		override_ int find_socket_index(Name const & _socketName) const;
		override_ Meshes::SocketDefinition const * get_socket(int _socketIndex) const;
		override_ int get_socket_count() const;

		override_ bool load_mesh_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		override_ Mesh* create_temporary_hard_copy() const;

		override_ void build_remap_bones(Mesh const* _toMeshToUse, Array<int>& _remapBones) const; // call for main mesh

	protected: friend class Skeleton;
		bool perform_deferred_import(LibraryLoadingContext & _lc);

	protected:
		virtual ~MeshSkinned();

		override_ void copy_from(Mesh const * _mesh);

	protected:
		UsedLibraryStored<Skeleton> skeleton; // always exists (due to find_or_create), but may point at skeleton not yet loaded

		friend struct MeshSkinnedMeshImporter;

	private:
		MeshSkinnedMeshImporter* deferredMeshImporter;
	};
};
