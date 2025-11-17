#include "damage.h"

#include "..\game\gameSettings.h"
#include "..\library\gameDefinition.h"
#include "..\library\weaponCoreModifiers.h"

#include "..\..\framework\module\module.h"
#include "..\..\framework\module\moduleData.h"
#include "..\..\framework\module\moduleDataImpl.inl"

#include "..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define INVESTIGATE_USE_FOR_PROJECTILE

//

using namespace TeaForGodEmperor;

//

// params
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageExtraInfo);
DEFINE_STATIC_NAME(damageType);
DEFINE_STATIC_NAME(selfDamageCoef);
DEFINE_STATIC_NAME(selfContinuousDamageCoef);
DEFINE_STATIC_NAME(meleeDamage);
DEFINE_STATIC_NAME(explosionDamage);
DEFINE_STATIC_NAME(damageOnlyToHitObject);
DEFINE_STATIC_NAME(damageAffectVelocity);
DEFINE_STATIC_NAME(damageAffectVelocityDead);
DEFINE_STATIC_NAME(armourPiercing);
//
DEFINE_STATIC_NAME(continuousDamage);
DEFINE_STATIC_NAME(continuousDamageTime);
DEFINE_STATIC_NAME(continuousDamageMinTime);
DEFINE_STATIC_NAME(continuousDamageType);
DEFINE_STATIC_NAME(continuousMeleeDamage);
DEFINE_STATIC_NAME(continuousExplosionDamage);

//

Damage Damage::zero;

static void apply_adjustments(Energy& damage, EnergyCoef const & armourPiercing, PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc)
{
#ifdef INVESTIGATE_USE_FOR_PROJECTILE
	output(TXT("apply_adjustments"));
#endif

	if (!_forProjectile)
	{
#ifdef INVESTIGATE_USE_FOR_PROJECTILE
		output(TXT("  nothing to apply"));
#endif
		return;
	}

	if (!GameSettings::get().difficulty.armourIneffective)
	{
		if (!_forProjectile->ricochetAmount.is_zero() && _adc && _adc->canRicochet)
		{
			bool ricochetNow = true;
			{
				float ricochetChance = clamp(_forProjectile->ricochetChance.get(1.0f), 0.0f, 1.0f);
				{
					if (_forProjectile->ricochetChanceDamageBased.is_set())
					{
						ricochetChance *= clamp(damage.as_float() * _forProjectile->ricochetChanceDamageBased.get(), 0.0f, 1.0f);
					}
				}
				if (ricochetChance < 1.0f)
				{
					Random::Generator rg;
					rg.set_seed(_adc->randSeed, ::System::Core::get_frame());
					ricochetNow = rg.get_chance(ricochetChance);
				}
			}
			if (ricochetNow)
			{
				EnergyCoef useArmourPiercingForRicochet = _forProjectile->forceRicochet ? EnergyCoef::zero() : armourPiercing;
				damage = damage.adjusted_clamped(EnergyCoef::one() - _forProjectile->ricochetAmount.adjusted_clamped(EnergyCoef::one() - useArmourPiercingForRicochet));
				if (_adc && useArmourPiercingForRicochet < EnergyCoef::one())
				{
					// something has ricocheted
					_adc->out_ricocheted = true;
				}
			}
		}
	}

	damage = damage.adjusted(_forProjectile->adjustDamage);

	if (!GameSettings::get().difficulty.armourIneffective)
	{
		EnergyCoef adjustArmourDamage = _forProjectile->adjustArmourDamage;
		adjustArmourDamage = max(adjustArmourDamage, EnergyCoef(clamp(1.0f - GameSettings::get().difficulty.armourEffectivenessLimit, 0.0f, 1.0f)));
		if (adjustArmourDamage < EnergyCoef::one())
		{
			if (!armourPiercing.is_zero())
			{
				EnergyCoef effectiveCoef = EnergyCoef::one() - adjustArmourDamage;
				effectiveCoef = effectiveCoef * (EnergyCoef::one() - armourPiercing);
				damage = damage.adjusted(EnergyCoef::one() - effectiveCoef);
			}
			else
			{
				damage = damage.adjusted(adjustArmourDamage);
			}
		}
	}
}

