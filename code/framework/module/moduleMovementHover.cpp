#include "moduleMovementHover.h"

#include "registeredModule.h"
#include "modules.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(rotationBlendTime);
DEFINE_STATIC_NAME(rotationFromSpeedLimit);
DEFINE_STATIC_NAME(rotationFromSpeedCoef);
DEFINE_STATIC_NAME(rotationFromTurningLimit);
DEFINE_STATIC_NAME(rotationFromTurningCoef);

//

REGISTER_FOR_FAST_CAST(ModuleMovementHover);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementHover(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleMovementHoverData(_inLibraryStored);
}

RegisteredModule<ModuleMovement> & ModuleMovementHover::register_itself()
{
	return Modules::movement.register_module(String(TXT("hover")), create_module, create_module_data);
}

ModuleMovementHover::ModuleMovementHover(IModulesOwner* _owner)
: base(_owner)
{
}

void ModuleMovementHover::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
}

void ModuleMovementHover::on_activate_movement(ModuleMovement* _prevMovement)
{
	base::on_activate_movement(_prevMovement);
}

void ModuleMovementHover::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);

	ModulePresence* const presence = get_owner()->get_presence();
	ModuleController const* const controller = get_owner()->get_controller();

	if (presence && controller && _deltaTime != 0.0f)
	{
		debug_context(presence->get_in_room());
		debug_subject(get_owner());
		debug_filter(moduleMovementDebug);

		MovementParameters movementParameters = controller->get_requested_movement_parameters();
		movementParameters.gaitName = currentGaitName;
		
		Transform upPlacement = presence->calculate_environment_up_placement();
		Vector3 velocityWS = _context.velocityLinear;
		Rotator3 rotation = _context.velocityRotation;
		//float const forwardTime = 0.5f;
		//velocityWS += (velocityWS - presence->get_velocity_linear()) * (forwardTime / _deltaTime);
		//rotation += _context.currentDisplacementRotation.to_rotator() * (forwardTime / _deltaTime);
		Vector3 velocityUP = upPlacement.vector_to_local(velocityWS);

		Rotator3 requestedRotation = Rotator3::zero;
		Rotator3 rfsLimit = movementParameters.gaitName.is_valid() ? find_rotation_from_speed_limit_of_gait(movementParameters.gaitName) : find_rotation_from_speed_limit_of_current_gait();
		Rotator3 rfsCoef = movementParameters.gaitName.is_valid() ? find_rotation_from_speed_coef_of_gait(movementParameters.gaitName) : find_rotation_from_speed_coef_of_current_gait();
		Rotator3 rftLimit = movementParameters.gaitName.is_valid() ? find_rotation_from_turning_limit_of_gait(movementParameters.gaitName) : find_rotation_from_turning_limit_of_current_gait();
		Rotator3 rftCoef = movementParameters.gaitName.is_valid() ? find_rotation_from_turning_coef_of_gait(movementParameters.gaitName) : find_rotation_from_turning_coef_of_current_gait();
		float rotBlendTime = movementParameters.gaitName.is_valid() ? find_rotation_blend_time_of_gait(movementParameters.gaitName) : find_rotation_blend_time_of_current_gait();

		if (!rfsCoef.is_zero())
		{
			requestedRotation.pitch += clamp(velocityUP.y * rfsCoef.pitch, -rfsLimit.pitch, rfsLimit.pitch);
			requestedRotation.roll += clamp(velocityUP.x * rfsCoef.roll, -rfsLimit.roll, rfsLimit.roll);
		}
		if (!rftCoef.is_zero())
		{
			requestedRotation.pitch += clamp(rotation.yaw * rftCoef.pitch, -rftLimit.pitch, rftLimit.pitch);
			requestedRotation.yaw += clamp(rotation.yaw * rftCoef.yaw, -rftLimit.yaw, rftLimit.yaw);
			requestedRotation.roll += clamp(rotation.yaw * rftCoef.roll, -rftLimit.roll, rftLimit.roll);
		}

		Quat requestedOrientation = upPlacement.to_world(requestedRotation.to_quat());

		Quat curOrientation = presence->get_placement().get_orientation();

		//Rotator3 reqRot = requestedOrientation.to_rotator();
		//Rotator3 curRot = curOrientation.to_rotator();
		
		//requestedOrientation = reqRot.to_quat();
		//curOrientation = curRot.to_quat();
		
		Quat diffOrientation = curOrientation.to_local(requestedOrientation);

		float useFull = clamp(_deltaTime / rotBlendTime, 0.0f, 1.0f);
		Quat useDiffOrientation = Quat::slerp(useFull, Quat::identity, diffOrientation);

		_context.currentDisplacementRotation = _context.currentDisplacementRotation.rotate_by(useDiffOrientation);

		debug_draw_transform_size_coloured(true, upPlacement, 0.2f, Colour::blue, Colour::blue, Colour::blue);
		debug_draw_transform_size_coloured(true, Transform(upPlacement.get_translation(), requestedOrientation), 0.1f, Colour::red, Colour::red, Colour::red);
		debug_draw_transform_size_coloured(true, presence->get_placement(), 0.07f, Colour::green, Colour::green, Colour::green);
		
		debug_no_filter();
		debug_no_subject();
		debug_no_context();
	}
}

