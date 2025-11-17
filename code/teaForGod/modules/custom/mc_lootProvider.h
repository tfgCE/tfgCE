#pragma once

#include "..\..\teaEnums.h"
#include "..\..\teaForGod.h"
#include "..\..\game\energy.h"

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"
#include "..\..\..\core\random\randomNumber.h"
#include "..\..\..\core\types\colour.h"

namespace Framework
{
	class ItemType;
};

namespace TeaForGodEmperor
{
	class LootSelector;
	class Persistence;

	namespace CustomModules
	{
		class LootProviderData;

		/**
		 *	releases specific objects on death
		 */
		class LootProvider
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(LootProvider);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			LootProvider(Framework::IModulesOwner* _owner);
			virtual ~LootProvider();

		public:
			Tags const& get_loot_tags() const;

			void release_loot(Vector3 const & _hitRelativeDir, Framework::IModulesOwner* _instigator);

			Energy get_experience() const;

			bool are_weapon_parts_from_weapon_setup_container() const;
			Name const & get_weapon_parts_from_weapon_setup_container_id() const;

			bool is_weapon_setup_from_weapon_setup_container() const;
			Name const & get_weapon_setup_from_weapon_setup_container_id() const;

		public:	// Module
			override_ void ready_to_activate();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);
			override_ void on_owner_destroy();

		private: friend class TeaForGodEmperor::LootSelector;
			void add_loot(Framework::IModulesOwner* _item, bool _weaponSetupContainerLoot = false);

		private:
			LootProviderData const * lootProviderData = nullptr;

			bool noLoot = false;

			Array<SafePtr<Framework::IModulesOwner>> lootItems;
			Array<SafePtr<Framework::IModulesOwner>> weaponSetupContainerLootItems;

			void destroy_not_released_loot();
		};

		class LootProviderData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(LootProviderData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			LootProviderData(Framework::LibraryStored* _inLibraryStored);
			virtual ~LootProviderData();

			void output_log(LogInfoContext& _log) const;

		protected: // Framework::ModuleData
			override_ bool read_parameter_from(IO::XML::Attribute const* _attr, Framework::LibraryLoadingContext& _lc);
			override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

		private:
			Tags lootTags;
			float probability = 1.0f; // probability to be used at all, loot selector (via lootTags) just tells which one should be used
			bool weaponPartsFromWeaponSetupContainer = false;
			Name weaponPartsFromWeaponSetupContainerId;
			bool weaponSetupFromWeaponSetupContainer = false;
			Name weaponSetupFromWeaponSetupContainerId;

#ifdef WITH_CRYSTALITE
			int crystalite = 0;
#endif

			Energy provideExperience = Energy::zero();

			friend class LootProvider;
		};
	};
};

