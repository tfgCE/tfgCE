#pragma once

#include "..\..\..\..\core\fastCast.h"

namespace TeaForGodEmperor
{
	class WeaponPart;
};

namespace Loader
{
	namespace HubInterfaces
	{
		interface_class IWeaponPartProvider
		{
			FAST_CAST_DECLARE(IWeaponPartProvider);
			FAST_CAST_END();

		public:
			virtual ~IWeaponPartProvider() {}

			virtual TeaForGodEmperor::WeaponPart const* get_weapon_part() const = 0;
		};

	};
};
