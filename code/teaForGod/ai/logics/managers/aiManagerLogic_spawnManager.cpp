#include "aiManagerLogic_spawnManager.h"

#include "aiManagerLogic_regionManager.h"

#include "..\..\..\ai\managerDatas\aiManagerData_spawnSet.h"
#include "..\..\..\game\energy.h"
#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\modules\custom\mc_carrier.h"
#include "..\..\..\pilgrimage\pilgrimageInstanceLinear.h"
#include "..\..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"
#include "..\..\..\roomGenerators\roomGeneratorBase.h"
#include "..\..\..\regionGenerators\regionGeneratorBase.h"
#include "..\..\..\roomGenerators\roomGenerationInfo.h"

#include "..\..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\..\core\random\randomUtils.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\region.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\roomRegionVariables.inl"
#include "..\..\..\..\framework\world\roomUtils.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		#define LOG_MORE
		//#define AN_OUTPUT_SPAWN
	#endif
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// ai hunch names
DEFINE_STATIC_NAME(investigate);
DEFINE_STATIC_NAME(wander);

// ai hunch reasons
DEFINE_STATIC_NAME(spawnManager);

// ai hunch parameters
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(room);

// parameters
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRegion);

// poi parameters
DEFINE_STATIC_NAME(withinMeshNodeBoundary); // name
DEFINE_STATIC_NAME(notIfInDoor); // bool
DEFINE_STATIC_NAME(notIfBlocksDoor); // bool
DEFINE_STATIC_NAME(atBoundaryEdge); // bool

// spawned parameters
DEFINE_STATIC_NAME(placementRadius); // float
DEFINE_STATIC_NAME(storeExistenceInPilgrimage);
DEFINE_STATIC_NAME(storeExistenceInPilgrimageWorldAddress);
DEFINE_STATIC_NAME(storeExistenceInPilgrimagePOIIdx);

// spawn type tags
DEFINE_STATIC_NAME(npcCharacter);
DEFINE_STATIC_NAME(hostileCharacter);
DEFINE_STATIC_NAME(hostileAtGameDirector);

// room tags
DEFINE_STATIC_NAME(inaccessible);

// object type parameters
DEFINE_STATIC_NAME(spawnReplacementEasier);

// room/region parameters
DEFINE_STATIC_NAME(gameDirectorSafeSpace);
DEFINE_STATIC_NAME(allowInvestigateHunchOnlyIfPlayerWithinRegion);
DEFINE_STATIC_NAME(spawnManager_distanceToSpawnOffset);

// sounds
DEFINE_STATIC_NAME(spawnTelegraph);

// object variables
DEFINE_STATIC_NAME(unableToAttack);
DEFINE_STATIC_NAME(noLoot);

//

GlobalSpawnManagerInfo* GlobalSpawnManagerInfo::s_globalSpawnManagerInfo = nullptr;

GlobalSpawnManagerInfo::GlobalSpawnManagerInfo()
{
}

GlobalSpawnManagerInfo::~GlobalSpawnManagerInfo()
{
}

void GlobalSpawnManagerInfo::initialise_static()
{
	an_assert(!s_globalSpawnManagerInfo);
	s_globalSpawnManagerInfo = new GlobalSpawnManagerInfo();
}

void GlobalSpawnManagerInfo::close_static()
{
	an_assert(s_globalSpawnManagerInfo);
	delete_and_clear(s_globalSpawnManagerInfo);
}

void GlobalSpawnManagerInfo::reset()
{
	if (!s_globalSpawnManagerInfo) return;
	for_every(e, s_globalSpawnManagerInfo->entries)
	{
		e->count = 0;
	}
}

int GlobalSpawnManagerInfo::get(Name const& _id)
{
	if (!s_globalSpawnManagerInfo) return 0;
	for_every(e, s_globalSpawnManagerInfo->entries)
	{
		if (e->id == _id) return e->count.get();
	}
	return 0;
}

void GlobalSpawnManagerInfo::increase(Name const& _id)
{
	if (!s_globalSpawnManagerInfo) return;
	for_every(e, s_globalSpawnManagerInfo->entries)
	{
		if (e->id == _id)
		{
			e->count.increase();
			return;
		}
	}

	Concurrency::ScopedSpinLock lock(s_globalSpawnManagerInfo->entriesLock);
	// in case it was added
	for_every(e, s_globalSpawnManagerInfo->entries)
	{
		if (e->id == _id)
		{
			e->count.increase();
			return;
		}
	}
	s_globalSpawnManagerInfo->entries.grow_size(1);
	auto& e = s_globalSpawnManagerInfo->entries.get_last();
	e.id = _id;
	e.count.increase();
}

void GlobalSpawnManagerInfo::decrease(Name const& _id)
{
	if (!s_globalSpawnManagerInfo) return;
	for_every(e, s_globalSpawnManagerInfo->entries)
	{
		if (e->id == _id)
		{
			e->count.decrease();
			return;
		}
	}
	an_assert(false, TXT("we shouldn't get here, we shouldn't decrease without increasing first"));
}

//

Concurrency::Counter totalSpawnCount = 0;

int apply_spawn_count_modifier(RegionManager const * _regionManager, Name const & _pool, int _number)
{
	return max(_number > 0? 1 : 0,
		TypeConversions::Normal::f_i_closest((float)_number
			* GameSettings::get().difficulty.spawnCountModifier
			* (_regionManager? _regionManager->get_concurrent_limit_coef(_pool) : 1.0f)));
}

//

Framework::ObjectType* get_object_type(Framework::Library* _library, Framework::LibraryName const & _name)
{
	Framework::ObjectType* objectType = nullptr;
	if (!objectType) objectType = _library->get_actor_types().find_may_fail(_name);
	if (!objectType) objectType = _library->get_scenery_types().find_may_fail(_name);
	if (!objectType) objectType = _library->get_item_types().find_may_fail(_name);
	if (!objectType)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!Framework::Library::may_ignore_missing_references())
#endif
		error(TXT("could not find \"%S\" via spawn manager"), _name.to_string().to_char());
	}
	return objectType;
}

bool has_anything_to_spawn(Name const& spawnSet, Array<Framework::UsedLibraryStored<Framework::ObjectType>> const& types, SpawnManager* _spawnManager)
{
	if (AI::ManagerDatasLib::SpawnSet::has_anything(spawnSet, _spawnManager->get_for_region(), _spawnManager->get_for_room()))
	{
		return true;
	}
	else if (!types.is_empty())
	{
		return true;
	}
	return false;
}

Framework::ObjectType* find_type_to_spawn(Name const& spawnSet, Array<Framework::UsedLibraryStored<Framework::ObjectType>> const& types, SpawnManager* _spawnManager,
										  OUT_ int& _count, OUT_ Vector3& _spawnOffset, std::function<int(Framework::ObjectType* _ofType, int _any)> _how_many_already_spawned)
{
	Framework::ObjectType* chosenOT = nullptr;
	if (!chosenOT && spawnSet.is_valid())
	{
		AI::ManagerDatasLib::SpawnSetChosenExtraInfo extraInfo;
		if (auto* ot = AI::ManagerDatasLib::SpawnSet::choose(spawnSet, _spawnManager->get_for_region(), _spawnManager->get_for_room(), _spawnManager->access_random(), &extraInfo, _how_many_already_spawned))
		{
			chosenOT = ot;
			_count = extraInfo.spawnBatch;
			_spawnOffset = extraInfo.spawnBatchOffset;
		}
	}
	if (! chosenOT && !types.is_empty())
	{
		int idx = RandomUtils::ChooseFromContainer<Array<Framework::UsedLibraryStored<Framework::ObjectType>>, Framework::UsedLibraryStored<Framework::ObjectType>>::choose(_spawnManager->access_random(), types,
			[](Framework::UsedLibraryStored<Framework::ObjectType> const& _objectType) { return Framework::LibraryStored::get_prob_coef_from_tags(_objectType.get()); });
		if (auto* ot = types[idx].get())
		{
			chosenOT = ot;
		}
	}
	if (chosenOT)
	{
		if (auto* pi = PilgrimageInstanceLinear::get())
		{
			todo_hack(TXT("this is only a temporary solution to not have harder enemies in the first level"));
			if (pi->is_linear() && pi->linear__get_part_idx() < 1)
			{
				Framework::ObjectType* newOT = SpawnUtils::get_replacement_easier(chosenOT);
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (newOT != chosenOT)
				{
					output(TXT("replaced spawning of \"%S\" with \"%S\""), chosenOT->get_name().to_string().to_char(), newOT->get_name().to_string().to_char());
				}
#endif
				chosenOT = newOT;
			}
		}
		return chosenOT;
	}
	return nullptr;
}

//

void give_hunch_to_player(Framework::IModulesOwner* _imo, Random::Generator & rg, OnSpawnHunches const & _hunches, Framework::Region* _spawnManagerRegion, Framework::Room* _spawnManagerRoom)
{
	if (auto* ai = _imo->get_ai())
	{
		if (rg.get_chance(_hunches.investigateChance.get(1.0f)))
		{
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (auto* pap = pa->get_presence())
					{
						auto* room = pap->get_in_room();
						bool allowInvestigateHunch = true;
						if (! _hunches.forceInvestigate.get(false))
						{
							bool allowInvestigateHunchOnlyIfPlayerWithinRegion = false;
							if (_spawnManagerRegion)
							{
								allowInvestigateHunchOnlyIfPlayerWithinRegion = _spawnManagerRegion->get_value<bool>(NAME(allowInvestigateHunchOnlyIfPlayerWithinRegion), allowInvestigateHunchOnlyIfPlayerWithinRegion);
							}
							else if (_spawnManagerRoom)
							{
								allowInvestigateHunchOnlyIfPlayerWithinRegion = _spawnManagerRoom->get_value<bool>(NAME(allowInvestigateHunchOnlyIfPlayerWithinRegion), allowInvestigateHunchOnlyIfPlayerWithinRegion);
							}
							if (allowInvestigateHunchOnlyIfPlayerWithinRegion)
							{
								if (!_spawnManagerRegion || !
									room->is_in_region(_spawnManagerRegion))
								{
									allowInvestigateHunch = false;
								}
							}
						}
						if (allowInvestigateHunch)
						{
							if (auto* hunch = ai->give_hunch(NAME(investigate), NAME(spawnManager), 10.0f))
							{
								hunch->access_param(NAME(room)).access_as<SafePtr<Framework::Room>>() = room;
								hunch->access_param(NAME(location)).access_as<Vector3>() = pap->get_centre_of_presence_WS();
							}
						}
					}
				}
			}
		}
		if (rg.get_chance(_hunches.wanderChance.get(0.7f)))
		{
			ai->give_hunch(NAME(wander), NAME(spawnManager), 2.0f);
		}
	}
}

bool load_allow_inaccessible(IO::XML::Node const* _node, REF_ Optional<bool>& allowInaccessible, REF_ bool & accessibilityAcknowledged)
{
	allowInaccessible.clear();
	if (_node->get_bool_attribute_or_from_child_presence(TXT("allowInaccessible"), false))
	{
		allowInaccessible = true;
	}
	if (_node->get_attribute(TXT("allowInaccessible")))
	{
		allowInaccessible = _node->get_bool_attribute_or_from_child_presence(TXT("allowInaccessible"), false);
	}
	if (_node->get_bool_attribute_or_from_child_presence(TXT("disallowInaccessible"), false))
	{
		allowInaccessible = false;
	}
	allowInaccessible.load_from_xml(_node, TXT("allowInaccessible"));

	if (allowInaccessible.is_set())
	{
		accessibilityAcknowledged = true;
	}
	else if (_node->get_bool_attribute_or_from_child_presence(TXT("acknowledgeAccessibility")))
	{
		accessibilityAcknowledged = true;
	}

	return true;
}

//

Framework::ObjectType* SpawnUtils::get_replacement_easier(Framework::ObjectType* _for)
{
	if (auto* sre = _for->get_custom_parameters().get_existing<Framework::LibraryName>(NAME(spawnReplacementEasier)))
	{
		if (auto* found = get_object_type(Framework::Library::get_current(), *sre))
		{
			return found;
		}
	}
	return _for;
}

//

SpawnManagerPOIDefinition::SpawnManagerPOIDefinition()
: tagged(new TagCondition())
, inRoomTagged(new TagCondition())
, beyondDoorTagged(new TagCondition())
, beyondDoorInRoomTagged(new TagCondition())
{


}

bool SpawnManagerPOIDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	warn_loading_xml_on_assert(! _node->get_attribute(TXT("id")), _node, TXT("use \"name\" instead of \"id\" - deprecated"));

	name = _node->get_name_attribute(TXT("name"), name);

	error_loading_xml_on_assert(name.is_valid(), result, _node, TXT("no name provided"));

	result &= tagged->load_from_xml_attribute_or_child_node(_node, TXT("tags"));
	result &= tagged->load_from_xml_attribute_or_child_node(_node, TXT("tagged"));
	result &= inRoomTagged->load_from_xml_attribute_or_child_node(_node, TXT("inRoomTagged"));
	beyondDoorDepth = _node->get_int_attribute_or_from_child(TXT("beyondDoorDepth"), beyondDoorDepth);
	result &= beyondDoorTagged->load_from_xml_attribute_or_child_node(_node, TXT("beyondDoorTagged"));
	result &= beyondDoorInRoomTagged->load_from_xml_attribute_or_child_node(_node, TXT("beyondDoorInRoomTagged"));
	doorsAppearLockedIfNotOpen = _node->get_bool_attribute_or_from_child_presence(TXT("doorsAppearLockedIfNotOpen"), doorsAppearLockedIfNotOpen);

	return result;
}

//

REGISTER_FOR_FAST_CAST(SpawnManager);

SpawnManager::SpawnManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, spawnManagerData(fast_cast<SpawnManagerData>(_logicData))
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);
}

SpawnManager::~SpawnManager()
{
	hand_over_dynamic_spawns_to_game_director();

	for_every_ref(wave, currentWaves)
	{
		wave->clean_up();
	}

	for_every_ref(ds, dynamicSpawns)
	{
		ds->clean_up();
	}
}

void SpawnManager::hand_over_dynamic_spawns_to_game_director()
{
	// check game director for info about orphans
	if (auto* gd = GameDirector::get_active())
	{
		for_every_ref(cw, currentWaves)
		{
			for_every(s, cw->spawned)
			{
				if (auto* imo = s->spawned.get())
				{
					gd->take_care_of_orphaned(imo);
				}
			}
			cw->spawned.clear();
		}
		for_every_ref(ds, dynamicSpawns)
		{
			for_every(s, ds->spawned)
			{
				if (auto* imo = s->spawned.get())
				{
					gd->take_care_of_orphaned(imo);
				}
			}
			ds->spawned.clear();
		}
		reservedPools.clear();
	}
}

void SpawnManager::update_region_manager()
{
	MEASURE_PERFORMANCE(spawnManager_updateRegionManager);
	if (regionManager.get())
	{
		return;
	}
	if (auto* r = inRoom.get())
	{
		regionManager = RegionManager::find_for(r);
	}
	else if (auto* r = inRegion.get())
	{
		regionManager = RegionManager::find_for(r);
	}
}

LATENT_FUNCTION(SpawnManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	auto * self = fast_cast<SpawnManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->started = false;

	// wait a bit to make sure POIs are there
	LATENT_WAIT(1.0f);

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());
	self->inRegion = imo->get_variables().get_value<SafePtr<Framework::Region>>(NAME(inRegion), SafePtr<Framework::Region>());
	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		self->inOpenWorldCell = piow->find_cell_at(imo);
	}

	{	// register ai manager
		if (auto* r = self->inRegion.get())
		{
			self->register_ai_manager_in(r);
		}
		else if (auto* r = self->inRoom.get())
		{
			self->register_ai_manager_in(r);
		}
	}

#ifdef AN_USE_AI_LOG
	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	if (self->inRegion.get() &&
		self->inRegion->get_region_type())
	{
		ai_log(self, TXT("inRegion \"%S\""), self->inRegion->get_region_type()->get_name().to_string().to_char());
	}
	if (self->inRoom.get())
	{
		ai_log(self, TXT("inRoom \"%S\""), self->inRoom->get_name().to_char());
	}
	if (self->inOpenWorldCell.is_set())
	{
		ai_log(self, TXT("cell: %ix%i"), self->inOpenWorldCell.get().x, self->inOpenWorldCell.get().y);
	}

	ai_log_no_colour(self);
