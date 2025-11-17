#pragma once

#include "meshGenCreateCollisionInfo.h"
#include "meshGenCreateSpaceBlockerInfo.h"
#include "meshGenGenerationInfo.h"
#include "meshGenTypes.h"

#include "..\..\core\mesh\mesh3d.h"

#include "..\appearance\appearanceControllerPtr.h"
#include "..\appearance\meshNode.h"
#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"
#include "..\video\material.h"
#include "..\world\pointOfInterestTypes.h"

class SimpleVariableStorage;
struct DebugRendererFrame;

namespace Collision
{
	class Model;
};

namespace Meshes
{
	class SocketDefinitionSet;
};

namespace WheresMyPoint
{
	interface_class IOwner;
};

namespace Framework
{
	class Mesh;
	class MeshGenerator;
	class Skeleton;
	struct SpaceBlockers;
	
	namespace MeshGeneration
	{
		class Element;
	};

	struct MeshGeneratorRequest
	{
		static int const MAX_PARAMETERS_COUNT = 4;

#ifdef AN_DEVELOPMENT
		static Optional<Random::Generator> commonRandomGenerator;
#endif
		MeshGeneratorRequest();

		MeshGeneratorRequest& for_wmp_owner(WheresMyPoint::IOwner* _wmpOwner) { wmpOwner = _wmpOwner; return *this; }

		MeshGeneratorRequest& use_library_name_for_new(LibraryName const & _useLibraryNameForNew) { useLibraryNameForNew = _useLibraryNameForNew; return *this; }

		MeshGeneratorRequest& for_library_mesh(Mesh* _forLibraryMesh) { forLibraryMesh = _forLibraryMesh; return *this; } // will setup library mesh afterwards
		MeshGeneratorRequest& for_library_skeleton(Skeleton* _forLibrarySkeleton) { forLibrarySkeleton = _forLibrarySkeleton; return *this; } // will setup library skeleton afterwards

		MeshGeneratorRequest& force_more_details(bool _forceMoreDetails = true) { forceMoreDetails = _forceMoreDetails; return *this; }
		MeshGeneratorRequest& for_lod(int _forLOD) { forLOD = _forLOD; lodCount = 0; return *this; }
		MeshGeneratorRequest& generate_lods(Optional<int> const& _lodCount = NP);
		MeshGeneratorRequest& no_lods() { lodCount = 0; return *this; }
		MeshGeneratorRequest& use_lod_fully(float _useLODFully) { useLODFully = _useLODFully; return *this; }
		MeshGeneratorRequest& use_1_as_sub_step_base(float _use1AsSubStepBase) { use1AsSubStepBase = _use1AsSubStepBase; return *this; }

		MeshGeneratorRequest& with_include_stack_processor(MeshGeneration::IncludeStackProcessorFunc _include_stack_processor) { include_stack_processor = _include_stack_processor; return *this; }

		MeshGeneratorRequest& using_parameters(SimpleVariableStorage const& _p) { an_assert(usingParametersCount < MAX_PARAMETERS_COUNT - 1); usingParameters[usingParametersCount] = &_p; ++usingParametersCount; return *this; }
		MeshGeneratorRequest& using_random_regenerator_if_not_set(Random::Generator const & _p) { if (!usingRandomGenerator) usingRandomGenerator = &_p; return *this; }
		MeshGeneratorRequest& using_random_regenerator(Random::Generator const & _p) { usingRandomGenerator = &_p; return *this; }
		MeshGeneratorRequest& using_random_random_regenerator() { usingRandomRandomGenerator = true; return *this; }
		MeshGeneratorRequest& using_skeleton(Skeleton * _p) { usingSkeletonOverride = _p; return *this; }
		MeshGeneratorRequest& using_material_setups(Array<MaterialSetup> const & _materialSetups) { usingMaterialSetups = _materialSetups; return *this; }
		MeshGeneratorRequest& using_mesh_nodes(Array<MeshNodePtr> const & _meshNodes) { usingMeshNodes.add_from(_meshNodes); return *this; } // note that if we drop any of those, the original will be dropped too!
		MeshGeneratorRequest& using_mesh_node(MeshNode* _meshNode) { usingMeshNodes.push_back(MeshNodePtr(_meshNode)); return *this; }
		
