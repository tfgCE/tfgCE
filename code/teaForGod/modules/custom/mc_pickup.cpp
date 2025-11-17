#include "mc_pickup.h"

#include "..\..\..\framework\module\modules.h"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// module params
DEFINE_STATIC_NAME(throwVelocityMultiplier);
DEFINE_STATIC_NAME(throwRotationMultiplierPitch);
DEFINE_STATIC_NAME(throwRotationMultiplierYaw);
DEFINE_STATIC_NAME(throwRotationMultiplierRoll);
DEFINE_STATIC_NAME(canBePutIntoPocket);
DEFINE_STATIC_NAME(pickupReferenceSocket);

//

REGISTER_FOR_FAST_CAST(Pickup);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new Pickup(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & Pickup::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("pickup")), create_module);
}

Pickup::Pickup(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

Pickup::~Pickup()
{
}

void Pickup::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	throwVelocityMultiplier = _moduleData->get_parameter<float>(this, NAME(throwVelocityMultiplier), throwVelocityMultiplier);
	throwRotationMultiplier.pitch = _moduleData->get_parameter<float>(this, NAME(throwRotationMultiplierPitch), throwRotationMultiplier.pitch);
	throwRotationMultiplier.yaw = _moduleData->get_parameter<float>(this, NAME(throwRotationMultiplierYaw), throwRotationMultiplier.yaw);
	throwRotationMultiplier.roll = _moduleData->get_parameter<float>(this, NAME(throwRotationMultiplierRoll), throwRotationMultiplier.roll);
	canBePutIntoPocket = _moduleData->get_parameter<bool>(this, NAME(canBePutIntoPocket), canBePutIntoPocket);
	pickupReferenceSocket = _moduleData->get_parameter<Name>(this, NAME(pickupReferenceSocket), pickupReferenceSocket);
}