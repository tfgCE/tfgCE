#pragma once

#include "doorVRCorridor.h"
#include "environmentUsage.h"
#include "pointOfInterestInstance.h"
#include "roomGeneratorInfoSet.h"
#include "roomPartInstance.h"
#include "roomType.h"
#include "spaceBlocker.h"
#include "worldObject.h"

#include "..\collision\againstCollision.h"
#include "..\collision\checkCollisionContext.h"
#include "..\collision\checkSegmentResult.h"
#include "..\interfaces\renderable.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"
#include "..\sound\soundSources.h"

#include "..\..\core\lifetimeIndicator.h"
#include "..\..\core\collision\modelInstanceSet.h"
#include "..\..\core\concurrency\mrswLock.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\mesh\mesh3dInstanceSet.h"
#include "..\..\core\pieceCombiner\pieceCombiner.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"
#include "..\..\core\vr\vrZone.h"

#include <functional>

#include "..\debugSettings.h"

//

#ifdef AN_DEVELOPMENT
	#ifdef WITH_HISTORY_FOR_OBJECTS
		#ifndef WITH_ROOM_HISTORY
			#define WITH_ROOM_HISTORY
		#endif
	#endif
	#ifndef WITH_ROOM_HISTORY
		//#define WITH_ROOM_HISTORY
	#endif
#endif

#ifdef WITH_ROOM_HISTORY
#define ROOM_HISTORY(room, ...) room->add_history(String::printf(__VA_ARGS__))
#else
#define ROOM_HISTORY(room, ...)
#endif

//

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Meshes
{
	class CombinedMesh3DInstanceSet;
};

namespace VR
{
	struct Zone;
};

// use these when not allowed to get_objects/get_doors etc (when in async or when a function can be called from different parts of code). locks for a moment

#define ROOM_DOORS_ON_STACK(arrayName, room) \
	int temp_variable_named(doorCount); \
	auto temp_variable_named(r) = (room); \
	temp_variable_named(r)->get_doors(temp_variable_named(doorCount)); \
	ARRAY_STACK(Framework::DoorInRoom*, arrayName, temp_variable_named(doorCount)); \
	temp_variable_named(r)->get_doors(temp_variable_named(doorCount), &arrayName); \

#define FOR_EVERY_DOOR_IN_ROOM(door, room) \
	ROOM_DOORS_ON_STACK(temp_variable_named(doorArray), room); \
	for_every_ptr(door, temp_variable_named(doorArray))

#define ROOM_ALL_DOORS_ON_STACK(arrayName, room) \
	int temp_variable_named(doorCount); \
	auto temp_variable_named(r) = (room); \
	temp_variable_named(r)->get_all_doors(temp_variable_named(doorCount)); \
	ARRAY_STACK(Framework::DoorInRoom*, arrayName, temp_variable_named(doorCount)); \
	temp_variable_named(r)->get_all_doors(temp_variable_named(doorCount), &arrayName); \

#define FOR_EVERY_ALL_DOOR_IN_ROOM(door, room) \
	ROOM_ALL_DOORS_ON_STACK(temp_variable_named(doorArray), room); \
	for_every_ptr(door, temp_variable_named(doorArray))

#define ROOM_OBJECTS_ON_STACK(arrayName, room) \
	int temp_variable_named(objectCount); \
	auto temp_variable_named(r) = (room); \
	temp_variable_named(r)->get_objects(temp_variable_named(objectCount)); \
	ARRAY_STACK(Framework::Object*, arrayName, temp_variable_named(objectCount)); \
	temp_variable_named(r)->get_objects(temp_variable_named(objectCount), &arrayName); \

#define FOR_EVERY_OBJECT_IN_ROOM(object, room) \
	ROOM_OBJECTS_ON_STACK(temp_variable_named(objectArray), room); \
	for_every_ptr(object, temp_variable_named(objectArray))

#define ROOM_ALL_OBJECTS_ON_STACK(arrayName, room) \
	int temp_variable_named(objectCount); \
	auto temp_variable_named(r) = (room); \
	temp_variable_named(r)->get_all_objects(temp_variable_named(objectCount)); \
	ARRAY_STACK(Framework::Object*, arrayName, temp_variable_named(objectCount)); \
	temp_variable_named(r)->get_all_objects(temp_variable_named(objectCount), &arrayName); \

#define FOR_EVERY_ALL_OBJECT_IN_ROOM(object, room) \
	ROOM_ALL_OBJECTS_ON_STACK(temp_variable_named(objectArray), room); \
	for_every_ptr(object, temp_variable_named(objectArray))

#define ROOM_TEMPORARY_OBJECTS_ON_STACK(arrayName, room) \
	int temp_variable_named(temporaryObjectCount); \
	auto temp_variable_named(r) = (room); \
	temp_variable_named(r)->get_temporary_objects(temp_variable_named(temporaryObjectCount)); \
	ARRAY_STACK(Framework::TemporaryObject*, arrayName, temp_variable_named(temporaryObjectCount)); \
	temp_variable_named(r)->get_temporary_objects(temp_variable_named(temporaryObjectCount), &arrayName); \