static void apply_post_impact_adjustments(Energy& damage, EnergyCoef const& armourPiercing, PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc)
{
	if (!_forProjectile)
	{
		return;
	}

	if (GameSettings::get().difficulty.armourIneffective)
	{
		// do not adjust damage
		return;
	}

	if (!_forProjectile->ricochetAmount.is_zero() && _adc && _adc->canRicochet) // canRicochet means it is now ricocheting
	{
		EnergyCoef useArmourPiercingForRicochet = _forProjectile->forceRicochet ? EnergyCoef::zero() : armourPiercing;
		damage = damage.adjusted_clamped(_forProjectile->ricochetAmount.adjusted_clamped(EnergyCoef::one() - useArmourPiercingForRicochet)).adjusted_clamped(_forProjectile->ricochetDamage);
	}
}

void Damage::setup_with(Framework::Module * _module, Framework::ModuleData const * _moduleData)
{
	damage = Energy::get_from_module_data(_module, _moduleData, NAME(damage), damage);
	{
		{
			String damageTypeString = _moduleData->get_parameter<String>(_module, NAME(damageType));
			if (!damageTypeString.is_empty())
			{
				damageType = DamageType::parse(damageTypeString, damageType);
			}
		}
		{
			Name damageTypeName = _moduleData->get_parameter<Name>(_module, NAME(damageType));
			if (damageTypeName.is_valid())
			{
				damageType = DamageType::parse(damageTypeName.to_string(), damageType);
			}
		}
	}
	damageExtraInfo = _moduleData->get_parameter<Name>(_module, NAME(damageExtraInfo), damageExtraInfo);
	selfDamageCoef = EnergyCoef::get_from_module_data(_module, _moduleData, NAME(selfDamageCoef), selfDamageCoef);

	// types
	meleeDamage = _moduleData->get_parameter<bool>(_module, NAME(meleeDamage), meleeDamage);
	explosionDamage = _moduleData->get_parameter<bool>(_module, NAME(explosionDamage), explosionDamage);

	armourPiercing = EnergyCoef::get_from_module_data(_module, _moduleData, NAME(armourPiercing), armourPiercing);

	damageOnlyToHitObject = _moduleData->get_parameter<bool>(_module, NAME(damageOnlyToHitObject), damageOnlyToHitObject);
	affectVelocity = _moduleData->get_parameter<float>(_module, NAME(damageAffectVelocity), affectVelocity);
	affectVelocityDead = _moduleData->get_parameter<float>(_module, NAME(damageAffectVelocity), affectVelocityDead);
	affectVelocityDead = _moduleData->get_parameter<float>(_module, NAME(damageAffectVelocityDead), affectVelocityDead);
}

void Damage::apply(PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc)
{
	if (_adc && ricocheting.is_set())
	{
		_adc->canRicochet = ricocheting.get();
	}
	apply_adjustments(damage, armourPiercing, _forProjectile, _adc);
	if (_adc && ricocheting.is_set())
	{
		_adc->out_ricocheted = ricocheting.get();
	}
}

void Damage::apply_post_impact(PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc)
{
	apply_post_impact_adjustments(damage, armourPiercing, _forProjectile, _adc);
}

//

ContinuousDamage ContinuousDamage::zero;

