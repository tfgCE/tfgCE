#pragma once

#include "..\appearance\socketID.h"
#include "..\interfaces\presenceObserver.h"
#include "..\module\moduleMovement.h"

namespace Framework
{
	class SoundSource;

	struct ScriptedMovement
	{
		struct Rotate
		{
			Optional<float> target;
			float speed = 0.0f;
			float acceleration = 0.0f; // immediate

			bool load_from_xml(IO::XML::Node const* _node);
		};

		bool repeat = false; // will repeat

		Optional<Name> toPOI;
		Optional<Vector3> offsetLocationOS;
		Optional<Rotator3> offsetRotation;
		Optional<Vector3> toLocationWS;
		Optional<float> toLocationZWS;
		Optional<float> randomOffsetLocationWS;

		Optional<Vector3> allowedMovementWS;

		Optional<Vector3> velocityLinearOS; // overrides target based (uses linearAcceleration)
		Optional<Rotator3> velocityRotationOS; // overrides anything else
		Optional<float> velocityRotationOSBlendTime; // overrides anything else

		// for linear movement
		Optional<float> linearSpeed;
		Optional<float> linearAcceleration;
		Optional<float> linearDeceleration;

		// if not time based
		Optional<float> rotationSpeed;
		// "rotate"s override rotation speed (any is enough to override)
		Rotate rotatePitch;
		Rotate rotateYaw;
		Rotate rotateRoll;
		
		// for time based / smoothed movement
		Optional<Range> time;
		Optional<float> smoothStart;
		Optional<float> smoothEnd;

		Name playSound;
		
		Name triggerGameScriptExecutionTrapOnDecelerate;
		Name triggerGameScriptExecutionTrapOnDone;

		bool load_from_xml(IO::XML::Node const* _node, bool _mayBeInvalid = false);

		bool is_valid() const { return is_speed_based() || is_time_based(); }
		bool is_speed_based() const { return linearSpeed.is_set() || linearAcceleration.is_set() || linearDeceleration.is_set() || velocityLinearOS.is_set() || rotatePitch.target.is_set() || rotateYaw.target.is_set() || rotateRoll.target.is_set(); }
		bool is_time_based() const { return time.is_set(); }
	};

	/*
	 *	this movement is to be controlled via scripts
	 *	provides very basic movements
	 */
	class ModuleMovementScripted
	: public Framework::ModuleMovement
	, public Framework::IPresenceObserver
	{
		FAST_CAST_DECLARE(ModuleMovementScripted);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_BASE(Framework::IPresenceObserver);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementScripted(Framework::IModulesOwner* _owner);
		virtual ~ModuleMovementScripted();
	
		bool is_at_destination() const;

		void stop(bool _immediately = false);
		void do_movement(ScriptedMovement const& _movement);

	protected:	// Module
		override_ void activate();
		override_ void on_owner_destroy();

	protected:	// ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context);
		override_ bool does_use_controller_for_movement() const { return false; }

	public: // IPresenceObserver
		implement_ void on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors);
		implement_ void on_presence_added_to_room(Framework::ModulePresence* _presence, Framework::Room* _room, Framework::DoorInRoom* _enteredThroughDoor);
		implement_ void on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room);
		implement_ void on_presence_destroy(Framework::ModulePresence* _presence);

	public:
		struct CurrentMovement
		: public ScriptedMovement
		{
			Optional<Vector3> useRandomOffsetLocationWS;

			float active = 0.0f;
			float activeTarget = 1.0f;
			float blendInTime = 0.0f;
			float blendOutTime = 0.0f;

			bool requiresSetup = true;
			Transform startingPlacementWS;
			Transform targetPlacementWS;
			BezierCurve<float> smoothCurve;

			float timeIn = 0.0f;
			Optional<float> useTime; // selectedAtRandom

			bool decelerating = false;
			bool atDestination = false;

			RefCountObjectPtr<Framework::SoundSource> playedSound;

			void reset_movement()
			{
				timeIn = 0.0f;
				useTime.clear();
				useRandomOffsetLocationWS.clear();
			}
		};

	private:
		Concurrency::SpinLock movementLock;
		Array<CurrentMovement> movements;

	private:
		Framework::ModulePresence* observingPresence = nullptr;

		void set_observe_presence(bool _observe);
	};
};

