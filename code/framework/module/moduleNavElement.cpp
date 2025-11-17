#include "moduleNavElement.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ModuleNavElement);

ModuleNavElement::ModuleNavElement(IModulesOwner* _owner)
: Module(_owner)
{
}

void ModuleNavElement::add_to(Nav::Mesh* _navMesh)
{
}
