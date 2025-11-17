#include "me_gun.h"

#include "..\moduleEnergyQuantum.h"
#include "..\modulePilgrim.h"
#include "..\moduleProjectile.h"

#include "..\..\custom\mc_emissiveControl.h"
#include "..\..\custom\mc_itemHolder.h"
#include "..\..\custom\health\mc_health.h"

#include "..\..\..\teaForGod.h"
#include "..\..\..\utils.h"
#include "..\..\..\gameplayBalance.h"
#include "..\..\..\ai\aiRayCasts.h"
#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameLog.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\game\weaponStatInfoImpl.inl"
#include "..\..\..\game\modes\gameMode_Pilgrimage.h"
#include "..\..\..\library\library.h"
#include "..\..\..\library\projectileSelector.h"
#include "..\..\..\library\weaponCoreModifiers.h"
#include "..\..\..\library\weaponPartType.h"
#include "..\..\..\music\musicPlayer.h"
#include "..\..\..\overlayInfo\elements\oie_text.h"
#include "..\..\..\pilgrimage\pilgrimageInstance.h"
#include "..\..\..\schematics\schematic.h"
#include "..\..\..\schematics\schematicLine.h"
#include "..\..\..\utils\buildStatsInfo.h"
#include "..\..\..\utils\collectInTargetCone.h"
#include "..\..\..\utils\lightningDischarge.h"

#include "..\..\..\..\framework\debug\previewGame.h"
#include "..\..\..\..\framework\game\gameConfig.h"
#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\..\framework\module\modules.h"
#include "..\..\..\..\framework\module\moduleCollisionProjectile.h"
#include "..\..\..\..\framework\module\moduleMovementProjectile.h"
#include "..\..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\object\temporaryObjectType.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\presenceLink.h"
#include "..\..\..\..\framework\world\subWorld.h"

#include "..\..\..\..\core\other\parserUtils.h"
#include "..\..\..\..\core\physicalSensations\physicalSensations.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define DEFAULT_SUPERCHARGER_EXTRA_PRICE EnergyCoef(0.5f) /* +50% */

#define DEFAULT_MAGAZINE_OUTPUT_SPEED Energy(20)

//#define PROJECTILE_COUNT_AND_SPEED_IN_ONE_LINE

//

//#define TEST_RANDOM_COLOURS

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#define DEBUG_SHOOTING_PROJECTILE
	//#define DEBUG_ENERGY_STATE
	//#define OUTPUT_ENERGY_LEVELS
	//#define LOG_WEAPON_BEING_CREATED
#endif

#define INSPECT_WEAPON_CREATION

//

using namespace TeaForGodEmperor;
using namespace ModuleEquipments;

//

// sounds/temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME_STR(shootPlasma, TXT("shoot_plasma"));
DEFINE_STATIC_NAME_STR(shootIon, TXT("shoot_ion"));
DEFINE_STATIC_NAME_STR(shootCorrosion, TXT("shoot_corrosion"));
DEFINE_STATIC_NAME_STR(shootLightning, TXT("shoot_lightning"));
//
DEFINE_STATIC_NAME(muzzleFlash);
DEFINE_STATIC_NAME_STR(muzzleFlashPlasma, TXT("muzzleFlash_plasma"));
DEFINE_STATIC_NAME_STR(muzzleFlashIon, TXT("muzzleFlash_ion"));
DEFINE_STATIC_NAME_STR(muzzleFlashCorrosion, TXT("muzzleFlash_corrosion"));
DEFINE_STATIC_NAME_STR(muzzleFlashLightning, TXT("muzzleFlash_lightning"));
//
DEFINE_STATIC_NAME_STR(onShoot, TXT("on shoot"));
DEFINE_STATIC_NAME_STR(onMiss, TXT("on miss"));
DEFINE_STATIC_NAME(idle);
DEFINE_STATIC_NAME(discharge);
DEFINE_STATIC_NAME_STR(dischargeMiss, TXT("discharge miss"));
DEFINE_STATIC_NAME_STR(lightningHit, TXT("lightning hit"));
DEFINE_STATIC_NAME(shootFailed);
DEFINE_STATIC_NAME(chamberCharged);
DEFINE_STATIC_NAME(chamberChargedWithMagazine);
DEFINE_STATIC_NAME(chamberChargedNoMagazine);
DEFINE_STATIC_NAME(magazineCharged);
DEFINE_STATIC_NAME(chamberCharge);
DEFINE_STATIC_NAME_STR(damagedShot, TXT("damaged shot"));

// module params
DEFINE_STATIC_NAME(allowEnergyDeposit);
DEFINE_STATIC_NAME(normalColour);
DEFINE_STATIC_NAME(readyColour);
DEFINE_STATIC_NAME(lockedColour);
DEFINE_STATIC_NAME(muzzleHotEnergyFull);
DEFINE_STATIC_NAME(muzzleHotCoolDownTime);
DEFINE_STATIC_NAME(superchargerColour);
DEFINE_STATIC_NAME(retaliatorColour);
DEFINE_STATIC_NAME(useRandomParts);
DEFINE_STATIC_NAME(forcePart);

// appearance variables
DEFINE_STATIC_NAME(gunInUse);

// shader param (appearance)
DEFINE_STATIC_NAME(sightDotInUse);
DEFINE_STATIC_NAME(sightDotRadiusOuter);
DEFINE_STATIC_NAME(sightDotColour);

// special features
DEFINE_STATIC_NAME(supercharger);
DEFINE_STATIC_NAME(retaliator);
DEFINE_STATIC_NAME(penetrator);

// colours
DEFINE_STATIC_NAME(weapon_normal);
DEFINE_STATIC_NAME(weapon_locked);
DEFINE_STATIC_NAME(weapon_ready);
DEFINE_STATIC_NAME(pilgrim_overlay_weaponPart_damaged);

DEFINE_STATIC_NAME(gun_supercharger);
DEFINE_STATIC_NAME(gun_retaliator);

DEFINE_STATIC_NAME(red_plasma);
DEFINE_STATIC_NAME(corrosion);
DEFINE_STATIC_NAME(lightning);

// emissive layers
// 0 main indicator
// 1 muzzle
// 2 storage state (0 empty, 1 filled)
// 3 sight indicator
DEFINE_STATIC_NAME(normal);
DEFINE_STATIC_NAME(locked);
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(muzzle);
DEFINE_STATIC_NAME(muzzleHot);
DEFINE_STATIC_NAME(energyStorage);
DEFINE_STATIC_NAME(energyStorageFull);
DEFINE_STATIC_NAME(sight);

// projectile's and muzzle's variables (also set in the gun)
DEFINE_STATIC_NAME(projectileSpeed);
DEFINE_STATIC_NAME(projectileDamage);
DEFINE_STATIC_NAME(projectileEmissiveColour);

// mesh generator variables (set in owner)
DEFINE_STATIC_NAME(genChamberSize);
DEFINE_STATIC_NAME(genDamageBase);
DEFINE_STATIC_NAME(genStorageSize);
DEFINE_STATIC_NAME(genMagazineSize);
DEFINE_STATIC_NAME(genProjectileSpeed);
DEFINE_STATIC_NAME(genProjectileSpread);
DEFINE_STATIC_NAME(genHasMuzzle);
//
DEFINE_STATIC_NAME(genMaxGunHeight);
DEFINE_STATIC_NAME(genMaxGunWidth);
DEFINE_STATIC_NAME(genMaxGunForward);
DEFINE_STATIC_NAME(genMaxGunForwardTotal);
DEFINE_STATIC_NAME(genMaxGunBackward);
DEFINE_STATIC_NAME(genHandPortRadius);
DEFINE_STATIC_NAME(genHandPortHeight);
DEFINE_STATIC_NAME(genRestPortRadius);
//
DEFINE_STATIC_NAME_STR(withLeftRestPoint, TXT("with left rest point"));
DEFINE_STATIC_NAME_STR(withRightRestPoint, TXT("with right rest point"));

// localised strings
DEFINE_STATIC_NAME_STR(lsCharsDamage, TXT("chars; damage"));
DEFINE_STATIC_NAME_STR(lsCharsAmmo, TXT("chars; ammo"));
DEFINE_STATIC_NAME_STR(lsCharsHealth, TXT("chars; health"));

// shared
	DEFINE_STATIC_NAME(value);
	DEFINE_STATIC_NAME(total);
	DEFINE_STATIC_NAME(damage);
	DEFINE_STATIC_NAME(normalDamage);
	DEFINE_STATIC_NAME(continuousDamage);
	DEFINE_STATIC_NAME(coef);
	DEFINE_STATIC_NAME(cost);
	DEFINE_STATIC_NAME(cap);
DEFINE_STATIC_NAME_STR(lsEquipmentDamaged, TXT("pi.ov.in; gun; damaged"));
DEFINE_STATIC_NAME_STR(lsEquipmentDamagedReplace, TXT("pi.ov.in; gun; damaged replace"));
DEFINE_STATIC_NAME_STR(lsEquipmentDamageToCostRatio, TXT("pi.ov.in; gun; damage to cost ratio"));
DEFINE_STATIC_NAME_STR(lsEquipmentDamage, TXT("pi.ov.in; gun; damage"));
DEFINE_STATIC_NAME_STR(lsEquipmentCost, TXT("pi.ov.in; gun; cost"));
DEFINE_STATIC_NAME_STR(lsEquipmentMagazine, TXT("pi.ov.in; gun; magazine"));
DEFINE_STATIC_NAME_STR(lsEquipmentArmourPiercing, TXT("pi.ov.in; gun; armour piercing"));
DEFINE_STATIC_NAME_STR(lsEquipmentAntiDeflection, TXT("pi.ov.in; gun; anti deflection"));
DEFINE_STATIC_NAME_STR(lsEquipmentMaxDist, TXT("pi.ov.in; gun; max dist"));
DEFINE_STATIC_NAME_STR(lsEquipmentKineticEnergyCoef, TXT("pi.ov.in; gun; kinetic energy coef"));
DEFINE_STATIC_NAME_STR(lsEquipmentConfuse, TXT("pi.ov.in; gun; confuse"));
DEFINE_STATIC_NAME_STR(lsEquipmentMedi, TXT("pi.ov.in; gun; medi"));
DEFINE_STATIC_NAME_STR(lsEquipmentContinuousFire, TXT("pi.ov.in; gun; continuous fire"));
DEFINE_STATIC_NAME_STR(lsEquipmentNoHitBoneDamage, TXT("pi.ov.in; gun; no hit bone damage"));
DEFINE_STATIC_NAME_STR(lsEquipmentRoundsPerMinute, TXT("pi.ov.in; gun; rounds per minute"));
DEFINE_STATIC_NAME_STR(lsEquipmentRoundsPerMinuteClass, TXT("pi.ov.in; gun; rounds per minute; class"));
DEFINE_STATIC_NAME_STR(lsEquipmentRoundsPerMinuteMagazineClass, TXT("pi.ov.in; gun; rounds per minute; magazine class"));
DEFINE_STATIC_NAME_STR(lsEquipmentProjectileCount, TXT("pi.ov.in; gun; projectile count"));
	DEFINE_STATIC_NAME(projectileCount);
DEFINE_STATIC_NAME_STR(lsEquipmentProjectileSpeed, TXT("pi.ov.in; gun; projectile speed"));
DEFINE_STATIC_NAME_STR(lsEquipmentProjectileSpread, TXT("pi.ov.in; gun; projectile spread"));
DEFINE_STATIC_NAME_STR(lsEquipmentProjectileCountAndSpeed, TXT("pi.ov.in; gun; projectile count and speed"));

DEFINE_STATIC_NAME_STR(lsSpecialFeatureSupercharger, TXT("weapon special feature; supercharger"));
DEFINE_STATIC_NAME_STR(lsSpecialFeatureRetaliator, TXT("weapon special feature; retaliator"));
DEFINE_STATIC_NAME_STR(lsSpecialFeaturePenetrator, TXT("weapon special feature; penetrator"));

DEFINE_STATIC_NAME(nodeCount);

// exm variable
DEFINE_STATIC_NAME(retainedPt);

// text colours
DEFINE_STATIC_NAME(core);
DEFINE_STATIC_NAME(adjCore);
DEFINE_STATIC_NAME(damaged);

//

static bool collect_parts(WeaponSetup const& _weaponSetup, WeaponPartType::Type _type, OUT_ ArrayStack<WeaponPart const*>& _parts)
{
	_parts.clear();
	for_every(dp, _weaponSetup.get_parts())
	{
		if (auto* wp = dp->part.get())
		{
			if (wp->get_weapon_part_type()->get_type() == _type)
			{
				_parts.push_back(wp);
			}
		}
	}
	return !_parts.is_empty();
}

//

#define ADD_AFFECTED_BY(what, from) \
	for_every(a, (from).affectedBy) \
	{ \
		if (what.affectedBy.has_place_left()) \
		{ \
			what.affectedBy.push_back_unique(*a); \
		} \
	}

#define ADD_AFFECTED_BY_NEG(what, from) \
	for_every(a, (from).affectedBy) \
	{ \
		if (what.affectedBy.has_place_left()) \
		{ \
			WeaponsStatAffectedByPart wsap = *a; \
			wsap.how = WeaponStatAffection::negate_effect(wsap.how); \
			what.affectedBy.push_back_unique(wsap); \
		} \
	}

#define ADD_AFFECTED_BY_OVERRIDE(what, from, _how) \
	for_every(a, (from).affectedBy) \
	{ \
		if (what.affectedBy.has_place_left()) \
		{ \
			WeaponsStatAffectedByPart wsap = *a; \
			wsap.how = _how; \
			what.affectedBy.push_back_unique(wsap); \
		} \
	}

#define ADD_AFFECTION(what, part, how) \
	if (part && how != WeaponStatAffection::Unknown) \
	{ \
		what.add_affection(part, how); \
	}

#define SET_AFFECTION(what, part, how) \
	if (part && how != WeaponStatAffection::Unknown) \
	{ \
		what.move_to_not_affected_by(); \
		what.add_affection(part, how); \
	}

#define RESET_CLEAR(what) \
	what.clear_affection(); what.value.clear();

#define RESET_TO(what, _value) \
	what.clear_affection(); what.value = _value;

#define ADD(what, source) \
	if (source.value.is_set()) { what.value += source.value.get(); ADD_AFFECTION(what, part, source.how); }

#define MUL_REL_TO(what, source, relativeTo) \
	if (source.value.is_set()) { what.value *= relativeTo + source.value.get(); ADD_AFFECTION(what, part, source.how); }

#define ADJ(what, source) \
	if (source.value.is_set()) { what.value = what.value.adjusted(source.value.get()); ADD_AFFECTION(what, part, source.how); }

#define ADD_SPECIAL_FEATURES(what, source) \
	for_every(sf, source) \
	{ \
		WeaponStatInfo<Name> si; \
		si.value = *sf; \
		ADD_AFFECTION(si, part, WeaponStatAffection::Special); \
		what.push_back_unique(si); \
	}

#define ADD_LOC_STR_ID(what, source_loc_str) \
	if (source_loc_str.is_valid() && ! source_loc_str.get().is_empty()) \
	{ \
		WeaponStatInfo<Name> si; \
		si.value = source_loc_str.get_id(); \
		ADD_AFFECTION(si, part, WeaponStatAffection::Special); \
		what.push_back(si); \
	}

#define SET(what, source) \
	if (source.value.is_set()) { what.value = source.value.get(); SET_AFFECTION(what, part, WeaponStatAffection::Set); }

#define SET_SUB_FIELD(what, field, source) \
	if (source.value.is_set()) { what.value.field = source.value.get(); SET_AFFECTION(what, part, WeaponStatAffection::Set); }

#define SET_IF_NOT_SET(what, source) \
	if (source.value.is_set() && ! what.value.is_set()) { what.value = source.value.get(); SET_AFFECTION(what, part, WeaponStatAffection::Set); }

#define MAX(what, source) \
	if (source.value.is_set() && what.value < source.value.get()) { what.value = source.value.get(); SET_AFFECTION(what, part, WeaponStatAffection::Set); }

#define SET_NON_STAT(what, source) \
	if (source.is_set()) what = source.get();

