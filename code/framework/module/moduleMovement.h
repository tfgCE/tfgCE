#pragma once

#include "module.h"
#include "moduleMovementData.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"

struct Rotator3;
struct Quat;

namespace Concurrency
{
	class JobExecutionContext;
};

// it has to be used in three places:
// moduleMovmeent.h DECLARE_FIND_GAITS_PROP_FUNCS
// moduleMovement.cpp DEFINE_FIND_GAITS_PROP_FUNCS_*
// moduleMovementData.cpp LOAD_GAITS_PROP

#define DECLARE_FIND_GAITS_PROP_FUNCS(class_name, function_name, current_gait_function_name, propName, typeName) \
	typeName function_name(Name const & _gaitName) const; \
	typeName current_gait_function_name() const;

#define DEFINE_FIND_GAITS_PROP_FUNCS(class_name, function_name, current_gait_function_name, propName, typeName) \
	typeName class_name::function_name(Name const & _gaitName) const \
	{ \
		return find_prop<typeName>(NAME(propName), _gaitName); \
	} \
	typeName class_name::current_gait_function_name() const \
	{ \
		return find_prop<typeName>(NAME(propName), currentGaitName); \
	}

#define DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(class_name, function_name, current_gait_function_name, propName, typeName, adjustProp) \
	typeName class_name::function_name(Name const & _gaitName) const \
	{ \
		return find_prop<typeName>(NAME(propName), _gaitName) * (adjustProp); \
	} \
	typeName class_name::current_gait_function_name() const \
	{ \
		return find_prop<typeName>(NAME(propName), currentGaitName) * (adjustProp); \
	}

namespace Framework
{
	struct MovementParameters;

	namespace AI
	{
		class Locomotion;
	};

