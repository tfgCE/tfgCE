#include "interactiveSwitchHandler.h"

#include "..\modules\custom\mc_grabable.h"
#include "..\modules\custom\mc_pickup.h"

#include "..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\world\world.h"

//

using namespace TeaForGodEmperor;

//

// vars
DEFINE_STATIC_NAME(interactiveDeviceId);

// tags
DEFINE_STATIC_NAME(interactiveSlider);

//

void InteractiveSwitchHandler::initialise(Framework::IModulesOwner* _owner, Name const & _interactiveDeviceIdVarInOwner, Name const & _interactiveDeviceIdVarInInteractives)
{
	Name interactiveDeviceIdVarInInteractives = _interactiveDeviceIdVarInInteractives;
	if (!interactiveDeviceIdVarInInteractives.is_valid())
	{
		interactiveDeviceIdVarInInteractives = hardcoded NAME(interactiveDeviceId);
	}
	switches.clear();
	if (auto* id = _owner->get_variables().get_existing<int>(_interactiveDeviceIdVarInOwner))
	{
		_owner->get_in_world()->for_every_object_with_id(interactiveDeviceIdVarInInteractives, *id,
			[this](Framework::Object* _object)
		{
			if (_object->get_tags().get_tag(NAME(interactiveSlider)) &&
				fast_cast<Framework::ModuleMovementSwitch>(_object->get_movement()))
			{
				switches.push_back(SafePtr<Framework::IModulesOwner>(_object));
			}
			return false; // keep going on
		});
	}
}

void InteractiveSwitchHandler::advance(float _deltaTime)
{
	prevSwitchAt = switchAt;
	float weight = 0.0f;
	switchAt = 0.0f;
	output = 0.0f;
	isGrabbed = false;
	Framework::IModulesOwner* newGrabbedBy = nullptr;
	{
		for_every_ref(switchObject, switches)
		{
			if (!switchObject) continue;
			if (auto * mms = fast_cast<Framework::ModuleMovementSwitch>(switchObject->get_movement()))
			{
				switchAt += mms->get_at();
				output += mms->get_output();
				weight += 1.0f;
			}
			if (auto * g = switchObject->get_custom<CustomModules::Grabable>())
			{
				if (g->is_grabbed())
				{
					isGrabbed = true;
					if (!newGrabbedBy)
					{
						auto& grabbedBy = g->get_grabbed_by();
						if (!grabbedBy.is_empty())
						{
							newGrabbedBy = grabbedBy.get_first().get();
						}
					}
				}
			}
		}
	}
	grabbedBy = newGrabbedBy;

	if (weight != 0.0f)
	{
		switchAt = switchAt / weight;
		output = output / weight;
	}
}