		MeshGeneratorRequest& store_movement_collision(Collision::Model*& _mc, bool _force = false) { if (!movementCollisionResultPtr || _force) movementCollisionResultPtr = &_mc; return *this; }
		MeshGeneratorRequest& store_precise_collision(Collision::Model*& _mc, bool _force = false) { if (!preciseCollisionResultPtr || _force) preciseCollisionResultPtr = &_mc; return *this; }
		MeshGeneratorRequest& store_generated_skeleton(UsedLibraryStored<Skeleton>& _s, bool _force = false) { if (!generatedSkeletonResultPtr || _force) generatedSkeletonResultPtr = &_s; return *this; }
		MeshGeneratorRequest& store_sockets(Meshes::SocketDefinitionSet & _s, bool _force = false) { if (!socketsResultPtr || _force) socketsResultPtr = &_s; return *this; }
		MeshGeneratorRequest& store_mesh_nodes(Array<MeshNodePtr>& _m, bool _force = false) { if (!meshNodesResultPtr || _force) meshNodesResultPtr = &_m; return *this; }
		MeshGeneratorRequest& store_appearance_controllers(Array<AppearanceControllerDataPtr> & _ac, bool _force = false) { if (!appearanceControllersResultPtr || _force) appearanceControllersResultPtr = &_ac; return *this; }
		MeshGeneratorRequest& store_pois(Array<PointOfInterestPtr> & _pois, bool _force = false) { if (!poisResultPtr || _force) poisResultPtr = &_pois; return *this; }
		MeshGeneratorRequest& store_space_blockers(SpaceBlockers & _spaceBlockers, bool _force = false) { if (!spaceBlockersResultPtr || _force) spaceBlockersResultPtr = &_spaceBlockers; return *this; }

		bool is_storing_anything() const
		{
			return movementCollisionResultPtr != nullptr ||
				   preciseCollisionResultPtr != nullptr ||
				   generatedSkeletonResultPtr != nullptr ||
				   socketsResultPtr != nullptr ||
				   meshNodesResultPtr != nullptr ||
				   appearanceControllersResultPtr != nullptr ||
				   poisResultPtr != nullptr ||
				   spaceBlockersResultPtr != nullptr;
		}

	private: friend class MeshGenerator;
		WheresMyPoint::IOwner* wmpOwner = nullptr;

		LibraryName useLibraryNameForNew = LibraryName::invalid();

		Mesh* forLibraryMesh = nullptr;
		Skeleton* forLibrarySkeleton = nullptr;

		bool forceMoreDetails = false;
		int forLOD = 0;
		Optional<float> useLODFully;
		Optional<float> use1AsSubStepBase;
		Optional<int> lodCount;

		MeshGeneration::IncludeStackProcessorFunc include_stack_processor = nullptr;
			
		int usingParametersCount = 0;
		SimpleVariableStorage const * usingParameters[MAX_PARAMETERS_COUNT];
		Random::Generator const * usingRandomGenerator = nullptr;
		bool usingRandomRandomGenerator = false;
		Skeleton* usingSkeletonOverride = nullptr;
		Array<MaterialSetup> usingMaterialSetups;
		Array<MeshNodePtr> usingMeshNodes;

		Collision::Model** movementCollisionResultPtr = nullptr;
		Collision::Model** preciseCollisionResultPtr = nullptr;
		UsedLibraryStored<Skeleton>* generatedSkeletonResultPtr = nullptr;
		Meshes::SocketDefinitionSet* socketsResultPtr = nullptr;
		Array<MeshNodePtr>* meshNodesResultPtr = nullptr;
		Array<AppearanceControllerDataPtr>* appearanceControllersResultPtr = nullptr;
		Array<PointOfInterestPtr>* poisResultPtr = nullptr;
		SpaceBlockers* spaceBlockersResultPtr = nullptr;
	};

