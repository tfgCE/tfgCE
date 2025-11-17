#include "aiLogicComponent_movementGaitTimeBased.h"

#include "..\aiLogic_npcBase.h"

#include "..\..\..\..\core\containers\arrayStack.h"
#include "..\..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define DEBUG_REFILL

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

MovementGaitTimeBased::MovementGaitTimeBased()
{
}

MovementGaitTimeBased::~MovementGaitTimeBased()
{
}

void MovementGaitTimeBased::set_owner_logic(NPCBase* _npcBase)
{
	owner = _npcBase;
}

void MovementGaitTimeBased::advance(float _deltaTime)
{
	if (active)
	{
		timeLeft -= _deltaTime;
		if (timeLeft < 0.0f)
		{
			availableGaits.clear();
			availableGaits.make_space_for(gaits.get_size());
			for_every(gs, gaits)
			{
				if (!setGait.is_set() || setGait.get() != gs->gait)
				{
					availableGaits.push_back(*gs);
				}
			}

			if (availableGaits.is_empty())
			{
				setGait.clear();
			}
			else
			{
				int idx = RandomUtils::ChooseFromContainer<Array<GaitSetup>, GaitSetup>::choose(rg, availableGaits,
					[](GaitSetup const& _gs) { return _gs.probCoef; });
				auto& gs = availableGaits[idx];
				selectedGait = gs.gait;
				timeLeft = rg.get(gs.timeInGait);
			}
		}

		if (selectedGait.is_set())
		{
			if (!setGait.is_set() || setGait.get() != selectedGait.get())
			{
				setGait = selectedGait;
				if (owner)
				{
					owner->set_movement_gait(setGait.get());
				}
			}
		}
	}
	else
	{
		selectedGait.clear();
		setGait.clear();
		timeLeft = 0.0f;
	}
}

void MovementGaitTimeBased::set_active(bool _active)
{
	active = _active;
}

void MovementGaitTimeBased::add_gait(GaitSetup const& _gs)
{
	gaits.push_back(_gs);
}

void MovementGaitTimeBased::add_gait(Name const& _gait, Range const& _timeInGait, Optional<float> const& _probCoef)
{
	gaits.push_back(GaitSetup(_gait, _timeInGait, _probCoef));
}