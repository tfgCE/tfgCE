#include "lhd_weaponPartWeaponSetup.h"

#include "..\..\..\game\weaponPart.h"
#include "..\..\..\tutorials\tutorialSystem.h"

//

using namespace Loader;
using namespace HubDraggables;

//

REGISTER_FOR_FAST_CAST(WeaponPartWeaponSetup);

WeaponPartWeaponSetup::WeaponPartWeaponSetup(TeaForGodEmperor::WeaponPart const* _wp)
: weaponPart(_wp)
{
	if (TeaForGodEmperor::TutorialSystem::check_active() && weaponPart.is_set())
	{
		tutorialHubId = weaponPart->get_tutorial_hub_id();
	}
}

TeaForGodEmperor::WeaponPart const* WeaponPartWeaponSetup::get_weapon_part() const
{
	return weaponPart.get();
}
