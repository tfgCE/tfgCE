#pragma once

#include "..\..\energyTransfer.h"
#include "..\..\gameplay\moduleEquipment.h"

#include "..\..\..\game\energy.h"
#include "..\..\..\game\weaponPart.h"
#include "..\..\..\game\weaponSetup.h"
#include "..\..\..\game\weaponStatInfo.h"
#include "..\..\..\library\weaponPartType.h"
#include "..\..\..\pilgrim\pilgrimStatsInfo.h"

#include "..\..\..\..\core\functionParamsStruct.h"
#include "..\..\..\..\core\math\math.h"
#include "..\..\..\..\core\vr\iVR.h"
#include "..\..\..\..\framework\module\moduleData.h"

namespace Framework
{
	class DoorInRoom;
	class PresenceLink;
	class SoundSource;
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class ModulePilgrim;
	class WeaponPartType;

	namespace AI
	{
		namespace Logics
		{
			class WeaponMixer;
		};
	};

	namespace OverlayInfo
	{
		struct TextColours;
	};

	namespace ModuleEquipments
	{
		class Gun;
		class GunData;
		struct GunStats;

		struct GunProjectile
		{
		public:
			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			int get_count() const { return count.value; }
			Framework::TemporaryObjectType* get_type() const { return type.get(); }
			Name const & get_place_at_socket() const { return placeAtSocket; }
			Vector3 const & get_relative_velocity() const { return relativeVelocity.value; }
			float get_spread() const { return spread.value; }
		private:
			bool hasMuzzle = true;
			WeaponStatInfo<int> count = 1;
			Framework::UsedLibraryStored<Framework::TemporaryObjectType> type;
			Name placeAtSocket;
			WeaponStatInfo<Vector3> relativeVelocity = Vector3::zero;
			WeaponStatInfo<float> spread = 0.0f;
			Optional<Colour> colour;

			friend class Gun;
			friend struct GunStats;
		};

		struct GunVRPulses
		{
			VR::Pulse shoot;
			VR::Pulse chamberLoaded;

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		};

		struct GunStats
		{
			GunProjectile projectile;
			GunProjectile autoProjectile;

			WeaponStatInfo<WeaponCoreKind::Type> coreKind = WeaponCoreKind::Plasma;
			WeaponStatInfo<EnergyCoef> damaged = EnergyCoef::zero();
			WeaponStatInfo<bool> continuousFire = false;
			WeaponStatInfo<bool> singleSpecialProjectile = false;
			WeaponStatInfo<Energy> chamberSize = Energy(2);
			WeaponStatInfo<Energy> baseDamageBoost = Energy::zero(); // sum before multiplier
			WeaponStatInfo<EnergyCoef> damageCoef = EnergyCoef::zero(); // adjusted plus one
			WeaponStatInfo<Energy> damageBoost = Energy::zero(); // sum at the end
			WeaponStatInfo<EnergyCoef> fromCoreDamageCoef = EnergyCoef::one(); // 0% to 100%
			WeaponStatInfo<EnergyCoef> armourPiercing = EnergyCoef::zero();
			bool usingMagazine = true; // result of magazine size
			WeaponStatInfo<float> magazineCooldown = 0.0f; // <=0.0 immediate
			WeaponStatInfo<Energy> magazineSize = Energy::zero();
			WeaponStatInfo<Energy> magazineOutputSpeed = Energy(20);
			WeaponStatInfo<Energy> maxStorage = Energy(0);
			WeaponStatInfo<Energy> storageOutputSpeed = Energy(2);
			WeaponStatInfo<float> antiDeflection = 0.0f;
			WeaponStatInfo<Optional<float>> maxDist;
			WeaponStatInfo<float> kineticEnergyCoef;
			WeaponStatInfo<float> confuse;
			WeaponStatInfo<EnergyCoef> mediCoef;
			WeaponStatInfo<bool> isUsingAutoProjectiles = false; // if using auto projectiles, gets value from below and projectiles from projectile manager
			WeaponStatInfo<float> continuousDamageTime = 0.0f;
			WeaponStatInfo<float> continuousDamageMinTime = 0.0f;
			WeaponStatInfo<bool> noHitBoneDamage = false;
			ArrayStatic<WeaponStatInfo<Name>, 32> specialFeatures;
			ArrayStatic<WeaponStatInfo<Name>, 12> statInfoLocStrs; // localised string ids

			// other
			bool sightColourSameAsProjectile = false;

			GunStats()
			{
				SET_EXTRA_DEBUG_INFO(specialFeatures, TXT("GunStats.specialFeatures"));
				SET_EXTRA_DEBUG_INFO(statInfoLocStrs, TXT("GunStats.statInfoLocStrs"));
			}