#endif

	// find all spawn point POIs
	self->spawnPointsAvailable.clear();
	self->spawnPointsBlocked.clear();

	if (self->spawnManagerData->waitUntilGenerated)
	{
		ai_log(self, TXT("wait until generated"));
		if (self->inRoom.get())
		{
			while (!Game::get_as<Game>()->is_generated(self->inRoom.get()))
			{
				LATENT_WAIT(self->random.get_float(0.1f, 0.3f));
			}
		}
		if (self->inRegion.get())
		{
			while (!Game::get_as<Game>()->is_generated(self->inRegion.get()))
			{
				LATENT_WAIT(self->random.get_float(0.1f, 0.3f));
			}
		}
		ai_log(self, TXT("generated, proceed"));
	}

	// get ALL POIs
	{
		ai_log(self, TXT("get all POIs"));
		{
			MEASURE_PERFORMANCE(spawnManager_execLogic_getAllPOIs);
			for_every(poi, self->spawnManagerData->spawnPOIs)
			{
				ai_log(self, TXT("(from spawn manager data)"));
				self->add_available_spawn_points(*poi);
			}
			for_every_ref(wave, self->spawnManagerData->waves)
			{
				for_every(poi, wave->spawnPOIs)
				{
					ai_log(self, TXT("(from wave)"));
					self->add_available_spawn_points(*poi);
				}
				for_every_ref(element, wave->elements)
				{
					for_every(poi, element->spawnPOIs)
					{
						ai_log(self, TXT("(from wave element)"));
						self->add_available_spawn_points(*poi);
					}
				}
			}
		}
		ai_log(self, TXT("available POIs: %i"), self->spawnPointsAvailable.get_size());
	}

	self->update_region_manager();

	// start wave/waves
	if (!self->spawnManagerData->waves.is_empty())
	{
		MEASURE_PERFORMANCE(spawnManager_execLogic_startWaves);
		if (!self->spawnManagerData->startWithWaves.is_empty())
		{
			if (self->spawnManagerData->chooseOneStart)
			{
				if (!self->spawnManagerData->startWithWaves.is_empty())
				{
					self->start_wave(self->spawnManagerData->startWithWaves[self->random.get_int(self->spawnManagerData->startWithWaves.get_size())]);
				}
			}
			else
			{
				for_every(waveName, self->spawnManagerData->startWithWaves)
				{
					self->start_wave(*waveName);
				}
			}
		}
		else if (!self->spawnManagerData->waves.is_empty())
		{
			self->start_wave(self->spawnManagerData->waves.get_first()->name);
		}
		else
		{
			error(TXT("no waves for spawn manager!"));
		}
	}

	// start dynamic
	{
		MEASURE_PERFORMANCE(spawnManager_execLogic_startDynamic);
		self->start_all_dynamic();
	}

	self->started = true;

	while (true)
	{
		self->update_region_manager();
		LATENT_WAIT(self->random.get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	self->unregister_ai_manager();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void SpawnManager::add_available_spawn_points(SpawnManagerPOIDefinition const & _poi)
{
	if (inRoom.is_set())
	{
		ai_log(this, TXT("from room"));
		if ((_poi.beyondDoorInRoomTagged.get() && !_poi.beyondDoorInRoomTagged->is_empty()) ||
			(_poi.beyondDoorTagged.get() && !_poi.beyondDoorTagged->is_empty()))
		{
			for_every_ptr(door, inRoom->get_all_doors())
			{
				if (!door->is_visible()) continue;
				if ((_poi.beyondDoorInRoomTagged.get() && !_poi.beyondDoorInRoomTagged->is_empty() && _poi.beyondDoorInRoomTagged->check(door->get_tags())) ||
					(_poi.beyondDoorTagged.get() && !_poi.beyondDoorTagged->is_empty() && door->get_door() && _poi.beyondDoorTagged->check(door->get_door()->get_tags())))
				{
					ai_log(this, TXT("check through door"));
					Framework::DoorInRoom* beyondDoor = door;

					int depthCount = max(1, _poi.beyondDoorDepth);
					for_count(int, depth, depthCount)
					{
						if (auto* room = beyondDoor->get_room_on_other_side())
						{
							ai_log(this, TXT("check room \"%S\" at depth %i (/%i)"), room->get_name().to_char(), depth, depthCount);

							if (_poi.inRoomTagged->check(room->get_tags()))
							{
								bool inaccessible = room->get_tags().get_tag_as_int(NAME(inaccessible));
								room->for_every_point_of_interest(_poi.name,
									[this, _poi, beyondDoor, inaccessible](Framework::PointOfInterestInstance* _fpoi)
								{
									ai_log(this, TXT("check POI \"%S\" in r%p tagged \"%S\""), _fpoi->get_name().to_char(), _fpoi->get_room(), _fpoi->get_tags().to_string().to_char());
									if (_poi.tagged->check(_fpoi->get_tags()))
									{
										ai_log(this, TXT("  ok"));
										SpawnPoint sp;
										sp.poi = _fpoi;
										sp.throughDoor = beyondDoor;
										sp.inaccessible = inaccessible;
										sp.beyondDoor = true;
										spawnPointsAvailable.push_back_unique(sp);
									}
								});
							}
							if (depth + 1 < depthCount) // no need to do this check if we're last
							{
								auto* doorOnOtherSide = beyondDoor->get_door_on_other_side();
								beyondDoor = nullptr;
								for_every_ptr(door, room->get_all_doors())
								{
									if (door == doorOnOtherSide)
									{
										continue;
									}
									if (door->can_move_through() && door->is_relevant_for_movement())
									{
										beyondDoor = door;
										break;
									}
								}
								if (!beyondDoor)
								{
									break;
								}
							}
						}
					}
					if (_poi.doorsAppearLockedIfNotOpen)
					{
						SafePtr<Framework::Door> d(door->get_door());
						Game::get()->add_sync_world_job(TXT("set flags to door"),
							[d]()
							{
								if (auto* door = d.get())
								{
									door->set_game_related_system_flag(DoorFlags::AppearLockedIfNotOpen);
								}
							}
						);
					}
				}
			}
		}
		else
		{
			bool inaccessible = inRoom->get_tags().get_tag_as_int(NAME(inaccessible));
			inRoom->for_every_point_of_interest(_poi.name,
				[this, _poi, inaccessible](Framework::PointOfInterestInstance * _fpoi)
			{
				ai_log(this, TXT("check POI \"%S\" in r%p tagged \"%S\""), _fpoi->get_name().to_char(), _fpoi->get_room(), _fpoi->get_tags().to_string().to_char());
				if (_poi.tagged->check(_fpoi->get_tags()))
				{
					ai_log(this, TXT("  tags ok"));
					if (!_fpoi->get_room() ||
						_poi.inRoomTagged->check(_fpoi->get_room()->get_tags()))
					{
						ai_log(this, TXT("  ok"));
						SpawnPoint sp;
						sp.poi = _fpoi;
						sp.inaccessible = inaccessible;
						spawnPointsAvailable.push_back_unique(sp);
					}
				}
			});
		}
	}
	if (inRegion.is_set())
	{
		ai_log(this, TXT("from region"));
		inRegion->for_every_point_of_interest(_poi.name,
			[this, _poi](Framework::PointOfInterestInstance * _fpoi)
		{
			ai_log(this, TXT("check POI \"%S\" in r%p tagged \"%S\""), _fpoi->get_name().to_char(), _fpoi->get_room(), _fpoi->get_tags().to_string().to_char());
			if (_poi.tagged->check(_fpoi->get_tags()))
			{
				ai_log(this, TXT("  ok"));
				SpawnPoint sp;
				sp.poi = _fpoi;
				if (auto* r = _fpoi->get_room())
				{
					sp.inaccessible = r->get_tags().get_tag_as_int(NAME(inaccessible));
				}
				spawnPointsAvailable.push_back_unique(sp);
			}
		});
	}
}

void SpawnManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		return;
	}
#endif

	MEASURE_PERFORMANCE(spawnManager_advance);

	wasDisabledByGameDirector = disabledByGameDirector;
	disabledByGameDirector = false;
	if (auto* gd = GameDirector::get_active())
	{
		disabledByGameDirector = gd->is_violence_disallowed();
		if (disabledByGameDirector)
		{
			if (spawnManagerData->hostile.is_set() &&
				!spawnManagerData->hostile.get())
			{
				disabledByGameDirector = false;
			}
		}
	}

	if (started)
	{
		update_spawn_points(_deltaTime);
		update_waves(_deltaTime);
		update_dynamic(_deltaTime);

		update_rare_advance(_deltaTime);
	}
}

void SpawnManager::update_rare_advance(float _deltaTime)
{
	MEASURE_PERFORMANCE(spawnManager_updateRareAdvance);
	bool hasAnythingToSpawn = false;
	bool needsToWait = !started || spawnManagerData->spawnIfInRoomSeenByPlayer || spawnManagerData->spawnIfPlayerInSameRoom;
	bool rareAdvance = true;
	bool shouldCeaseToExist = false;
	if (!spawnManagerData->waves.is_empty())
	{
		for_every_ref(wave, currentWaves)
		{
			rareAdvance &= wave->wave->rareAdvance;
			hasAnythingToSpawn |= wave->hasAnythingToSpawn;
			needsToWait |= wave->does_need_to_wait();
		}
		if (currentWaves.is_empty())
		{
			noWavesFor += _deltaTime;
		}
		else
		{
			noWavesFor = 0.0f;
		}
		if (noWavesFor > 10.0f)
		{
			shouldCeaseToExist = true;
		}
	}
	if (!spawnManagerData->dynamicSpawns.is_empty())
	{
		rareAdvance = false;
		for_every_ref(di, dynamicSpawns)
		{
			hasAnythingToSpawn |= di->hasAnythingToSpawn;
		}
	}
	if (rareAdvance)
	{
		set_advance_logic_rarely(Range(0.5f, 2.0f));
	}
	else
	{
		set_advance_logic_rarely(NP);
	}
	if (!hasAnythingToSpawn && !needsToWait)
	{
		if (!shouldCeaseToExist)
		{
#ifdef AN_USE_AI_LOG
			if (shouldAlreadyCeaseToExist)
			{
				return;
			}
			shouldAlreadyCeaseToExist = true;
#endif
			ai_log_colour(this, Colour::red);
			ai_log(this, TXT("nothing to spawn, cease to exist"));
#ifdef AN_USE_AI_LOG
			ai_log(this, TXT("but we want may want to debug it (ai logs on), keep it"));
#endif
			ai_log_no_colour(this);
			shouldCeaseToExist = true;
		}
	}
#ifndef AN_USE_AI_LOG
	if (shouldCeaseToExist)
	{
		if (auto* imo = get_imo())
		{
			get_imo()->cease_to_exist(false);
		}
	}
#endif
}

void SpawnManager::update_waves(float _deltaTime)
{
	MEASURE_PERFORMANCE(spawnManager_updateWaves);
	for (int i = 0; i < currentWaves.get_size(); ++i)
	{
		auto* wave = currentWaves[i].get();
		if (!wave->update(this, _deltaTime))
		{
			ai_log_colour(this, Colour::cyan);
			ai_log(this, TXT("< ending wave \"%S\""), currentWaves[i]->wave->name.to_char());
			ai_log_no_colour(this);
			wave->clean_up();
			currentWaves.remove_fast_at(i);
			recalculate_reserved_pools();
			break;
		}
	}
}

void SpawnManager::update_dynamic(float _deltaTime)
{
	MEASURE_PERFORMANCE(spawnManager_dynamic);
	for_every_ref(ds, dynamicSpawns)
	{
		ds->update(this, _deltaTime);
	}

	for_every_ref(ds, dynamicSpawns)
	{
		if (ds->toSpawn > 0)
		{
			if (ds->toSpawnWait == 0.0f &&
				ds->spawn(this))
			{
				break;
			}
			else if (ds->dynamicSpawnInfo->allowOthersToSpawnIfCantSpawn &&
					 (ds->noPlaceToSpawn || !ds->active))
			{
				continue;
			}
			break;
		}
	}
}

SpawnManagerWaveInstance* SpawnManager::start_wave(Name const & _wave)
{
	SpawnManagerWaveInstance* newWave = nullptr;
	for_every_ref(wave, spawnManagerData->waves)
	{
		if (wave->name == _wave)
		{
			newWave = wave->instantiate(this);
			currentWaves.push_back(RefCountObjectPtr<SpawnManagerWaveInstance>(newWave));
		}
	}
	return newWave;
}

void SpawnManager::start_all_dynamic()
{
	for_every_ref(dsi, spawnManagerData->dynamicSpawns)
	{
		DynamicSpawnInfoInstance* newDS = dsi->instantiate(this);
		dynamicSpawns.push_back(RefCountObjectPtr<DynamicSpawnInfoInstance>(newDS));
	}
	DynamicSpawnInfoInstance::sort_by_priority(dynamicSpawns);
}

void SpawnManager::update_spawn_points(float _deltaTime)
{
	MEASURE_PERFORMANCE(spawnManager_updateSpawnPoints);
	for (int i = 0; i < spawnPointsAvailable.get_size(); ++i)
	{
		auto & sp = spawnPointsAvailable[i];
		if (!sp.poi.is_set())
		{
			spawnPointsAvailable.remove_fast_at(i);
			--i;
		}
	}
	for (int i = 0; i < spawnPointsBlocked.get_size(); ++i)
	{
		auto & sp = spawnPointsBlocked[i];
		sp.blockTime -= _deltaTime;
		if (!sp.poi.is_set())
		{
			spawnPointsBlocked.remove_fast_at(i);
			--i;
		}
		else if (sp.blockTime <= 0.0f)
		{
			spawnPointsAvailable.push_back(sp);
			spawnPointsBlocked.remove_fast_at(i);
			--i;
		}
	}
}

Framework::PointOfInterestInstance* SpawnManager::acquire_available_spawn_point(int _idx, SpawnManagerWaveElementInstance const & _elementInstance)
{
	if (spawnPointsAvailable.is_index_valid(_idx))
	{
		spawnPointsBlocked.push_back(spawnPointsAvailable[_idx]);
		spawnPointsAvailable.remove_fast_at(_idx);
		spawnPointsBlocked.get_last().blockedByWaveElement = _elementInstance.element;
		spawnPointsBlocked.get_last().blockTime = _elementInstance.spawnPointBlockTime;
		return spawnPointsBlocked.get_last().poi.get();
	}
	else
	{
		return nullptr;
	}
}

Framework::PointOfInterestInstance* SpawnManager::acquire_available_spawn_point(int _idx, DynamicSpawnInfoInstance const& _dynamicSpawnInstance)
{
	if (spawnPointsAvailable.is_index_valid(_idx))
	{
		spawnPointsBlocked.push_back(spawnPointsAvailable[_idx]);
		spawnPointsAvailable.remove_fast_at(_idx);
		spawnPointsBlocked.get_last().blockedByDynamicSpawn = _dynamicSpawnInstance.dynamicSpawnInfo;
		spawnPointsBlocked.get_last().blockTime = _dynamicSpawnInstance.spawnPointBlockTime;
		return spawnPointsBlocked.get_last().poi.get();
	}
	else
	{
		return nullptr;
	}
}

struct VisitedRoom
{
	Framework::Room* room;
	float furthestDoorDistance; // if our entrance distance is further than this, we should give up
};

struct UpdateSpawnChoosePOI
{
	Framework::Room* room;
	Vector3 loc;
	Framework::DoorInRoom* throughDoor;
	float distanceSoFar;

	UpdateSpawnChoosePOI() {}
	UpdateSpawnChoosePOI(Framework::Room* _room, Vector3 const& _loc, Framework::DoorInRoom* _throughDoor, float _distanceSoFar) : room(_room), loc(_loc), throughDoor(_throughDoor), distanceSoFar(_distanceSoFar) {}
};

#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
void update_separation_distance(SpawnManager* _spawnManager, Array<SpawnChoosePOI> & choosePOIs, Array<VisitedRoom> & visitedRooms, Array<UpdateSpawnChoosePOI> & updateChoosePOIs)
#else
void update_separation_distance(SpawnManager* _spawnManager, ArrayStack<SpawnChoosePOI> & choosePOIs, ArrayStack<VisitedRoom> & visitedRooms, ArrayStack<UpdateSpawnChoosePOI> & updateChoosePOIs)
#endif
{
	Framework::Room* spawnManagerForRoom = _spawnManager->get_for_room();
	Framework::Region* spawnManagerForRegion = _spawnManager->get_for_region();
	while (!updateChoosePOIs.is_empty())
	{
		UpdateSpawnChoosePOI now = updateChoosePOIs.get_first();
		updateChoosePOIs.pop_front();

		Framework::Room * room = now.room;
		Vector3 loc = now.loc;
		Framework::DoorInRoom * throughDoor = now.throughDoor;
		float distanceSoFar = now.distanceSoFar;

		if (spawnManagerForRoom && room != spawnManagerForRoom)
		{
			continue;
		}

		if (spawnManagerForRegion)
		{
			bool inRegion = false;
			auto* region = room->get_in_region();
			while (region)
			{
				if (region == spawnManagerForRegion)
				{
					inRegion = true;
					break;
				}
				region = region->get_in_region();
			}
			if (!inRegion)
			{
				continue;
			}
		}

		bool skipThisPOI = false;
		for_every(vr, visitedRooms)
		{
			if (vr->room == room && vr->furthestDoorDistance <= distanceSoFar)
			{
				// we should not look any further
				skipThisPOI = true;
				break;
			}
		}

		if (skipThisPOI)
		{
			continue;
		}

		bool allPOIsWithinDistanceSoFar = true;
		for_every(choosePOI, choosePOIs)
		{
			if (! choosePOI->separationDistance.is_set() || choosePOI->separationDistance.get() > distanceSoFar)
			{
				allPOIsWithinDistanceSoFar = false;
			}
		}

		if (allPOIsWithinDistanceSoFar)
		{
			// we're distanceSoFar sorted, all others will be further
			break;
		}

		float furthestDoorDistance = distanceSoFar;
		for_every_ptr(door, room->get_doors())
		{
			if (door != throughDoor &&
				door->is_visible())
			{
				furthestDoorDistance = max(furthestDoorDistance, distanceSoFar + (loc - door->get_hole_centre_WS()).length());
			}
		}

		bool existsWithinVisitedRooms = false;
		for_every(vr, visitedRooms)
		{
			if (vr->room == room)
			{
				vr->furthestDoorDistance = min(vr->furthestDoorDistance, furthestDoorDistance);
				existsWithinVisitedRooms = true;
				break;
			}
		}
		if (!existsWithinVisitedRooms)
		{
			VisitedRoom vr;
			vr.room = room;
			vr.furthestDoorDistance = furthestDoorDistance;
#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
			visitedRooms.push_back(vr);
#else
			visitedRooms.push_back_if_possible(vr);
#endif
		}

		for_every(choosePOI, choosePOIs)
		{
			if (choosePOI->poi->get_room() == room)
			{
				float separationDistance = distanceSoFar + (loc - choosePOI->location).length();
				choosePOI->separationDistance = min(separationDistance, choosePOI->separationDistance.get(separationDistance));
			}
		}

		for_every_ptr(door, room->get_doors())
		{
			if (door != throughDoor &&
				door->is_visible() &&
				door->has_world_active_room_on_other_side())
			{
				UpdateSpawnChoosePOI add(door->get_world_active_room_on_other_side(), door->get_door_on_other_side()->get_hole_centre_WS(), door->get_door_on_other_side(), distanceSoFar + (loc - door->get_hole_centre_WS()).length());
				// this is to do distance based search first to cover all as close as possible
				bool alreadyAdded = false;
				for (int i = 0; i < updateChoosePOIs.get_size(); ++ i)
				{
					auto & ucpoi = updateChoosePOIs[i];
					if (ucpoi.room == add.room &&
						ucpoi.throughDoor == add.throughDoor)
					{
						if (ucpoi.distanceSoFar <= add.distanceSoFar)
						{
							alreadyAdded = true;
							break;
						}
						else
						{
							// new one is closer, remove this
							updateChoosePOIs.remove_at(i);
							--i;
						}
					}
				}
				if (!alreadyAdded)
				{
					bool added = false;
					for_every(ucpoi, updateChoosePOIs)
					{
						if (ucpoi->distanceSoFar > add.distanceSoFar)
						{
							int addAtIdx = for_everys_index(ucpoi);
#ifndef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
							if (!updateChoosePOIs.has_place_left())
							{
								updateChoosePOIs.pop_back();
							}
#endif
							updateChoosePOIs.insert_at(addAtIdx, add);
							added = true;
							break;
						}
					}
					if (!added)
					{
#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
						updateChoosePOIs.push_back(add);
#else
						updateChoosePOIs.push_back_if_possible(add);
#endif
					}
				}
			}
		}
	}
}

