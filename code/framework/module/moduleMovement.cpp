#include "moduleMovement.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\debug\debugRenderer.h"

#include "moduleMovementImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(immovable);
DEFINE_STATIC_NAME(immovableButUpdateCollisions);
DEFINE_STATIC_NAME(collisionFlags);
DEFINE_STATIC_NAME(collidesWithFlags);
DEFINE_STATIC_NAME(collisionStiffness);
DEFINE_STATIC_NAME(collisionRigidity);
DEFINE_STATIC_NAME(collisionPushOutTime);
DEFINE_STATIC_NAME(collisionVerticalPushOutTime);
DEFINE_STATIC_NAME(collisionDropVelocity);
DEFINE_STATIC_NAME(collisionFriction);
DEFINE_STATIC_NAME(collisionFrictionOrientation);
DEFINE_STATIC_NAME(collisionMaintainSpeed);
DEFINE_STATIC_NAME(requestedOrientationDeadZone);
DEFINE_STATIC_NAME(maintainVelocity);
DEFINE_STATIC_NAME(matchGravityOrientationSpeed);
DEFINE_STATIC_NAME(useGravity);
DEFINE_STATIC_NAME(useGravityForTranslation);
DEFINE_STATIC_NAME(useGravityForOrientation);
DEFINE_STATIC_NAME(useGravityFromTraceDotLimit);
DEFINE_STATIC_NAME(limitRequestedMovementToXYPlaneOS);
DEFINE_STATIC_NAME(use3dRotation);
DEFINE_STATIC_NAME(noRotation);
DEFINE_STATIC_NAME(gaitSpeedAdjustment);
DEFINE_STATIC_NAME(allowMovementDirectionDifference);
DEFINE_STATIC_NAME(limitMovementDirectionDifference);
DEFINE_STATIC_NAME(speed);
DEFINE_STATIC_NAME(acceleration);
DEFINE_STATIC_NAME(verticalSpeed);
DEFINE_STATIC_NAME(verticalAcceleration);
DEFINE_STATIC_NAME(orientationSpeed);
DEFINE_STATIC_NAME(orientationAcceleration);
DEFINE_STATIC_NAME(disallowOrientationMatchOvershoot);
DEFINE_STATIC_NAME(floorLevelOffset);
DEFINE_STATIC_NAME(allowsUsingGravityFromOutboundTraces);
DEFINE_STATIC_NAME(pretendFallUsingGravityPresenceTraces);
DEFINE_STATIC_NAME(allowMatchingRequestedVelocityAtAnyTime);

//

REGISTER_FOR_FAST_CAST(ModuleMovement);

Rotator3 ModuleMovement::default_orientation_speed() { return Rotator3(180.0f, 180.0f, 360.0f); }
Rotator3 ModuleMovement::default_orientation_acceleration() { return Rotator3(500.0f, 500.0f, 1000.0f); }

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovement(_owner);
}

RegisteredModule<ModuleMovement> & ModuleMovement::register_itself()
{
	return Modules::movement.register_module(String(TXT("base")), create_module);
}

ModuleMovement::ModuleMovement(IModulesOwner* _owner)
: base(_owner)
{
	SET_EXTRA_DEBUG_INFO(alignWithGravity, TXT("ModuleMovement.alignWithGravity"));
}

ModuleMovement::~ModuleMovement()
{
}

void ModuleMovement::reset()
{
	base::reset();
}

void ModuleMovement::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	moduleMovementData = fast_cast<ModuleMovementData>(_moduleData);
	if (_moduleData)
	{
		movementName = moduleMovementData->movementName;
		{
			String cf = _moduleData->get_parameter<String>(this, NAME(collisionFlags));
			if (!cf.is_empty())
			{
				collisionFlags = Collision::Flags::none();
				collisionFlags.access().apply(cf);
			}
		}
		{
			String cf = _moduleData->get_parameter<String>(this, NAME(collidesWithFlags));
			if (!cf.is_empty())
			{
				collidesWithFlags = Collision::Flags::none();
				collidesWithFlags.access().apply(cf);
			}
		}
		if (collisionFlags.is_set() || collidesWithFlags.is_set())
		{
			collisionMovementName = Name(String(TXT("movement")) + movementName.to_string());
		}

		immovable = _moduleData->get_parameter<bool>(this, NAME(immovable), immovable);
		immovableButUpdateCollisions = _moduleData->get_parameter<bool>(this, NAME(immovableButUpdateCollisions), immovableButUpdateCollisions);
		immovable |= immovableButUpdateCollisions;
		if (immovable)
		{
			useGravity = 0.0f;
		}
		requestedOrientationDeadZone = _moduleData->get_parameter<float>(this, NAME(requestedOrientationDeadZone), requestedOrientationDeadZone);
		collisionStiffness = _moduleData->get_parameter<float>(this, NAME(collisionStiffness), collisionStiffness);
		collisionRigidity = clamp(_moduleData->get_parameter<float>(this, NAME(collisionRigidity), collisionRigidity), 0.0f, 1.0f);
		collisionPushOutTime = _moduleData->get_parameter<float>(this, NAME(collisionPushOutTime), collisionPushOutTime);
		collisionVerticalPushOutTime = _moduleData->get_parameter<float>(this, NAME(collisionVerticalPushOutTime), collisionVerticalPushOutTime);
		collisionDropVelocity = _moduleData->get_parameter<float>(this, NAME(collisionDropVelocity), collisionDropVelocity);
		collisionFriction = _moduleData->get_parameter<float>(this, NAME(collisionFriction), collisionFriction);
		collisionFrictionOrientation = _moduleData->get_parameter<float>(this, NAME(collisionFrictionOrientation), collisionFrictionOrientation);
		collisionMaintainSpeed = _moduleData->get_parameter<float>(this, NAME(collisionMaintainSpeed), collisionMaintainSpeed);
		maintainVelocity = _moduleData->get_parameter<float>(this, NAME(maintainVelocity), maintainVelocity);
		matchGravityOrientationSpeed = _moduleData->get_parameter<float>(this, NAME(matchGravityOrientationSpeed), matchGravityOrientationSpeed);
		useGravity = _moduleData->get_parameter<float>(this, NAME(useGravity), useGravity);
		useGravityForTranslation = _moduleData->get_parameter<float>(this, NAME(useGravityForTranslation), useGravityForTranslation);
		useGravityForOrientation = _moduleData->get_parameter<float>(this, NAME(useGravityForOrientation), useGravityForOrientation);
		useGravityFromTraceDotLimit = _moduleData->get_parameter<float>(this, NAME(useGravityFromTraceDotLimit), useGravityFromTraceDotLimit);
		allowMatchingRequestedVelocityAtAnyTime = _moduleData->get_parameter<bool>(this, NAME(allowMatchingRequestedVelocityAtAnyTime), allowMatchingRequestedVelocityAtAnyTime);
		pretendFallUsingGravityPresenceTraces = _moduleData->get_parameter<bool>(this, NAME(pretendFallUsingGravityPresenceTraces), pretendFallUsingGravityPresenceTraces);
		use3dRotation = _moduleData->get_parameter<bool>(this, NAME(use3dRotation), use3dRotation);
		noRotation = _moduleData->get_parameter<bool>(this, NAME(noRotation), noRotation);
		limitRequestedMovementToXYPlaneOS = _moduleData->get_parameter<bool>(this, NAME(limitRequestedMovementToXYPlaneOS), limitRequestedMovementToXYPlaneOS);
		gaitSpeedAdjustment = _moduleData->get_parameter<float>(this, NAME(gaitSpeedAdjustment), gaitSpeedAdjustment);
		/*
		height = _moduleData->get_parameter<float>(String("height"), height);
		heightCentrePt = _moduleData->get_parameter<float>(String("heightcentrept"), heightCentrePt);
		heightCentrePt = _moduleData->get_parameter<float>(String("heightcenterpt"), heightCentrePt);
		allowSlide = _moduleData->get_parameter<bool>(String("allowslide"), allowSlide);
		*/

		disallowMovementLinearVars.clear();
		for_every(v, moduleMovementData->disallowMovementLinearVars)
		{
			auto found = get_owner()->access_variables().find<float>(*v);
			disallowMovementLinearVars.push_back(found);
		}
	}
}