			// two separate calls
			void calculate_stats(WeaponSetup const& _weaponSetup, bool _pilgrimWeapon);
			void calculate_stats_update_projectile(Gun const* _gun, PilgrimStatsInfoHand::MainEquipment const* _statsME); // _gun or _statsMe

			GunProjectile const& gun_stats_get_used_projectile() const { return isUsingAutoProjectiles.value ? autoProjectile : projectile; }
			GunProjectile const& gun_stats_get_auto_projectile() const { return autoProjectile; }
		};

		struct GunChosenStats
		{
			WeaponStatInfo<WeaponCoreKind::Type> coreKind = WeaponCoreKind::Plasma;
			WeaponStatInfo<EnergyCoef> damaged = EnergyCoef::zero();
			WeaponStatInfo<Energy> shotCost = Energy::zero();
			WeaponStatInfo<Energy> totalDamage = Energy::zero();
			WeaponStatInfo<Energy> damage = Energy::zero();
			WeaponStatInfo<Energy> continuousDamage = Energy::zero();
			WeaponStatInfo<Energy> medi = Energy::zero();
			WeaponStatInfo<bool> noHitBoneDamage = false;
			WeaponStatInfo<EnergyCoef> armourPiercing = EnergyCoef::zero();
			WeaponStatInfo<float> antiDeflection = 0.0f;
			WeaponStatInfo<Optional<float>> maxDist;
			WeaponStatInfo<float> kineticEnergyCoef;
			WeaponStatInfo<float> confuse;
			WeaponStatInfo<Energy> magazineSize = Energy::zero(); // if zero, not using
			WeaponStatInfo<Energy> storageOutputSpeed = Energy::zero();
			WeaponStatInfo<Energy> magazineOutputSpeed = Energy::zero();
			WeaponStatInfo<Energy> rpm = Energy::zero();
			WeaponStatInfo<Energy> rpmMagazine = Energy::zero();
			WeaponStatInfo<int> numberOfProjectiles = 0;
			WeaponStatInfo<float> projectileSpeed = 0.0f;
			WeaponStatInfo<float> projectileSpread = 0.0f;
			WeaponStatInfo<bool> continuousFire = false;
			WeaponStatInfo<bool> singleSpecialProjectile = false;
			ArrayStatic<WeaponStatInfo<Name>, 32> specialFeatures;
			ArrayStatic<WeaponStatInfo<Name>, 12> statInfoLocStrs; // localised string ids
			ArrayStatic<WeaponStatInfo<Name>, 12> projectileStatInfoLocStrs; // localised string ids

			void reset();

			bool operator ==(GunChosenStats const& _other) const;
			bool operator !=(GunChosenStats const& _other) const { return !(operator ==(_other)); }

			GunChosenStats()
			{
				SET_EXTRA_DEBUG_INFO(specialFeatures, TXT("GunChosenStats.specialFeatures"));
				SET_EXTRA_DEBUG_INFO(statInfoLocStrs, TXT("GunChosenStats.statInfoLocStrs"));
				SET_EXTRA_DEBUG_INFO(projectileStatInfoLocStrs, TXT("GunChosenStats.projectileStatInfoLocStrs"));
			}
			struct StatInfo
			{
				String text;
				ArrayStatic<WeaponsStatAffectedByPart, 10> affectedBy;

				static void split_multi_lines(List<StatInfo>& _statsInfo);
			};
			void build_overlay_stats_info(OUT_ List<StatInfo> & _statsInfo, bool _shortDamagedInfo = false, OPTIONAL_ OUT_ OverlayInfo::TextColours * _textColours = nullptr) const;
		};

