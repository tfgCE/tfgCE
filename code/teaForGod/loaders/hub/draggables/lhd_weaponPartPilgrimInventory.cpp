#include "lhd_weaponPartPilgrimInventory.h"

#include "..\..\..\game\weaponPart.h"
#include "..\..\..\tutorials\tutorialSystem.h"

//

using namespace Loader;
using namespace HubDraggables;

//

REGISTER_FOR_FAST_CAST(WeaponPartPilgrimInventory);

WeaponPartPilgrimInventory::WeaponPartPilgrimInventory(int _weaponPartID, TeaForGodEmperor::WeaponPart const* _wp, bool _partOfItem, bool _partOfMainEquipment)
: weaponPartID(_weaponPartID)
, weaponPart(_wp)
, partOfItem(_partOfItem)
, partOfMainEquipment(_partOfMainEquipment)
{
	if (TeaForGodEmperor::TutorialSystem::check_active() && weaponPart.is_set())
	{
		tutorialHubId = weaponPart->get_tutorial_hub_id();
	}
}

TeaForGodEmperor::WeaponPart const* WeaponPartPilgrimInventory::get_weapon_part() const
{
	return weaponPart.get();
}
