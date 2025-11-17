#pragma once

#include "..\teaEnums.h"

#include "..\game\energy.h"
#include "..\game\physicalViolence.h"
#include "..\library\weaponPartType.h"

#include "..\..\core\types\hand.h"

namespace Framework
{
	class ObjectType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;
	class PilgrimSetup;

	struct PilgrimStatsInfoDiffHand;
	struct PilgrimStatsInfoDiff;

	/**
	 *	Blackboard holds modifiers, this is a summary
	 */

	struct PilgrimStatsInfoHand
	{
		String additionalInfo;

		float pullEnergyDistanceRecentlyRenderedRoom = 0.0f;
		float pullEnergyDistanceRecentlyNotRenderedRoom = 0.0f;
		EnergyCoef energyInhaleCoef = EnergyCoef::zero();
		Energy handEnergyStorage;

		struct MainEquipment
		{
			bool isValid;
			WeaponCoreKind::Type coreKind;
			int shotCount;
			Energy totalDamage;
			Energy totalEnergy;
			int storageShotCount;
			Energy storageTotalDamage;
			Energy storageOutputSpeed;
			Energy storageRPM; // rounds per minute
			Energy storageDPS;
			Energy magazineSize;
			float magazineCooldown;
			int magazineShotCount;
			Energy magazineTotalDamage;
			Energy magazineOutputSpeed;
			Energy magazineRPM; // rounds per minute
			Energy magazineDPS;
			Energy chamberSize;
			Energy shotCost;
			Energy damage;
			Energy immediateDamage;
			Energy continuousDamage;
			EnergyCoef armourPiercing;
			float antiDeflection;
			float projectileSpeed;
			float projectileSpread;
			int projectilesPerShot;
			ArrayStatic<Name, 32> specialFeatures;

			MainEquipment()
			{
				SET_EXTRA_DEBUG_INFO(specialFeatures, TXT("PilgrimStatsInfo::MainEquipment.specialFeatures"));
			}
		} mainEquipment;

		struct PhysicalViolence
		{
			float minSpeed;
			float minSpeedStrong;
			Energy damage;
			Energy damageStrong;
		} physicalViolence;

		void update_diff(PilgrimStatsInfoHand const& _prev, PilgrimStatsInfoDiffHand& _diff, float _toValue) const;
	};

	struct PilgrimStatsInfo
	{
		static Framework::ObjectType const* find_suitable_object_type(Framework::IModulesOwner* playerActor = nullptr);

	public:
		PilgrimStatsInfoHand hands[Hand::MAX];

		String additionalInfo;

		Energy health = Energy::zero();
		Energy healthBackup = Energy::zero();
		Energy healthRegenRate = Energy::zero();
		EnergyCoef healthDamageReceivedCoef = EnergyCoef::zero();
		float healthRegenCooldown = 0.0f;
	
		void update(Framework::ObjectType const * _objectType, PilgrimSetup const& _setup, bool _onlyBase = false);

		void update_diff(PilgrimStatsInfo const& _prev, PilgrimStatsInfoDiff& _diff, float _toValue) const;
	};

	// > 0 went up
	// < 0 went down
	struct PilgrimStatsInfoDiffHand
	{
		float pullEnergyDistanceRecentlyRenderedRoom = 0.0f;
		float pullEnergyDistanceRecentlyNotRenderedRoom = 0.0f;
		float energyInhaleCoef = 0.0f;
		float handEnergyStorage = 0.0f;

		struct MainEquipment
		{
			float coreKind;
			float shotCount;
			float totalDamage;
			float totalEnergy;
			float storageShotCount;
			float storageTotalDamage;
			float storageOutputSpeed;
			float storageRPM;
			float storageDPS;
			float magazineSize;
			float magazineCooldown;
			float magazineShotCount;
			float magazineTotalDamage;
			float magazineOutputSpeed;
			float magazineRPM;
			float magazineDPS;
			float chamberSize;
			float shotCost;
			float damage;
			float immediateDamage;
			float continuousDamage;
#ifndef ARMOUR_DEFLECTION_ETC_INTO_ADDITIONAL_STATS
			float armourPiercing;
			float antiDeflection;
#endif
			float projectileSpeed;
			float projectileSpread;
			float projectilesPerShot;
		} mainEquipment;

		struct PhysicalViolence
		{
			float minSpeed;
			float minSpeedStrong;
			float damage;
			float damageStrong;
		} physicalViolence;

		float additionalInfo = 0.0f;

		void advance(float _time, float _speed);
	};

	struct PilgrimStatsInfoDiff
	{
		PilgrimStatsInfoDiffHand hands[Hand::MAX];

		float health = 0.0f;
		float healthBackup = 0.0f;
		float healthRegenRate = 0.0f;
		float healthRegenCooldown = 0.0f;
		float healthDamageReceivedCoef = 0.0f;

		float additionalInfo = 0.0f;

		void advance(float _time, float _speed);
	};

};
