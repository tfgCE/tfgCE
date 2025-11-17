#include "aiLogic_steamer.h"

#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\framework.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// temporary objects
DEFINE_STATIC_NAME(steam);

// params
DEFINE_STATIC_NAME(steamInterval);
DEFINE_STATIC_NAME(steamLength);

//

REGISTER_FOR_FAST_CAST(Steamer);

Steamer::Steamer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

Steamer::~Steamer()
{
}

void Steamer::learn_from(SimpleVariableStorage& _parameters)
{
	steamInterval = _parameters.get_value<Range>(NAME(steamInterval), steamInterval);
	steamLength = _parameters.get_value<Range>(NAME(steamLength), steamLength);
}

void Steamer::advance(float _deltaTime)
{
	set_auto_rare_advance_if_not_visible(Range(0.1f, 0.5f));

	base::advance(_deltaTime);
}

LATENT_FUNCTION(Steamer::execute_logic)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic steamer"));

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Random::Generator, rg);

	auto* self = fast_cast<Steamer>(logic);

	LATENT_BEGIN_CODE();

	while (true)
	{
		LATENT_WAIT(rg.get(self->steamInterval));
		self->start_steam();
		LATENT_WAIT(rg.get(self->steamLength));
		self->stop_steam();
	}

	LATENT_ON_BREAK();
	//
	
	LATENT_ON_END();
	//
	self->stop_steam();

	LATENT_END_CODE();
	LATENT_RETURN();
}

void Steamer::start_steam()
{
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* r = imo->get_presence()->get_in_room())
			{
				if (r->get_distance_to_recently_seen_by_player() > 3
#ifdef AN_DEVELOPMENT_OR_PROFILER
					&& ! Framework::is_preview_game()
#endif
					)
				{
					return;
				}
			}
			if (auto* to = imo->get_temporary_objects())
			{
				to->spawn_all(NAME(steam), NP, &steamObjects);
			}
		}
	}
}

void Steamer::stop_steam()
{
	for_every_ref(so, steamObjects)
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(so))
		{
			to->desire_to_deactivate();
		}
	}
	steamObjects.clear();
}

