#pragma once

#include "module.h"
#include "moduleAppearanceData.h"
#include "moduleEnums.h"
#include "..\appearance\appearanceControllerPtr.h"
#include "..\interfaces\collidable.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"
#include "..\..\core\mesh\mesh3dInstance.h"
#include "..\..\core\other\simpleVariableStorage.h"

// for inlines
#include "modulePresence.h"

#define AN_ADVANCE_VR_IMPORTANT_ONCE

#ifdef AN_ALLOW_BULLSHOTS
struct OptionalBoneTransform
{
	Optional<Vector3> translationLS;
	Optional<Rotator3> rotationLS;
	Optional<float> scaleLS;
};
#endif

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Meshes
{
	struct PoseMatBonesRef;
};

namespace Framework
{
	class AnimationSet;
	class AppearanceController;
	class DoorInRoom;
	class Room;
	class Mesh;
	class Skeleton;

	namespace Render
	{
		class Context;
		class PresenceLinkProxy;
	};

	/**
	 *	Only main appearance (first) is considered as "in-world" and this one provides collision (if has own)
	 *
	 *	Auto mesh generator is used during setup_with
	 */
	class ModuleAppearance
	: public Module
	{
		FAST_CAST_DECLARE(ModuleAppearance);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModuleAppearance> & register_itself();

		ModuleAppearance(IModulesOwner* _owner);
		virtual ~ModuleAppearance();

		bool is_main_appearance() const { return mainAppearance; }
		void be_main_appearance();

		bool should_be_visible_to_owner_only() const { return visibleToOwnerOnly; }
		
		bool is_visible() const { return visible; }
		void be_visible(bool _visible = true) { visible = _visible; }

		Tags const & get_tags() const { return tags; }
		Name const & get_name() const { return name; }
		bool does_require_syncing_to_other_apperance() const { return syncTo.is_valid(); }

		bool does_use_movement_collision_bounding_box_for_frustum_check() const { return useMovementCollisionBoundingBoxForFrustumCheck; }
		bool does_use_precise_collision_bounding_box_for_frustum_check() const { return usePreciseCollisionBoundingBoxForFrustumCheck; }
		int get_movement_collision_id() const { return usingMeshMovementCollisionId; }
		int get_precise_collision_id() const { return usingMeshPreciseCollisionId; }

		bool does_use_bones_bounding_box_for_frustum_check() const { return useBonesBoundingBoxForFrustumCheck; }
		Range3 const & get_bones_bounding_box() const { return bonesBoundingBox; }

		void use_another_mesh(); // only if has some other meshes to choose from
		void use_mesh(Mesh const * _mesh);
		void use_mesh_generator_on_setup(MeshGenerator const * _meshGenerator);
		void use_skeleton(Skeleton const * _skeleton);

		void generate_and_fuse(MeshGenerator * _meshGenerator, SimpleVariableStorage const * _usingVariables = nullptr); // creates new mesh and fuses it into our mesh

		float get_aggressive_lod() const { return aggressiveLOD; }
		float get_aggressive_lod_start() const { return aggressiveLODStart; }

		Meshes::Mesh3DInstance const& get_mesh_instance_for_lod(int _lod) const;
		Meshes::Mesh3DInstance const & get_mesh_instance() const { return meshInstance; }
		Meshes::Mesh3DInstance & access_mesh_instance() { return meshInstance; }
		Mesh * access_mesh() { return mesh.get(); }
		Mesh const * get_mesh() const { return mesh.get(); }
		Skeleton const * get_skeleton() const { return skeleton.get(); }
		Array<UsedLibraryStored<AnimationSet>> const& get_animation_sets() const { return animationSets; }

		void set_setup_mesh_generator_request(std::function<void(MeshGeneratorRequest & _request)> _setup_mesh_generator_request) { setup_mesh_generator_request = _setup_mesh_generator_request; }
		void recreate_mesh_with_mesh_generator(bool _changeAsSoonAsPossible = true); // read notes inside! (if _changeAsSoonAsPossible==false, will set pending mesh)
		void setup_with_pending_mesh();

		Meshes::Pose const * get_final_pose_LS() const { return finalPoseLS; }
		Meshes::Pose const * get_final_pose_MS(bool _allowPrev = true) const { an_assert(finalPoseMSUsable || _allowPrev); return finalPoseMSUsable ? finalPoseMS : prevFinalPoseMS; }
		Array<Matrix44> const & get_final_pose_mat_MS(bool _allowPrev = true) const { an_assert(finalPoseMSUsable || _allowPrev); return finalPoseMSUsable? finalPoseMatMS : prevFinalPoseMatMS; } // see note on finalPoseMatMS
		Meshes::PoseMatBonesRef get_final_pose_mat_MS_ref(bool _allowPrev = true) const;

#ifdef AN_ALLOW_BULLSHOTS
		Array<OptionalBoneTransform> & acccess_bullshot_final_pose_ls() { return bullshotFinalPoseLS; }
		void use_bullshot_final_pose(bool _use = true) { useBullshotFinalPose = _use; }
		Array<MaterialSetup>& access_bullshot_materials() { return bullshotMaterials; }
#endif

		void for_every_socket_starting_with(Name const& _socketPrefix, std::function<void(Name const& _socket)> _do) const;
		int find_socket_index(Name const& _socketName) const;
		bool has_socket(Name const & _socketName) const;
		Transform calculate_socket_os(Name const & _socketName, Meshes::Pose const *_usingPoseLS = nullptr) const; // by default final pose is used
		Transform calculate_socket_ms(Name const & _socketName, Meshes::Pose const *_usingPoseLS = nullptr) const; // by default final pose is used
		Transform calculate_socket_os(int _socketIdx, Meshes::Pose const *_usingPoseLS = nullptr) const; // by default final pose is used
		Transform calculate_socket_ms(int _socketIdx, Meshes::Pose const *_usingPoseLS = nullptr) const; // by default final pose is used

	public: // pois
		void process_mesh_pois(bool _force = false); // kind of hack, to get pois processed for actors (to spawn stuff etc) we may do this before object activation or at it
		void for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _fpoi, IsPointOfInterestValid _is_valid = nullptr) const;