void GunStats::calculate_stats(WeaponSetup const& _weaponSetup, bool _pilgrimWeapon)
{
#ifdef INSPECT_WEAPON_CREATION
	output(TXT("calculate stats"));
	{
		IO::XML::Document doc;
		IO::XML::Node* node = doc.get_root()->add_node(TXT("usingWeaponSetup"));
		_weaponSetup.save_to_xml(node);
		String asString = node->to_string_slow();
		output(TXT("%S"), asString.to_char());
	}
#endif
	{	// reset all
		RESET_TO(coreKind, WeaponCoreKind::None);
		RESET_TO(damaged, EnergyCoef::zero());
		RESET_TO(chamberSize, Energy::zero());
		RESET_TO(maxStorage, Energy::zero());
		RESET_TO(storageOutputSpeed, Energy::zero());
		RESET_TO(baseDamageBoost, Energy::zero());
		RESET_TO(damageCoef, EnergyCoef::zero());
		RESET_TO(damageBoost, Energy::zero());
		RESET_TO(armourPiercing, EnergyCoef::zero());
		RESET_TO(magazineSize, Energy::zero());
		RESET_TO(magazineCooldown, 0.0f);
		RESET_TO(magazineOutputSpeed, Energy::zero());
		RESET_TO(continuousFire, false);
		RESET_TO(singleSpecialProjectile, false);
		RESET_TO(isUsingAutoProjectiles, false);
		autoProjectile.relativeVelocity = Vector3::zero;
		autoProjectile.spread = 0.0f;
		autoProjectile.count = 0;
		autoProjectile.hasMuzzle = false;
		autoProjectile.colour.clear();
		RESET_TO(antiDeflection, 0.0f);
		RESET_CLEAR(maxDist);
		RESET_TO(kineticEnergyCoef, 0.0f);
		RESET_TO(confuse, 0.0f);
		RESET_TO(mediCoef, EnergyCoef::zero());
		RESET_TO(continuousDamageMinTime, 0.0f);
		RESET_TO(continuousDamageTime, 0.0f);
		RESET_TO(noHitBoneDamage, false);
		specialFeatures.clear();
		statInfoLocStrs.clear();
	}

	{	// by type of part
		ARRAY_STACK(WeaponPart const*, partsOfType, _weaponSetup.get_parts().get_size());

		WeaponPart const* corePart = nullptr;

		WeaponStatInfo<EnergyCoef> chamberSizeCoef = EnergyCoef::zero();

		WeaponStatInfo<float> coreProjectileSpeedCoef = 0.0f;
		WeaponStatInfo<float> coreProjectileSpreadCoef = 0.0f;

		WeaponStatInfo<float> magazineCooldownCoef = 0.0f;

		WeaponStatInfo<float> maxDistCoef = 0.0f;
		
		WeaponStatInfo<Energy> storageOutputSpeedAdd = Energy::zero();
		WeaponStatInfo<EnergyCoef> storageOutputSpeedAdj = EnergyCoef::one();
		WeaponStatInfo<Energy> magazineOutputSpeedAdd = Energy::zero();

		WeaponStatInfo<Optional<WeaponCoreKind::Type>> proposedCoreKind;

		for_every(part, _weaponSetup.get_parts())
		{
			damaged.value += part->part->get_damaged();
		}

		if (collect_parts(_weaponSetup, WeaponPartType::GunCore, OUT_ partsOfType))
		{
			for_every_ptr(part, partsOfType)
			{
				corePart = part; // always store, we assume there's just one
				SET(proposedCoreKind, part->get_core_kind());
				SET_NON_STAT(autoProjectile.colour, part->get_projectile_colour());
				ADD(chamberSize, part->get_chamber());
				ADD(chamberSizeCoef, part->get_chamber_coef());
				ADD(damageCoef, part->get_damage_coef());
				ADD(damageBoost, part->get_damage_boost());
				ADD(armourPiercing, part->get_armour_piercing());
				ADD(coreProjectileSpeedCoef, part->get_projectile_speed_coef());
				ADD(coreProjectileSpreadCoef, part->get_projectile_spread_coef());
				ADD(magazineCooldown, part->get_magazine_cooldown());
				ADD(antiDeflection, part->get_anti_deflection());
				ADD(maxDistCoef, part->get_max_dist_coef());
				ADD(kineticEnergyCoef, part->get_kinetic_energy_coef());
				ADD(confuse, part->get_confuse());
				ADD(mediCoef, part->get_medi_coef());
				ADD_SPECIAL_FEATURES(specialFeatures, part->get_special_features());
				ADD_LOC_STR_ID(statInfoLocStrs, part->get_stat_info());
			}
		}

		if (collect_parts(_weaponSetup, WeaponPartType::GunChassis, OUT_ partsOfType))
		{
			for_every_ptr(part, partsOfType)
			{
				SET_IF_NOT_SET(proposedCoreKind, part->get_core_kind());
				ADD(chamberSize, part->get_chamber());
				ADD(chamberSizeCoef, part->get_chamber_coef());
				ADD(damageCoef, part->get_damage_coef());
				ADD(damageBoost, part->get_damage_boost());
				ADD(armourPiercing, part->get_armour_piercing());
				ADD(maxStorage, part->get_storage());
				MAX(storageOutputSpeed, part->get_storage_output_speed());
				ADD(magazineSize, part->get_magazine());
				ADD(magazineCooldown, part->get_magazine_cooldown());
				ADD(magazineCooldownCoef, part->get_magazine_cooldown_coef());
				MAX(magazineOutputSpeed, part->get_magazine_output_speed());
				SET(continuousFire, part->get_continuous_fire());
				SET_SUB_FIELD(autoProjectile.relativeVelocity, y, part->get_projectile_speed());
				SET(autoProjectile.spread, part->get_projectile_spread());
				SET(autoProjectile.count, part->get_number_of_projectiles());
				ADD(antiDeflection, part->get_anti_deflection());
				ADD(maxDistCoef, part->get_max_dist_coef());
				ADD(kineticEnergyCoef, part->get_kinetic_energy_coef());
				ADD(confuse, part->get_confuse());
				ADD(mediCoef, part->get_medi_coef());
				ADD_SPECIAL_FEATURES(specialFeatures, part->get_special_features()); 
				ADD_LOC_STR_ID(statInfoLocStrs, part->get_stat_info());
			}
		}

		if (collect_parts(_weaponSetup, WeaponPartType::GunMuzzle, OUT_ partsOfType))
		{
			for_every_ptr(part, partsOfType)
			{
				ADD(damageCoef, part->get_damage_coef());
				ADD(damageBoost, part->get_damage_boost());
				ADD(armourPiercing, part->get_armour_piercing());
				SET_SUB_FIELD(autoProjectile.relativeVelocity, y, part->get_projectile_speed());
				SET(autoProjectile.spread, part->get_projectile_spread());
				SET(autoProjectile.count, part->get_number_of_projectiles());
				ADD(antiDeflection, part->get_anti_deflection());
				ADD(maxDistCoef, part->get_max_dist_coef());
				ADD(kineticEnergyCoef, part->get_kinetic_energy_coef());
				ADD(confuse, part->get_confuse());
				ADD(mediCoef, part->get_medi_coef());
				ADD_SPECIAL_FEATURES(specialFeatures, part->get_special_features());
				ADD_LOC_STR_ID(statInfoLocStrs, part->get_stat_info());
				autoProjectile.hasMuzzle = true;
			}
		}

		if (collect_parts(_weaponSetup, WeaponPartType::GunNode, OUT_ partsOfType))
		{
			WeaponStatInfo<float> projectileSpeedCoef = 0.0f;
			WeaponStatInfo<float> projectileSpreadCoef = 0.0f;
			for_every_ptr(part, partsOfType)
			{
				ADD(chamberSizeCoef, part->get_chamber_coef());
				ADD(baseDamageBoost, part->get_base_damage_boost());
				ADD(damageCoef, part->get_damage_coef());
				ADD(damageBoost, part->get_damage_boost());
				ADD(armourPiercing, part->get_armour_piercing());
				ADD(maxStorage, part->get_storage());
				MAX(storageOutputSpeed, part->get_storage_output_speed());
				ADD(storageOutputSpeedAdd, part->get_storage_output_speed_add());
				ADJ(storageOutputSpeedAdj, part->get_storage_output_speed_adj());
				ADD(magazineSize, part->get_magazine());
				ADD(magazineCooldown, part->get_magazine_cooldown());
				ADD(magazineCooldownCoef, part->get_magazine_cooldown_coef());
				MAX(magazineOutputSpeed, part->get_magazine_output_speed());
				ADD(magazineOutputSpeedAdd, part->get_magazine_output_speed_add());
				SET(continuousFire, part->get_continuous_fire());
				ADD(projectileSpeedCoef, part->get_projectile_speed_coef());
				ADD(projectileSpreadCoef, part->get_projectile_spread_coef());
				SET_SUB_FIELD(autoProjectile.relativeVelocity, y, part->get_projectile_speed());
				SET(autoProjectile.spread, part->get_projectile_spread());
				SET(autoProjectile.count, part->get_number_of_projectiles());
				ADD(antiDeflection, part->get_anti_deflection());
				ADD(maxDistCoef, part->get_max_dist_coef());
				ADD(kineticEnergyCoef, part->get_kinetic_energy_coef());
				ADD(confuse, part->get_confuse());
				ADD(mediCoef, part->get_medi_coef());
				ADD_SPECIAL_FEATURES(specialFeatures, part->get_special_features());
				ADD_LOC_STR_ID(statInfoLocStrs, part->get_stat_info());
			}
			autoProjectile.relativeVelocity.value *= 1.0f + projectileSpeedCoef.value;
			autoProjectile.spread.value *= 1.0f + projectileSpreadCoef.value;
			ADD_AFFECTED_BY(autoProjectile.relativeVelocity, projectileSpeedCoef);
			ADD_AFFECTED_BY(autoProjectile.spread, projectileSpreadCoef);
		}

		if (proposedCoreKind.value.is_set())
		{
			coreKind.value = proposedCoreKind.value.get();
			coreKind.affectedBy = proposedCoreKind.affectedBy;
		}

		// coefs should not go lower than -95%
		chamberSizeCoef.value = max(chamberSizeCoef.value, EnergyCoef::percent(-95));
		damageCoef.value = max(damageCoef.value, EnergyCoef::percent(-95));

		autoProjectile.count.value = max(1, autoProjectile.count.value);

		autoProjectile.relativeVelocity.value *= 1.0f + coreProjectileSpeedCoef.value;
		autoProjectile.spread.value *= 1.0f + coreProjectileSpreadCoef.value;
		ADD_AFFECTED_BY(autoProjectile.relativeVelocity, coreProjectileSpeedCoef);
		ADD_AFFECTED_BY(autoProjectile.spread, coreProjectileSpreadCoef);

		chamberSize.value = chamberSize.value.adjusted_plus_one(chamberSizeCoef.value);
		ADD_AFFECTED_BY(chamberSize, chamberSizeCoef);

		magazineCooldown.value = max(0.0f, magazineCooldown.value * (1.0f - magazineCooldownCoef.value));
		ADD_AFFECTED_BY(magazineCooldown, magazineCooldownCoef);

		storageOutputSpeed.value += storageOutputSpeedAdd.value;
		magazineOutputSpeed.value += magazineOutputSpeedAdd.value;
		ADD_AFFECTED_BY(storageOutputSpeed, storageOutputSpeedAdd);
		ADD_AFFECTED_BY(magazineOutputSpeed, magazineOutputSpeedAdd);

		storageOutputSpeed.value = storageOutputSpeed.value.adjusted(storageOutputSpeedAdj.value);
		ADD_AFFECTED_BY(storageOutputSpeed, storageOutputSpeedAdj);

		// apply core modifiers
		{
			RESET_TO(fromCoreDamageCoef, EnergyCoef::one());
			if (auto* gd = GameDefinition::get_chosen())
			{
				if (auto* wcm = gd->get_weapon_core_modifiers())
				{
					if (auto* fc = wcm->get_for_core(coreKind.value))
					{
						if (fc->chamberSizeAdj.value.is_set())
						{
							chamberSize.value = chamberSize.value.adjusted(fc->chamberSizeAdj.value.get());
							ADD_AFFECTION(chamberSize, corePart, fc->chamberSizeAdj.how);
						}
						if (fc->damageAdj.value.is_set())
						{
							fromCoreDamageCoef.value = fc->damageAdj.value.get();
							SET_AFFECTION(fromCoreDamageCoef, corePart, fc->damageAdj.how);
						}
						if (fc->continuousDamageMinTime.value.is_set())
						{
							continuousDamageMinTime.value = fc->continuousDamageMinTime.value.get();
							SET_AFFECTION(continuousDamageMinTime, corePart, fc->continuousDamageMinTime.how);
						}
						if (fc->continuousDamageTime.value.is_set())
						{
							continuousDamageTime.value = fc->continuousDamageTime.value.get();
							SET_AFFECTION(continuousDamageTime, corePart, fc->continuousDamageTime.how);
						}
						if (fc->noHitBoneDamage)
						{
							noHitBoneDamage = fc->noHitBoneDamage;
						}
						if (fc->outputSpeedAdj.value.is_set())
						{
							storageOutputSpeed.value = storageOutputSpeed.value.adjusted(fc->outputSpeedAdj.value.get());
							magazineOutputSpeed.value = magazineOutputSpeed.value.adjusted(fc->outputSpeedAdj.value.get());
							ADD_AFFECTION(storageOutputSpeed, corePart, fc->outputSpeedAdj.how);
							ADD_AFFECTION(magazineOutputSpeed, corePart, fc->outputSpeedAdj.how);
						}
						if (fc->projectileSpeedAdj.value.is_set())
						{
							autoProjectile.relativeVelocity.value *= fc->projectileSpeedAdj.value.get();
							ADD_AFFECTION(autoProjectile.relativeVelocity, corePart, fc->projectileSpeedAdj.how);
						}
						if (fc->projectileSpreadAdd.value.is_set())
						{
							autoProjectile.spread.value += fc->projectileSpreadAdd.value.get();
							ADD_AFFECTION(autoProjectile.spread, corePart, fc->projectileSpreadAdd.how);
						}
						if (fc->armourPiercing.value.is_set())
						{
							armourPiercing.value = fc->armourPiercing.value.get() + armourPiercing.value * (EnergyCoef::one() - fc->armourPiercing.value.get());
							ADD_AFFECTION(armourPiercing, corePart, fc->armourPiercing.how);
						}
						if (fc->antiDeflection.value.is_set())
						{
							antiDeflection.value = fc->antiDeflection.value.get() + antiDeflection.value * (1.0f - fc->antiDeflection.value.get());
							ADD_AFFECTION(antiDeflection, corePart, fc->antiDeflection.how);
						}
						if (fc->maxDist.value.is_set())
						{
							maxDist.value = fc->maxDist.value.get() * (1.0f + maxDistCoef.value);
							ADD_AFFECTION(maxDist, corePart, fc->maxDist.how);
						}
						// not affected by core
						//		kineticEnergyCoef 
						//		confuse
						//		mediCoef
						sightColourSameAsProjectile = fc->sightColourSameAsProjectile;
					}
				}
			}
		}
		
		// special features
		for_every(sf, specialFeatures) if (sf->value == NAME(supercharger))
		{
			continuousFire = false;
		}
		for_every(sf, specialFeatures) if (sf->value == NAME(penetrator))
		{
			autoProjectile.count = 1;
		}

		mediCoef.value = clamp(mediCoef.value, EnergyCoef::zero(), EnergyCoef::one());

		armourPiercing.value = clamp(armourPiercing.value, EnergyCoef::zero(), EnergyCoef::one());
		antiDeflection.value = clamp(antiDeflection.value, 0.0f, 1.0f);
		if (maxDist.value.is_set())
		{
			maxDist.value = max(0.5f, maxDist.value.get());
		}

		// modify damage boosts so there's always some damage done
		{
			Energy minNominalDamage = Energy(0.1f);
			baseDamageBoost.value = max(baseDamageBoost.value, minNominalDamage - chamberSize.value);
		}
		{
			Energy minNominalDamage = Energy(0.1f);
			damageBoost.value = max(damageBoost.value, minNominalDamage - chamberSize.value.adjusted_plus_one(damageCoef.value));
		}

		if (auto* gd = GameDefinition::get_chosen())
		{
			if (auto* wcm = gd->get_weapon_core_modifiers())
			{
				if (auto* fc = wcm->get_for_core(coreKind.value))
				{
					if (fc->noContinuousFire)
					{
						continuousFire = false;
						SET_AFFECTION(continuousFire, corePart, WeaponStatAffection::Special);
					}
					if (fc->singleSpecialProjectile)
					{
						singleSpecialProjectile = true;
						SET_AFFECTION(singleSpecialProjectile, corePart, WeaponStatAffection::Special);
						autoProjectile.count = 1;
						autoProjectile.relativeVelocity = Vector3::yAxis;
						autoProjectile.spread = 0.0f;
					}
				}
			}
		}

	}

	{	// get clour
		if (projectile.colour.is_set())
		{
			autoProjectile.colour = projectile.colour.get();
		}
		else if (!autoProjectile.colour.is_set())
		{
			if (coreKind.value == WeaponCoreKind::None)
			{
				autoProjectile.colour = Colour::white;
			}
			else if (coreKind.value == WeaponCoreKind::Plasma)
			{
				autoProjectile.colour = RegisteredColours::get_colour(NAME(red_plasma));
			}
			else if (coreKind.value == WeaponCoreKind::Corrosion)
			{
				autoProjectile.colour = RegisteredColours::get_colour(NAME(corrosion));
			}
			else if (coreKind.value == WeaponCoreKind::Lightning)
			{
				autoProjectile.colour = RegisteredColours::get_colour(NAME(lightning));
			}
			else
			{
				todo_implement;
			}
		}
	}

	{	// auto and sane values
		usingMagazine = !magazineSize.value.is_zero();

		if (chamberSize.value.is_zero())
		{
			warn(TXT("no chamber size"));
			chamberSize.value = Energy(1);
		}

		if (maxStorage.value.is_zero())
		{
			//warn(TXT("no (max) storage")); // no longer an issue as we use a different storage anyway
			maxStorage.value = Energy(200);
		}
		else if (!_pilgrimWeapon)
		{
			// make non pilgrim weapon have a greater storage as storages are now much smaller
			maxStorage.value = maxStorage.value * 2 + Energy(100);
		}

		if (usingMagazine && magazineOutputSpeed.value.is_zero())
		{
			warn(TXT("no magazine output speed"));
			magazineOutputSpeed.value = DEFAULT_MAGAZINE_OUTPUT_SPEED ;
		}

		if (storageOutputSpeed.value.is_zero())
		{
			warn(TXT("no storage output speed"));
			storageOutputSpeed.value = Energy(20);
		}
	}
}
	
