#include "interactiveButtonHandler.h"

#include "..\modules\custom\mc_pickup.h"

#include "..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\framework\world\world.h"

#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// vars
DEFINE_STATIC_NAME(interactiveDeviceId);

// tags
DEFINE_STATIC_NAME(interactiveButton);

//

#define BUTTON_RELEASE_TIME 0.2f

//

void InteractiveButtonHandler::initialise(Framework::IModulesOwner* _owner, Name const & _interactiveDeviceIdVarInOwner, Name const & _interactiveDeviceIdVarInInteractives)
{
	Name interactiveDeviceIdVarInInteractives = _interactiveDeviceIdVarInInteractives;
	if (!interactiveDeviceIdVarInInteractives.is_valid())
	{
		interactiveDeviceIdVarInInteractives = hardcoded NAME(interactiveDeviceId);
	}
	buttons.clear();
	if (auto* id = _owner->get_variables().get_existing<int>(_interactiveDeviceIdVarInOwner))
	{
		_owner->get_in_world()->for_every_object_with_id(interactiveDeviceIdVarInInteractives, *id,
			[this](Framework::Object* _object)
		{
			if (_object->get_tags().get_tag(NAME(interactiveButton)))
			{
				buttons.push_back(SafePtr<Framework::IModulesOwner>(_object));
			}
			return false; // keep going on
		});
	}

	asPickup = _owner->get_custom<CustomModules::Pickup>();

	waitForButtonToRelease = BUTTON_RELEASE_TIME + 0.1f;
}

void InteractiveButtonHandler::advance(float _deltaTime)
{
	waitForButtonToRelease = max(0.0f, waitForButtonToRelease - _deltaTime);

	prevButtonPressed = buttonPressed;
	buttonPressed = false;
	triggered = false;

	bool ok = true;

	if (hasToBeHeld)
	{
		if (!asPickup ||
			!asPickup->is_held())
		{
			ok = false;
		}
	}
		
	if (ok)
	{
		for_every_ref(buttonObject, buttons)
		{
			if (!buttonObject) continue;
			if (auto * mms = fast_cast<Framework::ModuleMovementSwitch>(buttonObject->get_movement()))
			{
				buttonPressed |= mms->is_active_at(1);
			}
		}
	}
	else
	{
		waitForButtonToRelease = BUTTON_RELEASE_TIME;
	}

	if (buttonPressed)
	{
		if (waitForButtonToRelease == 0.0f)
		{
			triggered = true;
		}
		waitForButtonToRelease = BUTTON_RELEASE_TIME;
	}

}
