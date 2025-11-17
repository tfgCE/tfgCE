#pragma once

#include "..\..\energyTransfer.h"

#include "..\..\..\game\damage.h"
#include "..\..\..\game\energy.h"
#include "..\..\..\game\gameStateSensitive.h"

#include "..\..\..\library\damageRules.h"

#include "..\..\..\..\framework\general\cooldownsTimeStampBased.h"
#include "..\..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\..\framework\module\moduleCustom.h"

#define HEALTH__CONTINUOUS_DAMAGE__TIME_BASED_INTENSITY

namespace Framework
{
	class Mesh;
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	class ModuleEnergyQuantum;
	class ModulePilgrim;

	namespace GameScript
	{
		namespace Elements
		{
			class Health;
		};
	};

	namespace CustomModules
	{
		class HealthData;

		enum HealthPostMoveReason
		{
			ContinuousDamage	= bit(1),
			ExtraEffect			= bit(2),
			Regen				= bit(3),
			Decomposing			= bit(4)
		};

		class Health
		: public Framework::ModuleCustom
		, public IEnergyTransfer
		, public IGameStateSensitive
		{
			FAST_CAST_DECLARE(Health);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_BASE(IEnergyTransfer);
			FAST_CAST_BASE(IGameStateSensitive);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			struct ExtraEffect
			{
				Name name;
				Optional<float> timeLeft;
				Optional<DamageType::Type> forContinuousDamageType; // if no such continuous damage, is removed
				SafePtr<Framework::IModulesOwner> particles;

				HealthExtraEffectParams params;
			};

		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			static Energy read_max_health_from(Framework::ModuleData const* _moduleData);

			static bool does_exists_and_is_alive(Framework::IModulesOwner* imo);

			Health(Framework::IModulesOwner* _owner);
			virtual ~Health();

			Health* get_actual_health_module();

			void adjust_damage_on_hit_with_extra_effects(REF_ Damage & _damage) const; // this is on hit, pre deal_damage, should be called before use_for_projectile (if used)
			bool do_extra_effects_allow_ricochets() const;

			void deal_damage(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info); // unadjusted damage might be used for movement

			bool has_extra_effect(Name const& _name) const;
			bool has_any_extra_effect() const { return !extraEffects.is_empty(); }
			void add_extra_effect(HealthExtraEffect const & _effect);
			void clear_extra_effects();
			Array<ExtraEffect> const& get_extra_effects() const { return extraEffects; }

			void add_continuous_damage(TeaForGodEmperor::ContinuousDamage const& _continuousDamage, DamageInfo& _info, float _percentage = 1.0f, bool _accumulate = true);
			void clear_continuous_damage();

			void apply_continuous_damage_instantly(DamageType::Type _ofType, Optional<Energy> const& _damage = NP, Optional<EnergyCoef> const& _damageCoef = NP, Framework::IModulesOwner* _instigator = nullptr);

			void override_death_effects(Optional<bool> const & _allowDecompose = NP, Optional<float> const & _ceaseOnDeathDelay = NP); // we explicitly want to disallow it, if we use external explosion, etc

			void setup_death_params(Optional<bool> const& _peacefulDeath = NP, Optional<bool> const& _meleeDeath = NP, Framework::IModulesOwner* _deathInstigator = nullptr);

			void perform_death_without_reward(); // for scripts (and ai) to make things dead (for ai remember to set death params)
			void perform_death(Optional<Framework::IModulesOwner*> const & _deathInstigator = NP, Optional<Vector3> const & _deathRelativeDir = NP);
			void perform_death(SafePtr<Framework::IModulesOwner> const & _deathInstigator);
			void perform_quick_death(Optional<Framework::IModulesOwner*> const& _deathInstigator, Optional<Vector3> const & _deathRelativeDir = NP); // not everything will trigger
			void give_reward_for_kill(Optional<Framework::IModulesOwner*> const& _deathInstigator = NP, Optional<Framework::IModulesOwner*> const& _source = NP);

			bool is_alive() const { return health.is_positive() && !isDying && !isPerformingDeath; }

			Framework::IModulesOwner* get_last_damage_instigator() const { return lastDamageInstigator.get(); }

		public:
			bool is_super_health_storage() const { return superHealthStorage; }
			Energy const & get_health() const { return health; }
			Energy get_max_health() const { return superHealthStorage ? get_max_total_health() : get_max_health_internal(); }
			Energy get_max_health_not_super_health_storage() const { return get_max_health_internal(); }
			virtual Energy get_total_health() const { return get_health(); }
			virtual Energy get_max_total_health() const { return get_max_health_internal(); }
			virtual void sanitise_energy_levels() { health = clamp(health, Energy::zero(), get_max_health()); }

		protected:
			Energy get_max_health_internal() const; // after modifiers, without super health storage

		public:
			void update_super_health_storage();

		public:
			int get_rule_count() const { return ruleInstances.get_size(); }
			bool is_rule_health_active(Name const & _id) const;
			bool is_rule_health_active(int _idx) const;
			Optional<Energy> get_rule_health(int _idx) const;
			bool get_rule_hit_info(int _idx, OUT_ Optional<Name> & _hitBone, OUT_ Optional<Name> & _hitSocketInvestigateInfo) const;

			bool does_allow_hit_from_same_instigator() const { return allowHitFromSameInstigator; }

			void be_invincible(Optional<float> _forTime = NP);
			void be_no_longer_invincible();
			bool is_invincible() const { return invincible; }

			void appear_dead(bool _appearDead = true);

			Colour get_global_tint() const { return globalTintColour; }

			bool can_be_vampirised_with_melee() const { return canBeVampirisedWithMelee; }

		public: // utils to get some useful info to show
			Optional<EnergyCoef> get_prone_to_explosions() const;

		public:
			void reset_energy_state(Optional<Energy> const & _to = NP, Optional<bool> const & _clearEffects = NP) { reset_health(_to, _clearEffects); }
			virtual void on_brought_to_life();
			virtual void set_health(Energy const& _health); // just set it, doesn't kill or destroy
			virtual void reset_health(Optional<Energy> const& _to = NP, Optional<bool> const& _clearEffects = NP);
			virtual void add_energy(REF_ Energy & _addEnergy); // adds some energy (removes from this value if exceeds)
			virtual void redistribute_energy(REF_ Energy& _excessEnergy);

		protected: friend class TeaForGodEmperor::ModuleEnergyQuantum;
			virtual void process_energy_quantum_health(ModuleEnergyQuantum* _eq);

		protected:
			virtual void on_deal_damage(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info);
			virtual void on_death(Damage const & _damage, DamageInfo const& _damageInfo) {}
			virtual void immortal_health_reached_below_minimum_by(Energy const& _belowMinimum) {} // negative value
			virtual void heal(REF_ Energy & _amountLeft);

			virtual void handle_excess_continuous_damage(REF_ Energy& _excessDamage) {} // modifies _excessDamage if handled

		protected:
			friend class GameScript::Elements::Health;
			virtual void ex__set_total_health(Energy const& _health);
			virtual void ex__set_peaceful_death(bool _peacefulDeath) { peacefulDeath = _peacefulDeath; }
			virtual void ex__set_lootable_death(bool _lootableDeath) { lootableDeath = _lootableDeath; }
			virtual void ex__drop_health_by(Energy const& _dropBy, Optional<Energy> const& _min);
			virtual void ex__set_immortal(Optional<bool> const & _immortal) { immortal = _immortal; }

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		public: // IEnergyTransfer
			implement_ bool handle_health_energy_transfer_request(EnergyTransferRequest & _request);
			implement_ Energy calculate_total_energy_available(EnergyType::Type _type) const { return _type == EnergyType::Health ? get_health() : Energy::zero(); }
			implement_ Energy calculate_total_max_energy_available(EnergyType::Type _type) const { return _type == EnergyType::Health ? get_max_health() : Energy::zero(); }

		public: // IGameStateSensitive
			implement_ void store_for_game_state(SimpleVariableStorage& _variables) const;
			implement_ void restore_from_game_state(SimpleVariableStorage const& _variables);

		public:
			virtual Energy calculate_total_energy_limit() const { return get_max_health(); }

		protected:
			HealthData const * healthData = nullptr;
			Random::Generator rg;

			bool particlesOnDeath = true; // spawn hit particles when died
			bool affectVelocityOnDeath = false;
			bool allowHitFromSameInstigator = true; // if we could be hit by object belonging to our own instigator (this is the case of a power ammo being pulled and shot at by player (true), for shields we have (false) to disallow damaging own shield)
			bool noRewardForKill = false;

			bool noEnergyTransfer = false;
			
			bool canBeVampirisedWithMelee = false;

			Name performDeathAIMessage; // if this ai message is to be created, standard perform_death is not called, it has to be called through AI
			Name triggerGameScriptTrapOnDeath; // when dies, this game script trap is triggered
			Name triggerGameScriptTrapOnHealthReachesBottom; // dies or immortal
			bool isDying = false; // if dying process was started, either through ai message or perform_death
			bool isPerformingDeath = false; // perform_death (or quick) was called
			bool rewardForKillGiven = false;
			bool peacefulDeath = false;
			bool lootableDeath = false; // has to be explicitly set
			bool wasDamagedByPlayer = false;
			SafePtr<Framework::IModulesOwner> lastDamageInstigator;
			SafePtr<Framework::IModulesOwner> deathInstigator;
			Vector3 deathRelativeDir = Vector3::zero;
			Optional<DamageType::Type> deathDamageType;

			Framework::CooldownsTimeStampBased<16, Name> spawnTemporaryObjectOnDamageRuleCooldowns;

			bool invincible = false; // no reaction on invincible (not velocity impulses, nothing)
			Optional<bool> immortal; // all normal reactions, just won't die, if false, will force not to be immortal

			float delayDeath = 0.0f; // may be set per instance, can be overriden on go (owner variable), this is to allow for certain objects to die in front of our eyes

			bool superHealthStorage = false; // if true, get_max_health returns total health
			Energy maxHealth = Energy(10);
			Energy health = Energy(0); // 0 kills, although it is possible to have a character with 0 health if it was set with set_health
			Energy startingHealth = Energy(0); // by default set to max health when setting up (ignored for pilgrim, uses PilgrimInitialEnergyLevels)

			Energy dampedHealth = Energy(0); // to track if we dropped health too quickly recently

			Energy lastPreDamageHealth = Energy(10);
			int lastPreDamageHealthFrame = 0; // to update last health only if game frame is different

			EnergyCoef explosionResistance = EnergyCoef(0); // base value

			float affectVelocityLinearLimit = 0.0f;
			float affectVelocityLinearAffectLimit = 0.0f;
			float affectVelocityLinearPerDamagePoint = 0.0f;
			float forcedAffectVelocityLinearLimit = 2.0f; // we require anything to be here

			Rotator3 affectVelocityRotationLimit = Rotator3::zero;
			Rotator3 affectVelocityRotationPerDamagePoint = Rotator3::zero;

			Colour globalTintColour = Colour::none; // makes sense only for player
			float globalTintColourActive = 0.0f;

			bool decompose = false; // if decomposing, will use delay for cease and take over the cease to exist functionality
			Optional<float> decomposingFor;
			Optional<float> decomposeTime;

			// similar to continuous damage but represents just an effect
			Array<ExtraEffect> extraEffects;

			struct ContinuousDamage
			{
				float timeLeft = 0.0f;
				Damage damagePerSecond;
				DamageInfo damageInfo;
				float damageMissingBit = 0.0f;
				float intensity = 0.0f; // might be fed to extra effect if present (!)
#ifdef HEALTH__CONTINUOUS_DAMAGE__TIME_BASED_INTENSITY
				float intensityFull = 0.0f;
#else
				Energy intensityFull = Energy::zero();
#endif
			};
			Array<ContinuousDamage> continuousDamage;

			struct OverrideDeathEffects
			{
				Optional<bool> allowDecompose;
				Optional<float> ceaseOnDeathDelay;
			} overrideDeathEffects; // explicitly by code

			Array<SafePtr<Framework::IModulesOwner> > spawnedTemporaryObjects;

			bool should_affect_velocity() const { return health >= Energy(0) || affectVelocityOnDeath; }

			void health_reached_below_zero(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info);

			void update_last_pre_damage_health();

			void on_decompose(); // done when decomposing is done

			void spawn_particles_on_death(OPTIONAL_ OUT_ bool* _allowDecompose = nullptr, OPTIONAL_ OUT_ float* _useCeaseOnDeathDelay = nullptr);

			void spawn_particles_on_decompose();

			void change_appearance_on_death();

			void trigger_game_script_trap_on_death();
			void trigger_game_script_trap_on_health_reaches_bottom();

		protected:
			struct RuleInstance // index is the same as rule in data
			{
				Energy health = Energy::zero();
			};
			Array<RuleInstance> ruleInstances;

			bool check_and_act(Damage & _damage, DamageInfo& _info); // false if damage should be ignored (continuous is for info, for explosions etc)
			bool check_and_act(TeaForGodEmperor::ContinuousDamage & _damage, DamageInfo& _info); // false if should be ignored
			bool check_and_act_filter(DamageRule const* _damageRule, Damage const* _damage, TeaForGodEmperor::ContinuousDamage const* _continuousDamage, DamageInfo& _info) const;

			void internal_add_continuous_damage(Damage const& _damagePerSecond, DamageInfo& _info, float _time, float _minTime, float _percentage, bool _accumulate);
		};

