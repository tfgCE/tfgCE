#include "aiLogic_exmReroll.h"

#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai message
DEFINE_STATIC_NAME(reroll);

//

REGISTER_FOR_FAST_CAST(EXMReroll);

EXMReroll::EXMReroll(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMReroll::~EXMReroll()
{
}

void EXMReroll::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && owner.get() && hand != Hand::MAX)
	{
		if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			if (exmModule)
			{
				if (exmModule->mark_exm_active_blink())
				{
					if (auto * message = owner->get_in_world()->create_ai_message(NAME(reroll)))
					{
						message->to_room(owner->get_presence()->get_in_room());
					}
				}
			}
		}
	}
}