		class Gun
		: public ModuleEquipment
		, private GunStats
		{
			FAST_CAST_DECLARE(Gun);
			FAST_CAST_BASE(ModuleEquipment);
			FAST_CAST_END();

			typedef ModuleEquipment base;
		public:
			static void set_common_port_values(REF_ SimpleVariableStorage& _variables);

		public:
			static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

			Gun(Framework::IModulesOwner* _owner);
			virtual ~Gun();

			void be_pilgrim_weapon(bool _pilgrimWeapon = true) { pilgrimWeapon = _pilgrimWeapon; }
			bool is_pilgrim_weapon() const { return pilgrimWeapon; }

			struct FireParams
			{
				ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(FireParams, bool, useNominalDamageNow, use_nominal_damage_now, false, true);
				ADD_FUNCTION_PARAM(FireParams, Energy, nominalFireCost, nominal_fire_cost);
				ADD_FUNCTION_PARAM(FireParams, Vector3, atLocationWS, at_location_WS); // off angles still apply
			};
			bool fire(FireParams const & _params);

			WeaponCoreKind::Type get_core_kind() const { return coreKind.value; }
			EnergyCoef get_damaged() const { return damaged.value; }
			bool is_damaged() const { return !damaged.value.is_zero(); }
			Energy get_chamber() const { return chamber; }
			Energy get_chamber_size(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers
			EnergyCoef get_armour_piercing(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers
			bool is_chamber_ready() const { return chamber >= get_chamber_size(); }
			Energy get_storage_output_speed() const; // with modifiers
			Energy get_magazine_output_speed() const; // with modifiers
			Energy get_magazine() const { return is_using_magazine() ? magazine : Energy::zero(); }
			Energy get_magazine_size(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers
			float get_magazine_cooldown() const; // with modifiers
			bool is_using_magazine(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers
			float get_anti_deflection(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers
			Optional<float> get_max_dist(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers
			float get_kinetic_energy_coef(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers (if any)
			float get_confuse(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers (if any)
			EnergyCoef get_medi_coef(Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers (if any)
			Energy get_storage() const { return storage; }
			Energy get_max_storage() const; // with modifiers
			bool is_continuous_fire() const { return continuousFire.value; }
			bool is_single_special_projectile() const { return singleSpecialProjectile.value; }
			bool is_empty() const { return chamber + storage + get_magazine() < get_chamber_size(); }
			Energy get_projectile_damage(bool _nominal = false /* false - basing on current amount of energy in the chamber, true - general for weapon */, OPTIONAL_ OUT_ Energy* _totalDamage = nullptr, OPTIONAL_ OUT_ Energy* _medi = nullptr, Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const; // with modifiers, if _nominal==false, gets enegry as we have in chamber
			int get_number_of_projectiles() const; // with modifiers
			Framework::TemporaryObjectType * get_projectile_type() const;
			float get_projectile_speed() const;
			float get_projectile_spread() const;

			ArrayStatic<WeaponStatInfo<Name>, 32> const & get_special_features() const { return specialFeatures; }

			int get_shots_left() const;
			float get_time_since_last_shot() const { return timeSinceLastShot; }

			float calculate_chamber_loading_time() const;

			Transform get_muzzle_placement_os() const;

		public:
			void ex__update_stats(); // updates stats only!

		public:
			void build_chosen_stats(REF_ GunChosenStats& gs, bool _fillAffection, Framework::IModulesOwner* _forOwner = nullptr, Optional<Hand::Type> const& _forHand = NP) const;

			void log_stats(LogInfoContext& _log) const;

		public:
			int calculate_total_ammo(Optional<Energy> const & _usingEnergy = NP) const { return (_usingEnergy.get(get_chamber() + get_magazine() + get_storage()) / (get_chamber_size())).floored().as_int(); }
			int calculate_ammo_in_chamber_and_magazine() const { return ((get_chamber() + get_magazine()) / (get_chamber_size())).floored().as_int(); }

		public:	// parts
			void remove_weapon_part_to_pilgrim_inventory(WeaponPartAddress const& _at); // may destroy the weapon, will put weapon parts to pilgrim inventory
			void fill_with_random_parts(Array<WeaponPartType const*> const* _usingWeaponParts = nullptr);
			void setup_with_weapon_setup(WeaponSetup const& _setup);
			WeaponSetup & access_weapon_setup() { return weaponSetup; }
			WeaponSetup const& get_weapon_setup() const { return weaponSetup; }
			void on_disassemble(bool _movePartsToInventory = true);

		private:
			friend class AI::Logics::WeaponMixer;

			void create_parts(bool _changeAsSoonAsPossible = true);
			void request_async_create_parts(bool _changeAsSoonAsPossible = true);

		public: // ModuleEquipment
			override_ void advance_post_move(float _deltaTime);
			override_ void advance_user_controls();

			override_ bool is_aiming_at(Framework::IModulesOwner const * imo, Framework::PresencePath const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const;
			override_ bool is_aiming_at(Framework::IModulesOwner const * imo, Framework::RelativeToPresencePlacement const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const;

			override_ Optional<Energy> get_primary_state_value() const { return chamber + magazine; }
			override_ Optional<Energy> get_secondary_state_value() const { return chamber; }
			override_ Optional<Energy> get_energy_state_value() const { return chamber + magazine + storage; }

		protected:
			override_ void ready_energy_quantum_context(EnergyQuantumContext & _eqc) const;
			override_ void process_energy_quantum_ammo(ModuleEnergyQuantum* _eq);

		protected: // ModuleEquipment
			override_ void on_change_user(ModulePilgrim* _user, Optional<Hand::Type> const& _hand, ModulePilgrim* _prevUser, Optional<Hand::Type> const& _prevHand);

		public: // ModuleGameplay
			override_ bool does_require_temporary_objects() const { return true; }

		protected: // Module
			override_ void reset();
			override_ void setup_with(Framework::ModuleData const * _moduleData);

			override_ void initialise();

		protected:
			GunData const * gunData = nullptr;

			// GunProjectile projectile; in GunStats
			// GunProjectile autoProjectile; in GunStats

			GunVRPulses vrPulses;

			bool mayCreateParts = false;
			bool requestedAsyncCreatingParts = false;
			WeaponSetup weaponSetup;
			SimpleVariableStorage meshGenerationParameters;

			float blockUseEquipmentPressed = 0.0f;
			float timeSinceLastShot = 1000.0f;
			bool allowEnergyDeposit = true;

			// setup
			// GunStats

			bool pilgrimWeapon = false; // if false, storages are twice as big + bonus

			float gunInUse = 0.0f;
			
			float blockEmitsDamagedLightnings = 1.0f;
			bool emitsDamagedLightnings = false;

			bool emitsIdleLightnings = false;

			struct SubEmissive
			{
				float state = 0.0f;
				Optional<float> target;
				float upTarget = 0.0f;
				float upTime = 0.05f;
				float coolDownTime = 3.0f;

				void update(float _deltaTime);
			};
			SubEmissive muzzleSE;
			float muzzleHotEnergyFull = 20.0f;

			SubEmissive superchargerSE;
			SubEmissive retaliatorSE;

			Colour defaultWeaponSightColour = Colour::white;
			
			Colour superchargerColour = Colour::blue;
			Colour retaliatorColour = Colour::red;

			float energyStorageState = 0.0f;
			float energyStorageStateFullBlinkFor = 0.0f;

			// current state (gun does not actually hold energy inside, those values are kind of cached and are synchronised with user's storage)
			Energy storage = Energy(1000); // is in use by both main equipment and other one (this might be max storage, initial level is ignored by pilgrim, uses PilgrimInitialEnergyLevels)
			Energy magazine = Energy(0);
			Energy chamber = Energy(0);
			bool isLocked = false;
			bool isReady = false;
			bool canBeReady = false; // still some energy left
			bool chamberJustLoaded = false;
			bool wasShooting = false;
			float blockInfiniteGeneratingFor = 0.0f;

			// missing bits
			float generatedMB = 0.0f;
			float magazineToChamberMB = 0.0f;

			Colour normalColour = Colour::black;
			Colour lockedColour = Colour::green;
			Colour readyColour = Colour::red;

			bool playedShotFailed = false;
			float blockPlayShootFailed = 0.0f;

			template <class PathClass>
			bool is_aiming_at_impl(Framework::IModulesOwner const * target, PathClass const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const;

			GunProjectile const& get_projectile() const { return isUsingAutoProjectiles.value ? autoProjectile : projectile; }

		protected:
			// sounds and temporary objects can be played separately although it is strongly advised to use sounds per temporary object if possible
			// the only exception is muzzle flash v shooting sound
			// stand alone sounds should not reflect temporary objects, they should be truly stand alone (like a sound that a particular chassis does)

			Array<SafePtr<Framework::IModulesOwner>> tempSpawnedTemporaryObjects;

			int spawn_temporary_objects_using_parts_or_own(Name const& _name, OPTIONAL_ int* _inOrder = nullptr, bool _stopOnFirstType = false);
			
			int play_sound_using_parts_or_own(Name const& _name, OPTIONAL_ int* _inOrder = nullptr, bool _stopOnFirstType = true); // if we want all parts to make a sound, we should explicitly say "false"

		protected:
			struct ShootContext
			{
				Energy damage = Energy::zero();
				Optional<Energy> cost; // used for healing
				EnergyCoef armourPiercing = EnergyCoef::zero();
			};
			bool shoot_lightning(REF_ ShootContext & _context);

		protected:
			void update_energy_state_from_user();
			void update_energy_state_to_user();

		private:
			template <typename Class>
			void add_projectile_damage_affected_by(REF_ WeaponStatInfo<Class>& _stat) const;

			Framework::UsedLibraryStored<EXMType> energyRetainerEXM;

			friend struct GunStats;
		};

		class GunData
		: public ModuleEquipmentData
		{
			FAST_CAST_DECLARE(GunData);
			FAST_CAST_BASE(ModuleEquipmentData);
			FAST_CAST_END();

			typedef ModuleEquipmentData base;
		public:
			GunData(Framework::LibraryStored* _inLibraryStored);
			virtual ~GunData();

			GunProjectile const & get_projectile() const { return projectile; }
			GunVRPulses const & get_vr_pulses() const { return vrPulses; }
			WeaponSetup const& get_weapon_setup() const { return weaponSetup; }
			bool should_fill_with_random_parts() const { return fillWithRandomParts; }
			Optional<bool> const & should_use_non_randomised_parts() const { return nonRandomisedParts; }

		public: // Framework::ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();

		private:
			GunProjectile projectile;

			GunVRPulses vrPulses;

			WeaponSetup weaponSetup;

			bool fillWithRandomParts = false;
			Optional<bool> nonRandomisedParts;
		};
	};
};