#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
void SpawnManager::gather_POIs(Array<SpawnChoosePOI> & choosePOIs, Array<SpawnManagerPOIDefinition> const* _usePOIs, Random::Generator & rg, SpawnPlacementCriteria const& _placementCriteria, Optional<RangeInt> const& _distanceFromRecentlySeenByPlayerRoom, Optional<bool> const& _allowInaccessible)
#else
void SpawnManager::gather_POIs(ArrayStack<SpawnChoosePOI> & choosePOIs, Array<SpawnManagerPOIDefinition> const* _usePOIs, Random::Generator & rg, SpawnPlacementCriteria const& _placementCriteria, Optional<RangeInt> const& _distanceFromRecentlySeenByPlayerRoom, Optional<bool> const& _allowInaccessible)
#endif
{
	for_every(sp, spawnPointsAvailable)
	{
		if (!_allowInaccessible.get(true) && sp->inaccessible)
		{
			continue;
		}
		auto * poi = sp->poi.get();
		if (_distanceFromRecentlySeenByPlayerRoom.is_set() &&
			poi->get_room())
		{
			int distance = AVOID_CALLING_ACK_ poi->get_room()->get_distance_to_recently_seen_by_player();
			if (!_distanceFromRecentlySeenByPlayerRoom.get().does_contain(distance))
			{
				continue;
			}
		}
		if (!_usePOIs || sp->is_ok_for(*_usePOIs))
		{
			// poi location is in poi room
			Vector3 poiLocation = poi->calculate_placement().get_translation();

			// both below are in the same room/space, ref is for player, poi is for poi (might be doors)
			Optional<Vector3> decisionRefPoint;
			Vector3 decisionPOIocation = poiLocation;


			{
				Framework::Room* poiRoom = poi->get_room();
				Framework::Room* forRoom = poiRoom;
				if (auto* d = sp->throughDoor.get())
				{
					forRoom = d->get_in_room();
					decisionPOIocation = d->get_placement().get_translation();
				}

				if (auto* r = forRoom)
				{
					if (AVOID_CALLING_ACK_ r->was_recently_seen_by_player() &&
						r->get_recently_seen_by_player_placement().is_set())
					{
						decisionRefPoint = r->get_recently_seen_by_player_placement().get().get_translation();
					}
				}

				if (! _placementCriteria.ignoreVisitedByPlayer)
				{
					if (auto* r = poiRoom)
					{
						float timeSinceLastVisitedByPlayer = r->get_time_since_last_visited_by_player();
						if (timeSinceLastVisitedByPlayer < GameSettings::get().difficulty.blockSpawningInVisitedRoomsTime)
						{
							continue;
						}
					}
				}
			}


			if ((_placementCriteria.minDistanceToAlive.is_set() ||
				 _placementCriteria.minTileDistanceToAlive.is_set()) &&
				poi->get_room())
			{
				float minDistanceToAlive = 0.0f;
				if (_placementCriteria.minDistanceToAlive.is_set())
				{
					minDistanceToAlive = rg.get(_placementCriteria.minDistanceToAlive.get());
				}
				if (_placementCriteria.minTileDistanceToAlive.is_set())
				{
					minDistanceToAlive = rg.get(_placementCriteria.minTileDistanceToAlive.get()) * RoomGenerationInfo::get().get_tile_size();
				}
				if (_placementCriteria.maxPlayAreaDistanceToAlive.is_set())
				{
					minDistanceToAlive = min(minDistanceToAlive,
						  rg.get(_placementCriteria.maxPlayAreaDistanceToAlive.get())
						* max(RoomGenerationInfo::get().get_play_area_rect_size().x, RoomGenerationInfo::get().get_play_area_rect_size().y));
				}
				// works for current room only!
				struct CheckRoom
				{
					Framework::Room* room;
					Vector3 from;
					float dist;

					CheckRoom() {}
					CheckRoom(Framework::Room* _room, Vector3 const& _from, float _dist) : room(_room), from(_from), dist(_dist) {}

					static int compare_distance(void const* _a, void const* _b)
					{
						CheckRoom const* a = plain_cast<CheckRoom>(_a);
						CheckRoom const* b = plain_cast<CheckRoom>(_b);
						if (a->dist < b->dist) return A_BEFORE_B;
						if (a->dist > b->dist) return B_BEFORE_A;
						return A_AS_B;
					}
				};
				ARRAY_STACK(CheckRoom, checkRooms, 6);
				bool distanceClear = true;
				checkRooms.push_back(CheckRoom(poi->get_room(), poiLocation, 0.0f));
				{
					int i = 0;
					while (i < checkRooms.get_size())
					{
						CheckRoom cr = checkRooms[i];
						for_every_ptr(obj, cr.room->get_objects())
						{
							if (auto* act = fast_cast<Framework::Actor>(obj))
							{
								float dist = (obj->get_presence()->get_centre_of_presence_WS() - cr.from).length();
								dist += cr.dist;
								if (dist < minDistanceToAlive &&
									(_placementCriteria.minDistanceActorTags.is_empty() ||
									 _placementCriteria.minDistanceActorTags.check(act->get_tags())))
								{
									distanceClear = false;
									break;
								}
							}
						}
						if (!distanceClear)
						{
							// fail quickly
							break;
						}
						for_every_ptr(door, cr.room->get_doors())
						{
							if (!door->is_visible()) continue;
							float dist = (door->get_hole_centre_WS() - cr.from).length();
							if (dist > 0.001f) // otherwise would be we're going back
							{
								float addDist = dist + cr.dist;
								if (addDist < minDistanceToAlive &&
									door->has_world_active_room_on_other_side())
								{
									CheckRoom addcr(door->get_world_active_room_on_other_side(), door->get_door_on_other_side()->get_hole_centre_WS(), addDist);
									int addAt = checkRooms.get_size();
									for_every(c, checkRooms)
									{
										if (addcr.dist < c->dist)
										{
											addAt = for_everys_index(c);
											break;
										}
									}
									if (addAt < checkRooms.get_size())
									{
										if (!checkRooms.has_place_left())
										{
											checkRooms.pop_back();
										}
										checkRooms.insert_at(addAt, addcr);
									}
									else
									{
										checkRooms.push_back_if_possible(addcr);
									}
								}
							}
						}
						++i;
					}
				}
				if (!distanceClear)
				{
					// skip this one
					continue;
				}
			}

			float match = 0.0f;

			if (_placementCriteria.atDoorDistance.is_set())
			{
				if (auto* room = poi->get_room())
				{
					float doorDistanceSq = sqr(1000.0f);
					for_every_ptr(door, room->get_doors())
					{
						if (!door->is_visible()) continue;
						float dist = (poiLocation - door->get_hole_centre_WS()).length_squared();
						doorDistanceSq = min(doorDistanceSq, dist);
					}
					if (!_placementCriteria.atDoorDistance.get().does_contain(doorDistanceSq))
					{
						// skip this one
						continue;
					}
				}
			}

			if (_placementCriteria.atYaw.is_set() && decisionRefPoint.is_set())
			{
				float yaw = Rotator3::get_yaw((decisionPOIocation - decisionRefPoint.get()).to_vector2());
				float atYaw = _placementCriteria.atYaw.get().centre();
				float atYawRadius = _placementCriteria.atYaw.get().length() * 0.5f;
				float off = Rotator3::normalise_axis(yaw - atYaw);
				if (abs(off) > atYawRadius)
				{
					// skip this one
					continue;
				}
			}

			if (decisionRefPoint.is_set())
			{
				if (_placementCriteria.atDistance.is_set() &&
					!_placementCriteria.atDistance.get().is_empty())
				{
					float distance = (decisionPOIocation - decisionRefPoint.get()).length();

					{
						auto const & preferAtDistance = _placementCriteria.atDistance.get();
						if (distance > preferAtDistance.max)
						{
							continue;
						}
						else if (distance < preferAtDistance.min)
						{
							continue;
						}
					}
				}

				if (_placementCriteria.preferAtDistance.is_set() &&
					!_placementCriteria.preferAtDistance.get().is_empty())
				{
					float distance = (decisionPOIocation - decisionRefPoint.get()).length();

					float matchDistance = 0.0f;
					{
						auto const & preferAtDistance = _placementCriteria.preferAtDistance.get();
						if (distance > preferAtDistance.max)
						{
							matchDistance -= distance - preferAtDistance.max;
						}
						else if (distance < preferAtDistance.min)
						{
							matchDistance -= preferAtDistance.min - distance;
						}
					}

					if (_placementCriteria.distanceCoef.is_set())
					{
						matchDistance *= _placementCriteria.distanceCoef.get();
					}
					match += matchDistance;
				}
				else if (_placementCriteria.distanceCoef.is_set())
				{
					float distance = (decisionPOIocation - decisionRefPoint.get()).length();

					match -= distance * _placementCriteria.distanceCoef.get();
				}
			}

			if (_placementCriteria.addRandom.is_set())
			{
				match += random.get_float(0.0f, _placementCriteria.addRandom.get());
			}

			choosePOIs.push_back(SpawnChoosePOI(match, for_everys_index(sp), poi, poiLocation));
		}
	}

#ifdef LOG_MORE
	ai_log(this, TXT(" choose from %i (poi) from %i (sp av)"), choosePOIs.get_size(), spawnPointsAvailable.get_size());
#endif

	if (!choosePOIs.is_empty() &&
		!_placementCriteria.separateFromTags.is_empty() &&
		(_placementCriteria.keepSeparatedDistance.get(1.0f) < 1.0f ||
		 _placementCriteria.keepSeparatedAmount.get(1.0f) < 1.0f ||
		 _placementCriteria.minSeparationDistance.get(0.0f) > 0.0f))
	{
		int roomCount = 0;
		if (auto* room = inRoom.get())
		{
			roomCount = room->get_in_sub_world()->get_all_rooms().get_size();
		}
		else if (auto* region = inRegion.get())
		{
			roomCount = region->get_in_sub_world()->get_all_rooms().get_size();
		}
		an_assert(roomCount);
#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
		Array<VisitedRoom> visitedRooms;
		Array<UpdateSpawnChoosePOI> updateChoosePOIs;
#else
		ARRAY_STACK(VisitedRoom, visitedRooms, roomCount);
		ARRAY_STACK(UpdateSpawnChoosePOI, updateChoosePOIs, roomCount);
#endif
		for_every(choosePOI, choosePOIs)
		{
			choosePOI->separationDistance.clear();
		}
		bool separated = false;
		for_every(spb, spawnPointsBlocked)
		{
			if (spb->blockedByWaveElement.is_set() &&
				!spb->blockedByWaveElement->placementCriteria.separationTags.is_empty() &&
				_placementCriteria.separateFromTags.check(spb->blockedByWaveElement->placementCriteria.separationTags))
			{
				separated = true;
				visitedRooms.clear();
				updateChoosePOIs.clear();
				updateChoosePOIs.push_back(UpdateSpawnChoosePOI(spb->poi->get_room(), spb->poi->calculate_placement().get_translation(), nullptr, 0.0f));
				update_separation_distance(this, choosePOIs, visitedRooms, updateChoosePOIs);
			}
			if (spb->blockedByDynamicSpawn.is_set() &&
				!spb->blockedByDynamicSpawn->placementCriteria.separationTags.is_empty() &&
				_placementCriteria.separateFromTags.check(spb->blockedByDynamicSpawn->placementCriteria.separationTags))
			{
				separated = true;
				visitedRooms.clear();
				updateChoosePOIs.clear();
				updateChoosePOIs.push_back(UpdateSpawnChoosePOI(spb->poi->get_room(), spb->poi->calculate_placement().get_translation(), nullptr, 0.0f));
				update_separation_distance(this, choosePOIs, visitedRooms, updateChoosePOIs);
			}
		}

		if (separated)
		{
			sort(choosePOIs, SpawnChoosePOI::compare_separtion_distance);
			float keepSeparatedDistance = _placementCriteria.keepSeparatedDistance.get(1.0f);
			float keepSeparatedAmount = _placementCriteria.keepSeparatedAmount.get(1.0f);
			int ksaCount = (int)((float)choosePOIs.get_size() * keepSeparatedAmount);
			int ksdCount = choosePOIs.get_size();
			if (keepSeparatedDistance < 1.0f)
			{
				float maxSeparationDistance = choosePOIs.get_first().separationDistance.get(0.0f /* should not happen? */) * (1.0f - keepSeparatedDistance);
				ksdCount = 0;
				for_every(c, choosePOIs)
				{
					if (c->separationDistance.is_set() &&
						c->separationDistance.get() < maxSeparationDistance)
					{
						ksdCount = for_everys_index(c);
						break;
					}
				}
			}
			if (keepSeparatedDistance >= 0.5f) { ksdCount = max(1, ksdCount); }
			if (keepSeparatedAmount >= 0.5f) { ksaCount = max(1, ksaCount); }
			int choosePOIsSize = min(ksdCount, ksaCount);
			choosePOIsSize = max(choosePOIsSize, min(_placementCriteria.keepSeparatedMinCount, choosePOIs.get_size()));
#ifdef LOG_MORE
			ai_log(this, TXT("  after separating, we're left with %i"), choosePOIsSize);
#endif
			if (_placementCriteria.minSeparationDistance.is_set())
			{
				for_every(c, choosePOIs)
				{
					if (c->separationDistance.is_set() &&
						c->separationDistance.get() < _placementCriteria.minSeparationDistance.get())
					{
						choosePOIsSize = for_everys_index(c);
						break;
					}
				}
#ifdef LOG_MORE
				ai_log(this, TXT("  after min distance, we're left with %i"), choosePOIsSize);
#endif
			}
			choosePOIs.set_size(choosePOIsSize);
		}
	}
}

void SpawnManager::log_logic_info(LogInfoContext& _log) const
{
	base::log_logic_info(_log);

	if (disabledByGameDirector)
	{
		_log.log(TXT("disabled by game director"));
	}

	if (!currentWaves.is_empty())
	{
		_log.log(TXT("WAVES"));
		LOG_INDENT(_log);
		for_every_ref(wave, currentWaves)
		{
			_log.log(TXT("wave %S"), wave->wave->name.to_char());
			LOG_INDENT(_log);

			// implement whatever is required
			_log.log(TXT("spawned %i"), wave->spawned.get_size());
			{
				LOG_INDENT(_log);
				for_every(sp, wave->spawned)
				{
					if (auto* spimo = sp->spawned.get())
					{
						_log.log(TXT("%S %S v:%S"), sp->poolName.to_char(), spimo->get_presence()->get_placement().get_translation().to_string().to_char(), spimo->get_presence()->get_velocity_linear().to_string().to_char());
					}
				}
			}

			_log.log(TXT("wave info"));
			{
				LOG_INDENT(_log);
				wave->log_logic_info(_log);
			}

			_log.log(TXT(""));
		}
	}

	if (!dynamicSpawns.is_empty())
	{
		_log.log(TXT("DYNAMIC SPAWNS"));
		LOG_INDENT(_log);
		for_every_ref(dsi, dynamicSpawns)
		{
			_log.log(TXT("dynamic spawn \"%S\""), dsi->dynamicSpawnInfo->name.to_char());
			LOG_INDENT(_log);

			Optional<Colour> colour;
			bool active = dsi->active;
			if (dsi->dynamicSpawnInfo->regionPoolConditions.useRegionPool.is_valid())
			{
				if (auto* rm = regionManager.get())
				{
					active &= rm->is_pool_active(dsi->dynamicSpawnInfo->regionPoolConditions.useRegionPool);
				}
				else
				{
					active = false;
				}
			}
			if (dsi->regionPoolConditionsInstance.lostPoolRequired.is_set())
			{
				if (auto* rm = regionManager.get())
				{
					active &= rm->get_lost_pool_value(dsi->dynamicSpawnInfo->regionPoolConditions.lostRegionPoolRequired.get(dsi->dynamicSpawnInfo->regionPoolConditions.useRegionPool)) > dsi->regionPoolConditionsInstance.lostPoolRequired.get();
				}
				else
				{
					active = false;
				}
			}
			if (! active)
			{
				colour = Colour::grey;
			}
			_log.set_colour(colour);
			if (active)
			{
				_log.log(TXT("[ACTIVE]"));
			}
			else
			{
				_log.log(TXT("[inactive] (time:%.1f)"), dsi->timeToActivate);
			}
			_log.log(TXT("priority      %6.1f"), dsi->dynamicSpawnInfo->priority);
			if (dsi->noPlaceToSpawn && dsi->toSpawn) _log.set_colour(Colour::red);
			_log.log(TXT("to spawn      %4i %S"), dsi->toSpawn, dsi->noPlaceToSpawn? TXT("[no place to spawn]") : TXT(""));
			_log.set_colour(colour);
			_log.log(TXT("  wait        %6.1f"), dsi->toSpawnWait);
			_log.log(TXT("  interval    %6.1f-%.1f"), dsi->dynamicSpawnInfo->spawnInterval.min, dsi->dynamicSpawnInfo->spawnInterval.max);
			if (! dsi->beingSpawned) _log.set_colour(Colour::grey);
			_log.log(TXT("being spawned %4i"), dsi->beingSpawned);
			_log.set_colour(colour);
			if (dsi->globalConcurrentCountId.is_valid())
			{
					_log.log(TXT("spawned       %4i < %i (of %i) [%S]"), dsi->spawned.get_size(), GlobalSpawnManagerInfo::get(dsi->globalConcurrentCountId), dsi->concurrentLimit, dsi->globalConcurrentCountId.to_char());
			}
			else
			{
				if (dsi->concurrentLimit >= 0)
				{
					_log.log(TXT("spawned       %4i (of %i)"), dsi->spawned.get_size(), dsi->concurrentLimit);
				}
				else
				{
					_log.log(TXT("spawned       %4i"), dsi->spawned.get_size());
				}
			}

			if (dsi->dynamicSpawnInfo->regionPoolConditions.useRegionPool.is_valid())
			{
				_log.log(TXT("region pool      %S"), dsi->dynamicSpawnInfo->regionPoolConditions.useRegionPool.to_char());
				if (auto* rm = regionManager.get())
				{
					_log.log(TXT("  left        %6.1f SV"), rm->get_pool_value(dsi->dynamicSpawnInfo->regionPoolConditions.useRegionPool).as_float());
					if (dsi->regionPoolConditionsInstance.lostPoolRequired.is_set())
					{
						_log.log(TXT("  req lost    %6.1f SV"), dsi->regionPoolConditionsInstance.lostPoolRequired.get().as_float());
						_log.log(TXT("    of pool      %S"), dsi->dynamicSpawnInfo->regionPoolConditions.lostRegionPoolRequired.get(dsi->dynamicSpawnInfo->regionPoolConditions.useRegionPool).to_char());
					}
				}
				else
				{
					_log.set_colour(Colour::red);
					_log.log(TXT("  [no region manager]"));
					_log.set_colour();
				}
			}
			else
			{
				if (dsi->left >= 0)
				{
					_log.log(TXT("left          %4i (of %i)"), dsi->left, dsi->limit);
				}
			}

			{
				_log.log(TXT("spawned"));
				LOG_INDENT(_log);
				for_every(s, dsi->spawned)
				{
					if (auto* imo = s->spawned.get())
					{
						_log.log(TXT(":o%p:%S"),imo, imo->ai_get_name().to_char());
					}
					else
					{
						_log.log(TXT(":o%p:[does not exist]"), imo);
					}
				}
			}

			_log.log(TXT(""));
			_log.set_colour();
		}
	}

#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
	_log.log(TXT("DETAILS"));
	{
		LOG_INDENT(_log);
		for_every(di, detailsInfo)
		{
			_log.log(TXT("%S"), *di);
		}
	}
#endif

	if (spawnManagerData && spawnManagerData->reservePools)
	{
		_log.log(TXT("RESERVED POOLS (normal waves only)"));
		LOG_INDENT(_log);
		for_every(rp, reservedPools)
		{
			_log.log(TXT("%S %S"), rp->reserved.as_string_auto_decimals().pad_left(8).to_char(), rp->pool.to_char());
		}
	}
}

