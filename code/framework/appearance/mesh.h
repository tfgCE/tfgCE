#pragma once

#include "..\..\core\mesh\usage.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\other\simpleVariableStorage.h"

#include "..\collision\collisionModel.h"
#include "..\library\libraryStored.h"
#include "..\video\material.h"
#include "..\world\pointOfInterestTypes.h"
#include "..\world\spaceBlocker.h"

#include "appearanceControllerPtr.h"
#include "meshNode.h"

namespace Meshes
{
	struct SocketDefinition;
	class SocketDefinitionSet;
};

namespace Framework
{
	class CollisionModel;
	class Skeleton;
	class Mesh;
	class MeshGenerator;
	class PhysicalMaterial;
	struct MeshMeshImporter;
	struct MeshSocketsImporter;
	struct MeshSkinnedMeshImporter;

	struct MeshLOD
	{
		Array<int> remapBones; // index is bone index in lod mesh, value is from where it should get bone data (bone index in main mesh), if empty, gets the same bones
		UsedLibraryStored<Mesh> mesh;
	};

	class Mesh
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Mesh);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Mesh(Library * _library, LibraryName const & _name);
		virtual ~Mesh();

		override_ void prepare_to_unload();

		void generated_on_demand_if_required();

		Meshes::Mesh3D * get_mesh() { an_assert(mesh, TXT("make sure mesh exists")); return mesh; }
		Meshes::Mesh3D const * get_mesh() const { an_assert(mesh, TXT("make sure mesh exists")); return mesh; }
		void make_sure_mesh_exists();
		void replace_mesh(Meshes::Mesh3D* _mesh);

		MeshGenerator * get_mesh_generator() const { return useMeshGenerator.get(); }

		virtual Skeleton const * get_skeleton() const { return nullptr; }
		virtual void set_skeleton(Skeleton* _skeleton);
		Meshes::SocketDefinitionSet & access_sockets() { return *sockets; } // sockets for mesh only!
		Meshes::SocketDefinitionSet const & get_sockets() const { return *sockets; } // sockets for mesh only!

		Array<MeshNodePtr> & access_mesh_nodes() { return meshNodes; }
		Array<MeshNodePtr> const & get_mesh_nodes() const { return meshNodes; }
		MeshNode const * find_mesh_node(Name const & _name) const;
		MeshNode * access_mesh_node(Name const& _name);

		Array<PointOfInterestPtr> & access_pois() { return pois; }
		Array<PointOfInterestPtr> const & get_pois() const { return pois; }

		SpaceBlockers & access_space_blockers() { return spaceBlockers; }
		SpaceBlockers const & get_space_blockers() const { return spaceBlockers; }
		
		Array<AppearanceControllerDataPtr> & access_controllers() { return controllerDatas; }
		Array<AppearanceControllerDataPtr> const & get_controllers() const { return controllerDatas; }

		// socket index 0-something for mesh, after that overrides go (for example skeleton)
		virtual int find_socket_index(Name const & _socketName) const;
		bool has_socket(Name const & _socketName) const { return find_socket_index(_socketName) != NONE; }
		Transform calculate_socket_using_ms(int _socketIndex, Meshes::Pose const * _poseMS = nullptr) const;
		Transform calculate_socket_using_ms(Name const & _socketName, Meshes::Pose const * _poseMS = nullptr) const { return calculate_socket_using_ms(find_socket_index(_socketName), _poseMS); }
		Transform calculate_socket_using_ls(int _socketIndex, Meshes::Pose const * _poseLS = nullptr) const;
		Transform calculate_socket_using_ls(Name const & _socketName, Meshes::Pose const * _poseLS = nullptr) const { return calculate_socket_using_ls(find_socket_index(_socketName), _poseLS); }
		bool get_socket_info(int _socketIndex, OUT_ int & _boneIdx, OUT_ Transform & _relativePlacement) const; // relative to 0,0,0 or to bone

		CollisionModel const * get_movement_collision_model() const { return movementCollisionModel.get(); }
		void set_movement_collision_model(CollisionModel * _cm);

		CollisionModel const * get_precise_collision_model() const { return preciseCollisionModel.get(); }
		void set_precise_collision_model(CollisionModel * _cm);

		Array<MaterialSetup> const & get_material_setups() const { return materialSetups; }
		void set_material(int _idx, Material* _material);
		void set_material_setup(int _idx, MaterialSetup const& _materialSetup);
		Material const * get_material(int _idx) const { if (_idx < materialSetups.get_size()) return materialSetups[_idx].get_material(); else return nullptr; }

		int get_all_materials_in_mesh_count() const;
		void set_missing_materials(Library* _library);

		void cache_collisions();

		struct FuseMesh
		{
			Mesh const* mesh;
			Optional<Transform> placement;
			FuseMesh() {}
			explicit FuseMesh(Mesh const* _mesh, Optional<Transform> const& _placement = NP) : mesh(_mesh), placement(_placement) {}
		};
		// if you want collisions to be fused, provide empty model so we have a place to fuse into
		void fuse(Array<FuseMesh> const & _meshes, Array<Meshes::RemapBoneArray const *> const * _remapBones = nullptr); // fuse other meshes into this one (will leave other meshes as they were), will fuse lods!

		virtual Mesh* create_temporary_hard_copy() const; // create copy of skeleton too!

		void be_unique_instance();
		void be_shared() { isUniqueInstance = false; }
		bool is_unique_instance() const { return isUniqueInstance; }
		bool is_shared() const { return !isUniqueInstance; }

		Mesh const * get_lod(int _lod) const; // 0 this, 1 lod0, 2 lod1 and so on
		int get_lod_count() const { return lods.get_size() + 1; }
		Array<int> const& get_remap_bones_for_lod(int _lod) const;

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		virtual void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ void debug_draw() const;
		override_ void log(LogInfoContext & _context) const;

	protected:
		void defer_mesh_delete();

		virtual bool load_mesh_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

		virtual Meshes::SocketDefinition const * get_socket(int _socketIndex) const; // this finds right socket
		virtual int get_socket_count() const;

		virtual bool fill_on_create_from_template(LibraryStored& _newObject, CreateFromTemplate const & _createFromTemplate) const;

		virtual void build_remap_bones(Mesh const* _toMeshToUse, Array<int>& _remapBones) const; // call for main mesh

		static Meshes::SocketDefinition const * get_socket_in(int _socketIndex, Meshes::SocketDefinitionSet const & _sockets);
		static int find_socket_index_in(Name const & _socketName, Meshes::SocketDefinitionSet const & _sockets);

	protected:
		Meshes::Mesh3D* mesh = nullptr;
		bool isUniqueInstance = false; // otherwise might be shared by multiple objects
		Optional<int> generateLODs;
		Optional<float> useLODFully; // used during generation
		Optional<float> use1AsSubStepBase;
		Meshes::Usage::Type meshUsage;
		Meshes::SocketDefinitionSet* sockets; // always exists! created in constructor
		UsedLibraryStored<CollisionModel> movementCollisionModel;
		UsedLibraryStored<CollisionModel> preciseCollisionModel;
		Array<MaterialSetup> materialSetups;
		UsedLibraryStored<MeshGenerator> useMeshGenerator; // optional, will generate mesh
		SimpleVariableStorage meshGeneratorParameters;
		bool useCustomParametersForMeshGeneration = false; // use only for testing/preview
		CACHED_ SimpleVariableStorage useMeshGeneratorParameters;
		Array<MeshNodePtr> loadedMeshNodes; // loaded from a file

		friend struct MeshMeshImporter;
		friend struct MeshSocketsImporter;
		friend struct MeshSkinnedMeshImporter;

		Array<MeshNodePtr> meshNodes; // those are static objects that can be used as reference points for some stuff (eg. generate navmesh)
		Array<PointOfInterestPtr> pois;
		Array<AppearanceControllerDataPtr> controllerDatas;
		SpaceBlockers spaceBlockers;

		// will create collision mesh automatically
		UsedLibraryStored<PhysicalMaterial> autoCreateMovementCollisionMeshPhysicalMaterial;
		UsedLibraryStored<PhysicalMaterial> autoCreatePreciseCollisionMeshPhysicalMaterial;
		UsedLibraryStored<PhysicalMaterial> autoCreateMovementCollisionBoxPhysicalMaterial;
		UsedLibraryStored<PhysicalMaterial> autoCreatePreciseCollisionBoxPhysicalMaterial;
		bool autoCreateSpaceBlocker = false;

	protected:
		Array<MeshLOD> lods; // lod 0 is this mesh, so the index 0 is lod 1, 1 is lod 2 etc, total count of lods is (lods.get_size() + 1)
		float sizeForLOD = 0.0f;

		virtual void copy_from(Mesh const * _mesh);

		void update_lods_remapping();

	private:
#ifdef AN_DEVELOPMENT
		mutable int debugSocketIdx = NONE;
		mutable int debugPOIIdx = NONE;
		mutable int debugMeshNodeIdx = NONE;
		static bool debugDrawMesh;
		static bool debugDrawSkeleton;
		static bool debugDrawSockets;
		static bool debugDrawMeshNodes;
		static bool debugDrawPOIs;
		static bool debugDrawPreciseCollision;
		static bool debugDrawMovementCollision;
		static bool debugDrawSpaceBlockers;
#endif

		Library* generateOnDemandForLibrary = nullptr;

		bool generate_mesh(Library* _library);

		bool sub_load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		friend class MeshGenerator; // to store LODs
	};
};
