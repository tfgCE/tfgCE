#include "aiLogic_exmDistractor.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\..\framework\module\moduleSound.h"

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

// variables
DEFINE_STATIC_NAME(distractionTime);

// sound
DEFINE_STATIC_NAME(distract);

//

REGISTER_FOR_FAST_CAST(EXMDistractor);

EXMDistractor::EXMDistractor(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMDistractor::~EXMDistractor()
{
}

void EXMDistractor::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	distractionTime = _parameters.get_value<float>(NAME(distractionTime), distractionTime);
}

void EXMDistractor::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX)
	{
		if (auto* gd = GameDirector::get())
		{
			//if (!gd->game_is_no_violence_forced())
			{
				if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
					pilgrimModule->is_active_exm_equipped(hand, owner.get()))
				{
					bool distract = exmModule->mark_exm_active_blink();

					if (distract)
					{
						gd->game_force_no_violence_for(distractionTime);
						if (auto* s = owner->get_sound())
						{
							s->play_sound(NAME(distract));
						}
					}
				}
			}
		}
	}
}
