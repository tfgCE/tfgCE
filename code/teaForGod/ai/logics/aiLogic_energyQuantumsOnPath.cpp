#include "aiLogic_energyQuantumsOnPath.h"

#include "..\..\game\game.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"

#include "..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\custom\mc_lightningSpawner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\framework\object\itemType.h"
#include "..\..\..\framework\object\object.h"

#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"

#include "..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_start, TXT("energy quantum on path; start"));
DEFINE_STATIC_NAME_STR(aim_stop, TXT("energy quantum on path; stop"));
DEFINE_STATIC_NAME_STR(aim_disappear, TXT("energy quantum on path; disappear"));

//

REGISTER_FOR_FAST_CAST(EnergyQuantumsOnPath);

EnergyQuantumsOnPath::EnergyQuantumsOnPath(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	eqopData = fast_cast<EnergyQuantumsOnPathData>(_logicData);
}

EnergyQuantumsOnPath::~EnergyQuantumsOnPath()
{
}

void EnergyQuantumsOnPath::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(EnergyQuantumsOnPath::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Random::Generator, rg);
	LATENT_VAR(int, i);
	
	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<EnergyQuantumsOnPath>(logic);

	LATENT_BEGIN_CODE();

	self->active = self->eqopData->autoStart;
	self->disappear = false;

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_start), [self](Framework::AI::Message const& _message)
			{
				self->active = true;
				self->disappear = false;
			});
		messageHandler.set(NAME(aim_stop), [self](Framework::AI::Message const& _message)
			{
				self->active = false;
			});
		messageHandler.set(NAME(aim_disappear), [self](Framework::AI::Message const& _message)
			{
				self->active = false;
				self->disappear = true;
			});
	}

	i = 0;
	while (i >= 0)
	{
		{
			Name reactorPOI(String::printf(TXT("%S%i"), self->eqopData->alongPOIPrefix.to_char(), i));
			if (auto* r = imo->get_presence()->get_in_room())
			{
				Framework::PointOfInterestInstance* foundPOI = nullptr;
				if (r->find_any_point_of_interest(reactorPOI, OUT_ foundPOI, false, &rg))
				{
					if (foundPOI)
					{
						self->placements.push_back(foundPOI->calculate_placement());
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
		if (self->active)
		{
			self->timeLeftToSpawn -= LATENT_DELTA_TIME;
			if (self->timeLeftToSpawn <= 0.0f)
			{
				self->timeLeftToSpawn = rg.get(self->eqopData->interval);
				self->timeLeftToSpawn /= clamp((GameSettings::get().difficulty.lootAmmo + GameSettings::get().difficulty.lootHealth) * 0.5f, 0.5f, 2.0f);

				if (!self->eqopData->entries.is_empty() &&
					!self->placements.is_empty() &&
					!self->spawningDOC.get())
				{
					int idx = RandomUtils::ChooseFromContainer<Array<EnergyQuantumsOnPathData::Entry>, EnergyQuantumsOnPathData::Entry>::choose(
							rg, self->eqopData->entries, [](EnergyQuantumsOnPathData::Entry const& _e) { return _e.probCoef; });

					auto& e = self->eqopData->entries[idx];
					if (auto* sit = e.itemType.get())
					{
						auto* inRoom = imo->get_presence()->get_in_room();
						Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
						doc->activateImmediatelyEvenIfRoomVisible = true;
						doc->wmpOwnerObject = imo;
						doc->inRoom = inRoom;
						doc->name = TXT("energy quantum on path");
						doc->objectType = sit;
						doc->placement = self->placements.get_first();
						doc->randomGenerator = imo->get_individual_random_generator();
						doc->priority = 1000;
						doc->checkSpaceBlockers = false;

						inRoom->collect_variables(doc->variables);

						self->spawningDOC = doc;

						Optional<Energy> energy;
						EnergyType::Type energyType = e.energyType;
						if (e.energy.is_set())
						{
							energy = e.energy.get();
							if (energyType == EnergyType::Health)
							{
								energy = energy.get().mul(GameSettings::get().difficulty.lootHealth);
							}
							if (energyType == EnergyType::Ammo)
							{
								energy = energy.get().mul(GameSettings::get().difficulty.lootAmmo);
							}
						}
						Optional<float> timeLimit = self->eqopData->timeLimit;
						bool singlePlacement = self->placements.get_size() == 1;
						float speed = self->eqopData->speed;
						doc->post_initialise_modules_function = [energy, energyType, timeLimit, singlePlacement, speed](Framework::Object* spawnedObject)
						{
							if (auto* meq = spawnedObject->get_gameplay_as<ModuleEnergyQuantum>())
							{
								meq->start_energy_quantum_setup();
								if (energy.is_set())
								{
									meq->set_energy(energy.get());
									meq->set_energy_type(energyType);
								}
								meq->end_energy_quantum_setup();
								if (timeLimit.is_set())
								{
									meq->set_time_left(timeLimit.get());
								}
								else
								{
									meq->no_time_limit();
								}
							}
							if (singlePlacement)
							{
								Random::Generator rg;
								spawnedObject->get_presence()->set_velocity_linear(Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal()* speed);
							}
						};

						Game::get()->queue_delayed_object_creation(doc);
					}
				}
			}
		}
		if (auto* doc = self->spawningDOC.get())
		{
			if (doc->is_done())
			{
				MoveObject mo;
				mo.imo = doc->createdObject.get();
				mo.imo->set_instigator(imo);
				mo.atIdx = 0;
				self->moveObjects.push_back(mo);
				self->spawningDOC.clear();
			}
		}
		{
			for(int i = 0; i < self->moveObjects.get_size(); ++ i)
			{
				auto& mo = self->moveObjects[i];
				if (auto* eqimo = mo.imo.get())
				{
					if (self->placements.get_size() >= 2)
					{
						Vector3 loc = eqimo->get_presence()->get_placement().get_translation();
						if (mo.atIdx < self->placements.get_size())
						{
							int targetIdx = min(mo.atIdx + 1, self->placements.get_size() - 1);
							int prevIdx = max(0, targetIdx - 1);
							targetIdx = prevIdx + 1;
							Vector3 nt = self->placements[targetIdx].get_translation();
							Vector3 np = self->placements[prevIdx].get_translation();
							Vector3 dp2t = (nt - np).normal();
							float along = Vector3::dot(dp2t, loc - np);
							float length = (nt - np).length();
							Vector3 shouldBeAt = np + dp2t * along;

							Vector3 targetVelocity = lerp(0.5f, (nt - np).normal(), (shouldBeAt - loc).normal()).normal();
							targetVelocity = targetVelocity.normal() * self->eqopData->speed;

							Vector3 currentVelocity = eqimo->get_presence()->get_velocity_linear();
							Vector3 impulse = (targetVelocity - currentVelocity) * 0.3f;
							eqimo->get_presence()->clear_velocity_impulses();
							eqimo->get_presence()->add_velocity_impulse(impulse);

							if (along > length - 1.0f)
							{
								++mo.atIdx;
							}
						}
						else
						{
							if (auto* meq = mo.imo->get_gameplay_as<ModuleEnergyQuantum>())
							{
								meq->mark_to_disappear();
								mo.imo.clear(); // will be removed in the next loop pass
							}
						}
					}
					if (self->disappear && mo.imo.get())
					{
						if (auto* meq = mo.imo->get_gameplay_as<ModuleEnergyQuantum>())
						{
							meq->mark_to_disappear();
							mo.imo.clear(); // will be removed in the next loop pass
						}
					}
				}
				else
				{
					self->moveObjects.remove_at(i);
					--i;
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

//

REGISTER_FOR_FAST_CAST(EnergyQuantumsOnPathData);

bool EnergyQuantumsOnPathData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD_NAME_ATTR(_node, alongPOIPrefix);
	autoStart = _node->get_bool_attribute_or_from_child_presence(TXT("autoStart"), autoStart);
	autoStart = ! _node->get_bool_attribute_or_from_child_presence(TXT("noAutoStart"), ! autoStart);
	XML_LOAD(_node, interval);
	XML_LOAD_FLOAT_ATTR(_node, speed);
	XML_LOAD(_node, timeLimit);

	entries.clear();
	for_every(n, _node->children_named(TXT("spawn")))
	{
		Entry e;
		e.itemType.load_from_xml(n, TXT("itemType"), _lc);
		XML_LOAD_STR(n, e.energy, TXT("energy"));
		XML_LOAD_FLOAT_ATTR_STR(n, e.probCoef, TXT("probCoef"));
		{
			String et = n->get_string_attribute_or_from_child(TXT("energyType"));;
			if (!et.is_empty())
			{
				e.energyType = EnergyType::parse(et, e.energyType);
			}
		}
		entries.push_back(e);
	}

	return result;
}

bool EnergyQuantumsOnPathData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(e, entries)
	{
		result &= e->itemType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	return result;
}

//

EnergyQuantumsOnPathData::Entry::Entry() {}
EnergyQuantumsOnPathData::Entry::~Entry() {}