	class ModuleMovement
	: public Module
	{
		FAST_CAST_DECLARE(ModuleMovement);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModuleMovement> & register_itself();

		static Rotator3 default_orientation_speed();
		static Rotator3 default_orientation_acceleration();

		ModuleMovement(IModulesOwner* _owner);
		virtual ~ModuleMovement();

		bool is_immovable() const { return immovable; }
		bool is_immovable_but_update_collisions() const { return immovable && immovableButUpdateCollisions; }
		
		virtual bool does_use_controller_for_movement() const { return true; }

		bool does_require_continuous_movement() const { return requiresContinuousMovement; }
		bool does_require_continuous_presence() const { return requiresContinuousPresence; }

		// advance - begin
		bool does_require_prepare_move() const { return requiresPrepareMove; }
		static void advance__prepare_move(IMMEDIATE_JOB_PARAMS);
		// advance - end

		Name const & get_name() const { return movementName; }

		Name const & get_current_gait_name() const { return currentGaitName; }

		ModuleMovementData::Gait const * find_gait(Name const & _gaitName) const;
		template <typename Class>
		Class const & find_prop(Name const & _prop, Name const & _gaitName) const;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		Array<Name> list_gaits() const;
#endif

		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_allow_movement_direction_difference_of_gait, find_allow_movement_direction_difference_of_current_gait, allowMovementDirectionDifference, float); // will disallow movement if direction difference is greater than this
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_limit_movement_direction_difference_of_gait, find_limit_movement_direction_difference_of_current_gait, limitMovementDirectionDifference, float); // if exceeds, will limit movement/velocity in that dir
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_speed_of_gait, find_speed_of_current_gait, speed, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_acceleration_of_gait, find_acceleration_of_current_gait, acceleration, float); // negative for instant
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_vertical_speed_of_gait, find_vertical_speed_of_current_gait, verticalSpeed, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_vertical_acceleration_of_gait, find_vertical_acceleration_of_current_gait, verticalAcceleration, float); // if 0 will use normal acceleration, negative will be instant
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_orientation_speed_of_gait, find_orientation_speed_of_current_gait, orientationSpeed, Rotator3);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_orientation_acceleration_of_gait, find_orientation_acceleration_of_current_gait, orientationAcceleration, Rotator3); // negative for instant
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_disallow_orientation_match_overshoot, find_disallow_orientation_match_overshoot_of_current_gait, disallowOrientationMatchOvershoot, Optional<bool>);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_use_gravity_from_trace_dot_limit_of_gait, find_use_gravity_from_trace_dot_limit_of_current_gait, useGravityFromTraceDotLimit, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_floor_level_offset_of_gait, find_floor_level_offset_of_current_gait, floorLevelOffset, float);
		DECLARE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_does_allow_using_gravity_from_outbound_traces_of_gait, find_does_allow_using_gravity_from_outbound_traces_of_current_gait, allowsUsingGravityFromOutboundTraces, bool);

		float get_use_gravity() const { return useGravity; }
		float get_use_gravity_for_orientation() const { return useGravityForOrientation; }
		float get_match_gravity_orientation_speed() const { return matchGravityOrientationSpeed; }

		bool does_use_3d_rotation() const { return use3dRotation; }
		bool is_rotation_allowed() const { return !noRotation;  }

		float get_align_with_gravity_weight(Name const & _type) const;
		void store_align_with_gravity_displacement(Name const & _type, Rotator3 const & _displacement);

	public:
		virtual void on_activate_movement(ModuleMovement* _prevMovement);
		virtual void on_deactivate_movement(ModuleMovement* _nextMovement);

	protected:
		struct PrepareMoveContext
		{
			bool disallowNormalTurns = false;
		};
		void prepare_move(float _deltaTime);
		// do not virtualise above method, add post/pre

		virtual void apply_gravity_to_velocity(float _deltaTime, REF_ Vector3 & _accelerationInFrame, REF_ Vector3 & _locationDisplacement);
		virtual void apply_gravity_to_rotation(float _deltaTime, Rotator3 const & _currentVelocity, REF_ Quat & _orientationDisplacement);

	public: // used by presence module
		// _velocity won't have effect in current movement - it is to affect any future movements, to affect current movement, modify _currentDisplacement
		// this way it is possible to keep velocity but limit movement or opposite, to affect velocity in any matter but do different movement than would be result of velocity
		// this is to finalise changes on velocity and current movement (current frame displacement)
		struct VelocityAndDisplacementContext;
		virtual void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context);

	public: // Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);

	protected:
		bool requiresPrepareMove = true; // change in constructor
		
		ModuleMovementData const * moduleMovementData;
		
		Name movementName;
		
		Name currentGaitName; // stored as not always requested gait is same as current

		// will be pushed onto stack
		Name collisionMovementName; // "movement" + movementName, useful for adding to collision
		Optional<Collision::Flags> collisionFlags;
		Optional<Collision::Flags> collidesWithFlags;

		bool immovable = false; // will not update velocity, move, post move etc. will still do "attachment movement" (if attached). this is useful to disable movement in particular cases
		bool immovableButUpdateCollisions = false; // allows to update collisions
		bool requiresContinuousMovement = false; // if set to true, rare advance move is disabled, this may help with larger gradient udpates that will hog CPU also if precise movement is required (scripts!)
		bool requiresContinuousPresence = false; // if set to true, presence is build using velocity and delta time to predict future movement, this is useful for detecting future collisions

		float requestedOrientationDeadZone = 1.0f; // won't rotate if requested orientation is lower than this

		float collisionStiffness = 0.9f; // stiffness of collision - 0.1 soft - 1.0 stiff - 2.0 bouncy - how velocity is affected
		float collisionRigidity = 0.7f; // rigidity of collision - how object enters other object - with lower rigidity than 1 it is good to have greater radius for gradient, extending 1.0 does not make any sense
		float collisionPushOutTime = 0.3f; // time required to be pushed out of collision (smaller time, faster it will be pushed out of collision)
		float collisionVerticalPushOutTime = -1.0f; // only for verticals - if negative, takes collision push out time value
		float collisionDropVelocity = 0.1f; // if velocity is below this against collision normal/gradient, it is dropped
		float collisionFriction = 0.0f; // will slow down this percent per second
		float collisionFrictionOrientation = 0.0f; // will slow down this percent per second
		float collisionMaintainSpeed = 0.0f; // when deflected, how much speed is maintained (velocity vector changes but object may keep speed)
		float maintainVelocity = 1.0f; // how much velocity should be maintained over time
		float matchGravityOrientationSpeed = 90.0f; // in degs
		float useGravity = 0.0f; // how much gravity is applied (by default off, temporary objects, particles mainly, don't even want to hear about using gravity at all, although they may end up with gravity vector when they're detached)
		float useGravityForTranslation = 1.0f; // for location/translation
		float useGravityForOrientation = 1.0f; // for orientation
		float useGravityFromTraceDotLimit = 0.6f; // will use gravity provided by trace hit when dot of <current up> and <hit normal> is above limit
		bool allowMatchingRequestedVelocityAtAnyTime = false; // if true, will always match requested velocity, not only if on ground
		bool pretendFallUsingGravityPresenceTraces = false;
		bool use3dRotation = false;
		bool limitRequestedMovementToXYPlaneOS = false;
		bool noRotation = false;

		float gaitSpeedAdjustment = 1.0f; // how speed from gaits is adjusted

		void apply_velocity_linear_difference(float _deltaTime, REF_ Vector3 & _accelerationInFrame, Vector3 const & _requestedDifference) const;
		void apply_velocity_orientation_difference(float _deltaTime, REF_ Rotator3 & _accelerationInFrame, Rotator3 const & _requestedDifference) const;
		void adjust_velocity_orientation_for(float _deltaTime, REF_ Rotator3 & _accelerationInFrame, Rotator3 const & _currentRotationVelocity, Rotator3 const & _requestedOrientationLocal) const;
		void adjust_velocity_orientation_yaw_for(float _deltaTime, REF_ Rotator3 & _accelerationInFrame, Rotator3 const & _currentRotationVelocity, Rotator3 const & _requestedOrientationLocal) const;

		virtual Vector3 calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const;
		void calculate_requested_linear_speed(MovementParameters const & _movementParameters, OUT_ float * _linearSpeed, OUT_ float * _verticalSpeed = nullptr) const; // this is just based solely on movement parameters
		Rotator3 calculate_requested_orientation_speed(MovementParameters const & _movementParameters) const; // this is just based solely on movement parameters

	public:
		struct VelocityAndDisplacementContext
		{
			Vector3 velocityLinear;
			Vector3 currentDisplacementLinear;
			Rotator3 velocityRotation;
			Quat currentDisplacementRotation;
			bool applyCollision = true;
			VelocityAndDisplacementContext(Vector3 const & _velocityLinear, Vector3 const & _currentDisplacementLinear, Rotator3 const & _velocityRotation, Quat const & _currentDisplacementRotation)
			: velocityLinear(_velocityLinear)
			, currentDisplacementLinear(_currentDisplacementLinear)
			, velocityRotation(_velocityRotation)
			, currentDisplacementRotation(_currentDisplacementRotation)
			{
				an_assert(currentDisplacementRotation.is_normalised(0.1f));
			}
			VelocityAndDisplacementContext& apply_collision(bool _applyCollision) { applyCollision = _applyCollision; return *this; }
		};

	protected:
		struct AlignWithGravity
		{
			Name type; // for reference
			Rotator3 orientationDisplacement = Rotator3::zero;
			CACHED_ int priority;
			CACHED_ float weight;
			CACHED_ bool requiresPresenceGravityTraces;
		};
		ArrayStatic<AlignWithGravity, 8> alignWithGravity; // shouldn't be more sources than that

		Quat solve_align_with_gravity_displacement() const; // this is called from apply_changes_to_velocity_and_displacement and uses alignWithGravity, if you want to ignore this, clear alignWithGravity before calling this (base) method

	protected:
		Array<SimpleVariableInfo> disallowMovementLinearVars;

		friend class AI::Locomotion;
	};

};

