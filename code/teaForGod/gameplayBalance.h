#pragma once

#include "teaForGod.h"

#include "game\energy.h"

#include "..\core\globalDefinitions.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class WeaponPart;

	namespace GameplayBalance
	{
		Energy health_for_melee_hit();
		Energy ammo_for_melee_hit();
		Energy health_for_melee_kill();
		Energy ammo_for_melee_kill();

		Energy permanent_upgrade_machine__base_energy();
		Energy permanent_upgrade_machine__cost_base();
		Energy permanent_upgrade_machine__cost_step();

		Energy permanent_upgrade_machine__xp_base();
		Energy permanent_upgrade_machine__xp_step();

		Energy upgrade_machine__xp();

		Energy automap__xp();

		Energy energy_balancer__xp();

		Energy engine_generator__xp();

		Energy persistent_normal_exm_cost();
		Energy persistent_permanent_exm_cost(int _idx);

		Energy pilgrimage_unlock_cost(int _unlockedSoFar);

	};
};