void SpawnManager::spawn_infos_async(Array<SpawnManagerSpawnInfo> const& spawnInfos, Random::Generator rg)
{
	ASSERT_ASYNC;

	scoped_call_stack_info(TXT("SpawnManager::spawn_infos_async"));
	SafePtr<Framework::IModulesOwner> safeImo;
	safeImo = get_mind()->get_owner_as_modules_owner();

	if (Game::get()->does_want_to_cancel_creation() || !safeImo.get())
	{
		return;
	}

	Framework::ScopedAutoActivationGroup saag(safeImo.get()->get_in_world());

#ifdef AN_USE_AI_LOG
	if (safeImo.get())
	{
		ai_log(this, TXT("starting spawn (%i)"), spawnInfos.get_size());
	}
#endif
#ifdef AN_OUTPUT_SPAWN
	output(TXT("[spawn manager] starting spawn"));
#endif

	for_every(si, spawnInfos)
	{
		int spawnCount = 1;
		Vector3 spawnOffset = Vector3::zero;
		Framework::ObjectType* spawnType = si->get_type_to_spawn(this, OUT_ spawnCount, OUT_ spawnOffset);
		if (!spawnType)
		{
#ifdef AN_OUTPUT_SPAWN
			output(TXT("[spawn manager] nothing to spawn via spawn manager"));
#endif
			warn(TXT("nothing to spawn via spawn manager"));
			si->restore_to_spawn();
			continue;
		}

		if (GameSettings::get().difficulty.npcs == GameSettings::NPCS::NoNPCs &&
			spawnType->get_tags().get_tag(NAME(npcCharacter)))
		{
#ifdef AN_OUTPUT_SPAWN
			output(TXT("[spawn manager] + we don't want NPCs, don't spawn \"%S\""), spawnType->get_name().to_string().to_char());
#endif
#ifdef AN_USE_AI_LOG
			ai_log(this, TXT("+ we don't want NPCs, don't spawn \"%S\""), spawnType->get_name().to_string().to_char());
#endif
			si->restore_to_spawn();
			continue;
		}

		if (auto* poi = si->atPOI.get())
		{
			if (auto* room = poi->get_room())
			{
				if (spawnType->get_tags().get_tag(NAME(hostileCharacter)) &&
					(GameSettings::get().difficulty.npcs <= GameSettings::NPCS::NoHostiles ||
					 poi->get_room()->get_value<bool>(NAME(gameDirectorSafeSpace), false, false)))
				{
#ifdef AN_OUTPUT_SPAWN
					output(TXT("[spawn manager] + we don't want hostile NPCs, don't spawn \"%S\""), spawnType->get_name().to_string().to_char());
#endif
#ifdef AN_USE_AI_LOG
					ai_log(this, TXT("+ we don't want hostile NPCs, don't spawn \"%S\""), spawnType->get_name().to_string().to_char());
#endif
					si->restore_to_spawn();
					continue;
				}

				Optional<Transform> goodPlacement;

				spawnCount = max(1, spawnCount);
				for (int i = 1; i < spawnCount; ++i) si->add_extra_being_spawned();
				bool continueSpawning = true;
				for_count(int, spawnIdx, spawnCount)
				{
					if (!continueSpawning)
					{
#ifdef LOG_MORE
						ai_log_colour(this, Colour::magenta);
						ai_log(this, TXT("don't continue spawning"));
						ai_log_no_colour(this);
#endif
						si->restore_to_spawn();
						continue;
					}
					Framework::Object* spawned = nullptr;
					bool okToPlace = true;
#ifdef AN_USE_AI_LOG
					if (safeImo.get())
					{
						ai_log(this, TXT(" spawn \"%S\""), spawnType->get_name().to_string().to_char());
					}
#endif
#ifdef AN_OUTPUT_SPAWN
					output(TXT("[spawn manager] spawn \"%S\""), spawnType->get_name().to_string().to_char());
#endif

					spawnType->load_on_demand_if_required();

					Game::get()->perform_sync_world_job(TXT("spawn via spawn manager"),
						[spawnType, safeImo, &spawned, this]()
					{
						if (safeImo.get())
						{
							spawned = spawnType->create(String::printf(TXT("%S %S [spawn manager: %S]"),
								inOpenWorldCell.is_set()? String::printf(TXT("%ix%i "), inOpenWorldCell.get().x, inOpenWorldCell.get().y).to_char() : TXT(""),
								String::printf(TXT("[%04i] %S"), totalSpawnCount.increase(), spawnType->get_name().to_string().to_char()).to_char(),
								safeImo->ai_get_name().to_char()));
							spawned->init(safeImo->get_in_sub_world());
						}
					});

					if (!spawned)
					{
#ifdef AN_USE_AI_LOG
						ai_log(this, TXT("didn't spawn"));
#endif
					}

#ifdef AN_USE_AI_LOG
					if (safeImo.get() && spawned)
					{
						ai_log(this, TXT(" spawned \"%S\""), spawned->get_name().to_char());
						ai_log(this, TXT(" o%p"), spawned);
						ai_log(this, TXT(" in room \"%S\" of type \"%S\""),
							room? room->get_name().to_char() : TXT("<no room>"),
							room && room->get_room_type()? room->get_room_type()->get_name().to_string().to_char() : TXT("<no room type>")
							);
					}
#endif
					if (spawned)
					{
						{
							scoped_call_stack_info(TXT("collect variables"));
							room->collect_variables(spawned->access_variables());
							si->setup_variables(spawned->access_variables());
							spawned->access_variables().set_from(safeImo->get_variables());
						}
						spawned->access_individual_random_generator() = rg.spawn();

						if (! spawnManagerData->ignoreGameDirector && si->hostile)
						{
							spawned->access_tags().set_tag(NAME(hostileAtGameDirector));
						}

						// store info to auto mark when killed
						if (auto* wei = si->waveElementInstance.get())
						{
							if (auto* e = wei->element.get())
							{
								if (e->storeExistenceInPilgrimage)
								{
									if (spawnIdx == 0)
									{
										scoped_call_stack_info(TXT("set store existence in pilgrimage"));
										spawned->access_variables().access<bool>(NAME(storeExistenceInPilgrimage)) = true;
										spawned->access_variables().access<Framework::WorldAddress>(NAME(storeExistenceInPilgrimageWorldAddress)) = room->get_world_address();
										spawned->access_variables().access<int>(NAME(storeExistenceInPilgrimagePOIIdx)) = si->poiIdx;
									}
									else
									{
										error(TXT("multiple spawns don't work with storing existence"));
									}
								}
							}
						}

						if (spawnManagerData && spawnManagerData->noLoot)
						{
							spawned->access_variables().access<bool>(NAME(noLoot)) = true;
						}

						spawned->initialise_modules();
					}

					Transform placeAt = poi->calculate_placement();

					{
						if (!goodPlacement.is_set())
						{
							Framework::PointOfInterestInstance::FindPlacementParams fppNoMB;
							if (!poi->get_parameters().is_empty())
							{
								// we're using some very generic spawn point, we may want to specify it a bit more
								// we may want to place on walls and if so, we may want to override some params
								// we do it only for non mesh boundary, for mesh boundary we should have strict definitions provided
								fppNoMB.stick_to_wall_in_any_dir();
								fppNoMB.stick_to_wall_ray_cast_distance(2.0f);
							}
							goodPlacement = poi->find_placement_for(spawned, room, rg.spawn(), NP, fppNoMB);
						}

						okToPlace = goodPlacement.is_set();
						placeAt = goodPlacement.get(placeAt);

#ifdef AN_USE_AI_LOG
						if (safeImo.get())
						{
							if (okToPlace)
							{
								ai_log(this, TXT("   place valid"));
							}
							else
							{
								ai_log(this, TXT("   no valid place found"));
							}
						}
#endif

					}

					if (!okToPlace && spawned)
					{
#ifdef LOG_MORE
						ai_log_colour(this, Colour::magenta);
						ai_log(this, TXT("not ok to place, destroy"));
						ai_log_no_colour(this);
#endif
						spawned->destroy();
						spawned = nullptr;
						si->restore_to_spawn();
						continueSpawning = false;
					}

					if (spawned)
					{
#ifdef AN_OUTPUT_SPAWN
						output(TXT("[spawn manager] spawned \"%S\" o%p"), spawned->get_name().to_char(), spawned);
#endif

						Framework::Object* travelWith = nullptr;

						if (Framework::ObjectType* travelWithType = si->get_travel_with_type(this))
						{
							travelWithType->load_on_demand_if_required();

							Game::get()->perform_sync_world_job(TXT("spawn travel-with via spawn manager"),
								[travelWithType, safeImo, &travelWith]()
								{
									if (safeImo.is_set())
									{
										travelWith = travelWithType->create(String::printf(TXT("%S [spawn manager: %S]"), travelWithType->get_name().to_string().to_char(), safeImo->ai_get_name().to_char()));
										travelWith->init(safeImo->get_in_sub_world());
									}
								});

							if (travelWith)
							{
								{
									scoped_call_stack_info(TXT("collect variables [travel with]"));
									room->collect_variables(travelWith->access_variables());
									si->setup_variables(travelWith->access_variables());
									travelWith->access_variables().set_from(safeImo->get_variables());
								}
								travelWith->access_individual_random_generator() = rg.spawn();

								travelWith->initialise_modules();

#ifdef AN_USE_AI_LOG
								if (safeImo.get())
								{
									ai_log_colour(this, Colour::c64Brown);
									ai_log(this, TXT("+ travel with \"%S\""), travelWith->get_name().to_char());
									ai_log_no_colour(this);
								}
#endif

								todo_note(TXT("spawn with capsule or flying device bringing it in"));
								Game::get()->perform_sync_world_job(TXT("place travel-with"),
									[travelWith, room, placeAt]()
									{
										if (!room->is_deletion_pending() &&
											!room->are_objects_no_longer_advanceable())
										{
											travelWith->get_presence()->place_in_room(room, placeAt);
										}
										else
										{
											// otherwise will be destroyed when can't be placed below
										}
									});
							}
						}

						if (!travelWith)
						{
							// offset placement just in case there's someone else in the very same spot
							// collisions will push as away
							// only if we're allowed to move
							if (spawned->get_movement())
							{
								placeAt.set_translation(placeAt.get_translation() + Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), 0.0f) * 0.05f);

								// offset for spawn batch
								placeAt.set_translation(placeAt.get_translation() + spawnOffset * (float)spawnIdx);
							}
						}

						{
							bool allowPlacingIfSeen = false; // or travelWith
							if (auto* wei = si->waveElementInstance.get())
							{
								if (wei->element.is_set())
								{
									allowPlacingIfSeen = wei->element->allowPlacingIfSeen;
								}
							}
							// check if we're not being placed where player may see us, if so, drop it
							bool placed = false;
							bool foundPlace = false;
							Game::get()->perform_sync_world_job(TXT("place spawned"),
								[spawned, room, placeAt, travelWith, &foundPlace, &placed, allowPlacingIfSeen]()
								{
									foundPlace = true;
									if (travelWith
#ifdef AN_DEVELOPMENT
										|| (true) // for development, ignore this check
#else
										|| (!room->was_recently_seen_by_player() || allowPlacingIfSeen)
#endif
										)
									{
										if (! room->is_deletion_pending() &&
											! room->are_objects_no_longer_advanceable())
										{
											placed = true;
											spawned->get_presence()->place_in_room(room, placeAt);
											if (travelWith)
											{
												if (auto* carrier = travelWith->get_custom<CustomModules::Carrier>())
												{
													carrier->carry(spawned);
												}
												else
												{
													error(TXT("travel-with \"%S\" does not have carrier custom module!"), travelWith->get_name().to_char());
												}
											}
										}
										else
										{
											// otherwise will be destroyed when can't be placed below
										}
									}
								});
#ifdef AN_USE_AI_LOG
							if (safeImo.get())
							{
								if (!foundPlace)
								{
									ai_log_colour(this, Colour::red);
									ai_log(this, TXT("+ spawned \"%S\" but no place found - destroyed"), spawned->get_name().to_char());
									ai_log(this, TXT("  o%p"), spawned);
									ai_log_no_colour(this);
								}
								else
								{
									if (placed)
									{
										ai_log_colour(this, Colour::yellow);
										ai_log(this, TXT("+ spawned \"%S\""), spawned->get_name().to_char());
										ai_log(this, TXT("  o%p"), spawned);
										ai_log_no_colour(this);
									}
									else
									{
										ai_log_colour(this, Colour::orange);
										ai_log(this, TXT("+ spawned \"%S\" but not placed - destroyed"), spawned->get_name().to_char());
										ai_log(this, TXT("  o%p"), spawned);
										ai_log_no_colour(this);
									}
								}
							}
#endif
#ifdef AN_OUTPUT_SPAWN
							output(TXT("[spawn manager] spawned done \"%S\" %S"), spawned->get_name().to_char(), !foundPlace ? TXT("no valid place") : (placed ? TXT("placed") : TXT("dropped (would be seen)")));
#endif
#ifdef AN_USE_AI_LOG
							if (safeImo.get())
							{
								ai_log(this, TXT(" spawn \"%S\" done %S"), spawnType->get_name().to_string().to_char(), !foundPlace? TXT("no valid place") : (placed? TXT("placed") : TXT("dropped (would be seen)")));
							}
#endif
							if (placed)
							{
								if (safeImo.get())
								{
									if (!spawnManagerData->tagSpawnedActor.is_empty())
									{
										if (auto* act = fast_cast<Framework::Actor>(spawned))
										{
											act->access_tags().set_tags_from(spawnManagerData->tagSpawnedActor);
										}
									}
								}
								si->on_spawned(this, spawned);
							}
							else
							{
								spawned->destroy();
								if (travelWith)
								{
									travelWith->destroy();
								}
								si->restore_to_spawn();
							}
						}
					}
				}
			}
		}
	}

#ifdef AN_USE_AI_LOG
#ifdef LOG_MORE
	if (safeImo.get())
	{
		ai_log(this, TXT("spawned all and done"));
	}
#endif
#endif

}

Energy SpawnManager::get_reserved_pools(Name const& _pool) const
{
	if (_pool.is_valid())
	{
		for_every(rp, reservedPools)
		{
			if (rp->pool == _pool)
			{
				return rp->reserved;
			}
		}
	}
	return Energy::zero();
}

void SpawnManager::reserve_pool(Name const& _pool, Energy const& _value)
{
	if (!spawnManagerData || !spawnManagerData->reservePools)
	{
		return;
	}
	if (!_pool.is_valid())
	{
		return;
	}
	for_every(rp, reservedPools)
	{
		if (rp->pool == _pool)
		{
			rp->reserved += _value;
			return;
		}
	}

	ReservedPool rp;
	rp.pool = _pool;
	rp.reserved = _value;
	reservedPools.push_back(rp);
}

void SpawnManager::free_pool(Name const& _pool, Energy const& _value)
{
	if (!_pool.is_valid())
	{
		return;
	}
	for_every(rp, reservedPools)
	{
		if (rp->pool == _pool)
		{
			rp->reserved -= _value;
			return;
		}
	}
}

void SpawnManager::recalculate_reserved_pools()
{
	reservedPools.clear();
	for_every_ref(wave, currentWaves)
	{
		for_every(s, wave->spawned)
		{
			if (!s->killed)
			{
				reserve_pool(s->poolName, s->poolValue);
			}
		}
	}
}

//

Framework::ObjectType* SpawnManagerSpawnInfo::get_type_to_spawn(SpawnManager* _spawnManager, OUT_ int & _count, OUT_ Vector3 & _spawnOffset) const
{
	if (auto* wei = waveElementInstance.get())
	{
		return wei->get_type_to_spawn(_spawnManager, OUT_ _count, OUT_ _spawnOffset);
	}
	if (auto* di = dynamicInstance.get())
	{
		return di->get_type_to_spawn(_spawnManager, OUT_ _count, OUT_ _spawnOffset);
	}
	return nullptr;
}

Framework::ObjectType* SpawnManagerSpawnInfo::get_travel_with_type(SpawnManager* _spawnManager) const
{
	if (auto* wei = waveElementInstance.get())
	{
		return wei->get_travel_with_type(_spawnManager);
	}
	if (auto* di = dynamicInstance.get())
	{
		return di->get_travel_with_type(_spawnManager);
	}
	return nullptr;
}

void SpawnManagerSpawnInfo::add_extra_being_spawned() const
{
	if (auto* wi = waveInstance.get())
	{
		wi->add_one_extra_being_spawned(this);
	}
	if (auto* di = dynamicInstance.get())
	{
		di->add_one_extra_being_spawned(this);
	}
}

