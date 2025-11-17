#pragma once

#include "..\..\..\..\core\physicalSensations\physicalSensations.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			struct EnergyTransferPhysSens
			{
				~EnergyTransferPhysSens();

				void update(bool _isTransferring, Framework::IModulesOwner* _toFromObject);
			
			private:
				Optional<PhysicalSensations::Sensation::ID> physSens;
			};
		};
	};
};