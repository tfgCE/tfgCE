#include "pilgrimageInstance.h"

#include "pilgrimage.h"
#include "pilgrimageEnvironment.h"
#include "pilgrimagePart.h"
#include "pilgrimageInstanceObserver.h"

#include "..\game\damage.h"
#include "..\game\game.h"
#include "..\game\gameDirector.h"
#include "..\game\gameSettings.h"
#include "..\game\gameState.h"
#include "..\library\library.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\custom\health\mc_health.h"
#include "..\roomGenerators\roomGenerationInfo.h"

#include "..\..\framework\game\delayedObjectCreation.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\world\doorInRoom.h"
#include "..\..\framework\world\environment.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\faultHandler.h"

// demo
#include "..\..\framework\module\moduleMovementPath.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\core\pieceCombiner\pieceCombinerDebugVisualiser.h"
#endif

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifdef AN_OUTPUT
		#define AN_OUTPUT_DEV_INFO
		#define AN_IMPORTANT_INFO

		#ifdef AN_ALLOW_EXTENSIVE_LOGS
			#define AN_OUTPUT_PILGRIMAGE_GENERATION
		#endif

		//#define DEBUG_KILLED_INFOS
	#endif

	#ifdef AN_OUTPUT_WORLD_GENERATION
		//#define OUTPUT_GENERATION_DEBUG_VISUALISER
	#endif
#endif

//

// pois
DEFINE_STATIC_NAME_STR(vrAnchor, TXT("vr anchor"));

// poi parameters
DEFINE_STATIC_NAME(vrAnchorId);

// room variables
DEFINE_STATIC_NAME(handleDoorOperation); // if set to true, pilgrimage won't be opening and closing doors
DEFINE_STATIC_NAME(handleEntranceDoorOperation); // if set to true, pilgrimage won't be opening and closing doors
DEFINE_STATIC_NAME(handleEntranceDoorAtLeastIn); // if is inside, at least this value, will close door
DEFINE_STATIC_NAME(allowEntranceDoorAuto); // if set to false, won't allow auto stay open, will force to stay open

// utility room variables
DEFINE_STATIC_NAME(forPilgrimagesMainRegion);
DEFINE_STATIC_NAME(forPilgrimagesMainRegionToBe);

// door in room tags
DEFINE_STATIC_NAME(exitDoor);
DEFINE_STATIC_NAME(entranceDoor);

// environment params
DEFINE_STATIC_NAME(lightAt);
DEFINE_STATIC_NAME(lightDir);
DEFINE_STATIC_NAME(lightSpecularSide);
DEFINE_STATIC_NAME(starsPt);
DEFINE_STATIC_NAME(starsMatrix);

// game state
DEFINE_STATIC_NAME(randomSeed);
DEFINE_STATIC_NAME(environmentCycleIdx);
DEFINE_STATIC_NAME(environmentCyclePt);
DEFINE_STATIC_NAME(environmentRotationYaw);
DEFINE_STATIC_NAME(lockEnvironmentCycleUntilSeen);
DEFINE_STATIC_NAME(lockEnvironmentCycleUntilSeenRoomTagged);
DEFINE_STATIC_NAME(upgradeMachinesUnlocked);

// bullshot
DEFINE_STATIC_NAME(bullshotEnvironmentCycleIdx);
DEFINE_STATIC_NAME(bullshotEnvironmentCyclePt);
DEFINE_STATIC_NAME(bullshotNoonYaw);
DEFINE_STATIC_NAME(bullshotNoonPitch);

//

using namespace TeaForGodEmperor;

//

struct PilgrimageDataMaintainer
{
	static const int LIMIT = 3;
	Concurrency::SpinLock usedDataLock;
	Array<Framework::UsedLibraryStored<Pilgrimage>> usedData;

	void use(Pilgrimage* _pilgrimage)
	{
		ASSERT_ASYNC;

		bool found = false;
		{
			Concurrency::ScopedSpinLock lock(usedDataLock);
			for_every_ref(u, usedData)
			{
				if (u == _pilgrimage)
				{
					output(TXT("[PilgrimageDataMaintainer] pushed \"%S\" to front"), _pilgrimage->get_name().to_string().to_char());
					usedData.remove_at(for_everys_index(u));
					usedData.push_front(Framework::UsedLibraryStored<Pilgrimage>(_pilgrimage));
					found = true;
					break;
				}
			}
		}
		if (!found)
		{
			output(TXT("[PilgrimageDataMaintainer] adding \"%S\""), _pilgrimage->get_name().to_string().to_char());
			{
				Concurrency::ScopedSpinLock lock(usedDataLock);
				usedData.push_front(Framework::UsedLibraryStored<Pilgrimage>(_pilgrimage));
			}
			_pilgrimage->load_on_demand_all_required();

			// next, current, last (last as we still may need some data)
			{
				Concurrency::ScopedSpinLock lock(usedDataLock);
				if (usedData.get_size() > LIMIT)
				{
					auto& lastUD = usedData.get_last();
					output(TXT("[PilgrimageDataMaintainer] dropping \"%S\""), lastUD->get_name().to_string().to_char());
					lastUD->unload_for_load_on_demand_all_required();
					usedData.pop_back();
				}
			}
		}

		{
			Concurrency::ScopedSpinLock lock(usedDataLock);
			output(TXT("[PilgrimageDataMaintainer] loaded:"));
			for_every_ref(ud, usedData)
			{
				output(TXT("[PilgrimageDataMaintainer]  +- \"%S\""), ud->get_name().to_string().to_char());
			}
		}
	}

	void drop_all(bool _dontUnloadJustDrop)
	{
		Concurrency::ScopedSpinLock lock(usedDataLock);

		if (!_dontUnloadJustDrop)
		{
			// we unload only if requested
			// if we're not unloading, it means we're dropping stuff when closing the game
			ASSERT_ASYNC;
			for_every_ref(u, usedData)
			{
				u->unload_for_load_on_demand_all_required();
			}
		}

		usedData.clear();
	}

	void log(LogInfoContext& _log)
	{
		_log.log(TXT("pilgrimage data loaded:"));
		{
			Concurrency::ScopedSpinLock lock(usedDataLock);

			LOG_INDENT(_log);
			for_every_ref(ud, usedData)
			{
				_log.log(TXT("%S"), ud->get_name().to_string().to_char());
			}
			for (int i = usedData.get_size(); i < LIMIT; ++i)
			{
				_log.log(TXT("--"));
			}
		}
	}

};
PilgrimageDataMaintainer* g_pilgrimageDataMaintainer = nullptr;

//

REGISTER_FOR_FAST_CAST(PilgrimageInstance);

PilgrimageInstance* PilgrimageInstance::s_current = nullptr;
Concurrency::Counter PilgrimageInstance::s_pilgrimageInstance_deferredEndJobs;

void PilgrimageInstance::initialise_static()
{
	an_assert(!g_pilgrimageDataMaintainer);
	g_pilgrimageDataMaintainer = new PilgrimageDataMaintainer();
}

void PilgrimageInstance::close_static()
{
	delete_and_clear(g_pilgrimageDataMaintainer);
}

void PilgrimageInstance::drop_all_pilgrimage_data(bool _dontUnloadJustDrop)
{
	g_pilgrimageDataMaintainer->drop_all(_dontUnloadJustDrop);
}

PilgrimageInstance::PilgrimageInstance(Game* _game, Pilgrimage* _pilgrimage)
: game(_game)
, pilgrimage(_pilgrimage)
{
	an_assert(pilgrimage.get());
	canCreateNextPilgrimage = pilgrimage->can_create_next_pilgrimage();

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("pilgrimage instance START"));
#endif
	if (!s_current) // we may create another one when we switch to it
	{
		make_current();
	}
	environmentCycleSpeed = pilgrimage->get_environment_cycle_speed();
}

PilgrimageInstance::~PilgrimageInstance()
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("pilgrimage instance END"));
#endif
	if (s_current == this)
	{
		make_no_longer_current(nullptr);
		s_current = nullptr;
	}
}

void PilgrimageInstance::reset_deferred_job_counters()
{
	s_pilgrimageInstance_deferredEndJobs = 0;
}

void PilgrimageInstance::will_continue(PilgrimageInstance const* _pi, Framework::Room * _transitionRoom, Framework::Door * _transitionRoomExitDoor)
{
	randomSeed = _pi->randomSeed;
	environmentCycleIdx = _pi->environmentCycleIdx;
	environmentCyclePt = _pi->environmentCyclePt;
	environmentRotationYaw = _pi->environmentRotationYaw;

	// we should not copy killedInfos, why were we doing this?
	/*
	{
		Concurrency::ScopedMRSWLockRead lock(killedInfoLock);
		Concurrency::ScopedMRSWLockRead lockPI(_pi->killedInfoLock);
		killedInfos = _pi->killedInfos; 
	}
	*/
}

void PilgrimageInstance::make_no_longer_current(PilgrimageInstance const* _switchingTo)
{
	if (! _switchingTo)
	{
		if (auto* gd = GameDirector::get())
		{
			gd->deactivate();
		}
	}
}

void PilgrimageInstance::make_current()
{
	if (s_current)
	{
		s_current->make_no_longer_current(this);
	}
	s_current = this;
	if (auto* gd = GameDirector::get())
	{
		auto* p = get_pilgrimage();
		bool gdActive = gd->is_active();
		bool gdShouldBeActive = p && p->does_use_game_director();
		if (gdActive ^ gdShouldBeActive)
		{
			if (gdShouldBeActive)
			{
				gd->activate();
			}
			else
			{
				gd->deactivate();
			}
		}
	}
}