void ModuleMovement::advance__prepare_move(IMMEDIATE_JOB_PARAMS)
{
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(advanceVelocity);
		if (ModuleMovement* self = modulesOwner->get_movement())
		{
			if (modulesOwner->get_presence()->get_in_room())
			{
				if (ModuleController const* controller = modulesOwner->get_controller())
				{
					todo_note(TXT("add trying to match requested parameters to make it possible to switch to gait, it might be modified here"));
					self->currentGaitName = controller->get_final_requested_gait_name();
				}
				self->prepare_move(ac->get_delta_time());
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModuleMovement::apply_velocity_linear_difference(float _deltaTime, REF_ Vector3 & _accelerationInFrame, Vector3 const & _requestedDifference) const
{
	Vector3 requestedDifference = _requestedDifference;

	//requestedDifference = requestedDifference.drop_using_normalised(get_owner()->get_presence()->get_placement().vector_to_world(Vector3::zAxis));

	if (! requestedDifference.is_zero())
	{
		float const accelerationOfGait = find_acceleration_of_current_gait();
		float const verticalAccelerationOfGait = find_vertical_acceleration_of_current_gait();
		if (verticalAccelerationOfGait != 0.0f)
		{
			Transform upPlacement = get_owner()->get_presence()->calculate_environment_up_placement();
			an_assert_immediate(upPlacement.is_ok());
			Vector3 reqDifUP = upPlacement.vector_to_local(_requestedDifference);

			float const reqDifUPLength = reqDifUP.length_2d();
			float const reqVerDifUP = reqDifUP.z;

			float const reqDifUPPossibleLength = clamp(reqDifUPLength, -accelerationOfGait * _deltaTime, accelerationOfGait * _deltaTime);
			float const reqVerDifUPPossible = verticalAccelerationOfGait > 0.0f? clamp(reqVerDifUP, -verticalAccelerationOfGait * _deltaTime, verticalAccelerationOfGait * _deltaTime) : reqDifUPPossibleLength;

			Vector3 const reqDifInFrame = (reqDifUPLength != 0.0f? (reqDifUP * Vector3(1.0f, 1.0f, 0.0f) * reqDifUPPossibleLength / reqDifUPLength) : Vector3::zero)
										+ Vector3(0.0f, 0.0f, reqVerDifUPPossible);
			an_assert_immediate(reqDifInFrame.is_ok());
			Vector3 const requestedDifferenceInFrame = upPlacement.vector_to_world(reqDifInFrame);
			_accelerationInFrame += requestedDifferenceInFrame;
			an_assert_immediate(_accelerationInFrame.is_ok());
		}
		else
		{
			float const requestedDifferenceLength = requestedDifference.length();
			if (requestedDifferenceLength != 0.0f)
			{
				if (accelerationOfGait < 0.0f)
				{
					_accelerationInFrame += requestedDifference;
					an_assert_immediate(_accelerationInFrame.is_ok());
				}
				else
				{
					float const requestedDifferencePossibleLength = clamp(requestedDifferenceLength, -accelerationOfGait * _deltaTime, accelerationOfGait * _deltaTime);
					Vector3 const requestedDifferenceInFrame = requestedDifference * requestedDifferencePossibleLength / requestedDifferenceLength;
					_accelerationInFrame += requestedDifferenceInFrame;
					an_assert_immediate(_accelerationInFrame.is_ok());
				}
			}
		}
	}
}

void ModuleMovement::apply_velocity_orientation_difference(float _deltaTime, REF_ Rotator3 & _accelerationInFrame, Rotator3 const & _requestedDifference) const
{
	if (!is_rotation_allowed())
	{
		return;
	}

	if (!_requestedDifference.is_zero()) // first quick check, it may still give 0 length
	{
		Rotator3 const orientationAccelerationOfGait = find_orientation_acceleration_of_current_gait();
		Rotator3 requestedDifferenceInFrame = Rotator3::zero;
		for_count(int, componentIdx, 3)
		{
			RotatorComponent::Type component = RotatorComponent::Type(componentIdx);
			if (!does_use_3d_rotation() && component != RotatorComponent::Yaw) continue;

			float const requestedDifferenceLength = _requestedDifference.length();
			if (requestedDifferenceLength != 0.0f)
			{
				if (orientationAccelerationOfGait.get_component(component) < 0.0f)
				{
					float const requestedDifferenceInFrame = _requestedDifference.get_component(component);
					_accelerationInFrame.access_component(component) += requestedDifferenceInFrame;
				}
				else
				{
					float const requestedDifferencePossibleLength = clamp(requestedDifferenceLength, -orientationAccelerationOfGait.get_component(component) * _deltaTime, orientationAccelerationOfGait.get_component(component) * _deltaTime);
					float const requestedDifferenceInFrame = _requestedDifference.get_component(component) * requestedDifferencePossibleLength / requestedDifferenceLength;
					_accelerationInFrame.access_component(component) += requestedDifferenceInFrame;
				}
			}
		}
	}
}

void ModuleMovement::adjust_velocity_orientation_for(float _deltaTime, REF_ Rotator3 & _accelerationInFrame, Rotator3 const & _currentRotationVelocity, Rotator3 const & _requestedOrientationLocal) const
{
	if (!is_rotation_allowed() || _deltaTime == 0.0f)
	{
		return;
	}

	Rotator3 orientationSpeedOfGait = find_orientation_speed_of_current_gait();
	Rotator3 orientationAccelerationOfGait = find_orientation_acceleration_of_current_gait();
	Optional<bool> disallowOrientationMatchOvershootOptional = find_disallow_orientation_match_overshoot_of_current_gait();
	Rotator3 invOrientationAccelerationOfGait;
	invOrientationAccelerationOfGait.pitch = orientationAccelerationOfGait.pitch != 0.0f ? 1.0f / orientationAccelerationOfGait.pitch : 0.0f;
	invOrientationAccelerationOfGait.yaw = orientationAccelerationOfGait.yaw != 0.0f ? 1.0f / orientationAccelerationOfGait.yaw : 0.0f;
	invOrientationAccelerationOfGait.roll = orientationAccelerationOfGait.roll != 0.0f ? 1.0f / orientationAccelerationOfGait.roll : 0.0f;
	/*
	s = vt + at^2 / 2
	0 = v - at
	t = v / a

	s = v'/a + a * v' / a' / 2
	s = v'/a + v' / a / 2
	s = v'/(2*a)
	s = v' * (0.5 * (1/a)) to slow down to 0
	*/
	Rotator3 const currentVelocity = _currentRotationVelocity;

	for_count(int, componentIdx, 3)
	{
		RotatorComponent::Type component = RotatorComponent::Type(componentIdx);
		if (!does_use_3d_rotation() && component != RotatorComponent::Yaw && currentVelocity.get_component(component) == 0.0f) continue;

		float provideAcceleration = 0.0f;

		if (orientationAccelerationOfGait.get_component(component) < 0.0f)
		{
			float gaitSpeed = orientationSpeedOfGait.get_component(component);
			float requires = _requestedOrientationLocal.get_component(component);
			float nextVelocity = requires / _deltaTime;
			if (gaitSpeed > 0.0f)
			{
				nextVelocity = clamp(nextVelocity, -gaitSpeed, gaitSpeed);
			}
			provideAcceleration = (nextVelocity - currentVelocity.get_component(component)) / _deltaTime;
		}
		else
		{
			float willCoverToSlowDown = sqr(currentVelocity.get_component(component)) * (0.5f * invOrientationAccelerationOfGait.get_component(component));

			float willCoverToSlowDownAddedFrame = abs(currentVelocity.get_component(component)) * _deltaTime // add one extra frame
												+ willCoverToSlowDown;

			float const extraFrames = 2.0f;
			float const extraTime = max(0.05f, _deltaTime * extraFrames);
			float willCoverToSlowDownExtra      = abs(currentVelocity.get_component(component)) * extraTime
												+ willCoverToSlowDown;

			float requires = _requestedOrientationLocal.get_component(component);

			bool disallowOrientationMatchOvershoot = disallowOrientationMatchOvershootOptional.get(orientationAccelerationOfGait.get_component(component) > 1000.0f);
			if (disallowOrientationMatchOvershoot &&
				willCoverToSlowDown > abs(requires))
			{
				float nextVelocity = requires / _deltaTime;
				// v1 = v0 + at
				// at = v1 - v0
				// a = (v1 - v0)/t
				provideAcceleration = (nextVelocity - currentVelocity.get_component(component)) / _deltaTime;
			}
			else if (disallowOrientationMatchOvershoot &&
				willCoverToSlowDownAddedFrame > abs(requires))
			{
				float nextVelocity = currentVelocity.get_component(component) * 0.5f;
				// v1 = v0 + at
				// at = v1 - v0
				// a = (v1 - v0)/t
				provideAcceleration = (nextVelocity - currentVelocity.get_component(component)) / _deltaTime;
			}
			else if (willCoverToSlowDownExtra > abs(requires) && sign(currentVelocity.get_component(component)) == sign(requires))
			{
				// 0 = v - at
				// t = v/a
				float t = max(_deltaTime, abs(currentVelocity.get_component(component)) / orientationAccelerationOfGait.get_component(component));
				// s = vt + at^2 / 2
				// at^2 / 2 = s - vt
				// at^2 = 2(s - vt)
				// a = 2(s - vt) / t^2
				if (t > 0.0f)
				{
					float acceleration = 2.0f * (abs(requires) - t * abs(currentVelocity.get_component(component))) / sqr(t);
					acceleration = -acceleration; // because above we assume it is a positive
					provideAcceleration = -sign(currentVelocity.get_component(component)) * acceleration;
				}
				else
				{
					provideAcceleration = -sign(currentVelocity.get_component(component)) * orientationAccelerationOfGait.get_component(component);
				}
				float nextVelocity = currentVelocity.get_component(component) + provideAcceleration * _deltaTime;
				if (nextVelocity * currentVelocity.get_component(component) <= 0.0f)
				{
					// do not overshoot
					provideAcceleration = -currentVelocity.get_component(component) / _deltaTime;
				}
			}
			else if (abs(requires) > requestedOrientationDeadZone)
			{
				todo_note(TXT("if very close, use smaller acceleration"));
				provideAcceleration = sign(requires) * orientationAccelerationOfGait.get_component(component);
				float nextVelocity = currentVelocity.get_component(component) + provideAcceleration * _deltaTime;
				float willCoverNextToStop = nextVelocity * _deltaTime
										  + (-sign(nextVelocity)) * sqr(nextVelocity) * (0.5f * invOrientationAccelerationOfGait.get_component(component));
				if ((currentVelocity.get_component(component) >= orientationSpeedOfGait.get_component(component) && provideAcceleration > 0.0f) ||
					(currentVelocity.get_component(component) <= -orientationSpeedOfGait.get_component(component) && provideAcceleration < 0.0f))
				{
					// stop
					provideAcceleration = 0.0f;
				}
				else if (nextVelocity > orientationSpeedOfGait.get_component(component))
				{
					// do not exceed
					provideAcceleration = (orientationSpeedOfGait.get_component(component) - currentVelocity.get_component(component)) / _deltaTime;
				}
				else if (nextVelocity < -orientationSpeedOfGait.get_component(component))
				{
					// do not exceed
					provideAcceleration = (-orientationSpeedOfGait.get_component(component) - currentVelocity.get_component(component)) / _deltaTime;
				}
				if (willCoverNextToStop >= requires && requires > 0.0f && provideAcceleration > 0.0f)
				{
					// will go too fast, do not accelerate, we will be slowing down in the next one
					provideAcceleration = 0.0f;
				}
				if (willCoverNextToStop <= requires && requires < 0.0f && provideAcceleration < 0.0f)
				{
					// will go too fast, do not accelerate, we will be slowing down in the next one
					provideAcceleration = 0.0f;
				}
			}
			else
			{
				provideAcceleration = -currentVelocity.get_component(component) / _deltaTime;
				provideAcceleration = clamp(provideAcceleration , -orientationAccelerationOfGait.get_component(component), orientationAccelerationOfGait.get_component(component));
			}
		}

		_accelerationInFrame.access_component(component) += provideAcceleration * _deltaTime;
	}
}

void ModuleMovement::adjust_velocity_orientation_yaw_for(float _deltaTime, REF_ Rotator3 & _accelerationInFrame, Rotator3 const & _currentRotationVelocity, Rotator3 const & _requestedOrientationLocal) const
{
	adjust_velocity_orientation_for(_deltaTime, REF_ _accelerationInFrame, _currentRotationVelocity, Rotator3(0.0f, _requestedOrientationLocal.yaw, 0.0f));
}

void ModuleMovement::calculate_requested_linear_speed(MovementParameters const & _movementParameters, OUT_ float* _linearSpeed, OUT_ float* _verticalSpeed) const
{
	float speed = 0.0f;
	float verticalSpeed = 0.0f;
	if (_movementParameters.absoluteSpeed.is_set())
	{
		speed = _movementParameters.absoluteSpeed.get();
		verticalSpeed = speed; // is it ok?
	}
	else
	{
		speed = _movementParameters.gaitName.is_valid() ? find_speed_of_gait(_movementParameters.gaitName) : find_speed_of_current_gait();
		verticalSpeed = _movementParameters.gaitName.is_valid() ? find_vertical_speed_of_gait(_movementParameters.gaitName) : find_vertical_speed_of_current_gait();
		if (_movementParameters.relativeSpeed.is_set())
		{
			speed *= _movementParameters.relativeSpeed.get();
			verticalSpeed *= _movementParameters.relativeSpeed.get();
		}
	}
	assign_optional_out_param(_linearSpeed, speed);
	assign_optional_out_param(_verticalSpeed, verticalSpeed);
}

Rotator3 ModuleMovement::calculate_requested_orientation_speed(MovementParameters const & _movementParameters) const
{
	Rotator3 speed = _movementParameters.gaitName.is_valid() ? find_orientation_speed_of_gait(_movementParameters.gaitName) : find_orientation_speed_of_current_gait();
	{
		float const defSpeed = 1000.0f;
		speed.pitch = replace_negative_zero_positive(speed.pitch, defSpeed, 0.0f, speed.pitch);
		speed.yaw = replace_negative_zero_positive(speed.yaw, defSpeed, 0.0f, speed.yaw);
		speed.roll = replace_negative_zero_positive(speed.roll, defSpeed, 0.0f, speed.roll);
	}
	return speed;
}

Vector3 ModuleMovement::calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext & _context) const
{
	IModulesOwner * modulesOwner = get_owner();

	ModulePresence * const presence = modulesOwner->get_presence();
	ModuleController const * const controller = modulesOwner->get_controller();
	if (presence && controller)
	{
		Vector3 requestedVelocity = Vector3::zero;
		Optional<Vector3> const & requestedMovementDirection = controller->get_requested_movement_direction();
		Optional<Rotator3> const & requestedVelocityOrientation = controller->get_requested_velocity_orientation();
		{
			if (requestedMovementDirection.is_set())
			{
				MovementParameters movementParameters = controller->get_requested_movement_parameters();
				movementParameters.gaitName = currentGaitName;
				float speed;
				float verticalSpeed;
				calculate_requested_linear_speed(movementParameters, OUT_ &speed, OUT_ &verticalSpeed);
				if (verticalSpeed != 0.0f)
				{
					Transform upPlacement = presence->calculate_environment_up_placement();
					Vector3 dirUP = upPlacement.vector_to_local(requestedMovementDirection.get());
					Vector3 velUP = dirUP * Vector3(speed, speed, verticalSpeed);
					requestedVelocity = upPlacement.vector_to_world(velUP);
				}
				else
				{
					requestedVelocity = requestedMovementDirection.get() * speed;
				}
			}
		}

		{
			Optional<Vector3> const & requestedVelocityLinear = controller->get_requested_velocity_linear();
			if (requestedVelocityLinear.is_set())
			{
				requestedVelocity = requestedVelocityLinear.get();
			}
		}

		{	// stop if not facing right direction
			if (requestedMovementDirection.is_set() || requestedVelocityOrientation.is_set())
			{
				float directionDifference = find_allow_movement_direction_difference_of_current_gait();
				if (directionDifference > 0.0f)
				{
					float movementAlignment = 1.0f;
					if (requestedMovementDirection.is_set())
					{
						movementAlignment = min(movementAlignment, Vector3::dot(requestedMovementDirection.get(), presence->get_placement().get_axis(Axis::Forward)));
					}
					if (requestedVelocityOrientation.is_set())
					{
						movementAlignment = min(movementAlignment, cos_deg(requestedVelocityOrientation.get().yaw * /*time pred*/1.0f));
					}
					float directionDifferentTre = cos_deg(directionDifference);
					if (movementAlignment < directionDifferentTre)
					{
						// disallow movement
						requestedVelocity = Vector3::zero;
						_context.disallowNormalTurns = presence->get_velocity_linear().length() > 0.1f;
					}
				}
				float limitDirectionDifference = find_limit_movement_direction_difference_of_current_gait();
				if (limitDirectionDifference >= 0.0f)
				{
					float movementAlignment = 1.0f;
					if (requestedMovementDirection.is_set())
					{
						movementAlignment = min(movementAlignment, Vector3::dot(requestedMovementDirection.get(), presence->get_placement().get_axis(Axis::Forward)));
					}
					if (requestedVelocityOrientation.is_set())
					{
						movementAlignment = min(movementAlignment, cos_deg(requestedVelocityOrientation.get().yaw * /*time pred*/1.0f));
					}
					float limitDirectionDifferenceTre = cos_deg(limitDirectionDifference);
					if (movementAlignment < limitDirectionDifferenceTre)
					{
						// limit direction difference, allow movement just can't rotate so quickly
						Vector3 requestedVelocityLocal = presence->get_placement().vector_to_local(requestedVelocity);
						Vector3 reqDirLocal = requestedVelocityLocal.normal();
						reqDirLocal = reqDirLocal.keep_within_angle_normalised(Vector3::yAxis, limitDirectionDifference);
						requestedVelocityLocal = reqDirLocal.normal() * requestedVelocityLocal.length();
						requestedVelocity = presence->get_placement().vector_to_world(requestedVelocityLocal);
					}
				}
			}
		}

		if (_adjustForStoppingEtc)
		{
			if (!requestedVelocity.is_zero())
			{
				Optional<float> const & distanceToStop = controller->get_distance_to_stop();
				Optional<float> const & distanceToSlowDown = controller->get_distance_to_slow_down();
				if (distanceToStop.is_set() || distanceToSlowDown.is_set())
				{
					Vector3 requestedDirection = requestedVelocity.normal();
					float const accelerationOfGait = find_acceleration_of_current_gait();
					if (accelerationOfGait > 0.0f)
					{
						float const invAccelerationOfGait = accelerationOfGait != 0.0f ? 1.0f / accelerationOfGait : 0.0f;
						float speed = Vector3::dot(requestedDirection, presence->get_velocity_linear());
						float const normalSpeed = find_speed_of_current_gait();
						float const slowSpeed = normalSpeed * controller->get_slow_down_percentage().get(0.25f);
						if ((distanceToStop.is_set() && speed >= 0.0f) ||
							(distanceToSlowDown.is_set() && speed >= slowSpeed))
						{
							// s = v * t - a * t^2 / 2
							// v1 = v0 - a * t
							// v0 - v1 = a * t
							// t = (v0 - v1) / a

							float timeRequiredToStop = sqr(speed) * invAccelerationOfGait;
							float timeRequiredToSlowDown = sqr(speed - slowSpeed) * invAccelerationOfGait;
							float distRequiredToStop = speed  * timeRequiredToStop - accelerationOfGait * sqr(timeRequiredToStop) / 2.0f;
							float distRequiredToSlowDown = speed  * timeRequiredToSlowDown - accelerationOfGait * sqr(timeRequiredToSlowDown) / 2.0f;

							Optional<float> requiresToStop = distanceToStop;
							Optional<float> requiresToSlowDown = distanceToSlowDown;

							if ((requiresToStop.is_set() && distRequiredToStop > requiresToStop.get()) ||
								(requiresToSlowDown.is_set() && distRequiredToSlowDown > requiresToSlowDown.get()))
							{
								// slow down
								requestedVelocity = requestedVelocity.normal() * clamp(speed - accelerationOfGait * _deltaTime, 0.0f, requestedVelocity.length());
							}
							else
							{
								// use partially requested speed (but at longer distances prefer requested)
								// if close to using stop (requires * %%), keep current speed
								float useRequestedSpeed = 1.0f;
								if (requiresToStop.is_set() && requiresToStop.get() > 0.0f)
								{
									useRequestedSpeed = min(useRequestedSpeed, clamp(sqr(1.5f * (requiresToStop.get() - distRequiredToStop) / requiresToStop.get()), 0.0f, 1.0f));
								}
								if (requiresToSlowDown.is_set() && requiresToSlowDown.get() > 0.0f)
								{
									useRequestedSpeed = min(useRequestedSpeed, clamp(sqr(1.5f * (requiresToSlowDown.get() - distRequiredToSlowDown) / requiresToSlowDown.get()), 0.0f, 1.0f));
								}
								requestedVelocity = requestedVelocity.normal() * clamp(speed * (1.0f - useRequestedSpeed) + useRequestedSpeed * requestedVelocity.length(), 0.0f, requestedVelocity.length());
							}
						}
					}
				}
			}
		}

		if (limitRequestedMovementToXYPlaneOS && !requestedVelocity.is_zero())
		{
			// keep length/speed
			requestedVelocity = requestedVelocity.drop_using_normalised(presence->get_placement().get_axis(Axis::Z)).normal() * requestedVelocity.length();
		}

		return requestedVelocity;
	}

	return Vector3::zero;
}

void ModuleMovement::prepare_move(float _deltaTime)
{
	IModulesOwner * modulesOwner = get_owner();

	alignWithGravity.clear();

	ModulePresence * const presence = modulesOwner->get_presence();
	ModuleController const * const controller = modulesOwner->get_controller();
	if (presence &&
		! presence->is_attached())
	{
		PrepareMoveContext prepareMoveContext;
		Vector3 const currentVelocity = presence->get_next_velocity_linear();

		// velocity
		{
			Vector3 accelerationInFrame = Vector3::zero;
			Vector3 locationDisplacement = Vector3::zero;

			// note: collision is applied in ModulePresence's prepare_move

			// we don't want various things to fight each other
			bool accelerationModified = false;
			
			if (!accelerationModified && controller)
			{
				Optional<Vector3> const & requestedPreciseMovement = controller->get_requested_precise_movement();
				if (requestedPreciseMovement.is_set())
				{
					presence->set_next_velocity_linear(Vector3::zero);
					presence->set_next_location_displacement(requestedPreciseMovement.get());
					accelerationModified = true;
				}
			}

			bool allowMatchingRquestedVelocityNow = presence->do_gravity_presence_traces_touch_surroundings() || presence->get_gravity().is_zero() || useGravity == 0.0f;
			if (allowMatchingRquestedVelocityNow || allowMatchingRequestedVelocityAtAnyTime)
			{
				// use requested velocity
				if (!accelerationModified)
				{
					Vector3 requestedVelocity = calculate_requested_velocity(_deltaTime, true, prepareMoveContext);
					{
						bool allowMovementLinear = true;
						for_every(v, disallowMovementLinearVars)
						{
							if (v->get<float>() > 0.5f)
							{
								allowMovementLinear = false;
							}
						}
						if (!allowMovementLinear)
						{
							requestedVelocity = Vector3::zero;
						}
					}
					if (allowMatchingRequestedVelocityAtAnyTime)
					{
						Vector3 gravity = useGravity == 0.0f? Vector3::zero : presence->get_gravity();
						if (! gravity.is_zero())
						{
							Vector3 gravityDir = gravity.normal();
							requestedVelocity = requestedVelocity.drop_using_normalised(gravityDir)
											  + currentVelocity.along_normalised(gravityDir);
						}
					}
					// calculate it first as we may want to setup other variables depending on what we require
					Vector3 const requestedDifference = requestedVelocity - currentVelocity;
					apply_velocity_linear_difference(_deltaTime, REF_ accelerationInFrame, requestedDifference);
					an_assert_immediate(accelerationInFrame.is_ok());
					accelerationModified = true;
				}
			}
			else
			{
				todo_note(TXT("slow down?"));
			}

			if (maintainVelocity != 1.0f)
			{
				accelerationInFrame += presence->get_next_velocity_linear() * _deltaTime * (maintainVelocity - 1.0f);
				an_assert_immediate(accelerationInFrame.is_ok());
			}

			if (controller &&
				(!controller->get_allow_gravity().is_set() || controller->get_allow_gravity().get()))
			{
				apply_gravity_to_velocity(_deltaTime, REF_ accelerationInFrame, REF_ locationDisplacement);
				an_assert_immediate(accelerationInFrame.is_ok());
			}

			presence->add_next_velocity_linear(accelerationInFrame);
			presence->add_next_location_displacement(locationDisplacement);
		}

		// orientation displacement due to control
		if (controller &&
			controller->get_snap_yaw_to_look_orientation().is_set() &&
			controller->get_snap_yaw_to_look_orientation().get())
		{
			Quat orientationDisplacement = presence->get_next_orientation_displacement();
			Rotator3 headRelativeOrientation = controller->get_requested_relative_look().get_orientation().to_rotator();
			orientationDisplacement = orientationDisplacement.to_world(Rotator3(0.0f, headRelativeOrientation.yaw, 0.0f).to_quat());
			presence->set_next_orientation_displacement(orientationDisplacement);
		}

		// orientation velocity
		{
			Rotator3 const currentVelocity = presence->get_next_velocity_rotation() * 0.99f;
			Rotator3 accelerationInFrame = Rotator3::zero;
			Quat orientationDisplacement = Quat::identity;

			// we don't want various things to fight each other
			bool accelerationModified = false;

			if (!accelerationModified && pretendFallUsingGravityPresenceTraces && !presence->do_gravity_presence_traces_touch_surroundings())
			{
				Vector3 lastTouchOS = presence->get_last_gravity_presence_traces_touch_OS();
				if (lastTouchOS.x != 0.0f || lastTouchOS.y != 0.0f)
				{
					Rotator3 fallingRotation = Rotator3::zero;

					Vector3 lastTouchOSFlatDir = (lastTouchOS * Vector3(1.0f, 1.0f, 0.0f)).normal();
					fallingRotation.pitch = 180.0f * lastTouchOSFlatDir.y;
					fallingRotation.roll = -180.0f * lastTouchOSFlatDir.x;

					accelerationInFrame = (fallingRotation - currentVelocity) * _deltaTime / 1.0f;
					an_assert_immediate(accelerationInFrame.is_ok());
					apply_velocity_orientation_difference(_deltaTime, REF_ accelerationInFrame, -currentVelocity);
					an_assert_immediate(accelerationInFrame.is_ok());
					accelerationModified = true;
				}
			}

			if (!accelerationModified && controller)
			{
				Optional<Quat> const & requestedPreciseRotation = controller->get_requested_precise_rotation();
				if (requestedPreciseRotation.is_set())
				{
					presence->set_next_velocity_rotation(Rotator3::zero);
					presence->set_next_orientation_displacement(requestedPreciseRotation.get());
					accelerationModified = true;
				}
			}

			// setup acceleration based on requested orientation
			if (!prepareMoveContext.disallowNormalTurns && 
				! accelerationModified && controller)
			{
				Optional<Quat> const & requestedOrientation = controller->get_requested_orientation();
				if (requestedOrientation.is_set())
				{	
					Quat const currentOrientation = presence->get_placement().get_orientation();
					todo_future(TXT("requested orientation should ..?!"));
					Rotator3 requestedLocalOrientation = currentOrientation.to_local(requestedOrientation.get()).to_rotator();
					if (does_use_3d_rotation())
					{
						adjust_velocity_orientation_for(_deltaTime, REF_ accelerationInFrame, presence->get_velocity_rotation(), requestedLocalOrientation);
						an_assert_immediate(accelerationInFrame.is_ok());
					}
					else
					{
						requestedLocalOrientation.pitch = 0;
						requestedLocalOrientation.roll = 0;
						adjust_velocity_orientation_yaw_for(_deltaTime, REF_ accelerationInFrame, presence->get_velocity_rotation(), requestedLocalOrientation);
						an_assert_immediate(accelerationInFrame.is_ok());
					}
					accelerationModified = true;
				}
			}

			// match requested velocity
			if (!prepareMoveContext.disallowNormalTurns &&
				!accelerationModified && controller)
			{
				Optional<Rotator3> const & requestedVelocityOrientation = controller->get_requested_velocity_orientation();
				if (requestedVelocityOrientation.is_set())
				{
					Rotator3 const requestedDifference = requestedVelocityOrientation.get() - currentVelocity;
					apply_velocity_orientation_difference(_deltaTime, REF_ accelerationInFrame, requestedDifference);
					an_assert_immediate(accelerationInFrame.is_ok());
					accelerationModified = true;
				}
			}

			// match requested relative velocity
			if (!prepareMoveContext.disallowNormalTurns &&
				!accelerationModified && controller)
			{
				Optional<Rotator3> const & requestedRelativeVelocityOrientation = controller->get_requested_relative_velocity_orientation();
				if (requestedRelativeVelocityOrientation.is_set())
				{
					an_assert_immediate(requestedRelativeVelocityOrientation.get().is_ok());
					Rotator3 orientationSpeedOfCurrentGait = find_orientation_speed_of_current_gait();
					{
						float const defSpeed = 1000.0f;
						orientationSpeedOfCurrentGait.pitch = replace_negative_zero_positive(orientationSpeedOfCurrentGait.pitch, defSpeed, 0.0f, orientationSpeedOfCurrentGait.pitch);
						orientationSpeedOfCurrentGait.yaw = replace_negative_zero_positive(orientationSpeedOfCurrentGait.yaw, defSpeed, 0.0f, orientationSpeedOfCurrentGait.yaw);
						orientationSpeedOfCurrentGait.roll = replace_negative_zero_positive(orientationSpeedOfCurrentGait.roll, defSpeed, 0.0f, orientationSpeedOfCurrentGait.roll);
					}
					Rotator3 const requestedVelocityOrientation = requestedRelativeVelocityOrientation.get() * orientationSpeedOfCurrentGait;
					Rotator3 const requestedDifference = requestedVelocityOrientation - currentVelocity;
					apply_velocity_orientation_difference(_deltaTime, REF_ accelerationInFrame, requestedDifference);
					an_assert_immediate(accelerationInFrame.is_ok());
					accelerationModified = true;
				}
			}

			if (!prepareMoveContext.disallowNormalTurns &&
				!accelerationModified &&
				controller && controller->get_follow_yaw_to_look_orientation().is_set())
			{
				if (controller->get_follow_yaw_to_look_orientation().get() &&
					controller->is_requested_look_set())
				{
					Quat const currentOrientation = presence->get_placement().get_orientation();
					Quat requestedLocalOrientation = currentOrientation.to_local(controller->get_requested_look_orientation());
					// check if we're going upside down
					float offXYPlane = requestedLocalOrientation.get_y_axis().z;
					requestedLocalOrientation = requestedLocalOrientation.rotate_by(Rotator3(-offXYPlane * 70.0f, 0.0f, 0.0f).to_quat()); // should be enough to bring us to proper front
					Rotator3 requestedLocalRotation = requestedLocalOrientation.to_rotator();
					requestedLocalRotation.pitch = 0;
					requestedLocalRotation.roll = 0;
					adjust_velocity_orientation_yaw_for(_deltaTime, REF_ accelerationInFrame, presence->get_velocity_rotation(), requestedLocalRotation);
					an_assert_immediate(accelerationInFrame.is_ok());
					accelerationModified = true;
				}
				else
				{
					// stop any existing rotation
					adjust_velocity_orientation_yaw_for(_deltaTime, REF_ accelerationInFrame, presence->get_velocity_rotation(), Rotator3::zero);
					an_assert_immediate(accelerationInFrame.is_ok());
					accelerationModified = true;
				}
			}

			// no instructions what to do? stop then!
			if (!accelerationModified)
			{
				apply_velocity_orientation_difference(_deltaTime, REF_ accelerationInFrame, -currentVelocity);
				an_assert_immediate(accelerationInFrame.is_ok());
				accelerationModified = true;
			}

			if (controller &&
				(!controller->get_allow_gravity().is_set() || controller->get_allow_gravity().get()))
			{
				apply_gravity_to_rotation(_deltaTime, currentVelocity, REF_ orientationDisplacement);
			}

			an_assert_immediate(accelerationInFrame.is_ok());
			an_assert_immediate(orientationDisplacement.is_ok());
			presence->add_next_velocity_rotation(accelerationInFrame);
			presence->add_next_orientation_displacement(orientationDisplacement);
		}
	}
}

void ModuleMovement::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	if (!does_use_controller_for_movement())
	{
		return;
	}

	IModulesOwner * modulesOwner = get_owner();

	// get from collision
	if (_context.applyCollision)
	{
		if (ModuleCollision const * collision = modulesOwner->get_collision())
		{
			Vector3 accelerationFromGradient = collision->get_gradient();
			Vector3 accelerationFromGradientStiff = collision->get_stiff_gradient();
			if (accelerationFromGradient.length() < 0.002f &&
				accelerationFromGradientStiff.length() < 0.002f &&
				_context.velocityLinear.length() < 0.2f &&
				useGravity > 0.0f && useGravityForTranslation > 0.0f &&
				Vector3::dot(accelerationFromGradient.normal(), _context.velocityLinear.normal()) < -0.995f &&
				Vector3::dot(accelerationFromGradient.normal(), modulesOwner->get_presence()->get_gravity_dir()) < -0.995f)
			{
				// it's wobbling on the ground. freeze it right there
				_context.velocityLinear = Vector3::zero;
				_context.currentDisplacementLinear = Vector3::zero;
			}
			else
			{
				if (!accelerationFromGradientStiff.is_zero())
				{
					Vector3 gradientDiff = accelerationFromGradientStiff - accelerationFromGradient;
					float normalPctAmount = clamp((gradientDiff.length() / accelerationFromGradientStiff.length()) * 2.0f, 0.0f, 0.95f);
					accelerationFromGradientStiff *= 1.0f - normalPctAmount; // if there is no other gradient, use full stiff, otherwise allow normal gradient processing to be done 
					accelerationFromGradient -= accelerationFromGradientStiff;
				}
				if (!accelerationFromGradient.is_zero())
				{
					debug_filter(moduleMovementDebug);
					debug_subject(get_owner());
					debug_context(get_owner()->get_presence()->get_in_room());
					debug_draw_arrow(true, Colour::red.with_alpha(0.5f), get_owner()->get_presence()->get_placement().get_translation(), get_owner()->get_presence()->get_placement().get_translation() + accelerationFromGradient);
					Vector3 collisionDisplacement;
					if (collisionVerticalPushOutTime < 0.0f || collisionVerticalPushOutTime == collisionPushOutTime)
					{
						collisionDisplacement = accelerationFromGradient * collisionRigidity * clamp(collisionPushOutTime != 0.0f ? _deltaTime / collisionPushOutTime : 1.0f, 0.0f, 1.0f);
					}
					else
					{
						Vector3 verticalAxis = modulesOwner->get_presence()->get_placement().get_axis(Axis::Z);
						Vector3 collisionDisplacementXY = accelerationFromGradient * collisionRigidity * clamp(collisionPushOutTime != 0.0f ? _deltaTime / collisionPushOutTime : 1.0f, 0.0f, 1.0f);
						Vector3 collisionDisplacementZ = accelerationFromGradient * collisionRigidity * clamp(collisionVerticalPushOutTime != 0.0f ? _deltaTime / collisionVerticalPushOutTime : 1.0f, 0.0f, 1.0f);
						collisionDisplacement = collisionDisplacementXY.drop_using_normalised(verticalAxis) + collisionDisplacementZ.along_normalised(verticalAxis);
					}
					debug_draw_arrow(true, Colour::red, get_owner()->get_presence()->get_placement().get_translation(), get_owner()->get_presence()->get_placement().get_translation() + collisionDisplacement);
					Vector3 collisionVelocity = _context.velocityLinear; // do not use collision displacement here
					if (!collisionDisplacement.is_zero())
					{
						// maybe we should stop or even go back with the collision
						Vector3 collisionDir = collisionDisplacement.normal();
						// add perpendicular to collision dir and along the collision dir, get maximum
						Vector3 displacementImmediate = _context.currentDisplacementLinear.drop_using_normalised(collisionDir)
							+ collisionDisplacement.drop_using_normalised(collisionDir)
							+ collisionDir * max(Vector3::dot(_context.currentDisplacementLinear, collisionDir), Vector3::dot(collisionDisplacement, collisionDir));
						float useImmediate = clamp(collisionPushOutTime != 0.0f ? _deltaTime / collisionPushOutTime : 1.0f, 0.0f, 1.0f);
						_context.currentDisplacementLinear = (_context.currentDisplacementLinear + collisionDisplacement) * (1.0f - useImmediate) + useImmediate * displacementImmediate;
					}
					Vector3 accelerationFromGradientNormalised = accelerationFromGradient.normal();
					// modify velocity
					{
						float againstGradientNormalised = Vector3::dot(collisionVelocity, accelerationFromGradientNormalised);
						Vector3 newVelocityLinear = _context.velocityLinear;
						if (abs(againstGradientNormalised) < collisionDropVelocity)
						{
							// zero it
							newVelocityLinear -= min(0.0f, Vector3::dot(collisionVelocity, accelerationFromGradientNormalised)) * accelerationFromGradientNormalised;
						}
						else
						{
							debug_draw_arrow(true, Colour::blue.with_alpha(0.4f), get_owner()->get_presence()->get_centre_of_presence_WS(), get_owner()->get_presence()->get_centre_of_presence_WS() + accelerationFromGradientNormalised);
							debug_draw_arrow(true, Colour::green.with_alpha(0.4f), get_owner()->get_presence()->get_centre_of_presence_WS(), get_owner()->get_presence()->get_centre_of_presence_WS() + collisionVelocity);
							newVelocityLinear -= min(0.0f, Vector3::dot(collisionVelocity, accelerationFromGradientNormalised)) * accelerationFromGradientNormalised * collisionStiffness * clamp(collisionPushOutTime != 0.0f ? _deltaTime / collisionPushOutTime : 1.0f, 0.0f, 1.0f);
							debug_draw_arrow(true, Colour::orange.with_alpha(0.4f), get_owner()->get_presence()->get_centre_of_presence_WS(), get_owner()->get_presence()->get_centre_of_presence_WS() + _context.velocityLinear);
						}
						if (collisionMaintainSpeed != 0.0f && !newVelocityLinear.is_almost_zero())
						{
							newVelocityLinear = newVelocityLinear.normal() * (newVelocityLinear.length() * (1.0f - collisionMaintainSpeed) + _context.velocityLinear.length() * collisionMaintainSpeed);
						}
						_context.velocityLinear = newVelocityLinear;
					}
					if (collisionFriction > 0.0f)
					{
						Vector3 droppedVelocityLinear = _context.velocityLinear.drop_using_normalised(accelerationFromGradientNormalised);
						Vector3 perpendicularRest = _context.velocityLinear - droppedVelocityLinear;
						Vector3 newDroppedVelocityLinear = droppedVelocityLinear * clamp(1.0f - collisionFriction * _deltaTime, 0.0f, 1.0f);
						if (newDroppedVelocityLinear.length() < collisionDropVelocity)
						{
							newDroppedVelocityLinear = Vector3::zero;
						}
						_context.velocityLinear = newDroppedVelocityLinear + perpendicularRest;
					}
					if (collisionFrictionOrientation > 0.0f)
					{
						_context.velocityRotation = _context.velocityRotation * clamp(1.0f - collisionFrictionOrientation * _deltaTime, 0.0f, 1.0f);
					}
					debug_no_filter();
					debug_no_subject();
					debug_no_context();
				}
				if (!accelerationFromGradientStiff.is_zero())
				{
					Vector3 collisionDisplacement;
					collisionDisplacement = accelerationFromGradientStiff * collisionRigidity;
					Vector3 collisionVelocity = _context.velocityLinear; // do not use collision displacement here
					if (!collisionDisplacement.is_zero())
					{
						// maybe we should stop or even go back with the collision
						Vector3 collisionDir = collisionDisplacement.normal();
						// add perpendicular to collision dir and along the collision dir, get maximum
						Vector3 displacementImmediate = _context.currentDisplacementLinear.drop_using_normalised(collisionDir)
							+ collisionDisplacement.drop_using_normalised(collisionDir)
							+ collisionDir * max(Vector3::dot(_context.currentDisplacementLinear, collisionDir), Vector3::dot(collisionDisplacement, collisionDir));
						float useImmediate = clamp(collisionPushOutTime != 0.0f ? _deltaTime / collisionPushOutTime : 1.0f, 0.0f, 1.0f);
						_context.currentDisplacementLinear = (_context.currentDisplacementLinear + collisionDisplacement) * (1.0f - useImmediate) + useImmediate * displacementImmediate;
					}
					Vector3 accelerationFromGradientNormalised = accelerationFromGradientStiff.normal();
					// modify velocity
					{
						float againstGradientNormalised = Vector3::dot(collisionVelocity, accelerationFromGradientNormalised);
						Vector3 newVelocityLinear = _context.velocityLinear;
						if (abs(againstGradientNormalised) < collisionDropVelocity)
						{
							// zero it
							newVelocityLinear -= min(0.0f, Vector3::dot(collisionVelocity, accelerationFromGradientNormalised)) * accelerationFromGradientNormalised;
						}
						else
						{
							newVelocityLinear -= min(0.0f, Vector3::dot(collisionVelocity, accelerationFromGradientNormalised)) * accelerationFromGradientNormalised * collisionStiffness * clamp(collisionPushOutTime != 0.0f ? _deltaTime / collisionPushOutTime : 1.0f, 0.0f, 1.0f);
						}
						if (collisionMaintainSpeed != 0.0f && !newVelocityLinear.is_almost_zero())
						{
							newVelocityLinear = newVelocityLinear.normal() * (newVelocityLinear.length() * (1.0f - collisionMaintainSpeed) + _context.velocityLinear.length() * collisionMaintainSpeed);
						}
						_context.velocityLinear = newVelocityLinear;
					}
				}
			}
		}
	}

	if (!alignWithGravity.is_empty())
	{
		_context.currentDisplacementRotation = _context.currentDisplacementRotation.rotate_by(solve_align_with_gravity_displacement());
		an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
	}
}

void ModuleMovement::apply_gravity_to_velocity(float _deltaTime, REF_ Vector3 & _accelerationInFrame, REF_ Vector3 & _locationDisplacement)
{
	if (useGravity == 0.0f || useGravityForTranslation == 0.0f)
	{
		return;
	}

	IModulesOwner * modulesOwner = get_owner();

	ModulePresence * const presence = modulesOwner->get_presence();
	if (presence)
	{
		if (ModuleCollision const * collision = modulesOwner->get_collision())
		{
			// drop onto floor
			todo_note(TXT("switch to falling and go back to normal only when hit floor"));
			Vector3 accelerationFromGradient = collision->get_gradient();
			float const useGravityNow = (useGravity * useGravityForTranslation);
			float gravityAgainstCollision = clamp(Vector3::dot(accelerationFromGradient, presence->get_gravity_dir()), -presence->get_gravity().length() * useGravityNow, 0.0f);
			_accelerationInFrame += (presence->get_gravity() * useGravityNow + presence->get_gravity_dir() * gravityAgainstCollision) * _deltaTime;
		}
	}
}

void ModuleMovement::apply_gravity_to_rotation(float _deltaTime, Rotator3 const & _currentVelocity, REF_ Quat & _orientationDisplacement)
{
	if (useGravity == 0.0f || useGravityForOrientation == 0.0f)
	{
		return;
	}

	IModulesOwner * modulesOwner = get_owner();

	ModulePresence * const presence = modulesOwner->get_presence();
	if (presence)
	{
		// align with gravity
		// we use gravity from hit locations as it will give us blend when gravity changes
		// we will be able to use room's gravity or gravity calculated from hit locations - this is handled in presence
		Vector3 gravity = presence->get_gravity_dir_from_hit_locations();
		if (!gravity.is_zero())
		{
			Vector3 gravityLocal = presence->get_placement().vector_to_local(gravity).normal();
			Vector3 forwardFromGravity = Vector3::yAxis;
			if (abs(gravityLocal.y) > 0.99f)
			{
				// we're looking right at gravity or away from it - simplify dirs!
				float dirMultiplier = gravityLocal.y > 0.0f ? 1.0f : -1.0f;
				forwardFromGravity = Vector3::zAxis * dirMultiplier;
				gravityLocal = Vector3::yAxis * dirMultiplier;
			}
			else
			{
				// simplify vectors to get pitch from forwardFromGravity and roll from gravityLocal
				forwardFromGravity = Vector3::cross(gravityLocal, Vector3::cross(forwardFromGravity, gravityLocal));
				forwardFromGravity = Vector3(0.0f, forwardFromGravity.y, forwardFromGravity.z).normal();
				gravityLocal = Vector3(gravityLocal.x, 0.0f, gravityLocal.z).normal();
			}

			Rotator3 requiredToMatchGravity = Rotator3::zero;
			// only pitch and roll! keep yaw as it is
			requiredToMatchGravity.pitch -= asin_deg(clamp(-forwardFromGravity.z, -1.0f, 1.0f));
			requiredToMatchGravity.roll -= asin_deg(clamp(gravityLocal.x, -1.0f, 1.0f));

			todo_note(TXT("setup match gravity time"));
			float const requiredToMatchGravityLength = requiredToMatchGravity.length();
			if (requiredToMatchGravityLength > 0.01f)
			{
				float const maxRotationSpeed = matchGravityOrientationSpeed;
				float const howMuchCanBeApplied = clamp(requiredToMatchGravityLength, -maxRotationSpeed * _deltaTime, maxRotationSpeed * _deltaTime);
				Rotator3 rotateThisFrame = requiredToMatchGravity * clamp(howMuchCanBeApplied / requiredToMatchGravityLength, 0.0f, 1.0f);
				float closeCoef = clamp((requiredToMatchGravityLength - 2.0f) / 10.0f, 0.05f, 1.0f);
				store_align_with_gravity_displacement(Name::invalid(), (rotateThisFrame * (useGravity * useGravityForOrientation) * closeCoef));
			}

			todo_note(TXT("add displacement for centre of weight"));
		}
	}
}

ModuleMovementData::Gait const * ModuleMovement::find_gait(Name const & _gaitName) const
{
	if (moduleMovementData)
	{
		return moduleMovementData->find_gait(_gaitName);
	}
	else
	{
		return nullptr;
	}
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
Array<Name> ModuleMovement::list_gaits() const
{
	if (moduleMovementData)
	{
		return moduleMovementData->list_gaits();
	}
	else
	{
		return Array<Name>();
	}
}
#endif

DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_allow_movement_direction_difference_of_gait, find_allow_movement_direction_difference_of_current_gait, allowMovementDirectionDifference, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_limit_movement_direction_difference_of_gait, find_limit_movement_direction_difference_of_current_gait, limitMovementDirectionDifference, float);
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovement, find_speed_of_gait, find_speed_of_current_gait, speed, float, gaitSpeedAdjustment);
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovement, find_acceleration_of_gait, find_acceleration_of_current_gait, acceleration, float, gaitSpeedAdjustment);
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovement, find_vertical_speed_of_gait, find_vertical_speed_of_current_gait, verticalSpeed, float, gaitSpeedAdjustment);
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovement, find_vertical_acceleration_of_gait, find_vertical_acceleration_of_current_gait, verticalAcceleration, float, gaitSpeedAdjustment);
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovement, find_orientation_speed_of_gait, find_orientation_speed_of_current_gait, orientationSpeed, Rotator3, gaitSpeedAdjustment);
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovement, find_orientation_acceleration_of_gait, find_orientation_acceleration_of_current_gait, orientationAcceleration, Rotator3, gaitSpeedAdjustment);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_disallow_orientation_match_overshoot, find_disallow_orientation_match_overshoot_of_current_gait, disallowOrientationMatchOvershoot, Optional<bool>);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_use_gravity_from_trace_dot_limit_of_gait, find_use_gravity_from_trace_dot_limit_of_current_gait, useGravityFromTraceDotLimit, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_floor_level_offset_of_gait, find_floor_level_offset_of_current_gait, floorLevelOffset, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovement, find_does_allow_using_gravity_from_outbound_traces_of_gait, find_does_allow_using_gravity_from_outbound_traces_of_current_gait, allowsUsingGravityFromOutboundTraces, bool);

