#include "aiManagerLogic_dischargeZone.h"

#include "..\..\..\game\game.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\utils\lightningDischarge.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// params/variables
DEFINE_STATIC_NAME(inRoom);

// lightnings
DEFINE_STATIC_NAME(idle);
DEFINE_STATIC_NAME_STR(idleDischarge, TXT("idle discharge"));
DEFINE_STATIC_NAME_STR(idleDischargeMiss, TXT("idle discharge miss"));

//

REGISTER_FOR_FAST_CAST(DischargeZoneManager);

DischargeZoneManager::DischargeZoneManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, dischargeZoneManagerData(fast_cast<DischargeZoneManagerData>(_logicData))
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);

	toDischargeTimeLeft = random.get(dischargeZoneManagerData->dischargeInterval);
}

DischargeZoneManager::~DischargeZoneManager()
{
}

LATENT_FUNCTION(DischargeZoneManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	auto * self = fast_cast<DischargeZoneManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->okToAdvance = false;

	// wait a bit to make sure POIs are there
	LATENT_WAIT(1.0f);

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());
	if (!self->inRoom.get())
	{
		self->inRoom = imo->get_presence()->get_in_room();
	}

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	ai_log_no_colour(self);

	if (self->inRoom.get())
	{
		self->inRoom->for_every_point_of_interest(self->dischargeZoneManagerData->poiName, [self](Framework::PointOfInterestInstance* _fpoi)
			{
				self->poiPlacements.push_back(_fpoi->calculate_placement());
			});
		if (! self->poiPlacements.is_empty())
		{
			self->okToAdvance = true;
		}
	}

	while (true)
	{
		LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void DischargeZoneManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (! okToAdvance)
	{
		return;
	}
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (!dischargeZoneDischarger.get() &&
			!dzdDOC.get() &&
			!poiPlacements.is_empty())
		{
			Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
			doc->activateImmediatelyEvenIfRoomVisible = true;
			doc->wmpOwnerObject = imo;
			doc->inRoom = inRoom.get();
			doc->name = TXT("discharge zone discharger");
			doc->objectType = dischargeZoneManagerData->dischargeSceneryType.get();
			doc->placement = Transform(Vector3::zAxis * 1.0f, Quat::identity);
			doc->randomGenerator = random.spawn();
			doc->priority = 1000;
			doc->checkSpaceBlockers = false;

			inRoom->collect_variables(doc->variables);

			dzdDOC = doc;

			Game::get()->queue_delayed_object_creation(doc);
		}

		if (auto* doc = dzdDOC.get())
		{
			if (doc->is_done())
			{
				dischargeZoneDischarger = doc->createdObject.get();
				dischargeZoneDischarger->set_instigator(imo);
				dzdDOC.clear();
			}
		}
	}

	if (auto* dzdIMO = dischargeZoneDischarger.get())
	{
		bool throwIdle = false;
		if (isIdling)
		{
			throwIdle = true;
			toDischargeTimeLeft -= _deltaTime;
			if (toDischargeTimeLeft <= 0.0f)
			{
				isIdling = false;
				preparationTimeLeft = dischargeZoneManagerData->preparationTime;
				preparationTimeLeft *= (1.0f - 0.3f * GameSettings::get().difficulty.aiMean);
				dischargesLeft = 0; 
				if (dischargeZoneManagerData->preparationSoundID.is_valid())
				{
					if (auto* s = dzdIMO->get_sound())
					{
						s->play_sound(dischargeZoneManagerData->preparationSoundID);
					}
				}
			}
		}
		else
		{
			throwIdle = preparationTimeLeft > dischargeZoneManagerData->preparationTime - 1.0f;
			preparationTimeLeft -= _deltaTime;
			if (preparationTimeLeft <= 0.0f)
			{
				if (dischargesLeft == 0)
				{
					dischargesLeft = random.get(dischargeZoneManagerData->dischargesAmount);
				}
				int processNow = dischargeZoneManagerData->dischargesBatch;
				while (processNow > 0 && dischargesLeft > 0)
				{
					if (auto* r = dzdIMO->get_presence()->get_in_room())
					{
						if (r->get_distance_to_recently_seen_by_player() < 3)
						{
							if (auto* ls = dzdIMO->get_custom<Framework::CustomModules::LightningSpawner>())
							{
								Transform placement = poiPlacements[random.get_int(poiPlacements.get_size())];
								LightningDischarge::Params params;

								params.for_imo(dzdIMO);
								params.for_instigator(get_mind()->get_owner_as_modules_owner()); // we need to track instigator to here to get the AI
								params.with_start_placement_OS(dzdIMO->get_presence()->get_placement().to_local(placement));
								params.with_max_dist(10.0f); // enough to catch everything
								params.with_setup_damage([this](Damage& dealDamage, DamageInfo& damageInfo)
									{
										dealDamage.damage = dischargeZoneManagerData->multipleDischargeDamage;
										dealDamage.cost = dischargeZoneManagerData->multipleDischargeDamage;
									});
								params.with_ray_cast_search_for_objects(false); // wide search is enough here
								params.in_single_room();
								params.ignore_narrative();

								LightningDischarge::perform(params);
							}
						}
					}
					--processNow;
					--dischargesLeft;
				}
				if (dischargesLeft <= 0)
				{
					toDischargeTimeLeft = random.get(dischargeZoneManagerData->dischargeInterval);
					isIdling = true;
				}
			}
		}

		if (throwIdle)
		{
			idleIntervalTimeLeft -= _deltaTime;
			if (idleIntervalTimeLeft <= 0.0f)
			{
				idleIntervalTimeLeft = random.get(dischargeZoneManagerData->idleInterval);

				if (auto* r = dzdIMO->get_presence()->get_in_room())
				{
					// only spawn idles when close, even if the won't get spawned, they will play a sound
					if (r->get_distance_to_recently_seen_by_player() < 3)
					{
						if (auto* ls = dzdIMO->get_custom<Framework::CustomModules::LightningSpawner>())
						{
							Transform placement = poiPlacements[random.get_int(poiPlacements.get_size())];
							placement = dzdIMO->get_presence()->get_placement().to_local(placement);
							if (random.get_chance(dischargeZoneManagerData->idleDischargeChance))
							{
								LightningDischarge::Params params;

								params.for_imo(dzdIMO);
								params.for_instigator(get_mind()->get_owner_as_modules_owner()); // we need to track instigator to here to get the AI
								params.with_start_placement_OS(placement);
								params.with_max_dist(10.0f); // enough to catch everything
								params.with_setup_damage([this](Damage& dealDamage, DamageInfo& damageInfo)
									{
										dealDamage.damage = dischargeZoneManagerData->idleDischargeDamage;
										dealDamage.cost = dischargeZoneManagerData->idleDischargeDamage;
									});
								params.with_ray_cast_search_for_objects(false); // wide search is enough here
								params.in_single_room();
								params.ignore_narrative();
								params.with_hit_id(NAME(idleDischarge));
								params.with_miss_id(NAME(idleDischargeMiss));

								LightningDischarge::perform(params);
							}
							else
							{
								Framework::CustomModules::LightningSpawner::LightningParams params;
								params.with_start_placement_os(placement);
								params.with_count(1);
								ls->single(NAME(idle), params);
							}
						}
					}
				}
			}
		}
	}
}

void DischargeZoneManager::debug_draw(Framework::Room* _room) const
{
/*
#ifdef AN_DEBUG_RENDERER
	debug_context(_room);

	debug_no_context();
#endif
*/
}

//

REGISTER_FOR_FAST_CAST(DischargeZoneManagerData);

DischargeZoneManagerData::DischargeZoneManagerData()
{
}

DischargeZoneManagerData::~DischargeZoneManagerData()
{
}

bool DischargeZoneManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	dischargeSceneryType.load_from_xml(_node, TXT("dischargeSceneryType"), _lc);

	XML_LOAD_NAME_ATTR(_node, poiName);
	XML_LOAD(_node, idleInterval);
	XML_LOAD_FLOAT_ATTR(_node, idleDischargeChance);

	XML_LOAD(_node, dischargeInterval);
	XML_LOAD_NAME_ATTR(_node, preparationSoundID);
	XML_LOAD_FLOAT_ATTR(_node, preparationTime);
	XML_LOAD(_node, multipleDischargeDamage);
	XML_LOAD(_node, idleDischargeDamage);

	XML_LOAD(_node, dischargesAmount);
	XML_LOAD_INT_ATTR(_node, dischargesBatch);

	return result;
}

bool DischargeZoneManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= dischargeSceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
