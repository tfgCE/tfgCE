#include "aiLogic_exmInvestigator.h"

#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
//

REGISTER_FOR_FAST_CAST(EXMInvestigator);

EXMInvestigator::EXMInvestigator(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMInvestigator::~EXMInvestigator()
{
}

void EXMInvestigator::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && owner.get() && hand != Hand::MAX)
	{
		if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			MODULE_OWNER_LOCK_FOR_IMO(pilgrimOwner.get(), TXT("switch show investigate"));
			pilgrimModule->access_overlay_info().switch_show_investigate();
			pilgrimModule->access_overlay_info().done_tip(PilgrimOverlayInfoTip::InputActivateEXM, hand);
		}
	}
}