Vector3 ModuleMovementHover::calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const
{
	return base::calculate_requested_velocity(_deltaTime, _adjustForStoppingEtc, _context);
}

static inline float adjust_gait_speed_adjustment(float _gaitSpeedAdjustment, float _adjustOverOne = 0.6f) { return _gaitSpeedAdjustment < 1.0f ? _gaitSpeedAdjustment : (1.0f + (_gaitSpeedAdjustment - 1.0f) * _adjustOverOne); }

DEFINE_FIND_GAITS_PROP_FUNCS(ModuleMovementHover, find_rotation_blend_time_of_gait, find_rotation_blend_time_of_current_gait, rotationBlendTime, float);
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHover, find_rotation_from_speed_limit_of_gait, find_rotation_from_speed_limit_of_current_gait, rotationFromSpeedLimit, Rotator3, adjust_gait_speed_adjustment(gaitSpeedAdjustment));
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHover, find_rotation_from_speed_coef_of_gait, find_rotation_from_speed_coef_of_current_gait, rotationFromSpeedCoef, Rotator3, adjust_gait_speed_adjustment(gaitSpeedAdjustment));
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHover, find_rotation_from_turning_limit_of_gait, find_rotation_from_turning_limit_of_current_gait, rotationFromTurningLimit, Rotator3, adjust_gait_speed_adjustment(gaitSpeedAdjustment));
DEFINE_FIND_GAITS_PROP_FUNCS_ADJUSTABLE(ModuleMovementHover, find_rotation_from_turning_coef_of_gait, find_rotation_from_turning_coef_of_current_gait, rotationFromTurningCoef, Rotator3, adjust_gait_speed_adjustment(gaitSpeedAdjustment));

//

REGISTER_FOR_FAST_CAST(ModuleMovementHoverData);

ModuleMovementHoverData::ModuleMovementHoverData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool ModuleMovementHoverData::load_gait_param_from_xml(ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	LOAD_GAITS_PROP(float, rotationBlendTime, 0.3f);
	LOAD_GAITS_PROP(Rotator3, rotationFromSpeedLimit, Rotator3(20.0f, 0.0f, 20.0f));
	LOAD_GAITS_PROP(Rotator3, rotationFromSpeedCoef, Rotator3(5.0f, 0.0f, 10.0f));
	LOAD_GAITS_PROP(Rotator3, rotationFromTurningLimit, Rotator3(0.0f, 0.0f, 20.0f));
	LOAD_GAITS_PROP(Rotator3, rotationFromTurningCoef, Rotator3(0.0f, 0.0f, 2.0f));

	return base::load_gait_param_from_xml(_gait, _node, _lc);
}
