#include "lhd_analysableItem.h"

#include "..\..\..\library\library.h"
#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace Loader;
using namespace HubDraggables;

//

REGISTER_FOR_FAST_CAST(AnalysableItem);

AnalysableItem::AnalysableItem(Framework::IModulesOwner* _object, Framework::TexturePart* _objectIcon)
: object(_object)
, objectIcon(_objectIcon)
{
}

AnalysableItem::AnalysableItem(TeaForGodEmperor::EXMType const* _exmType)
: exmType(_exmType)
{
}

AnalysableItem::AnalysableItem(int _weaponPartPilgrimInventoryID, TeaForGodEmperor::WeaponPart* _weaponPart)
: weaponPartPilgrimInventoryID(_weaponPartPilgrimInventoryID)
, weaponPart(_weaponPart)
{
}

