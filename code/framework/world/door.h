#pragma once

#include "doorType.h"
#include "worldObject.h"

#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"
#include "..\sound\soundSources.h"

#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\optional.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace VR
{
	struct Zone;
};

namespace Framework
{
	class DoorInRoom;
	class World;
	class SubWorld;
	class Room;

	struct DoorSoundsPlayer
	{
		void stop();
		void advance(Door* _door, DoorSounds const& _sounds, float _openTarget, float _openFactor, Transform const & _relPlacement = Transform::identity);

		bool advancedOnce = false; // if not advanced, we shouldn't be playing sounds as we moved

		bool prevMute = false;
		float prevOpenTarget = 0.0f;
		float prevOpenFactor = 0.0f;

		SoundSourcePtr closingSoundSource;
		SoundSourcePtr openingSoundSource;
	};

	/**
	 *	If door has tag "canBeBlocked", during checks (for POIs and spawn manager) if door is blocked, it is ignored if is blocked or not
	 */
	class Door
	: public SafeObject<Door>
	, public WorldObject
	{
		FAST_CAST_DECLARE(Door);
		FAST_CAST_BASE(WorldObject);
		FAST_CAST_END();
	public: // [demo hack]
		static bool demoHackLockAutoDoors;
		static bool hackCloseAllDoor; // don't want yet to make a proper mechanism for that, hence the hack

	public:
		static void set_nominal_door_size(Optional<float> const & _width, Optional<float> const & _height);

		static Optional<float> const & get_nominal_door_width() { return nominalDoorWidth; }
		static void set_nominal_door_width(float _nominalDoorWidth);

		static float get_nominal_door_height() { return nominalDoorHeight; }
		static void set_nominal_door_height(float _nominalDoorHeight);
		static float const get_default_nominal_door_height() { return 2.0f; }
		static float get_nominal_door_height_scale() { return nominalDoorHeight / get_default_nominal_door_height(); }

		static Transform reverse_door_placement(Transform const & _placement);

		static void set_vr_corridor_tags(Room* _to, Room const* _from, int _nestedOffset);

	public:
		Door(SubWorld* _inSubWorld, DoorType const * _doorType);
		~Door();

		float get_individual() const { return individual; }

	public:
		int get_game_related_system_flags() const { return gameRelatedSystemFlags; }
		bool is_game_related_system_flag_set(int _flag) const { return gameRelatedSystemFlags & _flag; }
		void set_game_related_system_flag(int _flag);
		void clear_game_related_system_flag(int _flag);

	public:
		bool is_visible() const { return visible; }
		void set_visible(bool _visible); // doesn't set immediately, uses game sync job to do so

	public:
		inline DoorHole const * get_hole() const { return doorType ? doorType->get_hole() : nullptr; }
		inline Vector2 get_hole_scale() const { return doorTimeScale * holeTimeScale * holeWholeScale; }
		inline float get_door_scale() const { return doorTimeScale; }
		inline ::System::ShaderProgramInstance const * get_door_hole_shader_program_instance() const { return doorHoleShaderProgramInstance.get_shader_program()? &doorHoleShaderProgramInstance : nullptr; } // only if used
		inline DoorType const * get_door_type() const { return doorType; }
		inline DoorType const * get_secondary_door_type() const { return secondaryDoorType; }
		inline DoorType const * get_forced_door_type() const { return doorTypeForced; }
		inline DoorType const * get_original_door_type() const { return doorTypeOriginal; }
		Tags const & get_tags() const { return tags; }
		
		SimpleVariableStorage & access_variables() { return variables; }
		SimpleVariableStorage const & get_variables() const { return variables; }

		void set_secondary_door_type(DoorType const * _doorType);

		void force_door_type(DoorType const * _doorType); // async!
		bool update_type(); // update door type, return true if changed

		Range2 calculate_compatible_size(DoorSide::Type _side) const;

#ifdef AN_DEVELOPMENT
		void dev_check_if_ok() const;
#endif

		void link(DoorInRoom* _dir);
		void link_a(DoorInRoom* _dir);
		void link_b(DoorInRoom* _dir);
		void unlink(DoorInRoom* _dir);

		inline SubWorld* get_in_sub_world() const { return inSubWorld; }

		inline bool is_linked_on_both_sides() const { return linkedDoorA && linkedDoorB; }
		inline bool is_linked(DoorInRoom const * _dir) const { return _dir == linkedDoorA || _dir == linkedDoorB; }

		inline bool is_linked_as_a(DoorInRoom const * _dir) const { return _dir == linkedDoorA; }
		inline bool is_linked_as_b(DoorInRoom const * _dir) const { return _dir == linkedDoorB; }

		DoorInRoom* get_linked_door_in(Room* _room) const;
		inline DoorInRoom* get_linked_door(int _idx) const { return _idx <= 0? linkedDoorA : linkedDoorB; }
		inline DoorInRoom* get_linked_door_a() const { return linkedDoorA; }
		inline DoorInRoom* get_linked_door_b() const { return linkedDoorB; }

		inline DoorInRoom* get_on_other_side(DoorInRoom const * _a) const { return _a == linkedDoorA ? linkedDoorB : (_a == linkedDoorB ? linkedDoorA : nullptr); }

		inline void block_environment_propagation(bool _block = true) { blocksEnvironmentPropagation = _block; }
		inline bool does_block_environment_propagation() const { return blocksEnvironmentPropagation || doorType->does_block_environment_propagation(); }

		inline float get_open_factor() const { return openFactor; }
		inline bool is_open_factor_still() const { return openFactor == openTarget; }
		inline float get_abs_open_factor() const { return abs(openFactor); }
		inline bool can_be_closed() const { return doorType->can_be_closed() || (secondaryDoorType && secondaryDoorType->can_be_closed()); }
		inline bool can_see_through_when_closed() const { return doorType->can_see_through_when_closed() && (!secondaryDoorType || secondaryDoorType->can_see_through_when_closed()); }
		DoorOperation::Type get_operation() const;
		bool is_operation_locked() const { return operationLocked; }
		DoorOperation::Type get_operation_if_unlocked() const { return operationIfUnlocked.get(get_operation()); }
		struct DoorOperationParams
		{
			ADD_FUNCTION_PARAM_DEF(DoorOperationParams, bool, allowAuto, allow_auto, true);
			ADD_FUNCTION_PARAM_DEF(DoorOperationParams, bool, forceOperation, force_operation, true); // to override lock
		};
		void set_operation(DoorOperation::Type _operation, DoorOperationParams const& _params = DoorOperationParams());
		void set_operation_lock(bool _lock);
		void set_initial_operation(Optional<DoorOperation::Type> const & _operation); // use when creating
		void set_auto_operation_distance(Optional<float> const& _autoOperationDistance);

		void mark_cant_move_through();
		bool can_move_through() const;

		bool is_relevant_for_movement() const;
		bool is_relevant_for_vr() const;

		bool is_new_placement_requested() const;

		float adjust_sound_level(float _soundLevel) const;

		void close();
		void open(float _side = 1.0f);
		void open_into(Room const * _room);
		inline bool is_closed() const { return openFactor == 0.0f; }
		inline bool is_open() const { return abs(openFactor) >= 1.0f; }
		inline bool is_opening() const { return abs(openTarget) > abs(openFactor); }

		void scale_door_time(float _scale, float _scaleTime);
		void scale_hole_time(float _scale, float _scaleTime);
		void set_relative_size(Vector2 const & _holeWholeScale) { holeWholeScale = _holeWholeScale; }
		Vector2 get_relative_size() const { return holeWholeScale; }

		// advance - begin
		static void advance__operation(IMMEDIATE_JOB_PARAMS);
		static void advance__physically(IMMEDIATE_JOB_PARAMS);
		static void advance__finalise_frame(IMMEDIATE_JOB_PARAMS);
		// advance - end

		bool does_require_advance_operation() const { return get_operation() != DoorOperation::Unclosable; }
		bool does_require_advance_physically() const { return get_operation() != DoorOperation::Unclosable || is_new_placement_requested() || visiblePending.is_set() || holeTimeScale != holeTimeScaleTarget || doorTimeScale != doorTimeScaleTarget; }
		bool does_require_finalise_frame() const { return sounds.does_require_advance(); }

		SoundSources const & get_sounds() const { return sounds; }
		SoundSources & access_sounds() { return sounds; }
		void set_mute(bool _mute = true);

	public:
		bool is_replacable_by_door_type_replacer() const;
		void set_replacable_by_door_type_replacer(Optional<bool> const& _replacableByDoorTypeReplacer) { replacableByDoorTypeReplacer = _replacableByDoorTypeReplacer; }

	public:
		DoorInRoom* change_into_room(RoomType const * _roomType, Room * _towardsRoom = nullptr, DoorType const * _changeDoorToTowardsRoomToDoorType = nullptr, tchar const * _proposedName = nullptr); // _towardsRoom, _changeDoorToTowardsRoomToDoorType -> we are going to change door pointing to towardsRoom into new door type, door returned is door in towardsRoom
		void change_into_vr_corridor(VR::Zone const & _beInZone, float _tileSize); // check implementation for more details
		float calculate_vr_width() const;
		static void make_vr_corridor(Room* corridor, DoorInRoom* corridorD0_andSource, DoorInRoom* corridorD1, VR::Zone const& _beInZone, float _tileSize);

		bool is_important_vr_door() const { return importantVRDoor; }
		void be_important_vr_door();

		bool is_world_separator_door() const { return worldSeparatorDoor; }
		bool is_world_separator_door_due_to(int _reason) const { return worldSeparatorDoor && (worldSeparatorReason & _reason); }
		int get_world_separator_reason() const { return worldSeparatorReason; }
		Room* get_room_separated_from_world() const { return separatesRoomFromWorld.get(); }
		void be_world_separator_door(Optional<int> const & _worldSeparatorReason = NP, Room* _separatesRoomFromWorld = nullptr);
		void pass_being_world_separator_to(Door* _otherDoor);

		float get_door_vr_corridor_priority(Random::Generator& _rg, float _default) const;
		RoomType const* get_door_vr_corridor_room_room_type(Random::Generator& _rg) const;
		RoomType const* get_door_vr_corridor_elevator_room_type(Random::Generator& _rg) const;

	public:
		Optional<float> const& get_vr_elevator_altitude() const;
		void set_vr_elevator_altitude(float _vrElevatorAltitude) { vrElevatorAltitude = _vrElevatorAltitude; }

	public: // WorldObject
		implement_ void ready_to_world_activate();
		implement_ void world_activate();

		implement_ String get_world_object_name() const { return String::printf(TXT("door d%p"), this); }

	public:
		void world_activate_linked_door_in_rooms(ActivationGroup* _ag);

	protected: friend class World;
			   friend class SubWorld;
		void set_in_sub_world(SubWorld* _subWorld) { inSubWorld = _subWorld; }

	private:
		static Optional<float> nominalDoorWidth; // used to scale doors when we require doors to be of specific width
		static float nominalDoorHeight; // used to scale doors when we require doors to be of specific height / by default doors are of height 2(m)

		SubWorld* inSubWorld;

		int gameRelatedSystemFlags = 0; // any super game related flags (like ability to cease by spawn managers)

		float individual = 0.0f;

		bool canMoveThrough = true; // may be overriden by door type, propaged to linked doors

		Optional<bool> replacableByDoorTypeReplacer;

		Tags tags; // tags from door type - do not set, do not override, use variables (below) for that
		SimpleVariableStorage variables; // if we need to customise our door, not set from the door type (and this should not change! as we may change door type on the go)

		DoorType const * doorTypeForced; // this is original one, used as base for doorType
		DoorType const * doorTypeOriginal; // this is original one, used as base for doorType
		DoorType const * doorType; // currently used one, it might get updated basing on rooms on both sides
		DoorType const * secondaryDoorType; // as above but this is secondary door, it can be used as a seal or something similar, not affected by replacer
		bool blocksEnvironmentPropagation;

		::System::ShaderProgramInstance doorHoleShaderProgramInstance;

		bool importantVRDoor = false; // all linked door-in-rooms should have the same vr placement!
		bool worldSeparatorDoor = false; // works as a barrier and things won't leak to other door/corridor, propagates doorVRCorridor
		int worldSeparatorReason = 0; // custom flag set (!) by the game that tells any game related systems why this is a world separator
		::SafePtr<Room> separatesRoomFromWorld; // some of the world separator doors are to separate a specific room from the rest of the world (then we know in which direction we should grow)
		DoorVRCorridor doorVRCorridor;

		Optional<float> vrElevatorAltitude; // this is set by room generators, eg. elevators, etc. it has a higher priority than room's vr elevator altitude, door in room is more important though

		// linked doors (in room)
		DoorInRoom* linkedDoorA;
		DoorInRoom* linkedDoorB;

		bool visible = true; // if not visible, can't move through, is not rendered or considered for rendering, this is used solely for scripting to enable/disable certain things
		Optional<bool> visiblePending; // visible has immediate effect, that's why set_visible uses visible pending for actual change

		bool mute = false;
		SoundSources sounds;
		DoorSoundsPlayer soundsPlayer;
		DoorSoundsPlayer soundsPlayerForSecondaryDoorType;
		Array<DoorSoundsPlayer> wingSoundsPlayer;

		// how much is open (0 closed, 1 opened into A, -1 opened into B (if makes sense - for future))
		Concurrency::SpinLock operationLock = Concurrency::SpinLock(TXT("Framework.Door.operationLock"));
		float openTarget;
		float prevOpenTarget;
		float openFactor;
		Optional<DoorOperation::Type> initialOperation; // overrides whatever is in the type
		Optional<DoorOperation::Type> operation; // this is taken from door type but might be overriden
		bool operationLocked = false;
		Optional<DoorOperation::Type> operationIfUnlocked; // if we locked, we stored the value here, so when we unlock, we take this one, always set if operation is set
		float openTime;
		Optional<float> autoOperationDistance; // to override

		Vector2 holeWholeScale = Vector2::one; // almost like hole

		float holeTimeScale = 1.0f;
		float holeTimeScaleTarget = 1.0f;
		float holeTimeScaleTime = 0.2f;

		// scales both hole and mesh (visible mesh and collision)
		float doorTimeScale = 1.0f;
		float doorTimeScaleTarget = 1.0f;
		float doorTimeScaleTime = 0.2f;

		void on_connection();

		void on_new_door_type();
		
		friend struct DoorSoundsPlayer;
	};
};

DECLARE_REGISTERED_TYPE(Framework::Door*);
DECLARE_REGISTERED_TYPE(SafePtr<Framework::Door>);
