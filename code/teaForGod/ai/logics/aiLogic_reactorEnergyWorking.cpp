#include "aiLogic_reactorEnergyWorking.h"

#include "..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiMindInstance.h"

#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"

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
DEFINE_STATIC_NAME(lightningStrikeID);

//

REGISTER_FOR_FAST_CAST(ReactorEnergyWorking);

ReactorEnergyWorking::ReactorEnergyWorking(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

ReactorEnergyWorking::~ReactorEnergyWorking()
{
}

void ReactorEnergyWorking::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	lightningStrikeID = _parameters.get_value<Name>(NAME(lightningStrikeID), lightningStrikeID);
}

LATENT_FUNCTION(ReactorEnergyWorking::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(int, i);
	
	LATENT_VAR(Random::Generator, rg);

	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<ReactorEnergyWorking>(logic);

	LATENT_BEGIN_CODE();

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

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
		if (lightningSpawner && self->lightningStrikeID.is_valid() &&
			imo->get_presence()->get_in_room() &&
			imo->get_presence()->get_in_room()->was_recently_seen_by_player())
		{
			for (int u = 0; u < 4; ++u)
			{
				int i = rg.get_int(self->reactorPath.get_size() - 2);

				int n = i + 1;
				int n2 = i + 2;

				if (n <= self->reactorPath.get_size() / 2)
				{
					swap(i, n2);
				}
				float pt = rg.get_float(0.0f, 1.0f);
				Vector3 ip = self->reactorPath[i].get_translation();
				Vector3 np = self->reactorPath[n].get_translation();
				Vector3 n2p = self->reactorPath[n2].get_translation();
				Vector3 loc = lerp(pt, ip, np);
				Vector3 tgt = lerp(pt, np, n2p);
				loc.x += rg.get_float(-0.1f, 0.1f);
				loc.y += rg.get_float(-0.1f, 0.1f);
				loc.z += rg.get_float(-0.1f, 0.1f);
				tgt.x += rg.get_float(-0.1f, 0.1f);
				tgt.y += rg.get_float(-0.1f, 0.1f);
				tgt.z += rg.get_float(-0.1f, 0.1f);

				lightningSpawner->single(self->lightningStrikeID,
					Framework::CustomModules::LightningSpawner::LightningParams()
						.with_start_placement_ws(look_matrix_no_up(loc, (tgt-loc).normal()).to_transform())
						.with_length((tgt-loc).length())
						.forced());
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

