#include "pilgrimStatsInfo.h"

#include "..\teaForGod.h"

#include "..\game\gameSettings.h"

#include "..\library\exmType.h"
#include "..\library\library.h"

#include "..\modules\custom\health\mc_health.h"
#include "..\modules\custom\health\mc_healthRegen.h"
#include "..\modules\gameplay\moduleEXM.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\gameplay\moduleProjectile.h"
#include "..\modules\gameplay\equipment\me_gun.h"

#include "..\pilgrimage\pilgrimageInstance.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME_STR(lsArmourPiercing, TXT("weapon part stats; armour piercing"));
DEFINE_STATIC_NAME_STR(lsAntiDeflection, TXT("weapon part stats; anti deflection"));
	DEFINE_STATIC_NAME(coef);

//

static void add_info(String* _to, String const& _add)
{
	if (_to)
	{
		if (!_to->is_empty())
		{
			*_to += TXT(", ");
		}
		*_to += _add;
	}
}

// diff related functions, thrown all together
#define UPDATE_DIFF(item) \
	if (item > _prev.item) _diff.item = _toValue; else \
	if (item < _prev.item) _diff.item = -_toValue;

#define UPDATE_DIFF_INV(item) \
	if (item < _prev.item) _diff.item = _toValue; else \
	if (item > _prev.item) _diff.item = -_toValue;

#define UPDATE_CHANGE(item) \
	if (item != _prev.item) _diff.item = 1.0f;

#define UPDATE_CHANGE_NEG(item, negativeValue) \
	if (item != _prev.item) _diff.item = item == negativeValue? -1.0f : 1.0f;

#define UPDATE_ADDITIONAL_INFO() \
	if (_prev.additionalInfo != additionalInfo) _diff.additionalInfo = 1.0f;

void PilgrimStatsInfoHand::update_diff(PilgrimStatsInfoHand const& _prev, PilgrimStatsInfoDiffHand& _diff, float _toValue) const
{
	UPDATE_DIFF(pullEnergyDistanceRecentlyRenderedRoom);
	UPDATE_DIFF(pullEnergyDistanceRecentlyNotRenderedRoom);
	UPDATE_DIFF(energyInhaleCoef);

	UPDATE_DIFF(handEnergyStorage);

	UPDATE_CHANGE_NEG(mainEquipment.coreKind, WeaponCoreKind::None);
	if (mainEquipment.isValid && _prev.mainEquipment.isValid)
	{
		UPDATE_DIFF(mainEquipment.shotCount);
		UPDATE_DIFF(mainEquipment.totalDamage);
		UPDATE_DIFF(mainEquipment.totalEnergy);
		UPDATE_DIFF(mainEquipment.storageShotCount);
		UPDATE_DIFF(mainEquipment.storageTotalDamage);
		UPDATE_DIFF(mainEquipment.storageOutputSpeed);
		UPDATE_DIFF(mainEquipment.storageRPM);
		UPDATE_DIFF(mainEquipment.storageDPS);
		UPDATE_DIFF(mainEquipment.magazineSize);
		UPDATE_DIFF_INV(mainEquipment.magazineCooldown);
		UPDATE_DIFF(mainEquipment.magazineShotCount);
		UPDATE_DIFF(mainEquipment.magazineTotalDamage);
		UPDATE_DIFF(mainEquipment.magazineOutputSpeed);
		UPDATE_DIFF(mainEquipment.magazineRPM);
		UPDATE_DIFF(mainEquipment.magazineDPS);
		UPDATE_DIFF(mainEquipment.chamberSize);
		UPDATE_DIFF_INV(mainEquipment.shotCost);
		UPDATE_DIFF(mainEquipment.damage);
		UPDATE_DIFF(mainEquipment.immediateDamage);
		UPDATE_DIFF(mainEquipment.continuousDamage);
#ifndef ARMOUR_DEFLECTION_ETC_INTO_ADDITIONAL_STATS
		UPDATE_DIFF(mainEquipment.armourPiercing);
		UPDATE_DIFF(mainEquipment.antiDeflection);
#endif
		if (mainEquipment.projectileSpeed != _prev.mainEquipment.projectileSpeed)
		{
			output(TXT("changing %.6f to %.6f"), _prev.mainEquipment.projectileSpeed, mainEquipment.projectileSpeed);
		}
		UPDATE_DIFF(mainEquipment.projectileSpeed);
		UPDATE_DIFF_INV(mainEquipment.projectileSpread);
		UPDATE_CHANGE(mainEquipment.projectilesPerShot);
	}
	else
	{
		float keepCore = _diff.mainEquipment.coreKind;
		_diff.mainEquipment = PilgrimStatsInfoDiffHand::MainEquipment();
		_diff.mainEquipment.coreKind = keepCore;
	}

	UPDATE_DIFF_INV(physicalViolence.minSpeed);
	UPDATE_DIFF_INV(physicalViolence.minSpeedStrong);
	UPDATE_DIFF(physicalViolence.damage);
	UPDATE_DIFF(physicalViolence.damageStrong);

	UPDATE_ADDITIONAL_INFO();
}

