#pragma once

#include "door.h"
#include "doorHole.h"
#include "..\collision\againstCollision.h"
#include "..\interfaces\aiObject.h"
#include "..\nav\navNode.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\mesh\mesh3dInstanceSet.h"
#include "..\..\core\collision\modelInstanceSet.h"
#include "..\..\core\collision\iCollidableObject.h"

#include "..\debugSettings.h"

#ifdef AN_DEVELOPMENT
//#define INVESTIGATE__CHECKS_FOR_DOOR_IN_ROOM
#endif
#ifdef INVESTIGATE__CHECKS_FOR_DOOR_IN_ROOM
#include "..\..\core\debug\debugRenderer.h"
#endif

namespace Collision
{
	struct QueryPrimitivePoint;
	struct QueryPrimitiveSegment;
};

#ifdef AN_DEVELOPMENT
	#ifdef WITH_HISTORY_FOR_OBJECTS
		#ifndef WITH_DOOR_IN_ROOM_HISTORY
			#define WITH_DOOR_IN_ROOM_HISTORY
		#endif
	#endif
	#ifndef WITH_DOOR_IN_ROOM_HISTORY
		//#define WITH_DOOR_IN_ROOM_HISTORY
	#endif
#endif

#ifdef WITH_DOOR_IN_ROOM_HISTORY
#define DOOR_IN_ROOM_HISTORY(dir, ...) dir->add_history(String::printf(__VA_ARGS__))
#else
#define DOOR_IN_ROOM_HISTORY(dir, ...)
#endif

namespace Framework
{
	class DoorInRoom;
	class Mesh;
	class Room;
	class RoomPart;
	class Skeleton;
	struct CheckCollisionContext;
	struct CheckSegmentResult;
	struct DoorWingType;
	struct SpaceBlockers;

	namespace AI
	{
		class Message;
	};

	namespace Nav
	{
		class Mesh;
		class Node;
	};

	namespace Render
	{
		class DoorInRoomProxy;
	};

	struct DoorWingInRoom
	{
		DoorWingType const * wingInType;
		UsedLibraryStored<Mesh> mesh;
		UsedLibraryStored<Skeleton> skeleton;
		int meshInstanceIndex;
		int movementCollisionInstanceIndex;
		int preciseCollisionInstanceIndex;
	};

	/**
	 *	Class that is interface to any part of memory to allow gathering "door in room"s
	 */
	struct DoorInRoomArrayPtr
	{
		DoorInRoomArrayPtr(DoorInRoom const ** _doorsPtr, int _limitOfDoors) : doorsPtr(_doorsPtr)
#ifdef AN_ASSERT
			, limitOfDoors(_limitOfDoors)
#endif
		{}

		void push_door(DoorInRoom const * _door) { an_assert(numberOfDoors < limitOfDoors); doorsPtr[numberOfDoors] = _door; ++numberOfDoors; }
		void pop_door() { an_assert(numberOfDoors > 0); --numberOfDoors; }
		void copy_from(DoorInRoomArrayPtr const & _other) { numberOfDoors = _other.numberOfDoors; if (numberOfDoors) { memory_copy(doorsPtr, _other.doorsPtr, sizeof(DoorInRoom const *)* numberOfDoors); } }

		bool is_empty() const { return numberOfDoors == 0.0f; }
		DoorInRoom const ** begin() const { return doorsPtr; }
		DoorInRoom const ** last_ptr() const { return &doorsPtr[numberOfDoors - 1]; }
		DoorInRoom const *& get_last() const { an_assert(!is_empty()); return doorsPtr[numberOfDoors - 1]; }

		int get_number_of_doors() const { return numberOfDoors; }
	private:
		DoorInRoom const ** doorsPtr;
		int numberOfDoors = 0;
#ifdef AN_ASSERT
		int limitOfDoors;
#endif
	};