#define FOR_EVERY_TEMPORARY_OBJECT_IN_ROOM(temporaryObject, room) \
	ROOM_TEMPORARY_OBJECTS_ON_STACK(temp_variable_named(temporaryObjectArray), room); \
	for_every_ptr(temporaryObject, temp_variable_named(temporaryObjectArray))

namespace Framework
{
	class Door;
	class DoorInRoom;
	class DoorTypeReplacer;
	class EnvironmentOverlay;
	class World;
	class Region;
	class RoomType;
	class SubWorld;
	class IModulesOwner;
	class Object;
	class PhysicalMaterial;
	class PresenceLink;
	class Material;
	class Mesh;
	class ModulePresence;
	class TemporaryObject;
	class Environment;
	class Reverb;
	class Room;
	class RoomPart;
	class RoomPartInstance;
	class RoomGeneratorInfo;
	struct PresenceLinkBuildContext;
	struct DoorInRoomArrayPtr;
	struct WorldAddress;

	namespace Nav
	{
		class Mesh;
		class System;
		struct PlacementAtNode;
	};

	namespace Render
	{
		class RoomProxy;
		class EnvironmentProxy;
	};

	struct FindRoomContext
	{
		FindRoomContext& only_visible(bool _onlyVisible = true) { onlyVisible = _onlyVisible; return *this; }
		FindRoomContext& with_depth_limit(Optional<int> const& _o) { depthLimit = _o; return *this; }
		FindRoomContext& with_max_distance(Optional<float> const& _o) { maxDistance = _o; return *this; }
		FindRoomContext& with_max_distance_not_visible(Optional<float> const& _o) { maxDistanceNotVisible = _o; return *this; }

		Optional<bool> onlyVisible;
		Optional<int> depthLimit;
		Optional<float> maxDistance;
		Optional<float> maxDistanceNotVisible;
	};

	struct FoundRoom
	{
		Room* room;
		FoundRoom* enteredFrom;
		DoorInRoom* enteredThroughDoor;
		::System::ViewFrustum viewFrustum;
		Transform originPlacement; // in room's space
		Vector3 entrancePoint; // to make it easier to approximate distance
		float distance; // this is not actual distance, this is distance used to sort which rooms should be checked, this is distance to doors (beyond enteredThroughDoor)
		float distanceNotVisible;
		int depth;
		bool scanned;
		bool visible;

		FoundRoom() {}
		FoundRoom(Room* _room, FoundRoom* _enteredFrom, DoorInRoom* _enteredThroughDoor, float _distance, float _distanceNotVisible, int _depth, bool _visible);
		FoundRoom(Room* _room, Vector3 const& _location, bool _visible);
		FoundRoom(Room* _room, Transform const& _placement, bool _visible);

		static void find(REF_ ArrayStack<FoundRoom>& _rooms, Room* _startRoom, Vector3 const& _startLoc, FindRoomContext& _context);
		static void find(REF_ ArrayStack<FoundRoom>& _rooms, Room* _startRoom, Transform const& _startPlacement, FindRoomContext& _context);
	private:
		static void scan_room(FoundRoom& _room, REF_ ArrayStack<FoundRoom>& _roomsToScan, FindRoomContext& _context);
	};

