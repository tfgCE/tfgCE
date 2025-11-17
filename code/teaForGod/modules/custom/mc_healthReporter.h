#pragma once

#include "..\..\game\energy.h"
#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	Gets information from all attached objects and itself, checks health reporters first then health modules
		 */
		class HealthReporter
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(HealthReporter);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			HealthReporter(Framework::IModulesOwner* _owner);
			virtual ~HealthReporter();

			Energy get_nominal_health() const { return nominalHealth; }
			Energy get_actual_health() const { return actualHealth; }

			void update();

		protected:
			int lastFrameUpdate = 0;

			Energy nominalHealth = Energy(0);
			Energy actualHealth = Energy(0);

			struct Entry
			{
				SafePtr<Framework::IModulesOwner> attached;
				Energy nominalHealth = Energy(0);
				Energy actualHealth = Energy(0);
			};
			Array<Entry> entries;

			void update_entry(Framework::IModulesOwner* imo);
		};
	};
};