void SpawnManagerSpawnInfo::restore_to_spawn() const
{
	if (auto* wi = waveInstance.get())
	{
		wi->restore_one_to_spawn(this);
	}
	if (auto* di = dynamicInstance.get())
	{
		di->restore_one_to_spawn(this);
	}
}

void SpawnManagerSpawnInfo::on_spawned(SpawnManager* _spawnManager, Framework::IModulesOwner* _spawned) const
{
	an_assert(_spawned);

	Framework::WorldAddress worldAddress;
	if (auto* poi = atPOI.get())
	{
		if (auto* r = poi->get_room())
		{
			worldAddress = r->get_world_address();
		}
	}

	if (auto* wei = waveElementInstance.get())
	{
		wei->on_spawned(_spawnManager, _spawned, worldAddress, poiIdx);
	}
	if (auto* di = dynamicInstance.get())
	{
		di->on_spawned(_spawnManager, _spawned, worldAddress, poiIdx);
	}


	float playSpawnTelegraphChance = GameSettings::get().difficulty.allowTelegraphOnSpawn;
	if (_spawnManager->access_random().get_chance(playSpawnTelegraphChance))
	{
		_spawned->set_timer(0.2f, NAME(spawnTelegraph), [](Framework::IModulesOwner* _imo)
			{
				if (auto* s = _imo->get_sound())
				{
					s->play_sound(NAME(spawnTelegraph));
				}
			});
	}
}

void SpawnManagerSpawnInfo::setup_variables(SimpleVariableStorage& _variables) const
{
	if (auto* wei = waveElementInstance.get())
	{
		_variables.set_from(wei->element->parameters);
	}
	if (auto* di = dynamicInstance.get())
	{
		_variables.set_from(di->dynamicSpawnInfo->parameters);
	}
}

//

bool SpawnManager::SpawnPoint::is_ok_for(Array<SpawnManagerPOIDefinition> const & _pois) const
{
	auto* actualPOI = poi.get();
	if (!actualPOI)
	{
		return false;
	}
	// has to match at least one
	for_every(pd, _pois)
	{
		if (actualPOI->get_name() == pd->name &&
			pd->tagged->check(actualPOI->get_tags()))
		{
			bool requiresBeyondDoor = false;
			if ((pd->beyondDoorTagged.get() && ! pd->beyondDoorTagged.get()->is_empty()) ||
				(pd->beyondDoorInRoomTagged.get() && !pd->beyondDoorInRoomTagged.get()->is_empty()))
			{
				requiresBeyondDoor = true;
			}
			if (requiresBeyondDoor == beyondDoor)
			{
				return true;
			}
		}
	}
	return false;
}

//

REGISTER_FOR_FAST_CAST(SpawnManagerData);

SpawnManagerData::SpawnManagerData()
{
}

SpawnManagerData::~SpawnManagerData()
{
}

bool SpawnManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	tagSpawnedActor.load_from_xml_attribute_or_child_node(_node, TXT("tagSpawnedActor"));

	ignoreGameDirector = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreGameDirector"), ignoreGameDirector);
	
	hostile.load_from_xml(_node, TXT("hostile"));

	waitUntilGenerated = _node->get_bool_attribute_or_from_child_presence(TXT("waitUntilGenerated"), waitUntilGenerated);
	
	spawnIfInRoomSeenByPlayer = _node->get_bool_attribute_or_from_child_presence(TXT("spawnIfInRoomSeenByPlayer"), spawnIfInRoomSeenByPlayer);
	spawnIfPlayerInSameRoom = _node->get_bool_attribute_or_from_child_presence(TXT("spawnIfPlayerInSameRoom"), spawnIfPlayerInSameRoom);

	bool accessibilityAcknowledged = false;
	load_allow_inaccessible(_node, REF_ allowInaccessible, REF_ accessibilityAcknowledged);

	result &= onSpawnHunches.load_from_xml(_node, _lc);

	chooseOneStart = _node->get_bool_attribute_or_from_child_presence(TXT("chooseOneStart"), chooseOneStart);
	if (_node->get_name_attribute(TXT("startWithWave")).is_valid())
	{
		startWithWaves.push_back(_node->get_name_attribute(TXT("startWithWave")));
	}
	for_every(node, _node->children_named(TXT("startWithWave")))
	{
		Name n = Name(node->get_text());
		if (n.is_valid())
		{
			startWithWaves.push_back(n);
		}
	}

	for_every(node, _node->children_named(TXT("wave")))
	{
		SpawnManagerWavePtr wave(new SpawnManagerWave());
		if (wave->load_from_xml(node, _lc))
		{
			waves.push_back(wave);
		}
		else
		{
			error_loading_xml(node, TXT("could not load wave"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("dynamic")))
	{
		DynamicSpawnInfoPtr dsi(new DynamicSpawnInfo());
		if (dsi->load_from_xml(node, _lc))
		{
			dynamicSpawns.push_back(dsi);
		}
		else
		{
			error_loading_xml(node, TXT("could not load dynamic-spawn-info"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("poi")))
	{
		SpawnManagerPOIDefinition poi;
		if (poi.load_from_xml(node))
		{
			if ((poi.beyondDoorTagged.get() && !poi.beyondDoorTagged->is_empty()) ||
				(poi.beyondDoorInRoomTagged.get() && !poi.beyondDoorInRoomTagged->is_empty()))
			{
				if (!accessibilityAcknowledged)
				{
					warn_loading_xml(node, TXT("using beyond door tagged but not decided about accessibility, if to be ignored, use \"acknowledgeAccessibility\" bool (or empty sub-node)"));
				}
			}
			spawnPOIs.push_back(poi);
		}
		else
		{
			error_loading_xml(node, TXT("problem loading poi for spawn manager"));
			result = false;
		}
	}
	spawnPointBlockTime = _node->get_float_attribute_or_from_child(TXT("spawnPointBlockTime"), spawnPointBlockTime);

	reservePools = _node->get_bool_attribute_or_from_child_presence(TXT("reservePools"), reservePools);
	
	noLoot = _node->get_bool_attribute_or_from_child_presence(TXT("noLoot"), noLoot);

	return result;
}

bool SpawnManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every_ref(wave, waves)
	{
		result &= wave->prepare_for_game(waves, _library, _pfgContext);
	}

	for_every_ref(dsi, dynamicSpawns)
	{
		result &= dsi->prepare_for_game(dynamicSpawns, _library, _pfgContext);
	}

	return result;
}

//

bool SpawnManagerWave::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	warn_loading_xml_on_assert(! _node->get_attribute(TXT("id")), _node, TXT("use \"name\" instead of \"id\" - deprecated"));

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	sameAs = _node->get_name_attribute_or_from_child(TXT("sameAs"), sameAs);

	chooseOneNext = _node->get_bool_attribute_or_from_child_presence(TXT("chooseOneNext"), chooseOneNext);

	if (_node->get_name_attribute(TXT("startWithMe")).is_valid())
	{
		startWithMe.push_back(_node->get_name_attribute(TXT("startWithMe")));
	}
	for_every(node, _node->children_named(TXT("startWithMe")))
	{
		Name n = Name(node->get_name_attribute(TXT("wave")));
		if (n.is_valid())
		{
			startWithMe.push_back(n);
		}
	}

	if (_node->get_name_attribute(TXT("next")).is_valid())
	{
		next.push_back(WaveWithChance(_node->get_name_attribute(TXT("next"))));
	}
	for_every(node, _node->children_named(TXT("next")))
	{
		Name n = Name(node->get_name_attribute(TXT("wave")));
		if (n.is_valid())
		{
			next.push_back(WaveWithChance(n, node->get_float_attribute(TXT("probCoef"), 1.0f)));
		}
	}

	if (_node->get_name_attribute(TXT("endWithMe")).is_valid())
	{
		endWithMe.push_back(_node->get_name_attribute(TXT("endWithMe")));
	}
	for_every(node, _node->children_named(TXT("endWithMe")))
	{
		Name n = Name(node->get_name_attribute(TXT("wave")));
		if (n.is_valid())
		{
			endWithMe.push_back(n);
		}
	}

	if (_node->get_name_attribute(TXT("nextOnCease")).is_valid())
	{
		nextOnCease.push_back(_node->get_name_attribute(TXT("nextOnCease")));
	}
	for_every(node, _node->children_named(TXT("nextOnCease")))
	{
		Name n = Name(node->get_name_attribute(TXT("wave")));
		if (n.is_valid())
		{
			nextOnCease.push_back(n);
		}
	}

	distanceToCease.load_from_xml(_node, TXT("distanceToCease"));
	
	spawnInterval.load_from_xml(_node, TXT("spawnInterval"));

	result &= onSpawnHunches.load_from_xml(_node, _lc);

	//

	for_every(node, _node->children_named(TXT("element")))
	{
		SpawnManagerWaveElementPtr element;
		element = new SpawnManagerWaveElement();
		if (element->load_from_xml(node, _lc))
		{
			elements.push_back(element);
		}
		else
		{
			error_loading_xml(node, TXT("could not load element"));
			result = false;
		}
	}

	//

	for_every(node, _node->children_named(TXT("poi")))
	{
		SpawnManagerPOIDefinition poi;
		if (poi.load_from_xml(node))
		{
			spawnPOIs.push_back(poi);
		}
		else
		{
			error_loading_xml(node, TXT("problem loading poi for spawn manager"));
			result = false;
		}
	}
	spawnPointBlockTime.load_from_xml(_node, TXT("spawnPointBlockTime"));

	//

	maxCountAtTheMoment.load_from_xml(_node, TXT("maxCountAtTheMoment"));
	spawnBelowCountPt.load_from_xml(_node, TXT("spawnBelowCountPt"));
	endAtCountPt.load_from_xml(_node, TXT("endAtCountPt"));
	endAtCount.load_from_xml(_node, TXT("endAtCount"));
	endAtUsefulCount.load_from_xml(_node, TXT("endAtUsefulCount"));
	safeSpaceWait = _node->get_bool_attribute_or_from_child_presence(TXT("safeSpaceWait"), safeSpaceWait);
	rareAdvance = _node->get_bool_attribute_or_from_child_presence(TXT("rareAdvance"), rareAdvance);
	endWhenSpawned = _node->get_bool_attribute_or_from_child_presence(TXT("endWhenSpawned"), endWhenSpawned);
	endIfPlayerIsIn = _node->get_bool_attribute_or_from_child_presence(TXT("endIfPlayerIsIn"), endIfPlayerIsIn);
	endIfPlayerIsInCell = _node->get_bool_attribute_or_from_child_presence(TXT("endIfPlayerIsInCell"), endIfPlayerIsInCell);
	endIfDistanceToSeenLowerThan.load_from_xml(_node, TXT("endIfDistanceToSeenLowerThan"));
	endIfDistanceToSeenAtLeast.load_from_xml(_node, TXT("endIfDistanceToSeenAtLeast"));
	endIfStoryFacts.load_from_xml_attribute_or_child_node(_node, TXT("endIfStoryFacts"));
	skipIfStoryFacts.load_from_xml_attribute_or_child_node(_node, TXT("skipIfStoryFacts"));
	setStoryFacts.load_from_xml_attribute_or_child_node(_node, TXT("setStoryFacts"));

	keepWaveAfterEnded = _node->get_bool_attribute_or_from_child_presence(TXT("keepWaveAfterEnded"), keepWaveAfterEnded);

	infiniteCount = _node->get_bool_attribute_or_from_child_presence(TXT("infiniteCount"), infiniteCount);
	timeBased.load_from_xml(_node, TXT("timeBased"));
	timeBased.load_from_xml(_node, TXT("endAfterTime"));

	return result;
}

bool SpawnManagerWave::prepare_for_game(Array<SpawnManagerWavePtr> const & _waves, ::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every_ref(element, elements)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::InitialPrepare)
	{
		if (sameAs.is_valid())
		{
			for_every_ref(wave, _waves)
			{
				if (wave != this &&
					wave->name == sameAs)
				{
					elements = wave->elements; // may share them!

					rareAdvance = wave->rareAdvance;

					maxCountAtTheMoment = wave->maxCountAtTheMoment;
					spawnBelowCountPt = wave->spawnBelowCountPt;

					endAtCountPt = wave->endAtCountPt;
					endAtCount = wave->endAtCount;
					endAtUsefulCount = wave->endAtUsefulCount;
					safeSpaceWait = wave->safeSpaceWait;
					endIfPlayerIsIn = wave->endIfPlayerIsIn;
					endIfPlayerIsInCell = wave->endIfPlayerIsInCell;
					endIfDistanceToSeenLowerThan = wave->endIfDistanceToSeenLowerThan;
					endIfDistanceToSeenAtLeast = wave->endIfDistanceToSeenAtLeast;
					endIfStoryFacts = wave->endIfStoryFacts;
					setStoryFacts = wave->setStoryFacts;
					skipIfStoryFacts = wave->skipIfStoryFacts;
					endWhenSpawned = wave->endWhenSpawned;

					keepWaveAfterEnded = wave->keepWaveAfterEnded;

					infiniteCount = wave->infiniteCount;
					timeBased = wave->timeBased;

					break;
				}
			}
		}
	}

	return result;
}

SpawnManagerWaveInstance* SpawnManagerWave::instantiate(SpawnManager* _spawnManager) const
{
	SpawnManagerWaveInstance* instance = new SpawnManagerWaveInstance();

	instance->wave = cast_to_nonconst(this);

	instance->hasAnythingToSpawn = false;
	for_every_ref(element, elements)
	{
		instance->elements.push_back(RefCountObjectPtr<SpawnManagerWaveElementInstance>(element->instantiate(_spawnManager, instance)));
		if (!instance->hasAnythingToSpawn)
		{
			instance->hasAnythingToSpawn |= has_anything_to_spawn(element->spawnSet, element->types, _spawnManager);
		}
	}

	if (maxCountAtTheMoment.is_set()) instance->maxCountAtTheMoment = _spawnManager->random.get(maxCountAtTheMoment.get());
	if (spawnBelowCountPt.is_set()) instance->spawnBelowCountPt = _spawnManager->random.get(spawnBelowCountPt.get());

	if (endAtCountPt.is_set()) instance->endAtCountPt = _spawnManager->random.get(endAtCountPt.get());
	if (endAtCount.is_set()) instance->endAtCount = _spawnManager->random.get(endAtCount.get());
	if (endAtUsefulCount.is_set()) instance->endAtUsefulCount = _spawnManager->random.get(endAtUsefulCount.get());

	instance->timeLeft = (timeBased.is_set() && ! timeBased.get().is_empty())? _spawnManager->random.get(timeBased.get()) : 0.0f;

	return instance;
}

//

bool SpawnPlacementCriteria::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	atDistance.load_from_xml(_node, TXT("atDistance"));
	preferAtDistance.load_from_xml(_node, TXT("preferAtDistance"));
	atDoorDistance.load_from_xml(_node, TXT("atDoorDistance"));
	atYaw.load_from_xml(_node, TXT("atYaw"));
	distanceCoef.load_from_xml(_node, TXT("distanceCoef"));
	addRandom.load_from_xml(_node, TXT("addRandom"));

	separationTags.load_from_xml(_node, TXT("separationTags"));
	separateFromTags.load_from_xml_attribute_or_child_node(_node, TXT("separateFromTags"));
	keepSeparatedDistance.load_from_xml(_node, TXT("keepSeparatedDistance"));
	keepSeparatedAmount.load_from_xml(_node, TXT("keepSeparatedAmount"));
	minSeparationDistance.load_from_xml(_node, TXT("minSeparationDistance"));
	keepSeparatedMinCount = _node->get_int_attribute(TXT("keepSeparatedMinCount"), keepSeparatedMinCount);

	minDistanceToAlive.load_from_xml(_node, TXT("minDistanceToAlive"));
	minTileDistanceToAlive.load_from_xml(_node, TXT("minTileDistanceToAlive"));
	maxPlayAreaDistanceToAlive.load_from_xml(_node, TXT("maxPlayAreaDistanceToAlive"));
	minDistanceActorTags.load_from_xml_attribute_or_child_node(_node, TXT("minDistanceActorTags"));

	ignoreVisitedByPlayer = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreVisitedByPlayer"), ignoreVisitedByPlayer);

	return result;
}

//

void SpawnRegionPoolConditionsInstance::init(SpawnManager* _spawnManager, SpawnRegionPoolConditions const& _with)
{
	if (_with.lostPoolRequired.is_set())
	{
		lostPoolRequired = _with.lostPoolRequired.get().get_random(_spawnManager->access_random());
	}
}

//

bool SpawnRegionPoolConditions::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	useRegionPool = _node->get_name_attribute_or_from_child(TXT("useRegionPool"), useRegionPool);
	lostRegionPoolRequired.load_from_xml(_node, TXT("lostRegionPoolRequired"));
	lostPoolRequired.load_from_xml(_node, TXT("lostPoolRequired"));

	return result;
}

