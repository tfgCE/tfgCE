#pragma once

#include "..\moduleGameplay.h"

#include "..\..\teaEnums.h"

#include "..\..\game\energy.h"
#include "..\..\game\physicalViolence.h"

#include "..\..\..\core\types\hand.h"
#include "..\..\..\framework\text\localisedString.h"

namespace TeaForGodEmperor
{
	class EXMType;
	class ModuleEXMData;
	class ModulePilgrim;
	struct EnergyQuantumContext;

	class ModuleEXM
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleEXM);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleEXM(Framework::IModulesOwner* _owner);
		virtual ~ModuleEXM();

		void set_exm_type(EXMType const* _exmType);
		EXMType const* get_exm_type() const;

		void set_hand(Hand::Type _hand) { hand = _hand; }

		bool is_exm_active() const { return exmIsActive; }
		bool does_exm_appear_active() const { return exmIsActive || deactivateBlinkIn > 0.0f; }
		void mark_exm_selected(bool _selected);
		bool mark_exm_active(bool _active); // returns true if activated (had enough energy etc)
		bool mark_exm_active_blink(); // returns true if has enough energy
		void blink_on_physical_violence();

		int get_activated_count() const { return exmActivatedCount; }
		int get_deactivated_count() const { return exmDeactivatedCount; }

		Optional<float> get_cooldown_left_pt() const;

	public: // get from module data
		Energy const& get_cost_single() const;
		Energy const& get_cost_per_second() const;
		EnergyCoef const& get_recall_percentage() const;

		float get_cooldown_time() const;

		Energy get_health() const;
		Energy get_health_backup() const;
		Energy get_health_regen_rate() const;
		EnergyCoef get_health_regen_cooldown_coef() const;
		EnergyCoef get_health_damage_received_coef() const;

		Energy get_hand_energy_storage() const;

		EnergyCoef get_main_equipment_chamber_size_coef() const;
		EnergyCoef get_main_equipment_damage_coef() const;
		EnergyCoef get_main_equipment_armour_piercing() const;
		EnergyCoef get_main_equipment_magazine_size_coef() const;
		Energy get_main_equipment_magazine_min_size() const;
		int get_main_equipment_magazine_min_shot_count() const;
		EnergyCoef get_main_equipment_storage_output_speed_coef() const;
		EnergyCoef get_main_equipment_magazine_output_speed_coef() const;
		Energy get_main_equipment_storage_output_speed_add() const;
		Energy get_main_equipment_magazine_output_speed_add() const;
		float get_main_equipment_anti_deflection() const;
		float get_main_equipment_max_dist() const;

		float get_pull_energy_distance_recently_rendered_room() const;
		float get_pull_energy_distance_recently_not_rendered_room() const;

		EnergyCoef get_energy_inhale_coef() const;

		bool get_link_equipment_to_main_equipment() const;

		PhysicalViolence const& get_physical_violence() const;

	protected: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const * _moduleData);

		override_ void initialise();

	protected: // ModuleGameplay
		override_ void advance_post_move(float _deltaTime);

	protected:
		ModuleEXMData const * exmData = nullptr;
		Framework::UsedLibraryStored<EXMType> exmType;

		Optional<Hand::Type> hand;

		bool exmCanBeActiveNotSelected = false;
		bool exmIsActive = false;
		bool exmIsSelected = false;
		int exmActivatedCount = 0;
		int exmDeactivatedCount = 0;

		float deactivateBlinkIn = 0.0f;
		float deactivatePhysicalViolenceBlinkIn = 0.0f;

		float costPerSecondMissingBit = 0.0f;

		float cooldownTimeLeft = 0.0f;
	};

	class ModuleEXMData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleEXMData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();

		typedef Framework::ModuleData base;
	public:
		ModuleEXMData(Framework::LibraryStored* _inLibraryStored);
		virtual ~ModuleEXMData();

	public:
		int get_priority() const { return priority; }

		Energy const& get_cost_single() const { return costSingle; }
		Energy const& get_cost_per_second() const { return costPerSecond; }
		EnergyCoef const& get_recall_percentage() const { return recallPercentage; }
		
		float get_cooldown_time() const { return cooldownTime; }

		Energy get_health() const { return health; }
		Energy get_health_backup() const { return healthBackup; }
		Energy get_health_regen_rate() const { return healthRegenRate; }
		EnergyCoef get_health_regen_cooldown_coef() const { return healthRegenCooldownCoef; }
		EnergyCoef get_health_damage_received_coef() const { return healthDamageReceivedCoef; }
		EnergyCoef get_explosion_resistance() const { return explosionResistance; }

		int get_additional_passive_exm_slot_count() const { return additionalPassiveEXMSlotCount; }
		int get_additional_pocket_level() const { return additionalPocketLevel; }

		Energy get_health_for_melee_kill() const { return healthForMeleeKill; }
		Energy get_ammo_for_melee_kill() const { return ammoForMeleeKill; }

		Energy get_hand_energy_storage() const { return handEnergyStorage; }
		EnergyCoef get_main_equipment_chamber_size_coef() const { return mainEquipmentChamberSizeCoef; }
		EnergyCoef get_main_equipment_damage_coef() const { return mainEquipmentDamageCoef; }
		EnergyCoef get_main_equipment_armour_piercing() const { return mainEquipmentArmourPiercing; }
		EnergyCoef get_main_equipment_magazine_size_coef() const { return mainEquipmentMagazineSizeCoef; }
		Energy get_main_equipment_magazine_min_size() const { return mainEquipmentMagazineMinSize; }
		int get_main_equipment_magazine_min_shot_count() const { return mainEquipmentMagazineMinShotCount; }
		float get_main_equipment_magazine_cooldown_coef() const { return mainEquipmentMagazineCooldownCoef; }
		EnergyCoef get_main_equipment_storage_output_speed_coef() const { return mainEquipmentStorageOutputSpeedCoef; }
		EnergyCoef get_main_equipment_magazine_output_speed_coef() const { return mainEquipmentMagazineOutputSpeedCoef; }
		Energy get_main_equipment_storage_output_speed_add() const { return mainEquipmentStorageOutputSpeedAdd; }
		Energy get_main_equipment_magazine_output_speed_add() const { return mainEquipmentMagazineOutputSpeedAdd; }
		float get_main_equipment_anti_deflection() const { return mainEquipmentAntiDeflection; }
		float get_main_equipment_max_dist() const { return mainEquipmentMaxDist; }

		float get_pull_energy_distance_recently_rendered_room() const { return pullEnergyDistanceRecentlyRenderedRoom; }
		float get_pull_energy_distance_recently_not_rendered_room() const { return pullEnergyDistanceRecentlyNotRenderedRoom; }

		EnergyCoef get_energy_inhale_coef() const { return energyInhaleCoef; }

		PhysicalViolence const & get_physical_violence() const { return physicalViolence; }

	public: // Framework::ModuleData
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	protected: // Framework::ModuleData
		// read all through load_from_xml
		//override_ bool read_parameter_from(IO::XML::Attribute const* _attr, Framework::LibraryLoadingContext& _lc) { return true; }
		//override_ bool read_parameter_from(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc) { return true; }

	private:
		friend class ModuleEXM;

		int priority = 0; // will sort exms as in some cases bonuses do not stack (they just override)

		Energy costSingle = Energy(0.0f);
		Energy costPerSecond = Energy(0.0f);
		EnergyCoef recallPercentage = EnergyCoef::zero(); // if possible to recall

		float cooldownTime = 0.0f;

		// this works as a conglomerate of many different types of modules, it is easier to hold everything here as it doesn't take that much more memory

		// body

		Energy health = Energy::zero();
		Energy healthBackup = Energy::zero();
		Energy healthRegenRate = Energy::zero();
		EnergyCoef healthRegenCooldownCoef = EnergyCoef::zero();
		EnergyCoef healthDamageReceivedCoef = EnergyCoef::zero();
		EnergyCoef explosionResistance = EnergyCoef::zero();
		
		int additionalPassiveEXMSlotCount = 0;
		int additionalPocketLevel = 0;

		Energy healthForMeleeKill = Energy(0.0f);
		Energy ammoForMeleeKill = Energy(0.0f);

		// hand

		Energy handEnergyStorage = Energy::zero();

		EnergyCoef mainEquipmentChamberSizeCoef = EnergyCoef::zero(); // this is relative to 0 (+ or -)
		EnergyCoef mainEquipmentDamageCoef = EnergyCoef::zero(); // this is relative to 0 (+ or -) (damage does not affect chamber size)
		EnergyCoef mainEquipmentArmourPiercing = EnergyCoef::zero(); // accumulates
		EnergyCoef mainEquipmentMagazineSizeCoef = EnergyCoef::zero(); // this is relative to 0 (+ or -)
		Energy mainEquipmentMagazineMinSize = Energy::zero(); // this overrides
		int mainEquipmentMagazineMinShotCount = 0; // this overrides
		float mainEquipmentMagazineCooldownCoef = 0.0f; // this is relative to 0 (+ or -)
		EnergyCoef mainEquipmentStorageOutputSpeedCoef = EnergyCoef::zero(); // this is relative to 0 (+ or -)
		EnergyCoef mainEquipmentMagazineOutputSpeedCoef = EnergyCoef::zero(); // this is relative to 0 (+ or -)
		Energy mainEquipmentStorageOutputSpeedAdd = Energy::zero(); // accumulates
		Energy mainEquipmentMagazineOutputSpeedAdd = Energy::zero(); // accumulates
		float mainEquipmentAntiDeflection = 0.0f; // accumulates
		float mainEquipmentMaxDist = 0.0f; // accumulates

		float pullEnergyDistanceRecentlyRenderedRoom = 0.0f;
		float pullEnergyDistanceRecentlyNotRenderedRoom = 0.0f;

		EnergyCoef energyInhaleCoef = EnergyCoef::zero(); // as other coefs

		PhysicalViolence physicalViolence;
	};
};

