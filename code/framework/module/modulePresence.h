#pragma once

#include "module.h"
#include "moduleEnums.h"
#include "modulePresenceData.h"

#include "..\appearance\socketID.h"
#include "..\collision\againstCollision.h"
#include "..\interfaces\collidable.h"
#include "..\interfaces\presenceObserver.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"
#include "..\presence\presencePath.h"

#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\math\math.h"
#include "..\..\core\mesh\boneID.h"
#include "..\..\core\system\timeStamp.h"

#ifndef BUILD_PUBLIC_RELEASE
#define WITH_PRESENCE_INDICATOR
#endif

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	class AdvanceContext;
	class DoorInRoom;
	class Room;
	class Door;
	class PresenceLink;
	class ModulePresenceData;
	class Skeleton;
	class World;
	interface_class IPresenceObserver;
	struct PresenceLinkBuildContext;
	struct CheckSegmentResult;
	struct CheckCollisionContext;
	struct RelativeToPresencePlacement;

	namespace CollisionTraceFlag
	{
		enum Type
		{
			ResultInPresenceRoom		= 1, // result is in owning presence's inRoom
			ResultInFinalRoom			= 2, // result is in actual final room
			ResultInObjectSpace			= 4, // result is in object's space
			ResultNotRequired			= 8, // result is not required to be filled
			StartInProvidedRoom			= 16,
			FirstTraceThroughDoorsOnly	= 32,
			ProvideNonHitResult			= 64, // result will be filled even if not hit
		};
		typedef int Flags;
	};

	namespace PresenceLinkOperation
	{
		enum Type
		{
			Clearing,
			Building,

			COUNT
		};
	};

	/**
	 *	inRoom means where placement.get_translation() is (check trace_collision())
	 *
	 *	isVRMovement means that vr is in room mode and movement is based on that
	 *		character should be fed with vr controls durign pre_advance of game frame
	 *		movement is based on vr relative to vr anchor - character follows vr precisely
	 *		relative look is against floor level in object's presence
	 *
	 */
	class ModulePresence
	: public Module
	{
		FAST_CAST_DECLARE(ModulePresence);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModulePresence> & register_itself();

		static void ignore_auto_velocities(int _ignoreAutoVelocities = 1) { s_ignoreAutoVelocities = _ignoreAutoVelocities;	}
		static void advance_ignore_auto_velocities() { s_ignoreAutoVelocities = max(0, s_ignoreAutoVelocities -1); }

		ModulePresence(IModulesOwner* _owner);
		virtual ~ModulePresence();

	public:
		bool is_vr_movement() const { return isVRMovement; }
		void set_vr_movement(bool _isVRMovement = true) { isVRMovement = _isVRMovement; }
		
		bool is_player_movement() const { return isPlayerMovement; }
		void set_player_movement(bool _isPlayerMovement = true) { isPlayerMovement = _isPlayerMovement; }

		bool is_vr_attachment() const; // checks if attached to one
		void set_vr_attachment(bool _isVRAttachment = true) { isVRAttachment = _isVRAttachment; }

	public:
		inline bool can_change_rooms() const { return canChangeRoom && (!canBeBase || baseThroughDoors); } // temporary if can be base, can't change room
		bool is_locked_as_base(ModulePresence const* _for) const;
		inline bool can_be_based_on() const { return canBeBasedOn && modulePresenceData && !modulePresenceData->basedOnPresenceTraces.is_empty(); }
		inline bool can_be_based_on_volumetric() const { return canBeBasedOnVolumetric; }
		inline bool is_base_through_doors() const { return baseThroughDoors; }

	public:
		void set_create_ai_messages(bool _createAIMessages) { createAIMessages = _createAIMessages; }

		void request_on_spawn_random_dir_teleport(Optional<float> const& _maxDist, Optional<float> const& _preferVertical = NP, Optional<float> const& _offWall = NP);
		struct TeleportRequestParams
		{
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(TeleportRequestParams, bool, findVRAnchor, find_vr_anchor, false, true);
			ADD_FUNCTION_PARAM(TeleportRequestParams, int, teleportPriority, with_teleport_priority);
			ADD_FUNCTION_PARAM_DEF(TeleportRequestParams, bool, silentTeleport, silent_teleport, true);
			ADD_FUNCTION_PARAM_DEF(TeleportRequestParams, bool, beVisible, be_visible, true);
			ADD_FUNCTION_PARAM_DEF(TeleportRequestParams, bool, keepVelocityOS, keep_velocity_os, true);
			ADD_FUNCTION_PARAM(TeleportRequestParams, Vector3, velocityLinearWS, with_velocity);
		};
		void request_teleport(Room * _intoRoom, Transform const & _placement, Optional<TeleportRequestParams> const & _params = NP);
		void request_teleport_within_room(Transform const & _placement, Optional<TeleportRequestParams> const& _params = NP);
		bool does_request_teleport() const { return teleportRequested; }
		bool get_request_teleport_details(OPTIONAL_ OUT_ Room** _intoRoom = nullptr, OPTIONAL_ OUT_ Transform * _placement = nullptr, OPTIONAL_ OUT_ int * _teleportPriority = nullptr) const;
		bool has_teleported() const { return teleported; }
		bool is_attached_to_teleported() const;

		void set_required_forced_update_look_via_controller(bool _requires) { requiresForcedUpdateLookViaController = _requires; }

#ifdef AN_ALLOW_BULLSHOTS
		Optional<Transform>& access_bullshot_placement() { return bullshotPlacement; }
#endif

		void force_update_presence_links() { forceUpdatePresenceLinks = true; } // useful when we change any radius (collision, presence)
		void request_lazy_update_presence_links() { if (!lazyUpdatePresenceLinksTS.is_set()) lazyUpdatePresenceLinksTS = ::System::TimeStamp(); } // not to spam every frame but we would like to update some stuff

		void set_light_based_presence_radius(Optional<float> const& _lightRadius = NP);

		Transform get_placement_in_vr_space() const;
		Transform const & get_vr_anchor() const { return vrAnchor; }
		Transform const & get_vr_anchor_last_displacement() const { return vrAnchorLastDisplacement; }
		void zero_vr_anchor(bool _continuous = false); // reset to character's placement, for continuous we only do that if required
		void set_vr_anchor(Transform const& _vrAnchor);
		void transform_vr_anchor(Transform const& _localToVRAnchorTransform); // useful for sliding locomotion to move vr anchor, therefore pretend we're moving
		void turn_vr_anchor_by_180(); // this will rotate vr anchor by 180', this should be done right before game starts! 

		// locking as base, by default we should only lock for vr movement only - so player looking outside of our base won't switch base
		void lock_as_base(bool _requiresVRMovement = true, bool _requiresPlayerMovement = true) { lockAsBase = true; lockAsBaseRequiresVRMovement = _requiresVRMovement; lockAsBaseRequiresPlayerMovement = _requiresPlayerMovement;  }
		void unlock_as_base() { lockAsBase = false; lockAsBaseRequiresVRMovement = false; lockAsBaseRequiresPlayerMovement = false; }

		void force_base_on(IModulesOwner* _onto); // will force and keep it there

		Array<IModulesOwner*> const & get_based() const { return based; }
		IModulesOwner* get_based_on() const { return basedOn.get(); }
		Transform const & get_placement_when_started_basing_on() const { an_assert(get_based_on()); return basedOn.get_placement_when_started_basing_on(); }

		Transform const & get_placement() const { return placement; }
		AVOID_CALLING_ Transform calculate_current_placement() const; // this goes through attachments and bases, should be used only in certain situations (for pilgrim grabbing on speedy objects)
		Transform const & get_prev_placement_offset() const { return prevPlacementOffset; }
		float get_prev_placement_offset_delta_time() const { return prevPlacementOffsetDeltaTime; }
		inline Transform const & get_os_to_ws_transform() const { return placement; } // object (module owner) space to world space
		inline Transform get_ws_to_os_transform() const { return get_os_to_ws_transform().inverted(); }
		Transform const & get_centre_of_presence_os() const { return presenceLinkCentreOS; }
		Vector3 get_centre_of_presence_WS() const { return placement.location_to_world(presenceLinkCentreOS.get_translation()); }
		Transform get_centre_of_presence_transform_WS() const { return placement.to_world(presenceLinkCentreOS); }
		Vector3 get_random_point_within_presence_os(float _radiusCoef = 1.0f) const;
		int get_three_point_of_presence_os(ArrayStatic<Vector3, 3> & _threePointOffsetsOS) const;

		float calculate_flat_distance_for_nav(Vector3 const& _point) const;
		float calculate_flat_distance(Vector3 const& _point) const;

		void set_scale(float _scale = 1.0f);

		bool get_presence_primitive_info(OUT_ Vector3 & _locA, OUT_ Vector3 & _locB, OUT_ float & _radius) const;

		Transform get_placement_for_nav() const { Concurrency::ScopedSpinLock lock(placementNavLock, TXT("ModulePresence::get_placement_for_nav")); Transform result = placementNav; return result; }

		// sets both
		void set_velocity_linear(Vector3 const& _velocity) { velocityLinear = _velocity; nextVelocityLinear = _velocity; useInitialVelocityLinearOS.clear(); an_assert(nextVelocityLinear.is_ok()); }
		void set_velocity_rotation(Rotator3 const & _velocity) { velocityRotation = _velocity; nextVelocityRotation = _velocity; }
		Vector3 const & get_velocity_linear() const { return velocityLinear; }
		Rotator3 const & get_velocity_rotation() const { return velocityRotation; }

		Vector3 const & get_base_velocity_linear() const { return baseVelocityLinear; }
		Rotator3 const & get_base_velocity_rotation() const { return baseVelocityRotation; }

		void set_next_velocity_linear(Vector3 const & _velocity) { nextVelocityLinear = _velocity; an_assert_immediate(nextVelocityLinear.is_ok()); }
		void add_next_velocity_linear(Vector3 const & _velocity) { nextVelocityLinear += _velocity; an_assert_immediate(nextVelocityLinear.is_ok()); }
		void set_next_velocity_rotation(Rotator3 const & _velocity) { nextVelocityRotation = _velocity; an_assert_immediate(nextVelocityRotation.is_ok()); }
		void add_next_velocity_rotation(Rotator3 const & _velocity) { nextVelocityRotation += _velocity; an_assert_immediate(nextVelocityRotation.is_ok()); }
		Vector3 const & get_next_velocity_linear() const { return nextVelocityLinear; }
		Rotator3 const & get_next_velocity_rotation() const { return nextVelocityRotation; }

		void clear_velocity_impulses();
		void add_velocity_impulse(Vector3 const & _impulse); // world space
		void add_orientation_impulse(Rotator3 const & _impulse);

		// these affect vr anchor (one frame displacement)
		void force_location_displacement_vr_anchor(Vector3 const& _locationDisplacement) { forceLocationDisplacementVRAnchor = _locationDisplacement; }
		void force_orientation_displacement_vr_anchor(Quat const& _orientationDisplacement) { forceOrientationDisplacementVRAnchor = _orientationDisplacement; }
		
		// one frame displacements
		void set_next_location_displacement(Vector3 const& _locationDisplacement) { nextLocationDisplacement = _locationDisplacement; an_assert_immediate(nextLocationDisplacement.is_ok()); }
		void add_next_location_displacement(Vector3 const & _locationDisplacement) { nextLocationDisplacement += _locationDisplacement; an_assert_immediate(nextLocationDisplacement.is_ok()); }
		Vector3 const & get_next_location_displacement() const { return nextLocationDisplacement; }
		void set_next_orientation_displacement(Quat const & _orientationDisplacement) { nextOrientationDisplacement = _orientationDisplacement; }
		void add_next_orientation_displacement(Quat const & _orientationDisplacement) { nextOrientationDisplacement = nextOrientationDisplacement.to_world(_orientationDisplacement); }
		Quat const & get_next_orientation_displacement() const { return nextOrientationDisplacement; }

		Transform get_requested_relative_look(bool _withAdditionalOffset = false) const;
		Transform get_requested_relative_look_for_vr(bool _withAdditionalOffset = false) const;

		bool is_requested_relative_hand_accurate(int _handIdx) const;
		Transform get_requested_relative_hand(int _handIdx) const;
		Transform get_requested_relative_hand_for_vr(int _handIdx) const;

		Transform const & get_eyes_relative_look() const { return eyesRelativeLook; }
		bool do_eyes_have_fixed_location() const { return eyesFixedLocation; }

		float get_vertical_look_offset() const { return verticalLookOffset; }
		void set_vertical_look_offset(float _verticalLookOffset) { verticalLookOffset = _verticalLookOffset; }
		void make_head_bone_os_safe(REF_ Transform& _headBoneOS) const;

		void place_in_room(Room* _inRoom, Vector3 const & _location, Name const & _reason = Name::invalid());
		void place_in_room(Room* _inRoom, Transform const & _placement, Name const & _reason = Name::invalid());
		void place_within_room(Transform const & _placement);
		Transform place_at(IModulesOwner const * _object, Name const & _socketName = Name::invalid(), Optional<Transform> const & _relativePlacement = NP, Optional<Vector3> const & _absoluteLocationOffset = NP); // returns intoRoomTransform if was placed in other room due to offset
		bool get_placement_to_place_at(IModulesOwner const * _object, Name const & _socketName, Optional<Transform> const & _relativePlacement, OUT_ Room*& _inRoom, OUT_ Transform & _placement) const;

		void add_to_room(Room* _room, DoorInRoom * _enteredThroughDoor = nullptr, Name const & _reason = Name::invalid());
		void remove_from_room(Name const & _reason = Name::invalid());
		
		void invalidate_presence_links(); // todo: maybe move it somewhere else - deactivate?
		void update_does_require_building_presence_links(Concurrency::Counter* _excessPresenceLinksLeft);
		bool does_require_building_presence_links(PresenceLinkOperation::Type _operation) const;

		Transform calculate_move_centre_offset_os() const;

		Room* get_in_room() const { return inRoom; }
		World* get_in_world() const;
		
		PresenceLink const* get_presence_links() const { return presenceLinks; }
		float get_presence_size() const;
		float get_presence_radius() const { return presenceLinkRadius; }
		Vector3 const & get_presence_centre_distance() const { return presenceLinkCentreDistance; }

		Vector3 const & get_gravity() const { return gravity; }
		Vector3 const & get_gravity_dir() const { return gravityDir; }
		Vector3 const & get_gravity_dir_from_hit_locations() const { return gravityDirFromHitLocations; }
		bool do_gravity_presence_traces_touch_surroundings() const { return gravityPresenceTracesTouchSurroundings; }
		Vector3 get_last_gravity_presence_traces_touch_OS() const { return lastGravityPresenceTracesTouchOS; }
		Vector3 get_environment_up_dir() const { return !gravityDir.is_zero() ? -gravityDir : placement.get_orientation().get_z_axis(); }
		Transform calculate_environment_up_placement() const;

		bool does_allow_falling() const { return disallowFallingTimeLeft == 0.0f; }
		void allow_falling() { disallowFallingTimeLeft = 0.0f; }
		void disallow_falling(float _forTime) { disallowFallingTimeLeft = _forTime; }

		float get_floor_level_offset_along_up_dir() const { return floorLevelOffsetAlongUpDir; }
		Range const & get_floor_level_match_offset_limit() const { return floorLevelMatchOffsetLimit; }
		Vector3 const & get_centre_of_mass() const { return centreOfMass; }

		void debug_draw(Colour const & _colour = Colour::green, float _alphaFill = 0.2f, bool _justPresence = false) const;
		void debug_draw_base_info() const;
		void log_base_info(LogInfoContext& _context) const;

		void log(LogInfoContext & _context) const;
		void log_presence_links(LogInfoContext & _context) const;

	private:
		Optional<Range3> const& get_safe_volume() const { return safeVolume; }

	public:
		AVOID_CALLING_ bool does_require_update_eye_look_relative() const;
		void update_eye_look_relative();

	public:
		enum CeaseToExistWhenNotAttachedFlag
		{
			Param = 1,
			User  = 2,
			Auto  = 4,
		};
		void cease_to_exist_when_not_attached(bool _ceaseToExistWhenNotAttached = true, int _flag = CeaseToExistWhenNotAttachedFlag::User);
		bool should_cease_to_exist_when_not_attached() const { return ceaseToExistWhenNotAttached != 0; }

		bool is_attached() const { return attachment.attachedTo != nullptr; }
		bool is_attached_at_all_to(IModulesOwner const * _to) const;
		bool is_attached_via_socket() const { return is_attached() && attachment.toSocket.is_valid() && attachment.viaSocket.is_valid(); }
		IModulesOwner* get_attached_to() const { return attachment.attachedTo; }
		PresencePath const * get_path_to_attached_to() const { return attachment.attachedTo? &attachment.pathToAttachedTo : nullptr; }
		bool get_attached_to_bone_info(OUT_ int & _boneIdx, OUT_ Transform & _relativePlacement) const { _boneIdx = attachment.toBoneIdx; _relativePlacement = attachment.relativePlacement; return is_attached() && attachment.toBone.is_set(); }
		bool get_attached_to_socket_info(OUT_ int & _socketIdx, OUT_ Transform & _relativePlacement) const { _socketIdx = attachment.toSocket.is_valid()? attachment.toSocket.get_index() : NONE; _relativePlacement = attachment.relativePlacement; return is_attached() && attachment.toSocket.is_valid(); }
		SocketID const & get_attached_to_socket() const { return attachment.toSocket; }
		void get_attached_placement_for_top(OUT_ Room*& _inRoom, OUT_ Transform & _placementWS, IModulesOwner* _stopAt, OUT_ IModulesOwner** _top = nullptr) const;
		bool get_attached_relative_placement_info(OUT_ Transform & _relativePlacement) const { _relativePlacement = attachment.relativePlacement; return is_attached() && ! attachment.toBone.is_set(); }
		bool has_attachments() const { return ! attached.is_empty(); }
		// request when we can't move (because we're in the logic part!)
		void request_attach(IModulesOwner* _object, bool _following = false, Transform const & _relativePlacement = Transform::identity, bool _doAttachmentMovement = true);
		void request_attach_to_using_in_room_placement(IModulesOwner* _object, bool _following = false, Transform const & _inRoomPlacement = Transform::identity, bool _doAttachmentMovement = true);
		void request_attach_to_bone_using_in_room_placement(IModulesOwner* _object, Name const & _bone, bool _following = false, Transform const & _inRoomPlacement = Transform::identity, bool _doAttachmentMovement = true);
		void request_attach_to_socket_using_in_room_placement(IModulesOwner* _object, Name const & _bone, bool _following = false, Transform const & _inRoomPlacement = Transform::identity, bool _doAttachmentMovement = true);
		void request_attach_to_socket(IModulesOwner* _object, Name const & _socket, bool _following = false, Transform const & _relativePlacement = Transform::identity, bool _doAttachmentMovement = true);
		// when we can move
		void attach_to(IModulesOwner* _object, bool _following = false, Transform const & _relativePlacement = Transform::identity, bool _doAttachmentMovement = true);
		void attach_to_using_in_room_placement(IModulesOwner* _object, bool _following = false, Transform const & _inRoomPlacement = Transform::identity, bool _doAttachmentMovement = true);
		void attach_to_bone(IModulesOwner* _object, Name const & _bone, bool _following = false, Transform const & _relativePlacement = Transform::identity, bool _doAttachmentMovement = true);
		void attach_to_bone_using_in_room_placement(IModulesOwner* _object, Name const & _bone, bool _following, Transform const & _inRoomPlacement, bool _doAttachmentMovement = true);
		void attach_to_socket(IModulesOwner* _object, Name const & _socket, bool _following = false, Transform const & _relativePlacement = Transform::identity, bool _doAttachmentMovement = true);
		void attach_to_socket_using_in_room_placement(IModulesOwner* _object, Name const & _socket, bool _following, Transform const & RoomPlacement, bool _doAttachmentMovement = true);
		void attach_via_socket(IModulesOwner* _object, Name const & _viaSocket, Name const & _toSocket, bool _following = false, Transform const & _relativePlacement = Transform::identity, bool _doAttachmentMovement = true);
		void do_post_attach_to_movement();
		void update_attachment_relative_placement(Transform const & _relativePlacement);
		void detach();
		void issue_reattach(); // this resets attachment path - most useful when we want to move us into closer/most appropriate location
						 // imagine that both objects moved through different rooms - they are in the same relative distance, but because of the doors, their string pulled distance is getting bigger and bigger
						 // we reattach to reappear in same room (most likely same) as object we're attached to
		void detach_attached();
		void calculate_distances_to_attached_to(OUT_ float & _relativeDistance, OUT_ float & _stringPulledDistance, Transform const & _attachedToRelativeOffset = Transform::identity, Transform const & _relativeOffset = Transform::identity) const;
		void refresh_attached(); // refresh where they are attached as some inner stuff has changed
		void refresh_attachment(); // refresh own attachment info
		void make_attachment_valid_on_zero_scaled_bones(); // skip zero scaled bones (this is best to use with attach_to without movement and call do_post_attach_movement)
		
		bool has_anything_attached() const { return attached.get_size() > 0; } // using this may give false values due to not locking attached lock so it's more for visuals that are updated often
		Concurrency::SpinLock & access_attached_lock() { return attachedLock; }
		Array<IModulesOwner*> const & get_attached() const { an_assert(attachedLock.is_locked_on_this_thread()); return attached; }
		
		// will auto lock
		void for_every_attached(std::function<void(IModulesOwner*)> _attached);

	private:
		void attach_to_bone_index(IModulesOwner* _object, int _boneIdx, bool _following, Transform const & _relativePlacement, bool _doAttachmentMovement);

	public: // advancements
		void quick_pre_update_presence(); // has to be called. if doesn't require update presence, this has to be called instead
		bool does_require_update_presence() const { return requiresUpdatePresence; }

		// advance - begin
		static void advance__update_presence(IMMEDIATE_JOB_PARAMS);
		static void advance__prepare_move(IMMEDIATE_JOB_PARAMS);
		static void advance__do_move(IMMEDIATE_JOB_PARAMS);
		static void advance__post_move(IMMEDIATE_JOB_PARAMS);
		static void advance__build_presence_links(IMMEDIATE_JOB_PARAMS);
		// advance - end

		static void advance_vr_important__looks(IModulesOwner* _modulesOwner); // this is useful to handle after processing vr controls
		
		bool does_require_post_move() const; // for any other reason than actual move
		bool does_require_do_move() const;
		bool does_require_forced_prepare_move() const;

	public: // observers
		void add_presence_observer(IPresenceObserver* _observer);
		void remove_presence_observer(IPresenceObserver* _observer);

	public: // velocity storing
		void store_velocities(int _storeVelocities = 0) { storeVelocities = _storeVelocities; }
		Vector3 const & get_prev_velocity_linear(int _idx = 0) { _idx = min(_idx, prevVelocities.get_size() - 1); return _idx < 0 ? get_velocity_linear() : prevVelocities[_idx]; }
		Vector3 calc_average_velocity() const;
		Vector3 get_max_velocity() const;

	public:
		bool move_through_doors(REF_ Transform & _nextPlacement, OUT_ Room *& _intoRoom) const; // from centre to next placement (that is currently in same room as presence) (will fill _intoRoom with current room)
		/**
		 *	It moves through initial room to start of trace so that is not required to be done separately
		 *
		 *	small example:
				Framework::CheckCollisionContext checkCollisionContext;
				checkCollisionContext.collision_info_needed();
				checkCollisionContext.use_collision_flags(__flags__); // imo->get_collision()->get_collides_with_flags()
				checkCollisionContext.ignore_temporary_objects();
				checkCollisionContext.avoid(__owner__); // imo
				Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
				collisionTrace.add_location(__startLocation__);
				collisionTrace.add_location(__endLocation__);
				Framework::CheckSegmentResult result;
				Framework::RelativeToPresencePlacement rtppIfRequired;
				rtppIfRequired.set_owner(get_owner());
				if (modulePresence->trace_collision(AgainstCollision::Precise, collisionTrace, result, Framework::CollisionTraceFlag::ResultInPresenceRoom, checkCollisionContext, &rtppIfRequired))
				{
					// collided
				}
		 *
		 */
		bool trace_collision(AgainstCollision::Type _againstCollision, CollisionTrace const & _collisionTrace, REF_ CheckSegmentResult & _result, CollisionTraceFlag::Flags _flags, CheckCollisionContext & _context, RelativeToPresencePlacement * _fillRelativeToPresencePlacement = nullptr) const;

	protected:
		void internal_add_to_room(Room* _room);
		void internal_remove_from_room();

		void change_room(Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors = nullptr, Name const & _reason = Name::invalid());
		void internal_change_room(Room * _intoRoom, Transform const & _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const * _throughDoors);

	protected:
		void update_gravity();
		void update_based_on();

		// used excusively by BasedOn
		void add_based(IModulesOwner* _basedOnThisOne);
		void remove_based(IModulesOwner* _basedOnThisOne);

		void drop_based();

	protected:
		void prepare_move(IModulesOwner * modulesOwner, float _deltaTime);
		void do_move(IModulesOwner * modulesOwner); // should remain non virtual as it is low level movement

		// do not virtualise above methods, add post/pre
		/* no need to be virtual ATM */ void post_move(IModulesOwner* modulesOwner);

	protected: friend class ModuleAppearance;
		void calculate_final_pose_and_attachments_1(float _deltaTime, PoseAndAttachmentsReason::Type _reason);
		void do_attachment_movement(float _deltaTime, PoseAndAttachmentsReason::Type _reason);
		bool does_require_appearance_bones_bounding_box() const { return updatePresenceLinkSetupFromAppearancesBonesBoundingBox; }
		bool does_require_update_of_presence_link_setup() const { return updatePresenceLinkSetup; }

	private: friend class PresenceLink;
		void add_presence_link(PresenceLinkBuildContext const & _context, PresenceLink* _link);
		void sync_remove_presence_link(PresenceLink* _link); // SYNC!

	private:
		struct InternalPresenceLinkBuildContext
		{
			Room* inRoom;
			PlaneSet throughDoorPlanes; // used if should accumulate
			DoorInRoom* throughDoor; // inRoom
			Transform currentTransformFromOrigin;
			int currentDepth;
			Vector3 currentCentre;
			bool useSegment = false;
			SegmentSimple currentSegment;
#ifdef AN_DEBUG_RENDERER
			Vector3 prevCentre;
			int depth;
#endif
			float radiusLeft; // actual presence + collisions + light source
			float actualPresenceRadiusLeft;
			float collisionRadiusLeft;
			float lightSourceRadiusLeft;
			bool validForCollision;
			bool validForCollisionGradient;
			bool validForCollisionDetection;
			InternalPresenceLinkBuildContext(Room* _inRoom);
			InternalPresenceLinkBuildContext(InternalPresenceLinkBuildContext const & _other);
		};
		void build_presence_link(PresenceLinkBuildContext const & _buildContext, InternalPresenceLinkBuildContext const & _context, PresenceLink* _linkTowardsOwner, int _depthLeft);

	public: // Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void ready_to_activate();
		override_ void activate();

	protected: // ai communication
		void ai_create_message_leaves_room(Room* _toRoom = nullptr, DoorInRoom* _throughDoor = nullptr, Name const & _reason = Name::invalid());
		void ai_create_message_enters_room(DoorInRoom * _throughDoor = nullptr, Name const & _reason = Name::invalid());

	protected: // expose some for derived classes
		Transform & access_next_placement() { return nextPlacement; }

		void set_placement_for_nav(Transform const & _placement) { Concurrency::ScopedSpinLock lock(placementNavLock, TXT("ModulePresence::set_placement_for_nav")); placementNav = _placement; }
		void copy_placement_for_nav() { Concurrency::ScopedSpinLock lock(placementNavLock, TXT("ModulePresence::copy_placement_for_nav")); placementNav = placement; }

	protected:
		struct BasedOn
		: public IPresenceObserver
		{
		public:
			BasedOn(IModulesOwner* _owner, ModulePresence* _ownerPresence) : owner(_owner), ownerPresence(_ownerPresence) {}
			~BasedOn() { clear(); }
			void clear() { set(nullptr, Transform::identity); }
			void set(IModulesOwner* _basedOn, Transform const & _placementWhenStartedBasingOn);
			bool is_set() const { return basedOn != nullptr && presence != nullptr; }
			IModulesOwner* get() const { return basedOn; }
			ModulePresence* get_presence() const { return presence; }
			Transform const & get_placement_when_started_basing_on() const { return placementWhenStartedBasingOn; }
			bool is_based_through_doors() const { return basedOnThroughDoors; }

			Transform transform_from_owner_to_base(Transform const& _ws) const;
			Transform transform_from_base_to_owner(Transform const& _ws) const;

		public: // IPresenceObserver
			implement_ void on_presence_changed_room(ModulePresence* _presence, Room* _intoRoom, Transform const& _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const* _throughDoors);
			implement_ void on_presence_removed_from_room(ModulePresence* _presence, Room* _room) { clear(); }
			implement_ void on_presence_destroy(ModulePresence* _presence) { clear(); }

		private:
			// basedLock is used for safety
			IModulesOwner* owner;
			ModulePresence* ownerPresence; // of basedOn
			IModulesOwner* basedOn = nullptr;
			ModulePresence* presence = nullptr; // of basedOn
			Transform placementWhenStartedBasingOn; // for reference (although it is actually a hack)
			PresencePath pathToBase; // only if base is "through rooms"
			bool basedOnThroughDoors = false;
		};

	protected:
		void post_placement_change();
		void post_placed();

	private:
		static int s_ignoreAutoVelocities; // it is used to avoid situations when we paused and controls went all crazy

		ModulePresenceData const * modulePresenceData = nullptr;

		Concurrency::SpinLock observersLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.observersLock"));
		Array<IPresenceObserver*> observers;

		bool requiresUpdatePresence = true;
		bool requiresForcedUpdateLookViaController = true; // this is for immovable objects that require to have requested look updated

		bool doesRequireBuildingPresenceLinks[PresenceLinkOperation::COUNT];

		bool createAIMessages = true;

		int ceaseToExistWhenNotAttached = 0; // if not attached, will be destroyed

		bool isPlayerMovement = false;	// for some functionalities we may want to limit player movement
		bool isVRMovement = false;		// if character is moving through vr, a lot changes in how it behaves, moves, how camera and look is treated etc. see above
										// note that if sliding locomotion is used in the game, this gets overriden partially (ie different input comes to controller)
		bool isVRAttachment = false;	// if depends on vr controls (use is_vr_attachment as it checks if we're attached to such one!
		bool moveUsingEyes = false;		// this means that instead of presence link centre offset os, character will be moved using eyes,
										// note that this means that presence links will be built using this offset to go through doors properly!
										// same for attachment movement
		bool moveUsingEyesZOnly = false; // overides above, affects only Z component
		bool ignoreImpulses = false;	// will ignore any impulses (mostly from explosions)
		bool noVerticalMovement = false;// no vertical movement, can move vertically with base but vertical component of velocity will be ignored

		Room* inRoom = nullptr;
		//public weak Place? inPlace = null;
		Transform placement = Transform::identity;
		Transform prevPlacementOffset = Transform::identity; // offset to previous placement (how much did we move but in reverse, how much would we have to move now to be at the prev placement)
		float prevPlacementOffsetDeltaTime = 0.0f; // delta time for which this offset has been calculated
		Transform preMovePlacement = Transform::identity; // used as reference for other objects during do_move (for bases that stay in the same room this won't change, for base-through-doors we keep the right order of calling)
		Transform preparedMovePlacement = Transform::identity; // used as reference for other objects during do_move (same as nextPlacement)
		Transform nextPlacement; // next placement - for move
		float nextPlacementDeltaTime = 0.0f;
		// this is vr anchor used for movement, to tell where in the world are we
		Transform vrAnchor = Transform::identity; // where in vr space is point 0,0,0
		Transform vrAnchorLastDisplacement = Transform::identity; // last displacement (last frame)
		Vector3 velocityLinear = Vector3::zero;
		Optional<Vector3> initialVelocityLinearOS; // this is used when preparing the first movement (forced!)
		Optional<Vector3> useInitialVelocityLinearOS; // to be used once
		Rotator3 velocityRotation = Rotator3::zero;
		// when base moves this is calculated
		Vector3 baseVelocityLinear = Vector3::zero;
		Rotator3 baseVelocityRotation = Rotator3::zero;
		// calculated this frame to be used in move in this frame
		Vector3 nextVelocityLinear = Vector3::zero;
		Rotator3 nextVelocityRotation = Rotator3::zero;
		// one frame
		Vector3 nextLocationDisplacement = Vector3::zero;
		Quat nextOrientationDisplacement = Quat::identity;
		// one frame that affect vr anchor too
		Optional<Vector3> forceLocationDisplacementVRAnchor;
		Optional<Quat> forceOrientationDisplacementVRAnchor;
		// parameters
		float floorLevel = 0.0f; // this is where in object space (against placement) floor should be by default
		Range floorLevelMatchOffsetLimit = Range(-0.5f, 1.0f); // this is the acceptable level of floor
		Vector3 centreOfMass = Vector3::zero; // local, object space
		// end of parameters

		// can be used but try avoiding going between rooms (resets!)
		float useRoomVelocity = 0.0f;
		float useRoomVelocityInTime = 0.0f;
		RUNTIME_ float useRoomVelocityTime = 0.0f;

		int storeVelocities = 0; // store velocities in case we need average or max velocity
		Array<Vector3> prevVelocities;

		Concurrency::SpinLock velocityImpulsesLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.velocityImpulsesLock"));
		Array<Vector3> velocityImpulses; // impulses are applied once during prepare_move

		Concurrency::SpinLock orientationImpulsesLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.orientationImpulsesLock"));
		Array<Rotator3> orientationImpulses; // impulses are applied once during prepare_move

		mutable Concurrency::SpinLock placementNavLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.placementNavLock"));
		Transform placementNav = Transform::identity; // copy of placement, use set_placement_for_nav

		// look relative - both parameters might be adjusted, they are truly relative to look anchor placement (or base head bone or whatever else)
		Transform requestedRelativeLook = Transform::identity; // requested (by controller) relative look, for seated experience it is relative to base head, for room it is relative to floor level
		Quat lookOrientationAdditionalOffset = Quat::identity; // additional offset that is taken from controller
		float verticalLookOffset = 0.0f; // only for crouching, debug?

		Transform requestedRelativeHands[2];
		bool requestedRelativeHandsAccurate[2];

		Transform eyesRelativeLook = Transform::identity; // eye placement build from requested relative look or from skeleton through appearance controllers

		struct Attachment
		{
			bool makeZeroScaleValid = true; // this will turn all zero scaled transforms into valid transforms, this way we are able to deal with things that disappear, are scaled down
			bool isFollowing = false; // following attachment is attachment that tries to follow relative placement instead of always teleporting from original "attached to" to relative one - you should prefer using following for movement (as it also gives better continuity for movement) but for objects attached permanently, choose "not following"
			IModulesOwner* attachedTo = nullptr;
			PresencePath pathToAttachedTo; // used only if "isFollowing" is set
			Optional<Name> toBone; // can be not set as long as toBoneIdx is set
			int toBoneIdx = NONE;
			SocketID toSocket; // to resolve into "toBone"
			SocketID viaSocket; // attached via socket, ATM this can be only used with toSocket and will be an active socket attachment (relative placement is relative to toSocket)
			Transform relativePlacement = Transform::identity; // it is always relative to whatever it was attached to, bone, socket (if via socket, it will calculate that extra via socket info (it should be always between via and to)

			Transform calculate_placement_of_attach(IModulesOwner* _owner, bool _calculateCurrentPlacement = false) const; // calculates where we should be attached, in attachedTo's room
		};

		Attachment attachment; // if attached, moves in post move
		Concurrency::SpinLock attachmentLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.attachmentLock"));
		Concurrency::SpinLock attachedLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.attachedLock"));
		Array<IModulesOwner*> attached; // all attached objects to be moved

		struct AttachmentRequest
		{
			enum Type
			{
				None,
				AttachToUsingInRoomPlacement,
				AttachToUsingRelativePlacement
			} type;
			SafePtr<IModulesOwner> toObject;
			Name toBone;
			Name toSocket;
			bool following;
			Transform placement; // in room or relative
			bool doAttachmentMovement;
		};
		AttachmentRequest attachmentRequest;
		Concurrency::SpinLock attachmentRequestLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.attachmentRequestLock"));

		bool lockAsBase = false; // if locked, objects based on it, can't change their base (unless they change room), new objects can't base on it either (at least not through presence traces)
		bool lockAsBaseRequiresVRMovement = false; // locked but has to be VR movement
		bool lockAsBaseRequiresPlayerMovement = false; // as above, if both set, only one has to be fulfilled
		BasedOn basedOn; // only within room
		bool forcedBasedOn = false; // if forced, cannot be changed, has to be changed by forcing to null
		Optional<Vector3> basedOnDoneAtLoc; // if we move more than 1cm we udpate it
		::System::TimeStamp basedOnDoneAtTime;
		Concurrency::SpinLock basedLock = Concurrency::SpinLock(TXT("Framework.ModulePresence.basedLock"));
		Array<IModulesOwner*> based; // objects based on this one

		// parameters to read look relative placement & orientation (from appearance, when it is processed by controllers)
		Name eyesLookRelativeAppearance;
		Meshes::BoneID eyesLookRelativeBone;
		SocketID eyesLookRelativeSocket; // more important
		bool eyesFixedLocation = false;

		bool teleportRequested = false;
		int teleportRequestedPriority = 0;
		bool teleported = false;
		bool silentTeleport = false; // although if teleports between rooms, it can't be silent!
		Room* teleportIntoRoom = nullptr;
		Transform teleportToPlacement;
		Optional<Vector3> teleportWithVelocity;
		Optional<bool> teleportBeVisible;
		Optional<bool> teleportKeepVelocityOS;
		bool teleportFindVRAnchor = false;

#ifdef AN_ALLOW_BULLSHOTS
		Optional<Transform> bullshotPlacement;
#endif

		// special movement cases
		bool shouldAvoidGoingThroughCollision = false; // this is used in general
		bool shouldAvoidGoingThroughCollisionForSlidingLocomotion = false; // this is used only for sliding locomotion
		bool shouldAvoidGoingThroughCollisionForVROnLockedBase = false; // this is used for vr movement when locked to base to prevent certain elevator issues
		bool beAlwaysAtVRAnchorLevel = false; // this is used to keep placement always at the same level as vr anchor, otherwise we may get bad things happening
		bool keepWithinSafeVolumeOnLockedBase = false; // if base is moving and is locked, we're forced to be kept within safe volume, to avoid falling out from an elevator etc

		// cache from appearance and collision?
		CACHED_ bool updatePresenceLinkSetup = false;
		bool updatePresenceLinkSetupContinuously = false; // automatically if using bones
		bool updatePresenceLinkSetupFromMovementCollisionPrimitive = false;
		bool updatePresenceLinkSetupFromMovementCollisionPrimitiveAsSphere = false;
		float updatePresenceLinkSetupMakeBigger = 0.0f;
		bool updatePresenceLinkSetupFromMovementCollisionBoundingBox = false;
		bool updatePresenceLinkSetupFromPreciseCollisionBoundingBox = false;
		bool updatePresenceLinkSetupFromAppearancesBonesBoundingBox = false; // will update centre offset and radius using bones bounding box from main appearance
		Transform presenceLinkCentreOS = Transform::identity;
		Optional<Transform> moveCentreOffsetOS;
		bool presenceLinkAccumulateDoorClipPlanes = true; // will be keeping clip planes from all doors we went through
		bool presenceLinkClipLess = false; // if clip less, presence link will be clipped against door only if centre is inside door hole
		bool presenceLinkDoNotClip = false; // do not clip at all
		bool presenceLinkAlwaysClipAgainstThroughDoor = true; // always clip against door we went through
		float presenceLinkRadius = 1.5f;
		float presenceLinkRadiusClipLessPt = -1.0f; // works as clip less if current distance is lower than (1.0 - pt) * radius (radiusLeft/radius is above this value), percentage value, the further we are, the more we clip!
		float presenceLinkRadiusClipLessRadius = -1.0f; // as above but distance has to be lower than this value (if with both, at least any has to be present
		Optional<float> lightSourceBasedPresenceRadius; // light sources may want to override this solely to propagate lights
		Vector3 presenceLinkCentreDistance = Vector3::zero; // if non-zero capsule - distance between centres
		bool forceUpdatePresenceLinks = false;
		Optional<::System::TimeStamp> lazyUpdatePresenceLinksTS; // when we want to update but not particualiry every frame
		PresenceLink* presenceLinks = nullptr;
		struct PresenceLinksBuiltFor
		{
			Room* room = nullptr; // just as a reference to a room
			Transform placement = Transform::identity;
			Transform presenceLinkCentreOS = Transform::identity;
			float presenceLinkRadius = 0.0f;
			float presenceLinkRadiusClipLessPt = -1.0f;
			float presenceLinkRadiusClipLessRadius = -1.0f;
			Optional<float> lightSourceBasedPresenceRadius;
			Vector3 presenceLinkCentreDistance = Vector3::zero;
			float collisionRadius = 0.0f;
		} presenceLinksBuiltFor;
		bool alwaysRebuildPresenceLinks = false; // to avoid comparing, this is useful for player and larger objects
		bool presenceInSingleRoom = false; // if this and stay in room is set, presence link is built once!
		CACHED_ bool presenceInSingleRoomTO = false; // temporary objects should have rebuild presence links the ususal way, not using excess counters
		bool canChangeRoom = true; // can move between rooms
		bool baseThroughDoors = false; // if is a base, is base that may take objects through rooms
		bool canBeBase = false; // other objects can be based on it
		bool canBeBaseForPlayer = false; // for player, if not defined otherwise, assumed that it is set the same as canBeBase
		bool canBeBasedOn = true; // can be based on other objects (by default, true, requires base presence traces), use can_be_based_on to check for traces too, if attached does ignore base
		bool canBeVolumetricBase = false; // as a base but volumetric
		bool canBeBasedOnVolumetric = false; // works with volumetric
		bool volumetricBaseFromAppearancesMesh = false;
		Optional<Range3> volumetricBaseProvided; // if both volumetricBaseFromAppearancesMesh and volumetricBaseProvided is set, will use max of two
		Optional<Range3> volumetricBase;
		bool safeVolumeFromMeshNodes = false; // will use mesh nodes called "safeVolume"
		bool safeVolumeFromVolumetricBase = false; // will copy volumetric base
		Vector3 safeVolumeExpandBy = Vector3::zero; // may be smaller - this is the final adjustment, modifies also provided (!)
		Optional<Range3> safeVolumeProvided; // if both safeVolumeFromMeshNodes+safeVolumeFromVolumetricBase and volumetricBaseProvided is set, will use max of two
		Optional<Range3> safeVolume;

		CACHED_ bool collides = false; // cached information if collides during this frame with anything
		Vector3 gravity = Vector3::zero; // dir + force
		Vector3 gravityDir = Vector3::zero;
		float floorLevelOffsetAlongUpDir = 0.0f; // how much along up axis (z) floor actually is (checked with presence checks, compared against floorLevel)
										  // allows to have gap between floating collision capsule and floor, makes movement on uneven terrain easier
		Vector3 gravityDirFromHitLocations = Vector3::zero;
		bool gravityPresenceTracesTouchSurroundings = false; // if not, it is in air
		Vector3 lastGravityPresenceTracesTouchOS = Vector3::zero;

		bool immuneToKillGravity = true;
		mutable Room* cachedKillGravityForRoom = nullptr;
		mutable Optional<Vector3> cachedKillGravityDir;
		mutable Optional<float> cachedKillGravityDistance;

		float disallowFallingTimeLeft = 0.1f; // a bit after spawn

		mutable int postMoveFrameInterval = -1;

		SimpleVariableInfo provideLinearSpeedVar; // will update during movement

		void update_vr_anchor_from_room();

		void internal_update_into_room(Transform const & _intoRoomTransform);

		void update_placements_for_rendering_in_presence_links(Transform const & _placementChange);

#ifdef AN_DEVELOPMENT
		struct DebugPresenceTraceHit
		{
			Vector3 location;
			Vector3 normal;
			DebugPresenceTraceHit() {}
			DebugPresenceTraceHit(Vector3 const & _location, Vector3 const & _normal) : location(_location), normal(_normal) {}
		};
		Array<DebugPresenceTraceHit> debugPresenceTraceHits;
#endif

		Transform transform_based_movement_placement(Transform const & _basedNextPlacement) const; // this is used for "based movement" (when base moves) to transform any placement 

#ifdef AN_DEVELOPMENT
		void assert_we_dont_move() const;
		void assert_we_may_move() const;
#else
		inline void assert_we_dont_move() const {}
		inline void assert_we_may_move() const {}
#endif

		inline void store_velocity_if_required();

		friend interface_class IModulesOwner;

		inline bool should_clip_less(float _radiusLeft) const;

		inline bool does_require_lazy_update_presence_links() const;

#ifdef WITH_PRESENCE_INDICATOR
	public:
		static bool showPresenceIndicator;

	private:
		bool presenceIndicatorUpdatedFor = false;
		bool presenceIndicatorVisible = false;

		void update_presence_indicator();
#endif

	};
};