	class Room
	: public SafeObject<Room>
	, public WorldObject
	, public Collision::ICollidableObject
	, public WheresMyPoint::IOwner
	{
		FAST_CAST_DECLARE(Room);
		FAST_CAST_BASE(WorldObject);
		FAST_CAST_BASE(Collision::ICollidableObject);
		FAST_CAST_BASE(WheresMyPoint::IOwner);
		FAST_CAST_END();

#ifdef DEBUG_WORLD_LIFETIME_INDICATOR
	public:
		LifetimeIndicator __lifetimeIndicator;
#endif

	public:
		static void initialise_static();
		static void close_static();
		static void before_building_presence_links();
		static Array<Room*> const & get_rooms_to_combine_presence_links_post_building_presence_links();

	public:
		Room(SubWorld* _inSubWorld, Region* _inRegion, RoomType const * _roomType, Random::Generator const& _rg);
		virtual ~Room();

		void set_origin_room(Room* _room);
		Room * get_origin_room() { return originRoom.is_set()? originRoom.get() : this; } // origin room or us
		Room const * get_origin_room() const { return originRoom.is_set()? originRoom.get() : this; } // origin room or us

		void mark_objects_no_longer_advanceable_due_to_room_destruction();
		bool are_objects_no_longer_advanceable() const { return objectsNoLongerAdvanceable; }

		bool is_small_room() const { return smallRoom; }

		void mark_to_be_deleted() { deletionPending = true; mark_objects_no_longer_advanceable_due_to_room_destruction(); }
		bool is_deletion_pending() const { return deletionPending; }

		//bool is_important_to_player () const { return AVOID_CALLING_ACK_ get_distance_to_recently_seen_by_player() < 2; }

		void set_always_suspend_advancement(bool _alwaysSuspendAdvancement = true) { alwaysSuspendAdvancement = _alwaysSuspendAdvancement; }
		bool should_always_suspend_advancement() const { return alwaysSuspendAdvancement; }

		void sync_destroy_all_doors();

		void set_name(String const & _name) { name = _name; }
		String const & get_name() const { return name; }

		void set_world_address_id(int _id) { worldAddressID = _id; }
		int get_world_address_id() const { return worldAddressID; }
		WorldAddress get_world_address() const;

		RoomType const * get_room_type() const { return roomType; }
		void set_room_type(RoomType const * _roomType);
		Tags & access_tags() { return tags; }
		Tags const & get_tags() const { return tags; }

		void mark_POIs_inside(Tags const & _tags);
		Tags const & get_mark_POIs_inside() const { return markPOIsInside; }

		void add_volumetric_limit_poi(Vector3 const & _at);
		void add_volumetric_limit_pois(Range3 const & _bbox);
		void add_poi(PointOfInterest* _poi, Transform const & _placement);

		DoorTypeReplacer* get_door_type_replacer() const;

		void clear_inside();

		bool is_vr_arranged() const { return isVRArranged; }
		bool is_mesh_generated() const { return isMeshGenerated; }
		bool is_generated() const { return is_vr_arranged() && is_mesh_generated(); }
		void mark_vr_arranged();
		void mark_mesh_generated();
		void reset_generation(); // useful when failed and wants to restart
		bool start_generation(); // returns true if able to continue
		void end_generation(bool _markGenerated = false); // true will call mark_vr_arranged and mark_generated

		inline SubWorld* get_in_sub_world() const { return inSubWorld; }
		inline Region* get_in_region() const { return inRegion; }
		bool is_in_region(Region* _region) const; // whole region chain

		void for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _fpoi, Optional<bool> const& _includeObjects = NP, IsPointOfInterestValid _is_valid = nullptr) const;
		bool find_any_point_of_interest(Optional<Name> const& _poiName, OUT_ PointOfInterestInstance*& _outPOI, Optional<bool> const& _includeObjects = NP, Random::Generator* _randomGenerator = nullptr, IsPointOfInterestValid _is_valid = nullptr);
		Door* find_door_tagged(Name const & _tag);

		void ready_for_game(); // starts in this room, propagates anchors, creates corridors etc. check RegionGenerator for more info
		Transform get_vr_anchor(Optional<Transform> const & _forPlacement = Transform::identity, Optional<Transform> const& _forVRPlacement = NP) const; // forPlacement is just to find the best anchor
		bool set_door_vr_placement_if_not_set(DoorInRoom* _dir = nullptr, ShouldAllowToFail _allowToFailSilently = DisallowToFail); // for all doors that do not have vr placement set, updates it, returns true if was required
		bool set_door_vr_placement_for(DoorInRoom* _dir, ShouldAllowToFail _allowToFailSilently = DisallowToFail);

		SimpleVariableStorage const & get_variables() const { return variables; }
		SimpleVariableStorage & access_variables() { return variables; }
		void collect_variables(REF_ SimpleVariableStorage & _variables) const;

		// has to include roomRegionVariables.inl
		// up means that first we do own and then proceed up (room is least important), for down we start from the top (room is most important)
		template <typename Class>
		void on_each_variable_up(Name const & _var, std::function<void(Class const &)> _do) const;
		template <typename Class>
		void on_each_variable_down(Name const & _var, std::function<void(Class const &)> _do) const;
		template <typename Class>
		void on_own_variable(Name const & _var, std::function<void(Class const &)> _do) const;

		template <typename Class>
		Class const * get_variable(Name const& _var, bool _checkOriginRoom = true) const;

		Random::Generator const & get_individual_random_generator() const { return individualRandomGenerator; }
		void set_individual_random_generator(Random::Generator const & _individualRandomGenerator, bool _lock = true);

		SoundSources const & get_sounds() const { return sounds; }
		SoundSources & access_sounds() { return sounds; }

