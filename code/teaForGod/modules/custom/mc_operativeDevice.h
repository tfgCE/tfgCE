#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	Gets information about status of device, if fully operational or not or if affected by user
		 */
		class OperativeDevice
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(OperativeDevice);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			OperativeDevice(Framework::IModulesOwner* _owner);
			virtual ~OperativeDevice();

			bool is_not_fully_operative_because_of_user() const; // someone modified it and it can't get fully operative status (damaged, disabled, etc).

		public: // Module
			override_ void activate();

		private:
			SimpleVariableInfo deviceOrderedStateVar;
		};
	};
};