void PilgrimStatsInfo::update_diff(PilgrimStatsInfo const& _prev, PilgrimStatsInfoDiff& _diff, float _toValue) const
{
	for_count(int, hIdx, Hand::MAX)
	{
		hands[hIdx].update_diff(_prev.hands[hIdx], _diff.hands[hIdx], _toValue);
	}

	UPDATE_DIFF(health);
	UPDATE_DIFF(healthBackup);
	UPDATE_DIFF(healthRegenRate);
	UPDATE_DIFF_INV(healthRegenCooldown);
	UPDATE_DIFF_INV(healthDamageReceivedCoef);

	UPDATE_ADDITIONAL_INFO();
}

#define ADVANCE(item) \
	item = blend_to_using_speed(item, 0.0f, _speed, _time);

void PilgrimStatsInfoDiffHand::advance(float _time, float _speed)
{
	ADVANCE(pullEnergyDistanceRecentlyRenderedRoom);
	ADVANCE(pullEnergyDistanceRecentlyNotRenderedRoom);
	ADVANCE(energyInhaleCoef);
	ADVANCE(handEnergyStorage);

	ADVANCE(mainEquipment.coreKind);
	ADVANCE(mainEquipment.shotCount);
	ADVANCE(mainEquipment.totalDamage);
	ADVANCE(mainEquipment.totalEnergy);
	ADVANCE(mainEquipment.storageShotCount);
	ADVANCE(mainEquipment.storageTotalDamage);
	ADVANCE(mainEquipment.storageOutputSpeed);
	ADVANCE(mainEquipment.storageRPM);
	ADVANCE(mainEquipment.storageDPS);
	ADVANCE(mainEquipment.magazineSize);
	ADVANCE(mainEquipment.magazineCooldown);
	ADVANCE(mainEquipment.magazineShotCount);
	ADVANCE(mainEquipment.magazineTotalDamage);
	ADVANCE(mainEquipment.magazineOutputSpeed);
	ADVANCE(mainEquipment.magazineRPM);
	ADVANCE(mainEquipment.magazineDPS);
	ADVANCE(mainEquipment.chamberSize);
	ADVANCE(mainEquipment.shotCost);
	ADVANCE(mainEquipment.damage);
	ADVANCE(mainEquipment.immediateDamage);
	ADVANCE(mainEquipment.continuousDamage);
#ifndef ARMOUR_DEFLECTION_ETC_INTO_ADDITIONAL_STATS
	ADVANCE(mainEquipment.armourPiercing);
	ADVANCE(mainEquipment.antiDeflection);
#endif
	ADVANCE(mainEquipment.projectileSpeed);
	ADVANCE(mainEquipment.projectileSpread);
	ADVANCE(mainEquipment.projectilesPerShot);

	ADVANCE(physicalViolence.minSpeed);
	ADVANCE(physicalViolence.minSpeedStrong);
	ADVANCE(physicalViolence.damage);
	ADVANCE(physicalViolence.damageStrong);

	ADVANCE(additionalInfo);
}

