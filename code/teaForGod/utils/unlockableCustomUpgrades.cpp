#include "unlockableCustomUpgrades.h"

#include "..\game\playerSetup.h"
#include "..\library\library.h"
#include "..\library\gameDefinition.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

void UnlockableCustomUpgrades::update()
{
	availableCustomUpgrades.clear();

	bool missionsActive = false;
	if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
	{
		if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
		{
			missionsActive = true;
		}
	}

	if (!missionsActive)
	{
		return;
	}

	Array< CustomUpgradeType const*> all;

	CustomUpgradeType::get_all(OUT_ all);

	// we want context here as we're aiming for unlockable custom upgrades, for the bridge shop or unlockable menu
	UnlockableCustomUpgradesContext context;
	AvailableMission::get_available_missions(OUT_ context.availableMissions);

	for_every_ptr(cu, all)
	{
		if (cu->is_available_to_unlock(&context))
		{
			availableCustomUpgrades.push_back(Available(cu));
		}
	}

	sort(availableCustomUpgrades);
}

//--

int UnlockableCustomUpgrades::Available::compare(void const* _a, void const* _b)
{
	UnlockableCustomUpgrades::Available const * a = plain_cast<UnlockableCustomUpgrades::Available>(_a);
	UnlockableCustomUpgrades::Available const * b = plain_cast<UnlockableCustomUpgrades::Available>(_b);
	if (a->upgrade && b->upgrade)
	{
		int aOrder = a->upgrade->get_order();
		int bOrder = b->upgrade->get_order();
		if (aOrder > bOrder) return A_BEFORE_B;
		if (aOrder < bOrder) return B_BEFORE_A;
		return Framework::LibraryName::compare(&a->upgrade->get_name(), &b->upgrade->get_name());
	}
	return A_AS_B;
}
