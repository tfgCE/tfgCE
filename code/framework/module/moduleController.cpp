#include "moduleController.h"

#include "registeredModule.h"
#include "modules.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ModuleController);

static ModuleController* create_module(IModulesOwner* _owner)
{
	return new ModuleController(_owner);
}

RegisteredModule<ModuleController> & ModuleController::register_itself()
{
	return Modules::controller.register_module(String(TXT("base")), create_module);
}

ModuleController::ModuleController(IModulesOwner* _owner)
: base(_owner)
{
}

void ModuleController::reset()
{
	clear_all();
	base::reset();
}

void ModuleController::clear_all()
{
	actions.clear();
	allowGravity.clear();
	distanceToStop.clear();
	requestedPreciseMovement.clear();
	requestedMovementDirection.clear();
	requestedRelativeMovementDirection.clear();// normalised - one of both is used but only one of them can be set
	requestedMovementParameters = MovementParameters();
	appearanceAllowedGait.clear();
	requestedVelocityLinear.clear();
	requestedPreciseRotation.clear();
	requestedVelocityOrientation.clear();
	requestedRelativeVelocityOrientation.clear();
	requestedOrientation.clear();
	requestedLookOrientation.clear();
	requestedRelativeLookOrientation.clear();
	requestedRelativeLook.clear();
	lookOrientationAdditionalOffset.clear();
	maintainRelativeRequestedLookOrientation.clear();
	snapYawToLookOrientation.clear();
	followYawToLookOrientation.clear();
	requestedRelativeHands[2].clear();
	requestedPlacement.clear();
}

Name const & ModuleController::get_final_requested_gait_name() const
{
	return appearanceAllowedGait.get(requestedMovementParameters.gaitName);
}

void ModuleController::setup_with(ModuleData const * _moduleData)
{
	Module::setup_with(_moduleData);
	if (_moduleData)
	{
		/*
		height = _moduleData->get_parameter<float>(String("height"), height);
		heightCentrePt = _moduleData->get_parameter<float>(String("heightcentrept"), heightCentrePt);
		heightCentrePt = _moduleData->get_parameter<float>(String("heightcenterpt"), heightCentrePt);
		allowSlide = _moduleData->get_parameter<bool>(String("allowslide"), allowSlide);
		*/
	}
}

void ModuleController::internal_change_room(Transform const & _intoRoomTransform)
{
	if (requestedVelocityLinear.is_set())
	{
		requestedVelocityLinear = _intoRoomTransform.vector_to_local(requestedVelocityLinear.get());
	}
	if (requestedLookOrientation.is_set())
	{
		requestedLookOrientation = _intoRoomTransform.get_orientation().to_local(requestedLookOrientation.get());
		clear_requested_relative_look();
	}
}

void ModuleController::set_relative_requested_velocity_linear(Vector3 const & _requestedVelocityLinear)
{
	ModulePresence* presence = get_owner()->get_presence();
	an_assert(presence);
	requestedVelocityLinear = presence->get_placement().get_orientation().to_world(_requestedVelocityLinear);
}

void ModuleController::set_requested_look_orientation(Quat const& _requestedLookOrientation)
{
	an_assert(_requestedLookOrientation.is_normalised());
	requestedLookOrientation = _requestedLookOrientation;
	requestedRelativeLookOrientation.clear();
	clear_requested_relative_look();
}

void ModuleController::set_relative_requested_look_orientation(Quat const & _requestedLookOrientation)
{
	ModulePresence* presence = get_owner()->get_presence();
	an_assert(presence);
	set_requested_look_orientation(presence->get_placement().get_orientation().to_world(_requestedLookOrientation));
	requestedRelativeLookOrientation = _requestedLookOrientation;
}

Quat ModuleController::get_relative_requested_look_orientation() const
{
	if (maintainRelativeRequestedLookOrientation.get(false) && requestedRelativeLookOrientation.is_set())
	{
		return requestedRelativeLookOrientation.get();
	}
	else if (requestedLookOrientation.is_set())
	{
		ModulePresence* presence = get_owner()->get_presence();
		an_assert(presence);
		return presence->get_placement().get_orientation().to_local(requestedLookOrientation.get());
	}
	else
	{
		return Quat::identity;
	}
}

Transform ModuleController::get_requested_relative_look() const
{
	if (requestedRelativeLook.is_set())
	{
		return requestedRelativeLook.get();
	}
	else
	{
		return Transform(Vector3::zero, get_relative_requested_look_orientation());
	}
}

Quat ModuleController::get_requested_look_orientation() const
{
	if (requestedRelativeLook.is_set())
	{
		ModulePresence* presence = get_owner()->get_presence();
		an_assert(presence);
		return presence->get_placement().get_orientation().to_world(requestedRelativeLook.get().get_orientation());
	}
	else if (requestedLookOrientation.is_set())
	{
		return requestedLookOrientation.get();
	}
	else
	{
		return Quat::identity;
	}
}

Optional<Vector3> ModuleController::get_requested_movement_direction() const
{
	if (requestedRelativeMovementDirection.is_set())
	{
		ModulePresence* presence = get_owner()->get_presence();
		an_assert(presence);
		return presence->get_placement().get_orientation().to_world(requestedRelativeMovementDirection.get());
	}
	return requestedMovementDirection;
}

Optional<Vector3> ModuleController::get_relative_requested_movement_direction() const
{
	if (requestedMovementDirection.is_set())
	{
		ModulePresence* presence = get_owner()->get_presence();
		an_assert(presence);
		return presence->get_placement().get_orientation().to_local(requestedMovementDirection.get());
	}
	return requestedRelativeMovementDirection;
}
