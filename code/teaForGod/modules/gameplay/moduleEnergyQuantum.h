#pragma once

#include "..\..\teaEnums.h"
#include "..\..\game\energy.h"

#include "..\..\..\core\concurrency\mrswLock.h"
#include "..\..\..\core\types\hand.h"

#include "..\moduleGameplay.h"

namespace Framework
{
	struct PresencePath;
	struct RelativeToPresencePlacement;
};

namespace TeaForGodEmperor
{
	class ModuleEnergyQuantumData;
	class ModulePilgrim;
	
	struct EnergyQuantumContext
	{
		Optional<Hand::Type> withHand;
		Optional<EnergyType::Type> energyTypeOverride;
		::Framework::IModulesOwner* adjustBy = nullptr; // any adjustments are done by this IMO
		::Framework::IModulesOwner* ammoReceiver = nullptr;
		::Framework::IModulesOwner* healthReceiver = nullptr;
		::Framework::IModulesOwner* sideEffectsReceiver = nullptr;
		::Framework::IModulesOwner* sideEffectsHealthReceiver = nullptr; // if null, health receiver is chosen

		EnergyQuantumContext() {}
		EnergyQuantumContext& with_hand(Optional<Hand::Type> const & _withHand) { withHand = _withHand; return *this; }
		EnergyQuantumContext& energy_type_override(Optional<EnergyType::Type> const & _type) { energyTypeOverride = _type; return *this; }
		EnergyQuantumContext& receiver(::Framework::IModulesOwner* _receiver) { adjustBy = _receiver; ammoReceiver = _receiver; healthReceiver = _receiver; sideEffectsReceiver = _receiver; sideEffectsReceiver = _receiver; return *this; }
		EnergyQuantumContext& adjust_by(::Framework::IModulesOwner* _adjustBy) { adjustBy = _adjustBy; return *this; }
		EnergyQuantumContext& ammo_receiver(::Framework::IModulesOwner* _ammoReceiver) { ammoReceiver = _ammoReceiver; return *this; }
		EnergyQuantumContext& health_receiver(::Framework::IModulesOwner* _healthReceiver) { healthReceiver = _healthReceiver; return *this; }
		EnergyQuantumContext& side_effects_receiver(::Framework::IModulesOwner* _sideEffectsReceiver) { sideEffectsReceiver = _sideEffectsReceiver; return *this; }
		EnergyQuantumContext& side_effects_health_receiver(::Framework::IModulesOwner* _sideEffectsHealthReceiver) { sideEffectsHealthReceiver = _sideEffectsHealthReceiver; return *this; }
	};

	/**
	 *	Processing energy quantum is a bit complicated
	 *		it is being processed in two places, both in ai logic
	 *		it requires processing to start and to end - this is lock
	 *		this is because different objects may differently handle energy (some may take both ammo and health and treat it as health)
	 */
	class ModuleEnergyQuantum
	: public ModuleGameplay
	{
		FAST_CAST_DECLARE(ModuleEnergyQuantum);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		static bool does_any_exist();

		ModuleEnergyQuantum(Framework::IModulesOwner* _owner);
		virtual ~ModuleEnergyQuantum();

		void set_energy_type(EnergyType::Type _type) { an_assert(processingLock.is_acquired_write_on_this_thread()); type = _type; }
		EnergyType::Type get_energy_type() const { return type; }

		void start_energy_quantum_setup() { processingLock.acquire_write(); }
		void end_energy_quantum_setup() { processingLock.release_write(); }

		// these modifications should be used with energy quantum setup
		void set_energy(Energy const & _energy);
		void set_side_effect_damage(Energy const & _sideEffectDamage);
		void clear_energy();
		Energy get_energy() const;
		bool has_energy() const { return get_energy() > Energy::zero(); }

		void process_energy_quantum(EnergyQuantumContext const & _eqc, bool _clearEnergy = true);
		bool is_being_processed_by_us() const { return processingLock.is_acquired_write_on_this_thread(); }

		void use_energy(Energy const & _usedEnergy); // this is to inform that someone has used energy and how much was used
		
		void explode(Framework::IModulesOwner* _instigator = nullptr);

		void no_time_limit() { noTimeLimit = true; }
		void set_time_left(float _time) { noTimeLimit = false; timeLeft = _time; }
		void mark_to_disappear() { markedToDisappear = true; }

		void mark_pulled_by_pilgrim(float _distance);

	private:
		void start_processing_energy_quantum() { processingLock.acquire_write(); }
		void end_processing_energy_quantum() { processingLock.release_write(); }

		void apply_side_effects_to(Framework::IModulesOwner* _toIMO, Framework::IModulesOwner* _healthReceiver); // this is to apply very specific side effects

	public:
		virtual void advance_post_move(float _deltaTime);

	protected: // Module
		override_ void reset();
		override_ void setup_with(Framework::ModuleData const * _moduleData);

		override_ void initialise();

	public: // ModuleGameplay
		override_ bool on_death(Damage const& _damage, DamageInfo const & _damageInfo);

	protected:
		ModuleEnergyQuantumData const * energyQuantumData = nullptr;

		mutable Concurrency::MRSWLock processingLock;

		ModulePilgrim* user = nullptr;

		float quantumSize = 1.0f;

		float timeLeft = 10.0f;
		bool noTimeLimit = false;
		bool markedToDisappear = false;

		float lifeScale = 1.0f; // scale due to life time - it fades out at the end
		float energyScale = 1.0f; // scale due to energy contained, uses table to scale properly
		Optional<float> finalScale;

		EnergyType::Type type = EnergyType::Ammo;

		bool explodeOnDeath = false;
		Collision::Flags explodeOnCollisionWithFlags = Collision::Flags::none();

		Energy sideEffectDamage = Energy::zero();
		Name sideEffectAIMessage;

		Optional<Energy> energy;

		int pulledByPilgrim = 0;
		float maxDistanceToPullByPilgrim = 0.0f;
	};

	class ModuleEnergyQuantumData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleEnergyQuantumData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();
		typedef Framework::ModuleData base;
	public:
		ModuleEnergyQuantumData(Framework::LibraryStored* _inLibraryStored);

	protected: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

	protected:	friend class ModuleEnergyQuantum;
		struct ScalePair
		{
			float energy = 0.0f;
			float size = 0.0f;

			bool is_valid() const { return energy > 0.0f && size >= 0.0f; }

			static int compare(void const * _a, void const * _b)
			{
				ScalePair const * a = plain_cast<ScalePair>(_a);
				ScalePair const * b = plain_cast<ScalePair>(_b);
				if (a->energy < b->energy) return A_BEFORE_B;
				if (a->energy > b->energy) return B_BEFORE_A;
				return A_AS_B;
			}
		};
		Array<ScalePair> scaling;
	};
};

