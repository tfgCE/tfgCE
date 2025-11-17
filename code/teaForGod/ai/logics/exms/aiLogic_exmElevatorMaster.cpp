#include "aiLogic_exmElevatorMaster.h"

#include "..\..\..\game\gameDirector.h"

#include "..\..\..\modules\gameplay\modulePilgrim.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

REGISTER_FOR_FAST_CAST(EXMElevatorMaster);

EXMElevatorMaster::EXMElevatorMaster(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMElevatorMaster::~EXMElevatorMaster()
{
}

void EXMElevatorMaster::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && owner.get() && hand != Hand::MAX)
	{
		if (!initialised)
		{
			initialised = true;
			if (auto* gd = GameDirector::get())
			{
				gd->game_set_elavators_follow_player_only(true);
			}
		}
	}
}

void EXMElevatorMaster::end()
{
	base::end();

	if (auto* gd = GameDirector::get())
	{
		gd->game_set_elavators_follow_player_only(false);
	}
}

