#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\mesh\boneRenamer.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\system\video\indexFormat.h"
#include "..\..\core\system\video\vertexFormat.h"
#include "..\..\core\wheresMyPoint\wmp.h"
#include "..\..\core\random\random.h"

#include "meshGenElement.h"
#include "meshGenTypes.h"

#include "..\appearance\appearanceControllerPtr.h"
#include "..\appearance\meshNode.h"
#include "..\video\material.h"

#include "..\world\pointOfInterestTypes.h"
#include "..\world\spaceBlocker.h"

#include "..\..\core\other\registeredType.h"

struct DebugRendererFrame;

namespace Collision
{
	class Model;
};

namespace Meshes
{
	class Mesh3D;
	class Skeleton;
	class SocketDefinitionSet;
	struct Mesh3DPart;
};

namespace Framework
{
	class Skeleton;
	struct MaterialSetup;

	namespace MeshGeneration
	{
		struct ElementInstance;
		struct GenerationInfo;

		struct Bone
		{
			Name boneName;
			Name parentName;
			Transform placement = Transform::identity;
		};

		struct GenerationContext
		: public WheresMyPoint::IOwner
		{
			FAST_CAST_DECLARE(GenerationContext);
			FAST_CAST_BASE(WheresMyPoint::IOwner);
			FAST_CAST_END();

			typedef WheresMyPoint::IOwner base;
		public:
			GenerationContext(WheresMyPoint::IOwner* _wmpOwner);
			~GenerationContext();

		public: // WheresMyPoint::IOwner
			override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
			override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
			override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
			override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
			override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
			override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;

			override_ WheresMyPoint::IOwner* get_wmp_owner() { return wmpOwner; }

		public:
			// preparation
			void use_random_generator(Random::Generator _generator) { randomGeneratorStack.get_last() = _generator; }

			void setup_for_skinned();
			void setup_for_static();
			
			bool is_mesh_static() const;

			void for_skeleton(::Meshes::Skeleton const * _skeleton);

			void use_vertex_format(::System::VertexFormat const & _vertexFormat);
			void use_index_format(::System::IndexFormat const & _indexFormat);

			void set_allow_vertices_optimisation(bool _allowVerticesOptimisation = true) { allowVerticesOptimisation = _allowVerticesOptimisation; }

			void set_force_more_details(bool _forceMoreDetails = true) { forceMoreDetails = _forceMoreDetails; }
			bool does_force_more_details() const { return forceMoreDetails; }

			void set_for_lod(int _forLOD) { forLOD = _forLOD; }
			int get_for_lod() const { return forLOD; }
			bool should_generate_at_lod(int _level) const { return forLOD <= _level; }

			void set_use_lod_fully(float _useLODFully) { useLODFully = _useLODFully; }
			float get_use_lod_fully() const { return useLODFully; }

			void set_1_as_sub_step_base(float _use1AsSubStepBase) { use1AsSubStepBase = _use1AsSubStepBase; }
			float get_1_as_sub_step_base() const { return use1AsSubStepBase; }

			void set_include_stack_processor(IncludeStackProcessorFunc _include_stack_processor) { include_stack_processor = _include_stack_processor; }
			MeshGenerator * run_include_stack_processor() const { return include_stack_processor? include_stack_processor(*this) : nullptr; }
			void push_include_stack(Name const& _element) { includeStack.push_back(_element); }
			void pop_include_stack() { includeStack.pop_back(); }
			Array<Name> const& get_include_stack() const { return includeStack; }
			String const get_include_stack_as_string() const;

			void set_require_movement_collision(bool _requires = true) { requiresMovementCollision = _requires; }
			bool does_require_movement_collision() const { return requiresMovementCollision; }

			void set_require_precise_collision(bool _requires = true) { requiresPreciseCollision = _requires; }
			bool does_require_precise_collision() const { return requiresPreciseCollision; }

			void set_require_space_blockers(bool _requires = true) { requiresSpaceBlockers = _requires; }
			bool does_require_space_blockers() const { return requiresSpaceBlockers; }

			void set_require_appearance_controllers(bool _requires = true) { requiresAppearanceControllers = _requires; }
			bool does_require_appearance_controllers() const { return requiresAppearanceControllers; }

			void use_material_setups(Array<MaterialSetup> const & _materialSetups) { materialSetups = _materialSetups; }
			Array<MaterialSetup> const & get_material_setups() const { return materialSetups; }

		public: // process
			bool process(Element const * _element, ElementInstance * _parent, int _idx = 0);
			void finalise(); // combine parts etc

		public:
			void log(LogInfoContext& _log) const;

		public: // utils for generators
			int get_material_index_for_part(::Meshes::Mesh3DPart* _part) const;
			int store_part(::Meshes::Mesh3DPart* _part, int _materialIdx);
			int store_part(::Meshes::Mesh3DPart* _part, ElementInstance & _instance, Optional<int> const& _overrideMaterialIdx = NP);
			int store_part_just_as(Meshes::Mesh3DPart* _part, Meshes::Mesh3DPart const * _existingPart);
			void change_material_index_for_part(::Meshes::Mesh3DPart* _part, int _changeToMaterialIdx);
			void store_movement_collision_part(::Collision::Model* _part);
			void store_precise_collision_part(::Collision::Model* _part);
			void store_movement_collision_parts(Array<::Collision::Model*> const & _parts);
			void store_precise_collision_parts(Array<::Collision::Model*> const & _parts);
			void ignore_part_for_collision(Meshes::Mesh3DPart const * _part); // do not create collision basing on it
			bool should_use_part_for_collision(Meshes::Mesh3DPart const* _part) const;
			void store_space_blocker(SpaceBlocker const & _sb);

		public:
			Checkpoint const & get_checkpoint(int _ancestor = 0) const { return checkpoints[max(0, checkpoints.get_size() - 1 - _ancestor)]; }
			void push_checkpoint(Checkpoint const & _checkpoint) { checkpoints.push_back(_checkpoint); }
			void pop_checkpoint() { checkpoints.pop_back(); }

			NamedCheckpoint& access_named_checkpoint(Name const& _id);

			Random::Generator & access_random_generator() { return randomGeneratorStack.get_last(); }
			void advance_random_generator(int _seedBase, int _seedOffset) { auto & r = access_random_generator(); r.advance_seed(_seedBase, _seedOffset); }
			Random::Generator const & get_random_generator() const { return randomGeneratorStack.get_last(); }
			void push_random_generator_stack() { an_assert( !randomGeneratorStack.is_empty()); randomGeneratorStack.push_back(randomGeneratorStack.get_last()); }
			void pop_random_generator_stack() { randomGeneratorStack.pop_back(); an_assert(!randomGeneratorStack.is_empty()); }

			::System::VertexFormat const & get_vertex_format() const { return vertexFormat; }
			::System::IndexFormat const & get_index_format() const { return indexFormat; } 

			::Meshes::Skeleton const * get_provided_skeleton() const { return providedSkeleton; }

			::Meshes::Skeleton* get_generated_skeleton() const { return generatedSkeleton; }
			::Meshes::Skeleton* consume_generated_skeleton() { an_assert(finalised); ::Meshes::Skeleton* consumed = generatedSkeleton; generatedSkeleton = nullptr; return consumed; }

			::Meshes::Mesh3D* get_generated_mesh() const { return mesh; }
			::Meshes::Mesh3D* consume_generated_mesh() { an_assert(finalised); ::Meshes::Mesh3D* consumed = mesh; mesh = nullptr; return consumed; }

			int get_current_sockets_generation_id() const { return socketsGenerationId; }
			int get_new_sockets_generation_id() { return ++socketsGenerationId; }
			::Meshes::SocketDefinitionSet & access_sockets() { an_assert(sockets); return *sockets; }
			::Meshes::SocketDefinitionSet const & get_sockets() const { an_assert(sockets); return *sockets; }
			int find_socket(Name const& _socketName) const;
			Transform calculate_socket_placement_ms(int _idx) const;

			Array<AppearanceControllerDataPtr> & access_appearance_controllers() { return appearanceControllers; }
			Array<AppearanceControllerDataPtr> const & get_appearance_controllers() const { return appearanceControllers; }

			::Collision::Model* get_generated_movement_collision() const { return movementCollision; }
			::Collision::Model* consume_generated_movement_collision() { an_assert(finalised); ::Collision::Model* consumed = movementCollision; movementCollision = nullptr; return consumed; }

			::Collision::Model* get_generated_precise_collision() const { return preciseCollision; }
			::Collision::Model* consume_generated_precise_collision() { an_assert(finalised); ::Collision::Model* consumed = preciseCollision; preciseCollision = nullptr; return consumed; }

			Array<RefCountObjectPtr<::Meshes::Mesh3DPart>> const & get_parts() const { return parts; } // parts themselves may be modified, array shouldn't be - instead of removing, clear a part
			Array<::Collision::Model*> const & get_movement_collision_parts() const { return movementCollisionParts; } // parts themselves may be modified, array shouldn't be - instead of removing, clear a part
			Array<::Collision::Model*> const & get_precise_collision_parts() const { return preciseCollisionParts; } // parts themselves may be modified, array shouldn't be - instead of removing, clear a part

			Array<MeshNodePtr> const & get_mesh_nodes() const { return meshNodes; }
			Array<MeshNodePtr> & access_mesh_nodes() { return meshNodes; }

			Array<PointOfInterestPtr> const & get_pois() const { return pois; }
			Array<PointOfInterestPtr> & access_pois() { return pois; }

			SpaceBlockers const & get_space_blockers() const { return spaceBlockers; }
			SpaceBlockers& access_space_blockers() { return spaceBlockers; }

			// current (top of stack)
			template <typename Class>
			inline Class const * get_parameter(Name const & _name) const;
			template <typename Class>
			inline Class const* get_global_parameter(Name const& _name) const;

			template <typename Class>
			inline void set_parameter(Name const & _name, Class const & _value);
			template <typename Class>
			inline void set_global_parameter(Name const & _name, Class const & _value);

			template <typename Class>
			inline void invalidate_parameter(Name const & _name);

			inline bool has_any_parameter(Name const & _name) const;
			inline bool get_any_parameter(Name const & _name, OUT_ TypeId & _id, OUT_ void const * & _value) const;
			inline bool has_parameter(Name const & _name, TypeId _id) const;
			inline bool get_parameter(Name const & _name, TypeId _id, void * _into) const;
			inline void set_parameter(Name const & _name, TypeId _id, void const * _value, int _ancestor = 0);
			inline bool convert_parameter(Name const& _name, TypeId _id);
			inline bool rename_parameter(Name const& _from, Name const &_to);

			inline void set_parameters(SimpleVariableStorage const & _parameters);

			// global (bottom of stack)
			inline bool get_any_global_parameter(Name const & _name, OUT_ TypeId & _id, OUT_ void const * & _value) const;
			inline void set_global_parameter(Name const & _name, TypeId _id, void const * _value);

		private: friend struct ElementInstance;
			// allow to drop parts
			Array<::Collision::Model*> & access_movement_collision_parts() { return movementCollisionParts; }
			Array<::Collision::Model*> & access_precise_collision_parts() { return preciseCollisionParts; }

		public:
			bool has_skeleton() const { return providedSkeleton || !bones.is_empty(); }
			int get_bones_count() const;
			Name get_bone_name(int _index) const;
			Transform get_bone_placement_MS(int _index) const;
			int32 find_bone_index(Name const & _boneName) const;
			int get_children_bone(int _index, OUT_ REF_ Array<int>& _childrenBones, bool _andDescendants = false) const;
			int32 get_bone_parent(int32 _index) const;
			Name const & find_skeleton_bone_name(int32 _boneIdx) const; // doesn't go through resolvers

			Array<Bone> & access_generated_bones() { return bones; }
			Array<Bone> const & get_generated_bones() const { return bones; }
			Bone const * get_generated_bone(Name const & _boneName) const;
			bool add_generated_bone(Name const & _boneName, ElementInstance & _instanceInstigator, OPTIONAL_ OUT_ int * _boneIdx = nullptr); // bone name should be already resolved!
			bool get_generated_bone_if_exists(Name const & _boneName, OPTIONAL_ OUT_ int * _boneIdx = nullptr); // bone name should be already resolved!
			Name resolve_bone(Name const & _boneName) const; // resolve bone name using bone renamer

			void set_current_parent_bone(Name const & _currentParent);
			Name const & get_current_parent_bone() const;

		public:
			// managing state of things that may change
			void push_stack();
			void pop_stack();

			void use_bone_renamer(::Meshes::BoneRenamer const & _boneRenamer);

			void push_element(Element const * _element);
			void pop_element();
			Element const* get_current_element() const { an_assert(!elementStack.is_empty()); return elementStack.get_last(); }

#ifdef AN_DEVELOPMENT
		public:
			struct PerformanceForElement
			{
				int depth;
				Element const * element;
				float timeTotal;
				float timeProcess; // only process
			};
			Array<PerformanceForElement> performanceForElements;
			int performanceForElementsDepth = 0;

			int add_performance_for_element(Element const * _element);
			void end_performance_for_element();

			String get_performance_for_elements() const;
#endif

		private:
#ifdef AN_ASSERT
			bool finalised = false;
#endif
			bool allowVerticesOptimisation = true;

			WheresMyPoint::IOwner* wmpOwner = nullptr;

			Array<Checkpoint> checkpoints; // checkpoints for element stack for current element (created when processing current element has started)
			Array<NamedCheckpoint> namedCheckpoints; // global names!

			int socketsGenerationId = 0;
			Meshes::SocketDefinitionSet* sockets = nullptr;

			::Meshes::Skeleton const * providedSkeleton = nullptr; // provided has priority over generated - should not be allowed to add bones if provided
			::Meshes::Skeleton * generatedSkeleton = nullptr;

			IncludeStackProcessorFunc include_stack_processor = nullptr;
			Array<Name> includeStack;

			Array<Random::Generator> randomGeneratorStack;

			Array<MaterialSetup> materialSetups;

			::Meshes::Mesh3D* mesh;
			Array<Array<::Meshes::Mesh3D::FuseMesh3DPart>> partsByMaterials; // by materials
			Array<RefCountObjectPtr<::Meshes::Mesh3DPart>> parts; // all added parts
			Array<RefCountObjectPtr<::Meshes::Mesh3DPart>> doNotUsePartsForCollision;

			bool forceMoreDetails = false;
			int forLOD = 0;
			float useLODFully = 1.0f;
			float use1AsSubStepBase = 0.0f; // if this is set to 1.0, sub step first modifier is 1.0 - always!

			bool requiresMovementCollision = false;
			::Collision::Model* movementCollision;
			Array<::Collision::Model*> movementCollisionParts; // will be combined into one movement collision

			bool requiresPreciseCollision = false;
			::Collision::Model* preciseCollision;
			Array<::Collision::Model*> preciseCollisionParts; // will be combined into one precise collision

			Array<MeshNodePtr> meshNodes;
			
			bool requiresSpaceBlockers = false;
			Array<PointOfInterestPtr> pois;

			SpaceBlockers spaceBlockers;

			Array<Bone> bones;

			bool requiresAppearanceControllers = false;
			Array<AppearanceControllerDataPtr> appearanceControllers; // this is actual data to create controllers

			::System::VertexFormat vertexFormat; // common for all parts
			::System::IndexFormat indexFormat; // common for all parts

		private:
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		public:
#endif
			struct StackElement
			: public SoftPooledObject<StackElement>
			{
				// --- copied along the stack
				SimpleVariableStorage parameters; // all, including element's own!
				Name currentParentBone;

				// --- those are individual per stack element (we travel along stack to use them all)
				::Meshes::BoneRenamer boneRenamer; // these are used when creating bones or skinning to them etc

			protected: friend class SoftPooledObject<StackElement>;
				override_ void on_get();
				override_ void on_release();
			};
		private:
			//
			Array<StackElement*> stack;

			Array<Element const*> elementStack;

#ifdef AN_DEVELOPMENT
		public:
			DebugRendererFrame* debugRendererFrame = nullptr;
			GenerationInfo* generationInfo = nullptr; // dimensions, sizes, info for preview
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		bool should_log_mesh_data() const { return logMeshData; }
		void set_log_mesh_data(bool _logMeshData) { logMeshData = _logMeshData; }
	private:
		bool logMeshData = false;
#endif

		};

		#include "meshGenGenerationContext.inl"
	};
};

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
TYPE_AS_CHAR(Framework::MeshGeneration::GenerationContext::StackElement);
#endif