bool SpawnRegionPoolConditions::check(SpawnManager* _spawnManager, SpawnRegionPoolConditionsInstance const& _instance) const
{
	if (useRegionPool.is_valid())
	{
		if (auto* rm = _spawnManager->regionManager.get())
		{
			if ((rm->get_available_for_pool(useRegionPool) - _spawnManager->get_reserved_pools(useRegionPool)).is_positive())
			{
				if (_instance.lostPoolRequired.is_set())
				{
					if (rm->get_lost_pool_value(lostRegionPoolRequired.get(useRegionPool)) <= _instance.lostPoolRequired.get())
					{
						return false;
					}
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool SpawnRegionPoolConditions::is_hostile(SpawnManager* _spawnManager) const
{
	if (useRegionPool.is_valid())
	{
		if (auto* rm = _spawnManager->regionManager.get())
		{
			return rm->is_pool_hostile(useRegionPool);
		}
	}
	return false;
}

int SpawnRegionPoolConditions::get_altered_limit(SpawnManager* _spawnManager, int _limit) const
{
	if (useRegionPool.is_valid())
	{
		if (auto* rm = _spawnManager->regionManager.get())
		{
			rm->alter_limit_for_pool(useRegionPool, _limit);
		}
	}
	return _limit;
}

//

Spawned::Spawned(Framework::IModulesOwner* imo, SpawnManagerWaveElement* _byWaveElement)
: spawned(imo)
, poolValue(RegionManager::get_value_for(imo))
#ifdef AN_DEVELOPMENT_OR_PROFILER
, name(imo ? imo->ai_get_name() : String::empty())
#endif
, byWaveElement(_byWaveElement)
{
}

//

bool SpawnManagerWaveElement::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	addCount.load_from_xml(_node, TXT("addCount"));
	chance = _node->get_float_attribute(TXT("chance"), chance);
	count.load_from_xml(_node, TXT("count"));
	countPct.load_from_xml(_node, TXT("countPct"));
	priority = _node->get_float_attribute(TXT("priority"), priority);
	mayBeSkipped = _node->get_bool_attribute_or_from_child_presence(TXT("mayBeSkipped"), mayBeSkipped);
	allowPlacingIfSeen = _node->get_bool_attribute_or_from_child_presence(TXT("allowPlacingIfSeen"), allowPlacingIfSeen);
	storeExistenceInPilgrimage = _node->get_bool_attribute_or_from_child_presence(TXT("storeExistenceInPilgrimage"), storeExistenceInPilgrimage);
	
	result &= onSpawnHunches.load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("poi")))
	{
		SpawnManagerPOIDefinition poi;
		if (poi.load_from_xml(node))
		{
			spawnPOIs.push_back(poi);
		}
		else
		{
			error_loading_xml(node, TXT("problem loading poi for spawn manager"));
			result = false;
		}
	}
	spawnPointBlockTime.load_from_xml(_node, TXT("spawnPointBlockTime"));

	{
		spawnBatch.load_from_xml(_node, TXT("spawnBatch"));
		spawnBatchOffset.load_from_xml(_node, TXT("spawnBatchOffset"));
		for_every(n, _node->children_named(TXT("spawnBatch")))
		{
			spawnBatchOffset.load_from_xml(n, TXT("offset"));
		}

		if (!spawnBatch.is_empty())
		{
			if (count == RangeInt::zero &&
				countPct.is_empty())
			{
				// we need at least one, if we have batch set
				count = RangeInt(1);
			}
		}
	}

	spawnSet = _node->get_name_attribute(TXT("spawnSet"), spawnSet);

	hostile.load_from_xml(_node, TXT("hostile"));
	result &= regionPoolConditions.load_from_xml(_node, _lc);

	if (_node->has_attribute(TXT("type")))
	{
		Framework::UsedLibraryStored<Framework::ObjectType> type;
		if (type.load_from_xml(_node, TXT("type"), _lc))
		{
			types.push_back(type);
		}
	}
	for_every(node, _node->children_named(TXT("type")))
	{
		Framework::UsedLibraryStored<Framework::ObjectType> type;
		if (type.load_from_xml(node, TXT("type"), _lc))
		{
			types.push_back(type);
		}
	}

	result &= travelWithType.load_from_xml(_node, TXT("travelWith"), _lc);

	parameters.load_from_xml_child_node(_node, TXT("parameters"));

	for_every(node, _node->children_named(TXT("placementCriteria")))
	{
		result &= placementCriteria.load_from_xml(node, _lc);
	}

	return result;
}

bool SpawnManagerWaveElement::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		for_every(type, types)
		{
			type->set(get_object_type(_library, type->get_name()));
			if (!type->get())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!Framework::Library::may_ignore_missing_references())
#endif
				{
					error(TXT("not found \"%S\""), type->get_name().to_string().to_char());
					result = false;
				}
			}
		}
		if (travelWithType.is_name_valid())
		{
			travelWithType.set(get_object_type(_library, travelWithType.get_name()));
			if (!travelWithType.get())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!Framework::Library::may_ignore_missing_references())
#endif
				{
					error(TXT("not found \"%S\""), travelWithType.get_name().to_string().to_char());
					result = false;
				}
			}
		}
	}

	return result;
}

SpawnManagerWaveElementInstance* SpawnManagerWaveElement::instantiate(SpawnManager* _spawnManager, SpawnManagerWaveInstance* _wave) const
{
	SpawnManagerWaveElementInstance* instance = new SpawnManagerWaveElementInstance();

	instance->element = cast_to_nonconst(this);
	instance->wave = _wave;
	instance->manager = _spawnManager;
	instance->countLeft = 0;

	instance->regionPoolConditionsInstance.init(_spawnManager, regionPoolConditions);

	if (!instance->element->spawnPOIs.is_empty())
	{
		instance->usePOIs = &instance->element->spawnPOIs;
	}
	else if (!instance->wave->wave->spawnPOIs.is_empty())
	{
		instance->usePOIs = &instance->wave->wave->spawnPOIs;
	}
	else
	{
		instance->usePOIs = &instance->manager->spawnManagerData->spawnPOIs;
	}

	if (chance == 1.0f || _spawnManager->random.get_chance(chance))
	{
		if (!countPct.is_empty())
		{
			RangeInt countAvailable;
			int availableSpawnPointsCount = 0;
			for_every(sp, _spawnManager->spawnPointsAvailable)
			{
				if (sp->is_ok_for(*instance->usePOIs))
				{
					++availableSpawnPointsCount;
				}
			}
			countAvailable.min = (int)round((float)availableSpawnPointsCount * countPct.min);
			countAvailable.max = (int)round((float)availableSpawnPointsCount * countPct.max);
			if (countAvailable.max <= countAvailable.min)
			{
				instance->countLeft = countAvailable.min;
			}
			else
			{
				instance->countLeft = _spawnManager->random.get(countAvailable);
			}
		}
		else
		{
			instance->countLeft = max(0, _spawnManager->random.get(count));
		}
		if (addCount.is_set())
		{
			if (auto* region = _spawnManager->get_for_region())
			{
				instance->countLeft += addCount.get_with_default(region, 0, AllowToFail);
			}
			if (auto* room = _spawnManager->get_for_room())
			{
				instance->countLeft += addCount.get_with_default(room, 0, AllowToFail);
			}
		}
	}
	instance->countLeft = max(0, instance->countLeft);
	instance->countLeft = regionPoolConditions.get_altered_limit(_spawnManager, instance->countLeft);
	instance->spawnPointBlockTime = spawnPointBlockTime.get(_wave->wave->spawnPointBlockTime.get(_spawnManager->spawnManagerData->get_spawn_point_block_time()));

	return instance;
}

//

void SpawnManagerWaveInstance::log_logic_info(LogInfoContext& _log) const
{
	if (state == NotStarted)
	{
		_log.set_colour(Colour::c64LightBlue);
		_log.log(TXT("not started"));
	}
	else if (state == Active)
	{
		_log.set_colour(Colour::c64LightGreen);
		_log.log(TXT("ACTIVE"));
	}
	else if (state == Ended)
	{
		_log.set_colour(Colour::c64LightRed);
		_log.log(TXT("ended"));
	}
	else if (state == Ended)
	{
		_log.log(TXT("??"));
	}
	_log.set_colour();
	_log.log(TXT("spawnRequested          : %S"), spawnRequested ? TXT("YES") : TXT("no"));
	_log.log(TXT("spawning                : %S"), spawning ? TXT("YES") : TXT("no"));
	_log.log(TXT("toSpawnLeft             : %i"), toSpawnLeft);
	_log.log(TXT("toSpawnWait             : %.3f"), toSpawnWait);
	_log.log(TXT("ceaseRequested          : %S"), ceaseRequested ? TXT("YES") : TXT("no"));
	_log.log(TXT("endRequested            : %S"), endRequested ? TXT("YES") : TXT("no"));
	_log.log(TXT("endRequestedWithoutNext : %S"), endRequestedWithoutNext ? TXT("YES") : TXT("no"));
	_log.log(TXT(""));
	_log.log(TXT("timeLeft                : %.3f"), timeLeft);

#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
	_log.log(TXT("DETAILS"));
	{
		LOG_INDENT(_log);
		for_every(di, detailsInfo)
		{
			_log.log(TXT("%S"), *di);
		}
	}
#endif
}

void SpawnManagerWaveInstance::clean_up()
{
	wave.clear();
	spawned.clear();

	for_every_ref(e, elements)
	{
		e->clean_up();
	}
	elements.clear();

#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
	detailsInfo.clear();
#endif
}

bool SpawnManagerWaveInstance::does_need_to_wait() const
{
	return timeLeft > 0.0f || (wave.get() && wave->does_need_to_wait());
}

bool SpawnManagerWaveInstance::update(SpawnManager* _spawnManager, float _deltaTime)
{
	update_spawned(_spawnManager);

	toSpawnWait = max(0.0f, toSpawnWait - _deltaTime);

	if (_spawnManager->spawnManagerData->spawnIfInRoomSeenByPlayer)
	{
		if (auto* r = _spawnManager->inRoom.get())
		{
			if (!r->was_recently_seen_by_player())
			{
				toSpawnWait = max(toSpawnWait, 0.1f);
			}
		}
	}
	if (_spawnManager->spawnManagerData->spawnIfPlayerInSameRoom)
	{
		if (auto* r = _spawnManager->inRoom.get())
		{
			if (!r->was_recently_visited_by_player())
			{
				toSpawnWait = max(toSpawnWait, 0.1f);
			}
		}
	}

	int count = spawned.get_size();

	bool endThisOne = false;
	bool ceaseThisOne = false;

	if (state == NotStarted)
	{
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
		detailsInfo.clear();
#endif

		bool shouldStart = true;
		if (wave->distanceToCease.is_set() && _spawnManager->inRoom.get() &&
			AVOID_CALLING_ACK_ _spawnManager->inRoom->get_distance_to_recently_seen_by_player() >= wave->distanceToCease.get())
		{
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
			detailsInfo.push_back(TXT("too far, do not start"));
#endif
			shouldStart = false;
		}
		if (!wave->skipIfStoryFacts.is_empty())
		{
			if (auto* gd = GameDirector::get_active())
			{
				Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
				if (wave->skipIfStoryFacts.check(gd->access_story_execution().get_facts()))
				{
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
					detailsInfo.push_back(TXT("story facts not met"));
#endif
					shouldStart = false;
					endThisOne = true;
				}
			}
		}
		if (shouldStart)
		{
			ai_log_colour(_spawnManager, Colour::cyan);
			ai_log(_spawnManager, TXT("> activating wave \"%S\""), wave->name.to_char());
			ai_log_no_colour(_spawnManager);
			for_every(name, wave->startWithMe)
			{
				_spawnManager->start_wave(*name);
			}
			spawnRequested = true;
			state = Active;
			if (!wave->setStoryFacts.is_empty())
			{
				if (auto* gd = GameDirector::get_active())
				{
					Concurrency::ScopedMRSWLockWrite lock(gd->access_story_execution().access_facts_lock());
					gd->access_story_execution().access_facts().set_tags_from(wave->setStoryFacts);
				}
			}
		}
	}

	if (spawnRequested)
	{
#ifdef LOG_MORE
		ai_log(_spawnManager, TXT("spawn requested"));
#endif
		spawnRequested = false;
		// spawn all
		toSpawnLeft = 0;
		for_every_ref(element, elements)
		{
			toSpawnLeft += element->countLeft;
		}
		maxTotalCount = max(maxTotalCount, toSpawnLeft);
		// unless there is a limit
		if (maxCountAtTheMoment.is_set())
		{
			toSpawnLeft = min(toSpawnLeft, maxCountAtTheMoment.get());
		}
		// update max count
		spawnedNowCount = count + toSpawnLeft;
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
		if (detailsInfo.is_empty() || (! String::compare_icase(detailsInfo.get_last(), TXT("spawn requested \\")) &&
									   ! String::compare_icase(detailsInfo.get_last(), TXT("spawn requested /"))))
		{
			detailsInfo.push_back(TXT("spawn requested \\"));
		}
		else
		{
			if (String::compare_icase(detailsInfo.get_last(), TXT("spawn requested \\")))
			{
				detailsInfo.get_last() = TXT("spawn requested /");
			}
			else
			{
				detailsInfo.get_last() = TXT("spawn requested \\");
			}
		}
#endif
	}

	if (!spawning && toSpawnLeft > 0 && toSpawnWait == 0.0f &&
		! Game::get()->is_removing_unused_temporary_objects())
	{
		Array<SpawnManagerSpawnInfo> spawnInfos;
		if (maxCountAtTheMoment.is_set())
		{
			// limit how many do we have to spawn
			toSpawnLeft = max(toSpawnLeft, maxCountAtTheMoment.get() - count);
		}

#ifdef LOG_MORE
		ai_log(_spawnManager, TXT("wants to spawn %i"), toSpawnLeft);
#endif

		Random::Generator rg = _spawnManager->access_random().spawn();

		while (toSpawnLeft > 0)
		{
			// check if we still have anything left to spawn
			float highestPriority = 0.0f;
			{
				int left = 0;
				for_every_ref(element, elements)
				{
					if (element->countLeft <= 0)
					{
						continue;
					}
					if (! element->element->regionPoolConditions.check(_spawnManager, element->regionPoolConditionsInstance))
					{
						continue; // choose something else
					}
					if (left == 0 || element->element->priority > highestPriority)
					{
						left = 0;
					}
					if (left == 0 || element->element->priority >= highestPriority)
					{
						highestPriority = element->element->priority;
						left += element->countLeft;
					}
				}
				if (left <= 0)
				{
					toSpawnLeft = 0;
					break;
				}
			}
			int elementIdx = RandomUtils::ChooseFromContainer<Array<RefCountObjectPtr<SpawnManagerWaveElementInstance>>, RefCountObjectPtr<SpawnManagerWaveElementInstance>>::choose(_spawnManager->random, elements,
				[](RefCountObjectPtr<SpawnManagerWaveElementInstance> const & _element)
				{
					return (float)_element.get()->countLeft;
				},
				[highestPriority](RefCountObjectPtr<SpawnManagerWaveElementInstance> const & _element)
				{
					return _element.get()->element->priority >= highestPriority;
				}
				);

			auto* element = elements[elementIdx].get();

			// gather pois
#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
			Array<SpawnChoosePOI> choosePOIs;
#else
			ARRAY_STACK(SpawnChoosePOI, choosePOIs, max(1, _spawnManager->spawnPointsAvailable.get_size()));
#endif
			bool allowInaccessible = _spawnManager->spawnManagerData->allowInaccessible.get(rg.get_chance(0.4f));
			_spawnManager->gather_POIs(choosePOIs, element->usePOIs, rg, element->element->placementCriteria, NP, allowInaccessible);

#ifdef LOG_MORE
			ai_log(_spawnManager, TXT(" left to spawn %i, choose from %i (poi)"), toSpawnLeft, choosePOIs.get_size());
#endif

			if (choosePOIs.is_empty() && !element->element->mayBeSkipped)
			{
				toSpawnWait = rg.get(wave->spawnInterval); // try later
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
				detailsInfo.push_back(TXT("no suitable POIs"));
				if (allowInaccessible)
				{
					detailsInfo.push_back(TXT("  inaccessible were allowed"));
				}
				else
				{
					detailsInfo.push_back(TXT("  inaccessible were not allowed"));
				}
#endif
				break;
			}

			if (!wave->infiniteCount)
			{
				element->countLeft--;
			}

			if (!choosePOIs.is_empty())
			{
				// sort them to have best match
				sort(choosePOIs);

				auto& choosePOI = choosePOIs[0];
				auto* atPOI = _spawnManager->acquire_available_spawn_point(choosePOI.idx, *element);

				bool addSpawn = true;
				if (element->element->storeExistenceInPilgrimage)
				{
					if (auto* pi = PilgrimageInstance::get())
					{
						if (auto* r = choosePOI.poi->get_room())
						{
#ifdef LOG_MORE
							output(TXT("spawn manager: check address=\"%S\" poiIdx=\"%i\""), r->get_world_address().to_string().to_char(), choosePOI.idx);
							output(TXT("check in pilgrimage : %S"), pi->has_been_killed(r->get_world_address(), choosePOI.idx) ? TXT("killed") : TXT("ok"));
#endif
							if (pi->has_been_killed(r->get_world_address(), choosePOI.idx))
							{
								addSpawn = false;
							}
						}
					}
				}

				if (addSpawn)
				{
					// add spawn info
					SpawnManagerSpawnInfo si;
					si.atPOI = atPOI;
					si.hostile = element->element->hostile.get(_spawnManager->spawnManagerData->hostile.get(element->element->regionPoolConditions.is_hostile(_spawnManager)));
					si.poiIdx = choosePOI.idx;
					si.waveInstance = this;
					si.waveElementInstance = element;
					spawnInfos.push_back(si);
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
					detailsInfo.push_back(TXT("add spawn info to process"));
#endif
				}
			}

			// less to spawn!
			toSpawnLeft--;
		}

		if (!spawnInfos.is_empty())
		{
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
			detailsInfo.push_back(TXT("issue async spawn"));
#endif
			ai_log(_spawnManager, TXT("queue spawning %i object(s)"), spawnInfos.get_size());
			spawning = true;
			Random::Generator rgBase = _spawnManager->random.spawn();
			RefCountObjectPtr<SpawnManagerWaveInstance> refSelf(this);
			SafePtr<Framework::IModulesOwner> safeImo(_spawnManager->get_mind()->get_owner_as_modules_owner());
			Game::get()->add_async_world_job(TXT("spawn bunch via spawn manager"),
				[spawnInfos, rgBase, refSelf, safeImo, _spawnManager]() // this is just to set spawning to false (if object does still exist), spawnInfo not as reference, we need a copy
			{
				MEASURE_PERFORMANCE(spawnBunchViaSpawnManager);

#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
				if (refSelf.is_set())
				{
					refSelf->detailsInfo.push_back(TXT("start async spawn"));
				}
#endif

				if (safeImo.is_set() && // should catch if owner was deleted
					_spawnManager)
				{
					_spawnManager->spawn_infos_async(spawnInfos, rgBase);
				}

				if (refSelf.is_set())
				{
#ifdef AN_SHOW_SPAWN_MANGER_DETAILS_INFO
					refSelf->detailsInfo.push_back(TXT("finished async spawn"));
#endif
					// done spawning, we just have to wait now to activate 
					refSelf->spawning = false;
				}
			});
		}
		toSpawnWait = max(rg.get(wave->spawnInterval), toSpawnWait);
	}

	if (state == Active &&
		!spawning &&
		!endRequested)
	{
		timeLeft = max(0.0f, timeLeft - _deltaTime);
		if (wave->timeBased.is_set())
		{
			if (timeLeft <= 0.0f)
			{
				endThisOne = true;
			}
		}

		int countLeft = 0;
		for_every_ref(element, elements)
		{
			countLeft += element->countLeft;
		}

		int countNowAndLeft = countLeft + count;

		if (wave->endIfPlayerIsIn)
		{
			if (auto* playerActor = Game::get_as<Game>()->access_player().get_actor())
			{
				if (auto* inRoom = _spawnManager->inRoom.get())
				{
					if (playerActor->get_presence() && playerActor->get_presence()->get_in_room() == _spawnManager->inRoom.get())
					{
						endThisOne = true;
					}
				}
				if (auto* inRegion = _spawnManager->inRegion.get())
				{
					if (auto* playerInRoom = playerActor->get_presence()->get_in_room())
					{
						auto* playerInRegion = playerInRoom->get_in_region();
						while (playerInRegion)
						{
							endThisOne |= playerInRegion == inRegion;
							playerInRegion = playerInRegion->get_in_region();
						}
					}
				}
			}
		}
		if (wave->endIfPlayerIsInCell)
		{
			if (_spawnManager->inOpenWorldCell.is_set())
			{
				if (auto* playerActor = Game::get_as<Game>()->access_player().get_actor())
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						auto playerCellAt = piow->find_cell_at(playerActor);
						if (playerCellAt.is_set() &&
							playerCellAt == _spawnManager->inOpenWorldCell)
						{
							endThisOne = true;
						}
					}
				}
			}
		}
		if (wave->endIfDistanceToSeenLowerThan.is_set())
		{
			if (_spawnManager->inRoom.get() &&
				AVOID_CALLING_ACK_ _spawnManager->inRoom->get_distance_to_recently_seen_by_player() < wave->endIfDistanceToSeenLowerThan.get())
			{
				endThisOne = true;
			}
		}
		if (wave->endIfDistanceToSeenAtLeast.is_set())
		{
			if (_spawnManager->inRoom.get() &&
				AVOID_CALLING_ACK_ _spawnManager->inRoom->get_distance_to_recently_seen_by_player() >= wave->endIfDistanceToSeenAtLeast.get())
			{
				endThisOne = true;
			}
		}
		if (!wave->endIfStoryFacts.is_empty())
		{
			if (auto* gd = GameDirector::get_active())
			{
				Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
				if (wave->endIfStoryFacts.check(gd->access_story_execution().get_facts()))
				{
					endThisOne = true;
				}
			}
		}
		if (!wave->infiniteCount && endAtCount.is_set() && countNowAndLeft <= endAtCount.get())
		{
			endThisOne = true;
		}
		if (!wave->infiniteCount && endAtUsefulCount.is_set())
		{
			int countUseful = 0;
			for_every(s, spawned)
			{
				if (auto* imo = s->spawned.get())
				{
					bool usefulForCombat = true;
					if (imo->get_variables().get_value<bool>(NAME(unableToAttack), false))
					{
						usefulForCombat = false;
					}
					if (usefulForCombat)
					{
						++countUseful;
					}
				}
			}
			int countUsefulNowAndLeft = countLeft + countUseful;
			if (countUsefulNowAndLeft <= endAtUsefulCount.get())
			{
				endThisOne = true;
			}
		}
		if (!wave->infiniteCount)
		{
			if (endAtCountPt.is_set() && maxTotalCount > 0)
			{
				float countPt = (float)countNowAndLeft / (float)maxTotalCount;
				if (countPt <= min(0.9999f, endAtCountPt.get()))
				{
					endThisOne = true;
				}
			}
		}
		if (spawnedNowCount > 0 && spawnBelowCountPt.is_set())
		{
			float countPt = (float)count / (float)spawnedNowCount;
			if (countPt <= min(0.9999f, spawnBelowCountPt.get()))
			{
				spawnRequested = true;
			}
		}
		if (spawned.get_size() == 0 && countLeft > 0)
		{
#ifdef LOG_MORE
			ai_log_colour(_spawnManager, Colour::magenta);
			ai_log(_spawnManager, TXT("DIDN'T SPAWN BUT TO SPAWN LEFT! request spawn"));
			ai_log_no_colour(_spawnManager);
#endif
			// no spawned? spawn them!
			spawnRequested = true;
		}
		if (!spawning && countLeft == 0)
		{
			if (wave->endWhenSpawned)
			{
				endThisOne = true;
			}
		}
	}

	if (state < Ended &&
		! endRequested &&
		! spawning)
	{
		if (wave->distanceToCease.is_set() && _spawnManager->inRoom.get() &&
			AVOID_CALLING_ACK_ _spawnManager->inRoom->get_distance_to_recently_seen_by_player() >= wave->distanceToCease.get())
		{
			ceaseThisOne = true;
		}
	}

	if (endThisOne)
	{
		endRequested = true;
	}

	if (ceaseThisOne)
	{
		ceaseRequested = true;
	}

	// force wait if game director requests "safe space"
	if (safeSpaceWait && _spawnManager->disabledByGameDirector)
	{
		endThisOne = false;
		endRequested = false;
		ceaseThisOne = false;
		ceaseRequested = false;
	}

	bool endOtherWavesNow = false;
	SpawnManagerWaveInstance* endOtherWavesNowWithWave = nullptr;

	if (ceaseRequested &&
		state != Ended)
	{
		state = Ended;
		SpawnManagerWaveInstance* nextWave = nullptr;
		if (!wave->nextOnCease.is_empty())
		{
			int idx = _spawnManager->random.get_int(wave->nextOnCease.get_size());
			auto* newWave = _spawnManager->start_wave(wave->next[idx].name);
			assign_if_not_assigned(nextWave, newWave);
		}
		if (nextWave)
		{
			for_every(si, spawned)
			{
				if (auto* o = si->spawned.get())
				{
					nextWave->add(_spawnManager, *si);
				}
			}
			spawned.clear();
		}
		// cease all spawned with us, unless they are in a visible room or shouldn't be ceased
		for_every(si, spawned)
		{
			if (auto* imo = si->spawned.get())
			{
				if (imo->get_presence() &&
					imo->get_presence()->get_in_room())
				{
					if (!imo->was_recently_seen_by_player() &&
						!imo->is_game_related_system_flag_set(ModulesOwnerFlags::BlockSpawnManagerAutoCease))
					{
						imo->cease_to_exist(false);
					}
				}
			}
		}
		endOtherWavesNow = true;
		endOtherWavesNowWithWave = nextWave;
	}

	if (endRequested &&
		state != Ended)
	{
		ai_log(_spawnManager, TXT("end requested %S"), endRequestedWithoutNext? TXT("without next") : TXT("with next"));
		state = Ended;
		SpawnManagerWaveInstance* nextWave = nullptr;
		if (!endRequestedWithoutNext)
		{
			if (wave->chooseOneNext)
			{
				if (!wave->next.is_empty())
				{
					int idx = RandomUtils::ChooseFromContainer<Array<SpawnManagerWave::WaveWithChance>, SpawnManagerWave::WaveWithChance>::choose(_spawnManager->random, wave->next,
						[](SpawnManagerWave::WaveWithChance const& _element)
						{
							return _element.probCoef;
						});
					auto* newWave = _spawnManager->start_wave(wave->next[idx].name);
					assign_if_not_assigned(nextWave, newWave);
				}
			}
			else
			{
				for_every(wn, wave->next)
				{
					auto* newWave = _spawnManager->start_wave(wn->name);
					assign_if_not_assigned(nextWave, newWave);
				}
			}
			if (nextWave)
			{
				for_every(si, spawned)
				{
					if (auto* o = si->spawned.get())
					{
						nextWave->add(_spawnManager, *si);
					}
				}
				spawned.clear();
			}
		}
		endOtherWavesNow = true;
		endOtherWavesNowWithWave = nextWave;
	}

	if (endOtherWavesNow)
	{
		// end other waves (if not ended already)
		for_every(end, wave->endWithMe)
		{
			for_every_ref(wave, _spawnManager->currentWaves)
			{
				if (wave->wave->name == *end &&
					!wave->endRequested)
				{
					wave->endRequested = true;
					wave->endRequestedWithoutNext = true;
					if (endOtherWavesNowWithWave)
					{
						for_every(si, wave->spawned)
						{
							if (auto* o = si->spawned.get())
							{
								endOtherWavesNowWithWave->add(_spawnManager, *si);
							}
						}
						wave->spawned.clear();
					}
				}
			}
		}
	}

	return state != Ended;
}