void PilgrimageInstance::do_clean_start_for_game_director()
{
	if (s_current && s_current == this)
	{
		if (auto* gd = GameDirector::get())
		{
			auto* p = get_pilgrimage();
			if (p && p->does_use_game_director())
			{
				gd->do_clean_start_for(this);
			}
		}
	}
	else
	{
		an_assert(false, TXT("we should call it only from the current pilgrimage instance"));
	}
}

void PilgrimageInstance::store_game_state(GameState* _gameState, StoreGameStateParams const& _params)
{
	if (randomSeed.get_seed_lo_number() == 0)
	{
		_gameState->pilgrimageState.access<int>(NAME(randomSeed)) = randomSeed.get_seed_hi_number();
	}
	else
	{
		_gameState->pilgrimageState.access<VectorInt2>(NAME(randomSeed)) = VectorInt2(randomSeed.get_seed_hi_number(), randomSeed.get_seed_lo_number());
	}
	_gameState->pilgrimageState.access<int>(NAME(environmentCycleIdx)) = environmentCycleIdx;
	_gameState->pilgrimageState.access<float>(NAME(environmentCyclePt)) = environmentCyclePt;
	_gameState->pilgrimageState.access<float>(NAME(environmentRotationYaw)) = (float)environmentRotationYaw;
	if (lockEnvironmentCycleUntilSeen.is_set())
	{
		_gameState->pilgrimageState.access<float>(NAME(lockEnvironmentCycleUntilSeen)) = lockEnvironmentCycleUntilSeen.get();
		_gameState->pilgrimageState.access<String>(NAME(lockEnvironmentCycleUntilSeenRoomTagged)) = lockEnvironmentCycleUntilSeenRoomTagged.to_string();
	}

	_gameState->pilgrimageState.access<int>(NAME(upgradeMachinesUnlocked)) = upgradeMachinesUnlocked;

	{
		Concurrency::ScopedMRSWLockRead lock(killedInfoLock);
		_gameState->killedInfos = killedInfos;
	}

	{
		_gameState->gear = new PilgrimGear();

		if (auto* pa = game->access_player().get_actor())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				// and while we're here, store last setup too
				if (auto* persistence = Persistence::access_current_if_exists())
				{
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
					output(TXT("[persistence] store last setup on store game state (pilgrimage instance)"));
#endif
					persistence->store_last_setup(mp);
				}
				mp->store_for_game_state(_gameState->gear.get(), OUT_ REF_ _gameState->atLeastHalfHealth);
			}
		}
	}
}

void PilgrimageInstance::create_and_start(GameState* _fromGameState, bool _asNextPilgrimageStart)
{
#ifdef AN_IMPORTANT_INFO
	output(TXT("create and start [pilgrimage instance] \"%S\" (%S)"), get_pilgrimage()->get_name().to_string().to_char(), is_current() ? TXT("current") : TXT("not current / next"));
	if (_fromGameState)
	{
		output(TXT(" + from game state"));
	}
	if (_asNextPilgrimageStart)
	{
		output(TXT(" + as next pilgrimage start"));
	}
#endif

	PlayerSetup::get_current().get_preferences().apply_to_game_tags(); // rotating elevators

	// for current pilgrimage this should be called once: create_and_start(***, false); this pilgrimage instance should be the current one
	// for next pilgrimage this should be called twice: create_and_start(nullptr, false); and then when it this pilgrimage instance becomes the current one, create_and_start(nullptr, true); 
	an_assert(!_fromGameState || is_current(), TXT("can't load game state for non current"));
	an_assert(!_fromGameState || !_asNextPilgrimageStart, TXT("can't load game state for next pilgrimage start"));
	an_assert(is_current() || !_asNextPilgrimageStart, TXT("can't start next pilgrimage start if not current"));

	{
		mixEnvironmentCycleVars.make_space_for(pilgrimage->get_environment_cycles().get_size());
		while (mixEnvironmentCycleVars.get_size() < pilgrimage->get_environment_cycles().get_size())
		{
			mixEnvironmentCycleVars.push_back(0.0f);
		}
	}

	{
		RefCountObjectPtr<PilgrimageInstance> keepThis(this);
		game->add_async_world_job(TXT("load on demand all that require doing so"),
			[keepThis, this]()
			{
				g_pilgrimageDataMaintainer->use(pilgrimage.get());
			});	
	}

	if (is_current())
	{
		// we're the current pilgrimage and we're starting not from a game state - clean start, process it in the game director
		if (!_fromGameState)
		{
			do_clean_start_for_game_director();
		}
		
		// unlock pilgrimage, save new chapter
		if (!_fromGameState)
		{
			if (auto* ps = PlayerSetup::access_current_if_exists())
			{
				if (auto* gs = ps->access_active_game_slot())
				{
					bool unlockPilgrimage = pilgrimage->should_auto_unlock_when_reached();
					Framework::LibraryName unlockPilgrimageName = pilgrimage->get_name();
					Energy experienceForReachingForFirstTime = pilgrimage->get_experience_for_reaching_for_first_time();
					game->add_immediate_sync_world_job(TXT("unlock pilgrimage"),
						[gs, unlockPilgrimageName, experienceForReachingForFirstTime, unlockPilgrimage]()
						{
							gs->sync_reached_pilgrimage(unlockPilgrimageName, experienceForReachingForFirstTime);
							if (unlockPilgrimage)
							{
								gs->sync_unlock_pilgrimage(unlockPilgrimageName);
							}
						});
				}
			}

			// check if we should store game state as chapter start (only when we start advancing)
			{
				requestSavingGameStateAsChapterStart = true;
				if (auto* p = get_pilgrimage())
				{
					if (!p->does_allow_store_chapter_start_game_state())
					{
						requestSavingGameStateAsChapterStart = false;
					}
				}
			}
		}

		if (_fromGameState)
		{
			if (auto* randomSeedParam = _fromGameState->pilgrimageState.get_existing<VectorInt2>(NAME(randomSeed)))
			{
				randomSeed.set_seed(randomSeedParam->x, randomSeedParam->y);
			}
			if (auto* randomSeedParam = _fromGameState->pilgrimageState.get_existing<int>(NAME(randomSeed)))
			{
				randomSeed.set_seed(*randomSeedParam, 0);
			}
			environmentCycleIdx = _fromGameState->pilgrimageState.get_value<int>(NAME(environmentCycleIdx), environmentCycleIdx);
			environmentCyclePt = _fromGameState->pilgrimageState.get_value<float>(NAME(environmentCyclePt), environmentCyclePt);
			environmentRotationYaw = (double)_fromGameState->pilgrimageState.get_value<float>(NAME(environmentRotationYaw), (float)environmentRotationYaw);
			if (auto* lecus = _fromGameState->pilgrimageState.get_existing<float>(NAME(lockEnvironmentCycleUntilSeen)))
			{
				lockEnvironmentCycleUntilSeen = *lecus;
				{
					String temp = _fromGameState->pilgrimageState.get_value<String>(NAME(lockEnvironmentCycleUntilSeenRoomTagged), String::empty());
					if (!temp.is_empty())
					{
						lockEnvironmentCycleUntilSeenRoomTagged.load_from_string(temp);
					}
				}
			}
			else
			{
				lockEnvironmentCycleUntilSeen.clear();
				lockEnvironmentCycleUntilSeenRoomTagged.clear();
			}

			upgradeMachinesUnlocked = _fromGameState->pilgrimageState.get_value<int>(NAME(upgradeMachinesUnlocked), 0);
		}
		else
		{
			if (!_asNextPilgrimageStart)
			{
				// reset to random
				environmentCycleIdx = Random::get_int(1024);
				environmentCyclePt = Random::get_float(0.0f, 1.0f);
				// or to just zero
				environmentRotationYaw = 0.0f;
			}
			lockEnvironmentCycleUntilSeen = pilgrimage->get_lock_environment_cycle_until_seen();
			lockEnvironmentCycleUntilSeenRoomTagged = pilgrimage->get_lock_environment_cycle_until_seen_room_tagged();
		}

		// restore or kill "killedInfos" (unless we continue)
		if (_fromGameState)
		{
			Concurrency::ScopedMRSWLockWrite lock(killedInfoLock);
			killedInfos = _fromGameState->killedInfos;
#ifdef DEBUG_KILLED_INFOS
			output(TXT("killed infos from game state"));
			for_every(ki, killedInfos)
			{
				output(TXT("  address=\"%S\" poiIdx=\"%i\""), ki->worldAddress.to_string().to_char(), ki->poiIdx);
			}
#endif
		}
		else if (!_asNextPilgrimageStart)
		{
			Concurrency::ScopedMRSWLockWrite lock(killedInfoLock);
			killedInfos.clear();
		}

#ifdef AN_IMPORTANT_INFO
		output(TXT("%S pilgrimage with seed %S"), _asNextPilgrimageStart? TXT("continue") : TXT("start"), randomSeed.get_seed_string().to_char());
#endif

		if (!_asNextPilgrimageStart) // if we continue, we don't have to reset stuff
		{
			{
				MEASURE_PERFORMANCE(updateRoomGenerationInfo);
				PlayerSetup::get_current().get_preferences().apply_to_vr_and_room_generation_info(); // make sure we have latest preferences in
				RoomGenerationInfo::access().setup_to_use();
				post_setup_rgi();
			}

			GameStats::use_pilgrimage();
			GameStats::get().reset(); // reset before setting any info
			GameStats::get().size = RoomGenerationInfo::get().get_play_area_rect_size().to_string();
			GameStats::get().seed = get_random_seed_info();

			GameSettings::get().restart_not_required();
		}

		if (!_asNextPilgrimageStart) // if we continue, we don't have to request new world, we already have it
		{
			Random::Generator rg = randomSeed;
			game->request_world(rg);
		}

#ifdef AN_IMPORTANT_INFO
		output(TXT("%S pilgrimage with seed %S"), _asNextPilgrimageStart ? TXT("continue") : TXT("start"), randomSeed.get_seed_string().to_char());
		output(TXT("issue create next pilgrimage"));
#endif
		// issue as soon as possible and handle its jobs as quickly as possible as there should be just a starting room created
		// and it might be required to process before we start creating cells (to know where to go)
		issue_create_next_pilgrimage();
	}

	// to make it continue from previous pilgrimage?
	issue_utility_rooms_creation(! _asNextPilgrimageStart);
}

