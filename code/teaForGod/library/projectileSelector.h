#pragma once

#include "..\..\core\memory\safeObject.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\library\usedLibraryStored.h"

#include "..\game\energy.h"

#include "..\pilgrim\pilgrimStatsInfo.h"

#include "weaponPartType.h"

namespace Framework
{
	class TemporaryObjectType;
};

namespace TeaForGodEmperor
{
	namespace ModuleEquipments
	{
		class Gun;
		struct GunStats;
	};

	struct ProjectileSelectionSource
	{
		// in this order
		ModuleEquipments::Gun const* gun = nullptr; // we need gun to get exms
		PilgrimStatsInfoHand::MainEquipment const* statsME = nullptr;

		WeaponCoreKind::Type get_core_kind() const;
		Energy get_single_projectile_damage() const;
		Energy get_total_projectile_damage() const;
		float get_projectile_speed() const;
		float get_projectile_spread() const;
		int get_projectiles_per_shot() const;
		bool does_have_special_feature(Name const & _specialFeature) const;
	};

	struct ProjectileSelectionInfo
	{
		bool numerousAsOne = false;
		bool noSpread = false;
	};

	class ProjectileSelector
	: public Framework::LibraryStored
	, public SafeObject<ProjectileSelector>
	{
		FAST_CAST_DECLARE(ProjectileSelector);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		static void initialise_static();
		static void close_static();
		static void clear_static();
		static void build_for_game(); // if required
		static void output_projectile_selector();
		static Framework::TemporaryObjectType const* select_for(ProjectileSelectionSource const & _source, OUT_ ProjectileSelectionInfo & _projectileSelectionInfo);

		static int compare_safe_ptrs_by_priority(void const* _a, void const* _b);

	public:
		ProjectileSelector(Framework::Library * _library, Framework::LibraryName const & _name);

		Framework::TemporaryObjectType const* get_projectile() const { return projectile.get(); }
		bool is_numerous_as_one() const { return numerousAsOne; }
		bool is_no_spread() const { return noSpread; }

		int get_priority() const { return priority; }
		ProjectileSelector const* get_specialisation_of() const { return specialisationOf.get(); }

		bool check_conditions_for(ProjectileSelectionSource const& _source) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~ProjectileSelector();

	protected:
		Framework::UsedLibraryStored<Framework::TemporaryObjectType> projectile;
		bool numerousAsOne = false;
		bool noSpread = false;

		int priority = 0;
		Framework::UsedLibraryStored<ProjectileSelector> specialisationOf; // if is specialisation of other, means that first the other has to be selected - this gives a tree like structure

		struct Condition
		{
			Name param;
			Name operation;
			// depending on param:
			Energy energyRef = Energy::zero();
			float floatRef = 0;
			int intRef = 0;
			Name nameRef = Name::invalid();

			bool load_from_xml(IO::XML::Node const* _node);

			bool is_weapon_core_kind() const;
			bool is_energy() const;
			bool is_float() const;
			bool is_int() const;
			bool is_name() const;

			bool check_for(ProjectileSelectionSource const& _source) const;
		};
		Array<Condition> conditions;

	};
};