	/**
	 *	placement is same as outbound (fwd into the door hole, outside the room)
	 *	door hole is stored using sides (a -> outbound, b -> inbound)
	 *	door wings are stored using placement
	 *
	 *	DoorInRoom* dir = new DoorInRoom;
	 *	door->link_a(dir);
	 *	dir->init_meshes();
	 *	door->ready_to_world_activate(); // or through ready to activate all inactive
	 *	door->world_activate(); // or through activate all inactive
	 */
	class DoorInRoom
	: public SafeObject<DoorInRoom>
	, public Collision::ICollidableObject
	, public IAIObject
	, public WorldObject
	{
		FAST_CAST_DECLARE(DoorInRoom);
		FAST_CAST_BASE(Collision::ICollidableObject);
		FAST_CAST_BASE(IAIObject);
		FAST_CAST_END();
	public:
		static bool same_with_orientation_for_vr(Transform const& _a, Transform const& _b, Optional<float> const& _translationThreshold = NP, Optional<float> const& _scaleThreshold = NP, Optional<float> const& _orientationThreshold = NP);

		DoorInRoom(Room* _inRoom);
		~DoorInRoom();

		void start_crawling_destruction(bool _keepDoorInRoom = false); // will destroy this door and then will destroy room beyond it

		float get_individual() const { return individual; }

		Tags & access_tags() { return tags; }
		Tags const & get_tags() const { return tags; }

		void set_connector_tag(Name const & _connectorTag) { connectorTag = _connectorTag; }
		Name const & get_connector_tag() const { return connectorTag; }

		inline void set_outer_name(Name const & _outerName) { outerName = _outerName; }
		inline Name const & get_outer_name() const { return outerName; }

		inline void set_group_id(int _id) { groupId = _id; }
		inline void clear_group_id(int _id) { groupId = NONE; }
		inline int get_group_id() const { return groupId; }

		inline DoorSide::Type get_side() const { return side; }

		void clear_meshes();
		void init_meshes();
		void reinit_meshes_if_init();

		inline bool is_linked(Door* _door) const { return door == _door; }
		inline Door* get_door() const { return door; }
		
		inline Room* get_in_room() const { return inRoom; }

		inline bool is_behind(Plane const & _plane) const;

		bool is_visible() const;

		bool can_see_through_it_now() const;

		bool is_vr_space_placement_set() const { return vrSpacePlacement.is_set(); }
		void set_vr_space_placement(Transform const & _vrSpacePlacement, bool _forceOver180 = false); // before setting, check is_vr_placement_immovable, if true, use grow_into_vr_corridor and then set the placement on other door
		bool is_vr_placement_immovable() const { return vrSpacePlacementImmovable; }
		void make_vr_placement_immovable();
		void make_vr_placement_movable();
		void update_vr_placement_from_door_on_other_side();
		Transform const & get_vr_space_placement() const;
		VR::Zone calc_vr_zone_outbound(Optional<float> const& _spaceRequiredBeyondDoor = NP) const;
		VR::Zone calc_vr_zone_outbound_if_were_at(Transform const& _vrPlacement, Optional<float> const& _spaceRequiredBeyondDoor = NP) const;

		void set_ignorable_vr_placement(bool _ignoreVRPlacement = true) { ignorableVRPlacement = _ignoreVRPlacement; }
		bool may_ignore_vr_placement() const { return ignorableVRPlacement; }

		DoorInRoom* grow_into_room(Optional<Transform> const& _requestedPlacement, Transform const& _requestedVRPlacement, RoomType const* _roomType, DoorType const* _newDoorDoorType = nullptr, tchar const* _proposedName = nullptr); // will create a room with a new door in the same room it was
		DoorInRoom* grow_into_vr_corridor(Optional<Transform> const& _requestedPlacement, Transform const& _requestedVRPlacement, VR::Zone const& _beInZone, float _tileSize); // will create a vr corridor and replace itself with a new door in the same room it was

		void set_placement(Transform const & _placement); // don't use it when the doors are already active, use request_placement instead

		void request_placement(Transform const& _placement) { requestedPlacement = _placement; }
		void update_placement_to_requested();
		bool is_new_placement_requested() const { return requestedPlacement.is_set(); }

		inline bool is_placed() const { return isPlaced; }
		inline Optional<Transform> get_placement_if_placed() const { return isPlaced? Optional<Transform>(placement) : NP; }
		inline Transform const & get_placement() const { an_assert(isPlaced); return placement; } // outbound
		inline Transform const & get_outbound_transform() const { return placement; } // outbound matrix is placement
		inline Matrix44 const& get_scaled_placement_matrix() const { return scaledOutboundMatrix; }
		inline Matrix44 const & get_placement_matrix() const { return outboundMatrix; } // outbound matrix is placement
		inline Matrix44 const & get_inbound_matrix() const { return inboundMatrix; }
		inline Matrix44 const & get_outbound_matrix() const { return outboundMatrix; }
		inline Matrix44 const & get_other_room_matrix() const { an_assert(roomOnOtherSide.get(), TXT("no connection through door made")); return otherRoomMatrix; }
		inline Transform const & get_other_room_transform() const { an_assert(roomOnOtherSide.get(), TXT("no connection through door made")); return otherRoomTransform; }
		inline Vector2 get_hole_scale() const { return door ? door->get_hole_scale() : Vector2::one; }
		inline float get_door_scale() const { return door ? door->get_door_scale() : 1.0f; }
		Vector3 get_hole_centre_WS() const; // world
		Vector3 get_dir_inside() const { return inboundMatrix.get_y_axis(); }
		Vector3 get_dir_outside() const { return -inboundMatrix.get_y_axis(); }

		inline DoorHole const * get_hole() const { return door ? door->get_hole() : nullptr; }
		inline Meshes::Mesh3DInstanceSet & access_mesh() { return mesh; }

		inline bool has_room_on_other_side() const { return roomOnOtherSide.is_set(); }
		bool has_world_active_room_on_other_side() const;
		inline Room* get_room_on_other_side() const { return roomOnOtherSide.get(); }
		Room* get_world_active_room_on_other_side() const;
		inline DoorInRoom* get_door_on_other_side() const { return door? door->get_on_other_side(this) : nullptr; }
		inline Plane const & get_plane() const { return plane; } // normal is inbound
		Range2 calculate_size() const;
		Range2 calculate_compatible_size() const; // compatible size

		// _onDoorPlaneInsideDoor is set properly only if within radius
		bool is_within_radius(Vector3 const & _point, float _dist, Optional<Range> const & _inFront = NP) const; // checks against range in front, following functions do only minInFront check
		bool is_in_radius(bool _checkFront, Vector3 const & _point, float _dist, OUT_ Vector3 & _onDoorPlaneInsideDoor, OUT_ float & _distLeft, Optional<float> const & _offHoleDistMultiplier = 1.0f, Optional<float> const & _minInFront = NP) const;
		bool is_in_radius(bool _checkFront, SegmentSimple const & _point, float _dist, OUT_ SegmentSimple & _onDoorPlaneInsideDoor, OUT_ float & _distLeft, Optional<float> const & _offHoleDistMultiplier = 1.0f, Optional<float> const & _minInFront = NP) const;
		bool is_in_radius(bool _checkFront, Collision::QueryPrimitivePoint const & _point, float _dist, Optional<float> const & _offHoleDistMultiplier = 1.0f, Optional<float> const & _minInFront = NP, OPTIONAL_ OUT_ float * _distOfPoint = nullptr) const;
		bool is_in_radius(bool _checkFront, Collision::QueryPrimitiveSegment const & _segment, float _dist, Optional<float> const & _offHoleDistMultiplier = 1.0f, Optional<float> const & _minInFront = NP, OPTIONAL_ OUT_ float * _distOfPoint = nullptr) const;
		bool is_inside_hole(Vector3 const & _point) const;
		bool is_inside_hole(SegmentSimple const & _segment) const;
		
		void skip_hole_check_on_moving_through();
		bool should_skip_hole_check_on_moving_through() const { return skipHoleCheckOnMovingThrough; }

		void update_movement_info();
		bool can_move_through() const { return canMoveThrough; }

		bool is_relevant_for_movement() const { return relevantForMovement; }
		bool is_relevant_for_vr() const { return relevantForVR; }

		inline float calculate_dist_of(Vector3 const & _point, float _offHoleDistMultiplier = 1.0f) const;

		bool does_go_through_hole(Vector3 const & _currLocation, Vector3 const & _nextLocation, OUT_ Vector3 & _destLocation, float _minInFront = -0.001f, float _isInsideDoorExt = 0.0f) const;

		bool check_segment(REF_ Segment & _segment, REF_ CheckSegmentResult & _result, float _minInFront = -0.001f) const;
		
		Vector3 find_inside_hole_for_string_pulling(Vector3 const & _a, Vector3 const & _b, Optional<Vector3> const & _lastInHole = NP, float _moveInside = 0.0f) const; // finds point inside hole that lies on line going through a and b (doesn't have to be exactly between a and b)
		Vector3 find_inside_hole(Vector3 const & _a, float _moveInside = 0.0f) const; // just inside hole - this may result in quite bad approximations. use with caution!

		bool is_in_front_of(Plane const & _plane, float _minDist = 0.0f) const; // at least one point is in front of

		bool are_meshes_ready() const { return meshesReady; }

		Collision::ModelInstanceSet const & get_against_collision(AgainstCollision::Type _againstCollision) const { return _againstCollision == AgainstCollision::Movement ? get_movement_collision() : get_precise_collision(); }
		Collision::ModelInstanceSet const & get_movement_collision() const { return movementCollision; }
		Collision::ModelInstanceSet const & get_precise_collision() const { return ! preciseCollision.is_empty()? preciseCollision : movementCollision; }
		bool check_segment_against(AgainstCollision::Type _againstCollision, REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context) const;

		void create_space_blocker(REF_ SpaceBlockers& _spaceBlockers, bool _localPlacement = false);

	public:
		Optional<float> const& get_vr_elevator_altitude() const { return vrElevatorAltitude; }
		void set_vr_elevator_altitude(float _vrElevatorAltitude) { vrElevatorAltitude = _vrElevatorAltitude; }

	private: friend class Door;
		void link(Door* _door);
		void link_as_a(Door* _door);
		void link_as_b(Door* _door);

	public:
		void unlink();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		DoorType const* get_meshes_done_for_door_type() const { return meshesDoneForDoorType; }
		DoorType const* get_meshes_done_for_secondary_door_type() const { return meshesDoneForSecondaryDoorType; }
#endif

	public: // SafeObject
		inline bool is_available_as_safe_object() const { return SafeObject<DoorInRoom>::is_available_as_safe_object(); }

	protected: // SafeObject
		void make_safe_object_available(DoorInRoom* _object);
		void make_safe_object_unavailable();

	public: // Nav
		void add_to(Nav::Mesh* _mesh);
		Nav::NodePtr const & get_nav_door_node() const { return navDoorNode; }
		void nav_connect_to(DoorInRoom* _other);

	public: // IAIObject
		implement_ bool does_handle_ai_message(Name const& _messageName) const { return false; }
		implement_ AI::MessagesToProcess * access_ai_messages_to_process() { return nullptr; }
		implement_ String const & ai_get_name() const { return String::empty(); }
		implement_ Transform ai_get_placement() const { Transform result = placement; result.set_translation(get_hole_centre_WS()); return result; }
		implement_ Room* ai_get_room() const { return inRoom; }

	protected:	friend class Door;
		void on_connection_through_door_created(); // called when doors on both sides are connected
		void on_connection_through_door_broken(); // called when door has lost connection

		void update_wings();

	public: // WorldObject
		implement_ void world_activate();

	public: // debug
		void debug_draw_collision(int _againstCollision);
		void debug_draw_hole(bool _front, Colour const & _colour) const;

	public:
		int get_nav_group_id() const { return navGroupId; }

	private: friend class Nav::Node;
		void set_nav_group_id(int _navGroupId) { navGroupId = _navGroupId; }
		void outdate_nav();

	private:
		Room* inRoom;
		Door* door;
		float individual = 0.0f;
		Optional<float> vrElevatorAltitude; // this is set by room generators, eg. elevators, etc. it has a higher priority than room's and door's vr elevator altitude, they auto override door's
		int groupId = NONE;
		float updatedForDoorOpenFactor = -9999.0f;
		bool meshesInit = false;
		DoorType const * meshesDoneForDoorType = nullptr;
		DoorType const * meshesDoneForSecondaryDoorType = nullptr;
		GeneratedMeshDeterminants meshesDoneForMeshDeterminants;
#ifdef AN_DEVELOPMENT
		String meshesDoneForGeneralDoorTypeParameters;
#endif
		DoorSide::Type side;
		// piece combiner -> room generation feedback/information passage
		Tags tags; // door in room can be tagged (this should/may help with room generation), this is different from door's tags! these are used to differentiate doors within room (especially when they're of he same door type)
		Name connectorTag; // as a unique identifier, reflects piece combiner's "connectorTag"

		bool skipHoleCheckOnMovingThrough = false; // when checking if moves through, checking if is inside hole might be skipped
		
		CACHED_ bool canMoveThrough = true; // if set to false, nothing can move through (from door and doortype)
		CACHED_ bool relevantForMovement = true; // more as an information about whether it should be considered for movement
		CACHED_ bool relevantForVR = true; // more as an information about whether it should be considered for vr (in general)
		
		bool ignorableVRPlacement = false; // if set to true, might be not required to have vr placement (at least valid)

		bool vrSpacePlacementImmovable = false; // if set, vrSpacePlacement can't be changed
		Optional<Transform> vrSpacePlacement; // (outbound) if placement for this door and one on the other side do not match, corridor has to be created, if not set, will be filled with door on the other side's vr placement
											  // in general, they can work if they're turned 180 (around 0,0,0) but within same room they should remain in the same (preferrably) non-turned space

		Name outerName; // to allow overriding door with room part override_ (from room type)

		::SafePtr<Room> CACHED_ roomOnOtherSide; // cached from door

		// to remove from nav mesh, to block and unblock etc
		int navGroupId = NONE;
		Nav::NodePtr navDoorNode;
		Nav::NodePtr navEntranceNode;
		Nav::Node::ConnectionSharedDataPtr navSharedData; // to block and unblock

		bool isPlaced;
		Optional<Transform> requestedPlacement;
		Transform placement; // (outbound) placement of door in the room, transforms from door's inner space to room's inner space (x-right, y-forward (pointing outside room), z-up)
		Plane CACHED_ plane; // door plane - for clipping, etc (normal from y - points inside room)
		Matrix44 CACHED_ inboundMatrix; // cached "inbound - into room" matrix (y is pointing inside)
		Transform CACHED_ inboundPlacement; // above but as transform
		Matrix44 CACHED_ outboundMatrix; // cached "outbound - out of the room" matrix (y is pointing outside - also placement of door)
		Matrix44 CACHED_ scaledOutboundMatrix; // with applied scale
		Matrix44 CACHED_ otherRoomMatrix; // cached "other room" matrix - useful when trying to get points from other room to this (to_world) or into other room (to_local)
		Transform CACHED_ otherRoomTransform;

		Transform applySidePlacement; // to make it easier to update wings, to place frame properly
		Array<DoorWingInRoom> wings;
		// relative to placement
		Meshes::Mesh3DInstanceSet mesh;
		Collision::ModelInstanceSet movementCollision;
		Collision::ModelInstanceSet preciseCollision;
		Array<UsedLibraryStored<Mesh>> usedMeshes;
		bool meshesReady = false;

		void on_set_placement();
		void update_other_room_transform();
		// TODO add linking through id

		void update_on_link();
		void update_side();

		void update_scaled_placement_matrix();

		inline float calculate_dist_of(Vector3 const & _point, float _inFront, OUT_ Vector3 & _onDoorPlaneInsideDoor, float _offHoleDistMultiplier) const;
		inline float calculate_dist_of(Vector3 const & _point, float _inFront, float _offHoleDistMultiplier) const;

		inline void update_collision_bounding_boxes();

		friend class Render::DoorInRoomProxy;
		friend class AI::Message;

		DoorInRoom* grow_into_thing(Optional<Transform> const& _requestedPlacement, Transform const& _requestedVRPlacement,
			OPTIONAL_ DoorType const* _useNewDoorType, std::function<void(Room* corridor, DoorInRoom* a, DoorInRoom* b)> _perform_on_thing);

#ifdef WITH_DOOR_IN_ROOM_HISTORY
	public:
		List<String> history;
		void add_history(String const& _entry) { history.push_back(_entry); }
#endif
	};

	typedef ArrayStatic<DoorInRoom const*, 32> StaticDoorArray;

	#include "doorInRoom.inl"
};

DECLARE_REGISTERED_TYPE(Framework::DoorInRoom*);
DECLARE_REGISTERED_TYPE(SafePtr<Framework::DoorInRoom>);
DECLARE_REGISTERED_TYPE(Framework::StaticDoorArray);