void PilgrimageInstance::issue_utility_rooms_creation(bool _keepUtilityRoomsOfCurrentPilgrimage)
{
	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(TXT("issue utility rooms creation"), [this, keepThis, _keepUtilityRoomsOfCurrentPilgrimage]()
	{
		async_create_utility_rooms(_keepUtilityRoomsOfCurrentPilgrimage);
	});
}

void PilgrimageInstance::pre_advance()
{
	if (requestSavingGameStateAsChapterStart)
	{
		requestSavingGameStateAsChapterStart = false;
		output(TXT("store game state - chapter start (on pilgrimage instance start)"));
		Game::get_as<Game>()->add_async_store_game_state(true, GameStateLevel::ChapterStart);
	}

	todo_multiplayer_issue(TXT("we just get player here"));
	if (auto* g = Game::get_as<Game>())
	{
		if (auto* pa = g->access_player().get_actor())
		{
			if (auto* p = pa->get_presence())
			{
				for (int i = 0; i < transitionRooms.get_size(); ++i)
				{
					auto& tr = transitionRooms[i];
					if (!tr.room.is_set())
					{
						transitionRooms.remove_at(i);
						--i;
						continue;
					}

					if (manageTransitionRoomDoors)
					{
						auto* transitionRoom = tr.room.get();
						if (auto* entranceDoor = tr.entranceDoor.get())
						{
							todo_note(TXT("maybe entrance door should be handled in a similair way as exit door in open world instance? by a variable that is set?"));

							if (p->get_in_room() == transitionRoom)
							{
								if (tr.manageEntranceDoor || tr.manageEntranceDoorAtLeastIn.is_set())
								{
									bool okToClose = true;
									if (auto* dir = entranceDoor->get_linked_door_in(transitionRoom))
									{
										okToClose = dir->get_plane().get_in_front(p->get_centre_of_presence_WS()) >= tr.manageEntranceDoorAtLeastIn.get(0.3f);
									}
									if (okToClose)
									{
										entranceDoor->set_operation(Framework::DoorOperation::StayClosed);
									}
								}
								if (entranceDoor->is_closed())
								{
									switchToNextPilgrimage = true;
								}
							}
							else
							{
								if (tr.manageEntranceDoor)
								{
									entranceDoor->set_operation(Framework::DoorOperation::StayOpen, Framework::Door::DoorOperationParams().allow_auto(tr.allowEntranceDoorAuto));
								}
							}
						}
					}
					if (!manageTransitionRoomDoors || !tr.entranceDoor.get())
					{
						if (p->get_in_room() == tr.room.get())
						{
							// the transition room has no entrance door but we are inside it, we could have teleported. switch to the next pilgrimage
							switchToNextPilgrimage = true;
						}
					}
				}
			}
		}
	}

	if (switchToNextPilgrimage)
	{
		issue_switch_to_next_pilgrimage();
	}
}

PilgrimageInstance::TransitionRoom PilgrimageInstance::async_create_transition_room_to_next_pilgrimage()
{
	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

	PilgrimageInstance::TransitionRoom result;
	if (auto* np = nextPilgrimage.get())
	{
		result = np->async_create_transition_room_for_previous_pilgrimage(this);
		if (result.room.is_set())
		{
			result.manageEntranceDoor = ! (result.room->get_value<bool>(NAME(handleDoorOperation), false, false /* just this room */) ||
										   result.room->get_value<bool>(NAME(handleEntranceDoorOperation), false, false /* just this room */));
			result.allowEntranceDoorAuto = result.room->get_value<bool>(NAME(allowEntranceDoorAuto), true, false /* just this room */);
			if (result.room->has_value<float>(NAME(handleEntranceDoorAtLeastIn), false /* just this room */))
			{
				result.manageEntranceDoorAtLeastIn = result.room->get_value<float>(NAME(handleEntranceDoorAtLeastIn), result.manageEntranceDoorAtLeastIn.get(0.3f), false /* just this room */);
			}
			{
				for_count(int, id, 2)
				{
					if (auto* d = (id == 0 ? result.entranceDoor.get() : result.exitDoor.get()))
					{
						for_count(int, i, 2)
						{
							if (auto* dir = d->get_linked_door(i))
							{
								if (dir->get_in_room() == result.room.get())
								{
									dir->access_tags().set_tag(id == 0 ? NAME(entranceDoor) : NAME(exitDoor));
								}
							}
						}
					}
				}
			}
			RefCountObjectPtr<PilgrimageInstance> keepThis(this);
			game->perform_sync_world_job(TXT("store transition room"),
				[keepThis, this, result]()
				{
					transitionRooms.push_back(result);
				});
		}
	}
	else
	{
		warn_dev_investigate(TXT("no next pilgrimage, might be on purpose"));
	}
	return result;
}

void PilgrimageInstance::set_can_create_next_pilgrimage(bool _canCreateNextPilgrimage)
{
	if (canCreateNextPilgrimage != _canCreateNextPilgrimage)
	{
		an_assert(!issuedCreateNextPilgrimage, TXT("already too late - next pilgrimage creation has been issued"));
		canCreateNextPilgrimage = _canCreateNextPilgrimage;
		if (canCreateNextPilgrimage && !issuedCreateNextPilgrimage)
		{
			issue_create_next_pilgrimage();
		}
	}
}

void PilgrimageInstance::issue_create_next_pilgrimage()
{
	if (!canCreateNextPilgrimage)
	{
#ifdef AN_IMPORTANT_INFO
		output(TXT("can't create next pilgrimage yet"));
#endif
		return;
	}
	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(TXT("create next pilgrimage, just as an empty shell"), [this, keepThis]()
		{
			async_create_next_pilgrimage();
		});
	issuedCreateNextPilgrimage = true;
}

Pilgrimage* PilgrimageInstance::get_next_pilgrimage() const
{
	if (auto* np = nextPilgrimage.get())
	{
		return np->get_pilgrimage();
	}

	Pilgrimage* nextP = nullptr;
	if (auto* ms = MissionState::get_current())
	{
		for_every(p, ms->get_pilgrimages())
		{
			if (pilgrimage == *p && for_everys_index(p) < ms->get_pilgrimages().get_size() - 1)
			{
				nextP = ms->get_pilgrimages()[for_everys_index(p) + 1].get();
			}
		}
	}
	else if (auto* ch = GameDefinition::get_chosen())
	{
		for_every(p, ch->get_pilgrimages())
		{
			if (pilgrimage == *p && for_everys_index(p) < ch->get_pilgrimages().get_size() - 1)
			{
				nextP = ch->get_pilgrimages()[for_everys_index(p) + 1].get();
			}
		}
	}
	if (!nextP)
	{
		nextP = pilgrimage->get_default_next_pilgrimage();
	}
	return nextP;
}

void PilgrimageInstance::async_create_next_pilgrimage()
{
	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	{
		Pilgrimage* nextP = get_next_pilgrimage();

		if (nextP)
		{
			nextPilgrimage = nextP->create_instance(game);
			nextPilgrimage->set_seed(randomSeed);
			// PilgrimageInstanceLinear ?

			// this will start shorter route, to create main region etc
			nextPilgrimage->create_and_start(nullptr /*no save state*/, false /*we don't start it yet!*/); // at proper time will step out of waiting and will start actual game
		}
		else
		{
#ifdef AN_IMPORTANT_INFO
			output(TXT("no next pilgrimage to create"));
#endif
		}
	}

	post_create_next_pilgrimage();
}

void PilgrimageInstance::post_create_next_pilgrimage()
{
}

void PilgrimageInstance::issue_switch_to_next_pilgrimage()
{
	if (isBusyAsync)
	{
		return;
	}

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	if (!game->does_allow_async_jobs())
	{
		return;
	}

	switchToNextPilgrimage = false;

	if (!nextPilgrimage.is_set() || ! is_current())
	{
		return;
	}

#ifdef AN_IMPORTANT_INFO
	output(TXT("request switch to next pilgrimage"));
#endif

	isBusyAsync = true;
	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(TXT("switch to next pilgrimage"),
		[keepThis, this]()
		{
#ifdef AN_IMPORTANT_INFO
			output(TXT("switch to next pilgrimage"));
#endif
			todo_multiplayer_issue(TXT("we just get player here"));
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (auto* p = pa->get_presence())
					{
						int bestTransitionRoomMatch = NONE;
						TransitionRoom* bestTransitionRoom = nullptr;

						for_every(tr, transitionRooms)
						{
							if (auto* transitionRoom = tr->room.get())
							{
								// check all transition rooms
								// prefer the one we're in
								// if can't tell where we are, get the one we've seen (although most likely we should be in that room!)
								// in all other cases (that are extremely unlikely) get the one that just simply exists
								{
									int trMatch = 0; // the room exists
									if (transitionRoom->was_recently_seen_by_player())
									{
										trMatch += 1;
									}
									if (p->get_in_room() == transitionRoom)
									{
										trMatch += 2;
									}
									if (trMatch > bestTransitionRoomMatch)
									{
										bestTransitionRoom = tr;
									}
								}

								if (auto* entranceDoor = tr->entranceDoor.get())
								{
									// we want to disconnect from the region we'll be deleting
									game->perform_sync_world_job(TXT("unlink entrance door - to other side"),
										[entranceDoor, transitionRoom]()
										{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
											output(TXT("unlink entrance door - to other side"));
#endif
											if (auto* d = entranceDoor->get_linked_door_a())
											{
												if (d->get_in_room() != transitionRoom)
												{
													d->unlink();
												}
											}
											if (auto* d = entranceDoor->get_linked_door_b())
											{
												if (d->get_in_room() != transitionRoom)
												{
													d->unlink();
												}
											}
										});
								}
							}
						}

						if (auto* tr = bestTransitionRoom)
						{
							auto* transitionRoom = tr->room.get();
							an_assert(transitionRoom);

							IPilgrimageInstanceObserver::pilgrimage_instance_switch(this, nextPilgrimage.get());

							// mark that we're continuing this pilgrimage right now
							nextPilgrimage->will_continue(this, transitionRoom, tr->exitDoor.get());

							// make the next pilgrimage current
							game->perform_sync_world_job(TXT("switch pilgrimage"), [this]() { nextPilgrimage->make_current(); });
								
							// issue destroying region here
							an_assert(!transitionRoom->is_in_region(mainRegion.get()), TXT("transition room should be created in main region of the next pilgrimage"));
							if (mainRegion.is_set())
							{
								mainRegion->start_destruction();
							}

							//
							// activate/initialise next pilgrimage, the transition room should be already there but make it the starting room
							nextPilgrimage->create_and_start(nullptr, true /* we're starting next pilgrimage */);

							isBusyAsync = false;
						}
					}
				}
			}

		}
	);

}

