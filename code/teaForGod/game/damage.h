#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\optional.h"

#include "..\..\framework\modulesOwner\modulesOwner.h"

#include "..\teaEnums.h"
#include "..\game\energy.h"
#include "..\library\physicalMaterial.h"

namespace Framework
{
	class Module;
	class ModuleData;
};

namespace TeaForGodEmperor
{
	class PhysicalMaterial;

	struct ApplyDamageContext
	{
		int randSeed = 0;
		bool canRicochet = false;

		ApplyDamageContext& can_ricochet(bool _canRicochet = true) { canRicochet = _canRicochet; return *this; }

		// if was ricocheted by the material
		bool out_ricocheted = false;

	};

	struct Damage
	{
		static Damage zero;

		Energy damage = Energy(1);
		Optional<Energy> cost; // used for healing
		DamageType::Type damageType = DamageType::Unknown;
		Name damageExtraInfo; // extra info about damage
		EnergyCoef selfDamageCoef = EnergyCoef::one(); // this is any damage you do to yourself (shooting too)
		bool instantDeath = false;
		bool meleeDamage = false;
		bool explosionDamage = false;
		bool damageOnlyToHitObject = false; // otherwise it will get instigator with health
		float affectVelocity = 1.0f;
		float affectVelocityDead = 1.0f; // if was not killed
		float forceAffectVelocityLinearPerDamagePoint = 0.0f; // force affect (from guns etc, ignores affect velocity limits
		float induceConfussion = 0.0f; // constant, will use max value

		// set on use, not loadable

		bool continousDamage = false; // this is not when shooting yourself but when being damaged due to damaging someone
		bool selfDamage = false; // this is not when shooting yourself but when being damaged due to damaging someone

		Optional<bool> ricocheting; // overrides materials

		EnergyCoef armourPiercing = EnergyCoef::zero(); // 0 to 1

		void setup_with(Framework::Module * _module, Framework::ModuleData const * _moduleData);
		void apply(PhysicalMaterial::ForProjectile const * _forProjectile, ApplyDamageContext* _adc);
		void apply_post_impact(PhysicalMaterial::ForProjectile const * _forProjectile, ApplyDamageContext* _adc); // if survived impact, we may decide to lower the damage etc
		Damage with_damage_adjusted_plus_one(EnergyCoef const& _coef) const { Damage dCopy = *this; dCopy.damage = dCopy.damage.adjusted_plus_one(_coef); return dCopy; }
	};

	struct ContinuousDamage
	{
		static ContinuousDamage zero;

		DamageType::Type damageType = DamageType::Unknown;
		Name damageExtraInfo; // extra info about damage
		Energy damage = Energy::zero(); // may be zero, will work as a buff/state
		EnergyCoef selfDamageCoef = EnergyCoef::one();
		float time = 0.0f;
		float minTime = 0.0f;
		bool meleeDamage = false;
		bool explosionDamage = false;

		// set on use, not loadable

		EnergyCoef armourPiercing = EnergyCoef::zero(); // 0 to 1

		bool is_valid() const { return time != 0.0f || minTime != 0.0f; }
		void setup_with(Framework::Module* _module, Damage* _damage /* may be null */, Framework::ModuleData const* _moduleData);
		void setup_using_weapon_core_modifier_companion_for(Damage const & _damage); // setups accompanying continuous damage (if we're not using projectile/explosion)
		void apply(PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc);
		void apply_post_impact(PhysicalMaterial::ForProjectile const* _forProjectile, ApplyDamageContext* _adc); // if survived impact, we may decide to lower the damage etc
		ContinuousDamage adjusted(float _percentage) const { ContinuousDamage cdCopy = *this; cdCopy.damage = damage.mul(_percentage); cdCopy.time *= _percentage; return cdCopy; }
		void set_on_use_from(Damage const& _damage) { armourPiercing = _damage.armourPiercing; }

	};

	struct DamageInfo
	{
		SafePtr<Framework::IModulesOwner> damager; // object that damaged us, eg. bullet, sword
		SafePtr<Framework::IModulesOwner> source; // source object, eg. gun, sword
		SafePtr<Framework::IModulesOwner> instigator; // instigator that was behind this (person holding a gun)
		// this is mostly for hit indicator but is also used for affecting velocity, dir is most important, location less, normal is last resort
		Optional<Vector3> hitRelativeDir; // dir in which hit was going
		Optional<Vector3> hitRelativeLocation;
		Optional<Vector3> hitRelativeNormal;
		// this is useful when dealing with health part
		Optional<Name> hitBone;
		PhysicalMaterial const * hitPhysicalMaterial = nullptr; // this should exist even if the owner doesn't

		bool spawnParticles = true;
		bool peacefulDamage = false; // if it is not actual damage but just energy draining, this way we should not explode on death etc.
		bool forceDamage = false; // ignore immortality, invincible etc.
		bool cantKillPilgrim = false;
		bool requestCombatAuto = true;
	};
};

DECLARE_REGISTERED_TYPE(TeaForGodEmperor::Damage);
DECLARE_REGISTERED_TYPE(TeaForGodEmperor::DamageInfo);