	class MeshGenerator
	: public LibraryStored
	{
		FAST_CAST_DECLARE(MeshGenerator);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		MeshGenerator(Library * _library, LibraryName const & _name);
		virtual ~MeshGenerator();

		void clear();

		::System::VertexFormat const & get_vertex_format() const { return vertexFormat; }
		::System::IndexFormat const & get_index_format() const { return indexFormat; }

		Array<MaterialSetup> const & get_material_setups() const { return materialSetups; }

		Optional<int> const& get_generate_lods() const { return generateLODs; }

#ifdef AN_DEVELOPMENT
		void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

	public: // all should make sure we're loaded on demand
		MeshGeneration::Element const * get_main_mesh_generation_element() const { return mainElement.get(); }
		::Meshes::Mesh3D* generate_mesh(REF_ MeshGeneratorRequest const & _request = MeshGeneratorRequest(), REF_ MeshGeneratorRequest const & _skelGeneratorRequest = MeshGeneratorRequest());
		UsedLibraryStored<Mesh> generate_temporary_library_mesh(Library* _library, REF_ MeshGeneratorRequest const & _usingRequest = MeshGeneratorRequest(), REF_ MeshGeneratorRequest const & _skelGeneratorRequest = MeshGeneratorRequest()); // will ignore existing storing collisions
		bool generate_for_library_mesh(UsedLibraryStored<Mesh>& _mesh, Library* _library, REF_ MeshGeneratorRequest const& _usingRequest, REF_ MeshGeneratorRequest const& _skelGeneratorRequest);

		::Meshes::Skeleton* generate_skel(REF_ MeshGeneratorRequest const & _request = MeshGeneratorRequest());
		UsedLibraryStored<Skeleton> generate_temporary_library_skeleton(Library* _library, REF_ MeshGeneratorRequest const & _usingRequest = MeshGeneratorRequest());

	public:
		MeshGeneration::CreateCollisionInfo const & get_create_movement_collision_box() const { return createMovementCollisionBox; }
		MeshGeneration::CreateCollisionInfo const & get_create_precise_collision_box() const { return createPreciseCollisionBox; }
		MeshGeneration::CreateCollisionInfo const & get_create_movement_collision_mesh() const { return createMovementCollisionMesh; }
		MeshGeneration::CreateCollisionInfo const & get_create_precise_collision_mesh() const { return createPreciseCollisionMesh; }
		
		MeshGeneration::CreateSpaceBlockerInfo get_create_space_blocker() const { return createSpaceBlocker; }

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		RefCountObjectPtr<MeshGeneration::Element> mainElement;
		UsedLibraryStored<Skeleton> skeleton;
		UsedLibraryStored<MeshGenerator> skelGenerator;
		bool canOnlyBeIncluded = false; // true if vertex format or index format not defined

		bool allowVerticesOptimisation = true;

		bool forceMoreDetails = false;
		Optional<int> generateLODs; // current order (from the most important): mesh >> mesh generator >> 1.0f
		Optional<float> useLODFully; // current order (from the most important): mesh >> mesh generator >> 1.0f
		Optional<float> use1AsSubStepBase; // current order (from the most important): mesh >> mesh generator >> 1.0f

		// if those are not provided in xml, they will have some assumed setup
		::System::VertexFormat vertexFormat;
		::System::IndexFormat indexFormat;

		MeshGeneration::CreateCollisionInfo createMovementCollisionBox;
		MeshGeneration::CreateCollisionInfo createPreciseCollisionBox;
		MeshGeneration::CreateCollisionInfo createMovementCollisionMesh;
		MeshGeneration::CreateCollisionInfo createPreciseCollisionMesh;
		MeshGeneration::CreateCollisionInfo createPreciseCollisionMeshSkinned; // will create mesh skinned
		
		MeshGeneration::CreateSpaceBlockerInfo createSpaceBlocker;

		// used when creating temporary mesh
		Array<MaterialSetup> materialSetups;
		int defaultMaterialSetupAs = NONE; // if more materials appear to be in use, this will be used as default material for them

		Meshes::SocketDefinitionSet* sockets = nullptr;

		void add_material_setups_to(REF_ MeshGeneratorRequest & _request) const;

		bool sub_load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

	private:
		void setup_library_skeleton_with_generated(Skeleton* _librarySkeleton, ::Meshes::Skeleton* _generatedSkeleton, MeshGeneratorRequest const& _request);
		void setup_library_mesh_with_generated(Mesh* _libraryMesh, ::Meshes::Mesh3D* _generatedMesh, MeshGeneratorRequest const& _request);

		void fill_vertex_info(Meshes::Mesh3D* _mesh);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		bool should_log_mesh_data() const { return logMeshData; }
	private:
		bool logMeshData = false;
#endif

#ifdef AN_DEVELOPMENT
	public:
		static void show_debug_data(bool _show = true) { showDebugData = _show; }
		static void gather_debug_data() { gatherDebugData = true; }
		static bool does_show_debug_data() { return showDebugData; }
		static bool does_gather_debug_data() { return gatherDebugData; }
		DebugRendererFrame* get_debug_renderer_frame() const { return gatherDebugData? debugRendererFrame : nullptr; }
	private:
		static bool gatherDebugData;
		static bool showDebugData;
		DebugRendererFrame* debugRendererFrame = nullptr;
		DebugRendererFrame* suspendedGatherDebugData = nullptr;
	private:

	public: // as above
		static void show_generation_info(bool _show = true, bool _only = false) { showGenerationInfo = _show; showGenerationInfoOnly = _only; }
		static void gather_generation_info() { gatherGenerationInfo = true; }
		static bool does_show_generation_info() { return showGenerationInfo; }
		static bool does_show_generation_info_only() { return showGenerationInfo && showGenerationInfoOnly; }
		static bool does_gather_generation_info() { return gatherGenerationInfo; }
		MeshGeneration::GenerationInfo* get_generation_info() const { return generationInfo; }
	private:
		static bool gatherGenerationInfo;
		static bool showGenerationInfo;
		static bool showGenerationInfoOnly;
		MeshGeneration::GenerationInfo* generationInfo = nullptr;
#endif
	};
};