	private:
		bool meshPOIsProcessed = false;

	public:
		void do_for_all_appearances(std::function<void(ModuleAppearance * _appearance)> _do);
		
		template<typename Class>
		void set_shader_param(Name const & _paramName, Class const & _value, int _materialIdx = NONE); // sets params in all materials or just in one

		template<typename Class>
		void set_shader_param(Name const & _paramName, Class const & _value, std::function<bool(::System::MaterialInstance const*)> _validateMaterialInstance); // sets params in all materials or just in one

		void copy_material_parameters();
		bool does_require_copying_material_parameters() const;

	public: // transforms
		inline Transform get_ms_to_ws_transform() const; // model/mesh space to world space
		inline Transform get_os_to_ws_transform() const; // object space to world space
		inline Transform get_ms_to_os_transform() const; // model/mesh space to object space
		// for inverted use to_local for transforms above - works as fast as to_world, inversion is skipped though

	public: // advancements
		// advance - begin
		static void advance__calculate_preliminary_pose(IMMEDIATE_JOB_PARAMS);
		static void advance__advance_controllers_and_adjust_preliminary_pose(IMMEDIATE_JOB_PARAMS);
		static void advance__calculate_final_pose_and_attachments(IMMEDIATE_JOB_PARAMS);
		static void advance__pois(IMMEDIATE_JOB_PARAMS);
		// advance - end

		static void advance_vr_important__controllers_and_attachments(IModulesOwner* _modulesOwner, float _deltaTime); // this is useful to handle after processing vr controls, advances controllers, readies for rendering

	public: // controllers related
		SimpleVariableStorage & access_controllers_variable_storage() { an_assert(get_owner()); return get_owner()->access_variables(); }
		Array<AppearanceControllerPtr> const & get_controllers() const { return controllers; }

	protected:
		static void calculate_preliminary_pose(IModulesOwner* _modulesOwner);
		static void advance_controllers_and_adjust_preliminary_pose(IModulesOwner* _modulesOwner, float _deltaTime, PoseAndAttachmentsReason::Type _reason);
		static void advance_controllers_and_adjust_preliminary_pose_and_attachments(IModulesOwner* _modulesOwner, float _deltaTime, PoseAndAttachmentsReason::Type _reason); // used for vr important
	protected: friend class ModulePresence;
		static void calculate_final_pose_and_attachments_0(IModulesOwner* _modulesOwner, float _deltaTime, PoseAndAttachmentsReason::Type _reason);

	protected:
		void advance_controllers_and_adjust_preliminary_pose(float _deltaTime);
		void calculate_final_pose(float _deltaTime, PoseAndAttachmentsReason::Type _reason);
		void ready_for_rendering();
		bool does_require_ready_for_rendering() const;
		Optional<float> const & get_ignore_rendering_beyond_distance() const { return ignoreRenderingBeyondDistance; }

	public:	// Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void initialise();
		override_ void ready_to_activate();
		override_ void activate();
		override_ bool does_require(Concurrency::ImmediateJobFunc _jobFunc) const;

	public:
		void debug_draw_skeleton() const;
		void debug_draw_sockets() const;
		void debug_draw_pois(bool _setDebugContext);

	private:
		bool mainAppearance = false;
		bool visible = true;
		bool visibleToOwnerOnly = false;
		Tags tags;
		Name name;
		Name syncTo; // synchronises bones to a different appearance
		ModuleAppearanceData const * moduleAppearanceData = nullptr;

#ifdef AN_ADVANCE_VR_IMPORTANT_ONCE
		bool doneAsVRImportant = false;
#endif