void GunStats::calculate_stats_update_projectile(Gun const* _gun, PilgrimStatsInfoHand::MainEquipment const* _statsME)
{
	if (_gun)
	{
		// continue from what we set in "auto"
		autoProjectile.count = _gun->gun_stats_get_auto_projectile().get_count();
		autoProjectile.relativeVelocity = _gun->gun_stats_get_auto_projectile().get_relative_velocity();
		autoProjectile.spread = max(0.0f, _gun->gun_stats_get_auto_projectile().get_spread());
	}
	else if (_statsME)
	{
		autoProjectile.count = _statsME->projectilesPerShot;
		autoProjectile.relativeVelocity = Vector3::yAxis * _statsME->projectileSpeed;
		autoProjectile.spread = max(0.0f, _statsME->projectileSpread);
	}
	else
	{
		an_assert(false, TXT("provide _gun or _statsME, we take values from there!"));
	}

	// fill missing
	if (autoProjectile.count.value < 1)
	{
		warn(TXT("no count for projectile"));
		autoProjectile.count.value = 1;
	}
	if (autoProjectile.relativeVelocity.value.is_zero())
	{
		warn(TXT("no speed for projectile"));
		autoProjectile.relativeVelocity.value = Vector3::yAxis * 10.0f;
	}

	{
		// select a projectile. do it at the end as we depend on the values
		isUsingAutoProjectiles = true; // to force using count and spread above but also for case if we don't have _gun provided, we assume we get the one

		autoProjectile.placeAtSocket = projectile.placeAtSocket;
		ProjectileSelectionSource pss;
		pss.gun = _gun;
		pss.statsME = _statsME;
		if (singleSpecialProjectile.value)
		{
			autoProjectile.type.clear();
			autoProjectile.count = 1;
			autoProjectile.spread = 0.0f;
			isUsingAutoProjectiles = true;
		}
		else
		{
			ProjectileSelectionInfo psi;
			autoProjectile.type = ProjectileSelector::select_for(pss, psi);
			if (psi.numerousAsOne)
			{
				autoProjectile.count = 1;
			}
			if (psi.noSpread)
			{
				autoProjectile.spread = 0.0f;
			}
			if (!autoProjectile.type.is_set())
			{
				// copy default?
				autoProjectile.type = projectile.type;
			}
			isUsingAutoProjectiles = autoProjectile.type.is_set();
		}
	}
}

//

#define RESET(stat) stat.reset()

void GunChosenStats::reset()
{
	RESET(coreKind);
	RESET(damaged);
	RESET(shotCost);
	RESET(totalDamage);
	RESET(damage);
	RESET(continuousDamage);
	RESET(medi);
	RESET(noHitBoneDamage);
	RESET(armourPiercing);
	RESET(antiDeflection);
	RESET(maxDist);
	RESET(kineticEnergyCoef);
	RESET(confuse);
	RESET(magazineSize);
	RESET(storageOutputSpeed);
	RESET(magazineOutputSpeed);
	RESET(rpm);
	RESET(rpmMagazine);
	RESET(numberOfProjectiles);
	RESET(projectileSpeed);
	RESET(projectileSpread);
	RESET(continuousFire);
	RESET(singleSpecialProjectile);
	specialFeatures.clear();
	statInfoLocStrs.clear();
	projectileStatInfoLocStrs.clear();
}

#define CORE_INFO__BOOL(feature) (fc && fc->feature)
#define CORE_INFO__OPT(feature) (fc && fc->feature.is_set())
#define CORE_INFO__BOOL__FULL_NO(feature) (CORE_INFO__BOOL(feature) ? fullCore : noCore)
#define CORE_INFO__OPT__FULL_NO(feature) (CORE_INFO__OPT(feature) ? fullCore : noCore)
#define CORE_INFO__OPT__ADJ_NO(feature) (CORE_INFO__OPT(feature) ? adjCore : noCore)
#define CORE_INFO__OPT__FULL_ADJ_NO(feature) (CORE_INFO__OPT(feature) ? (fc->feature.get() == feature? fullCore : adjCore) : noCore)
#define CORE_INFO__OPT2__FULL_ADJ_NO(feature) (CORE_INFO__OPT(feature) ? (fc->feature.get() == feature.get()? fullCore : adjCore) : noCore)
#define CORE_INFO__NO_CORE(feature) (noCore)
#define DAMAGED_PREFIX(otherwise) (! damagedPrefix.is_empty()? damagedPrefix : (otherwise))
#define COMPARE(stat) (stat.value == _other.stat.value)

template <typename ArrayClass>
bool compare_stat_array(ArrayClass const& _a, ArrayClass const& _b)
{
	if (_a.get_size() != _b.get_size())
	{
		return false;
	}
	auto* a = _a.begin();
	auto* b = _b.begin();
	for_count(int, i, _a.get_size())
	{
		if (a->value != b->value)
		{
			return false;
		}
		++a;
		++b;
	}
	return true;
}

#define COMPARE_ARRAY(stat) compare_stat_array(stat, _other.stat)

bool GunChosenStats::operator ==(GunChosenStats const& _other) const
{
	return COMPARE(coreKind)
		&& COMPARE(damaged)
		&& COMPARE(shotCost)
		&& COMPARE(totalDamage)
		&& COMPARE(damage)
		&& COMPARE(continuousDamage)
		&& COMPARE(medi)
		&& COMPARE(noHitBoneDamage)
		&& COMPARE(armourPiercing)
		&& COMPARE(antiDeflection)
		&& COMPARE(maxDist)
		&& COMPARE(kineticEnergyCoef)
		&& COMPARE(confuse)
		&& COMPARE(magazineSize)
		&& COMPARE(storageOutputSpeed)
		&& COMPARE(magazineOutputSpeed)
		&& COMPARE(rpm)
		&& COMPARE(rpmMagazine)
		&& COMPARE(numberOfProjectiles)
		&& COMPARE(projectileSpeed)
		&& COMPARE(projectileSpread)
		&& COMPARE(continuousFire)
		&& COMPARE(singleSpecialProjectile)
		&& COMPARE_ARRAY(specialFeatures)
		&& COMPARE_ARRAY(statInfoLocStrs)
		&& COMPARE_ARRAY(projectileStatInfoLocStrs)
		;
}

void GunChosenStats::StatInfo::split_multi_lines(List<StatInfo>& _statsInfo)
{
	for_every(l, _statsInfo)
	{
		while (l->text.does_contain('~'))
		{
			int at = l->text.find_first_of('~');
			String addBefore = l->text.get_left(at);
			String prefix;
			if (!addBefore.is_empty() && addBefore[0] == '#')
			{
				int at = addBefore.find_first_of('#', 1);
				if (at != NONE)
				{
					prefix = addBefore.get_left(at + 1);
				}
			}
			StatInfo addBeforeSI;
			addBeforeSI.text = addBefore;
			_statsInfo.insert(for_everys_iterator(l), addBeforeSI);
			l->text = prefix + l->text.get_sub(at + 1);
		}
	}
}


void GunChosenStats::build_overlay_stats_info(OUT_ List<StatInfo>& _statsInfo, bool _shortDamagedInfo, OPTIONAL_ OUT_ OverlayInfo::TextColours* _textColours) const
{
	String fullCore;
	String adjCore;
	String noCore;
	WeaponCoreModifiers::ForCore const* fc = nullptr;
	if (auto* gd = GameDefinition::get_chosen())
	{
		if (auto* wcm = gd->get_weapon_core_modifiers())
		{
			fc = wcm->get_for_core(coreKind.value);
		}
	}
	if (_textColours && fc)
	{
		if (fc->overlayInfoColour.is_set())
		{
			fullCore = TXT("#core#");
			adjCore = TXT("#adjCore#");
			_textColours->add(NAME(core), fc->overlayInfoColour.get());
			_textColours->add(NAME(adjCore), fc->overlayInfoColour.get().with_alpha(0.5f));
		}
	}

	_statsInfo.clear();

	if (! _shortDamagedInfo)
	{
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (fullCore +
			//LOC_STR(WeaponCoreKind::to_localised_string_id_long(coreKind.value))
			(LOC_STR(WeaponCoreKind::to_localised_string_id_long(coreKind.value)) + String::space() + LOC_STR(WeaponCoreKind::to_localised_string_id_icon(coreKind.value))).trim()
		);
		ADD_AFFECTED_BY(_statsInfo.get_last(), coreKind); // affected by core is already here
	}

	String damagedPrefix;

	if (!damaged.value.is_zero())
	{
		if (_textColours)
		{
			Colour tc = RegisteredColours::get_colour(NAME(pilgrim_overlay_weaponPart_damaged));
			if (!tc.is_none())
			{
				damagedPrefix = String(TXT("#damaged#"));
				_textColours->add(NAME(damaged), tc);
			}
		}
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (damagedPrefix + Framework::StringFormatter::format_sentence_loc_str(_shortDamagedInfo ? NAME(lsEquipmentDamaged) : NAME(lsEquipmentDamagedReplace), Framework::StringFormatterParams()
			.add(NAME(value), damaged.value.as_string_percentage())));
	}

	if (! _shortDamagedInfo)
	{
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (DAMAGED_PREFIX(CORE_INFO__OPT__ADJ_NO(damageAdj.value)) +
			Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentDamage), Framework::StringFormatterParams()
				.add(NAME(damage), LOC_STR(NAME(lsCharsDamage)) + damage.value.as_string(2))));
		ADD_AFFECTED_BY(_statsInfo.get_last(), damage); // affected by core is already here
		//
		if (!medi.value.is_zero())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (CORE_INFO__NO_CORE(medi.value) +
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentMedi), Framework::StringFormatterParams()
					.add(NAME(value), LOC_STR(NAME(lsCharsHealth)) + medi.value.as_string(2))
				));
			ADD_AFFECTED_BY(_statsInfo.get_last(), medi);
		}
		//
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (CORE_INFO__OPT__ADJ_NO(chamberSizeAdj.value) +
			Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentCost), Framework::StringFormatterParams()
				.add(NAME(cost), LOC_STR(NAME(lsCharsAmmo)) + shotCost.value.as_string(2))));
		ADD_AFFECTED_BY(_statsInfo.get_last(), shotCost); // affected by core is already here
		//
		_statsInfo.push_back(StatInfo());
		_statsInfo.get_last().text = (
			Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentDamageToCostRatio), Framework::StringFormatterParams()
				.add(NAME(value), LOC_STR(NAME(lsCharsDamage)) + totalDamage.value.div_to_coef(shotCost.value).as_string_percentage(0))));
		//ADD_AFFECTED_BY(_statsInfo.get_last(), damage); // affected by core is already here
		//ADD_AFFECTED_BY(_statsInfo.get_last(), shotCost); // affected by core is already here
		{
			String rpmInfo;
			if (rpmMagazine.value.is_zero())
			{
				rpmInfo = rpm.value.as_string(0);
			}
			else
			{
				rpmInfo = rpmMagazine.value.as_string(0)
					+ String(TXT(" ("))
					+ rpm.value.as_string(0)
					+ String(TXT(")"))
					;
			} 
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentRoundsPerMinute), Framework::StringFormatterParams()
					.add(NAME(value), rpmInfo)));
			ADD_AFFECTED_BY(_statsInfo.get_last(), rpm);
			if (!rpmMagazine.value.is_zero())
			{
				ADD_AFFECTED_BY(_statsInfo.get_last(), rpmMagazine);
			}
		}

		if (!magazineSize.value.is_zero())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentMagazine), Framework::StringFormatterParams()
					.add(NAME(value), ((magazineSize.value / shotCost.value).floored() + Energy::one() /* in chamber */).as_string(0))));
			ADD_AFFECTED_BY(_statsInfo.get_last(), magazineSize);
		}
	#ifdef GUN_STATS__SHOW__NO_HIT_BONE_DAMAGE
		if (noHitBoneDamage)
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (CORE_INFO__BOOL__FULL_NO(noHitBoneDamage) +
				LOC_STR(NAME(lsEquipmentNoHitBoneDamage)));
		}
	#endif
		// special features related to the gun
		for_every(sf, specialFeatures)
		{
			Name locStrId;
			if (sf->value == NAME(supercharger)) locStrId = NAME(lsSpecialFeatureSupercharger);
			if (sf->value == NAME(retaliator)) locStrId = NAME(lsSpecialFeatureRetaliator);
			if (locStrId.is_valid())
			{
				List<String> strs;
				LOC_STR(locStrId).split(String(TXT("~")), strs);
				for_every(str, strs)
				{
					_statsInfo.push_back(StatInfo());
					_statsInfo.get_last().text = (*str);
					ADD_AFFECTED_BY(_statsInfo.get_last(), *sf);
				}
			}
		}
		if (continuousFire.value)
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (LOC_STR(NAME(lsEquipmentContinuousFire)));
			ADD_AFFECTED_BY(_statsInfo.get_last(), continuousFire);
		}
		for_every(lsId, statInfoLocStrs)
		{
			List<String> strs;
			LOC_STR(lsId->value).split(String(TXT("~")), strs);
			for_every(str, strs)
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (*str);
				ADD_AFFECTED_BY(_statsInfo.get_last(), *lsId);
			}
		}
		if (fc && fc->projectileInfo.is_valid() && !fc->projectileInfo.get().is_empty())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (fullCore + fc->projectileInfo.get());
		}
		if (!singleSpecialProjectile.value)
		{
			{
				String speedStr;
				MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
				if (ms == MeasurementSystem::Imperial)
				{
					speedStr = MeasurementSystem::as_feet_speed(projectileSpeed.value);
				}
				if (ms == MeasurementSystem::Metric)
				{
					speedStr = MeasurementSystem::as_meters_speed(projectileSpeed.value, NP, nullptr, 0);
				}
#ifdef PROJECTILE_COUNT_AND_SPEED_IN_ONE_LINE
				{
					_statsInfo.push_back(StatInfo());
					_statsInfo.get_last().text = (CORE_INFO__OPT__ADJ_NO(projectileSpeedAdj.value) +
						Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileCountAndSpeed), Framework::StringFormatterParams()
							.add(NAME(projectileCount), numberOfProjectiles.value)
							.add(NAME(projectileSpeed), speedStr)));
					ADD_AFFECTED_BY(_statsInfo.get_last(), numberOfProjectiles);
					ADD_AFFECTED_BY(_statsInfo.get_last(), projectileSpeed);
				}
#else
				{
					_statsInfo.push_back(StatInfo());
					_statsInfo.get_last().text =
						Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileCount), Framework::StringFormatterParams()
							.add(NAME(projectileCount), numberOfProjectiles.value));
					ADD_AFFECTED_BY(_statsInfo.get_last(), numberOfProjectiles);
				}
				{
					_statsInfo.push_back(StatInfo());
					_statsInfo.get_last().text = (CORE_INFO__OPT__ADJ_NO(projectileSpeedAdj.value) +
						Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileSpeed), Framework::StringFormatterParams()
							.add(NAME(value), speedStr)));
					ADD_AFFECTED_BY(_statsInfo.get_last(), projectileSpeed);
				}
