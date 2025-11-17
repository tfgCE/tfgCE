#pragma once

#include "module.h"
#include "..\..\core\math\math.h"

#include "..\ai\movementParameters.h"

#define MAX_CONTROLLER_ACTIONS 16

namespace Framework
{
	class ModuleController
	:	public Module
	{
		FAST_CAST_DECLARE(ModuleController);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();

		typedef Module base;
	public:
		static RegisteredModule<ModuleController> & register_itself();

		ModuleController(IModulesOwner* _owner);

		void clear_all();

		// if we don't need direct control
		void set_requested_movement_parameters(MovementParameters const & _requestedMovementParameters) { requestedMovementParameters = _requestedMovementParameters; }
		MovementParameters const & get_requested_movement_parameters() const { return requestedMovementParameters; }
		
		// actions are general actions that can be set from the outside and processed by anything inside (logic/movement etc)
		// changing should be used on one thread only at the same time
		void reset_actions() { actions.clear(); }
		void clear_action(Name const& _action) { actions.remove_fast(_action); }
		void set_action(Name const& _action) { actions.push_back_unique(_action); }
		bool is_action_set(Name const& _action) const { return actions.does_contain(_action); }

		Name const & get_final_requested_gait_name() const; // this will also include one provided by appearance module
		void reset_appearace_allowed_gait() { appearanceAllowedGait.clear(); }
		void set_appearace_allowed_gait(Optional<Name> const & _gait) { appearanceAllowedGait = _gait; }

		void clear_allow_gravity() { allowGravity.clear(); }
		void set_allow_gravity(bool _allowGravity) { allowGravity = _allowGravity; }
		Optional<bool> const & get_allow_gravity() const { return allowGravity; }

		void clear_distance_to_stop() { distanceToStop.clear(); } // along requested movement direction
		void set_distance_to_stop(float _distanceToStop) { distanceToStop = _distanceToStop; }
		Optional<float> const & get_distance_to_stop() const { return distanceToStop; }

		void clear_distance_to_slow_down() { distanceToSlowDown.clear(); slowDownPercentage.clear(); } // along requested movement direction
		void set_distance_to_slow_down(float _distanceToSlowDown, Optional<float> const & _percentage) { distanceToSlowDown = _distanceToSlowDown; slowDownPercentage = _percentage; }
		Optional<float> const & get_distance_to_slow_down() const { return distanceToSlowDown; }
		Optional<float> const & get_slow_down_percentage() const { return slowDownPercentage; }

		void clear_requested_precise_movement() { requestedPreciseMovement.clear(); }
		void set_requested_precise_movement(Vector3 const & _requestedPreciseMovement) { requestedPreciseMovement = _requestedPreciseMovement; }
		Optional<Vector3> const & get_requested_precise_movement() const { return requestedPreciseMovement; }

		void clear_requested_movement_direction() { requestedMovementDirection.clear(); requestedRelativeMovementDirection.clear(); }
		void set_requested_movement_direction(Vector3 const & _requestedMovementDirection) { requestedMovementDirection = _requestedMovementDirection.normal(); requestedRelativeMovementDirection.clear(); }
		void set_relative_requested_movement_direction(Vector3 const & _requestedMovementDirection) { requestedRelativeMovementDirection = _requestedMovementDirection.normal(); requestedMovementDirection.clear(); }
		Optional<Vector3> get_requested_movement_direction() const;
		Optional<Vector3> get_relative_requested_movement_direction() const;

		void clear_requested_velocity_linear() { requestedVelocityLinear.clear(); }
		void set_requested_velocity_linear(Vector3 const & _requestedVelocityLinear) { requestedVelocityLinear = _requestedVelocityLinear; }
		void set_relative_requested_velocity_linear(Vector3 const & _requestedVelocityLinear);
		Optional<Vector3> const & get_requested_velocity_linear() const { return requestedVelocityLinear; }

		void clear_requested_precise_rotation() { requestedPreciseRotation.clear(); }
		void set_requested_precise_rotation(Quat const & _requestedPreciseRotation) { requestedPreciseRotation = _requestedPreciseRotation; }
		Optional<Quat> const & get_requested_precise_rotation() const { return requestedPreciseRotation; }

		// if we want to turn to given direction
		void clear_requested_orientation() { requestedOrientation.clear(); }
		void set_requested_orientation(Quat const & _requestedOrientation) { requestedOrientation = _requestedOrientation; }
		Optional<Quat> const & get_requested_orientation() const { return requestedOrientation; }

		// if we want to rotate with given velocity (works only if no requested orientation set)
		void clear_requested_velocity_orientation() { requestedVelocityOrientation.clear(); }
		void set_requested_velocity_orientation(Rotator3 const & _requestedVelocityOrientation) { requestedVelocityOrientation = _requestedVelocityOrientation; }
		Optional<Rotator3> const & get_requested_velocity_orientation() const { return requestedVelocityOrientation; }

		// if we want to rotate with given (relative, normalised) velocity (works only if no requested orientation set)
		void clear_requested_relative_velocity_orientation() { requestedRelativeVelocityOrientation.clear(); }
		void set_requested_relative_velocity_orientation(Rotator3 const& _requestedRelativeVelocityOrientation) { requestedRelativeVelocityOrientation = _requestedRelativeVelocityOrientation; an_assert_immediate(requestedRelativeVelocityOrientation.get().is_ok());  }
		Optional<Rotator3> const & get_requested_relative_velocity_orientation() const { return requestedRelativeVelocityOrientation; }

		// this is just look
		void clear_snap_yaw_to_look_orientation() { snapYawToLookOrientation.clear(); }
		void set_snap_yaw_to_look_orientation(bool const & _snapYawToLookOrientation) { snapYawToLookOrientation = _snapYawToLookOrientation; }
		Optional<bool> const & get_snap_yaw_to_look_orientation() const { return snapYawToLookOrientation; }

		void clear_follow_yaw_to_look_orientation() { followYawToLookOrientation.clear(); }
		void set_follow_yaw_to_look_orientation(bool const & _followYawToLookOrientation) { followYawToLookOrientation = _followYawToLookOrientation; }
		Optional<bool> const & get_follow_yaw_to_look_orientation() const { return followYawToLookOrientation; }

		// we set look orientation or relative look (transform). only one can be set at a time...
		void clear_requested_look_orientation() { requestedLookOrientation.clear(); requestedRelativeLookOrientation.clear(); }
		void set_requested_look_orientation(Quat const& _requestedLookOrientation);
		void set_relative_requested_look_orientation(Quat const & _requestedLookOrientation);
		Quat get_relative_requested_look_orientation() const;
		bool is_requested_look_orientation_set() const { return requestedLookOrientation.is_set(); }

		void clear_requested_relative_look() { requestedRelativeLook.clear(); }
		void set_requested_relative_look(Transform const & _requestedRelativeLook) { an_assert(abs(_requestedRelativeLook.get_orientation().length() - 1.0f) < 0.1f) requestedRelativeLook = _requestedRelativeLook; clear_requested_look_orientation(); }

		// ...but we can access result as one
		Transform get_requested_relative_look() const;
		Quat get_requested_look_orientation() const;
		bool is_requested_look_set() const { return requestedRelativeLook.is_set() || requestedLookOrientation.is_set(); }

		void clear_look_orientation_additional_offset() { lookOrientationAdditionalOffset.clear(); }
		void set_look_orientation_additional_offset(Quat const & _lookOrientationAdditionalOffset) { lookOrientationAdditionalOffset = _lookOrientationAdditionalOffset; }
		Quat get_look_orientation_additional_offset() const { return lookOrientationAdditionalOffset.is_set() ? lookOrientationAdditionalOffset.get() : Quat::identity; }

		void clear_maintain_relative_requested_look_orientation() { maintainRelativeRequestedLookOrientation.clear(); }
		void set_maintain_relative_requested_look_orientation(bool const & _maintainRelativeRequestedLookOrientation) { maintainRelativeRequestedLookOrientation = _maintainRelativeRequestedLookOrientation; }
		Optional<bool> get_maintain_relative_requested_look_orientation() const { return maintainRelativeRequestedLookOrientation; }

		// we set look orientation or relative look (transform). only one can be set at a time...
		void clear_requested_hand(int _idx) { requestedRelativeHands[_idx].clear(); requestedRelativeHandsAccurate[_idx].clear(); }
		void set_requested_hand(int _idx, Transform const& _placement, bool _accurate) { requestedRelativeHands[_idx] = _placement; requestedRelativeHandsAccurate[_idx] = _accurate; }
		bool is_requested_hand_set(int _idx) const { return requestedRelativeHands[_idx].is_set(); }
		bool is_requested_hand_accurate(int _idx) const { return requestedRelativeHandsAccurate[_idx].get(false); }
		Transform get_requested_hand(int _idx) const { return requestedRelativeHands[_idx].is_set() ? requestedRelativeHands[_idx].get() : Transform::identity; }

		void clear_requested_placement() { requestedPlacement.clear(); }
		void set_requested_placement(Transform const & _placement) { requestedPlacement = _placement; }
		bool is_requested_placement_set() const { return requestedPlacement.is_set(); }
		Transform get_requested_placement() const { return requestedPlacement.get(); }

	protected: friend class ModulePresence;
		void internal_change_room(Transform const & _intoRoomTransform);

	public: // Module
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);

