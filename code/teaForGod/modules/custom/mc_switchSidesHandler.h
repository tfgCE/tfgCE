#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"

#include "..\..\utils\interactiveButtonHandler.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	Allows to switch sides, changes social settings (sets faction to match instigator's faction).
		 *	Perception task considers switch sides handler and knows on which sides are we - so this is working automatically.
		 */
		class SwitchSidesHandler
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(SwitchSidesHandler);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			SwitchSidesHandler(Framework::IModulesOwner* _owner);
			virtual ~SwitchSidesHandler();

		public:
			Framework::IModulesOwner* get_switch_sides_to() const { return switchSidesTo.get(); }

			void get_switch_sides_to(Framework::IModulesOwner* _side);

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);
			override_ void activate();
			override_ void on_owner_destroy();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		private:
			bool hasToBeHeld = true; // held by user
			bool switchOnUseUsableByHolder = false; // it implies that it has to be held by user
			Name interactiveDeviceIdVar;
			InteractiveButtonHandler interactiveButtonHandler;
			
			Name switchedSidesVar;

			Name emissiveOnSwitchSides;
		
			SafePtr<Framework::IModulesOwner> switchSidesTo;

			void update_emissives();
		};
	};
};

