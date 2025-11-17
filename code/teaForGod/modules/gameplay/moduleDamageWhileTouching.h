#pragma once

#include "..\..\teaEnums.h"
#include "..\..\game\energy.h"

#include "..\..\..\core\concurrency\mrswLock.h"
#include "..\..\..\core\types\hand.h"

#include "..\moduleGameplay.h"

namespace Framework
{
	struct PresencePath;
	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;
	
	/**
	 *	If its health reaches zero (dies), the leak bursts open and increases a variable in owner (where it is attached to)
	 */
	class ModuleDamageWhileTouching
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleDamageWhileTouching);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleDamageWhileTouching(Framework::IModulesOwner* _owner);
		virtual ~ModuleDamageWhileTouching();

	protected: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const * _moduleData);

		override_ void initialise();

	protected: // ModuleGameplay
		override_ void advance_post_move(float _deltaTime);

	protected:
		float timeToDamage = 0.0f;
		bool shouldBeDamaging = true;
		float shouldBeStillDamagingTimeLeft = 0.0f;

		bool damageIgnoresExtraEffects = false;
		Energy damagePerSecond = Energy::zero();
		float damageInterval = 0.1f;
		float damageDelayIn = 0.0f;
		float damageDelayOut = 0.0f;
		bool damageRequestCombatAuto = false; // by default this should not trigger combat music
		DamageType::Type damageType = DamageType::Unknown;
		Name damageExtraInfo;
		EnergyCoef damageArmourPiercing = EnergyCoef::one(); // by default we just do damage to everything

		void do_damage(Energy const& _damage);
	};
};

