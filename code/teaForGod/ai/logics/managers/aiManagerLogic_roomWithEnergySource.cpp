#include "aiManagerLogic_roomWithEnergySource.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\library\library.h"
#include "..\..\..\modules\moduleAI.h"
#include "..\..\..\modules\moduleMovementAirshipProxy.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\object\scenery.h"
#include "..\..\..\..\framework\world\room.h"

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

// parameters
DEFINE_STATIC_NAME(inRoom);

// state variables
DEFINE_STATIC_NAME(energyType);
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(energyInStorage);
DEFINE_STATIC_NAME(maxStorage);
DEFINE_STATIC_NAME(transferSpeed);

// game script traps
DEFINE_STATIC_NAME_STR(gstHealthCharging, TXT("room with energy source; health; charging"));
DEFINE_STATIC_NAME_STR(gstHealthDone, TXT("room with energy source; health; done"));
DEFINE_STATIC_NAME_STR(gstAmmoCharging, TXT("room with energy source; ammo; charging"));
DEFINE_STATIC_NAME_STR(gstAmmoDone, TXT("room with energy source; ammo; done"));

// library global referencer
DEFINE_STATIC_NAME_STR(gr_gameScriptExecutor, TXT("game script executor"));

//

REGISTER_FOR_FAST_CAST(RoomWithEnergySource);

RoomWithEnergySource::RoomWithEnergySource(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const* _logicData)
: base(_mind, _logicData, execute_logic)
{
	context = new Context();
	context->roomWithEnergySourceData = fast_cast<RoomWithEnergySourceData>(_logicData);
}

RoomWithEnergySource::~RoomWithEnergySource()
{
}