void ContinuousDamage::setup_with(Framework::Module * _module, Damage * _damage, Framework::ModuleData const * _moduleData)
{
	// use either selfDamageCoef or selfContinuousDamageCoef
	selfDamageCoef = EnergyCoef::get_from_module_data(_module, _moduleData, NAME(selfDamageCoef), selfDamageCoef);
	selfDamageCoef = EnergyCoef::get_from_module_data(_module, _moduleData, NAME(selfDamageCoef), selfDamageCoef);

	// types, doubles with normal damage
	meleeDamage = _moduleData->get_parameter<bool>(_module, NAME(meleeDamage), meleeDamage);
	meleeDamage = _moduleData->get_parameter<bool>(_module, NAME(continuousMeleeDamage), meleeDamage);
	explosionDamage = _moduleData->get_parameter<bool>(_module, NAME(explosionDamage), explosionDamage);
	explosionDamage = _moduleData->get_parameter<bool>(_module, NAME(continuousExplosionDamage), explosionDamage);

	{
		{
			String damageTypeString = _moduleData->get_parameter<String>(_module, NAME(damageType));
			if (!damageTypeString.is_empty())
			{
				damageType = DamageType::parse(damageTypeString, damageType);
			}
		}
		{
			Name damageTypeName = _moduleData->get_parameter<Name>(_module, NAME(damageType));
			if (damageTypeName.is_valid())
			{
				damageType = DamageType::parse(damageTypeName.to_string(), damageType);
			}
		}
	}
	{
		{
			String damageTypeString = _moduleData->get_parameter<String>(_module, NAME(continuousDamageType));
			if (!damageTypeString.is_empty())
			{
				damageType = DamageType::parse(damageTypeString, damageType);
			}
		}
		{
			Name damageTypeName = _moduleData->get_parameter<Name>(_module, NAME(continuousDamageType));
			if (damageTypeName.is_valid())
			{
				damageType = DamageType::parse(damageTypeName.to_string(), damageType);
			}
		}
	}

	damageExtraInfo = _moduleData->get_parameter<Name>(_module, NAME(damageExtraInfo), damageExtraInfo);

	// read only continuousDamage! unless not provided, use weapon core modifiers then
	if (Energy::can_get_from_module_data(_module, _moduleData, NAME(continuousDamage)))
	{
		damage = Energy::get_from_module_data(_module, _moduleData, NAME(continuousDamage), damage);
	}
	else if (_damage)
	{
		if (auto* gd = GameDefinition::get_chosen())
		{
			if (auto* weaponCoreModifiers = gd->get_weapon_core_modifiers())
			{
				if (auto* fc = weaponCoreModifiers->get_for_core(WeaponCoreKind::from_damage_type(damageType)))
				{
					if (fc->continuousDamageTime.value.is_set())
					{
						time = fc->continuousDamageTime.value.get();
					}
					if (fc->continuousDamageMinTime.value.is_set())
					{
						minTime = fc->continuousDamageMinTime.value.get();
					}
				}
			}
		}
	}
				
	time = _moduleData->get_parameter<float>(_module, NAME(continuousDamageTime), time);
	minTime = _moduleData->get_parameter<float>(_module, NAME(continuousDamageMinTime), minTime);
}

void ContinuousDamage::setup_using_weapon_core_modifier_companion_for(Damage const& _damage)
{
	damageType = DamageType::Unknown;
	if (auto* gd = GameDefinition::get_chosen())
	{
		if (auto* weaponCoreModifiers = gd->get_weapon_core_modifiers())
		{
			if (auto* fc = weaponCoreModifiers->get_for_core(WeaponCoreKind::from_damage_type(_damage.damageType)))
			{
				if (fc->continuousDamageTime.value.is_set())
				{
					time = fc->continuousDamageTime.value.get();
				}
				if (fc->continuousDamageMinTime.value.is_set())
				{
					minTime = fc->continuousDamageMinTime.value.get();
				}

				if (is_valid()) // valid as continuous, only then change damage type to be the actual continuous damage type
				{
					damageType = _damage.damageType;
				}
			}
		}
	}
}

void ContinuousDamage::apply(PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc)
{
	apply_adjustments(damage, armourPiercing, _forProjectile, _adc);
}

void ContinuousDamage::apply_post_impact(PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc)
{
	apply_post_impact_adjustments(damage, armourPiercing, _forProjectile, _adc);
}

//
