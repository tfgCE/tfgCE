#pragma once

#include "..\..\core\memory\safeObject.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\library\usedLibraryStored.h"

#include "..\game\energy.h"

#include "weaponPartType.h"

namespace Framework
{
	class ItemType;
	class ObjectType;
	class TemporaryObjectType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class LootSelector;

	namespace CustomModules
	{
		class LootProvider;
	};

	struct LootSelectorContext
	{
		CustomModules::LootProvider* lootProvider = nullptr;
#ifdef WITH_CRYSTALITE
		int crystalite = 0;
#endif
	};

	class LootSelector
	: public Framework::LibraryStored
	, public SafeObject<LootSelector>
	{
		FAST_CAST_DECLARE(LootSelector);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		static void initialise_static();
		static void close_static();
		static void clear_static();
		static void create_loot_for(LootSelectorContext & _context);
		static void build_for_game(); // if required
		static void output_loot_selector();
		static void output_loot_providers();

		static int compare_safe_ptrs_by_priority(void const* _a, void const* _b);

	public:
		LootSelector(Framework::Library * _library, Framework::LibraryName const & _name);

		int get_priority() const { return priority; }
		float get_prob_coef_for(CustomModules::LootProvider const* _lootProvider) const;
		bool is_specialisation_of(LootSelector const* _lootSelector) const;

		bool check_conditions_for(CustomModules::LootProvider const* _lootProvider) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	protected:
		virtual ~LootSelector();

	protected:
		// structure
		//

		int priority = 0; // priority is used to throw away options with a smaller priority. For the same priority, one option is chosen using probCoef
		Array<Framework::UsedLibraryStored<LootSelector>> specialisationOf; // if is specialisation of other, means that first the other has to be selected - this gives a tree like structure
		Array<Name> mulProbCoefByTag; // probCoef is multiplied by value of tag
		Array<Name> overrideProbCoefByTag; // probCoef is overriden by value of tag
		Array<Name> codeProbCoefTag; // to allow the code alter it
		float probCoef = 1.0f;

		// conditions
		//
		
		TagCondition lootTags;

		// contents
		//

		struct Loot
		{
			struct EnergyQuantum
			{
				Name type;
				Random::Number<Energy> energy;
				Random::Number<Energy> sideEffectDamage;

				bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			};
			struct Object
			{
				Framework::UsedLibraryStored<Framework::ObjectType> objectType;
				SimpleVariableStorage variables;

				bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			};
			struct WeaponPart
			{
				Framework::UsedLibraryStored<TeaForGodEmperor::WeaponPartType> weaponPartType;
				TagCondition weaponPartTags;

				bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			};

			Array<EnergyQuantum> energyQuantums;
			Array<Object> objects;
			bool makeWeapon = false; // by default weapon parts are used to combine into a weapon
			Array<WeaponPart> weaponParts;
		} loot;

		void create_loot_objects_for(LootSelectorContext& _context) const;
#ifdef WITH_CRYSTALITE
		static void create_crystalite_for(LootSelectorContext& _context);
#endif
	};
};
