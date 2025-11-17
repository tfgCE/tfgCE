#pragma once

#include "energy.h"
#include "weaponStatInfo.h"

#include "..\library\weaponPartType.h"
#include "..\schematics\schematic.h"

#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\simpleVariableStorage.h"

#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ItemType;
};

namespace TeaForGodEmperor
{
	class Persistence;
	class WeaponPart;
	class WeaponPartType;
	struct TutorialHubId;

	namespace OverlayInfo
	{
		struct TextColours;
	};

	struct TransferWeaponPartContext
	{
		Optional<int> preDeactivationLevel;
		int activeBoosts = 0;

		TransferWeaponPartContext& for_pre_deactivation(int _level) { preDeactivationLevel = _level; return *this; }
		TransferWeaponPartContext& with_boosts(int _boosts) { activeBoosts = _boosts; return *this; }
	};

	struct WeaponPartAddress
	{
		ArrayStatic<Name, 8> at; // first one is plug of the main one, the second is one of the slots of the main one, the third is one of the slots of the one attached to the main one and so on
		Optional<int> loosePartIdx;

		WeaponPartAddress()
		{
			SET_EXTRA_DEBUG_INFO(at, TXT("WeaponPartAddress::at"));
		}
		explicit WeaponPartAddress(tchar const* _addr) { set(String(_addr)); }

		bool is_loose_part() const { return loosePartIdx.is_set(); }
		void be_loose_part(int _idx) { loosePartIdx = _idx; }
		int get_loose_part_idx() const { return loosePartIdx.get(); }

		void set(String const& _addr);
		void add(Name const& _id);

		bool operator==(Array<Name> const& _stack) const;
		bool operator==(WeaponPartAddress const& _other) const;

		bool is_empty() const { return at.is_empty(); }

		int as_index_number() const; // repacks tchar (!) value as integer to allow choosing "random" mesh generator
		
		String to_string() const;

		static int compare(void const* _a, void const* _b)
		{
			WeaponPartAddress const* a = (plain_cast<WeaponPartAddress>(_a));
			WeaponPartAddress const* b = (plain_cast<WeaponPartAddress>(_b));
			if (a->is_loose_part() && b->is_loose_part())
			{
				if (a->get_loose_part_idx() < b->get_loose_part_idx()) return A_BEFORE_B;
				if (a->get_loose_part_idx() > b->get_loose_part_idx()) return B_BEFORE_A;
				return A_AS_B;
			}
			else
			{
				for_count(int, i, min(a->at.get_size(), b->at.get_size()))
				{
					int s = String::compare_tchar_icase_sort(a->at[i].to_char(), b->at[i].to_char());
					if (s != 0)
					{
						return s;
					}
				}
				if (a->at.get_size() < b->at.get_size()) return A_BEFORE_B;
				if (a->at.get_size() > b->at.get_size()) return B_BEFORE_A;
				return A_AS_B;
			}
		}

	};

	struct UsedWeaponPart
	{
		WeaponPartAddress at;
		RefCountObjectPtr<WeaponPart> part; // if there's no presence, it is a dry address

		UsedWeaponPart() {}
		UsedWeaponPart(WeaponPartAddress const& _at, WeaponPart const* _part) : at(_at), part(_part) {}

	public:
		bool load_from_xml(Persistence* _persistence, IO::XML::Node const* _node);
		bool save_to_xml(IO::XML::Node* _node) const;
		bool resolve_library_links();
	};

