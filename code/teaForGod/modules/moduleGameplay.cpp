#include "moduleGameplay.h"

#include "..\..\framework\module\modules.h"

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(ModuleGameplay);

ModuleGameplay::ModuleGameplay(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModuleGameplay::~ModuleGameplay()
{
}