void PilgrimageInstance::for_our_utility_rooms(Framework::IModulesOwner* _forObject, std::function<void(Framework::Room* _room)> _do)
{
	ASSERT_NOT_SYNC_NOR_ASYNC; // this is gameplay function!
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* utilityRegion = game->get_utility_region())
		{
			for_every_ptr(utilityRoom, utilityRegion->get_own_rooms())
			{
				Framework::Region* forPilgrimagesMainRegion = nullptr;
				Framework::Region* forPilgrimagesMainRegionToBe = nullptr;
				if (auto* v = utilityRoom->get_variables().get_existing<SafePtr<Framework::Region>>(NAME(forPilgrimagesMainRegion)))
				{
					forPilgrimagesMainRegion = v->get();
				}
				if (auto* v = utilityRoom->get_variables().get_existing<SafePtr<Framework::Region>>(NAME(forPilgrimagesMainRegionToBe)))
				{
					forPilgrimagesMainRegionToBe = v->get();
				}

				bool isOk = false;
				auto* inRegion = _forObject->get_presence()->get_in_room()->get_in_region();
				while (inRegion)
				{
					if (inRegion == forPilgrimagesMainRegion ||
						inRegion == forPilgrimagesMainRegionToBe)
					{
						isOk = true;
						break;
					}
					inRegion = inRegion->get_in_region();
				}

				if (isOk)
				{
					_do(utilityRoom);
				}
			}
		}
	}
}

void PilgrimageInstance::async_create_utility_rooms(bool _keepUtilityRoomsOfCurrentPilgrimage)
{
	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output(TXT("create utility rooms %S for %S"), _keepUtilityRoomsOfCurrentPilgrimage? TXT("and keep existing ones for time being") : TXT("and destroy all existing that we don't use"), get_pilgrimage()->get_name().to_string().to_char());
#endif

	// we'll be going through the list of required utility rooms and if we create/found one, we're keeping it
	// others are going to the bin (unless we want to keep them)
	Array<SafePtr<Framework::Room>> utilityRoomsToRemove;
	if (auto* utilityRegion = game->get_utility_region())
	{
		if (! _keepUtilityRoomsOfCurrentPilgrimage)
		{
			Array<Framework::Room*> utilityRoomsToKeep;
			auto* pi = PilgrimageInstance::get(); // before deleting, check all existing instances!
			while (pi)
			{
				output(TXT("pilgrimage instance"));
				if (auto* pilgrimage = pi->get_pilgrimage())
				{
					output(TXT("check \"%S\" for still in use"), pilgrimage->get_name().to_string().to_char());
					for_every(ur, pilgrimage->get_utility_rooms())
					{
						output(TXT("check tag \"%S\""), ur->tag.to_char());
						Framework::Room* foundRoom = nullptr;

						game->perform_sync_world_job(TXT("find utility room"), [&foundRoom, utilityRegion, ur]()
						{
							foundRoom = utilityRegion->find_room_tagged(ur->tag);
						});

						if (foundRoom)
						{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
							output(TXT("utility room r%p \"%S\" still in use"), foundRoom, foundRoom->get_name().to_char());
#endif
							output(TXT("utility room r%p \"%S\" still in use"), foundRoom, foundRoom->get_name().to_char());
							utilityRoomsToKeep.push_back(foundRoom);
						}
					}
				}

				pi = pi->get_next_pilgrimage_instance();
			}
			game->perform_sync_world_job(TXT("get all rooms from utility region"), [&utilityRoomsToRemove, utilityRegion, this, &utilityRoomsToKeep]()
			{
				utilityRoomsToRemove.make_space_for(utilityRegion->get_own_rooms().get_size());
				for_every_ptr(room, utilityRegion->get_own_rooms())
				{
					output(TXT("utility room to destroy? r%p \"%S\""), room, room->get_name().to_char());
					if (room != game->get_locker_room() &&
						! utilityRoomsToKeep.does_contain(room))
					{
						output(TXT("utility room r%p \"%S\" to destroy"), room, room->get_name().to_char());
						utilityRoomsToRemove.push_back(SafePtr<Framework::Room>(room));
					}
				}
			});
		}

		for_every(ur, get_pilgrimage()->get_utility_rooms())
		{
			Framework::Room* foundRoom = nullptr;

			game->perform_sync_world_job(TXT("find utility room"), [&foundRoom, utilityRegion, ur]()
			{
				foundRoom = utilityRegion->find_room_tagged(ur->tag);
			});

			if (foundRoom)
			{
				for_every(urtr, utilityRoomsToRemove)
				{
					if (*urtr == foundRoom)
					{
						output(TXT("utility room r%p \"%S\" will be reused"), foundRoom, foundRoom->get_name().to_char());
						utilityRoomsToRemove.remove_fast_at(for_everys_index(urtr));
						break;
					}
				}
				// steal it, make this our utility room now
				if (!_keepUtilityRoomsOfCurrentPilgrimage)
				{
					foundRoom->access_variables().access<SafePtr<Framework::Region>>(NAME(forPilgrimagesMainRegion)) = mainRegion.get();
					foundRoom->access_variables().access<SafePtr<Framework::Region>>(NAME(forPilgrimagesMainRegionToBe)).clear();
				}
				else
				{
					foundRoom->access_variables().access<SafePtr<Framework::Region>>(NAME(forPilgrimagesMainRegionToBe)) = mainRegion.get();
				}
			}
			else
			{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output(TXT("creating utility room \"%S\""), ur->roomType.get()? ur->roomType->get_name().to_string().to_char() : TXT("--"));
#endif
				if (ur->roomType.get())
				{
					Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();
					Framework::Room* newRoom;
					Game::get()->perform_sync_world_job(TXT("create new room"), [&newRoom, this, utilityRegion, ur]()
					{
						newRoom = new Framework::Room(game->get_sub_world(), utilityRegion, ur->roomType.get(), randomSeed);
						ROOM_HISTORY(newRoom, TXT("utility room"));
					});
					newRoom->access_tags().set_tag(ur->tag);
					newRoom->access_variables().access<SafePtr<Framework::Region>>(NAME(forPilgrimagesMainRegion)) = mainRegion.get();
					newRoom->access_variables().access<SafePtr<Framework::Region>>(NAME(forPilgrimagesMainRegionToBe)).clear();
					newRoom->generate();
					game->request_ready_and_activate_all_inactive(game->get_current_activation_group());
					game->pop_activation_group(activationGroup.get());
				}
				else
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (!Library::may_ignore_missing_references())
#endif
					{
						error(TXT("could not find room type \"%S\""), ur->roomType.get_name().to_string().to_char());
					}
				}
			}
		}

		for_every_ref(urtr, utilityRoomsToRemove)
		{
			Game::get()->perform_sync_world_job(TXT("mark room to be deleted [utility rooms]"), [urtr]()
			{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output(TXT("utility room \"%S\" not used anymore"), urtr->get_name().to_char());
#endif
				output(TXT("utility room \"%S\" not used anymore"), urtr->get_name().to_char());
				if (urtr && !urtr->is_deletion_pending())
				{
					urtr->mark_to_be_deleted();
				}
			});
		}
	}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output(TXT("check locker room spawns for \"%S\""), get_pilgrimage()? get_pilgrimage()->get_name().to_string().to_char() : TXT("--"));