float ModuleMovement::get_align_with_gravity_weight(Name const & _type) const
{
	if (auto const * awgData = moduleMovementData->find_align_with_gravity(_type))
	{
		return awgData->weight;
	}
	else
	{
		return 0.0f; // not present, would be ignored any way
	}
}

void ModuleMovement::store_align_with_gravity_displacement(Name const & _type, Rotator3 const & _displacement)
{
	AlignWithGravity awg;
	awg.type = _type;
	awg.orientationDisplacement = _displacement;
	if (moduleMovementData)
	{
		if (auto const * awgData = moduleMovementData->find_align_with_gravity(_type))
		{
			awg.priority = awgData->priority;
			awg.weight = awgData->weight;
			awg.requiresPresenceGravityTraces = awgData->requiresPresenceGravityTraces;
		}
		else
		{
			// do not store! we do not know how to handle it
			return;
		}
	}
	for_every(eawg, alignWithGravity)
	{
		if (eawg->type == awg.type)
		{
			*eawg = awg;
			return;
		}
	}
	alignWithGravity.push_back(awg);
}

Quat ModuleMovement::solve_align_with_gravity_displacement() const
{
	if (alignWithGravity.is_empty())
	{
		return Quat::identity;
	}

	MEASURE_PERFORMANCE(solve_align_with_gravity_displacement);

	Rotator3 result = Rotator3::zero;

	int const lowestPriority = -(INF_INT - 1);
		
	bool hasPresenceGravityTraces = get_owner()->get_presence()->do_gravity_presence_traces_touch_surroundings();

	int maxPriority = INF_INT;
	float weightLeft = 1.0f;
	while (weightLeft)
	{
		int highestPriority = lowestPriority;
		for_every(awg, alignWithGravity)
		{
			if (awg->priority <= maxPriority &&
				(hasPresenceGravityTraces || ! awg->requiresPresenceGravityTraces))
			{
				highestPriority = max(awg->priority, highestPriority);
			}
		}
		if (highestPriority == lowestPriority)
		{
			break;
		}

		// tp - this priority
		float tpSumWeight = 0.0f;
		Rotator3 tpDisplacement = Rotator3::zero;
		for_every(awg, alignWithGravity)
		{
			if (awg->priority == highestPriority &&
				(hasPresenceGravityTraces || !awg->requiresPresenceGravityTraces))
			{
				tpSumWeight += awg->weight;
				tpDisplacement += awg->orientationDisplacement * awg->weight;
			}
		}
		if (tpSumWeight > weightLeft)
		{
			tpDisplacement *= weightLeft / tpSumWeight;
			tpSumWeight = weightLeft;
		}

		result += tpDisplacement * tpSumWeight;

		maxPriority = highestPriority - 1;
		weightLeft -= tpSumWeight;
	}

	return result.to_quat();
}

void ModuleMovement::on_activate_movement(ModuleMovement* _prevMovement)
{
	if (collisionFlags.is_set() || collidesWithFlags.is_set())
	{
		if (auto* c = get_owner()->get_collision())
		{
			if (collisionFlags.is_set())
			{
				c->push_collision_flags(collisionMovementName, collisionFlags.get());
			}
			if (collidesWithFlags.is_set())
			{
				c->push_collides_with_flags(collisionMovementName, collidesWithFlags.get());
			}
			c->update_collidable_object();
		}
	}
}

void ModuleMovement::on_deactivate_movement(ModuleMovement* _nextMovement)
{
	if (collisionFlags.is_set() || collidesWithFlags.is_set())
	{
		if (auto* c = get_owner()->get_collision())
		{
			if (collisionFlags.is_set())
			{
				c->pop_collision_flags(collisionMovementName);
			}
			if (collidesWithFlags.is_set())
			{
				c->pop_collides_with_flags(collisionMovementName);
			}
			c->update_collidable_object();
		}
	}
}
