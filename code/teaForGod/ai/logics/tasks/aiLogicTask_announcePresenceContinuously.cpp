#include "aiLogicTask_announcePresenceContinuously.h"

#include "..\..\aiCommonVariables.h"

#include "..\..\..\game\gameSettings.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\general\cooldowns.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// sounds
DEFINE_STATIC_NAME(announcePresence);

//

bool TeaForGodEmperor::AI::Logics::ShouldTask::announce_presence_continuously(Framework::IModulesOwner* imo)
{
	// always does it
	if (auto* s = imo->get_sound())
	{
		return s->does_handle_sound(NAME(announcePresence));
	}
	return false;
}

//


LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::announce_presence_continuously)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("announce_presence"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Framework::CooldownsSmall, cooldowns);

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	while (true)
	{
		if (cooldowns.is_available(NAME(announcePresence)))
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* p = imo->get_presence())
				{
					if (auto* r = p->get_in_room())
					{
						int distance = r->get_distance_to_recently_seen_by_player();
						if (distance <= 6)
						{
							if (auto* s = imo->get_sound())
							{
								s->play_sound(NAME(announcePresence));
							}
							cooldowns.set_if_longer(NAME(announcePresence), Random::get_float(4.0f, 12.0f));
						}
					}
				}
			}
		}
		LATENT_WAIT(Random::get_float(0.3f, 2.0f));
	}

	LATENT_ON_BREAK();
	//
	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