LATENT_FUNCTION(RoomWithEnergySource::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(Optional<DirFourClockwise::Type>, towardsObjective);
	LATENT_VAR(Energy, energyInStorage);
	LATENT_VAR(Energy, maxStorage);
	LATENT_VAR(bool, keepWaiting);
	LATENT_VAR(EnergyType::Type, energyType);
	LATENT_VAR(Optional<Energy>, previousEnergyLevel);
	LATENT_VAR(::System::TimeStamp, chargingGSTrapTS);

	auto * self = fast_cast<RoomWithEnergySource>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	// wait a bit to make sure POIs are there
	LATENT_WAIT(1.0f);

	// get information where it does work
	self->context->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());
	if (!self->context->inRoom.get())
	{
		error(TXT("no valid \"inRoom\" provided"));
	}
	self->context->pilgrimageDeviceTagged.clear();

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		self->context->mapCell = piow->find_cell_at(self->context->inRoom.get()).get(VectorInt2::zero);

		for_every(fc, self->context->roomWithEnergySourceData->forCells)
		{
			if (fc->at == self->context->mapCell)
			{
				self->context->pilgrimageDeviceTagged = fc->pilgrimageDeviceTagged;
				self->context->runGameScript = fc->runGameScript;
			}
		}
	}

	if (! self->context->pilgrimageDeviceTagged.is_empty())
	{
		while (!towardsObjective.is_set())
		{
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				towardsObjective = piow->query_towards_objective_at(self->context->mapCell);
			}
			LATENT_WAIT(Random::get_float(0.2f, 0.6f));
		}

		if (!self->context->door.is_set())
		{
			//FOR_EVERY_ALL_DOOR_IN_ROOM(d, self->context->inRoom.get())
			for_every_ptr(d, self->context->inRoom->get_doors())
			{
				if (!d->is_visible()) continue;
				if (PilgrimageInstanceOpenWorld::does_door_lead_in_dir(d, towardsObjective.get()))
				{
					self->context->door = d->get_door();
				}
			}
		}

		if (self->context->door.get())
		{
			if (Framework::DoorType* dt = self->context->roomWithEnergySourceData->doorType.get())
			{
				SafePtr<Framework::Door> d = self->context->door;
				Game::get()->add_async_world_job_top_priority(TXT("force door type"), [d, dt]()
					{
						if (d.get() && dt)
						{
							d->force_door_type(dt);
							d->set_operation(Framework::DoorOperation::StayClosed);
						}
					});
			}
		}

		if (!self->context->pilgrimageDevice.is_set())
		{
			RefCountObjectPtr<Context> context = self->context;
			Game::get()->add_async_world_job_top_priority(TXT("find pilgrimage device"), [context]()
				{
					if (auto* lib = Library::get_current_as<Library>())
					{
						for_every_ptr(ob, lib->get_pilgrimage_devices().get_tagged(context->pilgrimageDeviceTagged))
						{
							context->pilgrimageDevice = ob;
							break;
						}
					}
				});
		}

		while (! self->context->pilgrimageDevice.is_set())
		{
			LATENT_WAIT(Random::get_float(0.1f, 0.5f));
		}

		if (!self->context->pilgrimageDeviceObject.is_set())
		{
			RefCountObjectPtr<Context> context = self->context;
			Game::get()->add_async_world_job_top_priority(TXT("spawn pilgrimage device"), [context]()
				{
					if (Game::get()->does_want_to_cancel_creation())
					{
						return;
					}

					Random::Generator rg = context->inRoom->get_individual_random_generator();
					rg.advance_seed(25080, 9264);

					Array<RefCountObjectPtr<Framework::PointOfInterestInstance>> pois; // use ref count as we might be dealing with temporary instances (see ModuleAppearance::for_every_point_of_interest)

					// find proper POIs
					pois.clear();
					{
						for_every(pdiPOI, context->pilgrimageDevice->get_poi_definitions())
						{
							context->inRoom->for_every_point_of_interest(pdiPOI->name,
								[pdiPOI, &pois](Framework::PointOfInterestInstance* _fpoi)
								{
									if (pdiPOI->tagged->is_empty() || pdiPOI->tagged->check(_fpoi->get_tags()))
									{
										pois.push_back_unique(RefCountObjectPtr<Framework::PointOfInterestInstance>(_fpoi));
									}
								});
						}
					}

					if (pois.is_empty())
					{
						error(TXT("no pois to place pilgrimage device \"%S\", maybe you should disallow it here or provide larger rooms?"), context->pilgrimageDevice->get_name().to_string().to_char());
					}

					Framework::ScopedAutoActivationGroup saag(context->inRoom.get());

					Framework::Object* spawned = nullptr;

					// spawn device
					{
						context->pilgrimageDevice->get_scenery_type()->load_on_demand_if_required();

						Game::get()->perform_sync_world_job(TXT("spawn via spawn manager"),
							[context, &spawned]()
							{
								spawned = context->pilgrimageDevice->get_scenery_type()->create(String::printf(TXT("%S [room with enegry source's pilgrimage device for %ix%i]"),
									context->pilgrimageDevice->get_scenery_type()->create_instance_name().to_char(), context->mapCell.x, context->mapCell.y));
								spawned->init(context->inRoom->get_in_sub_world());
							});

						{
							context->inRoom->collect_variables(spawned->access_variables());
							spawned->access_variables().set_from(context->pilgrimageDevice->get_device_setup_variables());
							spawned->access_individual_random_generator() = rg.spawn();

							spawned->access_variables().access<Energy>(NAME(energy)) = Energy(magic_number hardcoded 2000.0f);
							spawned->access_variables().access<Energy>(NAME(maxStorage)) = Energy(magic_number hardcoded 2000.0f);
							spawned->access_variables().access<Energy>(NAME(transferSpeed)) = Energy(magic_number hardcoded 40.0f);

							spawned->initialise_modules();
						}
					}

					// try to place for a specific number of tries (if fails, fail with just an information, not even a warning)
					{
						bool isOk = false;
						int triesLeft = 50;
						while (!isOk && triesLeft > 0 && !pois.is_empty())
						{
							--triesLeft;

							RefCountObjectPtr<Framework::PointOfInterestInstance> poi;
							{
								int poiIdx = rg.get_int(pois.get_size());
								poi = pois[poiIdx];
								// don't remove
							}

							Transform placeAt = poi->calculate_placement();
							Framework::Room* room = poi->get_room();

							{
								Optional<Transform> goodPlacement = poi->find_placement_for(spawned, room, rg.spawn());

								isOk = goodPlacement.is_set();
								placeAt = goodPlacement.get(placeAt);
							}

							if (isOk)
							{
								Game::get()->perform_sync_world_job(TXT("place spawned"),
									[spawned, room, placeAt]()
									{
										spawned->get_presence()->place_in_room(room, placeAt);
									});

								if (auto* piow = PilgrimageInstanceOpenWorld::get())
								{
									piow->force_add_open_world_destination_at(room, context->pilgrimageDevice.get());
									piow->tag_open_world_directions_for_cell(room);
								}
							}
						}

						if (isOk)
						{
							context->pilgrimageDeviceObject = spawned;
						}
						else
						{
							error(TXT("couldn't place pilgrimage device \"%S\", maybe you should disallow it here or provide larger rooms?"), spawned->get_name().to_char());
							spawned->destroy();
							spawned = nullptr;
						}
					}
				});
		}

		while (!self->context->pilgrimageDeviceObject.is_set())
		{
			LATENT_WAIT(Random::get_float(0.1f, 0.5f));
		}

		if (auto* runGameScript = self->context->runGameScript.get())
		{
			RefCountObjectPtr<Context> context = self->context;
			SafePtr<Framework::IModulesOwner> safeImo(imo);
			Game::get()->add_async_world_job_top_priority(TXT("spawn run script executor"), [context, runGameScript, safeImo]()
				{
					if (auto* rgsSceneryType = Framework::Library::get_current()->get_global_references().get<Framework::SceneryType>(NAME(gr_gameScriptExecutor), false))
					{
						Framework::ScopedAutoActivationGroup saag(context->inRoom.get());

						Framework::Scenery* rgsScenery = nullptr;

						rgsSceneryType->load_on_demand_if_required();

						// subworld is same for both region and room
						auto* room = context->inRoom.get();
						Game::get()->perform_sync_world_job(TXT("spawn run game script executor"), [&rgsScenery, rgsSceneryType, room, runGameScript]()
							{
								rgsScenery = new Framework::Scenery(rgsSceneryType, runGameScript->get_name().to_string());
								rgsScenery->init(room->get_in_sub_world());
							});

						//rgsScenery->access_variables().set_from(parameters);
						if (auto* imo = safeImo.get())
						{
							rgsScenery->access_individual_random_generator() = imo->get_individual_random_generator();
						}

						rgsScenery->initialise_modules();

						if (auto* ai = fast_cast<ModuleAI>(rgsScenery->get_ai()))
						{
							ai->start_game_script(runGameScript);
						}

						// use room, we need to be placed in any room
						Game::get()->perform_sync_world_job(TXT("place game script executor"), [&rgsScenery, room]()
							{
								Vector3 const placementLoc = Vector3::zAxis * 100.0f;
								Vector3 const placementOffset = Vector3::xAxis;

#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
								output(TXT("add game script executor o%p to room r%p"), rgsScenery, room);
#endif
								rgsScenery->get_presence()->place_in_room(room, Transform(placementLoc + placementOffset, Quat::identity)); // place it in separate location
							});

						Game::get()->on_newly_created_object(rgsScenery);

						rgsScenery->access_variables().access<SafePtr<Framework::Room>>(NAME(inRoom)) = room;
					}
				});
		}

		{
			energyType = EnergyType::None;
			String et = self->context->pilgrimageDeviceObject->get_variables().get_value<String>(NAME(energyType), String::empty());
			if (!et.is_empty())
			{
				energyType = EnergyType::parse(et, EnergyType::None);
			}
		}

		energyInStorage = Energy::zero();
		maxStorage = Energy::zero();
		keepWaiting = true;

