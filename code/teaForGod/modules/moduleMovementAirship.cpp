#include "moduleMovementAirship.h"

#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\registeredModule.h"

#include "..\..\framework\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace TeaForGodEmperor;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(speed);

//

REGISTER_FOR_FAST_CAST(ModuleMovementAirship);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementAirship(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementAirship::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("airship")), create_module);
}

ModuleMovementAirship::ModuleMovementAirship(Framework::IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

void ModuleMovementAirship::be_at(Name const& _zone, Optional<VectorInt2> const& _mapCell, Transform const& _placement)
{
	airshipZone = _zone;
	airshipMapCell = _mapCell;
	airshipPlacement = _placement;
}
