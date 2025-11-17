#pragma once

#include "..\loaderHubDraggableData.h"
#include "..\interfaces\lhi_weaponPartProvider.h"

namespace TeaForGodEmperor
{
	class WeaponPart;
};

namespace Loader
{
	namespace HubDraggables
	{
		class WeaponPartPilgrimInventory
		: public Loader::IHubDraggableData
		, public HubInterfaces::IWeaponPartProvider
		{
			FAST_CAST_DECLARE(WeaponPartPilgrimInventory);
			FAST_CAST_BASE(Loader::IHubDraggableData);
			FAST_CAST_BASE(HubInterfaces::IWeaponPartProvider);
			FAST_CAST_END();
		public:
			WeaponPartPilgrimInventory(int _weaponPartID = NONE, TeaForGodEmperor::WeaponPart const* _wp = nullptr, bool _partOfItem = false, bool _partOfMainEquipment = false);

			void set_transfer_chance(int _transferChance) { transferChance = _transferChance; }
			int get_transfer_chance() const { return clamp(transferChance, 0, 100); }

			int get_weapon_part_id() const { return weaponPartID; }
			bool is_part_of_item() const { return partOfItem; }
			bool is_part_of_main_equipment() const { return partOfMainEquipment; }

		public: // IWeaponPartProvider
			implement_ TeaForGodEmperor::WeaponPart const* get_weapon_part() const;
		private:
			int weaponPartID; // inside inventory
			RefCountObjectPtr<TeaForGodEmperor::WeaponPart> weaponPart;
			bool partOfItem = false;
			bool partOfMainEquipment = false;
			int transferChance = 100;
		};
	};
};

