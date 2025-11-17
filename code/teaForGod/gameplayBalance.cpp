#include "gameplayBalance.h"

#include "library\gameDefinition.h"

//

using namespace TeaForGodEmperor;

//

Energy GameplayBalance::health_for_melee_hit()
{
	return Energy(1);
}

Energy GameplayBalance::ammo_for_melee_hit()
{
	return Energy(1);
}

Energy GameplayBalance::health_for_melee_kill()
{
	return Energy(0);
}

Energy GameplayBalance::ammo_for_melee_kill()
{
	return Energy(0);
}

Energy GameplayBalance::permanent_upgrade_machine__base_energy()
{
	return Energy(50);
}

Energy GameplayBalance::permanent_upgrade_machine__cost_base()
{
	return Energy(100);
}

Energy GameplayBalance::permanent_upgrade_machine__cost_step()
{
	return Energy(50);
}

Energy GameplayBalance::permanent_upgrade_machine__xp_base()
{
	return Energy(25);
}

Energy GameplayBalance::permanent_upgrade_machine__xp_step()
{
	return Energy(25);
}

Energy GameplayBalance::upgrade_machine__xp()
{
	return Energy(25);
}

Energy GameplayBalance::automap__xp()
{
	return Energy(10);
}

Energy GameplayBalance::energy_balancer__xp()
{
	return Energy(5);
}

Energy GameplayBalance::engine_generator__xp()
{
	return Energy(2);
}

Energy GameplayBalance::persistent_normal_exm_cost()
{
	return Energy(250);
}

Energy GameplayBalance::persistent_permanent_exm_cost(int _idx)
{
	if (auto* gd = GameDefinition::get_chosen())
	{
		if (gd->get_type() == GameDefinition::Type::ComplexRules)
		{
			return Energy(min(250 + 250 * _idx, 5000)); // start lower for complex rules as we have other things to unlock too
		}
	}
	return Energy(min(500 + 250 * _idx, 5000));
}

Energy GameplayBalance::pilgrimage_unlock_cost(int _unlockedSoFar)
{
	return Energy(min(1000 + 500 * _unlockedSoFar, 5000));
}