		RoomPartInstance* add_room_part(RoomPart const * _roomPart, Transform const & _placement);
		void add_mesh(Mesh const * _mesh, Transform const & _placement = Transform::identity, OUT_ Meshes::Mesh3DInstance** _meshInstance = nullptr, OUT_ int* _movementCollisionId = nullptr, OUT_ int* _preciseCollisionId = nullptr);
		void apply_replacer_to_mesh(REF_ Mesh const* & _mesh) const;
		void combine_appearance();
		void update_bounding_box();
		Meshes::Mesh3DInstanceSet & access_appearance() { return appearance; }
		Meshes::Mesh3DInstanceSet const & get_appearance() const { return appearance; }
		Meshes::Mesh3DInstanceSet & access_appearance_for_rendering();
		Meshes::Mesh3DInstanceSet const & get_appearance_for_rendering() const;
		Mesh* find_used_mesh_for(Meshes::IMesh3D const * _mesh) const;
		Collision::ModelInstanceSet const & get_against_collision(AgainstCollision::Type _againstCollision) const { return _againstCollision == AgainstCollision::Movement ? get_movement_collision() : get_precise_collision(); }
		Collision::ModelInstanceSet & access_movement_collision() { return movementCollision; }
		Collision::ModelInstanceSet const & get_movement_collision() const { return movementCollision; }
		Collision::ModelInstanceSet & access_precise_collision() { return preciseCollision; }
		Collision::ModelInstanceSet const & get_precise_collision() const { return preciseCollision.is_empty() ? movementCollision : preciseCollision; }
		void mark_requires_update_bounding_box() { requiresUpdateBoundingBox = true; }
		bool get_doors(int & _doorCount, ArrayStack<DoorInRoom*>* _doors = nullptr) const; // locks for a moment
		bool get_all_doors(int & _doorCount, ArrayStack<DoorInRoom*>* _doors = nullptr) const; // locks for a moment
		bool get_objects(int & _objectCount, ArrayStack<Object*>* _objects = nullptr) const; // locks for a moment
		bool get_all_objects(int & _objectCount, ArrayStack<Object*>* _objects = nullptr) const; // locks for a moment
		bool get_temporary_objects(int & _objectCount, ArrayStack<TemporaryObject*>* _objects = nullptr) const; // locks for a moment
		Array<DoorInRoom*> const & get_doors() const { assert_we_dont_modify_doors(); return doors; }
		Array<DoorInRoom*> const & get_all_doors() const { return allDoors; }
		Array<Object*> const & get_objects() const { assert_we_dont_modify_objects(); return objects; }
		Array<Object*> const & get_all_objects() const { assert_we_dont_modify_objects(); return allObjects; }
		Array<TemporaryObject*> const & get_temporary_objects() const { assert_we_dont_modify_objects(); return temporaryObjects; }
		PresenceLink const * get_presence_links() const { return presenceLinks; }
		int count_presence_links() const;

		void invalidate_presence_links(); // all in room

		void mark_check_if_doors_are_too_close(bool _check = true) { checkIfDoorsAreTooClose = _check; } // some generators may require such check to be done

		void set_room_for_rendering_additional_objects(Room* _room) { renderObjectsFromRoom = _room; }
		Room* get_room_for_rendering_additional_objects() const { return renderObjectsFromRoom.get(); }

		PredefinedRoomOcclusionCulling const& get_predefined_occlusion_culling() const { return predefinedRoomOcclusionCulling; }

		::System::ShaderProgramBindingContext const & get_shader_program_binding_context() const { return shaderProgramBindingContext; }
		::System::ShaderProgramBindingContext & access_shader_program_binding_context() { return shaderProgramBindingContext; }

		enum MoveThroughDoorsFlag
		{
			ForceMoveThrough = bit(1) // closed, can't move through etc
		};
		// inNewRoomPlacement = _intoRoomTransform.to_local(nextPlacement)
		bool move_through_doors(Transform const & _placement, Transform const & _nextPlacement, OUT_ Room *& _intoRoom, OUT_ Transform & _intoRoomTransform, OUT_ DoorInRoom ** _exitThrough = nullptr, OUT_ DoorInRoom ** _enterThrough = nullptr, OUT_ DoorInRoomArrayPtr * _throughDoors = nullptr, int _moveThroughFlags = 0) const;
		bool move_through_doors(Transform const & _placement, REF_ Transform & _nextPlacement, REF_ Room *& _intoRoom, int _moveThroughFlags = 0) const; // just moves into other room

		virtual bool generate(); // this is generating only this room
		void generate_environments(); // can happen only during generation
		bool generate_room_using_room_generator(REF_ RoomGeneratingContext& _roomGeneratingContext); // can happen only during generation
		
		void check_if_doors_valid(Optional<VR::Zone> _beInZone = NP) const;

		void set_room_generator_info(RoomGeneratorInfo const* _rgi) { roomGeneratorInfo = _rgi; }

#ifdef AN_DEVELOPMENT_OR_PROFILER
		RoomGeneratorInfo const * get_generated_with_room_generator() const { return generatedWithRoomGenerator; }
#endif

		// during generation, when POIs are yet unavailable, they may be stored as pending
		bool place_door_on_poi(DoorInRoom* _dir, Name const& _poi);
		bool place_door_on_poi_tagged(DoorInRoom* _dir, TagCondition const & _tagged);
		void place_pending_doors_on_pois();

		RoomGeneratorInfo const * get_room_generator_info() const; // will always result in same generator as long as we don't modify individual random generator
		RoomType const * get_door_vr_corridor_room_room_type(Random::Generator & _rg) const;
		RoomType const * get_door_vr_corridor_elevator_room_type(Random::Generator & _rg) const;
		Range get_door_vr_corridor_priority_range() const;
		float get_door_vr_corridor_priority(Random::Generator& _rg, float _default) const;
		void set_door_vr_corridor_priority(float _priority) { doorVRCorridor.set_priority(Range(_priority)); }
		void set_door_vr_corridor_priority(Optional<Range> const & _priority) { doorVRCorridor.set_priority(_priority); }

