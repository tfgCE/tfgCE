#pragma once

#include "mc_health.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Health;
		};
	};

	namespace CustomModules
	{
		class HealthRegen
		: public Health
		{
			FAST_CAST_DECLARE(HealthRegen);
			FAST_CAST_BASE(Health);
			FAST_CAST_END();

			typedef Health base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			static Energy read_max_health_backup_from(Framework::ModuleData const* _moduleData);
			static Energy read_regen_rate_from(Framework::ModuleData const* _moduleData);
			static float read_regen_cooldown_from(Framework::ModuleData const* _moduleData);

			HealthRegen(Framework::IModulesOwner* _owner);
			virtual ~HealthRegen();

		public:
			Energy get_health_backup() const { return healthBackup; }
			Energy get_max_health_backup() const { return superHealthStorage ? Energy::zero() : get_max_health_backup_internal(); }
			Energy get_regen_rate() const; // with modifiers
			float get_regen_cooldown() const; // with modifiers
			override_ Energy get_total_health() const { return get_health_backup() + base::get_total_health(); }
			override_ Energy get_max_total_health() const { return get_max_health_backup_internal() + base::get_max_total_health(); }
			override_ void sanitise_energy_levels() { healthBackup = clamp(healthBackup, Energy::zero(), get_max_health_backup()); base::sanitise_energy_levels(); }

		protected:
			Energy get_max_health_backup_internal() const; // // after modifiers, without super health storage

		public:
			void block_regen(bool _block) { regenBlocked = _block; timeAfterDamage = 0.0f; }
			void immediate_regen() { regenBlocked = false; timeAfterDamage = 10000.0f; }

			float get_time_after_damage() const { return timeAfterDamage; }

		public: // Health
			override_ void set_health(Energy const& _health) { base::set_health(_health); timeAfterDamage = 0.0f; }
			override_ void reset_health(Optional<Energy> const& _to = NP, Optional<bool> const& _clearEffects = NP);
			override_ void add_energy(REF_ Energy& _addEnergy);
			override_ void redistribute_energy(REF_ Energy & _excessEnergy); // redistributes energy

		protected: // Health
			override_ void on_deal_damage(Damage const& _damage, Damage const& _unadjustedDamage, DamageInfo& _info); // unadjusted damage might be used for movement
			override_ void on_death(Damage const& _damage, DamageInfo const& _damageInfo);
			override_ void immortal_health_reached_below_minimum_by(Energy const& _belowMinimum); // _belowMinimum should be negative
			override_ void handle_excess_continuous_damage(REF_ Energy& _excessDamage);
			override_ void heal(REF_ Energy & _amountLeft);

		protected: friend class ModuleEnergyQuantum;
			override_ void process_energy_quantum_health(ModuleEnergyQuantum* _eq);

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		public: // IEnergyTransfer
			override_ bool handle_health_energy_transfer_request(EnergyTransferRequest & _request);
			implement_ Energy calculate_total_energy_available(EnergyType::Type _type) const { return _type == EnergyType::Health ? base::calculate_total_energy_available(EnergyType::Health) + get_health_backup() : Energy::zero(); }
			implement_ Energy calculate_total_max_energy_available(EnergyType::Type _type) const { return _type == EnergyType::Health ? base::calculate_total_max_energy_available(EnergyType::Health) + get_max_health_backup() : Energy::zero(); }

		public: // IGameStateSensitive
			implement_ void store_for_game_state(SimpleVariableStorage& _variables) const;
			implement_ void restore_from_game_state(SimpleVariableStorage const& _variables);

		public: // Health
			implement_ Energy calculate_total_energy_limit() const { return base::calculate_total_energy_limit() + get_max_health_backup(); }

		protected:
			friend class GameScript::Elements::Health;
			override_ void ex__set_total_health(Energy const& _health);
			override_ void ex__drop_health_by(Energy const& _dropBy, Optional<Energy> const& _min);
			void ex__set_backup_health(Energy const& _health) { healthBackup = _health; }

		protected:
			bool regenBlocked = false;
			Energy regenRate = Energy(2);
			float regenCooldown = 1.0f;
			float regenMB = 0.0f;

			Energy startingHealthBackup = Energy(0); // if not provided during read, will use read health (ignored for pilgrim, uses PilgrimInitialEnergyLevels)
			Energy healthBackup = Energy(0); // empty by default
			Energy maxHealthBackup = Energy(0); // has to be at least healthBackup

			// runtime
			float timeAfterDamage = 0.0f;
		};
	};
};

