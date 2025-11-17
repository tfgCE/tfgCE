#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\core\types\colour.h"
#include "..\..\game\weaponPart.h"
#include "..\..\game\weaponSetup.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class WeaponSetupContainer
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(WeaponSetupContainer);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			WeaponSetupContainer(Framework::IModulesOwner* _owner);
			virtual ~WeaponSetupContainer();

			Name const& get_id() const { return id; }

			WeaponSetup & access_weapon_setup() { return weaponSetup; }
			WeaponSetup const& get_weapon_setup() const { return weaponSetup; }

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		protected:
			Name id;
			WeaponSetup weaponSetup;
		};

		class WeaponSetupContainerData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(WeaponSetupContainerData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			struct RandomWeaponParts
			{
				Optional<int> limit; // apply only if using loose parts
				float probCoef = 1.0f;
				Framework::UsedLibraryStored<WeaponPartType> weaponPartType;
				TagCondition tagged;
			};

		public:
			WeaponSetupContainerData(Framework::LibraryStored* _inLibraryStored);
			virtual ~WeaponSetupContainerData();

			WeaponSetupTemplate const& get_weapon_setup() const { return weaponSetup; }
			bool should_fill_with_random_parts() const { return fillWithRandomParts; }
			Optional<bool> const& should_use_non_randomised_parts() const { return nonRandomisedParts; }

			Array<RandomWeaponParts> const& get_random_parts() const { return randomParts; }

		public: // Framework::ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
			override_ void prepare_to_unload();

		private:
			WeaponSetupTemplate weaponSetup;

			Array<RandomWeaponParts> randomParts;

			bool fillWithRandomParts = false;
			Optional<bool> nonRandomisedParts;
		};
	};
};

