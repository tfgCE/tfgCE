#include "moduleMovementHoverOld.h"

#include "registeredModule.h"
#include "modules.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(inclinationLimit);
DEFINE_STATIC_NAME(leveledXYThrustCoef);
DEFINE_STATIC_NAME(stopInclinationCoef);
DEFINE_STATIC_NAME(changeSpeedInclinationCoef);
DEFINE_STATIC_NAME(airResistanceCoef);
DEFINE_STATIC_NAME(collisionAffectsVelocityFactor);
DEFINE_STATIC_NAME(maxCollisionSpeed);
DEFINE_STATIC_NAME(levelCollisionEffectFactor);

// module variables
DEFINE_STATIC_NAME(reorientationTime);

//

REGISTER_FOR_FAST_CAST(ModuleMovementHoverOld);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementHoverOld(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleMovementHoverOldData(_inLibraryStored);
}

RegisteredModule<ModuleMovement> & ModuleMovementHoverOld::register_itself()
{
	return Modules::movement.register_module(String(TXT("hover old")), create_module, create_module_data);
}

ModuleMovementHoverOld::ModuleMovementHoverOld(IModulesOwner* _owner)
: base(_owner)
{
}

void ModuleMovementHoverOld::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		reorientationTime = _moduleData->get_parameter<float>(this, NAME(reorientationTime), reorientationTime);
	}
}

void ModuleMovementHoverOld::on_activate_movement(ModuleMovement* _prevMovement)
{
	base::on_activate_movement(_prevMovement);
	reorientatePt = 0.0f;
	applyResultForce = 0.0f;
}

