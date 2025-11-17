#include "moduleDamageWhileTouching.h"

#include "..\custom/health/mc_health.h"

#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// module params
DEFINE_STATIC_NAME(damageIgnoresExtraEffects);
DEFINE_STATIC_NAME(damagePerSecond);
DEFINE_STATIC_NAME(damageInterval);
DEFINE_STATIC_NAME(damageArmourPiercing);
DEFINE_STATIC_NAME(damageType);
DEFINE_STATIC_NAME(damageExtraInfo);
DEFINE_STATIC_NAME(damageDelay);
DEFINE_STATIC_NAME(damageDelayIn);
DEFINE_STATIC_NAME(damageDelayOut);
DEFINE_STATIC_NAME(damageRequestCombatAuto);

//

REGISTER_FOR_FAST_CAST(ModuleDamageWhileTouching);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleDamageWhileTouching(_owner);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleDamageWhileTouching::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("damageWhileTouching")), create_module);
}

ModuleDamageWhileTouching::ModuleDamageWhileTouching(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

ModuleDamageWhileTouching::~ModuleDamageWhileTouching()
{
}

void ModuleDamageWhileTouching::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	damageIgnoresExtraEffects = _moduleData->get_parameter<bool>(this, NAME(damageIgnoresExtraEffects), damageIgnoresExtraEffects);
	damagePerSecond = Energy::get_from_module_data(this, _moduleData, NAME(damagePerSecond), damagePerSecond);
	damageArmourPiercing = EnergyCoef::get_from_module_data(this, _moduleData, NAME(damageArmourPiercing), damageArmourPiercing);
	{
		{
			auto const & dts = _moduleData->get_parameter<String>(this, NAME(damageType), String::empty());
			if (!dts.is_empty())
			{
				damageType = DamageType::parse(dts, damageType);
			}
		}
		{
			auto const & dts = _moduleData->get_parameter<Name>(this, NAME(damageType), Name::invalid());
			if (dts.is_valid())
			{
				damageType = DamageType::parse(dts.to_string(), damageType);
			}
		}
	}
	damageExtraInfo = _moduleData->get_parameter<Name>(this, NAME(damageExtraInfo), damageExtraInfo);
	damageInterval = _moduleData->get_parameter<float>(this, NAME(damageInterval), damageInterval);
	damageDelayIn = _moduleData->get_parameter<float>(this, NAME(damageDelay), damageDelayIn);
	damageDelayOut = _moduleData->get_parameter<float>(this, NAME(damageDelay), damageDelayOut);
	damageDelayIn = _moduleData->get_parameter<float>(this, NAME(damageDelayIn), damageDelayIn);
	damageDelayOut = _moduleData->get_parameter<float>(this, NAME(damageDelayOut), damageDelayOut);
	damageRequestCombatAuto = _moduleData->get_parameter<bool>(this, NAME(damageRequestCombatAuto), damageRequestCombatAuto);

	timeToDamage = damageDelayIn;
	shouldBeStillDamagingTimeLeft = damageDelayOut;
}

void ModuleDamageWhileTouching::reset()
{
	base::reset();

	timeToDamage = damageDelayIn;
	shouldBeStillDamagingTimeLeft = damageDelayOut;
}

void ModuleDamageWhileTouching::initialise()
{
	base::initialise();
}

void ModuleDamageWhileTouching::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);
	
	if (!damagePerSecond.is_positive() || damageType == DamageType::Unknown)
	{
		return;
	}

	shouldBeDamaging = true;
	if (auto* to = get_owner()->get_as_temporary_object())
	{
		if (to->is_desired_to_deactivate())
		{
			shouldBeDamaging = false;
		}
	}
	if (!shouldBeDamaging)
	{
		shouldBeStillDamagingTimeLeft -= _deltaTime;
		if (shouldBeStillDamagingTimeLeft < 0.0f)
		{
			return;
		}
	}
	else
	{
		shouldBeStillDamagingTimeLeft = damageDelayOut;
	}

	timeToDamage -= _deltaTime;
	float damagingTime = 0.0f;
	while (timeToDamage < 0.0f)
	{
		damagingTime += damageInterval != 0.0f ? damageInterval : _deltaTime;
		if (damageInterval != 0.0f)
		{
			timeToDamage += damageInterval;
		}
		else
		{
			timeToDamage = 0.0f;
		}
	}
	if (damagingTime > 0.0f)
	{
		do_damage(damagePerSecond.mul(damagingTime));
	}
}

void ModuleDamageWhileTouching::do_damage(Energy const& _damage)
{
	if (auto* imo = get_owner())
	{
		if (auto* c = imo->get_collision())
		{
			auto* imoti = imo->get_instigator_first_actor_or_valid_top_instigator();
			ArrayStatic<CustomModules::Health*, 16> hith;
			for_every(d, c->get_detected())
			{
				if (auto* ob = fast_cast<Framework::IModulesOwner>(d->ico))
				{
					if (auto* ti = ob->get_instigator_first_actor_or_valid_top_instigator())
					{
						if (imoti != ti) // we should not do damage to ourselves
						{
							if (auto* h = ti->get_custom<CustomModules::Health>())
							{
								hith.push_back_unique(h);
							}
						}
					}
				}
			}
			for_every_ptr(health, hith)
			{
				Damage dealDamage;

				dealDamage.damage = _damage;
				dealDamage.cost = _damage;

				dealDamage.damageType = damageType;
				dealDamage.damageExtraInfo = damageExtraInfo;
				dealDamage.armourPiercing = damageArmourPiercing;

				DamageInfo damageInfo;
				damageInfo.damager = imo->get_top_instigator();
				damageInfo.source = imo;
				damageInfo.instigator = damageInfo.damager;
				damageInfo.requestCombatAuto = damageRequestCombatAuto;

				ContinuousDamage dealContinuousDamage;
				dealContinuousDamage.setup_using_weapon_core_modifier_companion_for(dealDamage);

				if (!damageIgnoresExtraEffects)
				{
					health->adjust_damage_on_hit_with_extra_effects(REF_ dealDamage);
				}

				if (!dealDamage.damage.is_zero())
				{
					{
						auto* ep = health->get_owner()->get_presence();
						auto* presence = imo->get_presence();

						if (ep->get_in_room() == presence->get_in_room())
						{
							damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(presence->get_centre_of_presence_WS());
						}
						else
						{
							Framework::RelativeToPresencePlacement rpp;
							rpp.be_temporary_snapshot();
							if (rpp.find_path(imo, presence->get_in_room(), presence->get_centre_of_presence_transform_WS()))
							{
								damageInfo.hitRelativeLocation = ep->get_placement().location_to_local(rpp.get_placement_in_owner_room().get_translation());
							}
						}
					}
					health->deal_damage(dealDamage, dealDamage, damageInfo);
					if (dealContinuousDamage.is_valid())
					{
						health->add_continuous_damage(dealContinuousDamage, damageInfo);
					}
				}
			}
		}
	}
}