#endif
	if (!lockerRoomSpawnsDone)
	{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output(TXT("locker room spawns not done yet"));
#endif
		if (auto* lockerRoom = game->get_locker_room())
		{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("perform locker room spawns"));
#endif
			
			lockerRoomSpawnsDone = true;

			Random::Generator rg = randomSeed;
			rg.advance_seed(2345, 59723);

			for_every(lrs, get_pilgrimage()->get_locker_room_spawns())
			{
				rg.advance_seed(23, 4);

				bool spawn = true;
				if (! lrs->name.is_empty())
				{
					game->perform_sync_world_job(TXT("check if locker room spawn is needed"), [&spawn, lrs, lockerRoom]()
					{
						for_every_ptr(o, lockerRoom->get_all_objects())
						{
							if (auto* obj = fast_cast<Framework::Object>(o))
							{
								if (obj->get_name() == lrs->name)
								{
									spawn = false;
									break;
								}
							}
						}
					});
				}

				if (spawn)
				{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output(TXT("performing locker room spawn \"%S\""), lrs->name.to_char());
#endif

					RefCountObjectPtr<Framework::DelayedObjectCreation> doc(new Framework::DelayedObjectCreation());

					if (!doc->inSubWorld)
					{
						doc->inSubWorld = lockerRoom->get_in_sub_world();
					}
					if (!doc->inSubWorld)
					{
						doc->inSubWorld = game->get_sub_world();
					}
					doc->inRoom = lockerRoom;
					doc->name = lrs->name.is_empty() ? String(TXT("pilgrimage's locker room object")) : lrs->name;
					doc->tagSpawned = lrs->tags;
					if (auto* ot = lrs->actorType.get()) doc->objectType = ot;
					if (auto* ot = lrs->itemType.get()) doc->objectType = ot;
					if (auto* ot = lrs->sceneryType.get()) doc->objectType = ot;
					doc->placement = Transform::identity;
					doc->randomGenerator = rg;

					if (auto * r = mainRegion.get()) // use main region as we don't have anything else and still, in main region there might be lots of data
					{
						r->collect_variables(doc->variables);
					}

					doc->variables.set_from(lrs->parameters);

					// do not delay, do it immediately
					doc->process(Game::get(), true);

					if (auto* obj = doc->createdObject.get())
					{
						obj->suspend_advancement();
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
						if (auto* fo = fast_cast<Framework::Object>(obj))
						{
							output(TXT("spawned for locker room o%p \"%S\" tagged \"%S\""), obj, obj->ai_get_name().to_char(), fo->get_tags().to_string().to_char());
						}
						else
						{
							output(TXT("spawned for locker room o%p \"%S\" non-object"), obj, obj->ai_get_name().to_char());
						}
#endif
					}
					else
					{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
						error(TXT("did not spawn for locker room \"%S\""), doc->name.to_char());
#endif
					}
				}
				else
				{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output(TXT("locker room spawn not needed for \"%S\""), lrs->name.to_char());
#endif
				}
			}
		}
		else
		{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("locker room missing"));
#endif
			error(TXT("locker room missing"));
		}
	}
	else
	{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output(TXT("locker room spawns already done"));
#endif
	}
}

void PilgrimageInstance::on_end(PilgrimageResult::Type _result)
{
	IPilgrimageInstanceObserver::pilgrimage_instance_end(this, _result);
}

void PilgrimageInstance::async_generate_base(bool _asNextPilgrimageStart)
{
	scoped_call_stack_info(TXT("PilgrimageInstance::async_generate_base"));
	scoped_call_stack_info_str_printf(TXT("in pilgrimage \"%S\""), get_pilgrimage()->get_name().to_string().to_char());

	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output(TXT("async generate base"));
	output(TXT("  for pilgrimage \"%S\""), get_pilgrimage()->get_name().to_string().to_char());
#endif

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	an_assert(!subWorld || _asNextPilgrimageStart);

	MEASURE_PERFORMANCE(createBase);

	game->perform_sync_world_job(TXT("get sub world"), [this]()
	{
		subWorld = game->get_sub_world();
		if (subWorld && is_current())
		{
			subWorld->access_collision_info_provider().set_gravity_dir_based(-Vector3::zAxis, 9.81f);
			subWorld->access_collision_info_provider().set_kill_gravity_distance(1000.0f); // will die eventually
		}
	});

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output(TXT("subworld sw'%p"), subWorld);
#endif

	if (!_asNextPilgrimageStart && subWorld)
	{
		scoped_call_stack_info(TXT("create main region and environments"));

		an_assert(!mainRegion.is_set());
		game->perform_sync_world_job(TXT("create main region"), [this]()
		{
			Random::Generator rg = randomSeed;
			rg.advance_seed(8063, 97235);
			mainRegion = new Framework::Region(subWorld, nullptr, get_main_region_type(), rg);
			mainRegion->set_world_address_id(NONE); // NONE for main
		});

		{
			mainRegion->run_wheres_my_point_processor_setup();
			mainRegion->generate_environments();
		}

		{
			Random::Generator rg = randomSeed;
			for_every(pe, pilgrimage->get_environments())
			{
				scoped_call_stack_info_str_printf(TXT("add environment %i"), for_everys_index(pe));
				Framework::Environment* environment;
				game->perform_sync_world_job(TXT("create environment"), [this, &rg, pe, &environment]()
				{
					scoped_call_stack_info(TXT("create environment"));
					rg.advance_seed(12425, 65480);
					environment = new Framework::Environment(pe->name, pe->parent, subWorld, mainRegion.get(), pe->environment.get(), rg);
					mainRegion->add(environment);
					environment->update_parent_environment(mainRegion.get());
				});
				environment->generate();
				game->perform_sync_world_job(TXT("add to the mix"), [this, pe, &environment]() // only after generated
				{
					scoped_call_stack_info(TXT("add to the mix"));
					environments[pe->name] = environment;
					environmentMixes.push_back(EnvironmentMix(pe->name, pe->environment.get()));
				});
			}

			game->perform_sync_world_job(TXT("mark environment mixes good to use"), [this]()
			{
				environmentMixesReadyToUse = true;
			});

			{
				scoped_call_stack_info(TXT("update default environment for main region"));
				mainRegion->update_default_environment();
			}
		}
	}

	// as we have main region now, utility rooms should be made aware of it
	issue_utility_rooms_creation(!_asNextPilgrimageStart);

	generatedBase = true;
}

void PilgrimageInstance::wait_for_no_world_jobs_to_end_generation()
{
	scoped_call_stack_info(TXT("PilgrimageInstance::wait_for_no_world_jobs_to_end_generation"));
	if (game)
	{
		if (game->get_number_of_world_jobs() == 0 &&
			game->is_removing_unused_temporary_objects() == 0)
		{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output_colour(1, 1, 0, 1);
			output(TXT("no more world jobs, we're done"));
			output_colour();
#endif
			isBusyAsync = false;
		}
		else
		{
			game->add_sync_world_job(TXT("wait for no world jobs"), [this]() { wait_for_no_world_jobs_to_end_generation(); });
		}
	}
}

bool PilgrimageInstance::get_poi_placement(Framework::Room* _room, Name const & _poiName, Optional<Name> const& _optionalPOIName, OUT_ Transform & _vrAnchor, OUT_ Transform & _placement, ShouldAllowToFail _allowToFail) const
{
	bool placementFound = false;
	Transform placement = Transform::identity;
	Name vrAnchorId;

	_room->for_every_point_of_interest(_poiName, [&placementFound, &placement, &vrAnchorId](Framework::PointOfInterestInstance * _fpoi)
		{
			placementFound = true;
			placement = _fpoi->calculate_placement();
			vrAnchorId = _fpoi->get_parameters().get_value<Name>(NAME(vrAnchorId), Name::invalid());
		});
	if (!placementFound && _optionalPOIName.is_set())
	{
		_room->for_every_point_of_interest(_optionalPOIName, [&placementFound, &placement, &vrAnchorId](Framework::PointOfInterestInstance * _fpoi)
			{
				placementFound = true;
				placement = _fpoi->calculate_placement();
				vrAnchorId = _fpoi->get_parameters().get_value<Name>(NAME(vrAnchorId), Name::invalid());
			});
	}

	Transform vrAnchor = Transform::identity;
	Optional<float> closestDistance;
	_room->for_every_point_of_interest(NAME(vrAnchor), [placement , &vrAnchor, vrAnchorId, &closestDistance](Framework::PointOfInterestInstance * _fpoi)
		{
			Transform poiPlacement = _fpoi->calculate_placement();
			float distance = (poiPlacement.get_translation() - placement.get_translation()).length();
			if (vrAnchorId.is_valid())
			{
				// we have to find a matching one
				Name poiVRAnchorId = _fpoi->get_parameters().get_value<Name>(NAME(vrAnchorId), Name::invalid());
				if (poiVRAnchorId != vrAnchorId)
				{
					return;
				}
			}
			if (!closestDistance.is_set() || closestDistance.get() > distance)
			{
				closestDistance = distance;
				vrAnchor = poiPlacement;
			}
		});

	// commented out for rooms that have no vr anchor as they do not need it (throne corridors)
	/*
	if (_allowToFail == ShouldAllowToFail::DisallowToFail)
	{
		if (!placementFound)
		{
			if (_optionalPOIName.is_set())
			{
				error(TXT("no \"%S\" nor \"%S\" POI for generated (current) room \"%S\"!"), _poiName.to_char(), _optionalPOIName.get().to_char(), _room->get_room_type()->get_name().to_string().to_char());
			}
			else
			{
				error(TXT("no \"%S\" POI for generated (current) room \"%S\"!"), _poiName.to_char(), _room->get_room_type()->get_name().to_string().to_char());
			}
		}
		if (!closestDistance.is_set())
		{
			error(TXT("no \"vr anchor\" POI for generated (current) room \"%S\"!"), _room->get_room_type()->get_name().to_string().to_char());
		}
	}
	*/

	_vrAnchor = vrAnchor;
	_placement = placement;

	return placementFound;
}

Framework::Door* PilgrimageInstance::create_door(Framework::DoorType* _doorType, Framework::Room* _room, Tags const& _tags) const
{
	MEASURE_PERFORMANCE(createDoor);

	Framework::Door* door = nullptr;
	game->perform_sync_world_job(TXT("create door"), [this, _doorType, &door]()
	{
		door = new Framework::Door(subWorld, _doorType);
	});

	if (_room)
	{
		create_door_in_room(door, _room, _tags);
	}

	return door;
}