#endif
			}
			if (projectileSpread.value > 0.0f)
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (CORE_INFO__OPT__ADJ_NO(projectileSpreadAdd.value) +
					Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentProjectileSpread), Framework::StringFormatterParams()
						.add(NAME(value), String::printf(TXT("%.0f"), projectileSpread.value) + String::degree())));
				ADD_AFFECTED_BY(_statsInfo.get_last(), projectileSpread);
			}
		}
		// special features related to the projectile(s)
		for_every(sf, specialFeatures)
		{
			Name locStrId;
			if (sf->value == NAME(penetrator)) locStrId = NAME(lsSpecialFeaturePenetrator);
			if (locStrId.is_valid())
			{
				List<String> strs;
				LOC_STR(locStrId).split(String(TXT("~")), strs);
				for_every(str, strs)
				{
					_statsInfo.push_back(StatInfo());
					_statsInfo.get_last().text = (*str);
					ADD_AFFECTED_BY(_statsInfo.get_last(), *sf);
				}
			}
		}
		if (maxDist.value.is_set())
		{
			String str;
			MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
			if (ms == MeasurementSystem::Imperial)
			{
				str = MeasurementSystem::as_feet_inches(maxDist.value.get());
			}
			else
			{
				str = MeasurementSystem::as_meters(maxDist.value.get());
			}
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (CORE_INFO__OPT2__FULL_ADJ_NO(maxDist.value) +
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentMaxDist), Framework::StringFormatterParams()
					.add(NAME(value), str)));
			ADD_AFFECTED_BY(_statsInfo.get_last(), maxDist);
		}
		if (!armourPiercing.value.is_zero())
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (CORE_INFO__OPT__FULL_ADJ_NO(armourPiercing.value) +
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentArmourPiercing), Framework::StringFormatterParams()
					.add(NAME(value), !armourPiercing.value.is_one() ? armourPiercing.value.as_string_percentage(0) : String::empty())).trim());
			ADD_AFFECTED_BY(_statsInfo.get_last(), armourPiercing);
		}
		if (antiDeflection.value > 0.0f)
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (CORE_INFO__OPT__FULL_ADJ_NO(antiDeflection.value) +
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentAntiDeflection), Framework::StringFormatterParams()
					.add(NAME(value), antiDeflection.value < 1.0f ? String::printf(TXT("%.0f%%"), 100.0f * antiDeflection.value) : String::empty())).trim());
			ADD_AFFECTED_BY(_statsInfo.get_last(), antiDeflection);
		}
		if (kineticEnergyCoef.value != 0.0f)
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (CORE_INFO__NO_CORE(kineticEnergyCoef.value) +
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentKineticEnergyCoef), Framework::StringFormatterParams()
					.add(NAME(value), String::printf(TXT("%.2f"), (kineticEnergyCoef.value * WEAPON_NODE_KINETIC_ENERGY_IMPULSE_SHOW)))
					.add(NAME(total), String::printf(TXT("(%.2f)"), (totalDamage.value.as_float() * kineticEnergyCoef.value * WEAPON_NODE_KINETIC_ENERGY_IMPULSE_SHOW)))
				));
			ADD_AFFECTED_BY(_statsInfo.get_last(), kineticEnergyCoef);
		}
		if (confuse.value != 0.0f)
		{
			_statsInfo.push_back(StatInfo());
			_statsInfo.get_last().text = (CORE_INFO__NO_CORE(confuse.value) +
				Framework::StringFormatter::format_sentence_loc_str(NAME(lsEquipmentConfuse), Framework::StringFormatterParams()
					.add(NAME(value), ParserUtils::float_to_string_auto_decimals(confuse.value, 2))
				));
			ADD_AFFECTED_BY(_statsInfo.get_last(), confuse);
		}
		for_every(lsId, projectileStatInfoLocStrs)
		{
			List<String> strs;
			LOC_STR(lsId->value).split(String(TXT("~")), strs);
			for_every(str, strs)
			{
				// additional projectile stat infos are always result of core (as it decides the projectile)
				// hence fullCore here
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (fullCore + *str);
				ADD_AFFECTED_BY(_statsInfo.get_last(), *lsId);
			}
		}
		if (fc && fc->additionalWeaponInfo.is_valid())
		{
			String str = fc->additionalWeaponInfo.get();
			if (!str.is_empty())
			{
				_statsInfo.push_back(StatInfo());
				_statsInfo.get_last().text = (String::empty()); // empty line to separate info about core
				List<String> lines;
				str.split(String(TXT("~")), lines);
				for_every(line, lines)
				{
					_statsInfo.push_back(StatInfo());
					_statsInfo.get_last().text = (fullCore + *line);
					//if (for_everys_index(line) == 0)
					{
						ADD_AFFECTED_BY_OVERRIDE(_statsInfo.get_last(), coreKind, WeaponStatAffection::Special); // affected by core, set
					}
				}
			}
		}
	} // ! _shortDamageInfo

	StatInfo::split_multi_lines(REF_ _statsInfo);
}

//

bool GunProjectile::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	count.value = _node->get_int_attribute_or_from_child(TXT("count"), count.value);
	result &= type.load_from_xml(_node, TXT("temporaryObjectType"), _lc);
	placeAtSocket = _node->get_name_attribute_or_from_child(TXT("placeAtSocket"), placeAtSocket);
	relativeVelocity.value.load_from_xml_child_node(_node, TXT("relativeVelocity"));
	spread.value = _node->get_float_attribute_or_from_child(TXT("dispersionAngle"), spread.value);
	spread.value = _node->get_float_attribute_or_from_child(TXT("spread"), spread.value);
	colour.load_from_xml(_node, TXT("colour"));

	return result;
}

bool GunProjectile::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= type.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool GunVRPulses::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= shoot.load_from_xml(_node, TXT("shoot"));
	result &= chamberLoaded.load_from_xml(_node, TXT("chamberLoaded"));

	return result;
}

//

REGISTER_FOR_FAST_CAST(Gun);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new Gun(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new GunData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & Gun::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("gun")), create_module, create_module_data);
}

Gun::Gun(Framework::IModulesOwner* _owner)
: base( _owner )
, weaponSetup(Persistence::access_current_if_exists())
{
	normalColour = RegisteredColours::get_colour(NAME(weapon_normal));
	lockedColour = RegisteredColours::get_colour(NAME(weapon_locked));
	readyColour = RegisteredColours::get_colour(NAME(weapon_ready));
	superchargerColour = RegisteredColours::get_colour(NAME(gun_supercharger));
	retaliatorColour = RegisteredColours::get_colour(NAME(gun_retaliator));
}

Gun::~Gun()
{
}

void Gun::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	gunData = fast_cast<GunData>(_moduleData);
	if (gunData)
	{
		projectile = gunData->get_projectile();
		vrPulses = gunData->get_vr_pulses();
	}

	allowEnergyDeposit = _moduleData->get_parameter<bool>(this, NAME(allowEnergyDeposit), allowEnergyDeposit);

	normalColour = _moduleData->get_parameter<Colour>(this, NAME(normalColour), normalColour);
	lockedColour = _moduleData->get_parameter<Colour>(this, NAME(lockedColour), lockedColour);
	readyColour = _moduleData->get_parameter<Colour>(this, NAME(readyColour), readyColour);

	muzzleHotEnergyFull = _moduleData->get_parameter<float>(this, NAME(muzzleHotEnergyFull), muzzleHotEnergyFull);
	muzzleSE.coolDownTime = _moduleData->get_parameter<float>(this, NAME(muzzleHotCoolDownTime), muzzleSE.coolDownTime);

	retaliatorSE.upTime = 0.4f;
	retaliatorSE.coolDownTime = 0.4f;
	superchargerSE.coolDownTime = 0.5f;

	superchargerColour = _moduleData->get_parameter<Colour>(this, NAME(superchargerColour), superchargerColour);
	retaliatorColour = _moduleData->get_parameter<Colour>(this, NAME(retaliatorColour), retaliatorColour);

	bool useRandomParts = _moduleData->get_parameter<bool>(this, NAME(useRandomParts), false);

	if (weaponSetup.is_empty() && gunData && !gunData->get_weapon_setup().get_parts().is_empty())
	{
		// build parts if not already provided
		for_every(dp, gunData->get_weapon_setup().get_parts())
		{
			if (auto* wp = dp->part.get())
			{
				weaponSetup.add_part(dp->at, wp, gunData->should_use_non_randomised_parts());
			}
		}

		// setup with parts will be called on initialise
	}
	if (useRandomParts || (gunData && gunData->should_fill_with_random_parts()))
	{
		Array<WeaponPartType const*> useParts;
		{
			Framework::LibraryName forcePart = _moduleData->get_parameter<Framework::LibraryName>(this, NAME(forcePart), Framework::LibraryName::invalid());
			if (forcePart.is_valid())
			{
				WeaponPartType* wpt = nullptr;
				if (auto* lib = Library::get_current_as<Library>())
				{
					wpt = lib->get_weapon_part_types().find(forcePart);
				}
				if (wpt)
				{
					useParts.push_back(wpt);
				}
			}
		}
		fill_with_random_parts(&useParts);
	}

	// don't call create_parts, it would fail anyway and it is done when initialising the module
}

Energy Gun::get_max_storage() const
{
	return maxStorage.value;
}

#define OWNER_HAND get_owner(), get_main_equipment_hand()
#define OWNER_HAND_PASS _forOwner, _forHand
#define OWNER_HAND_USE_PARAMS _forOwner ? _forOwner : get_owner(), _forHand.get(get_main_equipment_hand())

Energy Gun::get_chamber_size(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const & _forHand) const
{
	return chamberSize.value.adjusted_plus_one(PilgrimBlackboard::get_main_equipment_chamber_size_coef_modifier_for(OWNER_HAND_USE_PARAMS));
}

EnergyCoef Gun::get_armour_piercing(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	return clamp(armourPiercing.value + PilgrimBlackboard::get_main_equipment_armour_piercing_modifier_for(OWNER_HAND_USE_PARAMS), EnergyCoef::zero(), EnergyCoef::one());
}

float Gun::get_anti_deflection(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	return clamp(antiDeflection.value + PilgrimBlackboard::get_main_equipment_anti_deflection_modifier_for(OWNER_HAND_USE_PARAMS), 0.0f, 1.0f);
}

Optional<float> Gun::get_max_dist(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	if (maxDist.value.is_set())
	{
		return max(0.1f, maxDist.value.get() + PilgrimBlackboard::get_main_equipment_anti_deflection_modifier_for(OWNER_HAND_USE_PARAMS));
	}
	return NP;
}

float Gun::get_kinetic_energy_coef(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	return kineticEnergyCoef.value;
}

float Gun::get_confuse(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	return confuse.value;
}

EnergyCoef Gun::get_medi_coef(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	return mediCoef.value;
}

bool Gun::is_using_magazine(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	return usingMagazine || PilgrimBlackboard::get_main_equipment_using_magazine(OWNER_HAND_USE_PARAMS);
}

float Gun::get_magazine_cooldown() const
{
	if (is_using_magazine())
	{
		return magazineCooldown.value * (1.0f + PilgrimBlackboard::get_main_equipment_cooldown_coef_modifier_for(OWNER_HAND));
	}
	else
	{
		return 0.0f;
	}
}

Energy Gun::get_magazine_size(Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	if (is_using_magazine(_forOwner, _forHand))
	{
		Energy useMagazineSize = magazineSize.value;
		useMagazineSize = useMagazineSize.adjusted_plus_one(PilgrimBlackboard::get_main_equipment_magazine_size_coef_modifier_for(OWNER_HAND_USE_PARAMS));
		Energy minSize = PilgrimBlackboard::get_main_equipment_magazine_min_size_for(OWNER_HAND_USE_PARAMS);
		int minShotCount = PilgrimBlackboard::get_main_equipment_magazine_min_shot_count_for(OWNER_HAND_USE_PARAMS);
		useMagazineSize = max(useMagazineSize, minSize);
		if (minShotCount > 0)
		{
			auto chamberSize = get_chamber_size(_forOwner, _forHand);
			useMagazineSize = max(useMagazineSize, chamberSize * minShotCount);
		}
		return useMagazineSize;
	}
	else
	{
		return Energy::zero();
	}
}

Energy Gun::get_storage_output_speed() const
{
	return storageOutputSpeed.value.adjusted_plus_one(PilgrimBlackboard::get_main_equipment_storage_output_speed_coef_modifier_for(OWNER_HAND))
		+ PilgrimBlackboard::get_main_equipment_storage_output_speed_add_modifier_for(OWNER_HAND);
}

Energy Gun::get_magazine_output_speed() const
{
	if (is_using_magazine())
	{
		Energy useMagazineOutputSpeed = magazineOutputSpeed.value;
		if (useMagazineOutputSpeed.is_zero())
		{
			useMagazineOutputSpeed = DEFAULT_MAGAZINE_OUTPUT_SPEED;
		}
		return useMagazineOutputSpeed.adjusted_plus_one(PilgrimBlackboard::get_main_equipment_magazine_output_speed_coef_modifier_for(OWNER_HAND))
			+ PilgrimBlackboard::get_main_equipment_magazine_output_speed_add_modifier_for(OWNER_HAND);
	}
	else
	{
		return Energy::zero();
	}
}

Framework::TemporaryObjectType * Gun::get_projectile_type() const
{
	return get_projectile().get_type();
}

Energy Gun::get_projectile_damage(bool _nominal, OPTIONAL_ OUT_ Energy* _totalDamage, OPTIONAL_ OUT_ Energy* _medi, Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	Energy totalDamage = (((_nominal? get_chamber_size(OWNER_HAND_PASS) : chamber) + baseDamageBoost.value).adjusted_plus_one(damageCoef.value) + damageBoost.value).adjusted(EnergyCoef::one() - damaged.value);
	totalDamage = totalDamage.adjusted(fromCoreDamageCoef.value);
	totalDamage = totalDamage.adjusted_plus_one(PilgrimBlackboard::get_main_equipment_damage_coef_modifier_for(OWNER_HAND_USE_PARAMS));
	Energy medi = totalDamage.adjusted(mediCoef.value);
	totalDamage = totalDamage - medi;
	Energy damagePerProjectile = totalDamage / get_number_of_projectiles();
	assign_optional_out_param(_totalDamage, totalDamage);
	assign_optional_out_param(_medi, medi);
	return damagePerProjectile;
}

template <typename Class>
void Gun::add_projectile_damage_affected_by(REF_ WeaponStatInfo<Class>& _stat) const
{
	ADD_AFFECTED_BY(_stat, chamberSize);
	ADD_AFFECTED_BY(_stat, baseDamageBoost);
	ADD_AFFECTED_BY(_stat, damageCoef);
	ADD_AFFECTED_BY(_stat, damageBoost);
	//ADD_AFFECTED_BY(_stat, damaged);
	ADD_AFFECTED_BY(_stat, fromCoreDamageCoef);
	ADD_AFFECTED_BY_NEG(_stat, mediCoef);
}

int Gun::get_number_of_projectiles() const
{
	int projectileCount = max(1, get_projectile().get_count());
	return projectileCount;
}

float Gun::get_projectile_speed() const
{
	return get_projectile().get_relative_velocity().length();
}

float Gun::get_projectile_spread() const
{
	return get_projectile().get_spread();
}

void Gun::reset()
{
	base::reset();

	mayCreateParts = false;

	update_energy_state_from_user();
}

void Gun::fill_with_random_parts(Array<WeaponPartType const*> const* _usingWeaponParts)
{
	weaponSetup.fill_with_random_parts_at(WeaponPartAddress(), (gunData? gunData->should_use_non_randomised_parts() : NP), _usingWeaponParts);
	create_parts();
}

void Gun::setup_with_weapon_setup(WeaponSetup const& _setup)
{
	weaponSetup.copy_from(_setup);

	create_parts();
}

void Gun::request_async_create_parts(bool _changeAsSoonAsPossible)
{
	requestedAsyncCreatingParts = true;
	SafePtr<Framework::IModulesOwner> ownerCheck(get_owner());
	Game::get_as<Game>()->add_async_world_job_top_priority(TXT("create parts"),
		[this, ownerCheck, _changeAsSoonAsPossible]()
		{
			if (auto* imo = ownerCheck.get())
			{
				Framework::BlockDestruction blockDestruction(fast_cast<Framework::IDestroyable>(imo));

				create_parts(_changeAsSoonAsPossible);
			}
		});
}

