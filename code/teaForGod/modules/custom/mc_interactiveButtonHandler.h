#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"

#include "..\..\utils\interactiveButtonHandler.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	Sort of wrapper around interactive button handler
		 */
		class InteractiveButtonHandler
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(InteractiveButtonHandler);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			InteractiveButtonHandler(Framework::IModulesOwner* _owner);
			virtual ~InteractiveButtonHandler();

		public:
			bool is_on() const { return isOn; }

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);
			override_ void activate();
			override_ void on_owner_destroy();

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		private:
			bool hasToBeHeld = true; // held by user
			bool worksWithUseUsableByHolder = true; // it implies that it has to be held by user
			Name interactiveDeviceIdVar;
			TeaForGodEmperor::InteractiveButtonHandler interactiveButtonHandler;

			Name emissiveOnUse;

			bool isOn = false;
		};
	};
};