Framework::DoorInRoom* PilgrimageInstance::create_door_in_room(Framework::Door* _door, Framework::Room* _room, Tags const & _tags) const
{
	MEASURE_PERFORMANCE(createDoorInRoom);

	Framework::DoorInRoom* dir = nullptr;
	game->perform_sync_world_job(TXT("create door in room"), [_room, _door, _tags, &dir]()
	{
		dir = new Framework::DoorInRoom(_room);
		DOOR_IN_ROOM_HISTORY(dir, TXT("door via pilgrimage instance in room \"%S\""), _room->get_name().to_char());
		dir->access_tags().set_tags_from(_tags);
		_door->link(dir); // a, then b
	});
	dir->init_meshes();

	game->on_newly_created_door_in_room(dir);

	return dir;
}

Framework::DoorInRoom* PilgrimageInstance::create_and_place_door_in_room(Framework::Door* _door, Framework::Room* _room, Tags const& _tags, Name const & _atPOINamed, Optional<Name> const & _atPOINamedAlt) const
{
	MEASURE_PERFORMANCE(createAndPlaceDoorInRoom);

	Transform vrAnchor = Transform::identity;
	Transform dirPlacement = Transform::identity;
	bool found = get_poi_placement(_room, _atPOINamed, NP, OUT_ vrAnchor, OUT_ dirPlacement, _atPOINamedAlt.is_set() ? AllowToFail : DisallowToFail);
	if (!found && _atPOINamedAlt.is_set())
	{
		found = get_poi_placement(_room, _atPOINamedAlt.get(), NP, OUT_ vrAnchor, OUT_ dirPlacement, DisallowToFail);
	}
	if (found)
	{
		// add door in room
		Framework::DoorInRoom* dir = create_door_in_room(_door, _room, _tags);

		Transform dirVRPlacement = vrAnchor.to_local(dirPlacement);

		// if it has a fixed vr placement already, grow into a vr corridor
		if (!dir->is_vr_space_placement_set() ||
			!Framework::DoorInRoom::same_with_orientation_for_vr(dir->get_vr_space_placement(), dirVRPlacement))
		{
			if (dir->is_vr_placement_immovable() &&
				dir->is_relevant_for_vr())
			{
				RoomGenerationInfo const& roomGenerationInfo = RoomGenerationInfo::get();
				dir = dir->grow_into_vr_corridor(NP, dirVRPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
			}
			if (!dir->is_vr_placement_immovable())
			{
				dir->set_vr_space_placement(dirVRPlacement);
			}
		}
		dir->make_vr_placement_immovable();
		dir->set_placement(dirPlacement);
		dir->init_meshes();

		return dir;
	}
	else
	{
		error(TXT("no idea where to place door (\"%S\"), not created"), _atPOINamed.to_char());

		return nullptr;
	}
}

void PilgrimageInstance::modify_advance_environment_cycle_by(float _value)
{
	environmentCyclePt += _value;

	while (environmentCyclePt >= 1.0f)
	{
		environmentCyclePt -= 1.0;
		++environmentCycleIdx;
	}
	while (environmentCyclePt < 0.0f)
	{
		environmentCyclePt += 1.0;
		--environmentCycleIdx;
	}

}

void PilgrimageInstance::advance_environments(float _deltaTime, PilgrimageInstance* _beShadowOf)
{
	scoped_call_stack_info(TXT("PilgrimageInstance::advance_environments"));
	scoped_call_stack_info_str_printf(TXT("in pilgrimage \"%S\""), get_pilgrimage()->get_name().to_string().to_char());
	if (_beShadowOf)
	{
		environmentCycleIdx = _beShadowOf->environmentCycleIdx;
		environmentCyclePt = _beShadowOf->environmentCyclePt;
		environmentRotationYaw = _beShadowOf->environmentRotationYaw;
	}
	else
	{
		if (environmentRotationCycleLength != 0.0f)
		{
			environmentRotationYaw += 360.0 * (double)_deltaTime / environmentRotationCycleLength;
			if (environmentRotationYaw > 360.0)
			{
				environmentRotationYaw -= 360.0;
			}
			if (environmentRotationYaw < 0.0)
			{
				environmentRotationYaw += 360.0;
			}
		}
		if (environmentCycleSpeed != 0.0f)
		{	// advance environment
			float targetEnvironmentCycleSpeed = environmentCycleSpeed;
			/*
			// this should be handled by special devices only
			if (environmentCycleSpeed != 0.0f)
			{
				if (auto * playerActor = game->access_player().get_actor())
				{
					if (auto * inRoom = playerActor->get_presence()->get_in_room())
					{
						if (auto * env = inRoom->get_environment())
						{
							// if we're looking at the sun/moon, time goes faster
							Transform eyesRelativeLook = playerActor->get_presence()->get_eyes_relative_look();
							Vector3 lookingDirWS = playerActor->get_presence()->get_placement().vector_to_world(eyesRelativeLook.get_axis(Axis::Y));
							auto *lightSpecularSideParam = inRoom->get_environment()->get_param(NAME(lightSpecularSide));
							auto *lightDirParam = inRoom->get_environment()->access_param(NAME(lightDir));
							float side = lightSpecularSideParam->as<float>() > 0.0f ? 1.0f : -1.0f;
							float atLight = Vector3::dot(lookingDirWS.normal(), -lightDirParam->as<Vector3>().normal() * side);
							float threshold = 0.97f;
							atLight = clamp((atLight - threshold) / (1.0f - threshold), 0.0f, 1.0f);
							targetEnvironmentCycleSpeed = targetEnvironmentCycleSpeed + atLight * 0.3f;
						}
					}
				}
			}
			*/
			currentEnvironmentCycleSpeed = blend_to_using_time(currentEnvironmentCycleSpeed, targetEnvironmentCycleSpeed, 0.5f, _deltaTime);
			if (currentEnvironmentCycleSpeed != 0.0f)
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
				static bool useCycleSpeed = true;
				if (System::Input::get()->is_key_pressed(System::Key::LeftCtrl) ||
					System::Input::get()->is_key_pressed(System::Key::RightCtrl))
				{
					if (System::Input::get()->has_key_been_pressed(System::Key::Slash))
					{
						useCycleSpeed = !useCycleSpeed;
					}
				}
				if (useCycleSpeed)
#endif
#endif
					environmentCyclePt += currentEnvironmentCycleSpeed * _deltaTime;
			}
		}
	}

	if (environmentMixesReadyToUse)
	{
		scoped_call_stack_info_str_printf(TXT("environmentMixes %i"), environmentMixes.get_size());

		for_every(em, environmentMixes)
		{
			em->clear_targets();
		}

		if (!pilgrimage->get_environment_cycles().is_empty())
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
			if (System::Input::get()->is_key_pressed(System::Key::LeftCtrl) ||
				System::Input::get()->is_key_pressed(System::Key::RightCtrl))
			{
				float speed = System::Input::get()->is_key_pressed(System::Key::LeftShift) ||
					System::Input::get()->is_key_pressed(System::Key::RightShift) ? 0.8f : 0.2f;
				if (System::Input::get()->is_key_pressed(System::Key::Comma))
				{
					environmentCyclePt -= speed * System::Core::get_raw_delta_time();
				}
				if (System::Input::get()->is_key_pressed(System::Key::Period))
				{
					environmentCyclePt += speed * System::Core::get_raw_delta_time();
				}
			}
#endif
#endif
			if (lockEnvironmentCycleUntilSeen.is_set())
			{
				environmentCycleIdx = TypeConversions::Normal::f_i_cells(lockEnvironmentCycleUntilSeen.get());
				environmentCyclePt = lockEnvironmentCycleUntilSeen.get() - (float)environmentCycleIdx;
				// check if we shouldn't unlock
				{
					if (!lockEnvironmentCycleUntilSeenRoomTagged.is_empty())
					{
						if (auto* r = mainRegion.get())
						{
							r->for_every_room([this](Framework::Room* _r)
								{
									if (_r->was_recently_seen_by_player() &&
										lockEnvironmentCycleUntilSeenRoomTagged.check(_r->get_tags()))
									{
										lockEnvironmentCycleUntilSeen.clear();
										lockEnvironmentCycleUntilSeenRoomTagged.clear();
									}
								});
						}
					}
				}
			}

#ifdef AN_ALLOW_BULLSHOTS
			if (Framework::BullshotSystem::is_active())
			{
				if (Framework::BullshotSystem::access_lock().acquire_if_not_locked())
				{
					auto& bv = Framework::BullshotSystem::access_variables();
					if (auto* v = bv.get_existing<int>(NAME(bullshotEnvironmentCycleIdx)))
					{
						environmentCycleIdx = *v;
					}
					if (auto* v = bv.get_existing<float>(NAME(bullshotEnvironmentCyclePt)))
					{
						environmentCyclePt = *v;
					}
					Framework::BullshotSystem::access_lock().release();
				}
			}
#endif
			Optional<float> forcedEnvironmentMixPt = PlayerPreferences::get_forced_environment_mix_pt();

			while (environmentCyclePt >= 1.0f)
			{
				environmentCyclePt -= 1.0;
				++environmentCycleIdx;
			}
			while (environmentCyclePt < 0.0f)
			{
				environmentCyclePt += 1.0;
				--environmentCycleIdx;
			}

			for_every_ref(ec, pilgrimage->get_environment_cycles())
			{
				if (!ec->get_environments().is_empty())
				{
					int i, n;
					float usePt;
					if (ec->should_mix_using_var())
					{
						float value = 0.0f;
						if (mixEnvironmentCycleVars.is_index_valid(for_everys_index(ec)))
						{
							value = mixEnvironmentCycleVars[for_everys_index(ec)];
						}
						i = TypeConversions::Normal::f_i_cells(value);
						usePt = value - (float)i;
						i = mod(i, ec->get_environments().get_size());
						n = mod(i + 1, ec->get_environments().get_size());
					}
					else
					{
						int useEnvironmentCycleIdx = environmentCycleIdx;
						float useEnvironmentCyclePt = environmentCyclePt;

						if (forcedEnvironmentMixPt.is_set())
						{
							float mixPt = forcedEnvironmentMixPt.get();
							float scaledMixPt = mixPt * (float)(ec->get_environments().get_size() + 1);
							useEnvironmentCycleIdx = TypeConversions::Normal::f_i_cells(scaledMixPt);
							useEnvironmentCyclePt = scaledMixPt - (float)useEnvironmentCycleIdx;
						}

						i = mod(useEnvironmentCycleIdx, ec->get_environments().get_size());
						n = mod(i + 1, ec->get_environments().get_size());
						an_assert(ec->get_name() == ec->get_environments()[i]->get_name(), TXT("those should be same on loading!"));
						an_assert(ec->get_name() == ec->get_environments()[n]->get_name(), TXT("those should be same on loading!"));
						usePt = clamp(ec->get_transition_range().get_pt_from_value(useEnvironmentCyclePt), 0.0f, 1.0f);
					}
					if (ec->get_environments().is_index_valid(i) &&
						ec->get_environments().is_index_valid(n))
					{
						{
							REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(TXT("mix env %i/%i"), i, ec->get_environments().get_size());
							add_mix(ec->get_environments()[i].get(), 1.0f - usePt, ec);
						}
						{
							REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(TXT("mix env %i/%i"), n, ec->get_environments().get_size());
							add_mix(ec->get_environments()[n].get(), usePt, ec);
						}
					}
				}
			}
		}
		else
		{
			add_environment_mixes_if_no_cycles();
		}

		for_every(em, environmentMixes)
		{
			if (auto* emenv = environments.get_existing(em->name))
			{
				if (auto* env = fast_cast<Framework::Environment>(emenv->get()))
				{
					em->advance(_deltaTime, env, firstAdvance);
					update_auto_environment_variables(env);
				}
			}
		}

		// remove unused, otherwise if we start to blend in to previous we may disrupt mix as it is not that great (lerping with mix/mixsofar)
		for_every(em, environmentMixes)
		{
			em->remove_unused();
		}

	}

	firstAdvance = false;
}