		class HealthData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(HealthData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored);
				
			HealthData(Framework::LibraryStored* _inLibraryStored);
			virtual ~HealthData();

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

		private:
			struct OnDeath
			{
				bool disallowDecompose = false;
				bool notDuringPeacefulDeath = false;
				Name ifExtraEffectActive;
				EnergyRange preDeathHealth = EnergyRange::empty;
				Optional<float> ceaseOnDeathDelay;
				Optional<DamageType::Type> forDamageType;
				Optional<DamageType::Type> forDamageTypeNot;
				// spawn
				Name temporaryObject;
				::Framework::UsedLibraryStored<::Framework::TemporaryObjectType> temporaryObjectType;
			};
			Array<OnDeath> onDeath;
			struct OnDecompose
			{
				// spawn
				Name temporaryObject;
				::Framework::UsedLibraryStored<::Framework::TemporaryObjectType> temporaryObjectType;
			};
			Array<OnDecompose> onDecompose;
			struct ChangeAppearanceOnDeath
			{
				Name hideAppearance;
				Name showAppearance;
				Name setMainAppearance;
				float delay = 0.0f;
			};
			Array<ChangeAppearanceOnDeath> changeAppearanceOnDeath;
			bool ceaseOnDeath = true; // may be overriden to be handled via logic
			float ceaseOnDeathDelay = 0.0f; // on actual death
			bool disableMindOnDeath = true; // when dying, death sequence

