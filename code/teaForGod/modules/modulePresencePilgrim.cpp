#include "modulePresencePilgrim.h"

#include "..\..\framework\module\modules.h"
#include "..\..\core\random\random.h"
#include "..\..\core\debug\debugRenderer.h"

#include "gameplay\modulePilgrim.h"

using namespace TeaForGodEmperor;

REGISTER_FOR_FAST_CAST(ModulePresencePilgrim);

static Framework::ModulePresence* create_module(Framework::IModulesOwner* _owner)
{
	return new ModulePresencePilgrim(_owner);
}

Framework::RegisteredModule<Framework::ModulePresence> & ModulePresencePilgrim::register_itself()
{
	return Framework::Modules::presence.register_module(String(TXT("pilgrim")), create_module);
}

ModulePresencePilgrim::ModulePresencePilgrim(Framework::IModulesOwner* _owner)
: base( _owner )
{
}
