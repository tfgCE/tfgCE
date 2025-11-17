#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\types\optional.h"

#include "..\teaEnums.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	interface_class IModulesOwner;
}

namespace TeaForGodEmperor
{
	struct Energy;

	class Modules
	{
	public:
		static void initialise_static();

		enum CalculateTotalEnergyFlags
		{
			OnlyOwn = 1
		};
		static Energy calculate_total_energy_of(Framework::IModulesOwner const * _imo, Optional<EnergyType::Type> const & _ofType = NP, int _flags = 0);
	};
};