		bool generate_plane_room(Material * _planeMaterial = nullptr, PhysicalMaterial * _planePhysMaterial = nullptr, float u = 0.0f, bool _withCeiling = false, float _zOffset = 0.0f); // generate plane for room
		bool generate_plane(Material * _planeMaterial = nullptr, PhysicalMaterial * _planePhysMaterial = nullptr, float u = 0.0f, bool _withCeiling = false, float _zOffset = 0.0f); // generate plane for room
		bool generate_hills(Material * _planeMaterial, PhysicalMaterial * _planePhysMaterial, float u, float _height, float _size, float _step, Vector3 const & _at, int _tryCount = 200); // generate hills for room

		void add_sky_mesh(Material* _usingMaterial, Optional<Vector3> const & _centre = NP, Optional<float> const & _radius = NP); // add sky box mesh
		void add_clouds(Material* _usingMaterial, Optional<Vector3> const& _centre = NP, Optional<float> const& _radius = NP, float _lowAngle = 5.0f, float _highAngle = 60.0f); // add clouds mesh

		ConvexHull const& get_volumetric_limit() const { return volumetricLimit; }

		// advance - begin
		static void advance__pois(IMMEDIATE_JOB_PARAMS);
		static void advance__clear_presence_links(IMMEDIATE_JOB_PARAMS);
		static void advance__combine_presence_links(IMMEDIATE_JOB_PARAMS);
		static void advance__finalise_frame(IMMEDIATE_JOB_PARAMS);
		//
		static void advance__player_related(Array<Room*> const & _onRooms, float _deltaTime);
		static void advance__suspending_advancement(IMMEDIATE_JOB_PARAMS);
		// advance - end

		bool does_require_finalise_frame() const;
		bool does_require_clear_presence_links() const { return presenceLinks != nullptr; }
		bool does_require_combine_presence_links() const;

		bool do_pois_require_advancement() const { return poisRequireAdvancement; }

		// collision _intoEndRoomTransform.to_world(nextPlacement)
		bool check_segment_against(AgainstCollision::Type _againstCollision, REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, OUT_ Room const * & _endsInRoom, OUT_ Transform & _intoEndRoomTransform, OUT_ DoorInRoomArrayPtr * _throughDoors = nullptr) const;

		Optional<Vector3> get_gravity_dir() const;
		void update_gravity(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _gravityForce);
		void update_kill_gravity_distance(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _kill_gravity_distance);

		Optional<Vector3> get_environmental_dir(Name const & _environmentalID) const;
		void set_environmental_dir(Name const & _environmentalID, Vector3 const & _dir);

		Vector3 get_room_velocity() const { return roomVelocity; }
		void set_room_velocity(Vector3 const& _roomVelocity) { roomVelocity = _roomVelocity; }

		// environment
		virtual void set_environment(Environment* _environment, Transform const& _environmentPlacement = Transform::identity);
		Environment* get_environment() const { return environmentUsage.environment; }
		Transform const& get_environment_requested_placement() const { return environmentUsage.requestedPlacement; }
		EnvironmentUsage const & get_environment_usage() const { return environmentUsage; }
		EnvironmentUsage & access_environment_usage() { return environmentUsage; }
		Array<UsedLibraryStored<EnvironmentOverlay>> const & get_environment_overlays() const { return environmentOverlays; }
		Array<UsedLibraryStored<EnvironmentOverlay>> & access_environment_overlays() { return environmentOverlays; }

		Room const * get_room_with_vars_for_environment_overlays() const { if (auto* r = environmentOverlaysGetRoomVarsFromRoom.get()) return r; return this; }
		void set_room_with_vars_for_environment_overlays(Room* _room) { environmentOverlaysGetRoomVarsFromRoom = _room; }

		void create_own_copy_of_environment(); // only if doesn't already have own environment - useful when we want to alter some values

		// reverb
		void set_reverb(Reverb* _reverb);
		Reverb* get_reverb() const { return useReverb.get(); }

		void mark_to_activate(TemporaryObject* _temporaryObject);

		bool run_wheres_my_point_processor_setup_parameters();
		bool run_wheres_my_point_processor_pre_generate();
		bool run_wheres_my_point_processor_on_generate();

		static void log(LogInfoContext & _log, Room* const & _room);
		static void log(LogInfoContext & _log, ::SafePtr<Room> const & _room);

	public:
		void keep_door_types() { keepDoorTypes = true; }
		bool should_keep_door_types() const { return keepDoorTypes; }

	public:
		bool is_close_to_collidable(Vector3 const& _loc, Collision::Flags const& _withFlags, float _maxDist) const;

	public:
		Optional<float> get_vr_elevator_altitude() const { return vrElevatorAltitude; } // as is set
		Optional<float> find_vr_elevator_altitude(Room* _awayFromRoom, Door* _throughDoor, OPTIONAL_ OUT_ int* _depth = nullptr) const;
		void set_vr_elevator_altitude(float _vrElevatorAltitude) { vrElevatorAltitude = _vrElevatorAltitude; }

