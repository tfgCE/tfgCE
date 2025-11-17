#include "aiManagerLogic_airPursuitManager.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\object\actorType.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\world\room.h"

#include "..\..\..\..\core\debug\\debugRenderer.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_ap_setTarget, TXT("air pursuit; set target"));
DEFINE_STATIC_NAME_STR(aim_af_addTarget, TXT("airfighter; add target"));
DEFINE_STATIC_NAME_STR(aim_af_setup, TXT("airfighter; setup"));
DEFINE_STATIC_NAME_STR(aim_hc_instantTransitSpeed, TXT("h_craft; instant transit speed"));
DEFINE_STATIC_NAME(targetIMO);
DEFINE_STATIC_NAME(followOffset);

// params/variables
DEFINE_STATIC_NAME(inRoom);

// doc related
DEFINE_STATIC_NAME(delayObjectCreationPriority);
DEFINE_STATIC_NAME(delayObjectCreationDevSkippable);

//

REGISTER_FOR_FAST_CAST(AirPursuitManager);

AirPursuitManager::AirPursuitManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, airPursuitManagerData(fast_cast<AirPursuitManagerData>(_logicData))
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);
}

AirPursuitManager::~AirPursuitManager()
{
}

LATENT_FUNCTION(AirPursuitManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<AirPursuitManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->started = false;
	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_ap_setTarget), [self](Framework::AI::Message const& _message)
			{
				if (auto* param = _message.get_param(NAME(targetIMO)))
				{
					self->target = param->get_as<SafePtr<Framework::IModulesOwner>>();
				}
			});
	}

	// wait a bit to make sure POIs are there
	LATENT_WAIT(0.1f);

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());
	if (!self->inRoom.get())
	{
		self->inRoom = imo->get_presence()->get_in_room();
	}

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	ai_log_no_colour(self);

	self->started = true;

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

void AirPursuitManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (! started)
	{
		return;
	}

	auto* world = get_mind()->get_owner_as_modules_owner()->get_in_world();

	if (pursuits.is_empty())
	{
		pursuits.make_space_for(airPursuitManagerData->pursuits.get_size());
		for_every(ps, airPursuitManagerData->pursuits)
		{
			pursuits.grow_size(1);
			auto& p = pursuits.get_last();
			p.setup = ps;
		}
	}

	for_every(p, pursuits)
	{
		bool sendSetupAIMessages = false;

		// always one getting ready (unless just one wave total, ie. no interval)
		if (p->activeWaves.is_empty() ||
			(! p->setup->interval.is_empty() && ! p->activeWaves.get_last().getReady))
		{
			++p->wave;
			p->activeWaves.grow_size(1);
			auto& w = p->activeWaves.get_last();
			w.inLockerRoom = true;
			w.getReady = true;
			w.addDocCount = p->setup->count.get(random);
			if (!p->setup->individualWaveFollowOffsetPoint.x.is_empty())
			{
				w.individualWaveFollowOffsetPoint.x = random.get(p->setup->individualWaveFollowOffsetPoint.x);
			}
			if (!p->setup->individualWaveFollowOffsetPoint.y.is_empty())
			{
				w.individualWaveFollowOffsetPoint.y = random.get(p->setup->individualWaveFollowOffsetPoint.y);
			}
			if (!p->setup->individualWaveFollowOffsetPoint.z.is_empty())
			{
				w.individualWaveFollowOffsetPoint.z = random.get(p->setup->individualWaveFollowOffsetPoint.z);
			}
		}

		bool mayStart = false;
		bool mayReady = false;

		if (! p->permanentlyStopped && target.get())
		{
			if (auto* gd = GameDirector::get())
			{
				Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
				if (! p->setup->storyFactRequired.is_empty())
				{
					if (p->setup->storyFactRequired.check(gd->access_story_execution().get_facts()))
					{
						mayStart = true;
					}
				}
				else
				{
					mayStart = false;
				}

				if (mayStart)
				{
					mayReady = true;
				}
				else
				{
					if (!p->setup->storyFactReady.is_empty())
					{
						if (p->setup->storyFactReady.check(gd->access_story_execution().get_facts()))
						{
							mayReady = true;
						}
					}
					else
					{
						mayReady = false;
					}
				}
			}
		}

		bool newWave = false;
		if (mayStart)
		{
			if (p->wave == NONE)
			{
				newWave = true;
			}
			p->intervalLeft = max(0.0f, p->intervalLeft - _deltaTime);
			if (p->intervalLeft <= 0.0f && !p->setup->interval.is_empty())
			{
				newWave = true;
			}
		}

		if (newWave)
		{
			auto& w = p->activeWaves.get_last();
			if (w.getReady &&
				(w.docs.is_empty() && w.addDocCount == 0))
			{
				w.getReady = false;
				w.aiMessagesPending = true;

				for_every(pursuit, w.pursuiters)
				{
					pursuit->sendAIMessageDelay = random.get_float(0.0f, 0.6f);
				}

				Transform targetPlacement = look_matrix(target->get_presence()->get_placement().get_translation(), target->get_presence()->get_velocity_linear().normal_2d(), Vector3::zAxis).to_transform();
				{
					ArrayStatic<int, 32> spawnPointIdx;
					{
						for_count(int, i, p->setup->spawnPointInfos.get_size())
						{
							if (!spawnPointIdx.has_place_left())
							{
								spawnPointIdx.remove_fast_at(random.get_int(spawnPointIdx.get_size()));
								break;
							}
							spawnPointIdx.push_back(i);
						}
					}
					ArrayStatic<int, 32> followOffsetPointIdx;
					{
						for_count(int, i, p->setup->followOffsetPoints.get_size())
						{
							if (!followOffsetPointIdx.has_place_left())
							{
								followOffsetPointIdx.remove_fast_at(random.get_int(followOffsetPointIdx.get_size()));
								break;
							}
							followOffsetPointIdx.push_back(i);
						}
					}
					for_every(pursuiter, w.pursuiters)
					{
						auto* imo = pursuiter->imo.get();

						if (!imo)
						{
							continue;
						}

						if (spawnPointIdx.is_empty())
						{
							break;
						}
						int spIdx;
						{
							int i = random.get_int(spawnPointIdx.get_size());
							spIdx = spawnPointIdx[i];
							spawnPointIdx.remove_fast_at(i);
						}
						int fopIdx = NONE;
						if (!followOffsetPointIdx.is_empty())
						{
							int i = random.get_int(followOffsetPointIdx.get_size());
							fopIdx = followOffsetPointIdx[i];
							followOffsetPointIdx.remove_fast_at(i);
						}

						// teleport into the right place
						auto& sp = p->setup->spawnPointInfos[spIdx];
						if (fopIdx != NONE)
						{
							pursuiter->followOffsetPoint = p->setup->followOffsetPoints[fopIdx];
						}
					
						{
							Transform placement = targetPlacement.to_world(sp.placementRel);
							Framework::ModulePresence::TeleportRequestParams tp;
							if (sp.speed.is_set())
							{
								tp.with_velocity(placement.get_axis(Axis::Forward) * sp.speed.get());
							}
							imo->get_presence()->request_teleport(inRoom.get(), placement, tp);
						}
					}
					w.inLockerRoom = false;
				}

				++p->wave;
				if (!p->setup->interval.is_empty())
				{
					p->intervalLeft = random.get(p->setup->interval);
				}
			}
		}

		Optional<int> removeWaveIndex;
		for_every(w, p->activeWaves)
		{
			if (w->getReady)
			{
				if (mayReady && w->addDocCount)
				{
					--w->addDocCount;
					if (auto* game = Game::get_as<Game>())
					{
						auto* intoRoom = game->get_locker_room();

						RefCountObjectPtr<Framework::DelayedObjectCreation> doc(new Framework::DelayedObjectCreation());
						if (intoRoom)
						{
							doc->inSubWorld = intoRoom->get_in_sub_world();
						}
						doc->imoOwner = get_mind()->get_owner_as_modules_owner();
						doc->inRoom = intoRoom;
						doc->objectType = p->setup->actorType.get();
						doc->tagSpawned = p->setup->tagSpawned;
						doc->placement = Transform::identity;
						doc->randomGenerator = random.spawn();
						doc->activateImmediatelyEvenIfRoomVisible = true; // that's the point!
						doc->processPOIsImmediately = true;
						doc->checkSpaceBlockers = false;
						// skip collecting variables
						doc->variables.set_from(p->setup->spawnParameters);
						doc->priority = p->setup->spawnParameters.get_value<int>(NAME(delayObjectCreationPriority), doc->priority);
						doc->devSkippable = p->setup->spawnParameters.get_value<bool>(NAME(delayObjectCreationDevSkippable), doc->devSkippable);

						game->queue_delayed_object_creation(doc.get());

						w->docs.push_back(doc);
					}
				}
				for_every(docRef, w->docs)
				{
					if (auto* doc = docRef->get())
					{
						if (doc->is_done())
						{
							w->pursuiters.grow_size(1);
							auto& pursuiter = w->pursuiters.get_last();
							pursuiter.imo = doc->createdObject.get();
							docRef->clear();
						}
					}
				}
			}
			while (!w->docs.is_empty() && !w->docs.get_first().is_set())
			{
				w->docs.pop_front();
			}
			if (w->aiMessagesPending && _deltaTime > 0.0f)
			{
				bool sendAIMessagesNow = true;

				/*
				{
					int inOurRoom = 0;
					for_every(pursuit, w->pursuiters)
					{
						if (auto* imo = pursuit->imo.get())
						{
							if (imo->get_presence()->get_in_room() == inRoom.get())
							{
								++inOurRoom;
							}
						}
					}
					if (inOurRoom && inOurRoom == w->pursuiters.get_size())
					{
						sendAIMessagesNow = true;
					}
				}
				*/

				if (sendAIMessagesNow)
				{
					w->aiMessagesPending = false;
					sendSetupAIMessages = true;

					for_every(pursuit, w->pursuiters)
					{
						// don't sync the messages
						bool sendThisOne = false;
						if (pursuit->sendAIMessageDelay >= 0.0f && _deltaTime > 0.0f)
						{
							pursuit->sendAIMessageDelay -= _deltaTime;
							if (pursuit->sendAIMessageDelay < 0.0f)
							{
								sendThisOne = true;
							}
							else
							{
								w->aiMessagesPending = true;
							}
						}
						if (sendThisOne)
						{
							if (auto* imo = pursuit->imo.get())
							{
								// send messages about target etc
								if (!p->setup->instantTransitSpeedAIMessageParams.is_empty())
								{
									if (auto* aim = world->create_ai_message(NAME(aim_hc_instantTransitSpeed)))
									{
										aim->to_ai_object(imo);
										aim->set_params_from(p->setup->instantTransitSpeedAIMessageParams);
									}
								}
								if (!p->setup->addTargetAIMessageParams.is_empty())
								{
									if (auto* aim = world->create_ai_message(NAME(aim_af_addTarget)))
									{
										aim->to_ai_object(imo);
										aim->set_params_from(p->setup->addTargetAIMessageParams);
										aim->access_param(NAME(targetIMO)).access_as<SafePtr<Framework::IModulesOwner>>() = target;
									}
								}
							}
						}
					}
				}
			}
			if (!w->inLockerRoom)
			{
				bool anyAlive = false;
				Optional<int> removeDeadIdx;
				for_every(pursuiter, w->pursuiters)
				{
					if (auto* imo = pursuiter->imo.get())
					{
						anyAlive = true;
						break;
					}
					else
					{
						removeDeadIdx.set_if_not_set(for_everys_index(pursuiter));
					}
				}
				if (!anyAlive)
				{
					sendSetupAIMessages = true;
					removeWaveIndex.set_if_not_set(for_everys_index(w));
				}
				else
				{
					if (removeDeadIdx.is_set())
					{
						w->pursuiters.remove_at(removeDeadIdx.get());
					}
				}
			}
		}
		if (removeWaveIndex.is_set())
		{
			p->activeWaves.remove_at(removeWaveIndex.get());
		}

		if (sendSetupAIMessages)
		{
			Vector3 concurrentWaveFollowOffsetPoint = Vector3::zero;
			float iwMultiplier = 1.0f;
			for_every(w, p->activeWaves)
			{
				if (w->inLockerRoom || w->getReady || w->aiMessagesPending)
				{
					continue;
				}
				Vector3 waveOffset = w->individualWaveFollowOffsetPoint * iwMultiplier;
				for_every(pursuiter, w->pursuiters)
				{
					auto* imo = pursuiter->imo.get();
					if (!imo)
					{
						continue;
					}
					if (!p->setup->setupAIMessageParams.is_empty())
					{
						if (auto* aim = world->create_ai_message(NAME(aim_af_setup)))
						{
							aim->to_ai_object(imo);
							aim->set_params_from(p->setup->setupAIMessageParams);
							aim->access_param(NAME(followOffset)).access_as<Vector3>() +=
								  pursuiter->followOffsetPoint.get(Vector3::zero)
								+ concurrentWaveFollowOffsetPoint
								+ waveOffset;
						}
					}
				}
				concurrentWaveFollowOffsetPoint += p->setup->concurrentWaveFollowOffset;
				iwMultiplier *= p->setup->individualWaveFollowOffsetPointWaveMultiplier;
			}
		}
	}

}