void Gun::log_stats(LogInfoContext& _log) const
{
	Energy totDamage;
	Energy medi;
	Energy projDamage = get_projectile_damage(true, &totDamage, &medi);
	_log.log(TXT("core kind              : %S"), WeaponCoreKind::to_char(coreKind.value));
	_log.log(TXT("chamber size           : %S"), get_chamber_size().as_string().to_char());
	_log.log(TXT("max storage            : %S"), get_max_storage().as_string().to_char());
	_log.log(TXT("storage output speed   : %S"), get_storage_output_speed().as_string().to_char());
	_log.log(TXT("damage (sin.proj)      : %S"), projDamage.as_string().to_char());
	_log.log(TXT("damage (total)         : %S"), totDamage.as_string().to_char());
	_log.log(TXT("medi                   : %S"), medi.as_string().to_char());
	_log.log(TXT("base damage boost      : %S"), baseDamageBoost.value.as_string().to_char());
	_log.log(TXT("damage coef            : %S"), damageCoef.value.as_string_percentage_relative().to_char());
	_log.log(TXT("damage boost           : %S"), damageBoost.value.as_string().to_char());
	_log.log(TXT("continuous time        : %.3f"), continuousDamageTime.value);
	_log.log(TXT("continuous min time    : %.3f"), continuousDamageMinTime.value);
	_log.log(TXT("hit bone damage        : %S"), noHitBoneDamage.value ? TXT("no") : TXT("yes"));
	_log.log(TXT("armour piercing        : %S"), get_armour_piercing().as_string_percentage().to_char());
	_log.log(TXT("magazine size          : %S"), get_magazine_size().as_string().to_char());
	_log.log(TXT("magazine cooldown      : %.3f"), get_magazine_cooldown());
	_log.log(TXT("magazine output speed  : %S"), get_magazine_output_speed().as_string().to_char());
	_log.log(TXT("continuous fire        : %S"), is_continuous_fire()? TXT("continuous") : TXT("single"));
	_log.log(TXT("using projectile       : %S"), singleSpecialProjectile.value ? TXT("single special") : (isUsingAutoProjectiles.value ? TXT("auto") : TXT("prov")));
	if (!singleSpecialProjectile.value)
	{
		_log.log(TXT("projectile type        : %S"), gun_stats_get_used_projectile().get_type()? gun_stats_get_used_projectile().get_type()->get_name().to_string().to_char() : TXT("<none>"));
		_log.log(TXT("projectile speed       : %.3f"), gun_stats_get_used_projectile().get_relative_velocity().length());
		_log.log(TXT("projectile spread      : %.3f"), gun_stats_get_used_projectile().get_spread());
		_log.log(TXT("projectile count       : %i"), gun_stats_get_used_projectile().get_count());
		_log.log(TXT("has muzzle             : %S"), gun_stats_get_used_projectile().hasMuzzle? TXT("muzzle") : TXT("no muzzle"));
	}
}

void Gun::ex__update_stats()
{
	calculate_stats(weaponSetup, is_pilgrim_weapon());
	calculate_stats_update_projectile(this, nullptr);
}

void Gun::set_common_port_values(REF_ SimpleVariableStorage& _variables)
{
	_variables.access<float>(NAME(genHandPortRadius)) = 0.0125f;
	_variables.access<float>(NAME(genHandPortHeight)) = 0.005f;
	_variables.access<float>(NAME(genRestPortRadius)) = 0.005f;
}

void Gun::create_parts(bool _changeAsSoonAsPossible)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	if (!mayCreateParts)
	{
		return;
	}

	if (! weaponSetup.is_empty())
	{	// calculate stuff using parts

		calculate_stats(weaponSetup, is_pilgrim_weapon());
		calculate_stats_update_projectile(this, nullptr);
#ifdef LOG_WEAPON_BEING_CREATED
		{
			LogInfoContext log;
			log_stats(log);
			log.output_to_output();
		}
#endif

		{	// vr pulses
			{	// shooting
				vrPulses.shoot.strength = remap_value(chamberSize.value.as_float(), 1.0f, 8.0f, 0.4f, 1.0f);
				vrPulses.shoot.strengthEnd = remap_value(chamberSize.value.as_float(), 2.0f, 11.0f, 0.3f, 0.8f);
				vrPulses.shoot.length = clamp(0.08f + 0.2f * chamberSize.value.as_float() / 10.0f, 0.05f, 0.4f);
				vrPulses.shoot.frequency = 0.0f;
				vrPulses.shoot.frequencyEnd.clear();
				vrPulses.shoot.make_valid();
			}

			{	// chamber loaded - none by default
				vrPulses.chamberLoaded.strength = 0.0f;
				vrPulses.chamberLoaded.strengthEnd.clear();
				vrPulses.chamberLoaded.length = 0.0f;
				vrPulses.chamberLoaded.frequency = 0.0f;
				vrPulses.chamberLoaded.frequencyEnd.clear();
				if (!is_using_magazine())
				{
					// if not using magazine, every time chamber is filled, issue a pulse
					vrPulses.chamberLoaded.strength = 0.1f;
					vrPulses.chamberLoaded.strengthEnd = 0.05f;
					vrPulses.chamberLoaded.length = 0.1f;
				}
				vrPulses.chamberLoaded.make_valid();
			}
		}
	}

	if (get_owner()->get_appearances().get_size() != 1)
	{
		error(TXT("for now, only one appearance for a gun is supported"));
	}

	{	// setup parameters used by mesh generator
		auto& ovar = get_owner()->access_variables();

		// get raw values, not affected by exms
		ovar.access<float>(NAME(genChamberSize)) = chamberSize.value.as_float();
		ovar.access<float>(NAME(genDamageBase)) = ((chamberSize.value + baseDamageBoost.value).adjusted_plus_one(damageCoef.value) + damageBoost.value).adjusted(fromCoreDamageCoef.value).as_float();
		ovar.access<float>(NAME(genStorageSize)) = hardcoded 50.0f /* we should remove this altogether as storage is now part of the weapon */;
		ovar.access<float>(NAME(genMagazineSize)) = magazineSize.value.as_float();
		ovar.access<float>(NAME(genProjectileSpeed)) = autoProjectile.get_relative_velocity().length();
		ovar.access<float>(NAME(genProjectileSpread)) = autoProjectile.get_spread();
		ovar.access<bool>(NAME(genHasMuzzle)) = autoProjectile.hasMuzzle;
		// these are hardcoded but can be anything, they are mostly to scale things
		ovar.access<float>(NAME(genMaxGunHeight)) = 0.06f;
		ovar.access<float>(NAME(genMaxGunWidth)) = 0.08f;
		ovar.access<float>(NAME(genMaxGunForward)) = autoProjectile.hasMuzzle? 0.12f : 0.16f;
		ovar.access<float>(NAME(genMaxGunForwardTotal)) = 0.20f;
		ovar.access<float>(NAME(genMaxGunBackward)) = -0.08f;
		set_common_port_values(REF_ ovar);

		// rest points - always
		ovar.access<bool>(NAME(withLeftRestPoint)) = true;
		ovar.access<bool>(NAME(withRightRestPoint)) = true;
		if (auto* pi = PilgrimageInstance::get())
		{
			if (auto* p = pi->get_pilgrimage())
			{
				ovar.set_from(p->get_equipment_parameters());
			}
		}
		{
			todo_multiplayer_issue(TXT("we just get player here"));
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
					{
						if (auto* pd = mp->get_pilgrim_data())
						{
							ovar.set_from(pd->get_equipment_parameters());
						}
					}
				}
			}
		}
	}

	if (auto* ap = get_owner()->get_appearance())
	{
#ifdef INSPECT_WEAPON_CREATION
		output(TXT("generate mesh"));
		output(TXT("for \"%S\""), get_owner()->ai_get_name().to_char());
		output(TXT("of type \"%S\""), get_owner()->get_as_object()->get_object_type()->get_name().to_string().to_char());
#endif
		ap->set_setup_mesh_generator_request([this](Framework::MeshGeneratorRequest& _request)
		{
			struct VariablesSource
			{
				SimpleVariableStorage const* variables;
				int priority;
				tchar const* name;
				VariablesSource() {}
				VariablesSource(WeaponPart const* wp)
				{
					variables = &wp->get_mesh_generation_parameters();
					priority = wp->get_mesh_generation_parameters_priority();
					name = wp->get_weapon_part_type()->get_name().get_name().to_char();
				}
				static int compare(void const* _a, void const* _b)
				{
					// most important at the end
					VariablesSource const* a = plain_cast<VariablesSource>(_a);
					VariablesSource const* b = plain_cast<VariablesSource>(_b);
					if (a->priority < b->priority) return A_BEFORE_B;
					if (a->priority > b->priority) return B_BEFORE_A;
					return String::compare_tchar_icase_sort(a->name, b->name);
				}
			};
			Array<VariablesSource> variableSources;
			for_every(part, weaponSetup.get_parts())
			{
				variableSources.push_back(VariablesSource(part->part.get()));
			}

			sort(variableSources);

			meshGenerationParameters.clear();
			for_every(vs, variableSources)
			{
				meshGenerationParameters.set_from(*vs->variables);
			}

			_request.using_parameters(meshGenerationParameters);

			_request.with_include_stack_processor([this](Framework::MeshGeneration::GenerationContext const& _context)
			{
				for_every(part, weaponSetup.get_parts())
				{
					if (part->at == _context.get_include_stack())
					{
						if (auto* p = part->part.get())
						{
							if (auto* wpt = p->get_weapon_part_type())
							{
								Random::Generator rg = get_owner()->get_individual_random_generator();
								rg.advance_seed(part->at.as_index_number(), 0);
								return wpt->get_mesh_generator(rg.get_int());
							}
						}
					}
				}
				return (Framework::MeshGenerator*)nullptr;
			});
		});
		ap->recreate_mesh_with_mesh_generator(_changeAsSoonAsPossible);
	}

}

void Gun::initialise()
{
	base::initialise();

	mayCreateParts = true;

	create_parts();

	if (auto * em = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		{
			auto& layer = em->emissive_access(NAME(normal), 0);
			layer.material_index(0);
			layer.packed_array_index(0);
			layer.set_colour(normalColour);
			layer.set_power(0.5f);
		}
		{
			auto& layer = em->emissive_access(NAME(muzzle), 0);
			layer.material_index(1);
			layer.packed_array_index(1);
			layer.set_colour(Colour::black);
			layer.set_power(0.0f); // default - no
		}
		{
			auto& layer = em->emissive_access(NAME(muzzleHot), 0);
			layer.material_index(1);
			layer.packed_array_index(1);
			layer.set_colour(get_projectile().colour.get(Colour::white));
			layer.set_power(0.0f); // set each frame
		}
		{
			auto& layer = em->emissive_access(NAME(energyStorage), 0);
			layer.material_index(2);
			layer.packed_array_index(2);
			layer.set_colour(get_projectile().colour.get(Colour::white));
			layer.set_power(0.0f); // set each frame
		}
		{
			auto& layer = em->emissive_access(NAME(energyStorageFull), 10);
			layer.material_index(2);
			layer.packed_array_index(2);
			layer.set_colour(Colour::white);
			layer.set_base_colour(Colour::white);
			layer.square(0.2f, 0.0f);
			layer.set_power(1.0f);
			layer.deactivate(0.0f);
		}
		{
			auto& layer = em->emissive_access(NAME(sight), 10);
			layer.material_index(3);
			layer.packed_array_index(3);
			defaultWeaponSightColour.parse_from_string(String(TXT("weapon_sight")));
			layer.set_colour(defaultWeaponSightColour);
			layer.set_base_colour(defaultWeaponSightColour);
			layer.set_power(1.0f);
			layer.activate(0.0f);
		}
		{
			auto& layer = em->emissive_access(NAME(supercharger), 0);
			layer.material_index(4);
			layer.packed_array_index(4);
			layer.set_colour(get_projectile().colour.get(Colour::white));
			layer.set_power(0.0f); // set each frame
		}
		{
			auto& layer = em->emissive_access(NAME(retaliator), 0);
			layer.material_index(5);
			layer.packed_array_index(5);
			layer.set_colour(get_projectile().colour.get(Colour::white));
			layer.set_power(0.0f); // set each frame
		}
	}
}