void ModuleMovementHoverOld::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	auto const * presence = get_owner()->get_presence();
	Vector3 gravity = presence->get_gravity_dir();
	if (gravity.is_zero())
	{
		gravity = -presence->get_placement().get_axis(Axis::Up);
	}

	gravity.normalise(); // otherwise they go crazy

	if (reorientatePt < 1.0f)
	{
		if (reorientatePt == 0.0f)
		{
			currentReorientationTime = reorientationTime;
			Vector3 up = presence->get_placement().get_axis(Axis::Up);
			float orientated = clamp(Vector3::dot(up, -gravity), 0.0f, 1.0f); // if more than 90' then just not orientated at all
			currentReorientationTime = currentReorientationTime * (1.0f - orientated);
		}

		float prevReorientatePt = reorientatePt;
		reorientatePt = min(1.0f, currentReorientationTime != 0.0f? reorientatePt + _deltaTime / currentReorientationTime : 1.0f);
		Transform currentPlacement = presence->get_placement();
		Transform idealPlacement = matrix_from_up_right(currentPlacement.get_translation(), -gravity, Vector3::cross(currentPlacement.get_axis(Axis::Forward), -gravity)).to_transform();
		
		//debug_context(presence->get_in_room());
		//debug_draw_transform_size(true, currentPlacement, 0.4f);
		//debug_draw_transform_size(true, idealPlacement, 0.2f);
		//debug_no_context();

		Rotator3 rotationRequired = currentPlacement.get_orientation().to_local(idealPlacement.get_orientation()).to_rotator();

		// just stop with velocity rotation
		_context.velocityRotation = Rotator3::zero;

		// we want to reorientate in more controllable amount of time
		{
			float r = BlendCurve::cubic(reorientatePt);
			float p = BlendCurve::cubic(prevReorientatePt);
			_context.currentDisplacementRotation = (rotationRequired * ((r - p) / (1.0f - p))).to_quat(); // use percentage
			an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
		}

		// try to stop
		_context.velocityLinear = _context.velocityLinear * max(0.0f, 1.0f - 3.0f * _deltaTime);
		_context.currentDisplacementLinear = _context.velocityLinear * _deltaTime;
	}
	else if (_deltaTime != 0.0f && !gravity.is_zero())
	{
		applyResultForce = blend_to_using_speed_based_on_time(applyResultForce, 1.0f, 0.5f, _deltaTime);

		/*					thrust ^  .^ result thrust
		 *						  / .'
		 *						 /.'
		 *	 - - air resist <---*---> result force - - - - - - - leveled
		 *						|
		 *						|
		 *						v gravity
		 *
		 *
		 *	for thrust we leave z component (in leveled space) untouch
		 *	but xy we scale by leveledXYThrustCoef
		 *
		 *	try to have z component of thrust to counter gravity (but not if vertical movement is required)
		 *
		 *	air resistance is dependent on velocity
		 *
		 *	when trying to find inclination we use two factors
		 *		1.	find inclination basing on requested speed to have constant movement (we have to counter air resistance in xy plane)
		 *		2.	if current speed is different to requested, we use further inclination to speed up or slow down (when close to final speed, do not use this)
		 *
		 *	one unknown thing is inclination factor for constant speed, we calculate that for both axes separately in calculate_inclination_factor
		 */
		float const & inclinationLimit = find_inclination_limit_of_current_gait();
		float const & leveledXYThrustCoef = find_leveled_xy_thrust_coef_of_current_gait();
		float const & stopInclinationCoef = find_stop_inclination_coef_of_current_gait();
		float const & changeSpeedInclinationCoef = find_change_speed_inclination_coef_of_current_gait();
		float const & airResistanceCoef = find_air_resitance_coef_of_current_gait();

		float const gravityForce = gravity.length();
		Transform placement = presence->get_placement();

		// calculate leveled space - using gravity, just like gyrocopters do
		Transform leveled;
		leveled.build_from_axes(Axis::Up, Axis::Forward, placement.get_axis(Axis::Forward), placement.get_axis(Axis::Right), -gravity.normal(), placement.get_translation());

		Transform placementInLeveled = leveled.to_local(placement);

		PrepareMoveContext prepareMoveContext;
		Vector3 currentVelocityLocalInLeveled = leveled.vector_to_local(_context.velocityLinear);
		Vector3 requestedVelocityLocalInLeveled = leveled.vector_to_local(calculate_requested_velocity(_deltaTime, true, prepareMoveContext));

		// calculate how we want to be placed in leveled space (pitch and roll)
		Rotator3 requestedRotationLocalInLeveled = Rotator3::zero;
		// more rapid moves when slowing down
		float diffCoefX = abs(requestedVelocityLocalInLeveled.x) < 0.1f ? stopInclinationCoef : changeSpeedInclinationCoef;
		float diffCoefY = abs(requestedVelocityLocalInLeveled.y) < 0.1f ? stopInclinationCoef : changeSpeedInclinationCoef;
		requestedRotationLocalInLeveled.pitch = -clamp(calculate_inclination_angle(requestedVelocityLocalInLeveled.y, gravityForce, leveledXYThrustCoef, airResistanceCoef) + reduce_to_zero(requestedVelocityLocalInLeveled.y - currentVelocityLocalInLeveled.y, requestedVelocityLocalInLeveled.y * 0.2f) * diffCoefY, -inclinationLimit, inclinationLimit);
		requestedRotationLocalInLeveled.roll = clamp(calculate_inclination_angle(requestedVelocityLocalInLeveled.x, gravityForce, leveledXYThrustCoef, airResistanceCoef) + reduce_to_zero(requestedVelocityLocalInLeveled.x - currentVelocityLocalInLeveled.x, requestedVelocityLocalInLeveled.x * 0.2f) * diffCoefX, -inclinationLimit, inclinationLimit);

		Quat const requestedOrientationLocalInLeveled = requestedRotationLocalInLeveled.to_quat();
		Quat const requestedOrientationLocalInPlacement = placementInLeveled.to_local(requestedOrientationLocalInLeveled);
		Rotator3 const requestedRotationLocal = requestedOrientationLocalInPlacement.to_rotator();

		// use normal rotation
		Rotator3 accelerationRotationFrame = Rotator3::zero;
		adjust_velocity_orientation_for(_deltaTime, REF_ accelerationRotationFrame, _context.velocityRotation, requestedRotationLocal);
		accelerationRotationFrame.yaw = 0.0f;

		// calculate our actual velocity and displacement in this frame
		Rotator3 nextVelocityRotation = _context.velocityRotation + accelerationRotationFrame;
		// keep yaw
		_context.velocityRotation.pitch = nextVelocityRotation.pitch;
		_context.velocityRotation.roll = nextVelocityRotation.roll;
		_context.currentDisplacementRotation = (_context.velocityRotation * _deltaTime).to_quat();
		an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));

		//

		// linear velocity
		Vector3 thrustDirLocalInLeveled = placementInLeveled.vector_to_world(Vector3::zAxis);
		todo_note(TXT("expose as parameters"));
		Vector3 thrustLocalInLeveled = thrustDirLocalInLeveled * (gravityForce / max(0.3f, thrustDirLocalInLeveled.z))
			* (1.0f + clamp(6.0f * (requestedVelocityLocalInLeveled.z - currentVelocityLocalInLeveled.z), -3.0f, 4.5f)); // vertical movement
		thrustLocalInLeveled.x = thrustDirLocalInLeveled.x * 0.7f + 0.3f * thrustLocalInLeveled.x; // try to keep normal movement
		thrustLocalInLeveled.y = thrustDirLocalInLeveled.y * 0.7f + 0.3f * thrustLocalInLeveled.y;
		thrustLocalInLeveled.x *= leveledXYThrustCoef;
		thrustLocalInLeveled.y *= leveledXYThrustCoef;
		Vector3 thrust = leveled.vector_to_world(thrustLocalInLeveled);
		Vector3 resultForce = thrust + gravity
							- _context.velocityLinear * airResistanceCoef; // slow down

		_context.velocityLinear = _context.velocityLinear + resultForce * _deltaTime * applyResultForce;
		_context.currentDisplacementLinear = _context.velocityLinear * _deltaTime;
	}

	Vector3 originalDisplacementLinear = _context.currentDisplacementLinear;
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
	if (_deltaTime != 0.0f)
	{
		float const & collisionAffectsVelocityFactor = find_collision_affects_velocity_factor_of_current_gait();
		float const & maxCollisionSpeed = find_max_collision_speed_of_current_gait();
		float const & levelCollisionEffectFactor = find_level_collision_effect_factor_of_current_gait();

		// apply collision to velocity but prefer xy plane
		Vector3 hitForce = collisionAffectsVelocityFactor * (_context.currentDisplacementLinear - originalDisplacementLinear) / _deltaTime;
		hitForce = hitForce * (1.0f - levelCollisionEffectFactor) + levelCollisionEffectFactor * hitForce.drop_using(gravity);
		_context.velocityLinear += hitForce;
		if (maxCollisionSpeed > 0.0f &&
			_context.velocityLinear.length_squared() > sqr(maxCollisionSpeed))
		{
			_context.velocityLinear = _context.velocityLinear.normal() * maxCollisionSpeed;
		}
	}
}