void AirPursuitManager::log_logic_info(LogInfoContext& _log) const
{
	if (!started)
	{
		_log.set_colour(Colour::red);
		_log.log(TXT("NOT YET STARTED"));
		_log.set_colour();
	}
	if (auto* imo = target.get())
	{
		_log.log(TXT("target: %S"), imo->ai_get_name().to_char());
	}
	else
	{
		_log.set_colour(Colour::red);
		_log.log(TXT("NO TARGET"));
		_log.set_colour();
	}
	_log.log(TXT("pursuits: %i"), pursuits.get_size());
	{
		LOG_INDENT(_log);
		for_every(p, pursuits)
		{
			_log.log(TXT("pursuit %i"), for_everys_index(p));
			LOG_INDENT(_log);
			if (p->permanentlyStopped)
			{
				_log.set_colour(Colour::yellow);
				_log.log(TXT("PERMANENTLY STOPPED"));
				_log.set_colour();
			}
			_log.log(TXT("wave #%i"), p->wave);
			_log.log(TXT("intervalLeft %.3f"), p->intervalLeft);
			for_every(w, p->activeWaves)
			{
				_log.log(TXT("+- wave %i"), for_everys_index(w));
				LOG_INDENT(_log);
				if (w->getReady)
				{
					_log.set_colour(Colour::blue);
					_log.log(TXT("to get ready"));
				}
				else if (w->inLockerRoom)
				{
					_log.set_colour(Colour::cyan);
					_log.log(TXT("in locker room"));
				}
				else if (w->aiMessagesPending)
				{
					_log.set_colour(Colour::yellow);
					_log.log(TXT("ai messages pending"));
				}
				else
				{
					_log.set_colour(Colour::orange);
					_log.log(TXT("in pursuit"));
				}
				_log.set_colour();
				_log.log(TXT("pursuiters %i"), w->pursuiters.get_size());
				_log.log(TXT("docs %i+%i"), w->docs.get_size(), w->addDocCount);
			}
		}
	}
}

