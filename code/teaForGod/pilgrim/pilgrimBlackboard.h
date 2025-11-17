#pragma once

#include "..\teaEnums.h"

#include "..\game\energy.h"
#include "..\game\physicalViolence.h"

#include "..\..\core\types\hand.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;
	class PilgrimSetup;
	struct PilgrimInventory;
	struct PilgrimStatsInfo;
	struct PilgrimStatsInfoHand;

	struct PilgrimBlackboardHand
	{
		friend struct PilgrimStatsInfo;
		friend struct PilgrimStatsInfoHand;

		struct MainEquipment
		{
			EnergyCoef chamberSizeCoef = EnergyCoef::zero();
			EnergyCoef damageCoef = EnergyCoef::zero();
			EnergyCoef armourPiercing = EnergyCoef::zero();
			EnergyCoef magazineSizeCoef = EnergyCoef::zero();
			Energy magazineMinSize = Energy::zero();
			int magazineMinShotCount = 0;
			float magazineCooldownCoef = 0.0f;
			EnergyCoef storageOutputSpeedCoef = EnergyCoef::zero();
			EnergyCoef magazineOutputSpeedCoef = EnergyCoef::zero();
			Energy storageOutputSpeedAdd = Energy::zero();
			Energy magazineOutputSpeedAdd = Energy::zero();
			float antiDeflection = 0.0f;
			float maxDist = 0.0f;
		} mainEquipment;
		Energy handEnergyStorage = Energy::zero();
		float pullEnergyDistanceRecentlyRenderedRoom = 0.0f;
		float pullEnergyDistanceRecentlyNotRenderedRoom = 0.0f;
		EnergyCoef energyInhaleCoef = EnergyCoef::zero();

		PhysicalViolence physicalViolence;
	};

	/**
	 *	All coef calculations are handled by this class.
	 *	From it you can access how values are modified.
	 */
	class PilgrimBlackboard
	{
		friend struct PilgrimStatsInfo;
		friend struct PilgrimStatsInfoHand;

	public:
		struct AdditionalInfos
		{
			String* body = nullptr;
			String* hands[2] = { nullptr, nullptr };
		};
	public:
		static int calculate_passive_exm_slot_count(PilgrimSetup const& _setup, OPTIONAL_ PilgrimInventory const* _inventory);
		void update(PilgrimSetup const & _setup, OPTIONAL_ PilgrimInventory const * _inventory, OUT_ AdditionalInfos* _ai = nullptr);

	public:
		// various utilities (if hand is MAX, it is invalid and results in false/0)
		static Energy get_health_modifier_for(Framework::IModulesOwner const* _imo);
		static Energy get_health_backup_modifier_for(Framework::IModulesOwner const* _imo);
		static Energy get_health_regen_rate_modifier_for(Framework::IModulesOwner const* _imo);
		static EnergyCoef get_health_regen_cooldown_coef_modifier_for(Framework::IModulesOwner const* _imo);
		static EnergyCoef get_health_damage_received_coef_modifier_for(Framework::IModulesOwner const* _imo);
		static EnergyCoef get_explosion_resistance(Framework::IModulesOwner const* _imo);
		
		static int get_passive_exm_slot_count(Framework::IModulesOwner const* _imo);
		static int get_pocket_level(Framework::IModulesOwner const* _imo);

		static Energy get_health_for_melee_kill(Framework::IModulesOwner const* _imo);
		static Energy get_ammo_for_melee_kill(Framework::IModulesOwner const* _imo);

		static Energy get_hand_energy_storage(Framework::IModulesOwner const* _imo, Hand::Type _hand);

		static EnergyCoef get_main_equipment_chamber_size_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static EnergyCoef get_main_equipment_damage_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static EnergyCoef get_main_equipment_armour_piercing_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static EnergyCoef get_main_equipment_magazine_size_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static Energy get_main_equipment_magazine_min_size_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static int get_main_equipment_magazine_min_shot_count_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static float get_main_equipment_cooldown_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static bool get_main_equipment_using_magazine(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static EnergyCoef get_main_equipment_storage_output_speed_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static EnergyCoef get_main_equipment_magazine_output_speed_coef_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static Energy get_main_equipment_storage_output_speed_add_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static Energy get_main_equipment_magazine_output_speed_add_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static float get_main_equipment_anti_deflection_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static float get_main_equipment_max_dist_modifier_for(Framework::IModulesOwner const* _imo, Hand::Type _hand);
		static float get_pull_energy_distance(Framework::IModulesOwner const* _imo, Hand::Type _hand, bool _recentlyRenderedRoom = true);
		static EnergyCoef get_energy_inhale_coef(Framework::IModulesOwner const* _imo, Hand::Type _hand);

		static void update_physical_violence_damage(Framework::IModulesOwner const* _imo, Hand::Type _hand, bool _strong, REF_ Energy & _damage);
		static void update_physical_violence_speeds(Framework::IModulesOwner const* _imo, Hand::Type _hand, REF_ float & _minSpeed, REF_ float& _minSpeedStrong);

	private:
		PilgrimBlackboardHand hands[Hand::MAX];

		Energy health = Energy::zero();
		Energy healthBackup = Energy::zero();
		Energy healthRegenRate = Energy::zero();
		EnergyCoef healthRegenCooldownCoef = EnergyCoef::zero();
		EnergyCoef healthDamageReceivedCoef = EnergyCoef::zero();
		EnergyCoef explosionResistance = EnergyCoef::zero();

		int passiveEXMSlots = 1;
		int pocketLevel = 0;

		Energy healthForMeleeKill = Energy(0.0f);
		Energy ammoForMeleeKill = Energy(1.0f);

	};
};