Vector3 ModuleMovementHoverOld::calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const
{
	Vector3 requestedVelocity = base::calculate_requested_velocity(_deltaTime, _adjustForStoppingEtc, _context);

	IModulesOwner * modulesOwner = get_owner();

	ModulePresence * const presence = modulesOwner->get_presence();
	ModuleController const * const controller = modulesOwner->get_controller();
	if (presence && controller)
	{
		Vector3 pureRequestedVelocity = requestedVelocity;
		{
			Optional<Vector3> const & requestedMovementDirection = controller->get_requested_movement_direction();
			if (requestedMovementDirection.is_set())
			{
				MovementParameters movementParameters = controller->get_requested_movement_parameters();
				movementParameters.gaitName = currentGaitName;
				float speed;
				calculate_requested_linear_speed(movementParameters, OUT_ &speed);
				pureRequestedVelocity = requestedMovementDirection.get() * speed;
			}
		}

		{
			Optional<Vector3> const & requestedVelocityLinear = controller->get_requested_velocity_linear();
			if (requestedVelocityLinear.is_set())
			{
				pureRequestedVelocity = requestedVelocityLinear.get();
			}
		}

		requestedVelocity.z = pureRequestedVelocity.z;
	}

	return requestedVelocity;
}

float ModuleMovementHoverOld::calculate_inclination_angle(float _forVelocity, float _gravityForce, float const & leveledXYThrustCoef, float const & airResistanceCoef) const
{
	an_assert(_gravityForce >= 0.0f);
	float const airResistance = -_forVelocity * airResistanceCoef;
	float const requiredResultForce = -airResistance;
	float const requiredThrustAtLevel = requiredResultForce / leveledXYThrustCoef;

	/*
	 *	   -gravity ^   ^ thrust
	 *				|  /
	 *				|a/
	 *				|/
	 *				*---> required thrust at level
	 *
	 *	our inclination angle is a
	 */
	float const a = atan_deg(requiredThrustAtLevel / _gravityForce);

	return a;
}