void Gun::advance_post_move(float _deltaTime)
{
	MEASURE_PERFORMANCE(gun_post_move);

	base::advance_post_move(_deltaTime);

	MODULE_OWNER_LOCK(TXT("Gun::advance_post_move")); // lock to avoid changing user

	MEASURE_PERFORMANCE(gun_post_move_locked);

	Energy prevChamber = chamber;
	Energy prevMagazine = magazine;

	update_energy_state_from_user();

	timeSinceLastShot += _deltaTime;

	bool wasLocked = isLocked;
	bool wasReady = isReady;

	blockInfiniteGeneratingFor = max(0.0f, blockInfiniteGeneratingFor - _deltaTime);

	Energy currentChamberSize = get_chamber_size();
	Energy currentStorageOutputSpeed = get_storage_output_speed();
	Energy currentMagazineSize = is_using_magazine() ? get_magazine_size() : Energy::zero();

	Energy generated = Energy::zero();
	if (get_user()) // generated energy only if used by someone
	{
		Energy couldGenerate = currentStorageOutputSpeed.timed(_deltaTime, REF_ generatedMB);
		generated = couldGenerate;
		if (is_using_magazine())
		{
			generated = min(generated, currentMagazineSize - magazine);
		}
		else
		{
			generated = min(generated, currentChamberSize - chamber);
		}

		if (!GameSettings::get().difficulty.regenerateEnergy)
		{
			if (GameSettings::get().difficulty.weaponInfinite && storage.is_zero() && blockInfiniteGeneratingFor == 0.0f &&
				! GameDirector::get()->is_infinite_ammo_blocked())
			{
				// always generate if infinite and nothing in storage
				storage = max(generated, storage);
			}
			generated = min(generated, storage);
		}
	}

	if (is_using_magazine())
	{
		Energy currentMagazineOutputSpeed = get_magazine_output_speed();
		Energy prevMagazine = magazine;
		if (timeSinceLastShot >= get_magazine_cooldown())
		{
			magazine += generated;
		}
		Energy magazineToChamber = min(magazine, currentMagazineOutputSpeed.timed(_deltaTime, REF_ magazineToChamberMB));
		Energy magazineToChamberMove = max(Energy(0), min(magazineToChamber, currentChamberSize - chamber));
		chamber += magazineToChamberMove;
		magazine -= magazineToChamberMove;
		an_assert(magazine >= Energy(0));
		magazine = clamp(magazine, Energy(0), currentMagazineSize);

		// we could use energy generated to magazine partially
		Energy magazineChange = magazine - prevMagazine;
		generated = min(generated, magazineChange + magazineToChamberMove);
	}
	else
	{
		generated = max(Energy(0), min(generated, currentChamberSize - chamber));
		chamber += generated;
	}

	storage -= generated;
	storage = max(Energy(0), storage);

	isReady = chamber >= currentChamberSize;
	canBeReady = chamber + storage + (is_using_magazine() ? magazine : Energy(0)) >= currentChamberSize;
	chamberJustLoaded = isReady && chamber > prevChamber;

#ifdef DEBUG_ENERGY_STATE
	if (get_user())
	{
		output(TXT("[gun] chamber:%.3f[%.3f] {%S}"), chamber.as_float(), currentChamberSize.as_float(), isReady ? TXT("ready") : TXT("not"));
	}
#endif

	{
		if (_deltaTime > 0.0f)
		{
			if (prevChamber == Energy(0) && chamber > Energy(0))
			{
				if (auto* sound = get_owner()->get_sound())
				{
					sound->stop_sound(NAME(chamberCharge));
				}
				// to restart it
				play_sound_using_parts_or_own(NAME(chamberCharge));
			}
			else if (chamber == prevChamber)
			{
				if (auto* sound = get_owner()->get_sound())
				{
					sound->stop_sound(NAME(chamberCharge));
				}
			}
			bool usingMagazineNow = is_using_magazine();
			if (chamber == currentChamberSize &&
				prevChamber < currentChamberSize)
			{
				bool playedSound = false;
				bool spawnedTO = false;
				if (usingMagazineNow)
				{
					playedSound |= play_sound_using_parts_or_own(NAME(chamberChargedWithMagazine)) != 0;
					spawnedTO |= spawn_temporary_objects_using_parts_or_own(NAME(chamberChargedWithMagazine)) != 0;
				}
				else
				{
					playedSound |= play_sound_using_parts_or_own(NAME(chamberChargedNoMagazine)) != 0;
					spawnedTO |= spawn_temporary_objects_using_parts_or_own(NAME(chamberChargedNoMagazine)) != 0;
				}
				if (!playedSound)
				{
					play_sound_using_parts_or_own(NAME(chamberCharged));
				}
				if (!spawnedTO)
				{
					spawn_temporary_objects_using_parts_or_own(NAME(chamberCharged));
				}
			}
			if (usingMagazineNow &&
				magazine == currentMagazineSize &&
				prevMagazine < currentMagazineSize)
			{
				play_sound_using_parts_or_own(NAME(magazineCharged));
				spawn_temporary_objects_using_parts_or_own(NAME(magazineCharged));
			}
		}
	}

	isLocked = false;
	/*
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* pilgrimage = fast_cast<GameModes::Pilgrimage>(game->get_mode()))
		{
			if (pilgrimage->has_pilgrimage_instance())
			{
				auto & pilgrimageInstance = pilgrimage->access_pilgrimage_instance();
				isLocked = !pilgrimageInstance.is_current_open();
			}
		}
	}
	*/

	{
		muzzleSE.upTarget = clamp(muzzleSE.upTarget, 0.0f, 1.5f);
		muzzleSE.update(_deltaTime);

		//

		superchargerSE.update(_deltaTime);

		//

		bool hasRetaliator = false;
		for_every(sf, specialFeatures) if (sf->value == NAME(retaliator))
		{
			if (auto* mp = get_user())
			{
				if (auto* h = mp->get_owner()->get_custom<CustomModules::Health>())
				{
					Energy ch = h->get_health();
					Energy mh = h->get_max_health();

					retaliatorSE.target = clamp(1.0f - ch.div_to_float(mh), 0.0f, 1.0f);
					hasRetaliator = true;
				}
			}
		}
		if (!hasRetaliator)
		{
			retaliatorSE.target.clear();
		}
		retaliatorSE.update(_deltaTime);
	}

	if (auto * em = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		if (isLocked ^ wasLocked)
		{
			if (isLocked)
			{
				auto & layer = em->emissive_access(NAME(locked), 30);
				layer.material_index(0);
				layer.packed_array_index(0);
				layer.set_colour(lockedColour);
				layer.set_power(2.0f);
				layer.activate(0.005f);
			}
			else
			{
				em->emissive_deactivate(NAME(locked), 0.1f);
			}
		}
		if (isReady ^ wasReady)
		{
			if (isReady)
			{
				auto & layer = em->emissive_access(NAME(ready), 10);
				layer.material_index(0);
				layer.packed_array_index(0);
				layer.set_colour(readyColour);
				layer.set_power(2.0f);
				layer.activate(0.005f);
			}
			else
			{
				em->emissive_deactivate(NAME(ready), 0.1f);
			}
		}
		{
			auto& layer = em->emissive_access(NAME(sight), 10);
			Colour c = defaultWeaponSightColour;
			if (! singleSpecialProjectile.value && sightColourSameAsProjectile)
			{
				c = gun_stats_get_used_projectile().colour.get(c);
			}
			layer.set_colour(c);
			layer.set_base_colour(c);
		}
		if (muzzleSE.state > 0.0f)
		{
			auto& layer = em->emissive_access(NAME(muzzleHot), 0);
			Colour c = get_projectile().colour.get(Colour::white);
			layer.set_colour(c);
			layer.set_base_colour(c.mul_rgb(0.8f));
			float useMH = clamp(muzzleSE.state, 0.0f, 1.0f);
			useMH = 1.0f - sqr(1.0f - useMH);
			layer.set_power(useMH);
			layer.activate(0.1f);
		}
		else
		{
			em->emissive_deactivate(NAME(muzzleHot), 0.2f);
		}
		if (superchargerSE.state > 0.0f)
		{
			auto& layer = em->emissive_access(NAME(supercharger), 0);
			Colour c = superchargerColour;
			layer.set_colour(c);
			layer.set_base_colour(c.mul_rgb(1.2f));
			float useMH = clamp(superchargerSE.state, 0.0f, 1.0f);
			useMH = 1.0f - sqr(1.0f - useMH);
			layer.set_power(useMH);
			layer.activate(0.1f);
		}
		else
		{
			em->emissive_deactivate(NAME(supercharger), 0.2f);
		}
		if (retaliatorSE.state > 0.0f)
		{
			auto& layer = em->emissive_access(NAME(retaliator), 0);
			Colour c = retaliatorColour;
			layer.set_colour(c);
			layer.set_base_colour(c.mul_rgb(1.2f));
			float useMH = clamp(retaliatorSE.state, 0.0f, 1.0f);
			useMH = 1.0f - sqr(1.0f - useMH);
			layer.set_power(useMH);
			layer.activate(0.1f);
		}
		else
		{
			em->emissive_deactivate(NAME(retaliator), 0.2f);
		}
		{
			float maxAll = (currentChamberSize + currentMagazineSize + get_max_storage()).as_float();
			float target = maxAll != 0.0f ? (chamber + magazine + storage).as_float() / maxAll : 0.0f;
			energyStorageState = blend_to_using_time(energyStorageState, target, 0.2f, _deltaTime);

			auto& layer = em->emissive_access(NAME(energyStorage), 0);
			layer.set_colour(get_projectile().colour.get(Colour::white));
			layer.set_power(clamp(energyStorageState, 0.0f, 1.0f));
			layer.activate(0.1f);

			if (energyStorageStateFullBlinkFor > 0.0f)
			{
				energyStorageStateFullBlinkFor = clamp(energyStorageStateFullBlinkFor, 0.0f, 1.0f);
				energyStorageStateFullBlinkFor = max(0.0f, energyStorageStateFullBlinkFor - _deltaTime);
				if (energyStorageStateFullBlinkFor > 0.0f)
				{
					em->emissive_activate(NAME(energyStorageFull), 0.0f);
				}
				else
				{
					em->emissive_deactivate(NAME(energyStorageFull), 0.0f);
				}
			}
		}
	}
	blockPlayShootFailed = max(0.0f, blockPlayShootFailed - _deltaTime);

	{
		float gunInUseTarget = get_user() ? 1.0f : 0.0f;
		if (auto* pilgrim = get_user())
		{
			if (!pilgrim->does_want_to_hold_equipment(get_hand()))
			{
				gunInUseTarget = 0.0f;
			}
		}
		if (gunInUseTarget < 1.0f)
		{
			auto* imo = get_owner();
			while (imo)
			{
				if (imo->get_custom<CustomModules::ItemHolder>())
				{
					todo_hack(TXT("force open if in item holder, hack? for weapon mixer"));
					gunInUseTarget = 1.0f;
				}
				if (auto* p = imo->get_presence())
				{
					imo = p->get_attached_to();
				}
				else
				{
					break;
				}
			}
		}
		gunInUse = blend_to_using_speed_based_on_time(gunInUse, gunInUseTarget, 0.3f, _deltaTime);

		float sightsOuterRingRadius = 0.004f;
		Colour sightsColour = Colour::red;

		if (auto* ps = PlayerSetup::get_current_if_exists())
		{
			auto& p = ps->get_preferences();
			sightsOuterRingRadius = p.sightsOuterRingRadius;
			sightsColour = p.sightsColour;
		}

		sightsColour.a = 1.0f;

		for_every_ptr(a, get_owner()->get_appearances())
		{
			a->access_controllers_variable_storage().access<float>(NAME(gunInUse)) = gunInUse;
			a->set_shader_param(NAME(sightDotInUse), gunInUse);
			if (gunInUse > 0.0f)
			{
				a->set_shader_param(NAME(sightDotRadiusOuter), sightsOuterRingRadius);
				a->set_shader_param(NAME(sightDotColour), sightsColour.to_vector4());
			}
		}
	}

	{
		blockEmitsDamagedLightnings = max(0.0f, blockEmitsDamagedLightnings - _deltaTime);
		bool shouldLightning = blockEmitsDamagedLightnings == 0.0f &&
			(timeSinceLastShot < 2.0f || (timeInHand.is_set() && timeInHand.get() < 1.0f)) &&
			(!damaged.value.is_zero() && (get_user() != nullptr || get_owner()->get_top_instigator(true) != nullptr));
		if (emitsDamagedLightnings != shouldLightning)
		{
			emitsDamagedLightnings = shouldLightning;
			if (auto* ls = get_owner()->get_custom<::Framework::CustomModules::LightningSpawner>())
			{
				if (emitsDamagedLightnings)
				{
					ls->start(NAME(damaged));
				}
				else
				{
					ls->stop(NAME(damaged));
				}
			}
			if (emitsDamagedLightnings)
			{
				if (auto* to = get_owner()->get_temporary_objects())
				{
					to->spawn(NAME(damagedShot));
				}
			}
		}
	}

	if (false)
	{
		bool shouldLightning = coreKind.value == WeaponCoreKind::Lightning && chamber >= currentChamberSize;
		if (emitsIdleLightnings != shouldLightning)
		{
			emitsIdleLightnings = shouldLightning;
			if (auto* ls = get_owner()->get_custom<::Framework::CustomModules::LightningSpawner>())
			{
				if (emitsIdleLightnings)
				{
					ls->start(NAME(idle));
				}
				else
				{
					ls->stop(NAME(idle));
				}
			}
		}
	}

	update_energy_state_to_user();
}

void Gun::on_change_user(ModulePilgrim* _user, Optional<Hand::Type> const& _hand, ModulePilgrim* _prevUser, Optional<Hand::Type> const& _prevHand)
{
	base::on_change_user(_user, _hand, _prevUser, _prevHand);
	if (_user)
	{
		blockUseEquipmentPressed = 0.2f;
	}

	// drop all energy
	storage = Energy::zero();
	magazine = Energy::zero();
	chamber = Energy::zero();

	if (_user && _user->has_exm_equipped(EXMID::Passive::instant_weapon_charge()))
	{
		// this is to fill chamber and magazine with all energy available
		update_energy_state_from_user();
		chamber = get_chamber_size();
		magazine = get_magazine_size();
		update_energy_state_from_user();
	}
}

void Gun::advance_user_controls()
{
	base::advance_user_controls();

	if (!user)
	{
		return;
	}

	if (chamberJustLoaded)
	{
		chamberJustLoaded = false;
		if (auto* vr = VR::IVR::get())
		{
			auto* userImo = get_user_as_modules_owner();
			if (userImo &&
				Framework::GameUtils::is_local_player(userImo))
			{
				vr->trigger_pulse(Hand::get_vr_hand_index(get_hand()), vrPulses.chamberLoaded);
			}
		}
	}

	bool shoot = false;
	if (is_continuous_fire())
	{
		shoot = user->is_controls_use_equipment_pressed(get_hand());
		if (blockUseEquipmentPressed)
		{
			if (!shoot)
			{
				blockUseEquipmentPressed = max(0.0f, blockUseEquipmentPressed - ::System::Core::get_delta_time());
			}
			shoot = false;
		}
	}
	else
	{
		shoot = user->has_controls_use_equipment_been_pressed(get_hand());
	}
	
	if (shoot)
	{
		MEASURE_PERFORMANCE(shoot);

		MODULE_OWNER_LOCK(TXT("Gun::advance_user_controls  shoot"));

		EnergyCoef superchargerExtraPrice = DEFAULT_SUPERCHARGER_EXTRA_PRICE;

#ifdef DEBUG_ENERGY_STATE
		if (get_user())
		{
			output(TXT("[gun] fire gun? chamber:%.3f {%S}"), chamber.as_float(), isReady? TXT("ready") : TXT("not"));
		}
#endif
		bool readyNow = isReady;
		bool useNominalDamageNow = false;
		Energy nominalFireCost = get_chamber_size();
		if (!isReady)
		{
			for_every(sf, specialFeatures) if (sf->value == NAME(supercharger))
			{
				update_energy_state_from_user();
				Energy currentChamberSize = get_chamber_size();
				Energy missing = max(Energy::zero(), currentChamberSize - chamber).adjusted_plus_one(superchargerExtraPrice);
				readyNow = missing <= (magazine + storage);
				nominalFireCost += max(Energy::zero(), currentChamberSize - chamber); // this means that you will loose energy this way
				update_energy_state_to_user();
				useNominalDamageNow = true; // as we're not filling the chamber
#ifdef DEBUG_ENERGY_STATE
				output(TXT("[gun] super charger used"));
#endif
			}
		}

		if (readyNow)
		{
			FireParams params;
			params.use_nominal_damage_now(useNominalDamageNow);
			params.nominal_fire_cost(nominalFireCost);
			bool gunFired = fire(params);

			if (gunFired)
			{
				/*
				if (auto* game = Game::get_as<Game>())
				{
					if (auto* pilgrimage = fast_cast<GameModes::Pilgrimage>(game->get_mode()))
					{
						if (pilgrimage->has_pilgrimage_instance())
						{
							auto & pilgrimageInstance = pilgrimage->access_pilgrimage_instance();
							if (!pilgrimageInstance.is_current_open())
							{
								Energy putBack = min(chamber, get_max_storage() - storage);
								storage += putBack; // get it back
							}
						}
					}
				}
				*/
				muzzleSE.upTarget = muzzleSE.state + (chamber.as_float() / muzzleHotEnergyFull);
				update_energy_state_from_user();
				if (!isReady)
				{
					for_every(sf, specialFeatures) if (sf->value == NAME(supercharger))
					{
						Energy currentChamberSize = get_chamber_size();
						Energy missing = max(Energy::zero(), currentChamberSize - chamber).adjusted_plus_one(superchargerExtraPrice);
						superchargerSE.upTarget = missing.div_to_float(currentChamberSize);
						{
							Energy v = min(magazine, missing);
							magazine -= v;
							missing -= v;
							chamber += v;
						}
						{
							Energy v = min(storage, missing);
							storage -= v;
							missing -= v;
							chamber += v;
						}
						if (!missing.is_zero())
						{
							warn(TXT("we've cheated a bit here! we didn't have enough ammo in chamber+magazine+storage"));
						}
					}
				}
				if (user && user->has_exm_equipped(EXMID::Passive::energy_retainer()))
				{
					if (hand.is_set())
					{
						float percentage = 0.1f;
						if (!energyRetainerEXM.get())
						{
							energyRetainerEXM = EXMType::find(EXMID::Passive::energy_retainer());
						}
						if (auto* exm = energyRetainerEXM.get())
						{
							percentage = exm->get_custom_parameters().get_value<float>(NAME(retainedPt), percentage);
						}
						user->add_hand_energy_storage(Hand::other_hand(hand.get()), chamber.mul(percentage));
					}
				}
				chamber = Energy(0);
				update_energy_state_to_user();
				timeSinceLastShot = 0.0f;
				playedShotFailed = false;
				blockPlayShootFailed = 0.5f;

				MusicPlayer::request_combat_auto_bump_low();

#ifdef DEBUG_ENERGY_STATE
				if (get_user())
				{
					output(TXT("[gun] fired chamber:%.3f[%.3f]"), chamber.as_float());
				}
#endif
			}

		}
		else
		{
			bool playShootFailed = true;
			if (is_continuous_fire())
			{
				playShootFailed = false;
				if (!playedShotFailed && (!canBeReady || !wasShooting) && blockPlayShootFailed == 0.0f)
				{
					playShootFailed = true;
				}
			}
			if (playShootFailed)
			{
				if (auto * gunObject = get_owner_as_object())
				{
					if (auto* sound = gunObject->get_sound())
					{
						play_sound_using_parts_or_own(NAME(shootFailed));
					}
				}
				playedShotFailed = true;
			}
			if (!canBeReady)
			{
				if (auto* mp = get_user())
				{
					mp->access_overlay_info().tried_to_shoot_empty(get_hand());
				}
			}
		}

		if (Framework::GameUtils::is_local_player(get_user_as_modules_owner()))
		{
			if (calculate_total_ammo() == 0)
			{
				GameLog::get().increase_counter(GameLog::AmmoDepleted);
				GameLog::get().check_for_energy_balance(get_user_as_modules_owner());
			}
		}
	}
	else
	{
		blockPlayShootFailed = 0.0f;
		playedShotFailed = false;
	}
	wasShooting = shoot;
}

