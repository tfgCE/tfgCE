#include "mc_operativeDevice.h"

#include "mc_healthReporter.h"
#include "mc_powerSource.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

DEFINE_STATIC_NAME(deviceOrderedState); // hardcoded - ordered state of a device
DEFINE_STATIC_NAME(deviceRequestedState); // hardcoded - requested state of a device as possible
DEFINE_STATIC_NAME(deviceTargetState); // hardcoded - target state of a device

//

REGISTER_FOR_FAST_CAST(OperativeDevice);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new OperativeDevice(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & OperativeDevice::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("operativeDevice")), create_module);
}

OperativeDevice::OperativeDevice(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

OperativeDevice::~OperativeDevice()
{
}

void OperativeDevice::activate()
{
	base::activate();

	deviceOrderedStateVar = get_owner()->access_variables().find<float>(NAME(deviceOrderedState));
}

bool OperativeDevice::is_not_fully_operative_because_of_user() const
{
	todo_note(TXT("there are magical thresholds here to determine if something was damaged or disabled etc..."));

	if (auto* hr = get_owner()->get_custom<CustomModules::HealthReporter>())
	{
		hr->update();
		if (hr->get_actual_health() < hr->get_nominal_health().mul(0.95f))
		{
			return true;
		}
	}

	if (auto* ps = get_owner()->get_custom<CustomModules::PowerSource>())
	{
		if (ps->get_active() < 0.5f)
		{
			return true;
		}
	}

	if (deviceOrderedStateVar.is_valid())
	{
		if (deviceOrderedStateVar.get<float>() < 0.5f)
		{
			return true;
		}
	}

	return false;
}
