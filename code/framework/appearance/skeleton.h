#pragma once

#include "..\..\core\io\xml.h"
#include "..\..\core\mesh\skeleton.h"
#include "..\..\core\mesh\socketDefinitionSet.h"

#include "..\library\libraryStored.h"

#include "..\meshGen\meshGenerator.h"

namespace Framework
{
	class MeshSkinned;
	class MeshGenerator;
	struct SkeletonSkeletonImporter;
	struct SkeletonSocketsImporter;

	class Skeleton
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Skeleton);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Skeleton(Library * _library, LibraryName const & _name);

		override_ void prepare_to_unload();

		MeshGenerator * get_mesh_generator() const { return useMeshGenerator.get(); }

		bool has_skeleton() const { return skeleton != nullptr && skeletonIsReady; }
		Meshes::Skeleton * get_skeleton() { an_assert(skeleton); return skeleton; }
		Meshes::Skeleton const * get_skeleton() const { an_assert(skeleton); return skeleton; }

		void replace_skeleton(Meshes::Skeleton* _skeleton);

		Meshes::SocketDefinitionSet & access_sockets() { return *sockets; }
		Meshes::SocketDefinitionSet const & get_sockets() const { return *sockets; }
		Array<AppearanceControllerDataPtr> const & get_controllers() const { return controllerDatas; }

		void fuse(Skeleton const * _skeleton, Meshes::RemapBoneArray & _remapBones); // remap in skeleton being fused
		Skeleton* create_temporary_hard_copy() const;

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		virtual void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ void debug_draw() const;
		override_ void log(LogInfoContext & _context) const;

	public:
		void debug_draw_pose_MS(Array<Matrix44> const & _matMS) const;

	protected: friend class MeshSkinned;
		void defer_mesh_skinned_import(MeshSkinned* _mesh, LibraryLoadingContext & _lc);

	protected:
		~Skeleton();

		bool generate_skeleton(Library* _library, LibraryPrepareContext& _pfgContext, REF_ MeshGeneratorRequest const & _meshGeneratorRequest = MeshGeneratorRequest());

	protected:
		bool skeletonIsReady = false; // important only for loading
		Meshes::Skeleton* skeleton;
		Meshes::SocketDefinitionSet* sockets; // always exists! created in constructor

		friend struct SkeletonSkeletonImporter;
		friend struct SkeletonSocketsImporter;

	protected:
		Concurrency::SpinLock deferredMeshesToImportLock = Concurrency::SpinLock(TXT("Framework.Skeleton.deferredMeshesToImportLock"));
		Array<MeshSkinned*> deferredMeshesToImport;
		UsedLibraryStored<MeshGenerator> useMeshGenerator; // optional, will generate skeleton
		SimpleVariableStorage meshGeneratorParameters;
		Array<AppearanceControllerDataPtr> controllerDatas;

		void copy_from(Skeleton const * _skeleton);

	private:
		void log_bone_info(int _boneIdx, LogInfoContext & _context) const;

#ifdef AN_DEVELOPMENT
		mutable int debugBoneIdx = NONE;
		mutable int debugSocketIdx = NONE;
#endif
	};
};