	/**
	 *	Weapon part is an instance of weapon part type
	 *	It is used to build a gun
	 *
	 *	They always exist within a persistence
	 */
	class WeaponPart
	: public RefCountObject
	, public SafeObject<WeaponPart>
	{
	public:
		static WeaponPart* create_instance_of(Persistence* _forPersistence, WeaponPartType const* _weaponPartType, bool _nonRandomised = false, EnergyCoef const& _damaged = EnergyCoef::zero(), bool _defaultMeshParams = false, Optional<int> const & _rgIndex = NP);
		static WeaponPart* create_instance_of(Persistence* _forPersistence, Framework::LibraryName const& _weaponPartType, bool _nonRandomised = false, EnergyCoef const& _damaged = EnergyCoef::zero(), bool _defaultMeshParams = false, Optional<int> const& _rgIndex = NP);
		static WeaponPart* create_load_from_xml(Persistence* _forPersistence, IO::XML::Node const* _node);
		WeaponPart* create_copy_for_different_persistence(Persistence* _forPersistence, Optional<bool> const& _nonRandomised = NP, Optional<EnergyCoef> const& _damaged = NP, Optional<bool> const & _defaultMeshParams = NP, Optional<bool> const& _forStoring = NP) const; // has to be a different persistence!

		static bool compare(WeaponPart const& _a, WeaponPart const& _b);

		static Vector2 get_schematic_size();
		static Vector2 get_container_schematic_size();

		static int adjust_transfer_chance(int _transferChance, TransferWeaponPartContext const& _context);
		
		static bool is_same_content(WeaponPart const * _a, WeaponPart const * _b); // to compare different instances of the same part

	public:
		Persistence* get_persistence() const { return persistence.get(); }
		bool is_non_randomised() const { return nonRandomised; }
		EnergyCoef const & get_damaged() const { return damaged; }
		bool should_use_default_mesh_params() const { return defaultMeshParams; }
		int get_rand_index() const { return rgIndex; }
		int get_transfer_chance() const { return transferChance; }

		bool save_to_xml(IO::XML::Node* _node) const;
		bool resolve_library_links();

		void mark_in_current_use();
		void clear_in_current_use();

	public:
		void ex__set_damaged(EnergyCoef const& _damaged) { damaged = _damaged; }

	public:
		Meshes::Mesh3DInstance const* get_schematic_mesh_instance() const { return visibleSchematic.get() ? &visibleSchematic->get_mesh_instance() : nullptr; }
		Meshes::Mesh3DInstance const* get_schematic_mesh_instance_for_overlay() const { return visibleSchematic.get() ? &visibleSchematic->get_mesh_instance_for_overlay() : nullptr; }
		Meshes::Mesh3DInstance const* get_schematic_mesh_instance_outline_only() const { return visibleSchematic.get() ? &visibleSchematic->get_mesh_instance_outline() : nullptr; }
		Meshes::Mesh3DInstance const* get_container_schematic_mesh_instance() const { return visibleContainerSchematic.get() ? &visibleContainerSchematic->get_mesh_instance() : nullptr; }
		Meshes::Mesh3DInstance const* get_container_schematic_mesh_instance_outline_only() const { return visibleContainerSchematic.get() ? &visibleContainerSchematic->get_mesh_instance_outline() : nullptr; }
		void make_sure_schematic_mesh_exists(); // this should be called before we need to show a weapon part
		Optional<Range2> get_container_schematic_slot_at(WeaponPartAddress const& _address) const;

	public:
		String build_ui_name() const;
		String build_use_info() const;

		struct StatInfo
		{
			String text;
			WeaponStatAffection::Type how = WeaponStatAffection::Unknown;
		};
		void build_overlay_stats_info(OUT_ List<StatInfo> & _statsInfo, OPTIONAL_ OUT_ OverlayInfo::TextColours* _textColours = nullptr) const;

	public:
		SimpleVariableStorage const& get_mesh_generation_parameters() const { return meshGenerationParameters; }
		int get_mesh_generation_parameters_priority() const;
		WeaponPartType const* get_weapon_part_type() const { return weaponPartType.get(); }

		WeaponPartStatInfo<WeaponCoreKind::Type> const& get_core_kind() const { return coreKind; }
		Optional<Colour> const& get_projectile_colour() const { return projectileColour; }
		WeaponPartStatInfo<Energy> const& get_chamber() const { return chamber; }
		WeaponPartStatInfo<EnergyCoef> const& get_chamber_coef() const { return chamberCoef; }
		WeaponPartStatInfo<Energy> get_base_damage_boost() const { return (baseDamageBoost); }
		WeaponPartStatInfo<EnergyCoef> get_damage_coef() const { return damageCoef; }
		WeaponPartStatInfo<Energy> get_damage_boost() const { return (damageBoost); }
		WeaponPartStatInfo<EnergyCoef> get_armour_piercing() const { return (armourPiercing); }
		WeaponPartStatInfo<Energy> get_storage() const { return (storage); }
		WeaponPartStatInfo<Energy> get_storage_output_speed() const { return (storageOutputSpeed); }
		WeaponPartStatInfo<Energy> get_storage_output_speed_add() const { return (storageOutputSpeedAdd); }
		WeaponPartStatInfo<EnergyCoef> get_storage_output_speed_adj() const { return (storageOutputSpeedAdj); }
		WeaponPartStatInfo<Energy> get_magazine() const { return (magazine); }
		WeaponPartStatInfo<float> get_magazine_cooldown() const { return (magazineCooldown); }
		WeaponPartStatInfo<float> get_magazine_cooldown_coef() const { return (magazineCooldownCoef); }
		WeaponPartStatInfo<Energy> get_magazine_output_speed() const { return (magazineOutputSpeed); }
		WeaponPartStatInfo<Energy> get_magazine_output_speed_add() const { return (magazineOutputSpeedAdd); }
		WeaponPartStatInfo<bool> const& get_continuous_fire() const { return continuousFire; }
		WeaponPartStatInfo<float> get_projectile_speed() const { return projectileSpeed; }
		WeaponPartStatInfo<float> get_projectile_spread() const { return projectileSpread; }
		WeaponPartStatInfo<float> get_projectile_speed_coef() const { return (projectileSpeedCoef); }
		WeaponPartStatInfo<float> get_projectile_spread_coef() const { return (projectileSpreadCoef); }
		WeaponPartStatInfo<int> const& get_number_of_projectiles() const { return numberOfProjectiles; }
		WeaponPartStatInfo<float> get_anti_deflection() const { return (antiDeflection); }
		WeaponPartStatInfo<float> get_max_dist_coef() const { return (maxDistCoef); }
		WeaponPartStatInfo<float> get_kinetic_energy_coef() const { return (kineticEnergyCoef); }
		WeaponPartStatInfo<float> get_confuse() const { return (confuse); }
		WeaponPartStatInfo<EnergyCoef> get_medi_coef() const { return (mediCoef); }
		ArrayStatic<Name, 8> const& get_special_features() const;
		Framework::LocalisedString const& get_stat_info() const;
		Framework::LocalisedString const& get_part_only_stat_info() const;

	public:
		TutorialHubId get_tutorial_hub_id() const;

	protected:
		WeaponPart();
		virtual ~WeaponPart();

	private:
		::SafePtr<Persistence> persistence; // may be null, then can't be used - just as a reference (for loading, starting gear etc)

		int rgIndex; // randomly generated
		bool nonRandomised = false;
		EnergyCoef damaged = EnergyCoef::zero();
		bool defaultMeshParams = false;

		Framework::UsedLibraryStored<WeaponPartType> weaponPartType; // on which this one is based

		// during run
		//

		bool inCurrentUse = false; // if in current use -by default we show cells in weapon setup when readying for a run and we show what WILL be during the run
								   // when we start a run, we increase usage and this flag tells to back that usage to show the proper level again
								   // MAYBE it should be done the other way around, but it works now

		// instance - this is automatically created (even if loaded from a file)
		//

		int transferChance = 100; // 0 to 100, base one

		SimpleVariableStorage meshGenerationParameters; // if there were any random parameters, they are changed into actual values

		// gun
		WeaponPartStatInfo<WeaponCoreKind::Type> coreKind;
		Optional<Colour> projectileColour;
		WeaponPartStatInfo<Energy> chamber;
		WeaponPartStatInfo<EnergyCoef> chamberCoef;
		WeaponPartStatInfo<Energy> baseDamageBoost;
		WeaponPartStatInfo<EnergyCoef> damageCoef; // zero based
		WeaponPartStatInfo<Energy> damageBoost;
		WeaponPartStatInfo<EnergyCoef> armourPiercing; // adds
		WeaponPartStatInfo<Energy> storage;
		WeaponPartStatInfo<Energy> storageOutputSpeed;
		WeaponPartStatInfo<Energy> storageOutputSpeedAdd;
		WeaponPartStatInfo<EnergyCoef> storageOutputSpeedAdj;
		WeaponPartStatInfo<Energy> magazine;
		WeaponPartStatInfo<float> magazineCooldown;
		WeaponPartStatInfo<float> magazineCooldownCoef;
		WeaponPartStatInfo<Energy> magazineOutputSpeed;
		WeaponPartStatInfo<Energy> magazineOutputSpeedAdd;
		WeaponPartStatInfo<bool> continuousFire;
		WeaponPartStatInfo<float> projectileSpeed;
		WeaponPartStatInfo<float> projectileSpread; // muzzle overrides chassis (in angles)
		WeaponPartStatInfo<float> projectileSpeedCoef;
		WeaponPartStatInfo<float> projectileSpreadCoef; // 100% + this
		WeaponPartStatInfo<int> numberOfProjectiles; // muzzle overrides chassis
		WeaponPartStatInfo<float> antiDeflection;
		WeaponPartStatInfo<float> maxDistCoef; // 100% + this
		WeaponPartStatInfo<float> kineticEnergyCoef; // how much of the damage goes into kinectic force (+ pushes away, - pulls closer)
		WeaponPartStatInfo<float> confuse; // confussion time
		WeaponPartStatInfo<EnergyCoef> mediCoef; // how much of the damage goes into healing

		mutable Concurrency::SpinLock schematicCreationLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.WeaponPart.schematicCreationLock"));
		Concurrency::Counter creatingSchematic;
		RefCountObjectPtr<Schematic> visibleSchematic; // not cleared when we request a new schematic
		RefCountObjectPtr<Schematic> schematic;

		struct SchematicSlot
		{
			Name slotId; // if none, chassis
			Range2 at;
			WeaponPartType::Type weaponPartType;
			WeaponPartAddress address;
			Random::Generator rg;
		};
		RefCountObjectPtr<Schematic> containerSchematic;
		RefCountObjectPtr<Schematic> visibleContainerSchematic;
		bool containerSchematicSlotsAreAccessible = false;
		Array<SchematicSlot> containerSchematicSlots;

		void be_instance(bool _nonRandomised, bool _defaultMeshParams);

		bool check_against(IO::XML::Node const* _node);
		bool load_from_xml(IO::XML::Node const* _node);

		void build_schematic_mesh();
		void build_container_schematic_mesh();

		void add_container_schematic_slot(Name const& slotId, Vector2 const& _at, Random::Generator const& rg, WeaponPartType::Type _weaponPartType, WeaponPartAddress const& _address);

	};

	#include "weaponPart.inl"
};

