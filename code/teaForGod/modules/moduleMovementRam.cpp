#include "moduleMovementRam.h"

#include "..\..\framework\module\modules.h"

using namespace TeaForGodEmperor;

//

// gait params
DEFINE_STATIC_NAME(ramInclination);
DEFINE_STATIC_NAME(ramInclinationBlendTime);
DEFINE_STATIC_NAME(collisionAffectsVelocityFactor);
DEFINE_STATIC_NAME(maxCollisionSpeed);

//

REGISTER_FOR_FAST_CAST(ModuleMovementRam);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementRam(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleMovementRamData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementRam::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("ram")), create_module, create_module_data);
}

ModuleMovementRam::ModuleMovementRam(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

void ModuleMovementRam::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	auto const * presence = get_owner()->get_presence();
	Vector3 gravityDir = presence->get_gravity_dir();
	if (gravityDir.is_zero())
	{
		gravityDir = -presence->get_placement().get_axis(Axis::Up);
	}

	gravityDir.normalise(); // otherwise they go crazy

	// velocity itself will be adjusted in prepare move properly
	// we need to adjust orientation only
	PrepareMoveContext prepareMoveContext;
	Vector3 requestedVelocity = calculate_requested_velocity(_deltaTime, true, prepareMoveContext);
	Rotator3 requestedOrientationLocal = (presence->get_placement().vector_to_local(requestedVelocity) * Vector3(0.0f, 1.0f, 1.0f)).to_rotator();
	requestedOrientationLocal.yaw = 0.0f; // adjust it other way
	requestedOrientationLocal.pitch -= find_ram_inclination_of_current_gait();
	float blendTime = max(0.0001f, find_ram_inclination_blend_time_of_current_gait());
	// add pitch/roll displacement
	_context.currentDisplacementRotation = _context.currentDisplacementRotation.to_world((requestedOrientationLocal * clamp(_deltaTime / blendTime, 0.0f, 1.0f)).to_quat());

	Vector3 originalDisplacementLinear = _context.currentDisplacementLinear;
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
	if (_deltaTime != 0.0f)
	{
		float const & collisionAffectsVelocityFactor = find_collision_affects_velocity_factor_of_current_gait();
		float const & maxCollisionSpeed = find_max_collision_speed_of_current_gait();
		float levelCollisionEffectFactor = 0.8f;

		// apply collision to velocity but prefer xy plane
		Vector3 hitForce = collisionAffectsVelocityFactor * (_context.currentDisplacementLinear - originalDisplacementLinear) / _deltaTime;
		hitForce = hitForce * (1.0f - levelCollisionEffectFactor) + levelCollisionEffectFactor * hitForce.drop_using(gravityDir);
		_context.velocityLinear += hitForce;
		if (maxCollisionSpeed > 0.0f &&
			_context.velocityLinear.length_squared() > sqr(maxCollisionSpeed))
		{
			_context.velocityLinear = _context.velocityLinear.normal() * maxCollisionSpeed;
		}
	}
}

Vector3 ModuleMovementRam::calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const
{
	Vector3 requestedVelocity = base::calculate_requested_velocity(_deltaTime, _adjustForStoppingEtc, _context);

	Framework::IModulesOwner * modulesOwner = get_owner();

	Framework::ModulePresence * const presence = modulesOwner->get_presence();
	Framework::ModuleController const * const controller = modulesOwner->get_controller();
	if (presence && controller)
	{
		Vector3 pureRequestedVelocity = requestedVelocity;
		{
			Optional<Vector3> const & requestedMovementDirection = controller->get_requested_movement_direction();
			if (requestedMovementDirection.is_set())
			{
				Framework::MovementParameters movementParameters = controller->get_requested_movement_parameters();
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

		//requestedVelocity.z = pureRequestedVelocity.z;
	}

	return requestedVelocity;
}
static inline float adjust_gait_speed_adjustment(float _gaitSpeedAdjustment, float _adjustOverOne = 0.6f) { return _gaitSpeedAdjustment < 1.0f ? _gaitSpeedAdjustment : (1.0f + (_gaitSpeedAdjustment - 1.0f) * _adjustOverOne); }

DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementRam, find_ram_inclination_of_gait, find_ram_inclination_of_current_gait, ramInclination, float, adjust_gait_speed_adjustment(gaitSpeedAdjustment, 0.0f));
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementRam, find_ram_inclination_blend_time_of_gait, find_ram_inclination_blend_time_of_current_gait, ramInclinationBlendTime, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementRam, find_collision_affects_velocity_factor_of_gait, find_collision_affects_velocity_factor_of_current_gait, collisionAffectsVelocityFactor, float);
DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementRam, find_max_collision_speed_of_gait, find_max_collision_speed_of_current_gait, maxCollisionSpeed, float);

//

REGISTER_FOR_FAST_CAST(ModuleMovementRamData);

ModuleMovementRamData::ModuleMovementRamData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleMovementRamData::load_gait_param_from_xml(Framework::ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	LOAD_GAITS_PROP(float, ramInclination, 20.0f);
	LOAD_GAITS_PROP(float, ramInclinationBlendTime, 0.1f);
	LOAD_GAITS_PROP(float, collisionAffectsVelocityFactor, 1.0f);
	LOAD_GAITS_PROP(float, maxCollisionSpeed, 0.0f); // no limit

	return base::load_gait_param_from_xml(_gait, _node, _lc);
}
