#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class PowerSource
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(PowerSource);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			PowerSource(Framework::IModulesOwner* _owner);
			virtual ~PowerSource();

			void set_active(float _active) { active = clamp(_active, 0.0f, 1.0f); }
			float get_active() const { return active; }

			float get_nominal_power_output() const { return nominalPowerOutput; }
			void set_nominal_power_output(float _powerOutput) { nominalPowerOutput = _powerOutput; }
			
			float get_possible_power_output() const { return possiblePowerOutput.is_set() ? possiblePowerOutput.get() : get_nominal_power_output() * useNominal; }
			void set_possible_power_output(Optional<float> const & _possiblePowerOutput = NP) { possiblePowerOutput = _possiblePowerOutput; }

			float get_current_power_output() const { return currentPowerOutput.is_set()? currentPowerOutput.get() : get_possible_power_output() * active; }
			void set_current_power_output(Optional<float> const & _currentPowerOutput = NP) { currentPowerOutput = _currentPowerOutput; }

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		protected:
			// nominal -useNominal-> possible -active-> current
			float nominalPowerOutput = 0.0f; // this is nominal power output
			float useNominal = 1.0f;
			Optional<float> possiblePowerOutput;
			float active = 1.0f;
			Optional<float> currentPowerOutput;
		};
	};
};