	public:
		RoomType::UseEnvironment & access_use_environment() { useEnvironment.set_if_not_set(); return useEnvironment.access(); }
	protected:
		virtual void get_use_environment_from_type();
		virtual void setup_using_use_environment();

	protected: // SafeObject
		void make_safe_object_available(Room* _object);
		void make_safe_object_unavailable();

	public: // WheresMyPoint::IOwner
		override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
		override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
		override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const; // shouldn't be called!

		override_ WheresMyPoint::IOwner* get_wmp_owner();

	private:
		bool restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value, bool _checkOriginRoom) const;

	public:
		template <typename Class>
		inline Class const & get_value(Name const& _name, Class const& _fallbackValue, bool _checkOriginRoom = true) const // wrapper for restore_global_value_for_wheres_my_point, goes through region etc
		{
			if (Class const* v = get_variable<Class>(_name, _checkOriginRoom))
			{
				return *v;
			}
			else
			{
				return _fallbackValue;
			}
		}

		template <typename Class>
		inline bool has_value(Name const& _name, bool _checkOriginRoom = true) const // wrapper for restore_global_value_for_wheres_my_point, goes through region etc
		{
			if (get_variable<Class>(_name, _checkOriginRoom))
			{
				return true;
			}
			else
			{
				return false;
			}
		}

	public:
		static void update_player_related(Framework::IModulesOwner* _player, bool _visited, bool _seen);
		//static void update_seen_by_player(Room* _room);
		AVOID_CALLING_ bool was_recently_seen_by_player() const { return timeSinceSeenByPlayer <= c_timeSinceSeenByPlayerLimit; }
		AVOID_CALLING_ int get_distance_to_recently_seen_by_player() const { return distanceToRecentlySeenByPlayer; }
		AVOID_CALLING_ bool was_recently_visited_by_player() const { return timeSincePlayerVisited <= c_timeSinceVisitedByPlayerLimit; }
		AVOID_CALLING_ float get_time_since_last_visited_by_player() const { return timeSincePlayerVisited; }
		Optional<Transform> const& get_recently_seen_by_player_placement() const { return recentlySeenByPlayer; }
#ifdef AN_DEVELOPMENT_OR_PROFILER
		// for PreviewGame only
		void mark_recently_seen_by_player(Transform const& placement);
#endif

	public:
		Array<Nav::MeshPtr> & access_nav_meshes() { return navMeshes; }
		Nav::Mesh* access_nav_mesh(Name const & _navMeshId);
		void use_nav_mesh(Nav::Mesh* _navMesh); // will replace existing one
		bool should_be_avoided_to_navigate_into() const { return shouldBeAvoidedToNavigateInto; }

		void mark_nav_mesh_creation_required();

		Nav::PlacementAtNode find_nav_location(Transform const & _placement, OUT_ float * _dist = nullptr, Name const & _navMeshId = Name::invalid(), Optional<int> const & _navMeshKeepGroup = NP) const;

	private: friend class Nav::System;
		void queue_build_nav_mesh_task();

	public: // WorldObject
		implement_ void world_activate();

		implement_ String get_world_object_name() const { return get_name(); }

	protected: friend class Environment;
		void on_environment_destroyed(Environment * _environment);

	public:
		void update_space_blockers();
		bool check_space_blockers(SpaceBlockers const& _spaceBlockers, Transform const& _placement);
#ifdef AN_DEVELOPMENT
		SpaceBlockers const& get_space_blockers() const { return spaceBlockers; }
#endif

	public: // debug
		void debug_draw_collision(int _againstCollision, bool _objectsOnly, Object* _skipObject);
		void debug_draw_door_holes();
		void debug_draw_volumetric_limit();
		void debug_draw_skeletons();
		void debug_draw_sockets();
		void debug_draw_pois(bool _setDebugContext = true);
		void debug_draw_mesh_nodes();
		void debug_draw_nav_mesh();
		void debug_draw_space_blockers();

#ifdef AN_PERFORMANCE_MEASURE
	public:
		String const & get_performance_context_info() const { return name; }
