#include "aiLogic_coreShaftDischarges.h"

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
DEFINE_STATIC_NAME(firstLightningStrikeID);
DEFINE_STATIC_NAME(lightningStrikeID);

// pois
DEFINE_STATIC_NAME(coreShaftDischarge);

// poi params
DEFINE_STATIC_NAME(pairIdx);
DEFINE_STATIC_NAME(radius);
DEFINE_STATIC_NAME(amount);
DEFINE_STATIC_NAME(interval);

//

REGISTER_FOR_FAST_CAST(CoreShaftDischarges);

CoreShaftDischarges::CoreShaftDischarges(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

CoreShaftDischarges::~CoreShaftDischarges()
{
}

void CoreShaftDischarges::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(CoreShaftDischarges::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Random::Generator, rg);

	LATENT_VAR(Framework::CustomModules::LightningSpawner*, lightningSpawner);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<CoreShaftDischarges>(logic);

	LATENT_BEGIN_CODE();

	lightningSpawner = imo->get_custom<Framework::CustomModules::LightningSpawner>();

	{
		if (auto* r = imo->get_presence()->get_in_room())
		{
			r->for_every_point_of_interest(NAME(coreShaftDischarge), [self](Framework::PointOfInterestInstance* _poi)
				{
					int pairIdx = _poi->get_parameters().get_value<int>(NAME(pairIdx), self->pairs.get_size());
					self->pairs.set_size(max(self->pairs.get_size(), pairIdx + 1));
					auto& p = self->pairs[pairIdx];
					if (p.placements.has_place_left())
					{
						Pair::Placement pl;
						pl.loc = _poi->calculate_placement().get_translation();
						pl.distFromPrev = 0.0f;
						if (!p.placements.is_empty())
						{
							pl.distFromPrev = (pl.loc - p.placements.get_last().loc).length();
							p.totalLength += pl.distFromPrev;
						}
						p.placements.push_back(pl);
					}
					else
					{
						error(TXT("too many placements for core shaft discharges"));
					}
					p.radius = _poi->get_parameters().get_value<float>(NAME(radius), p.radius);
					p.amount = _poi->get_parameters().get_value<RangeInt>(NAME(amount), p.amount);
					p.interval = _poi->get_parameters().get_value<Range>(NAME(interval), p.interval);
					p.firstLightningStrikeID = _poi->get_parameters().get_value<Name>(NAME(firstLightningStrikeID), p.firstLightningStrikeID);
					p.lightningStrikeID = _poi->get_parameters().get_value<Name>(NAME(lightningStrikeID), p.lightningStrikeID);
				});
		}
	}

	while (true)
	{
		if (lightningSpawner &&
			imo->get_presence()->get_in_room())
		{
			float deltaTime = LATENT_DELTA_TIME;
			for_every(p, self->pairs)
			{
				p->timeLeft -= deltaTime;
				if (p->timeLeft <= 0.0f && ! p->placements.is_empty() && p->lightningStrikeID.is_valid())
				{
					p->timeLeft = rg.get(p->interval);
					if (imo->get_presence()->get_in_room()->was_recently_seen_by_player())
					{
						int amount = rg.get(p->amount);
						for_count(int, idx, amount)
						{
							float at = rg.get_float(0.0f, p->totalLength);
							float pt = 0.0f;
							int locIdx = 0;
							while (locIdx < p->placements.get_size())
							{
								auto& pl = p->placements[locIdx];
								if (at <= pl.distFromPrev)
								{
									pt = pl.distFromPrev == 0.0f? 0.0f : at / pl.distFromPrev;
									--locIdx;
									break;
								}
								at -= pl.distFromPrev;
								++locIdx;
							}
							locIdx = clamp(locIdx, 0, p->placements.get_size());
							auto& pc = p->placements[locIdx];
							auto& pn = p->placements[min(locIdx + 1, p->placements.get_size() - 1)];
							Vector3 pdir = (pn.loc - pc.loc).normal();
							Vector3 loc = lerp(pt, pc.loc, pn.loc);
							Vector3 dir;
							dir.x = rg.get_float(-1.0f, 1.0f);
							dir.y = rg.get_float(-1.0f, 1.0f);
							dir.z = rg.get_float(-1.0f, 1.0f);
							dir.normalise();
							float length = p->radius * 2.0f;
							if (!pdir.is_zero())
							{
								dir = dir.drop_using_normalised(pdir).normal();
								Vector3 right = Vector3::cross(pdir, dir);
								right.normalise();

								float off = rg.get_float(-0.95f, 0.95f) * p->radius;
								length = sqrt(max(0.0f, sqr(p->radius) - sqr(off))) * 2.0f; // a' + b' = c'
								loc += right * off;
							}
							Vector3 start = loc - dir * (length * 0.5f);

							if (!dir.is_zero())
							{
								lightningSpawner->single(idx == 0 && p->firstLightningStrikeID.is_valid()? p->firstLightningStrikeID : p->lightningStrikeID,
									Framework::CustomModules::LightningSpawner::LightningParams()
									.with_start_placement_ws(look_matrix_no_up(start, dir).to_transform())
									.with_length(length)
									.forced());
							}
						}
					}
				}
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