	private:
		ArrayStatic<Name, MAX_CONTROLLER_ACTIONS> actions;
		Optional<bool> allowGravity;
		Optional<float> distanceToStop;
		Optional<float> distanceToSlowDown; // to take corners etc
		Optional<float> slowDownPercentage; // 0.0f stop 1.0f full
		Optional<Vector3> requestedPreciseMovement; // overrides everything, is used to move character by very precise movement, resets velocity
		Optional<Vector3> requestedMovementDirection; // normalised - one of both is used but only one of them can be set
		Optional<Vector3> requestedRelativeMovementDirection; // normalised - see above
		MovementParameters requestedMovementParameters;
		Optional<Name> appearanceAllowedGait; // this works as feedback from appearance, allowing to modify current movement depending on state of the appearance
		Optional<Vector3> requestedVelocityLinear; // this might be not set if movement parameters are set
		Optional<Quat> requestedPreciseRotation;
		Optional<Rotator3> requestedVelocityOrientation;
		Optional<Rotator3> requestedRelativeVelocityOrientation; // normalised
		Optional<Quat> requestedOrientation;
		Optional<Quat> requestedLookOrientation; // requested (in room space) look orientation, used by AI
		Optional<Quat> requestedRelativeLookOrientation; // if w
		Optional<Transform> requestedRelativeLook; // if set, overrides requested look orientation, used by player/VR, for vr movement (vr mode: room, this is relative to 0,0,0)
		Optional<Quat> lookOrientationAdditionalOffset; // additional offset for orientation, mostly used to smooth orientation, not used for VR
		Optional<bool> maintainRelativeRequestedLookOrientation; // when rotating character relative requested look orientation will be maintained (as relative) or will keep looking in that dir
		Optional<bool> snapYawToLookOrientation; // this makes look orientation add up to orientation - snaps
		Optional<bool> followYawToLookOrientation; // orientation tries to follow look orientation
		Optional<Transform> requestedRelativeHands[2]; // for vr this is relative to 0,0,0, otherwise to final look (!)
		Optional<bool> requestedRelativeHandsAccurate[2]; // if this is true position, not predicted or guessed
		Optional<Transform> requestedPlacement; // some movement modules may handle this
	};
};

