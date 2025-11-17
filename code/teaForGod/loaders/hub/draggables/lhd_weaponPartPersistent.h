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
		class WeaponPartPersistent
		: public Loader::IHubDraggableData
		, public HubInterfaces::IWeaponPartProvider
		{
			FAST_CAST_DECLARE(WeaponPartPersistent);
			FAST_CAST_BASE(Loader::IHubDraggableData);
			FAST_CAST_BASE(HubInterfaces::IWeaponPartProvider);
			FAST_CAST_END();
		public:
			WeaponPartPersistent(TeaForGodEmperor::WeaponPart const* _wp = nullptr);
		public: // IWeaponPartProvider
			implement_ TeaForGodEmperor::WeaponPart const* get_weapon_part() const;
		private:
			RefCountObjectPtr<TeaForGodEmperor::WeaponPart> weaponPart;
		};
	};
};

