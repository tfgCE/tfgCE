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

	struct InteractiveSwitchHandler
	{
	public:
		void initialise(Framework::IModulesOwner* _owner, Name const & _interactiveDeviceIdVarInOwner, Name const & _interactiveDeviceIdVarInInteractives = Name::invalid());

		void advance(float _deltaTime);

		bool is_valid() const { return !switches.is_empty(); }

		float get_switch_at() const { return switchAt; }
		float get_prev_switch_at() const { return prevSwitchAt; }
		float get_output() const { return output; }
		bool is_on() const { return switchAt >= 0.5f; }
		bool is_off() const { return ! is_on(); }

		bool is_grabbed() const { return isGrabbed; }
		Framework::IModulesOwner* get_grabbed_by() const { return grabbedBy.get(); }

		Array<SafePtr<Framework::IModulesOwner>> const& get_switches() const { return switches; }

	private:
		Name interactiveDeviceIdVar; // var id in owner that holds id to objects that are buttons

		Array<SafePtr<Framework::IModulesOwner>> switches;

		float switchAt = 0.0f;
		float prevSwitchAt = 0.0f;
		float output = 0.0f;

		bool isGrabbed = false;
		SafePtr<Framework::IModulesOwner> grabbedBy;
	};
};
