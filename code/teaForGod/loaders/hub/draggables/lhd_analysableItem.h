#pragma once

#include "..\loaderHubDraggableData.h"

#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"

namespace Framework
{
	class TexturePart;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class WeaponPart;
};

namespace Loader
{
	namespace HubDraggables
	{
		class AnalysableItem
		: public Loader::IHubDraggableData
		{
			FAST_CAST_DECLARE(AnalysableItem);
			FAST_CAST_BASE(Loader::IHubDraggableData);
			FAST_CAST_END();
		public:
			AnalysableItem(Framework::IModulesOwner* _object, Framework::TexturePart * _objectIcon);
			AnalysableItem(TeaForGodEmperor::EXMType const* _exmType);
			AnalysableItem(int _weaponPartPilgrimInventoryID, TeaForGodEmperor::WeaponPart * _weaponPart);

			Framework::IModulesOwner* get_object() const { return object.get(); }
			Framework::TexturePart* get_object_icon() const { return object.is_set() ? objectIcon.get() : nullptr; }
			//
			TeaForGodEmperor::EXMType const * get_exm_type() const { return exmType; }
			//
			int get_weapon_part_pilgrim_inventory_id() const { return weaponPartPilgrimInventoryID; }
			TeaForGodEmperor::WeaponPart * get_weapon_part() const { return weaponPart.get(); }

		private:
			SafePtr<Framework::IModulesOwner> object;
			Framework::UsedLibraryStored<Framework::TexturePart> objectIcon;
			//
			TeaForGodEmperor::EXMType const* exmType = nullptr;
			//
			int weaponPartPilgrimInventoryID = NONE;
			RefCountObjectPtr<TeaForGodEmperor::WeaponPart> weaponPart;
		};
	};
};

