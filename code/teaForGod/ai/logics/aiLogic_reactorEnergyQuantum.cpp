#include "aiLogic_reactorEnergyQuantum.h"

#include "..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiMindInstance.h"

#include "..\..\..\framework\module\modulePresence.h"

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

// object vars
DEFINE_STATIC_NAME(movementSpeed);

//

REGISTER_FOR_FAST_CAST(ReactorEnergyQuantum);

ReactorEnergyQuantum::ReactorEnergyQuantum(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

ReactorEnergyQuantum::~ReactorEnergyQuantum()
{
}

void ReactorEnergyQuantum::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	movementSpeed = _parameters.get_value<float>(NAME(movementSpeed), movementSpeed);
}

LATENT_FUNCTION(ReactorEnergyQuantum::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(int, i);
	
	LATENT_VAR(Random::Generator, rg);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<ReactorEnergyQuantum>(logic);

	LATENT_BEGIN_CODE();

	i = 0;
	while (i >= 0)
	{
		{
			Name reactorPOI(String::printf(TXT("reactor_%i"), i));
			if (auto* r = imo->get_presence()->get_in_room())
			{
				Framework::PointOfInterestInstance* foundPOI = nullptr;
				if (r->find_any_point_of_interest(reactorPOI, OUT_ foundPOI, false, &rg))
				{
					if (foundPOI)
					{
						self->reactorPath.push_back(foundPOI->calculate_placement());
					}
					else
					{
						i = -1;
					}
				}
				else
				{
					i = -1;
				}
			}
			else
			{
				i = -1;
			}
		}
		if (i >= 0)
		{
			++i;
		}
		LATENT_YIELD();
	}

	while (true)
	{
		{
			Vector3 loc = imo->get_presence()->get_placement().get_translation();
			int targetIdx = self->reactorPath.get_size() - 1;
			for_every(p, self->reactorPath)
			{
				if (p->location_to_local(loc).y < 0.0f)
				{
					targetIdx = for_everys_index(p);
					break;
				}
			}
			int prevIdx = max(0, targetIdx - 1);
			targetIdx = prevIdx + 1;
			Vector3 nt = self->reactorPath[targetIdx].get_translation();
			Vector3 np = self->reactorPath[prevIdx].get_translation();
			Vector3 dp2t = (nt - np).normal();
			float along = Vector3::dot(dp2t, loc - np);
			Vector3 shouldBeAt = np + dp2t * along;
			
			Vector3 targetVelocity = lerp(0.5f, (nt - np).normal(), (shouldBeAt - loc).normal()).normal();
			targetVelocity.x += rg.get_float(-0.1f, 0.1f);
			targetVelocity.y += rg.get_float(-0.1f, 0.1f);
			targetVelocity.z += rg.get_float(-0.1f, 0.1f);
			targetVelocity = targetVelocity.normal() * self->movementSpeed;

			Vector3 currentVelocity = imo->get_presence()->get_velocity_linear();
			Vector3 impulse = (targetVelocity - currentVelocity) * 0.7f;
			imo->get_presence()->clear_velocity_impulses();
			imo->get_presence()->add_velocity_impulse(impulse);
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