bool Gun::fire(FireParams const& _params)
{
	EnergyCoef immediateDamageCoef = EnergyCoef::zero();
	Energy nominalFireCost;
	if (_params.nominalFireCost.is_set())
	{
		nominalFireCost = _params.nominalFireCost.get();
	}
	else
	{
		nominalFireCost = get_chamber_size();
	}

	//

	bool gunFired = false;

	//

	for_every(sf, specialFeatures) if (sf->value == NAME(retaliator))
	{
		if (auto* mp = get_user())
		{
			if (auto* h = mp->get_owner()->get_custom<CustomModules::Health>())
			{
				Energy ch = h->get_health();
				Energy mh = h->get_max_health();

				immediateDamageCoef += (EnergyCoef::one() - ch.as_part_of(mh, true)) * 2;
			}
		}
	}
	if (gunData)
	{
		if (auto* gunObject = get_owner_as_object())
		{
			if (auto* subWorld = gunObject->get_in_sub_world())
			{
				bool isPenetrator = false;
				for_every(sf, specialFeatures) if (sf->value == NAME(penetrator)) { isPenetrator = true; break; }
				int projectileCount = get_number_of_projectiles();
				auto* projectileType = get_projectile_type();
				{
					Energy totalDamage;
					Energy medi;
					Energy damagePerProjectile = get_projectile_damage(_params.useNominalDamageNow, OUT_ &totalDamage, OUT_ &medi);

					if (!medi.is_zero())
					{
						if (auto* mp = get_user())
						{
							if (auto* h = mp->get_owner()->get_custom<CustomModules::Health>())
							{
								MODULE_OWNER_LOCK(TXT("Gun::fire, give medi"));
								h->add_energy(REF_ medi);
								// unused medi is redistributed to totalDamage, so the gun is still useful
								totalDamage += medi;
							}
						}
					}

					totalDamage = totalDamage.adjusted_plus_one(immediateDamageCoef);
					damagePerProjectile = damagePerProjectile.adjusted_plus_one(immediateDamageCoef);

					Energy damageLeft = totalDamage;
					Energy nominalFireCostLeft = nominalFireCost;
					Energy nominalFireCostPerProjectile = nominalFireCost.adjusted(damagePerProjectile.div_to_coef(totalDamage));
					EnergyCoef useArmourPiercing = get_armour_piercing();
					float useAntiDeflection = get_anti_deflection();
					float projectileSpeed = 10.0f;
#ifdef TEST___COLOURS
					{
						auto& pr = get_projectile();
						projectile.colour = Colour::white;
						while (true)
						{
							pr.colour.access().r = Random::get_float(0.2f, 1.0f);
							pr.colour.access().g = Random::get_float(0.2f, 1.0f);
							pr.colour.access().b = Random::get_float(0.2f, 1.0f);
							float maxv = max(pr.colour.access().r, max(pr.colour.access().g, pr.colour.access().b));
							pr.colour.access().r /= maxv;
							pr.colour.access().g /= maxv;
							pr.colour.access().b /= maxv;
							float minv = min(pr.colour.access().r, min(pr.colour.access().g, pr.colour.access().b));
							float newMinV = 0.3f;
							if (minv >= 0.95f) // disallow white ones
							{
								continue;
							}
							pr.colour.access().r = 1.0f + ((pr.colour.access().r - 1.0f) / (minv - 1.0f)) * (newMinV - 1.0f);
							pr.colour.access().g = 1.0f + ((pr.colour.access().g - 1.0f) / (minv - 1.0f)) * (newMinV - 1.0f);
							pr.colour.access().b = 1.0f + ((pr.colour.access().b - 1.0f) / (minv - 1.0f)) * (newMinV - 1.0f);
							break;
						}
					}
#endif
					if (is_single_special_projectile())
					{
						MEASURE_PERFORMANCE(special_projectile);

						if (coreKind.value == WeaponCoreKind::Lightning)
						{
							ShootContext context;
							context.damage = totalDamage;
							context.cost = nominalFireCost;
							context.armourPiercing = useArmourPiercing;
							gunFired = shoot_lightning(REF_ context);
						}
						else
						{
							todo_implement;
						}
					}
					else if (projectileType && gunObject->is_world_active() && gunObject->get_presence() && gunObject->get_presence()->get_in_room())
					{
						MEASURE_PERFORMANCE(using_projectile_type);

						for_count(int, i, projectileCount)
						{
							Energy thisProjectileDamage = damagePerProjectile;
							Energy thisProjectileNominalFireCost = nominalFireCostPerProjectile;
							if (i == projectileCount - 1)
							{
								thisProjectileDamage = damageLeft; // the last one gets all the missing bits
								thisProjectileNominalFireCost = nominalFireCostLeft;
							}
							if (auto* projTempObj = subWorld->get_one_for(projectileType, gunObject->get_presence()->get_in_room()))
							{
								projTempObj->set_creator(gunObject);
								projTempObj->set_instigator(gunObject);
								auto& useProjectile = get_projectile();
								projTempObj->on_activate_place_at(gunObject, useProjectile.get_place_at_socket());
								if (_params.atLocationWS.is_set())
								{
									projTempObj->on_activate_face_towards(_params.atLocationWS.get());
								}
#ifdef DEBUG_SHOOTING_PROJECTILE
								output(TXT("[shooting] to'%p projectile to be placed at \"%S\"'s socket \"%S\""), projTempObj, gunObject->get_name().to_char(), useProjectile.get_place_at_socket().to_char());
#endif
								if (useProjectile.colour.is_set())
								{
									projTempObj->access_variables().access<Colour>(NAME(projectileEmissiveColour)) = useProjectile.colour.get();
								}
								Vector3 velocity = useProjectile.get_relative_velocity();
								if (useProjectile.get_spread() > 0.0f)
								{
									Quat spread = Rotator3(0.0f, 0.0f, Random::get_float(-180.0f, 180.0f)).to_quat().to_world(Rotator3(Random::get_float(0.0f, useProjectile.get_spread()), 0.0f, 0.0f).to_quat());
									Quat velocityDir = velocity.to_rotator().to_quat();
									projectileSpeed = velocity.length();
									velocity = velocityDir.to_world(spread).get_y_axis() * projectileSpeed;
								}
								projTempObj->on_activate_set_relative_velocity(velocity);
								if (auto* mp = projTempObj->get_gameplay_as<ModuleProjectile>())
								{
									auto& mpDamage = mp->access_damage();
									mpDamage.damage = thisProjectileDamage;
									mpDamage.cost = thisProjectileNominalFireCost;
									mpDamage.damageType = WeaponCoreKind::to_damage_type(coreKind.value);
									mpDamage.armourPiercing = useArmourPiercing;
									mpDamage.forceAffectVelocityLinearPerDamagePoint = kineticEnergyCoef.value;
									mpDamage.induceConfussion = confuse.value;
									auto& mpContinuousDamage = mp->access_continuous_damage();
									mpContinuousDamage.damage = Energy::zero();
									mpContinuousDamage.damageType = mpDamage.damageType;
									mpContinuousDamage.time = 0.0f;
									if (continuousDamageMinTime.value > 0.0f)
									{
										mpContinuousDamage.time = continuousDamageMinTime.value;
									}
									mpContinuousDamage.minTime = continuousDamageMinTime.value;
									mpContinuousDamage.armourPiercing = useArmourPiercing;
									mp->set_anti_deflection(useAntiDeflection);
									mp->be_penetrator(isPenetrator);
								}
								for_every_ptr(m, projTempObj->get_movements())
								{
									if (auto* mp = fast_cast<Framework::ModuleMovementProjectile>(m))
									{
										if (auto* a = gunObject->get_appearance())
										{
											Transform socketOS = a->calculate_socket_os(useProjectile.get_place_at_socket());
											Vector3 backLocation = Vector3::zero;
											Vector3 fwdDir = socketOS.get_axis(Axis::Forward);
											// if possible, make it along the socket's forward dir, stop at 0,0,0
											if (Vector3::dot(fwdDir, socketOS.get_translation() - backLocation) > 0.0f)
											{
												Vector3 sockToBack = backLocation - socketOS.get_translation();
												sockToBack = sockToBack.along_normalised(fwdDir);
												backLocation = socketOS.get_translation() + sockToBack;
											}
											mp->request_one_back_check(socketOS.location_to_local(backLocation));
										}
									}
								}
								if (auto* collision = projTempObj->get_collision())
								{
#ifdef DEBUG_SHOOTING_PROJECTILE
									output(TXT("[shooting] to'%p ignore collision with o%p and up to top instigator for 0.05"), projTempObj, gunObject);
#endif
									collision->dont_collide_with_up_to_top_instigator(gunObject, 0.05f);
									if (!fast_cast<Framework::ModuleCollisionProjectile>(collision))
									{
										collision->dont_collide_with_if_attached_to_top(gunObject, 0.05f);
									}
								}
								//
								projTempObj->mark_to_activate();
							}
							damageLeft -= thisProjectileDamage;
							nominalFireCostLeft = max(Energy::zero(), nominalFireCostLeft - thisProjectileNominalFireCost);
							if (!damageLeft.is_positive())
							{
								break;
							}
						}
						gunFired = true;
					}
					if (gunFired)
					{
						MEASURE_PERFORMANCE(sounds_and_temporary_objects);
						auto* userImo = get_user_as_modules_owner();
						if (userImo &&
							Framework::GameUtils::is_local_player(userImo))
						{
							GameStats::get().shot_fired();
							if (auto* vr = VR::IVR::get())
							{
								vr->trigger_pulse(Hand::get_vr_hand_index(get_hand()), vrPulses.shoot);
							}
						}
						{
							gunObject->access_variables().access<float>(NAME(projectileSpeed)) = projectileSpeed;
							gunObject->access_variables().access<float>(NAME(projectileDamage)) = totalDamage.as_float();
						}
						if (auto* sound = gunObject->get_sound())
						{
							int played = 0;
							if (coreKind.value == WeaponCoreKind::Plasma) played = play_sound_using_parts_or_own(NAME(shootPlasma)); else
							if (coreKind.value == WeaponCoreKind::Ion) played = play_sound_using_parts_or_own(NAME(shootIon)); else
							if (coreKind.value == WeaponCoreKind::Corrosion) played = play_sound_using_parts_or_own(NAME(shootCorrosion)); else
							if (coreKind.value == WeaponCoreKind::Lightning) played = play_sound_using_parts_or_own(NAME(shootLightning)); else
								todo_implement
							if (!played)
							{
								todo_note(TXT("we should not rely on generic shoot sound, we may deliberately have no sound or play it by other means"));
								//play_sound_using_parts_or_own(NAME(shoot));
							}
						}
						{
							todo_note(TXT("this is no longer used, we always have flash from the muzzle and we always have the muzzle"));
							int inOrder[] = { WeaponPartType::GunMuzzle, WeaponPartType::GunChassis, WeaponPartType::GunCore, NONE };
							int spawned = 0;
							if (coreKind.value == WeaponCoreKind::Plasma) spawned = spawn_temporary_objects_using_parts_or_own(NAME(muzzleFlashPlasma), inOrder, true); else
							if (coreKind.value == WeaponCoreKind::Ion) spawned = spawn_temporary_objects_using_parts_or_own(NAME(muzzleFlashIon), inOrder, true); else
							if (coreKind.value == WeaponCoreKind::Corrosion) spawned = spawn_temporary_objects_using_parts_or_own(NAME(muzzleFlashCorrosion), inOrder, true); else
							if (coreKind.value == WeaponCoreKind::Lightning) spawned = spawn_temporary_objects_using_parts_or_own(NAME(muzzleFlashLightning), inOrder, true); else
								todo_implement
							if (!spawned)
							{
								todo_note(TXT("we should not rely on generic muzzle flash here, we may deliberately have no muzzle flash"));
								//spawn_temporary_objects_using_parts_or_own(NAME(muzzleFlash), inOrder, true);
							}
						}
						if (!damaged.value.is_zero() && blockEmitsDamagedLightnings == 0.0f)
						{
							if (auto* ls = get_owner()->get_custom<::Framework::CustomModules::LightningSpawner>())
							{
								ls->start(NAME(damaged));
							}
							if (auto* to = get_owner()->get_temporary_objects())
							{
								to->spawn(NAME(damagedShot));
							}
						}

						if (userImo &&
							Framework::GameUtils::is_local_player(userImo))
						{
							PhysicalSensations::SingleSensation::Type psRecoil = PhysicalSensations::SingleSensation::RecoilWeak;
							if (totalDamage >= Energy(20)) psRecoil = PhysicalSensations::SingleSensation::RecoilVeryStrong; else
							if (totalDamage >= Energy(10)) psRecoil = PhysicalSensations::SingleSensation::RecoilStrong; else
							if (totalDamage >= Energy(5)) psRecoil = PhysicalSensations::SingleSensation::RecoilMedium;

							PhysicalSensations::SingleSensation s(psRecoil);
							s.for_hand(get_hand());
							PhysicalSensations::start_sensation(s);
						}
#ifdef OUTPUT_ENERGY_LEVELS
						output(TXT("[gun energy] fired %S + %S + %S = %S"), chamber.as_string_auto_decimals().to_char(), magazine.as_string_auto_decimals().to_char(), storage.as_string_auto_decimals().to_char(),
							(chamber + magazine + storage).as_string_auto_decimals().to_char());
#endif
					}
				}
			}
		}
	}

	return gunFired;
}

bool Gun::shoot_lightning(REF_ ShootContext & _context)
{
	LightningDischarge::Params params;

	params.for_imo(get_owner());
	params.with_start_placement_OS(get_muzzle_placement_os());
	params.with_max_dist(get_max_dist().get(2.0f));
	params.with_setup_damage([this, _context](Damage& dealDamage, DamageInfo& damageInfo)
		{
			dealDamage.damage = _context.damage;
			dealDamage.cost = _context.cost;
			dealDamage.armourPiercing = _context.armourPiercing; // useful against materials
			dealDamage.forceAffectVelocityLinearPerDamagePoint = kineticEnergyCoef.value;
			dealDamage.induceConfussion = confuse.value;

			damageInfo.instigator = get_owner()->get_valid_top_instigator();
		});
	params.with_ray_cast_search_for_objects(true);
	params.ignore_narrative();

	return LightningDischarge::perform(params);
}

Transform Gun::get_muzzle_placement_os() const
{
	if (auto* appearance = get_owner()->get_appearance())
	{
		auto& useProjectile = get_projectile();

		Transform const muzzleOS = appearance->calculate_socket_os(useProjectile.get_place_at_socket());

		return muzzleOS;
	}
	else
	{
		return Transform::identity;
	}
}

int Gun::spawn_temporary_objects_using_parts_or_own(Name const& _name, OPTIONAL_ int* _inOrder, bool _stopOnFirstType)
{
	int howMany = 0;
	if (auto* to = get_owner()->get_temporary_objects())
	{
		tempSpawnedTemporaryObjects.clear();
		if (!weaponSetup.is_empty())
		{
			int inOrder[] = { WeaponPartType::GunMuzzle, WeaponPartType::GunChassis, WeaponPartType::GunCore, WeaponPartType::GunNode, NONE };
			int const* useOrder = _inOrder ? _inOrder : inOrder;
			for (int i = 0;; ++i)
			{
				if (useOrder[i] == NONE)
				{
					break;
				}
				int tryPartType = (WeaponPartType::Type)(useOrder[i]);
				int spawned = 0;
				for_every(part, weaponSetup.get_parts())
				{
					if (auto* p = part->part.get())
					{
						if (auto* wpt = p->get_weapon_part_type())
						{
							if (wpt->get_type() == tryPartType)
							{
								int spawnedNow = to->spawn_all_using(_name, wpt->get_temporary_objects_data(), NP, &tempSpawnedTemporaryObjects);
								spawned += spawnedNow;
								howMany += spawnedNow;
							}
						}
					}
				}
				if (_stopOnFirstType && spawned)
				{
					break;
				}
			}
		}
		if (howMany == 0 || !_stopOnFirstType)
		{
			howMany += to->spawn_all(_name, NP, &tempSpawnedTemporaryObjects);
		}
		{
			auto& useProjectile = get_projectile();
			float projectileSpeed = useProjectile.get_relative_velocity().length();
			Energy totalDamage;
			get_projectile_damage(false, OUT_ & totalDamage);
			for_every_ref(to, tempSpawnedTemporaryObjects)
			{
				if (useProjectile.colour.is_set())
				{
					to->access_variables().access<Colour>(NAME(projectileEmissiveColour)) = useProjectile.colour.get();
				}
				to->access_variables().access<float>(NAME(projectileSpeed)) = projectileSpeed;
				to->access_variables().access<float>(NAME(projectileDamage)) = totalDamage.as_float();
			}
		}
	}
	return howMany;
}

int Gun::play_sound_using_parts_or_own(Name const& _name, OPTIONAL_ int* _inOrder, bool _stopOnFirstType)
{
	int howMany = 0;
	if (auto* sound = get_owner()->get_sound())
	{
		if (!weaponSetup.is_empty())
		{
			int inOrder[] = { WeaponPartType::GunMuzzle, WeaponPartType::GunCore, WeaponPartType::GunChassis, WeaponPartType::GunNode, NONE };
			int const* useOrder = _inOrder? _inOrder : inOrder;
			for (int i = 0;; ++ i)
			{
				if (useOrder[i] == NONE)
				{
					break;
				}
				int tryPartType = (WeaponPartType::Type)(useOrder[i]);
				int played = 0;
				for_every(part, weaponSetup.get_parts())
				{
					if (auto* p = part->part.get())
					{
						if (auto* wpt = p->get_weapon_part_type())
						{
							if (wpt->get_type() == tryPartType)
							{
								if (sound->play_sound_using(_name, wpt->get_sound_data()))
								{
									++played;
									++howMany;
								}
							}
						}
					}
				}
				if (_stopOnFirstType && played)
				{
					break;
				}
			}
		}
		if (howMany == 0 || !_stopOnFirstType)
		{
			if (sound->play_sound(_name))
			{
				++howMany;
			}
		}
	}
	return howMany;
}

