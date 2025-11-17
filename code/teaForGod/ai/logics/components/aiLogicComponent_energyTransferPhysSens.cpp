#include "aiLogicComponent_energyTransferPhysSens.h"

#include "..\..\..\modules\gameplay\modulePilgrimHand.h"

#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

EnergyTransferPhysSens::~EnergyTransferPhysSens()
{
	if (physSens.is_set())
	{
		PhysicalSensations::stop_sensation(physSens.get());
		physSens.clear();
	}
}

void EnergyTransferPhysSens::update(bool _isTransferring, Framework::IModulesOwner* _toFromObject)
{
	bool shouldPlayPhysSens = _isTransferring;
	Optional<Hand::Type> forHand;
	if (shouldPlayPhysSens)
	{
		shouldPlayPhysSens = false;
		if (_toFromObject)
		{
			if (Framework::GameUtils::is_controlled_by_local_player(_toFromObject))
			{
				if (auto* mph = _toFromObject->get_gameplay_as<ModulePilgrimHand>())
				{
					shouldPlayPhysSens = true;
					forHand = mph->get_hand();
				}
			}
		}
	}
	shouldPlayPhysSens &= forHand.is_set();
	if (shouldPlayPhysSens && !physSens.is_set())
	{
		PhysicalSensations::OngoingSensation s(PhysicalSensations::OngoingSensation::EnergyCharging);
		s.for_hand(forHand.get());
		physSens = PhysicalSensations::start_sensation(s);
	}
	else if (physSens.is_set() && !shouldPlayPhysSens)
	{
		PhysicalSensations::stop_sensation(physSens.get());
		physSens.clear();
	}
}