void PilgrimageInstance::advance(float _deltaTime)
{
	an_assert(s_current == this);
	currentFor += _deltaTime;

	MEASURE_PERFORMANCE(pilgrimage_advance__environments);
	advance_environments(_deltaTime, nullptr);

	// next pilgrimage is not advanced but we should still advance environments so the transition room if has a view, should be updated
	if (auto* np = nextPilgrimage.get())
	{
		np->advance_environments(_deltaTime, this);
	}
}

void PilgrimageInstance::add_mix(Array<RefCountObjectPtr<PilgrimageEnvironmentMix>> const & _environments, float _strength, PilgrimageEnvironmentCycle const * _fromCycle)
{
	scoped_call_stack_info(TXT("PilgrimageInstance::add_mix[1]"));
	for_every_ref(env, _environments)
	{
		add_mix(env, _strength, _fromCycle);
	}
}

void PilgrimageInstance::add_mix(PilgrimageEnvironmentMix const * _environmentMix, float _strength, PilgrimageEnvironmentCycle const * _fromCycle)
{
	scoped_call_stack_info(TXT("PilgrimageInstance::add_mix[2]"));
	for_every(em, environmentMixes)
	{
		if (em->name == _environmentMix->get_name())
		{
			scoped_call_stack_info_str_printf(TXT("mix env \"%S\""), em->name.to_char());
			for_every(entry, _environmentMix->get_entries())
			{
				EnvironmentMix::Part* found = nullptr;
				for_every(e, em->parts)
				{
					if (e->environment == entry->environment.get() &&
						e->fromCycle == _fromCycle)
					{
						found = e;
						break;
					}
				}
				if (!found)
				{
					scoped_call_stack_info(TXT("not found, add"));
					em->parts.push_back(EnvironmentMix::Part(entry->environment.get(), 0.0f, _fromCycle));
					scoped_call_stack_info(TXT("not found, get pointer"));
					found = &em->parts.get_last();
				}
				if (found)
				{
					scoped_call_stack_info(TXT("found, alter target"));
					found->target += _strength * entry->mix;
				}
			}
		}
	}
}

void PilgrimageInstance::debug_log_environments(LogInfoContext & _log) const
{
	_log.log(TXT("environment pt : %.3f"), environmentPt);
	_log.log(TXT("environment cycle idx : %2i"), environmentCycleIdx);
	_log.log(TXT("environment cycle pt : %.3f"), environmentCyclePt);
	_log.log(TXT("environment exterior rotation (yaw) : %.3f"), environmentRotationYaw);
	LOG_INDENT(_log);
	for_every(em, environmentMixes)
	{
		_log.log(TXT("%S"), em->name.to_char());
		LOG_INDENT(_log);
		_log.log(TXT("mix"));
		{
			LOG_INDENT(_log);
			for_every(part, em->parts)
			{
				_log.log(TXT("%.3f -> %.3f: %S"), part->mix, part->target, part->environment->get_name().to_string().to_char());
			}
		}
		if (auto* exEnv = environments.get_existing(em->name))
		{
			if (auto* env = fast_cast<Framework::Environment>(exEnv->get()))
			{
				env->get_info().get_params().log(_log);
			}
		}
	}
}

void PilgrimageInstance::update_auto_environment_variables(Framework::Environment* _env)
{
	todo_note(TXT("put those as params"));
	float useNoonYaw = get_noon_yaw();
	float noonPitch = 60.0f;
	float sunOffZeroPlane = 10.0f;

	if (pilgrimage->get_forced_noon_yaw().is_set())
	{
		useNoonYaw = pilgrimage->get_forced_noon_yaw().get();
	}
	if (pilgrimage->get_forced_noon_pitch().is_set())
	{
		noonPitch = pilgrimage->get_forced_noon_pitch().get();
	}

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		if (Framework::BullshotSystem::access_lock().acquire_if_not_locked())
		{
			auto& bv = Framework::BullshotSystem::access_variables();
			if (auto* v = bv.get_existing<float>(NAME(bullshotNoonYaw)))
			{
				noonYaw = *v;
			}
			if (auto* v = bv.get_existing<float>(NAME(bullshotNoonPitch)))
			{
				noonPitch = *v;
			}
			Framework::BullshotSystem::access_lock().release();
		}
	}
#endif

	Matrix33 defaultSkyRotation = Rotator3(noonPitch - sunOffZeroPlane, useNoonYaw, 0.0f).to_quat().to_matrix_33();

	// adjust light dir using "light at" param
	// if for some reason light dir is adjusted, check if light at is added as an environment param
	auto *lightAtParam = _env->get_param(NAME(lightAt));
	auto *lightDirParam = _env->access_param(NAME(lightDir));
	if (lightAtParam && lightDirParam)
	{
		float useOffset = 1.0f;
		if (auto *lightSpecularSideParam = _env->get_param(NAME(lightSpecularSide)))
		{
			// moon in our plane
			useOffset = clamp(0.5f + lightSpecularSideParam->as<float>() * 6.0f, 0.0f, 1.0f);
		}
		lightDirParam->as<Vector3>() = -defaultSkyRotation.to_world(Rotator3(sunOffZeroPlane * useOffset, lightAtParam->as<float>() * (90.0f + sunOffZeroPlane * 1.5f * useOffset), 0.0f).to_quat().to_matrix_33()).get_y_axis();
	}

	auto *starsPtParam = _env->get_param(NAME(starsPt));
	auto *starsMatrixParam = _env->access_param(NAME(starsMatrix));
	if (starsPtParam && starsMatrixParam)
	{
		starsMatrixParam->as<Matrix33>() = defaultSkyRotation.to_world(Rotator3(0.0f, starsPtParam->as<float>() * 360.0f, 0.0f).to_quat().to_matrix_33());
	}
}

String PilgrimageInstance::get_random_seed_info() const
{
	String info = randomSeed.get_seed_string();
	return info;
}

void PilgrimageInstance::log(LogInfoContext& _log) const
{
	_log.log(TXT("pilgrimage : %S"), pilgrimage.get()? pilgrimage->get_name().to_string().to_char() : TXT("--"));
	_log.log(TXT("seed : %S"), randomSeed.get_seed_string().to_char());
	_log.log(TXT("play area : %S"), RoomGenerationInfo::get().get_play_area_rect_size().to_string().to_char());
	_log.log(TXT("main region : %p : %S"), mainRegion.get(), mainRegion.get() && mainRegion->get_region_type()? mainRegion->get_region_type()->get_name().to_string().to_char() : TXT("--"));
	g_pilgrimageDataMaintainer->log(_log);
}

bool PilgrimageInstance::has_been_killed(Framework::WorldAddress const& _worldAddress, int _poiIndex) const
{
	if (_worldAddress.is_valid() &&
		_poiIndex != NONE)
	{
		Concurrency::ScopedMRSWLockRead lock(killedInfoLock);
		for_every(ki, killedInfos)
		{
			if (ki->poiIdx == _poiIndex &&
				ki->worldAddress == _worldAddress)
			{
#ifdef DEBUG_KILLED_INFOS
				output(TXT("already killed address=\"%S\" poiIdx=\"%i\""), _worldAddress.to_string().to_char(), _poiIndex);
#endif
				return true;
			}
		}
	}
#ifdef DEBUG_KILLED_INFOS
	output(TXT("safe address=\"%S\" poiIdx=\"%i\""), _worldAddress.to_string().to_char(), _poiIndex);
#endif
	return false;
}