bool Gun::is_aiming_at(Framework::IModulesOwner const * imo, Framework::PresencePath const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const
{
	return is_aiming_at_impl(imo, _usePath, OUT_ _distance, _maxOffPath, _maxAngleOff);
}

bool Gun::is_aiming_at(Framework::IModulesOwner const * imo, Framework::RelativeToPresencePlacement const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const
{
	return is_aiming_at_impl(imo, _usePath, OUT_ _distance, _maxOffPath, _maxAngleOff);
}

template <class PathClass>
bool Gun::is_aiming_at_impl(Framework::IModulesOwner const * target, PathClass const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const
{
	if (gunData)
	{
		Framework::IModulesOwner* user = get_user_as_modules_owner();
		Framework::IModulesOwner* owner = get_owner();
		auto const * presence = owner->get_presence();
		if (auto const * appearance = owner->get_appearance())
		{
			auto& useProjectile = get_projectile();
			Transform const projectileStartsAt = presence->get_os_to_ws_transform().to_world(appearance->calculate_socket_os(useProjectile.get_place_at_socket()));
			Vector3 projectilesStartsAtLoc = projectileStartsAt.get_translation();
			Vector3 fliesInDir = projectileStartsAt.vector_to_world(useProjectile.get_relative_velocity()).normal();

			Transform targetPlacement = target->ai_get_placement();
			if (auto const * ownerAI = owner->get_ai())
			{
				targetPlacement = target->get_presence()->get_placement().to_world(ownerAI->get_target_placement_os_for(target));
			}
			else if (auto const * ai = target->get_ai())
			{
				targetPlacement = target->get_presence()->get_placement().to_world(ai->get_me_as_target_placement_os());
			}

			bool canCheck = target->ai_get_room() == presence->get_in_room();
			if (!canCheck && _usePath)
			{
				if (! _usePath->is_there_clear_line())
				{
					return false;
				}
				// handle both cases, as path might lead from owner to target or from target to owner
				an_assert(_usePath->get_owner() == user || _usePath->get_owner() == target);
				an_assert(_usePath->get_target() == user || _usePath->get_target() == target);

				// but first check if we, as a gun are in same room as user
				if (user->get_presence()->get_in_room() != presence->get_in_room())
				{
					// no? check path to attached to, go up until we encounter user
					// if we won't find user, return false
					// on the way, move projectile info from our space to user space (_usePath points at the user!)
					auto* goUp = presence;
					while (goUp)
					{
						if (auto * pPath = goUp->get_path_to_attached_to())
						{
							if (!pPath->is_active())
							{
								return false;
							}
							projectilesStartsAtLoc = pPath->location_from_owner_to_target(projectilesStartsAtLoc);
							fliesInDir = pPath->vector_from_owner_to_target(fliesInDir);
						}
						else
						{
							return false;
						}
						if (auto* attachedTo = goUp->get_attached_to())
						{
							goUp = attachedTo->get_presence();
							if (goUp->get_owner() == user)
							{
								break;
							}
						}
						else
						{
							return false;
						}
					}
					if (!goUp)
					{
						return false;
					}
				}

				if (_usePath->get_owner() == user)
				{
					targetPlacement = _usePath->from_target_to_owner(targetPlacement);
					canCheck = true;
				}
				if (_usePath->get_target() == user)
				{
					targetPlacement = _usePath->from_target_to_owner(targetPlacement);
					canCheck = true;
				}
			}
			if (canCheck)
			{
				float offPath = Segment(projectilesStartsAtLoc, projectilesStartsAtLoc + fliesInDir * 1000.0f).get_distance(targetPlacement.get_translation());
				float dot = Vector3::dot(fliesInDir, (targetPlacement.get_translation() - projectilesStartsAtLoc).normal());
				return offPath < _maxOffPath ||
					dot >= cos_deg(_maxAngleOff);
			}
		}
	}
	
	return false;
}

void Gun::ready_energy_quantum_context(EnergyQuantumContext & _eqc) const
{
	base::ready_energy_quantum_context(_eqc);

	_eqc.ammo_receiver(get_owner());
}

void Gun::process_energy_quantum_ammo(ModuleEnergyQuantum* _eq)
{
	an_assert(_eq->is_being_processed_by_us());

	if (_eq->has_energy())
	{
		MODULE_OWNER_LOCK(TXT("Gun::process_energy_quantum_ammo"));

		if (allowEnergyDeposit)
		{
			Energy currentMaxStorage = get_max_storage();
			Energy addAmmo = min(_eq->get_energy(), currentMaxStorage - storage);
			storage += addAmmo;

			if (storage >= currentMaxStorage)
			{
				energyStorageStateFullBlinkFor = 1000.0f;
			}

			_eq->use_energy(addAmmo);
		}
	}

	base::process_energy_quantum_ammo(_eq);
}

void Gun::on_disassemble(bool _movePartsToInventory)
{
	ModulePilgrim* mp = get_owner()->get_gameplay_for_attached_to<ModulePilgrim>();

	if (mp)
	{
		//remove_all_energy_to_attached(mp->get_owner());

		if (_movePartsToInventory)
		{
			// move other parts to pilgrim inventory
			for_every(part, weaponSetup.get_parts())
			{
				mp->access_pilgrim_inventory().add_inventory_weapon_part(part->part.get());
			}
		}
		weaponSetup.clear();
	}
	
	//

	// update pilgrim setup (and pending too!), if we don't do this, pilgrim will still think it has items
	if (is_main_equipment() && mp)
	{
		mp->update_pilgrim_setup_for(get_owner(), true);
	}

	// cease to exist after we updated pilgrim setup
	get_owner()->cease_to_exist(false);
}

void Gun::remove_weapon_part_to_pilgrim_inventory(WeaponPartAddress const& _at)
{
	ModulePilgrim* mp = get_owner()->get_gameplay_for_attached_to<ModulePilgrim>();
	weaponSetup.remove_part(_at);
	if (! weaponSetup.is_valid())
	{
		on_disassemble();
	}
	else
	{
		request_async_create_parts();
	
		// update pilgrim setup (and pending too!), if we don't do this, pilgrim will still think it has items
		if (is_main_equipment() && mp)
		{
			mp->update_pilgrim_setup_for(get_owner(), true);
		}
	}
}

#define COPY_AFFECTED_BY_FROM_EX_OVERRIDE(what, from, _how) \
	{ \
		for_every(a, from.affectedBy) \
		{\
			if (what.affectedBy.has_place_left()) \
			{ \
				WeaponsStatAffectedByPart wsap = *a; \
				wsap.how = _how; \
				what.affectedBy.push_back(wsap); \
			} \
		} \
	}

#define COPY_AFFECTED_BY_FROM(what, from) \
	{ \
		for_every(a, from.affectedBy) \
		{\
			if (gs.what.affectedBy.has_place_left()) \
			{ \
				gs.what.affectedBy.push_back(*a); \
			} \
		} \
	}

#define COPY_AFFECTED_BY_FROM_NEG(what, from) \
	{ \
		for_every(a, from.affectedBy) \
		{\
			if (gs.what.affectedBy.has_place_left()) \
			{ \
				gs.what.affectedBy.push_back(*a); \
				auto& l = gs.what.affectedBy.get_last(); \
				l.how = WeaponStatAffection::negate_effect(l.how); \
			} \
		} \
	}

#define COPY_AFFECTED_BY(what) COPY_AFFECTED_BY_FROM(what, what);

void Gun::build_chosen_stats(REF_ GunChosenStats & gs, bool _fillAffection, Framework::IModulesOwner* _forOwner, Optional<Hand::Type> const& _forHand) const
{
	gs.coreKind = get_core_kind();
	if (_fillAffection) COPY_AFFECTED_BY(coreKind);
	gs.damaged = get_damaged();
	if (_fillAffection) COPY_AFFECTED_BY(damaged);
	gs.shotCost = get_chamber_size(_forOwner, _forHand);
	if (_fillAffection) COPY_AFFECTED_BY_FROM(shotCost, chamberSize);
	get_projectile_damage(true, &gs.totalDamage.value, &gs.medi.value, _forOwner, _forHand);
	add_projectile_damage_affected_by(gs.totalDamage);
	if (_fillAffection) COPY_AFFECTED_BY_FROM(medi, mediCoef);
	{
		gs.damage = gs.totalDamage;
		gs.continuousDamage = Energy::zero();
	}
	gs.noHitBoneDamage = noHitBoneDamage;
	gs.armourPiercing = get_armour_piercing(_forOwner, _forHand);
	if (_fillAffection) COPY_AFFECTED_BY(armourPiercing);
	gs.antiDeflection = get_anti_deflection(_forOwner, _forHand);
	if (_fillAffection) COPY_AFFECTED_BY(antiDeflection);
	gs.maxDist = get_max_dist(_forOwner, _forHand);
	if (_fillAffection) COPY_AFFECTED_BY(maxDist);
	gs.kineticEnergyCoef = get_kinetic_energy_coef(_forOwner, _forHand);
	if (_fillAffection) COPY_AFFECTED_BY(kineticEnergyCoef);
	gs.confuse = get_confuse(_forOwner, _forHand);
	if (_fillAffection) COPY_AFFECTED_BY(confuse);
	gs.magazineSize = get_magazine_size(_forOwner, _forHand);
	if (_fillAffection) COPY_AFFECTED_BY(magazineSize);
	gs.numberOfProjectiles = get_number_of_projectiles();
	if (_fillAffection) COPY_AFFECTED_BY_FROM(numberOfProjectiles, get_projectile().count);
	gs.projectileSpeed = get_projectile_speed();
	if (_fillAffection) COPY_AFFECTED_BY_FROM(numberOfProjectiles, get_projectile().relativeVelocity);
	gs.projectileSpread = get_projectile_spread();
	if (_fillAffection) COPY_AFFECTED_BY_FROM(numberOfProjectiles, get_projectile().spread);
	gs.continuousFire = is_continuous_fire();
	if (_fillAffection) COPY_AFFECTED_BY(continuousFire);

	gs.storageOutputSpeed = get_storage_output_speed();
	if (_fillAffection) COPY_AFFECTED_BY(storageOutputSpeed);
	gs.magazineOutputSpeed = get_magazine_output_speed();
	if (_fillAffection) COPY_AFFECTED_BY(magazineOutputSpeed);

	gs.rpm = gs.shotCost.value.is_zero()? Energy::zero() : (gs.storageOutputSpeed.value * 60) / gs.shotCost.value;
	if (_fillAffection)
	{
		COPY_AFFECTED_BY_FROM_NEG(rpm, gs.shotCost);
		COPY_AFFECTED_BY_FROM(rpm, gs.storageOutputSpeed);
	}
	if (is_using_magazine())
	{
		gs.rpmMagazine = gs.shotCost.value.is_zero() || ! is_using_magazine()? Energy::zero() : (gs.magazineOutputSpeed.value * 60) / gs.shotCost.value;
		if (_fillAffection)
		{
			COPY_AFFECTED_BY_FROM_NEG(rpmMagazine, gs.shotCost);
			COPY_AFFECTED_BY_FROM_NEG(rpmMagazine, gs.magazineOutputSpeed);
		}
	}

	gs.specialFeatures = specialFeatures;
	gs.statInfoLocStrs = statInfoLocStrs;

	{
		gs.projectileStatInfoLocStrs.clear();
		if (! singleSpecialProjectile.value)
		{
			auto& useProjectile = isUsingAutoProjectiles.value ? autoProjectile : projectile;
			if (auto* pt = useProjectile.get_type())
			{
				if (auto* pd = pt->get_gameplay_as<ModuleProjectileData>())
				{
					for_every(ad, pd->get_additional_gun_stats_info())
					{
						gs.projectileStatInfoLocStrs.push_back(ad->get_id());
						COPY_AFFECTED_BY_FROM_EX_OVERRIDE(gs.projectileStatInfoLocStrs.get_last(), coreKind, WeaponStatAffection::Special);
					}
				}
			}
		}
	}

	gs.singleSpecialProjectile = singleSpecialProjectile;
}

void Gun::update_energy_state_from_user()
{
	Energy energyAvailable = Energy::zero();
	if (user && hand.is_set())
	{
		energyAvailable = user->get_hand_energy_storage((hand.get()));
		maxStorage = user->get_hand_energy_max_storage((hand.get()));
	}
	else
	{
		maxStorage = Energy::zero();
	}
	if (energyAvailable != chamber + magazine + storage)
	{
#ifdef OUTPUT_ENERGY_LEVELS
		output(TXT("[gun energy] %S + %S + %S = %S update to %S"), chamber.as_string_auto_decimals().to_char(), magazine.as_string_auto_decimals().to_char(), storage.as_string_auto_decimals().to_char(),
			(chamber + magazine + storage).as_string_auto_decimals().to_char(), energyAvailable.as_string_auto_decimals().to_char());
#endif
	}
	chamber = min(chamber, energyAvailable);
	energyAvailable -= chamber;
	if (is_using_magazine())
	{
		magazine = min(magazine, energyAvailable);
		energyAvailable -= magazine;
	}
	else
	{
		magazine = Energy::zero();
	}
	storage = min(storage, energyAvailable);
	energyAvailable -= storage;
	if (energyAvailable.is_positive())
	{
		// fill storage
		storage = min(maxStorage.value, storage + energyAvailable);
	}
}

void Gun::update_energy_state_to_user()
{
	if (user && hand.is_set())
	{
#ifdef OUTPUT_ENERGY_LEVELS
		output(TXT("[gun energy] store in user %S + %S + %S = %S (was %S)"), chamber.as_string_auto_decimals().to_char(), magazine.as_string_auto_decimals().to_char(), storage.as_string_auto_decimals().to_char(),
			(chamber + magazine + storage).as_string_auto_decimals().to_char(),
			user->get_hand_energy_storage((hand.get())).as_string_auto_decimals().to_char());
#endif
		if (auto* gd = GameDirector::get())
		{
			if (gd->are_ammo_storages_unavailable())
			{
				return;
			}
		}
		user->set_hand_energy_storage((hand.get()), storage + magazine + chamber);
	}
}

int Gun::get_shots_left() const
{
	Energy energy = get_chamber() + get_magazine() + get_storage();
	Energy chamberSize = get_chamber_size();

	return (energy / chamberSize).floored().as_int();
}

float Gun::calculate_chamber_loading_time() const
{
	Energy chamberSize = get_chamber_size();
	Energy chargingSpeed = get_storage_output_speed();

	if (is_using_magazine())
	{
		Energy magOS = get_magazine_output_speed();

		chargingSpeed = max(magOS, chargingSpeed);
	}

	float loadingTime = chamberSize.div_to_float(chargingSpeed);

	return loadingTime;
}

//--

void Gun::SubEmissive::update(float _deltaTime)
{
	if (abs(upTarget - state) < 0.01f)
	{
		upTarget = 0.0f;
	}
	float useTarget = upTarget;
	if (target.is_set())
	{
		useTarget = max(useTarget, target.get());
	}
	if (useTarget > 0.0f)
	{
		state = blend_to_using_speed_based_on_time(state, useTarget, upTime, _deltaTime);
	}
	else
	{
		state = blend_to_using_time(state, 0.0f, coolDownTime, _deltaTime);
	}
}

//--

REGISTER_FOR_FAST_CAST(GunData);

GunData::GunData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
, weaponSetup(nullptr)
{
}

GunData::~GunData()
{
}

bool GunData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("projectile")))
	{
		result &= projectile.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("vrPulses")))
	{
		result &= vrPulses.load_from_xml(node, _lc);
	}

	fillWithRandomParts = _node->get_bool_attribute_or_from_child_presence(TXT("fillWithRandomParts"), fillWithRandomParts);
	nonRandomisedParts.load_from_xml(_node, TXT("nonRandomisedParts"));

	weaponSetup.clear();

	for_every(node, _node->children_named(TXT("parts")))
	{
		weaponSetup.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("weaponSetup")))
	{
		weaponSetup.load_from_xml(node);
	}

	return result;
}

bool GunData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= projectile.prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		if (!weaponSetup.resolve_library_links())
		{
			error(TXT("couldn't prepare weapon setup"));
			result = false;
		}
	}

	return result;
}

void GunData::prepare_to_unload()
{
	base::prepare_to_unload();
}