#ifdef AN_DEVELOPMENT
		maxStorage = Energy(1000); // this is to open instantly
#endif

		while (maxStorage.is_zero() || keepWaiting)
		{
			energyInStorage = self->context->pilgrimageDeviceObject->get_variables().get_value<Energy>(NAME(energyInStorage), energyInStorage);
			maxStorage = self->context->pilgrimageDeviceObject->get_variables().get_value<Energy>(NAME(maxStorage), maxStorage);

			LATENT_WAIT(Random::get_float(0.1f, 0.5f));

			if (energyInStorage.is_zero())
			{
				keepWaiting = false;
			}
			else if (auto* g = Game::get_as<Game>())
			{
				Optional<Energy> newEnergyLevel;
				todo_multiplayer_issue(TXT("we just get player here"));
				if (auto* pa = g->access_player().get_actor())
				{
					if (pa->get_presence()->get_in_room() == imo->get_presence()->get_in_room())
					{
						if (energyType == EnergyType::Ammo)
						{
							if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
							{
								newEnergyLevel = mp->get_hand_energy_storage(Hand::Left) + mp->get_hand_energy_storage(Hand::Right);
								if (mp->get_hand_energy_storage(Hand::Left) >= mp->get_hand_energy_max_storage(Hand::Left) - Energy::one() &&
									mp->get_hand_energy_storage(Hand::Right) >= mp->get_hand_energy_max_storage(Hand::Right) - Energy::one())
								{
									keepWaiting = false;
								}
							}
						}
						if (energyType == EnergyType::Health)
						{
							if (auto* mp = pa->get_custom<CustomModules::Health>())
							{
								newEnergyLevel = mp->get_total_health();
								if (mp->get_total_health() >= mp->get_max_total_health() - Energy::one())
								{
									keepWaiting = false;
								}
							}
						}
						if (previousEnergyLevel.is_set() && newEnergyLevel.is_set() &&
							previousEnergyLevel.get() < newEnergyLevel.get())
						{
							if (chargingGSTrapTS.get_time_since() > 1.0f)
							{
								Framework::GameScript::ScriptExecution::trigger_execution_trap(energyType == EnergyType::Health? NAME(gstHealthCharging) : NAME(gstAmmoCharging));
							}
							chargingGSTrapTS.reset();
						}
						else
						{
							chargingGSTrapTS.reset(-50.0f);
						}
					}
				}
				previousEnergyLevel = newEnergyLevel;
			}
		}

		{
			todo_hack(TXT("it should be used only here, so let's just allow it to be implemented this way"));
			if (energyType == EnergyType::Ammo)
			{
				GameDirector::get()->set_infinite_ammo_blocked(false);
			}
			if (energyType == EnergyType::Health)
			{
				GameDirector::get()->set_immortal_health_regen_blocked(false);
			}
		}
		if (auto* d = self->context->door.get())
		{
			d->set_operation(Framework::DoorOperation::StayOpen, Framework::Door::DoorOperationParams().allow_auto(false)); // don't allow auto, stay open
		}

		Framework::GameScript::ScriptExecution::trigger_execution_trap(energyType == EnergyType::Health ? NAME(gstHealthDone) : NAME(gstAmmoDone));
	}

	while (true)
	{
		LATENT_WAIT(10.0f); // wait till it's all over
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void RoomWithEnergySource::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

//

REGISTER_FOR_FAST_CAST(RoomWithEnergySourceData);

RoomWithEnergySourceData::RoomWithEnergySourceData()
{
}

RoomWithEnergySourceData::~RoomWithEnergySourceData()
{
}

bool RoomWithEnergySourceData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	doorType.load_from_xml(_node, TXT("doorType"), _lc);

	for_every(n, _node->children_named(TXT("cell")))
	{
		ForCell fc;
		fc.at.load_from_xml(n);
		fc.pilgrimageDeviceTagged.load_from_xml_attribute(n, TXT("pilgrimageDeviceTagged"));
		fc.runGameScript.load_from_xml(n, TXT("runGameScript"), _lc);
		forCells.push_back(fc);
	}

	return result;
}

bool RoomWithEnergySourceData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= doorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	for_every(c, forCells)
	{
		result &= c->runGameScript.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	return result;
}