		// when creating with mesh generator
		LibraryName sharedMeshLibraryName;
		RangeInt sharedMeshLibraryNameSuffix = RangeInt::empty;

		UsedLibraryStored<MeshGenerator> meshGenerator; // if not set, uses one from data
		std::function<void(MeshGeneratorRequest & _request)> setup_mesh_generator_request = nullptr;

		bool useLODs = true;
		float aggressiveLOD = 0.0f; // how whole lod is more aggressive (>0 nearer, <0 further)
		float aggressiveLODStart = 0.0f; // when does it start (>0 nearer, <0 further)

		Meshes::Mesh3DInstance meshInstance; // location relative to presence's placement
		struct LODMeshInstance
		{
			bool bonesUpdated = false;
			bool requested = true;
			Meshes::Mesh3DInstance meshInstance; // location relative to presence's placement
			Array<int> remapBones;
		};
		Array<LODMeshInstance> lodMeshInstances;
		UsedLibraryStored<Mesh> mesh;
		UsedLibraryStored<Skeleton> skeleton;
		UsedLibraryStored<Mesh> pendingMesh;
		bool provideSkeletonToMeshGenerator = false;
		bool meshFromAppearanceDataMeshes = false;
		int meshFromAppearanceDataMeshesIdx = 0;
		
		Array<UsedLibraryStored<AnimationSet>> animationSets;

		int usingMeshMovementCollisionId = NONE;
		int usingMeshPreciseCollisionId = NONE;

		bool useMovementCollisionBoundingBoxForFrustumCheck = false; // movement collision
		bool usePreciseCollisionBoundingBoxForFrustumCheck = false; // precise collision
		bool useBonesBoundingBoxForFrustumCheck = false; // will calculate bones location
		float expandBonesBoundingBox = 0.1f; // make bounding box bigger
		Range3 bonesBoundingBox = Range3::empty;

		bool forceCalculateFinalPose = true;
		Meshes::Pose * preliminaryPoseLS;
		Meshes::Pose * workingFinalPoseLS; // will be swapped with final pose LS
		Meshes::Pose * finalPoseLS;
		Meshes::Pose * prevFinalPoseLS;
#ifdef AN_ALLOW_BULLSHOTS
		Array<OptionalBoneTransform> bullshotFinalPoseLS;
		bool useBullshotFinalPose = false;
		Array<MaterialSetup> bullshotMaterials;
#endif
		bool finalPoseMSUsable = true;

		Meshes::Pose * finalPoseMS; // is calculated at the end of final pose LS calculation! use it only afterwards!
		Array<Matrix44> finalPoseMatMS; // (see above)

		Meshes::Pose * prevFinalPoseMS; // might be used if we require during calculation of final pose
		Array<Matrix44> prevFinalPoseMatMS; // (see above)

		bool useControllers = true; // useful to switch off if we just sync to other appearance (then has to be explicitly set)
		Array<AppearanceControllerPtr> controllers; // chain of controllers

		ModuleAppearance* syncToAppearance; // if appearance changes, skeleton changes etc it should be reset
		UsedLibraryStored<Skeleton> syncToSkeleton;
		Array<int> syncToBoneMap; // NONE if should not sync this bone

		Array<PointOfInterestInstancePtr> pois;

		CACHED_ bool requireAutoProcessingOrderForControllers = true;

		Optional<float> ignoreRenderingBeyondDistance; // won't even try to render if beyond the limit

		void auto_processing_order_for_controllers_if_required() { if (requireAutoProcessingOrderForControllers) auto_processing_order_for_controllers(); }
		void auto_processing_order_for_controllers();

		void clear_controllers();
		void clear_mesh();
		void clear_skeleton();
		void clear_animation_sets();

		void update_sync_to();
		void sync_from(Meshes::Pose * _dest, Meshes::Pose const * _source);

		void initialise_not_initialised_controllers();
		void activate_inactive_controllers();

		void update_collision_module_if_initialised();

		void setup_with_just_generated_mesh(Mesh const* _mesh); // this will setup skeleton, mesh and other things

		void use_mesh_internal(Mesh const* _mesh);
		void add_controllers_from_data();

		void create_mesh_with_mesh_generator_internal(bool _force = false, bool _changeAsSoonAsPossible = true); // read notes inside!

		inline void copy_material_param(Name const & _var, TypeId _varType, void const * _value);
		template <typename Class>
		inline bool copy_material_param(Name const& _var, TypeId _varType, void const* _value);

		void update_ra_pose_advance(float _deltaTime);

		void update_mesh_instance_individual_offset();

		friend interface_class IModulesOwner;
		friend class Render::PresenceLinkProxy;
	};

	#include "moduleAppearance.inl"
};

