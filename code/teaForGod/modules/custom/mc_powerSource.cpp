#include "mc_powerSource.h"

#include "..\..\..\framework\module\moduleAppearanceImpl.inl"
#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\moduleAppearance.h"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(nominalPowerOutput);
DEFINE_STATIC_NAME(powerOutput);

//

REGISTER_FOR_FAST_CAST(PowerSource);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new PowerSource(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & PowerSource::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("powerSource")), create_module);
}

PowerSource::PowerSource(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

PowerSource::~PowerSource()
{
}

void PowerSource::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		nominalPowerOutput = _moduleData->get_parameter<float>(this, NAME(nominalPowerOutput), nominalPowerOutput);
		nominalPowerOutput = _moduleData->get_parameter<float>(this, NAME(powerOutput), nominalPowerOutput);
	}
}
