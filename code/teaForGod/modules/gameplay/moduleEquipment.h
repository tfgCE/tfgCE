#pragma once

#include "..\moduleGameplay.h"

#include "..\..\game\energy.h"

#include "..\..\..\core\types\hand.h"

namespace Framework
{
	class Display;
	struct PresencePath;
	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	class ModuleEnergyQuantum;
	class ModuleEquipmentData;
	class ModulePilgrim;
	struct EnergyQuantumContext;

	namespace GameScript
	{
		namespace Elements
		{
			class MainEquipment;
		};
	};

	class ModuleEquipment
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleEquipment);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleEquipment(Framework::IModulesOwner* _owner);
		virtual ~ModuleEquipment();

		bool is_main_equipment() const { return isMainEquipment; }
		void be_main_equipment(bool _isMainEquipment, Optional<Hand::Type> const& _hand = NP);
		void set_failsafe_equipment_hand(Optional<Hand::Type> const& _hand = NP) { failsafeEquipmentHand = _hand; }
		Hand::Type get_main_equipment_hand() const { return mainEquipmentHand.is_set() ? mainEquipmentHand.get() : failsafeEquipmentHand.get(Hand::MAX); }

		bool is_non_interactive() const { return nonInteractive; }

		bool should_allow_easy_release_for_auto_interim_equipment() const { return allowEasyReleaseForAutoInterimEquipment; }
		void set_user(ModulePilgrim* _user, Optional<Hand::Type>  const & _hand = NP); // to be set when actually used by a user
		ModulePilgrim* get_user() const { return user; }
		Hand::Type get_hand() const { an_assert(hand.is_set()); return hand.get(); }
		Framework::IModulesOwner* get_user_as_modules_owner() const;

		void mark_kill() { timeSinceLastKill = 0.0f; }
		float get_time_since_last_kill() const { return timeSinceLastKill; }

		Vector3 const & get_release_velocity() const { return releaseVelocity; }

	public:
		virtual void advance_post_move(float _deltaTime);
		virtual void advance_user_controls();

		virtual bool is_aiming_at(Framework::IModulesOwner const * imo, Framework::PresencePath const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const { return false; } // by default equipment is not a weapon
		virtual bool is_aiming_at(Framework::IModulesOwner const * imo, Framework::RelativeToPresencePlacement const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const { return false; } // by default equipment is not a weapon

		virtual void ready_energy_quantum_context(EnergyQuantumContext & _eqc) const; // to allow to decide
		virtual void process_energy_quantum_ammo(ModuleEnergyQuantum* _eq);
		virtual void process_energy_quantum_health(ModuleEnergyQuantum* _eq);

		bool does_require_display_extra() const { return requiresDisplayExtra; }
		bool is_display_rotated() const { return rotatedDisplay; }
		virtual void display_extra(::Framework::Display* _display, bool _justStarted) { todo_important(TXT("implement_!"));  }

	public: // for UI
		virtual Optional<Energy> get_primary_state_value() const { return NP; }
		virtual Optional<Energy> get_secondary_state_value() const { return NP; }
		virtual Optional<Energy> get_energy_state_value() const { return NP; }

	protected: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const * _moduleData);

		override_ void initialise();

	protected:
		virtual void on_change_user(ModulePilgrim* _user, Optional<Hand::Type> const& _hand, ModulePilgrim* _prevUser, Optional<Hand::Type> const& _prevHand) {}

	protected:
		ModuleEquipmentData const * equipmentData = nullptr;

		bool isMainEquipment = false;
		Optional<Hand::Type> mainEquipmentHand;
		Optional<Hand::Type> failsafeEquipmentHand; // to know at which side/pocket are we

		ModulePilgrim* user = nullptr;
		Optional<Hand::Type> hand;

		bool nonInteractive = false;

		Optional<float> timeInHand;
		float timeSinceLastKill = 1000.0f;

		bool allowEasyReleaseForAutoInterimEquipment = false; // in specific case this is for vive wand to allow relasing equipment when trigger (that was used to grab a object) was released
		bool requiresDisplayExtra = false;
		bool rotatedDisplay = false; // is put up and display is rotated 90 left or right

		Vector3 releaseVelocity = Vector3::zero; // this is velocity in OS that is added when object is released from hand
	};

	class ModuleEquipmentData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleEquipmentData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();

		typedef Framework::ModuleData base;
	public:
		ModuleEquipmentData(Framework::LibraryStored* _inLibraryStored);
		virtual ~ModuleEquipmentData();

	public: // Framework::ModuleData
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	private:
		Vector3 releaseVelocity = Vector3::zero;

		friend class ModuleEquipment;
	};
};

