#pragma once

#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\name.h"

#include "..\..\framework\modulesOwner\modulesOwner.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class Pickup;
	};

	struct InteractiveButtonHandler
	{
	public:
		void initialise(Framework::IModulesOwner* _owner, Name const & _interactiveDeviceIdVarInOwner, Name const & _interactiveDeviceIdVarInInteractives = Name::invalid());

		void advance(float _deltaTime);

		bool is_valid() const { return ! buttons.is_empty(); }

		bool is_button_pressed() const { return buttonPressed; }
		bool has_button_been_pressed() const { return buttonPressed && !prevButtonPressed; }
		bool is_triggered() const { return triggered; }

		// setup
		void has_to_be_held(bool _hasToBeHeld = true) { hasToBeHeld = _hasToBeHeld; }

		Array<SafePtr<Framework::IModulesOwner>> const& get_buttons() const { return buttons; }

	private:
		Name interactiveDeviceIdVar; // var id in owner that holds id to objects that are buttons

		TeaForGodEmperor::CustomModules::Pickup* asPickup = nullptr;

		Array<SafePtr<Framework::IModulesOwner>> buttons;

		bool buttonPressed = false;
		bool prevButtonPressed = false;

		bool triggered = false; // more reliable to avoid very quick switching that may happen due to lack of precision of motion controllers

		float waitForButtonToRelease = 0.0f;

		// setup
		bool hasToBeHeld = false;
	};
};
