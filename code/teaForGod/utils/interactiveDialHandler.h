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

	struct InteractiveDialHandler
	{
	public:
		void initialise(Framework::IModulesOwner* _owner, Name const & _interactiveDeviceIdVarInOwner, Name const & _interactiveDeviceIdVarInInteractives = Name::invalid());

		void advance(float _deltaTime);

		bool is_valid() const { return !dials.is_empty(); }

		int get_dial_at() const { return absoluteDialAt - dialZero; }
		void reset_dial_zero(int _at = 0) { dialZero = absoluteDialAt - _at; }

		bool is_grabbed() const { return isGrabbed; }
		Framework::IModulesOwner* get_grabbed_by() const { return grabbedBy.get(); }

		Array<SafePtr<Framework::IModulesOwner>> const& get_dials() const { return dials; }

	private:
		Name interactiveDeviceIdVar; // var id in owner that holds id to objects that are buttons

		Array<SafePtr<Framework::IModulesOwner>> dials;

		int absoluteDialAt = 0;
		int dialZero = 0;

		bool isGrabbed = false;
		SafePtr<Framework::IModulesOwner> grabbedBy;
	};
};
