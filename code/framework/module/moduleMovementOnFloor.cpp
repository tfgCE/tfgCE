#include "moduleMovementOnFloor.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(floorLevelMatchTime);
DEFINE_STATIC_NAME(forceMatchFloorLevel);

//

REGISTER_FOR_FAST_CAST(ModuleMovementOnFloor);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementOnFloor(_owner);
}

RegisteredModule<ModuleMovement> & ModuleMovementOnFloor::register_itself()
{
	return Modules::movement.register_module(String(TXT("onFloor")), create_module);
}

ModuleMovementOnFloor::ModuleMovementOnFloor(IModulesOwner* _owner)
: base(_owner)
{
}

void ModuleMovementOnFloor::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		floorLevelMatchTime = _moduleData->get_parameter<float>(this, NAME(floorLevelMatchTime), floorLevelMatchTime);
		forceMatchFloorLevel = _moduleData->get_parameter<bool>(this, NAME(forceMatchFloorLevel), forceMatchFloorLevel);
		/*
		height = _moduleData->get_parameter<float>(String(TXT("height")), height);
		heightCentrePt = _moduleData->get_parameter<float>(String(TXT("heightcentrept")), heightCentrePt);
		heightCentrePt = _moduleData->get_parameter<float>(String(TXT("heightcenterpt")), heightCentrePt);
		allowSlide = _moduleData->get_parameter<bool>(String(TXT("allowslide")), allowSlide);
		*/
	}
}

void ModuleMovementOnFloor::apply_gravity_to_velocity(float _deltaTime, REF_ Vector3 & _accelerationInFrame, REF_ Vector3 & _locationDisplacement)
{
	IModulesOwner * modulesOwner = get_owner();

	ModulePresence * const presence = modulesOwner->get_presence();
	if (presence)
	{
		if (ModuleCollision const * collision = modulesOwner->get_collision())
		{
			debug_filter(moduleMovementDebug);
			debug_subject(get_owner());
			debug_context(get_owner()->get_presence()->get_in_room());
			float floorLevelOffset = presence->get_floor_level_offset_along_up_dir();
			Range const floorLevelOffsetRange = presence->get_floor_level_match_offset_limit();
			if (!forceMatchFloorLevel &&
				(floorLevelOffset < floorLevelOffsetRange.min || !presence->do_gravity_presence_traces_touch_surroundings()))
			{
				// fall onto floor
				todo_note(TXT("switch to falling and go back to normal only when hit floor"));
				Vector3 accelerationFromGradient = collision->get_gradient();
				float gravityAgainstCollision = clamp(Vector3::dot(accelerationFromGradient, presence->get_gravity_dir()), -presence->get_gravity().length() * useGravity, 0.0f);
				_accelerationInFrame += (presence->get_gravity() * useGravity + presence->get_gravity_dir() * gravityAgainstCollision) * _deltaTime;
			}
			else
			{
				Vector3 upDir = presence->get_environment_up_dir();
				debug_draw_arrow(true, Colour::blue.with_alpha(0.5f), get_owner()->get_presence()->get_placement().get_translation(), get_owner()->get_presence()->get_placement().get_translation() + upDir * floorLevelOffset);
				floorLevelOffset -= Vector3::dot(_locationDisplacement, upDir); // incorporate displacement that there already is
				floorLevelOffset = floorLevelOffsetRange.clamp(floorLevelOffset);
				debug_draw_arrow(true, Colour::blue, get_owner()->get_presence()->get_placement().get_translation(), get_owner()->get_presence()->get_placement().get_translation() + upDir * floorLevelOffset);
				// push out to floor level
				_locationDisplacement += upDir * floorLevelOffset * clamp(floorLevelMatchTime != 0.0f ? _deltaTime / floorLevelMatchTime : 1.0f, 0.0f, 1.0f);
			}
			debug_no_filter();
			debug_no_subject();
			debug_no_context();
		}
	}
}