void PilgrimageInstance::mark_killed(Framework::WorldAddress const& _worldAddress, int _poiIndex)
{
	if (_worldAddress.is_valid() &&
		_poiIndex != NONE)
	{
#ifdef DEBUG_KILLED_INFOS
		output(TXT("mark killed address=\"%S\" poiIdx=\"%i\""), _worldAddress.to_string().to_char(), _poiIndex);
#endif
		Concurrency::ScopedMRSWLockWrite lock(killedInfoLock);
		PilgrimageKilledInfo ki;
		ki.worldAddress = _worldAddress;
		ki.poiIdx = _poiIndex;
		killedInfos.push_back(ki);
	}
}

float PilgrimageInstance::get_mix_environment_cycle_var(Name const& _environmentCycleName) const
{
	if (pilgrimage.is_set())
	{
		for_every_ref(c, pilgrimage->get_environment_cycles())
		{
			if (c->get_name() == _environmentCycleName)
			{
				return mixEnvironmentCycleVars[for_everys_index(c)];
			}
		}
	}
	warn(TXT("no environment cycle\"%S\""), _environmentCycleName.to_char());
	return 0.0f;
}

void PilgrimageInstance::set_mix_environment_cycle_var(Name const& _environmentCycleName, float _value)
{
	if (pilgrimage.is_set())
	{
		for_every_ref(c, pilgrimage->get_environment_cycles())
		{
			if (c->get_name() == _environmentCycleName)
			{
				mixEnvironmentCycleVars[for_everys_index(c)] = _value;
				return;
			}
		}
	}
	warn(TXT("no environment cycle\"%S\""), _environmentCycleName.to_char());
}

void PilgrimageInstance::adjust_starting_pilgrim_setup(PilgrimSetup& _pilgrimSetup) const
{
	if (auto* p = pilgrimage.get())
	{
		if (p->should_start_here_without_exms())
		{
			Concurrency::ScopedMRSWLockWrite lock(_pilgrimSetup.access_lock());
			for_count(int, hIdx, Hand::MAX)
			{
				auto& handSetup = _pilgrimSetup.access_hand((Hand::Type)hIdx);
				{
					Concurrency::ScopedSpinLock lock(handSetup.exmsLock);
					handSetup.remove_exms();
				}
			}
		}
		if (p->should_start_here_with_exms_from_last_setup())
		{
			Concurrency::ScopedMRSWLockWrite lock(_pilgrimSetup.access_lock());
			if (auto* persistence = Persistence::access_current_if_exists())
			{
				Concurrency::ScopedSpinLock lock(persistence->access_lock());
				auto& lastSetup = persistence->get_last_used_setup();
				for_count(int, hIdx, Hand::MAX)
				{
					auto& handSetup = _pilgrimSetup.access_hand((Hand::Type)hIdx);
					{
						Concurrency::ScopedSpinLock lock(handSetup.exmsLock);
						handSetup.activeEXM.exm = lastSetup.activeEXMs[hIdx];
						handSetup.passiveEXMs.clear();
						for_every(pexm, lastSetup.passiveEXMs[hIdx])
						{
							if (handSetup.passiveEXMs.has_place_left())
							{
								handSetup.passiveEXMs.push_back();
								handSetup.passiveEXMs.get_last().exm = *pexm;
							}
						}
						handSetup.keep_only_unlocked_exms();
					}
				}
			}
		}
		if (p->should_start_here_with_only_unlocked_exms())
		{
			Concurrency::ScopedMRSWLockWrite lock(_pilgrimSetup.access_lock());
			for_count(int, hIdx, Hand::MAX)
			{
				auto& handSetup = _pilgrimSetup.access_hand((Hand::Type)hIdx);
				{
					Concurrency::ScopedSpinLock lock(handSetup.exmsLock);
					handSetup.keep_only_unlocked_exms();
				}
			}
		}
		if (p->should_start_here_without_weapons())
		{
			Concurrency::ScopedMRSWLockWrite lock(_pilgrimSetup.access_lock());
			for_count(int, hIdx, Hand::MAX)
			{
				auto& handSetup = _pilgrimSetup.access_hand((Hand::Type)hIdx);	
				{
					Concurrency::ScopedSpinLock lock(handSetup.weaponSetupLock);
					handSetup.weaponSetup.clear();
				}
			}
		}
		if (p->should_start_here_with_weapons_from_armoury())
		{
			Concurrency::ScopedMRSWLockWrite lock(_pilgrimSetup.access_lock());
			if (auto* persistence = Persistence::access_current_if_exists())
			{
				Concurrency::ScopedSpinLock lock(persistence->access_lock());
				for_count(int, hIdx, Hand::MAX)
				{
					auto& handSetup = _pilgrimSetup.access_hand((Hand::Type)hIdx);
					{
						Concurrency::ScopedSpinLock lock(handSetup.weaponSetupLock);
						handSetup.weaponSetup.clear();

						for_every(wia, persistence->get_weapons_in_armoury())
						{
							if (wia->onPilgrimInfo.onPilgrim &&
								wia->onPilgrimInfo.mainWeaponForHand.is_set() &&
								wia->onPilgrimInfo.mainWeaponForHand.get() == hIdx)
							{
								handSetup.weaponSetup.copy_from(wia->setup);
								break;
							}
						}
					}
				}
			}
		}
		if (p->should_start_here_with_damaged_weapons())
		{
			Concurrency::ScopedMRSWLockWrite lock(_pilgrimSetup.access_lock());
			for_count(int, hIdx, Hand::MAX)
			{
				auto& handSetup = _pilgrimSetup.get_hand((Hand::Type)hIdx);
				{
					Concurrency::ScopedSpinLock lock(handSetup.weaponSetupLock);
					for_every(part, handSetup.weaponSetup.get_parts())
					{
						if (auto* wp = part->part.get())
						{
							if (auto* wpt = wp->get_weapon_part_type())
							{
								if (wpt->get_type() == WeaponPartType::GunCore)
								{
									wp->ex__set_damaged(EnergyCoef::half());
								}
							}
						}
					}
				}
			}
		}
	}
}

void PilgrimageInstance::refresh_force_instant_object_creation(Optional<bool> const& _newValue)
{
	if (_newValue.is_set())
	{
		forceInstantObjectCreation = _newValue.get();
	}
	game->force_instant_object_creation(forceInstantObjectCreation);
}

PilgrimageInstance* PilgrimageInstance::get_based_on_room(Framework::Room* _inRoom)
{
	if (auto* pi = get())
	{
		if (auto* npi = pi->nextPilgrimage.get())
		{
			if (_inRoom)
			{
				for_every(tr, pi->transitionRooms)
				{
					if (_inRoom == tr->room.get())
					{
						return npi;
					}
				}
			}
		}
		return pi;
	}
	return nullptr;
}

bool PilgrimageInstance::is_follow_guidance_blocked() const
{
	if (pilgrimage.get() &&
		pilgrimage->open_world__get_definition())
	{
		return pilgrimage->open_world__get_definition()->is_follow_guidance_blocked();
	}
	return false;
}

void PilgrimageInstance::reset_pilgrims_destination()
{
	pilgrimsDestination.clear();
	pilgrimsDestinationDevice = nullptr;
	pilgrimsDestinationSystem = Name::invalid();
	pilgrimsDestinationDeviceMainIMO.clear();
}

Framework::Room* PilgrimageInstance::get_pilgrims_destination(OPTIONAL_ OUT_ Name* _actualSystem, OPTIONAL_ OUT_ PilgrimageDevice const** _actualDevice, OPTIONAL_ OUT_ Framework::IModulesOwner** _actualDeviceIMO) const
{
	assign_optional_out_param(_actualSystem, pilgrimsDestinationSystem);
	assign_optional_out_param(_actualDevice, pilgrimsDestinationDevice);
	assign_optional_out_param(_actualDeviceIMO, pilgrimsDestinationDeviceMainIMO.get());
	return pilgrimsDestination.get();
}

//--

PilgrimageInstance::EnvironmentMix::EnvironmentMix(Name const & _name, Framework::EnvironmentType* _environment)
{
	name = _name;
	parts.push_back(Part(_environment, 1.0f, nullptr));
}

void PilgrimageInstance::EnvironmentMix::clear_targets()
{
	for_every(part, parts)
	{
		part->target = 0.0f;
	}
}

void PilgrimageInstance::EnvironmentMix::remove_unused()
{
	for (int i = 0; i < parts.get_size(); ++i)
	{
		if (parts[i].mix <= 0.001f &&
			parts[i].target <= 0.001f)
		{
			parts.remove_at(i);
			--i;
		}
	}
}

void PilgrimageInstance::EnvironmentMix::advance(float _deltaTime, Framework::Environment* _alter, bool _firstAdvance)
{
	bool anyTarget = false;
	for_every(part, parts)
	{
		if (part->target != 0.0f)
		{
			anyTarget = true;
		}
	}

	if (!anyTarget && !parts.is_empty())
	{
		parts.get_first().target = 1.0f;
	}

	bool firstInfo = true;
	float mixSoFar = 0.0f; // used for auto normalisation
	for_every(part, parts)
	{
		part->mix = blend_to_using_time(part->mix, part->target, _firstAdvance? 0.0f : 0.3f, _deltaTime);
		if (part->mix > 0.0f)
		{
			mixSoFar += part->mix;
			if (firstInfo)
			{
				_alter->access_info().set_params_from(part->environment->get_info(), part->fromCycle ? &part->fromCycle->get_keep_params() : nullptr);
			}
			else
			{
				_alter->access_info().lerp_params_with(part->mix / mixSoFar, part->environment->get_info(), part->fromCycle ? &part->fromCycle->get_keep_params() : nullptr);
			}
			firstInfo = false;
		}
	}
}