void PilgrimStatsInfoDiff::advance(float _time, float _speed)
{
	for_count(int, hIdx, Hand::MAX)
	{
		hands[hIdx].advance(_time, _speed);
	}

	ADVANCE(health);
	ADVANCE(healthBackup);
	ADVANCE(healthRegenRate);
	ADVANCE(healthRegenCooldown);
	ADVANCE(healthDamageReceivedCoef);

	ADVANCE(additionalInfo);
}

//

Framework::ObjectType const* PilgrimStatsInfo::find_suitable_object_type(Framework::IModulesOwner* playerActor)
{
	if (auto* a = fast_cast<Framework::Object>(playerActor))
	{
		return a->get_object_type();
	}
	todo_note(TXT("get from game?"));
	if (auto* pi = PilgrimageInstance::get())
	{
		if (auto* p = pi->get_pilgrimage())
		{
			return p->get_pilgrim();
		}
	}
	if (auto* pilgrimType = Framework::Library::get_current()->get_actor_types().find(Framework::LibraryName(String(hardcoded TXT("pilgrims:actor human")))))
	{
		return pilgrimType;
	}
	an_assert(false);
	return nullptr;
}


void PilgrimStatsInfo::update(Framework::ObjectType const * _objectType, PilgrimSetup const& _setup, bool _onlyBase)
{
	PilgrimBlackboard blackboard;

	// 
	// clear

	additionalInfo = String::empty();

	health = Energy::zero();
	healthBackup = Energy::zero();
	healthRegenRate = Energy::zero();
	healthRegenCooldown = 0.0f;
	healthDamageReceivedCoef = EnergyCoef::zero();

	for_count(int, iHand, Hand::MAX)
	{
		auto& hand = hands[iHand];
		hand.additionalInfo = String::empty();
		hand.pullEnergyDistanceRecentlyRenderedRoom = 0.0f;
		hand.pullEnergyDistanceRecentlyNotRenderedRoom = 0.0f;
		hand.energyInhaleCoef = EnergyCoef::zero();
		hand.handEnergyStorage = Energy::zero();
		hand.mainEquipment.chamberSize = Energy::zero();
		hand.mainEquipment.shotCost = Energy::zero();
		hand.mainEquipment.damage = Energy::zero();
		hand.mainEquipment.immediateDamage = Energy::zero();
		hand.mainEquipment.continuousDamage = Energy::zero();
		hand.mainEquipment.armourPiercing = EnergyCoef::zero();
		hand.mainEquipment.antiDeflection = 0.0f;
		hand.mainEquipment.projectileSpeed = 0.0f;
		hand.mainEquipment.projectileSpread = 0.0f;
		hand.mainEquipment.projectilesPerShot = 1;
		hand.physicalViolence.minSpeed = 100.0f;
		hand.physicalViolence.minSpeedStrong = 100.0f;
		hand.physicalViolence.damage = Energy::zero();
		hand.physicalViolence.damageStrong = Energy::zero();
	}

	// 
	// read base

	if (auto* gDef = _objectType->get_gameplay())
	{
		if (auto* m = fast_cast<ModulePilgrimData>(gDef->get_data()))
		{
			TeaForGodEmperor::PhysicalViolence pv = ModulePilgrim::read_physical_violence_from(m);
			Energy baseMaxEnergy = ModulePilgrim::read_hand_energy_base_max_from(m);
			for_count(int, iHand, Hand::MAX)
			{
				auto& hand = hands[iHand];
				hand.physicalViolence.minSpeed = pv.minSpeed.get(0.0f);
				hand.physicalViolence.minSpeedStrong = pv.minSpeedStrong.get(0.0f);
				hand.physicalViolence.damage = pv.damage.get(Energy::zero());
				hand.physicalViolence.damageStrong = pv.damageStrong.get(Energy::zero());
				hand.handEnergyStorage = baseMaxEnergy;
			}
		}
	}

	for_every(customModuleDef, _objectType->get_customs())
	{
		if (auto * m = fast_cast<CustomModules::HealthData>(customModuleDef->get_data()))
		{
			health = CustomModules::Health::read_max_health_from(m);
			healthBackup = CustomModules::HealthRegen::read_max_health_backup_from(m);
			healthRegenRate = CustomModules::HealthRegen::read_regen_rate_from(m);
			healthRegenCooldown = CustomModules::HealthRegen::read_regen_cooldown_from(m);
		}
	}

	// we need some coefs and boosts from weapon parts to stay
	ModuleEquipments::GunStats gunStatsHands[Hand::MAX];

	for_count(int, iHand, Hand::MAX)
	{
		auto& hand = hands[iHand];
		auto& me = hand.mainEquipment;
		auto& gunStats = gunStatsHands[iHand];
		{
			gunStats.calculate_stats(_setup.get_hand(Hand::Type(iHand)).weaponSetup, true);
			me.isValid = _setup.get_hand(Hand::Type(iHand)).weaponSetup.is_valid();
			me.coreKind = gunStats.coreKind.value;
			me.magazineSize = gunStats.magazineSize.value;
			me.magazineCooldown = gunStats.magazineCooldown.value;
			me.storageOutputSpeed = gunStats.storageOutputSpeed.value;
			me.magazineOutputSpeed = gunStats.magazineOutputSpeed.value;
			me.chamberSize = gunStats.chamberSize.value;
			me.shotCost = gunStats.chamberSize.value;
			me.damage = ((me.shotCost + gunStats.baseDamageBoost.value).adjusted_plus_one(gunStats.damageCoef.value) + gunStats.damageBoost.value).adjusted(gunStats.fromCoreDamageCoef.value);
			me.continuousDamage = Energy::zero();
			me.immediateDamage = me.damage - me.continuousDamage;
			me.armourPiercing = gunStats.armourPiercing.value;
			me.antiDeflection = gunStats.antiDeflection.value;
			// store as it is, so we have me filled - it is used when updating a projectile - we want auto projectile, too (no projectile chosen yet!)
			me.projectileSpeed = gunStats.gun_stats_get_auto_projectile().get_relative_velocity().length();
			me.projectileSpread = gunStats.gun_stats_get_auto_projectile().get_spread();
			me.projectilesPerShot = gunStats.gun_stats_get_auto_projectile().get_count();

			me.specialFeatures.clear();
			for_every(sf, gunStats.specialFeatures)
			{
				me.specialFeatures.push_back(sf->value);
			}

			// we will update projectile with final stats
		}
	}
	
	// 
	// update to full (current)

	if (!_onlyBase)
	{
		PilgrimBlackboard::AdditionalInfos ai;
		ai.body = &additionalInfo;
		ai.hands[Hand::Left] = &hands[Hand::Left].additionalInfo;
		ai.hands[Hand::Right] = &hands[Hand::Right].additionalInfo;
		blackboard.update(_setup, nullptr, &ai);

		health += blackboard.health;
		healthBackup += blackboard.healthBackup;
		healthRegenRate += blackboard.healthRegenRate;
		healthRegenCooldown = blackboard.healthRegenCooldownCoef.adjust_plus_one(healthRegenCooldown);
		healthDamageReceivedCoef += blackboard.healthDamageReceivedCoef;

		for_count(int, iHand, Hand::MAX)
		{
			auto& hand = hands[iHand];
			auto& bbHand = blackboard.hands[iHand];
			hand.pullEnergyDistanceRecentlyRenderedRoom += bbHand.pullEnergyDistanceRecentlyRenderedRoom;
			hand.pullEnergyDistanceRecentlyNotRenderedRoom += bbHand.pullEnergyDistanceRecentlyNotRenderedRoom;
			hand.energyInhaleCoef = max(-EnergyCoef::one(), hand.energyInhaleCoef + bbHand.energyInhaleCoef);
			hand.handEnergyStorage += bbHand.handEnergyStorage;
			auto& me = hand.mainEquipment;
			auto& gunStats = gunStatsHands[iHand];
			me.magazineSize = me.magazineSize.adjusted_plus_one(bbHand.mainEquipment.magazineSizeCoef);
			me.magazineCooldown = me.magazineCooldown * (1.0f + bbHand.mainEquipment.magazineCooldownCoef);
			me.chamberSize = me.chamberSize.adjusted_plus_one(bbHand.mainEquipment.chamberSizeCoef);
			me.magazineSize = max(me.magazineSize, bbHand.mainEquipment.magazineMinSize);
			if (bbHand.mainEquipment.magazineMinShotCount > 0)
			{
				me.magazineSize = max(me.magazineSize, me.chamberSize * bbHand.mainEquipment.magazineMinShotCount);
			}
			me.shotCost = me.shotCost.adjusted_plus_one(bbHand.mainEquipment.chamberSizeCoef);
			me.damage = ((me.shotCost + gunStats.baseDamageBoost.value).adjusted_plus_one(gunStats.damageCoef.value) + gunStats.damageBoost.value).adjusted(gunStats.fromCoreDamageCoef.value);
			me.damage = me.damage.adjusted_plus_one(bbHand.mainEquipment.damageCoef);
			me.continuousDamage = Energy::zero();
			me.immediateDamage = me.damage - me.continuousDamage;
			me.armourPiercing = me.armourPiercing + bbHand.mainEquipment.armourPiercing;
			me.antiDeflection = me.antiDeflection + bbHand.mainEquipment.antiDeflection;
			me.storageOutputSpeed = me.storageOutputSpeed.adjusted_plus_one(bbHand.mainEquipment.storageOutputSpeedCoef) + bbHand.mainEquipment.storageOutputSpeedAdd;
			me.magazineOutputSpeed = me.magazineOutputSpeed.adjusted_plus_one(bbHand.mainEquipment.magazineOutputSpeedCoef) + bbHand.mainEquipment.magazineOutputSpeedAdd;

			// now update projectile only as we already have stats
			gunStats.calculate_stats_update_projectile(nullptr, &me);
			me.projectileSpeed = gunStats.gun_stats_get_used_projectile().get_relative_velocity().length();
			me.projectileSpread = gunStats.gun_stats_get_used_projectile().get_spread();
			me.projectilesPerShot = gunStats.gun_stats_get_used_projectile().get_count();

			me.specialFeatures.clear();
			for_every(sf, gunStats.specialFeatures)
			{
				me.specialFeatures.push_back(sf->value);
			}

			if (auto* pt = gunStats.gun_stats_get_used_projectile().get_type())
			{
				if (auto* pd = pt->get_gameplay_as<ModuleProjectileData>())
				{
					for_every(ad, pd->get_additional_gun_stats_info())
					{
						add_info(&hand.additionalInfo, ad->get());
					}
				}
			}

			auto& pv = hand.physicalViolence;
			if (bbHand.physicalViolence.minSpeed.is_set()) pv.minSpeed *= bbHand.physicalViolence.minSpeed.get();
			if (bbHand.physicalViolence.minSpeedStrong.is_set()) pv.minSpeedStrong *= bbHand.physicalViolence.minSpeedStrong.get();
			if (bbHand.physicalViolence.damage.is_set()) pv.damage = bbHand.physicalViolence.damage.get();
			if (bbHand.physicalViolence.damageStrong.is_set()) pv.damageStrong = bbHand.physicalViolence.damageStrong.get();

			todo_note(TXT("add information about core kind modifiers"));
		}
	}

	//
	// other auto

	// mins
	health = max(health, Energy(10));
	healthBackup = max(healthBackup, Energy::zero());
	healthRegenRate = max(healthRegenRate, Energy(10));
	healthRegenCooldown = max(0.0f, healthRegenCooldown);
	healthDamageReceivedCoef = max(EnergyCoef(0.10f), healthDamageReceivedCoef);

	for_count(int, iHand, Hand::MAX)
	{
		auto& hand = hands[iHand];
		auto& me = hand.mainEquipment;
		if (me.shotCost.is_zero())
		{
			me.chamberSize = Energy::zero();
			me.storageShotCount = 0;
			me.storageTotalDamage = Energy::zero();
			me.storageRPM = Energy::zero();
			me.storageDPS = Energy::zero();
			me.magazineShotCount = 0;
			me.magazineTotalDamage = Energy::zero();
			me.magazineRPM = Energy::zero();
			me.magazineDPS = Energy::zero();
		}
		else
		{
			me.storageShotCount = (hand.handEnergyStorage / me.shotCost).floored().as_int();
			me.magazineShotCount = (me.magazineSize / me.shotCost).floored().as_int();
			me.storageTotalDamage = me.damage * me.storageShotCount;
			me.magazineTotalDamage = me.damage * me.magazineShotCount;
			#define RPS me.storageOutputSpeed / me.shotCost
			me.storageRPM = Energy(60) * RPS;
			me.storageDPS = me.damage * RPS;
			#undef RPS
			#define RPS (me.magazineSize.is_zero() ? Energy::zero() : me.magazineOutputSpeed) / me.shotCost
			me.magazineRPM = Energy(60) * RPS;
			me.magazineDPS = me.damage * RPS;
			#undef RPS
		}
		me.armourPiercing = clamp(me.armourPiercing, EnergyCoef::zero(), EnergyCoef::one());
		me.antiDeflection = clamp(me.antiDeflection, 0.0f, 1.0f);
		// including chamber
		me.shotCount = 1 + me.storageShotCount + me.magazineShotCount;
		me.totalDamage = me.damage + me.storageTotalDamage + me.magazineTotalDamage;
		me.totalEnergy = me.chamberSize + me.magazineSize + hand.handEnergyStorage;

		// mins
		hand.physicalViolence.minSpeedStrong = max(hand.physicalViolence.minSpeed, hand.physicalViolence.minSpeedStrong);
		hand.physicalViolence.damageStrong = max(hand.physicalViolence.damage, hand.physicalViolence.damageStrong);
		if (hand.physicalViolence.minSpeedStrong <= hand.physicalViolence.minSpeed)
		{
			hand.physicalViolence.damage = hand.physicalViolence.damageStrong;
		}

		// store additional info
		{
#ifdef ARMOUR_DEFLECTION_ETC_INTO_ADDITIONAL_STATS
			if (!hand.mainEquipment.armourPiercing.is_zero())
			{
				add_info(&hand.additionalInfo, Framework::StringFormatter::format_sentence_loc_str(NAME(lsArmourPiercing), Framework::StringFormatterParams()
					.add(NAME(coef), hand.mainEquipment.armourPiercing.as_string_percentage())));
			}
			if (hand.mainEquipment.antiDeflection > 0.0f)
			{
				add_info(&hand.additionalInfo, Framework::StringFormatter::format_sentence_loc_str(NAME(lsAntiDeflection), Framework::StringFormatterParams()
					.add(NAME(coef), String::printf(TXT("%.0f%%"), hand.mainEquipment.antiDeflection * 100.0f))));
			}
#endif
		}

	}

	if (GameSettings::get().difficulty.commonHandEnergyStorage)
	{
		hands[Hand::Right].handEnergyStorage = Energy::zero();
	}
}