void SpawnManagerWaveInstance::add_one_extra_being_spawned(SpawnManagerSpawnInfo const * _si)
{
	// we don't count beingSpawned
}

void SpawnManagerWaveInstance::restore_one_to_spawn(SpawnManagerSpawnInfo const * _si)
{
	if (_si)
	{
		if (auto* wei = _si->waveElementInstance.get())
		{
			if (!wei->element->mayBeSkipped)
			{
				++toSpawnLeft;
				++wei->countLeft;
			}
		}
	}
}

void SpawnManagerWaveInstance::update_spawned(SpawnManager* _spawnManager)
{
	for (int i = 0; i < spawned.get_size(); ++i)
	{
		auto& sp = spawned[i];
		if (sp.update_unable_to_attack())
		{
			sp.on_killed(_spawnManager); // won't be used any more, treat as killed
		}
		if (!sp.spawned.get())
		{
			ai_log_colour(_spawnManager, Colour::yellow);
#ifdef AN_DEVELOPMENT_OR_PROFILER
			ai_log(_spawnManager, TXT("- lost one \"%S\""), sp.name.to_char());
#ifdef AN_OUTPUT_SPAWN
			output(TXT("[spawn manager] [wave] lost one \"%S\""), sp.name.to_char());
#endif
#else
			ai_log(_spawnManager, TXT("- lost one"));
#ifdef AN_OUTPUT_SPAWN
			output(TXT("[spawn manager] [wave] lost one"));
#endif
#endif
			ai_log_no_colour(_spawnManager);
			sp.on_killed(_spawnManager);
			spawned.remove_fast_at(i);
			--i;
		}
	}
	for_every_ref(element, elements)
	{
		element->update_spawned();
	}
}

Spawned & SpawnManagerWaveInstance::add(SpawnManager* _spawnManager, Framework::IModulesOwner* _imo, Framework::WorldAddress const& _spawnWorldAddress, int _spawnPOIIdx, SpawnManagerWaveElement* _byWaveElement)
{
	Spawned s(_imo, _byWaveElement);
	s.spawnWorldAddress = _spawnWorldAddress;
	s.spawnPoiIdx = _spawnPOIIdx;
	spawned.push_back(s);
	return spawned.get_last();
}

Spawned & SpawnManagerWaveInstance::add(SpawnManager* _spawnManager, Spawned const & _s)
{
	spawned.push_back(_s);
	return spawned.get_last();
}

//

void SpawnManagerWaveElementInstance::clean_up()
{
	element.clear();
	wave.clear();

	spawned.clear();
}

Framework::ObjectType* SpawnManagerWaveElementInstance::get_type_to_spawn(SpawnManager* _spawnManager, OUT_ int& _count, OUT_ Vector3& _spawnOffset) const
{
	{	// default values
		_count = 1;
		_spawnOffset = Vector3(0.0f, 0.0f, 0.2f);
	}
	auto* ot = find_type_to_spawn(element->spawnSet, element->types, _spawnManager, OUT_ _count, OUT_ _spawnOffset,
		[this](Framework::ObjectType* _ofType, int _any)
		{
			if (_any)
			{
				return spawned.get_size();
			}
			else
			{
				int count = 0;
				for_every_ref(s, spawned)
				{
					if (auto* o = fast_cast<Framework::Object>(s))
					{
						if (o->get_object_type() == _ofType)
						{
							++count;
						}
					}
				}
				return count;
			}
			return 0;
		});
	{	// override with provided
		if (!element->spawnBatch.is_empty())
		{
			_count = element->spawnBatch.get(_spawnManager->access_random());
		}
		if (element->spawnBatchOffset.is_set())
		{
			_spawnOffset = element->spawnBatchOffset.get();
		}
	}
	return ot;
}

Framework::ObjectType * SpawnManagerWaveElementInstance::get_travel_with_type(SpawnManager * _spawnManager) const
{
	return element->travelWithType.get();
}
	
void SpawnManagerWaveElementInstance::update_spawned()
{
	for (int i = 0; i < spawned.get_size(); ++i)
	{
		if (!spawned[i].get())
		{
			spawned.remove_fast_at(i);
			--i;
		}
	}
}

void SpawnManagerWaveElementInstance::on_spawned(SpawnManager* _spawnManager, Framework::IModulesOwner* _imo, Framework::WorldAddress const& _spawnWorldAddress, int _spawnPOIIdx)
{
	spawned.push_back(SafePtr<Framework::IModulesOwner>(_imo));
	auto& nsi = wave->add(_spawnManager, _imo, _spawnWorldAddress, _spawnPOIIdx, element.get());

	if (auto* we = element.get())
	{
		nsi.poolName = we->regionPoolConditions.useRegionPool;
		if (nsi.poolName.is_valid() && _spawnManager && _spawnManager->regionManager.get())
		{
			_spawnManager->reserve_pool(we->regionPoolConditions.useRegionPool, nsi.poolValue);
		}
	}

	OnSpawnHunches osh;
	if (_spawnManager &&
		_spawnManager->spawnManagerData)
	{
		osh.override_with(_spawnManager->spawnManagerData->onSpawnHunches);
	}
	if (wave.get() && wave->wave.get())
	{
		osh.override_with(wave->wave->onSpawnHunches);
	}
	if (element.get())
	{
		osh.override_with(element->onSpawnHunches);
	}
	give_hunch_to_player(_imo, _spawnManager->access_random(), osh, _spawnManager->inRegion.get(), _spawnManager->inRoom.get());
}

//

void DynamicSpawnInfoInstance::clean_up()
{
	dynamicSpawnInfo.clear();
	spawned.clear();
}

void DynamicSpawnInfoInstance::init(SpawnManager* _spawnManager)
{
	concurrentLimitMax = NONE;
	auto* dsi = dynamicSpawnInfo.get();
	if (!dsi->limit.is_empty()) limit = _spawnManager->random.get(dsi->limit);
	if (!dsi->concurrentLimit.is_empty()) concurrentLimitMax = apply_spawn_count_modifier(_spawnManager->regionManager.get(), dsi->regionPoolConditions.useRegionPool, _spawnManager->random.get(dsi->concurrentLimit));

	spawnPointBlockTime = _spawnManager->spawnManagerData->get_spawn_point_block_time();
	usePOIs = &_spawnManager->spawnManagerData->spawnPOIs;
	spawnPointBlockTime = dsi->spawnPointBlockTime.get(spawnPointBlockTime);
	if (!dsi->spawnPOIs.is_empty())
	{
		usePOIs = &dsi->spawnPOIs;
	}
	regionPoolConditionsInstance.init(_spawnManager, dsi->regionPoolConditions);

	active = true;
	if (dsi->activateAfter.max > 0.0f)
	{
		active = false;
		timeToActivate = Random::get(dsi->activateAfter);
	}

	concurrentLimit = concurrentLimitMax != NONE? concurrentLimitMax :NONE;
	globalConcurrentCountId = dsi->globalConcurrentCountId;
	left = limit != NONE? limit :NONE;

	hasAnythingToSpawn = has_anything_to_spawn(dsi->spawnSet, dsi->types, _spawnManager);

	distanceToSpawnOffset = 0;
	if (auto* r = _spawnManager->inRoom.get())
	{
		distanceToSpawnOffset += r->get_value<int>(NAME(spawnManager_distanceToSpawnOffset), 0);
	}
	if (auto* r = _spawnManager->inRegion.get())
	{
		distanceToSpawnOffset += r->get_value<int>(NAME(spawnManager_distanceToSpawnOffset), 0);
	}
}

void DynamicSpawnInfoInstance::update(SpawnManager* _spawnManager, float _deltaTime)
{
	if (!hasAnythingToSpawn)
	{
		return;
	}

	if (_spawnManager->disabledByGameDirector)
	{
		toSpawn = 0;
		return;
	}
	if (_spawnManager->wasDisabledByGameDirector)
	{
		// restart
		active = false;
		if (dynamicSpawnInfo.get())
		{
			toSpawnWait = Random::get(dynamicSpawnInfo->spawnInterval);
			if (dynamicSpawnInfo->activateAfter.max > 0.0f)
			{
				timeToActivate = Random::get(dynamicSpawnInfo->activateAfter);
			}
			else
			{
				timeToActivate = 0.0f;
			}
		}
	}

	update_spawned(_spawnManager);

	if (!active)
	{
		timeToActivate = max(0.0f, timeToActivate - _deltaTime);
		if (timeToActivate <= 0.0f)
		{
			active = true;
		}
		return;
	}

	toSpawnWait = max(0.0f, toSpawnWait - _deltaTime);
	
	if (_spawnManager->spawnManagerData->spawnIfInRoomSeenByPlayer)
	{
		if (auto* r = _spawnManager->inRoom.get())
		{
			if (!r->was_recently_seen_by_player())
			{
				toSpawnWait = max(toSpawnWait, 0.1f);
			}
		}
	}
	if (_spawnManager->spawnManagerData->spawnIfPlayerInSameRoom)
	{
		if (auto* r = _spawnManager->inRoom.get())
		{
			if (!r->was_recently_visited_by_player())
			{
				toSpawnWait = max(toSpawnWait, 0.1f);
			}
		}
	}

	if (toSpawnWait == 0.0f && beingSpawned == 0)
	{
		an_assert(dynamicSpawnInfo.is_set());

		toSpawn = 0;
		todo_note(TXT("have general concurrent limit?"));
		concurrentLimit = apply_spawn_count_modifier(_spawnManager->regionManager.get(), dynamicSpawnInfo->regionPoolConditions.useRegionPool, dynamicSpawnInfo->regionPoolConditions.get_altered_limit(_spawnManager, concurrentLimitMax));
		int spawnedSoFar = spawned.get_size();
		if (globalConcurrentCountId.is_valid())
		{
			spawnedSoFar = GlobalSpawnManagerInfo::get(globalConcurrentCountId);
		}
		toSpawn = max(toSpawn, concurrentLimit - spawnedSoFar);
		if (dynamicSpawnInfo->regionPoolConditions.useRegionPool.is_valid())
		{
			if (dynamicSpawnInfo->regionPoolConditions.check(_spawnManager, regionPoolConditionsInstance))
			{
				toSpawn = min(toSpawn, 1); // for conditions, one by one
			}
			else
			{
				toSpawn = 0;
			}
		}
		else if (left >= 0)
		{
			toSpawn = min(toSpawn, left);
		}
	}
}