#endif

	private: friend class ModulePresence;
		// should be managed through presence
		void add_object(IModulesOwner* _object);
		void remove_object(IModulesOwner* _object);

	protected: friend class DoorInRoom;
		void add_door(DoorInRoom* _door);
		void remove_door(DoorInRoom* _door);
		void replace_door(DoorInRoom* _replace, DoorInRoom* _with);

	private: friend class PresenceLink;
		void add_presence_link(PresenceLinkBuildContext const & _context, PresenceLink* _link);
		void sync_remove_presence_link(PresenceLink* _link); // SYNC!
		void removed_first_presence_link(PresenceLink* _link, PresenceLink* _replaceWith);

	protected: friend class World;
			   friend class SubWorld;
			   friend class Region;
		inline void set_in_sub_world(SubWorld* _subWorld) { inSubWorld = _subWorld; }
		void set_in_region(Region* _inRegion);

		// checks if can be spawned and if so, adds to the list
		bool check_temporary_object_spawn_block(TemporaryObjectType const* _type, Vector3 const& _at, float _dist, Optional<float> const& _time, Optional<int> const& _count);

	protected:
		int worldAddressID = NONE; // partial, of this one

		String name;
		Tags tags;
		RoomType const * roomType;
		SubWorld* inSubWorld;
		Region* inRegion;
		::SafePtr<Room> renderObjectsFromRoom; // to allow rendering objects from a different room

		Tags markPOIsInside; // tags for all POIs inside

		bool smallRoom = false; // very small room that might be ignored for some stuff (door directions, distances)

		bool deletionPending = false; // if we're deletion pending, disallow particular things (navmesh regeneration, building presence links etc)
		bool objectsNoLongerAdvanceable = false; // to avoid adding new objects

		bool important = false; // important rooms have stuff advanced every frame

		bool shouldHaveNavMesh = true;
		bool shouldBeAvoidedToNavigateInto = false; // we may have some utility rooms that characters should not choose to navigate into
		mutable Concurrency::MRSWLock navMeshesLock; // because we may trigger nav mesh deletion from room destructor and we want to decouple nav mesh and room
		Array<Nav::MeshPtr> navMeshes;

		Array<PointOfInterestInstancePtr> pois; // global ones in room, room parts hold their points of interest inside them 
		CACHED_ bool poisRequireAdvancement = false;

		bool isVRArranged = false; // doors are vr placed - this is early step of generation, it can be done without actual room generation or may trigger room generation
		bool isMeshGenerated = false; // if mesh for room is generated

		Concurrency::Counter generationProcessStarted;
		Concurrency::SpinLock generationProcessStartedLock = Concurrency::SpinLock(TXT("Framework.Room.generationProcessStartedLock"));

		::SafePtr<Room> originRoom; // if we were spawned from another room, use it to fill variable
		Random::Generator individualRandomGenerator; // used as a base, should be kept same through lifetime of the room
		bool individualRandomGeneratorLocked = false;
		SimpleVariableStorage variables; // used for various modules (eg. when creating mesh via generator), should not be modified when running (!)
		bool individualRandomGeneratorSet = false;
		RoomGeneratorInfo const * roomGeneratorInfo = nullptr; // can be overriden when created
		RoomGeneratorInfoSet roomGeneratorInfoSet;
		DoorVRCorridor doorVRCorridor;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		RoomGeneratorInfo const * generatedWithRoomGenerator = nullptr;
