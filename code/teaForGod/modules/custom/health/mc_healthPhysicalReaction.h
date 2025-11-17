#pragma once

#include "mc_health.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class HealthPhysicalReaction
		: public Health
		{
			FAST_CAST_DECLARE(HealthPhysicalReaction);
			FAST_CAST_BASE(Health);
			FAST_CAST_END();

			typedef Health base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			HealthPhysicalReaction(Framework::IModulesOwner* _owner);
			virtual ~HealthPhysicalReaction();

		protected: // Health
			override_ void on_deal_damage(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info); // unadjusted damage might be used for movement

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected:
			bool acceptAdjustedDamage = false;
			bool acceptUnadjustedDamage = false;
			Optional<Energy> reactAfterAdjustedDamage;
			Energy adjustedDamageAccumulated = Energy(0);

			// both should be not negative as we look for a particular range and we can choose direction on our own, z is exception because right now, at 2am I have no better idea
			Range affectVelocityRotationYawAfterAdjustedDamage = Range::zero;
			Range affectVelocityRotationYawAfterUnadjustedDamage = Range::zero;
			Range3 affectVelocityLinearAfterAdjustedDamage = Range3::zero;
			Range3 affectVelocityLinearAfterUnadjustedDamage = Range3::zero;
		};
	};
};