bool DynamicSpawnInfoInstance::spawn(SpawnManager* _spawnManager)
{
	bool result = false;
	if (toSpawn > 0)
	{
		noPlaceToSpawn = false;
#ifdef LOG_MORE
		ai_log(_spawnManager, TXT("dynamic spawn info wants to spawn %i"), toSpawn);
#endif

		Array<SpawnManagerSpawnInfo> spawnInfos;

		RangeInt distanceToRecentlySeenByPlayer(max(1, dynamicSpawnInfo->distanceToSpawn + distanceToSpawnOffset), dynamicSpawnInfo->distanceToCease - 1);

		Random::Generator rg = _spawnManager->access_random().spawn();

		while (toSpawn > 0)
		{
			// gather pois
#ifdef SPAWN_MANAGER__USE_ARRAYS_INSTEAD_OF_STACK_ARRAYS
			Array<SpawnChoosePOI> choosePOIs;
#else
			ARRAY_STACK(SpawnChoosePOI, choosePOIs, max(1, _spawnManager->spawnPointsAvailable.get_size()));
#endif
			bool allowInaccessible = _spawnManager->spawnManagerData->allowInaccessible.get(rg.get_chance(0.4f));
			_spawnManager->gather_POIs(choosePOIs, usePOIs, rg, dynamicSpawnInfo->placementCriteria, distanceToRecentlySeenByPlayer, allowInaccessible);

#ifdef LOG_MORE
			ai_log(_spawnManager, TXT(" left to spawn %i, choose from %i (poi)"), toSpawn, choosePOIs.get_size());
#endif

			if (choosePOIs.is_empty())
			{
				noPlaceToSpawn = true;
				toSpawnWait = Random::get(dynamicSpawnInfo->spawnInterval); // try later
				break;
			}

			if (!choosePOIs.is_empty())
			{
				// sort them to have best match
				sort(choosePOIs);

				auto& choosePOI = choosePOIs[0];
				auto* atPOI = _spawnManager->acquire_available_spawn_point(choosePOI.idx, *this);

				// add spawn info
				SpawnManagerSpawnInfo si;
				si.atPOI = atPOI;
				si.hostile = dynamicSpawnInfo->hostile.get(_spawnManager->spawnManagerData->hostile.get(dynamicSpawnInfo->regionPoolConditions.is_hostile(_spawnManager)));
				si.dynamicInstance = this;
				spawnInfos.push_back(si);
			}

			// less to spawn!
			toSpawn--;

			if (dynamicSpawnInfo->differentSpawnBatchSize > 0 && spawnInfos.get_size() >= dynamicSpawnInfo->differentSpawnBatchSize)
			{
				break;
			}
		}

		if (!spawnInfos.is_empty())
		{
			ai_log(_spawnManager, TXT("queue spawning %i object(s)"), spawnInfos.get_size());
			beingSpawned += spawnInfos.get_size();
			Random::Generator rgBase = _spawnManager->random.spawn();
			SafePtr<Framework::IModulesOwner> safeImo(_spawnManager->get_mind()->get_owner_as_modules_owner());
			Game::get()->add_async_world_job(TXT("spawn bunch via spawn manager"),
				[spawnInfos, rgBase, safeImo, _spawnManager]() // this is just to set spawning to false (if object does still exist), spawnInfo not as reference, we need a copy
				{
					MEASURE_PERFORMANCE(spawnBunchViaSpawnManager);

					if (safeImo.is_set() && // should catch if owner was deleted
						_spawnManager)
					{
						_spawnManager->spawn_infos_async(spawnInfos, rgBase);
					}
				});
			result = true;
		}

		toSpawnWait = max(Random::get(dynamicSpawnInfo->spawnInterval), toSpawnWait);
	}

	return result;
}

void DynamicSpawnInfoInstance::add_one_extra_being_spawned(SpawnManagerSpawnInfo const* _si)
{
	++beingSpawned;
}

void DynamicSpawnInfoInstance::restore_one_to_spawn(SpawnManagerSpawnInfo const* _si)
{
	if (left >= 0)
	{
		++left;
	}
	an_assert(beingSpawned > 0);
	--beingSpawned;
}

void DynamicSpawnInfoInstance::update_spawned(SpawnManager* _spawnManager)
{
	for (int i = 0; i < spawned.get_size(); ++i)
	{
		auto& sp = spawned[i];
		if (sp.update_unable_to_attack())
		{
			sp.on_killed(_spawnManager); // won't be used any more, treat as killed
		}
		if (auto* imo = sp.spawned.get())
		{
			if (auto* imoObj = fast_cast<Framework::Object>(imo))
			{
				if (imoObj->is_world_active() // do not cease something that hasn't activated yet
					&& !imoObj->is_game_related_system_flag_set(ModulesOwnerFlags::BlockSpawnManagerAutoCease))
				{
					if (imo->get_room_distance_to_recently_seen_by_player() >= dynamicSpawnInfo->distanceToCease)
					{
						ai_log_colour(_spawnManager, Colour::red);
#ifdef AN_DEVELOPMENT_OR_PROFILER
						ai_log(_spawnManager, TXT("- too far, cease \"%S\""), sp.name.to_char());
#ifdef AN_OUTPUT_SPAWN
						output(TXT("[spawn manager] [dyn] too far, cease \"%S\""), sp.name.to_char());
#endif
#else
						ai_log(_spawnManager, TXT("- too far, cease"));
#ifdef AN_OUTPUT_SPAWN
						output(TXT("[spawn manager] [dyn] too far, cease"));
#endif
#endif
						ai_log_no_colour(_spawnManager);

						imo->cease_to_exist(false);
						spawned.remove_fast_at(i);
						if (globalConcurrentCountId.is_valid())
						{
							GlobalSpawnManagerInfo::decrease(globalConcurrentCountId);
						}
						--i;
					}
					else if (imo->get_room_distance_to_recently_seen_by_player() > 1)
					{
						// remove objects that are not visible if pool got deactivated
						if (dynamicSpawnInfo->regionPoolConditions.useRegionPool.is_valid() && _spawnManager && _spawnManager->regionManager.get())
						{
							if (!_spawnManager->regionManager->is_pool_active(dynamicSpawnInfo->regionPoolConditions.useRegionPool))
							{

								ai_log_colour(_spawnManager, Colour::red);
#ifdef AN_DEVELOPMENT_OR_PROFILER
								ai_log(_spawnManager, TXT("- pool deactivated, cease \"%S\""), sp.name.to_char());
#ifdef AN_OUTPUT_SPAWN
								output(TXT("[spawn manager] [dyn] pool deactivated, cease \"%S\""), sp.name.to_char());
#endif
#else
								ai_log(_spawnManager, TXT("- pool deactivated, cease"));
#ifdef AN_OUTPUT_SPAWN
								output(TXT("[spawn manager] [dyn] pool deactivated, cease"));
#endif
#endif
								ai_log_no_colour(_spawnManager);

								imo->cease_to_exist(false);
								spawned.remove_fast_at(i);
								if (globalConcurrentCountId.is_valid())
								{
									GlobalSpawnManagerInfo::decrease(globalConcurrentCountId);
								}
								--i;
							}
						}
					}
				}
			}
		}
		else
		{
			ai_log_colour(_spawnManager, Colour::yellow);
#ifdef AN_DEVELOPMENT_OR_PROFILER
			ai_log(_spawnManager, TXT("- lost one \"%S\""), sp.name.to_char());
#ifdef AN_OUTPUT_SPAWN
			output(TXT("[spawn manager] [dyn] lost one \"%S\""), sp.name.to_char());
#endif
#else
			ai_log(_spawnManager, TXT("- lost one"));
#ifdef AN_OUTPUT_SPAWN
			output(TXT("[spawn manager] [dyn] lost one"));
#endif
#endif
			ai_log_no_colour(_spawnManager);
			sp.on_killed(_spawnManager);
			spawned.remove_fast_at(i);
			if (globalConcurrentCountId.is_valid())
			{
				GlobalSpawnManagerInfo::decrease(globalConcurrentCountId);
			}
			--i;
			// use greater interval
			toSpawnWait = max(toSpawnWait, Random::get(dynamicSpawnInfo->spawnIntervalOnDeath));
		}
	}
}


void DynamicSpawnInfoInstance::on_spawned(SpawnManager* _spawnManager, Framework::IModulesOwner* _imo, Framework::WorldAddress const& _spawnWorldAddress, int _spawnPOIIdx)
{
	_imo->make_advancement_not_suspendable(); // no, we will decide if we want to cease it or not
	Spawned s(_imo, nullptr /* no wave element for dynamic spawn */);
	if (dynamicSpawnInfo.get())
	{
		s.poolName = dynamicSpawnInfo->regionPoolConditions.useRegionPool;
	}
	spawned.push_back(s);
	if (globalConcurrentCountId.is_valid())
	{
		GlobalSpawnManagerInfo::increase(globalConcurrentCountId);
	}
	an_assert(beingSpawned > 0);
	--beingSpawned;

	OnSpawnHunches osh;
	if (_spawnManager &&
		_spawnManager->spawnManagerData)
	{
		osh.override_with(_spawnManager->spawnManagerData->onSpawnHunches);
	}
	if (dynamicSpawnInfo.get())
	{
		osh.override_with(dynamicSpawnInfo->onSpawnHunches);
	}
	give_hunch_to_player(_imo, _spawnManager->access_random(), osh, _spawnManager->inRegion.get(), _spawnManager->inRoom.get());
}

void DynamicSpawnInfoInstance::sort_by_priority(Array<RefCountObjectPtr<DynamicSpawnInfoInstance>>& _array)
{
	for_every_ref(dsii, _array)
	{
		dsii->sortRandomness = Random::get_int(100);
	}
	sort(_array, compare_ref_priority);
}

int DynamicSpawnInfoInstance::compare_ref_priority(void const* _a, void const* _b)
{
	RefCountObjectPtr<DynamicSpawnInfoInstance> const* a = plain_cast<RefCountObjectPtr<DynamicSpawnInfoInstance>>(_a);
	RefCountObjectPtr<DynamicSpawnInfoInstance> const* b = plain_cast<RefCountObjectPtr<DynamicSpawnInfoInstance>>(_b);
	if ((*a)->dynamicSpawnInfo->priority > (*b)->dynamicSpawnInfo->priority) return A_BEFORE_B;
	if ((*a)->dynamicSpawnInfo->priority < (*b)->dynamicSpawnInfo->priority) return B_BEFORE_A;
	if ((*a)->sortRandomness > (*b)->sortRandomness) return A_BEFORE_B;
	if ((*a)->sortRandomness < (*b)->sortRandomness) return B_BEFORE_A;
	return String::compare_tchar_icase_sort((*a)->dynamicSpawnInfo->name.to_char(), (*b)->dynamicSpawnInfo->name.to_char());
}

Framework::ObjectType* DynamicSpawnInfoInstance::get_type_to_spawn(SpawnManager* _spawnManager, OUT_ int& _count, OUT_ Vector3& _spawnOffset) const
{
	{	// default values
		_count = 1;
		_spawnOffset = Vector3(0.0f, 0.0f, 0.2f);
	}
	auto* ot = find_type_to_spawn(dynamicSpawnInfo->spawnSet, dynamicSpawnInfo->types, _spawnManager, OUT_ _count, OUT_ _spawnOffset,
		[this](Framework::ObjectType* _ofType, int _any)
		{
			if (_any)
			{
				return spawned.get_size();
			}
			else
			{
				int count = 0;
				for_every(s, spawned)
				{
					if (auto* o = fast_cast<Framework::Object>(s->spawned.get()))
					{
						if (o->get_object_type() == _ofType)
						{
							++count;
						}
					}
				}
				return count;
			}
			return 0;
		});
	{	// override with provided
		if (!dynamicSpawnInfo->spawnBatch.is_empty())
		{
			_count = dynamicSpawnInfo->spawnBatch.get(_spawnManager->access_random());
		}
		if (dynamicSpawnInfo->spawnBatchOffset.is_set())
		{
			_spawnOffset = dynamicSpawnInfo->spawnBatchOffset.get();
		}
	}
	return ot;
}

Framework::ObjectType* DynamicSpawnInfoInstance::get_travel_with_type(SpawnManager* _spawnManager) const
{
	return dynamicSpawnInfo->travelWithType.get();
}

//

bool DynamicSpawnInfo::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	warn_loading_xml_on_assert(!_node->get_attribute(TXT("id")), _node, TXT("use \"name\" instead of \"id\" - deprecated"));

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	hostile.load_from_xml(_node, TXT("hostile"));
	priority = _node->get_float_attribute(TXT("priority"), priority);

	spawnSet = _node->get_name_attribute_or_from_child(TXT("spawnSet"), spawnSet);

	result &= onSpawnHunches.load_from_xml(_node, _lc);

	if (_node->has_attribute(TXT("type")))
	{
		Framework::UsedLibraryStored<Framework::ObjectType> type;
		if (type.load_from_xml(_node, TXT("type"), _lc))
		{
			types.push_back(type);
		}
	}
	for_every(node, _node->children_named(TXT("type")))
	{
		Framework::UsedLibraryStored<Framework::ObjectType> type;
		if (type.load_from_xml(node, TXT("type"), _lc))
		{
			types.push_back(type);
		}
	}

	result &= travelWithType.load_from_xml(_node, TXT("travelWith"), _lc);

	for_every(node, _node->children_named(TXT("poi")))
	{
		SpawnManagerPOIDefinition poi;
		if (poi.load_from_xml(node))
		{
			spawnPOIs.push_back(poi);
		}
		else
		{
			error_loading_xml(node, TXT("problem loading poi for spawn manager"));
			result = false;
		}
	}
	spawnPointBlockTime.load_from_xml(_node, TXT("spawnPointBlockTime"));
	parameters.load_from_xml_child_node(_node, TXT("parameters"));

	for_every(node, _node->children_named(TXT("placementCriteria")))
	{
		result &= placementCriteria.load_from_xml(node, _lc);
	}

	{
		spawnBatch.load_from_xml(_node, TXT("spawnBatch"));
		spawnBatchOffset.load_from_xml(_node, TXT("spawnBatchOffset"));
		for_every(n, _node->children_named(TXT("spawnBatch")))
		{
			spawnBatchOffset.load_from_xml(n, TXT("offset"));
		}
	}

	differentSpawnBatchSize = _node->get_int_attribute_or_from_child(TXT("differentSpawnBatchSize"), differentSpawnBatchSize);
	distanceToSpawn = _node->get_int_attribute_or_from_child(TXT("distanceToSpawn"), distanceToSpawn);
	distanceToCease = _node->get_int_attribute_or_from_child(TXT("distanceToCease"), distanceToCease);
	error_loading_xml_on_assert(distanceToCease > distanceToSpawn, result, _node, TXT("distanceToCease should be greater than distanceToSpawn"));

	allowOthersToSpawnIfCantSpawn = _node->get_bool_attribute_or_from_child_presence(TXT("allowOthersToSpawnIfCantSpawn"), allowOthersToSpawnIfCantSpawn);
	allowOthersToSpawnIfCantSpawn = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowOthersToSpawnIfCantSpawn"), ! allowOthersToSpawnIfCantSpawn);

	result &= regionPoolConditions.load_from_xml(_node, _lc);

	limit.load_from_xml(_node, TXT("limit"));
	concurrentLimit.load_from_xml(_node, TXT("concurrentLimit"));
	globalConcurrentCountId = _node->get_name_attribute_or_from_child(TXT("globalConcurrentCountId"), globalConcurrentCountId);
	//
	error_loading_xml_on_assert(!limit.is_empty() || !concurrentLimit.is_empty(), result, _node, TXT("there should be either limit or concurrent limit provided"));

	spawnInterval.load_from_xml(_node, TXT("spawnInterval"));
	spawnIntervalOnDeath.load_from_xml(_node, TXT("spawnIntervalOnDeath"));

	activateAfter.load_from_xml(_node, TXT("activateAfter"));

	return result;
}

bool DynamicSpawnInfo::prepare_for_game(Array<DynamicSpawnInfoPtr> const & _dynamicSpawns, ::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		for_every(type, types)
		{
			type->set(get_object_type(_library, type->get_name()));
			if (!type->get())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!Framework::Library::may_ignore_missing_references())
#endif
				{
					error(TXT("not found \"%S\""), type->get_name().to_string().to_char());
					result = false;
				}
			}
		}
		if (travelWithType.is_name_valid())
		{
			travelWithType.set(get_object_type(_library, travelWithType.get_name()));
			if (!travelWithType.get())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!Framework::Library::may_ignore_missing_references())
#endif
				{
					error(TXT("not found \"%S\""), travelWithType.get_name().to_string().to_char());
					result = false;
				}
			}
		}
	}

	return result;
}

DynamicSpawnInfoInstance* DynamicSpawnInfo::instantiate(SpawnManager* _spawnManager) const
{
	DynamicSpawnInfoInstance* instance = new DynamicSpawnInfoInstance();

	instance->dynamicSpawnInfo = cast_to_nonconst(this);

	instance->init(_spawnManager);

	return instance;
}

//

bool OnSpawnHunches::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("onSpawnHunches")))
	{
		investigateChance.load_from_xml(node, TXT("investigateChance"));
		forceInvestigate.load_from_xml(node, TXT("forceInvestigate"));
		wanderChance.load_from_xml(node, TXT("wanderChance"));
	}

	return result;
}

//

bool Spawned::update_unable_to_attack()
{
	if (!unableToAttack)
	{
		if (auto* imo = spawned.get())
		{
			if (imo->get_variables().get_value<bool>(NAME(unableToAttack), false))
			{
				unableToAttack = true;
				return true;
			}
		}
	}
	return false;
}

void Spawned::on_killed(SpawnManager* _spawnManager)
{
	if (killed)
	{
		return;
	}
	
	if (auto* we = byWaveElement.get())
	{
		if (we->storeExistenceInPilgrimage &&
			spawnPoiIdx != NONE)
		{
			if (auto* pi = PilgrimageInstance::get())
			{
				pi->mark_killed(spawnWorldAddress, spawnPoiIdx);
			}
		}
	}

	if (poolName.is_valid() && _spawnManager && _spawnManager->regionManager.get())
	{
		_spawnManager->regionManager->consume_for_pool(poolName, poolValue); // consume only on kill
		_spawnManager->free_pool(poolName, poolValue);
	}

	killed = true;
}
