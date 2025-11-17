#include "gameUtils.h"

#include "..\module\moduleAI.h"
#include "..\modulesOwner\modulesOwner.h"

//

using namespace Framework;

//

bool GameUtils::is_player(Framework::IModulesOwner const* imo)
{
	if (imo)
	{
		if (auto* ai = imo->get_ai())
		{
			return ai->is_controlled_by_player() || ai->is_considered_player();
		}
	}
	return false;
}

bool GameUtils::is_local_player(Framework::IModulesOwner const* imo)
{
	return is_player(imo);
}

bool GameUtils::is_controlled_by_player(Framework::IModulesOwner const* imo)
{
	while (imo)
	{
		if (auto* ai = imo->get_ai())
		{
			if (ai->is_controlled_by_player() || ai->is_considered_player())
			{
				return true;
			}
		}
		auto* nextImo = imo->get_instigator();
		if (imo == nextImo)
		{
			break;
		}
		imo = nextImo;
	}
	return false;
}

bool GameUtils::is_controlled_by_local_player(Framework::IModulesOwner const* imo)
{
	return is_controlled_by_player(imo);
}
