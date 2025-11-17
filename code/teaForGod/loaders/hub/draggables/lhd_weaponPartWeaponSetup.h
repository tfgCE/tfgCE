#pragma once

#include "..\loaderHubDraggableData.h"
#include "..\interfaces\lhi_weaponPartProvider.h"

#include "..\..\..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	class WeaponPart;
};

namespace Loader
{
	namespace HubDraggables
	{
		class WeaponPartWeaponSetup
		: public Loader::IHubDraggableData
		, public HubInterfaces::IWeaponPartProvider
		{
			FAST_CAST_DECLARE(WeaponPartWeaponSetup);
			FAST_CAST_BASE(Loader::IHubDraggableData);
			FAST_CAST_BASE(HubInterfaces::IWeaponPartProvider);
			FAST_CAST_END();
		public:
			WeaponPartWeaponSetup(TeaForGodEmperor::WeaponPart const* _wp = nullptr);

			void clear_weapon_part_id_while_dragging() { weaponPartIDWhileDragged.clear(); }
			void set_weapon_part_id_while_dragging(int _id) { weaponPartIDWhileDragged = _id; }
			bool has_weapon_part_id_while_dragging() const { return weaponPartIDWhileDragged.is_set(); }
			int get_weapon_part_id_while_dragging() const { return weaponPartIDWhileDragged.get(); }

		public: // IWeaponPartProvider
			implement_ TeaForGodEmperor::WeaponPart const* get_weapon_part() const;
		private:
			Optional<int> weaponPartIDWhileDragged; // this should be only set while dragging
			RefCountObjectPtr<TeaForGodEmperor::WeaponPart> weaponPart;
		};
	};
};

