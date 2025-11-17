#include "modulePresenceBackgroundMoverBase.h"

#include "..\..\framework\module\modules.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(ModulePresenceBackgroundMoverBase);

static Framework::ModulePresence* create_module(Framework::IModulesOwner* _owner)
{
	return new ModulePresenceBackgroundMoverBase(_owner);
}

Framework::RegisteredModule<Framework::ModulePresence> & ModulePresenceBackgroundMoverBase::register_itself()
{
	return Framework::Modules::presence.register_module(String(TXT("background mover base")), create_module);
}

ModulePresenceBackgroundMoverBase::ModulePresenceBackgroundMoverBase(Framework::IModulesOwner* _owner)
: base( _owner )
{
}
