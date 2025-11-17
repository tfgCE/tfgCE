#pragma once

#include "..\game\persistence.h"
#include "..\library\missionDefinition.h"

namespace TeaForGodEmperor
{
	class CustomUpgradeType;
	struct UnlockableCustomUpgradesContext
	{
		Array<AvailableMission> availableMissions;
	};
	struct UnlockableCustomUpgrades
	{
		struct Available
		{
			CustomUpgradeType const * upgrade;

			Available() {}
			explicit Available(CustomUpgradeType const* u) : upgrade(u) {}

			static int compare(void const* _a, void const* _b);
		};
		Array<Available> availableCustomUpgrades;

		void update();
	};
};

