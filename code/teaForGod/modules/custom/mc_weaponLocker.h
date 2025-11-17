#pragma once

#include "..\..\pilgrimage\pilgrimageDeviceStateSensitive.h"

#include "..\..\..\framework\module\moduleCustom.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class WeaponLocker
		: public Framework::ModuleCustom
		, public IPilgrimageDeviceStateSensitive
		{
			FAST_CAST_DECLARE(WeaponLocker);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_BASE(IPilgrimageDeviceStateSensitive);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			WeaponLocker(Framework::IModulesOwner* _owner);
			virtual ~WeaponLocker();

			void store_weapon_setup();

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const* _moduleData);

		public: // IPilgrimageDeviceStateSensitive
			implement_ void on_restore_pilgrimage_device_state();

		private:
			Name weaponInsideVar;
			Name weaponInsideRGVar;
			void create_weapon_from_stored_weapon_setup();
		};
	};
};