static inline float adjust_gait_speed_adjustment(float _gaitSpeedAdjustment, float _adjustOverOne = 0.6f) { return _gaitSpeedAdjustment < 1.0f ? _gaitSpeedAdjustment : (1.0f + (_gaitSpeedAdjustment - 1.0f) * _adjustOverOne); }

DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHoverOld, find_inclination_limit_of_gait, find_inclination_limit_of_current_gait, inclinationLimit, float, adjust_gait_speed_adjustment(gaitSpeedAdjustment, 0.2f));
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHoverOld, find_leveled_xy_thrust_coef_of_gait, find_leveled_xy_thrust_coef_of_current_gait, leveledXYThrustCoef, float, adjust_gait_speed_adjustment(gaitSpeedAdjustment));
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHoverOld, find_stop_inclination_coef_of_gait, find_stop_inclination_coef_of_current_gait, stopInclinationCoef, float, adjust_gait_speed_adjustment(gaitSpeedAdjustment));
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHoverOld, find_change_speed_inclination_coef_of_gait, find_change_speed_inclination_coef_of_current_gait, changeSpeedInclinationCoef, float, adjust_gait_speed_adjustment(gaitSpeedAdjustment));
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_air_resitance_coef_of_gait, find_air_resitance_coef_of_current_gait, airResistanceCoef, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_collision_affects_velocity_factor_of_gait, find_collision_affects_velocity_factor_of_current_gait, collisionAffectsVelocityFactor, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_max_collision_speed_of_gait, find_max_collision_speed_of_current_gait, maxCollisionSpeed, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementHoverOld, find_level_collision_effect_factor_of_gait, find_level_collision_effect_factor_of_current_gait, levelCollisionEffectFactor, float);

//

REGISTER_FOR_FAST_CAST(ModuleMovementHoverOldData);

ModuleMovementHoverOldData::ModuleMovementHoverOldData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleMovementHoverOldData::load_gait_param_from_xml(ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	LOAD_GAITS_PROP(float, inclinationLimit, 30.0f);
	LOAD_GAITS_PROP(float, leveledXYThrustCoef, 15.0f);
	LOAD_GAITS_PROP(float, stopInclinationCoef, 10.0f);
	LOAD_GAITS_PROP(float, changeSpeedInclinationCoef, 5.0f);
	LOAD_GAITS_PROP(float, airResistanceCoef, 0.9f);
	LOAD_GAITS_PROP(float, collisionAffectsVelocityFactor, 1.0f);
	LOAD_GAITS_PROP(float, maxCollisionSpeed, 0.0f); // no limit
	LOAD_GAITS_PROP(float, levelCollisionEffectFactor, 0.7f);

	return base::load_gait_param_from_xml(_gait, _node, _lc);
}