void AirPursuitManager::debug_draw(Framework::Room* _room) const
{
}

//

REGISTER_FOR_FAST_CAST(AirPursuitManagerData);

AirPursuitManagerData::AirPursuitManagerData()
{
}

AirPursuitManagerData::~AirPursuitManagerData()
{
}

bool AirPursuitManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("pursuit")))
	{
		pursuits.push_back(PursuitSetup());
		auto& ps = pursuits.get_last();
		
		ps.storyFactRequired.load_from_xml_attribute_or_child_node(node, TXT("storyFactRequired"));
		ps.storyFactReady.load_from_xml_attribute_or_child_node(node, TXT("storyFactReady"));
		ps.storyFactToStopPermanently.load_from_xml_attribute_or_child_node(node, TXT("storyFactToStopPermanently"));

		ps.instantTransitSpeedAIMessageParams.load_from_xml_child_node(node, TXT("instantTransitSpeedAIMessageParams"));
		ps.addTargetAIMessageParams.load_from_xml_child_node(node, TXT("addTargetAIMessageParams"));
		ps.setupAIMessageParams.load_from_xml_child_node(node, TXT("setupAIMessageParams"));

		for_every(n, node->children_named(TXT("followOffsetPoint")))
		{
			Vector3 fp = Vector3(0.0f, -10.0f, 0.0f);
			fp.load_from_xml(n);
			ps.followOffsetPoints.push_back(fp);
		}
		ps.concurrentWaveFollowOffset.load_from_xml_child_node(node, TXT("concurrentWaveFollowOffset"));
		ps.individualWaveFollowOffsetPoint.load_from_xml_child_node(node, TXT("individualWaveFollowOffsetPoint"));
		for_every(n, node->children_named(TXT("individualWaveFollowOffsetPoint")))
		{
			ps.individualWaveFollowOffsetPointWaveMultiplier = n->get_float_attribute(TXT("waveMultiplier"), ps.individualWaveFollowOffsetPointWaveMultiplier);
		}

		for_every(n, node->children_named(TXT("spawnPointInfo")))
		{
			PursuitSetup::SpawnPointInfo spi;
			spi.placementRel.load_from_xml_child_node(n, TXT("placementRel"));
			spi.speed.load_from_xml(n, TXT("speed"));
			ps.spawnPointInfos.push_back(spi);
		}

		ps.count.load_from_xml(node, TXT("count"));
		ps.interval.load_from_xml(node, TXT("interval"));

		ps.actorType.load_from_xml(node, TXT("actorType"), _lc);
		ps.tagSpawned.load_from_xml_attribute_or_child_node(node, TXT("tagSpawned"));
		ps.spawnParameters.load_from_xml_child_node(node, TXT("spawnParameters"));
	}

	return result;
}

bool AirPursuitManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(p, pursuits)
	{
		result &= p->actorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}