			struct SoundOnDamage
			{
				Range damage = Range::empty;
				Name sound;
			};
			Array<SoundOnDamage> soundsOnDamage;
			Array<SoundOnDamage> soundsOnDamageControlledByPlayer;

			struct Rule
			{
				Name id; // more as a reference/info

				// conditions
				Name hitBone;
				Name hitSocketInvestigateInfo; // just to show investigate info
				bool includeChildrenBones = false;
				bool activeIfDepleted = false; // even if depleted, should be active
				Energy health = Energy::zero(); // if zero, applied always (coefs!)
				// material?
				SimpleVariableStorage requiredVariables; // only if variables from this list match

				// general
				EnergyCoef applyDamageToMainHealthCoef = EnergyCoef::zero(); // if depletable, this affects only the part that is up to depleted

				// on deplete
				SimpleVariableStorage setVariablesOnDeplete;
				SimpleVariableStorage setAppearanceControllerVariablesOnDeplete;
				Array<Name> temporaryObjectsOnDeplete;
				Array<Name> soundsOnDeplete;
				bool allowDamageToGoThroughOnDeplete = false; // if set to true, damage that has been not used (on depletion) goes further
				Energy extraDamageOnDeplete = Energy::zero(); // if rule is depleted, this is extra damage applied
				bool extraDamageCanKill = false;
				Optional<Energy> dropHealthToOnDeplete; // if rule is depleted, main health is dropped to this value - this happens before any damage going through!
				Range3 applyVelocityOnDeplete = Range3(Range(-1.0f, 1.0f), Range(-1.0f, 1.0f), Range(-1.0f, 1.0f)); // in object space
				Range applySpeedOnDeplete = Range::zero;
				Optional<Range> autoDeathTime; // if set, will start a timer to auto death that will grant no experience

				bool load_from_xml(IO::XML::Node const* _node, HealthData const * _for);
			};
			Array<Rule> rules; // order of rules is important!

			Array<DamageRule> damageRules; // order of damage rules is important! all are processed
			Array<Framework::UsedLibraryStored<DamageRuleSet>> includeDamageRuleSets;

			friend class Health;
		};
	};
};