#endif

		UsedLibraryStored<DoorTypeReplacer> doorTypeReplacer;

		bool keepDoorTypes = false; // if set to true, can't change door types for doors here
		Array<DoorInRoom*> allDoors;
		Array<DoorInRoom*> doors;
		mutable Concurrency::MRSWLock doorsLock;
		bool checkIfDoorsAreTooClose = false;

		Array<Object*> allObjects;
		Array<Object*> objects;
		mutable Concurrency::MRSWLock objectsLock;

		Array<TemporaryObject*> allTemporaryObjects;
		Array<TemporaryObject*> temporaryObjects;
		mutable Concurrency::MRSWLock temporaryObjectsLock;

		Meshes::CombinedMesh3DInstanceSet* combinedAppearance;
		Meshes::Mesh3DInstanceSet appearance;
		Collision::ModelInstanceSet movementCollision;
		Collision::ModelInstanceSet preciseCollision;
		bool requiresUpdateBoundingBox = false;

		Array<UsedLibraryStored<Mesh>> usedMeshes;

		::System::ShaderProgramBindingContext shaderProgramBindingContext; // for any parameters

		Array<RoomPartInstance*> roomPartInstances;

		mutable Concurrency::MRSWLock appearanceSpaceBlockersLock;
		mutable Concurrency::MRSWLock spaceBlockersLock;
		SpaceBlockers appearanceSpaceBlockers;
		SpaceBlockers spaceBlockers;

		PresenceLink* presenceLinks;
		Array<PresenceLink*> threadPresenceLinks;

		Optional<RoomType::UseEnvironment> useEnvironment;

		EnvironmentUsage environmentUsage;
		Environment* ownEnvironment;

		Array<UsedLibraryStored<EnvironmentOverlay>> environmentOverlays;
		::SafePtr<Room> environmentOverlaysGetRoomVarsFromRoom;

		UsedLibraryStored<Reverb> useReverb;

		SoundSources sounds;

		PredefinedRoomOcclusionCulling predefinedRoomOcclusionCulling;

		ConvexHull volumetricLimit; // build on POIs, allows having a (convex) volume to move within

		Concurrency::SpinLock killGravityCacheLock = Concurrency::SpinLock(TXT("Framework.Room.killGravityCacheLock"));
		CACHED_ Name killGravityAnchorPOI;
		CACHED_ Name killGravityAnchorParam;
		CACHED_ Name killGravityOverrideAnchorPOI;
		CACHED_ Name killGravityOverrideAnchorParam;
		CACHED_ Optional<float> killGravityAnchorPOIDistance;
		CACHED_ Optional<float> killGravityAnchorParamDistance;
		CACHED_ Optional<float> killGravityOverrideAnchorPOIDistance;
		CACHED_ Optional<float> killGravityOverrideAnchorParamDistance;

		Vector3 roomVelocity = Vector3::zero; // for rooms that pretend to be moving but are stationary. the velocity tells how the objects that pretend to be moving would be moving (the stationary objects move the other way)

		Concurrency::SpinLock environmentalDirsLock = Concurrency::SpinLock(TXT("Framework.Room.environmentalDirsLock"));
		struct EnvironmentalDir
		{
			Name id;
			Vector3 dir;
		};
		ArrayStatic<EnvironmentalDir, 2> environmentalDirs;

		Optional<float> vrElevatorAltitude; // this is set from door or from tags and auto during readying for vr - doors are more important when it comes to vr elevator altitude

		mutable Concurrency::SpinLock temporaryObjectSpawnBlockingLock = Concurrency::SpinLock(TXT("Framework.Room.temporaryObjectSpawnBlockingLock")); // because we may trigger nav mesh deletion from room destructor and we want to decouple nav mesh and room
		struct TemporaryObjectSpawnBlocking
		{
			TemporaryObjectType const* temporaryObjectType;
			Vector3 at;
			float dist;
			Optional<float> timeLeft;
			Optional<int> countLeft;
		};
		Array<TemporaryObjectSpawnBlocking> temporaryObjectSpawnBlocking;

		Concurrency::SpinLock worldActivationLock = Concurrency::SpinLock(TXT("Framework.Room.worldActivationLock")); // to not get in conflict with temporary objects to activate 
		Array<TemporaryObject*> temporaryObjectsToActivate; // on world activate

		static const float c_timeSinceSeenByPlayerLimit;
		static const float c_timeSinceVisitedByPlayerLimit;
		Concurrency::SpinLock seenByPlayerLock = Concurrency::SpinLock(TXT("Framework.Room.seenByPlayerLock"));
		float timeSinceSeenByPlayer = 100000.0f; // raw delta but clamped! (without time speed)
		float timeSincePlayerVisited = 100000.0f; // delta time that may use time speed
		int distanceToRecentlySeenByPlayer = NONE;
		int prevDistanceToRecentlySeenByPlayer = NONE; // so we update objects inside only if we change
		Optional<Transform> recentlySeenByPlayer; // because this is the "recently seen", we have a good chance of having a straight line to a player
		Optional<Transform> recentlySeenByPlayerCandidate;
		int recentlySeenByPlayerCandidateDepth = NONE;
		int advancementSuspensionRoomDistance = 5;
		bool alwaysSuspendAdvancement = false; // useful for some utility rooms

		bool pulseEnvironment = false;

		struct PendingDoorOnPOI
		{
			::SafePtr<DoorInRoom> door;
			Name poi;
			TagCondition tagged;
		};
		Array<PendingDoorOnPOI> pendingDoorsOnPOIs;

		friend class Render::RoomProxy;
		friend class Render::EnvironmentProxy;

		bool check_segment_against_worker(AgainstCollision::Type _againstCollision, REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, OUT_ ArrayStack<DoorInRoom const *> & _throughDoors, int _depthLeft) const;

		void setup_reverb_from_type();

		bool generate_fallback_room(); // generate fallback solution

		void destroy_all_objects_inside();

		void defer_combined_apperance_delete();

		template <typename Class>
		void apply_replacer_to(REF_ Class* & _object) const;

#ifdef AN_DEVELOPMENT
		void assert_we_dont_modify_doors() const;
		void assert_we_may_modify_doors() const;
		void assert_we_dont_modify_objects() const;
		void assert_we_may_modify_objects() const;
#else
		inline void assert_we_dont_modify_doors() const {}
		inline void assert_we_may_modify_doors() const {}
		inline void assert_we_dont_modify_objects() const {}
		inline void assert_we_may_modify_objects() const {}
#endif

		RoomGeneratorInfo const * get_room_generator_info_internal(Random::Generator & _rg) const;

		void update_own_environment_parent();

		void update_door_type_replacer(); // using room generator info and individual random generator, this is automatically called when room type or individual random generator is set
		void internal_update_door_type_replacer_with(DoorTypeReplacer* _dtr); // will check with room type's and choose the one with higher priority (for equal, _dtr is used)
		void set_door_type_replacer(DoorTypeReplacer* _dtr); // will update doors inside

		void update_volumetric_limit();

		void update_small_room();

#ifdef WITH_ROOM_HISTORY
	public:
		List<String> history;
		void add_history(String const& _entry) { history.push_back(_entry); }
#endif
	};
};

DECLARE_REGISTERED_TYPE(Framework::Room*);
DECLARE_REGISTERED_TYPE(SafePtr<Framework::Room>);
