#include "gameDirector.h"

#include "game.h"

#include "..\teaForGodTest.h"

#include "..\modules\custom\health\mc_healthRegen.h"

#include "..\library\missionDifficultyModifier.h"

#include "..\music\musicPlayer.h"

#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageInstance.h"

#include "..\story\storyChapter.h"
#include "..\story\storyScene.h"

#include "..\..\core\debug\debug.h"
#include "..\..\core\system\video\foveatedRenderingSetup.h"

#include "..\..\framework\debug\testConfig.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\sound\soundSample.h"
#include "..\..\framework\world\door.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// room/region parameters
DEFINE_STATIC_NAME(gameDirectorSafeSpace);
DEFINE_STATIC_NAME(gameStateSafePlace);

// room tags
DEFINE_STATIC_NAME(openWorldExterior);

// test tags
DEFINE_STATIC_NAME(testPilgrimage);
DEFINE_STATIC_NAME(testPilgrimageGameScript);
DEFINE_STATIC_NAME(testStoryFacts);
DEFINE_STATIC_NAME(gameTestScenarioName);

// POI
DEFINE_STATIC_NAME_STR(environmentAnchor, TXT("environment anchor"));

//

GameDirector* GameDirector::s_director = nullptr;

GameDirector::GameDirector()
{
	an_assert(!s_director);
	s_director = this;
}

GameDirector::~GameDirector()
{
	if (s_director == this)
	{
		s_director = nullptr;
	}
}

void GameDirector::reset()
{
	storyExecution.reset();
	deactivate();

	// update to reset other variables
	update(0.0f);

	state = State();

	doors.doors.clear();

	autoHostiles = AutoHostiles();

	//playerInfo.autoCreateLastMomentGameStateTimeLeft.clear();
	playerInfo = PlayerInfo();

	airProhibitedPlaces.places.clear();

	forPilgrimageInstance = nullptr;
}

void GameDirector::activate()
{
	isActive = true;

	playerInfo.autoCreateLastMomentGameStateTimeLeft.clear();

#ifndef BUILD_PUBLIC_RELEASE
	if (auto* testStoryFacts = Framework::TestConfig::get_params().get_existing<Tags>(NAME(testStoryFacts)))
	{
		Concurrency::ScopedMRSWLockWrite lock(storyExecution.access_facts_lock());
		storyExecution.access_facts().set_tags_from(*testStoryFacts);
	}
#endif
}

void GameDirector::deactivate()
{
	isActive = false;
}

void GameDirector::take_care_of_orphaned(Framework::IModulesOwner* _imo)
{
	SafePtr<Framework::IModulesOwner> simo(_imo);

	Concurrency::ScopedSpinLock lock(orphansLock);
	orphans.push_back(simo);
}

GameDirector::DoorState GameDirector::get_door_state(Framework::Door const* _door) const
{
	GameDirector::DoorState ds;

	Concurrency::ScopedMRSWLockRead lock(doors.lock);

	for_every(d, doors.doors)
	{
		if (d->door == _door)
		{
			ds.opening = d->timeToOpen.is_set();
			ds.overridingLock = d->overridingLock.is_set();
			ds.overridingLockHighlight = d->overridingLock.get(0.0f) < d->overridingLockHighlight.get(0.0f);
			ds.timeToOpen = d->timeToOpen.get(0.0f);
			ds.timeToOpenPeriod = d->timeToOpenPeriod;
			break;
		}
	}

	return ds;
}

void GameDirector::set_door_closing(Framework::Door const* _door, float _timeToClose, DoorClosingParams const& _params)
{
	Concurrency::ScopedMRSWLockWrite lock(doors.lock);

	for_every(d, doors.doors)
	{
		if (d->door == _door)
		{
			d->timeToOpen.clear();
			d->overridingLock.clear();
			d->timeToClose = _timeToClose;
			d->closedSound = _params.closedSound;
			return;
		}
	}

	Doors::Door d;
	d.door = _door;
	d.timeToClose = _timeToClose;
	d.closedSound = _params.closedSound;
	doors.doors.push_back(d);
}

void GameDirector::set_door_override_lock(Framework::Door const* _door, bool _overrideLock)
{
	Concurrency::ScopedMRSWLockWrite lock(doors.lock);

	for_every(d, doors.doors)
	{
		if (d->door == _door)
		{
			if (_overrideLock)
			{
				d->timeToOpen.clear();
				d->timeToClose.clear();
				d->overridingLock = 0.0f; // will reset
			}
			else
			{
				doors.doors.remove_at(for_everys_index(d));
			}
			return;
		}
	}

	if (_overrideLock)
	{
		Doors::Door d;
		d.door = _door;
		d.overridingLock = 0.0f; // will reset
		doors.doors.push_back(d);
	}
}

void GameDirector::set_door_opening(Framework::Door const* _door, float _timeToOpen, DoorOpeningParams const& _params)
{
	Concurrency::ScopedMRSWLockWrite lock(doors.lock);

	for_every(d, doors.doors)
	{
		if (d->door == _door)
		{
			d->timeToClose.clear();
			d->overridingLock.clear();
			d->timeToOpen = _timeToOpen;
			d->timeToOpenPeriod = _params.timeToOpenPeriod.get(0.5f);
			d->openingSound = _params.openingSound;
			d->openSound = _params.openSound;
			return;
		}
	}

	Doors::Door d;
	d.door = _door;
	d.timeToOpen = _timeToOpen;
	d.timeToOpenPeriod = _params.timeToOpenPeriod.get(0.5f);
	d.openingSound = _params.openingSound;
	d.openSound = _params.openSound;
	doors.doors.push_back(d);
}

void GameDirector::extend_door_opening(Framework::Door const* _door, int _extendByPeriodNum, Optional<float> const& _maxTimeToExtend)
{
	Concurrency::ScopedMRSWLockWrite lock(doors.lock);

	for_every(d, doors.doors)
	{
		if (d->door == _door)
		{
			d->timeToClose.clear();
			if (d->timeToOpen.is_set() &&
				d->timeToOpen.get() < _maxTimeToExtend.get(d->timeToOpenPeriod))
			{
				d->timeToOpen = d->timeToOpen.get() + (float)_extendByPeriodNum * d->timeToOpenPeriod;
			}
			return;
		}
	}
}

void GameDirector::do_clean_start_for(PilgrimageInstance const* _instance)
{
	if (_instance)
	{
		if (auto* p = _instance->get_pilgrimage())
		{
			usingDefinition = p->get_game_director_definition();
			storyExecution.do_clean_start_for(usingDefinition.storyChapter.get());
		}
	}
}

void GameDirector::update(float _deltaTime)
{
	MEASURE_PERFORMANCE(gameDirector);

	{
		bool changed = false;
		changed |= System::FoveatedRenderingSetup::set_temporary_foveation_level_offset(state.temporaryFoveationLevelOffset);
		if (changed)
		{
			System::FoveatedRenderingSetup::force_set_foveation();
		}
	}

	// cease orphans
	if (! orphans.is_empty())
	{
		// check one at a time
		Concurrency::ScopedSpinLock lock(orphansLock);
		orphanIdx = mod(orphanIdx, orphans.get_size());
		if (orphans.is_index_valid(orphanIdx))
		{
			auto& pOrphan = orphans[orphanIdx];
			if (auto* orphan = pOrphan.get())
			{
				if (orphan->get_room_distance_to_recently_seen_by_player() > 2)
				{
					orphan->cease_to_exist(false);
					orphans.remove_fast_at(orphanIdx);
				}
			}
			else
			{
				orphans.remove_fast_at(orphanIdx);
			}
		}
		++orphanIdx;
	}

	{
		float const blendTime = state.statusBlendTime;
		state.healthStatus = blend_to_using_speed_based_on_time(state.healthStatus, state.healthStatusBlocked? 0.0f : 1.0f, blendTime, _deltaTime);
		state.ammoStatus = blend_to_using_speed_based_on_time(state.ammoStatus, state.ammoStatusBlocked? 0.0f : 1.0f, blendTime, _deltaTime);
	}

	if (state.game.forceNoViolenceTimeLeft.is_set())
	{
		state.game.forceNoViolenceTimeLeft = state.game.forceNoViolenceTimeLeft.get() - _deltaTime;
		if (state.game.forceNoViolenceTimeLeft.get() <= 0.0f)
		{
			state.game.forceNoViolenceTimeLeft.clear();
		}
	}

	// regain trust if narrative not active
	state.narrativeTrustLost &= ! state.narrativeRequested;

	{	// doors
		Concurrency::ScopedMRSWLockWrite lock(doors.lock);
		bool mayRequireCleanUp = false;
		for_every(d, doors.doors)
		{
			if (d->timeToOpen.is_set())
			{
				float prev = d->timeToOpen.get();
				d->timeToOpen = d->timeToOpen.get() - _deltaTime;
				if (d->timeToOpen.get() <= 0.0f)
				{
					mayRequireCleanUp = true;
					d->timeToOpen.clear();
					if (auto* ad = d->door.get())
					{
						ad->set_operation(Framework::DoorOperation::StayOpen, Framework::Door::DoorOperationParams().allow_auto().force_operation());
						if (auto* os = d->openSound.get())
						{
							if (auto* ad = d->door.get())
							{
								ad->access_sounds().play(os, ad);
							}
						}
					}
				}
				else
				{
					if (d->timeToOpenPeriod > 0.0f)
					{
						float tp = mod(prev, d->timeToOpenPeriod) / d->timeToOpenPeriod;
						float t = mod(d->timeToOpen.get(), d->timeToOpenPeriod) / d->timeToOpenPeriod;
						if (t > tp)
						{
							if (auto* os = d->openingSound.get())
							{
								if (auto* ad = d->door.get())
								{
									ad->access_sounds().play(os, ad);
								}
							}
						}
					}
				}
			}
			if (d->timeToClose.is_set())
			{
				d->timeToClose = d->timeToClose.get() - _deltaTime;
				if (d->timeToClose.get() <= 0.0f)
				{
					mayRequireCleanUp = true; 
					d->timeToClose.clear();
					if (auto* ad = d->door.get())
					{
						ad->set_operation(Framework::DoorOperation::StayClosed, Framework::Door::DoorOperationParams().force_operation());
						if (auto* cs = d->closedSound.get())
						{
							ad->access_sounds().play(cs, ad);
						}
					}
				}
			}
			if (d->overridingLock.is_set())
			{
				d->overridingLock = d->overridingLock.get() - _deltaTime;
				if (d->overridingLock.get() < 0.0f || !d->overridingLockHighlight.is_set())
				{
					d->overridingLock = rg.get_float(0.3f, 1.0f);
					d->overridingLockHighlight = d->overridingLock.get() * rg.get_float(0.1f, 0.8f);
				}
			}
		}
		if (mayRequireCleanUp)
		{
			for (int i = 0; i < doors.doors.get_size(); ++i)
			{
				if (!doors.doors[i].timeToOpen.is_set() &&
					!doors.doors[i].timeToClose.is_set() &&
					!doors.doors[i].overridingLock.is_set())
				{
					doors.doors.remove_fast_at(i);
					--i;
				}
			}
		}
	}

	if (!is_active())
	{
		storyExecution.stop();

		state.inSafeSpace = false;
		state.blockHostileSpawns = false;
		state.blockHostileSpawnsDueToNarrative = false;
		state.blockHostileSpawnsDueToBlockedAwayCells = false;
		state.forceHostileSpawns = false;
		state.hideImmobileHostiles = false;
		state.blockDoors = false;
		state.blockElevators = false;
	}
	else
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testPilgrimage), Framework::LibraryName()).is_valid() ||
			 Framework::TestConfig::get_params().get_value<bool>(NAME(testPilgrimageGameScript), false) ||
			 Framework::TestConfig::get_params().get_value<Name>(NAME(gameTestScenarioName), Name::invalid()).is_valid())
#endif
		{
			PilgrimageInstance const* currentPilgrimageInstance = PilgrimageInstance::get();
			if (forPilgrimageInstance != currentPilgrimageInstance)
			{
				forPilgrimageInstance = currentPilgrimageInstance;
				if (forPilgrimageInstance)
				{
					if (auto* p = forPilgrimageInstance->get_pilgrimage())
					{
						usingDefinition = p->get_game_director_definition();
						storyExecution.set_chapter(usingDefinition.storyChapter.get());

						if (usingDefinition.startWithBlockedDoors)
						{
							state.blockDoors = true;
						}
					}
				}
			}
		}

		storyExecution.update(_deltaTime);

		// manage narrative change
		if (state.narrativeActive ^ state.narrativeRequested)
		{
			state.narrativeActive = state.narrativeRequested;
		}

		// manage auto hostiles
		{
			auto useAutoHostiles = prepare_auto_hostiles_to_use();
			Range fullHostilesTimeInMinutes = useAutoHostiles.fullHostilesTimeInMinutes.get(Range::empty);
			Range normalHostilesTimeInMinutes = useAutoHostiles.normalHostilesTimeInMinutes.get(Range::empty);
			if (state.autoHostilesAllowed)
			{
				autoHostiles.requestedActive = !fullHostilesTimeInMinutes.is_empty() ||
					!normalHostilesTimeInMinutes.is_empty();
			}
			else
			{
				autoHostiles.requestedActive = false;
			}

			bool autoHostilesRequestedActiveNow = !state.narrativeRequested && autoHostiles.requestedActive;
			if (autoHostilesRequestedActiveNow ^ autoHostiles.active)
			{
				autoHostiles.active = autoHostilesRequestedActiveNow;
				if (autoHostiles.active)
				{
					autoHostiles.hostilesActive = false;
					autoHostiles.fullHostiles = false;
					autoHostiles.timeLeft = 0.0f;
				}
			}

			state.blockHostileSpawnsDueToNarrative = state.narrativeActive;

			state.blockHostileSpawnsDueToBlockedAwayCells = false;
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				state.blockHostileSpawnsDueToBlockedAwayCells = piow->should_blocked_away_cells_distance_block_hostiles();
			}

			if (autoHostiles.active)
			{
				autoHostiles.timeLeft -= _deltaTime;
				if (autoHostiles.timeLeft <= 0.0f)
				{
					if (!autoHostiles.hostilesActive &&
						!fullHostilesTimeInMinutes.is_empty() &&
						!normalHostilesTimeInMinutes.is_empty())
					{
						autoHostiles.hostilesActive = true;
						autoHostiles.fullHostiles = rg.get_chance(useAutoHostiles.fullHostilesChance.get(0.2f));
					}
					else if (!autoHostiles.hostilesActive &&
						!fullHostilesTimeInMinutes.is_empty())
					{
						autoHostiles.hostilesActive = true;
						autoHostiles.fullHostiles = true;
					}
					else if (!autoHostiles.hostilesActive &&
						!normalHostilesTimeInMinutes.is_empty())
					{
						autoHostiles.hostilesActive = true;
						autoHostiles.fullHostiles = false;
					}
					else
					{
						autoHostiles.hostilesActive = false;
						autoHostiles.fullHostiles = false;
					}

					{
						// working in minutes
						Range useForTimeLeft = autoHostiles.hostilesActive
							? (autoHostiles.fullHostiles
								? fullHostilesTimeInMinutes
								: normalHostilesTimeInMinutes)
							: useAutoHostiles.breakTimeInMinutes.get(Range::empty);
						if (useForTimeLeft.is_empty())
						{
							autoHostiles.timeLeft = 1.0f;
						}
						else
						{
							autoHostiles.timeLeft = rg.get(useForTimeLeft);
						}
						// move to seconds
						autoHostiles.timeLeft *= 60.0f;
					}
				}

				if (autoHostiles.hostilesActive)
				{
					state.blockHostileSpawns = false;
					state.forceHostileSpawns = autoHostiles.fullHostiles;
				}
				else
				{
					state.blockHostileSpawns = true;
					state.forceHostileSpawns = false;
				}
			}
			else
			{
				// we don't use auto hostiles, so just let them be as they are
				state.blockHostileSpawns = false;
				state.forceHostileSpawns = false;
			}
		}

#ifdef TEST_GAME_DIRECTOR
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->has_key_been_pressed(::System::Key::G))
		{
			state.forceHostileSpawns = !state.forceHostileSpawns;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::H))
		{
			state.blockHostileSpawns = !state.blockHostileSpawns;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::J))
		{
			state.hideImmobileHostiles = !state.hideImmobileHostiles;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::U))
		{
			state.blockDoors = !state.blockDoors;
		}
		if (::System::Input::get()->has_key_been_pressed(::System::Key::Y))
		{
			state.blockElevators = !state.blockElevators;
		}
#endif
#endif
#endif

		if (state.forceHostileSpawns)
		{
			// force means force
			state.blockHostileSpawns = false;
			state.blockHostileSpawnsDueToNarrative = false;
			state.blockHostileSpawnsDueToBlockedAwayCells = false;
		}

		playerInfo.update(this, _deltaTime);

		state.inSafeSpace = playerInfo.inSafeSpace;
	}

	// propagate further
	{
		Framework::Door::hackCloseAllDoor = state.blockDoors;
	}
}

GameDirectorDefinition::AutoHostiles GameDirector::prepare_auto_hostiles_to_use() const
{
	GameDirectorDefinition::AutoHostiles useAutoHostiles = usingDefinition.autoHostiles;
	if (auto* ms = MissionState::get_current())
	{
		if (auto* mdm = ms->get_difficulty_modifier())
		{
			useAutoHostiles.override_with(mdm->get_auto_hostiles());
		}
	}
	return useAutoHostiles;
}

void GameDirector::log(LogInfoContext& _log) const
{
	_log.set_colour(NP);
	_log.log(TXT("foveated rendering level offset : %.3f"), state.temporaryFoveationLevelOffset);
	if (!is_active())
	{
		_log.set_colour(Colour::grey);
		_log.log(TXT("not active"));
		_log.set_colour();
	}
	else
	{
		_log.set_colour(state.inSafeSpace ? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("%S : in safe space"), state.inSafeSpace ? TXT("YES") : TXT(" no"));
		_log.set_colour(state.narrativeActive? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("%S : narrative"), state.narrativeActive ? TXT("YES") : TXT(" no"));
		_log.set_colour(state.activeScene ? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("%S : active scene"), state.activeScene ? TXT("YES") : TXT(" no"));
		_log.set_colour(state.weaponsBlocked ? Optional<Colour>(Colour::red) : NP);
		_log.log(TXT("%S : block weapons"), state.blockDoors ? TXT("YES") : TXT(" no"));
		_log.set_colour(state.mapBlocked ? Optional<Colour>(Colour::red) : NP);
		_log.log(TXT("%S : block map"), state.mapBlocked ? TXT("YES") : TXT(" no"));
		_log.set_colour(state.blockDoors ? Optional<Colour>(Colour::red) : NP);
		_log.log(TXT("%S : block doors"), state.blockDoors ? TXT("YES") : TXT(" no"));
		_log.set_colour(state.blockElevators ? Optional<Colour>(Colour::red) : NP);
		_log.log(TXT("%S : block elevators"), state.blockElevators ? TXT("YES") : TXT(" no"));
		_log.set_colour(should_block_hostile_spawns()? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("%S : block hostile spawns"), state.blockHostileSpawns || state.blockHostileSpawnsDueToNarrative || state.blockHostileSpawnsDueToBlockedAwayCells ? TXT("YES") : TXT(" no"));
		_log.set_colour(state.blockHostileSpawns ? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("  %c : block hostile spawns in general"), state.blockHostileSpawns ? 'y' : 'n');
		_log.set_colour(state.blockHostileSpawnsDueToNarrative ? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("  %c : block hostile spawns due to narrative"), state.blockHostileSpawnsDueToNarrative ? 'y' : 'n');
		_log.set_colour(state.blockHostileSpawnsDueToBlockedAwayCells ? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("  %c : block hostile spawns due to blocked away cells"), state.blockHostileSpawnsDueToBlockedAwayCells? 'y' : 'n');
		_log.set_colour(state.forceHostileSpawns ? Optional<Colour>(Colour::red) : NP);
		_log.log(TXT("%S : force hostile spawns"), state.forceHostileSpawns ? TXT("YES") : TXT(" no"));
		_log.set_colour(!state.blockHostileSpawns && state.autoHostilesAllowed ? Optional<Colour>(Colour::orange) : NP);
		_log.log(TXT("%S : auto hostiles allowed"), state.autoHostilesAllowed ? TXT("YES") : TXT(" no"));
		_log.set_colour(autoHostiles.active ? Optional<Colour>(Colour::red) : NP);
		_log.log(TXT("%S : auto hostiles%S time left: %.3f"), autoHostiles.active ? TXT("YES") : TXT(" no"), autoHostiles.requestedActive? TXT(" (requested to be active)") : TXT(""), autoHostiles.timeLeft);
		_log.set_colour(state.hideImmobileHostiles ? Optional<Colour>(Colour::green) : NP);
		_log.log(TXT("%S : hide immobile hostiles"), state.hideImmobileHostiles ? TXT("YES") : TXT(" no"));
		_log.set_colour();
		{
			_log.log(TXT("game"));
			LOG_INDENT(_log);
			_log.set_colour(state.game.forceNoViolenceTimeLeft.is_set() ? Optional<Colour>(Colour::green) : NP);
			_log.log(TXT("%S : force no violence"), state.game.forceNoViolenceTimeLeft.is_set() ? TXT("YES") : TXT(" no"));
			_log.set_colour(state.game.objectiveRequiresNoViolence ? Optional<Colour>(Colour::green) : NP);
			_log.log(TXT("%S : objective requires no violence"), state.game.objectiveRequiresNoViolence ? TXT("YES") : TXT(" no"));
			_log.set_colour(state.game.elevatorsFollowPlayerOnly ? Optional<Colour>(Colour::green) : NP);
			_log.log(TXT("%S : elevators follow player only"), state.game.elevatorsFollowPlayerOnly ? TXT("YES") : TXT(" no"));
		}
		{
			int hostiles = 0;
			int hostilesAll = 0;
			int hostilesNonHostile = 0;
			{
				Concurrency::ScopedSpinLock lock(registeredHostileAIsLock);
				hostilesAll = registeredHostileAIs.get_size();
				hostiles += hostilesAll;
			}
			{
				Concurrency::ScopedSpinLock lock(nonHostileAIsLock);
				hostilesNonHostile = nonHostileAIs.get_size();
				hostiles -= hostilesNonHostile;
			}
			if (should_block_hostile_spawns() || is_in_safe_space())
			{
				_log.set_colour(hostiles > 0 ? Colour::orange : Colour::green);
			}
			_log.log(TXT("%3i : registered hostile AI (%i-%i)"), hostiles, hostilesAll, hostilesNonHostile);
			_log.set_colour();
		}
		{
			int immobileHostiles = 0;
			int immobileHostilesAll = 0;
			int immobileHostilesHidden = 0;
			{
				Concurrency::ScopedSpinLock lock(registeredImmobileHostileAIsLock);
				immobileHostilesAll = registeredImmobileHostileAIs.get_size();
				immobileHostiles += immobileHostilesAll;
			}
			{
				Concurrency::ScopedSpinLock lock(hiddenImmobileHostileAIsLock);
				immobileHostilesHidden = hiddenImmobileHostileAIs.get_size();
				immobileHostiles -= immobileHostilesHidden;
			}
			if (should_hide_immobile_hostiles())
			{
				_log.set_colour(immobileHostiles > 0? Colour::orange : Colour::green);
			}
			_log.log(TXT("%3i : registered immobile hostile AI (%i-%i)"), immobileHostiles, immobileHostilesAll, immobileHostilesHidden);
			_log.set_colour();
		}
		{
			_log.log(TXT("air prohibited places"));
			LOG_INDENT(_log);
			Concurrency::ScopedMRSWLockRead lock(airProhibitedPlaces.placesLock);
			for_every(ao, airProhibitedPlaces.places)
			{
				_log.log(TXT("%S (%.0fm)"), ao->id.to_char(), ao->radius);
			}
		}
		{
			storyExecution.log(_log);
		}
	}
}

void GameDirector::register_hostile_ai(Framework::IModulesOwner* _imo)
{
	SafePtr<Framework::IModulesOwner> safeIMO(_imo);

	Concurrency::ScopedSpinLock lock(registeredHostileAIsLock);

	registeredHostileAIs.push_back_unique(safeIMO);
}

void GameDirector::unregister_hostile_ai(Framework::IModulesOwner* _imo)
{
	SafePtr<Framework::IModulesOwner> safeIMO(_imo);

	Concurrency::ScopedSpinLock lock(registeredHostileAIsLock);

	registeredHostileAIs.remove_fast(safeIMO);
}

void GameDirector::mark_hostile_ai_non_hostile(Framework::IModulesOwner * _imo, bool _nonHostile)
{
	SafePtr<Framework::IModulesOwner> safeIMO(_imo);

	Concurrency::ScopedSpinLock lock(nonHostileAIsLock);

	if (_nonHostile)
	{
		nonHostileAIs.push_back_unique(safeIMO);
#ifdef AN_DEVELOPMENT
		Concurrency::ScopedSpinLock lock(registeredHostileAIsLock);
		an_assert(registeredHostileAIs.does_contain(safeIMO));
#endif
	}
	else
	{
		nonHostileAIs.remove_fast(safeIMO);
	}
}

void GameDirector::register_immobile_hostile_ai(Framework::IModulesOwner* _imo)
{
	SafePtr<Framework::IModulesOwner> safeIMO(_imo);

	Concurrency::ScopedSpinLock lock(registeredImmobileHostileAIsLock);

	registeredImmobileHostileAIs.push_back_unique(safeIMO);
}

void GameDirector::unregister_immobile_hostile_ai(Framework::IModulesOwner* _imo)
{
	SafePtr<Framework::IModulesOwner> safeIMO(_imo);

	Concurrency::ScopedSpinLock lock(registeredImmobileHostileAIsLock);

	registeredImmobileHostileAIs.remove_fast(safeIMO);
}

void GameDirector::mark_immobile_hostile_ai_hidden(Framework::IModulesOwner* _imo, bool _hidden)
{
	SafePtr<Framework::IModulesOwner> safeIMO(_imo);

	Concurrency::ScopedSpinLock lock(hiddenImmobileHostileAIsLock);

	if (_hidden)
	{
		hiddenImmobileHostileAIs.push_back_unique(safeIMO);
#ifdef AN_DEVELOPMENT
		Concurrency::ScopedSpinLock lock(registeredImmobileHostileAIsLock);
		an_assert(registeredImmobileHostileAIs.does_contain(safeIMO));
#endif
	}
	else
	{
		hiddenImmobileHostileAIs.remove_fast(safeIMO);
	}
}

//

void GameDirector::PlayerInfo::update(GameDirector* gd, float _deltaTime)
{
	Framework::Room* nowInRoom = nullptr;

	Optional<VectorInt2> newInCell;
	if (auto* g = Game::get_as<Game>())
	{
		if (auto* pa = g->access_player().get_actor())
		{
			nowInRoom = pa->get_presence()->get_in_room();

			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				newInCell = piow->find_cell_at(pa);
			}
		}
	}

	// auto store persistence
	{
		bool autoStoringPersistenceAllowed = false;
		if (auto* pi = PilgrimageInstance::get())
		{
			if (auto* p = pi->get_pilgrimage())
			{
				if (p->does_allow_auto_store_of_persistence())
				{
					autoStoringPersistenceAllowed = true;
				}
			}
		}
		
		if (autoStoringPersistenceAllowed)
		{
			{
				float atLeast = 5.0f;
				if (!lastAutoStoringPersistenceTimeLeft.is_set())
				{
					lastAutoStoringPersistenceTimeLeft = atLeast;
				}
				lastAutoStoringPersistenceTimeLeft = max(0.0f, lastAutoStoringPersistenceTimeLeft.get() - _deltaTime);
			}

			if (autoStoringPersistenceRequired && lastAutoStoringPersistenceTimeLeft.get() == 0)
			{
				autoStoringPersistenceRequired = false;
				lastAutoStoringPersistenceTimeLeft.clear(); // will trigger time to prevent saving on each moment

				if (auto* g = Game::get_as<Game>())
				{
					g->add_async_store_persistence(true);
				}
			}
		}
		else
		{
			autoStoringPersistenceRequired = false;
		}
	}

	// store game state
	{
		{
			float atLeast = 10.0f;
			if (!autoCreateLastMomentGameStateTimeLeft.is_set())
			{
				autoCreateLastMomentGameStateTimeLeft = atLeast;
			}
			autoCreateLastMomentGameStateTimeLeft = max(0.0f, autoCreateLastMomentGameStateTimeLeft.get() - _deltaTime);
		}

		bool storingGameStateAllowed = true;
		if (auto* pi = PilgrimageInstance::get())
		{
			if (auto* p = pi->get_pilgrimage())
			{
				if (!p->does_allow_store_intermediate_game_state())
				{
					storingGameStateAllowed = false;
				}
			}
		}

		if (gd->is_storing_intermediate_game_states_blocked())
		{
			storingGameStateAllowed = false;
		}

		if (storingGameStateAllowed)
		{
			bool saveOnCellChange = false;
			bool saveOnTimeInterval = false;

			if (GameSettings::get().difficulty.autoMapOnly)
			{
				saveOnTimeInterval = true;
			}
			else
			{
				saveOnCellChange = true;
			}

			if (checkedForRoom != nowInRoom)
			{
				bool changedCell = newInCell != cellAt;
				Optional<VectorInt2> prevCell = cellAt;
				cellAt = newInCell;

				checkedForRoom = nowInRoom;
				if (checkedForRoom)
				{
					inSafeSpace = checkedForRoom->get_value<bool>(NAME(gameDirectorSafeSpace), false, false);
				}
				else
				{
					inSafeSpace = true;
				}

				if (checkedForRoom && checkedForRoom->get_value<bool>(NAME(gameStateSafePlace), false, false))
				{
					if (!playerInGameStateSafePlace)
					{
						playerInGameStateSafePlaceTime = 0.0f;
					}
					playerInGameStateSafePlace = true;
				}
				else
				{
					if (playerInGameStateSafePlace)
					{
						playerOutGameStateSafePlaceTime = 0.0f;
					}
					playerInGameStateSafePlace = false;
				}

				if (saveOnCellChange &&
					changedCell && prevCell.is_set() && autoCreateLastMomentGameStateTimeLeft.get() <= 0.0f)
				{
					bool okToSave = true;
					// always allow to save, it might be abused, but let the players have fun the way they want
					/*
					if (MusicPlayer::is_playing_combat())
					{
						okToSave = false;
					}
					if (! okToSave)
					{
						okToSave = true; // even with combat music, if we don't see anyone, it's ok to save?
						Concurrency::ScopedSpinLock lock(gd->registeredHostileAIsLock);
						for_every_ref(imo, gd->registeredHostileAIs)
						{
							if (auto* p = imo->get_presence())
							{
								if (auto* r = p->get_in_room())
								{ 
									if (r->was_recently_seen_by_player())
									{
										okToSave = false;
										break;
									}
								}
							}
						}
					}
					*/
					if (okToSave)
					{
						if (auto* g = Game::get_as<Game>())
						{
							if (auto* pa = g->access_player().get_actor())
							{
								if (auto* h = pa->get_custom<CustomModules::HealthRegen>())
								{
									if (h->get_time_after_damage() > 5.0f)
									{
										autoCreateLastMomentGameStateTimeLeft.clear();
										if (auto* piow = PilgrimageInstanceOpenWorld::get())
										{
											// we will be saving due to changing cell
											piow->clear_last_visited_haven();
										}
										output(TXT("store game state - auto when changing cell"));
										g->add_async_store_game_state(true, NP);
									}
								}
							}
						}
					}
				}
			}

			if (saveOnTimeInterval)
			{
				float interval = 3.0f * 60.0f;
				if (!autoCreateLastMomentGameStateTimeLeft.is_set())
				{
					autoCreateLastMomentGameStateTimeLeft = interval;
				}
				autoCreateLastMomentGameStateTimeLeft = autoCreateLastMomentGameStateTimeLeft.get() - _deltaTime;
				if (autoCreateLastMomentGameStateTimeLeft.get() < 0.0f)
				{
					if (auto* g = Game::get_as<Game>())
					{
						if (auto* pa = g->access_player().get_actor())
						{
							if (auto* h = pa->get_custom<CustomModules::HealthRegen>())
							{
								if (h->get_time_after_damage() > 10.0f)
								{
									autoCreateLastMomentGameStateTimeLeft = interval;
#ifdef AN_ALLOW_EXTENSIVE_LOGS
									output(TXT("store game state - auto on time interval"));
#endif
									g->add_async_store_game_state(true, NP);
								}
							}
						}
					}
					autoCreateLastMomentGameStateTimeLeft = max(10.0f, autoCreateLastMomentGameStateTimeLeft.get()); // try again if failed, if succeed, we'll wait
				}
			}
		}

		if (storingGameStateAllowed && playerInGameStateSafePlace)
		{
			float prevInTime = playerInGameStateSafePlaceTime;
			float const timeCool = 1.0;
			playerInGameStateSafePlaceTime += _deltaTime;

			if (playerOutGameStateSafePlaceTime > 5.0f && // player was not in for a while - this is mostly to prevent savegame instantly after starting from a checkpoint
				prevInTime < playerInGameStateSafePlaceTime &&
				prevInTime < timeCool &&
				playerInGameStateSafePlaceTime >= timeCool &&
				cellAt.is_set())
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->set_last_visited_haven(cellAt.get());
				}
				if (storingGameStateAllowed)
				{
					output(TXT("store game state - player in safe place"));
					Game::get_as<Game>()->add_async_store_game_state(true, GameStateLevel::Checkpoint);
				}
			}
		}
		else
		{
			playerOutGameStateSafePlaceTime += _deltaTime;
		}
	}
}

void GameDirector::set_respawn_and_continue_blocked(bool _v)
{
	Concurrency::ScopedSpinLock lock(state.lock);
	state.respawnAndContinueBlocked = _v;
}

void GameDirector::set_immortal_health_regen_blocked(bool _v)
{
	Concurrency::ScopedSpinLock lock(state.lock);
	state.immortalHealthRegenBlocked = _v;
}

void GameDirector::set_infinite_ammo_blocked(bool _v)
{
	Concurrency::ScopedSpinLock lock(state.lock);
	state.infiniteAmmoBlocked = _v;
}

void GameDirector::set_ammo_storages_unavailable(bool _v)
{
	Concurrency::ScopedSpinLock lock(state.lock);
	state.ammoStoragesUnavailable = _v;
}

void GameDirector::set_real_movement_input_tip_blocked(bool _v)
{
	Concurrency::ScopedSpinLock lock(state.lock);
	state.realMovementInputTipBlocked = _v;
}

void GameDirector::add_air_prohibited_place(Name const& _id, Segment const& _prohibitedPlace, float _radius, bool _relativeToPlayersTileLoc)
{
	Vector3 offset = Vector3::zero;

	if (_relativeToPlayersTileLoc)
	{
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					Optional<VectorInt2> cellCoord = piow->find_cell_at(pa);
					if (cellCoord.is_set())
					{
						offset += cellCoord.get().to_vector2().to_vector3() * piow->get_cell_size();
					}
				}
				if (auto* r = pa->get_presence()->get_in_room())
				{
					if (r->get_tags().has_tag(NAME(openWorldExterior)))
					{
						Framework::PointOfInterestInstance* foundPOI = nullptr;
						if (r->find_any_point_of_interest(NAME(environmentAnchor), foundPOI))
						{
							Transform ea = foundPOI->calculate_placement();
							offset += ea.location_to_local(pa->get_presence()->get_placement().get_translation()) * Vector3(1.0f, 1.0f, 0.0f);
						}
					}
				}
			}
		}
	}

	AirProhibitedPlace ao;
	ao.id = _id;;
	ao.radius = _radius;
	ao.place = Segment(_prohibitedPlace.get_start() + offset, _prohibitedPlace.get_end() + offset);

	{
		Concurrency::ScopedMRSWLockWrite lock(airProhibitedPlaces.placesLock);
		airProhibitedPlaces.places.push_back(ao);
	}
}

void GameDirector::remove_air_prohibited_place(Name const& _id)
{
	{
		Concurrency::ScopedMRSWLockWrite lock(airProhibitedPlaces.placesLock);
		for_every(ao, airProhibitedPlaces.places)
		{
			if (ao->id == _id)
			{
				airProhibitedPlaces.places.remove_at(for_everys_index(ao));
			}
		}
	}
}

void GameDirector::remove_all_air_prohibited_places()
{
	Concurrency::ScopedMRSWLockWrite lock(airProhibitedPlaces.placesLock);
	airProhibitedPlaces.places.clear();
}

int GameDirector::get_air_prohibited_places(OUT_ ArrayStack<AirProhibitedPlace>* _prohibitedPlaces) const
{
	Concurrency::ScopedMRSWLockRead lock(airProhibitedPlaces.placesLock);
	if (!_prohibitedPlaces)
	{
		return airProhibitedPlaces.places.get_size();
	}
	else
	{
		int added = 0;
		for_every(ao, airProhibitedPlaces.places)
		{
			if (_prohibitedPlaces->has_place_left())
			{
				_prohibitedPlaces->push_back(*ao);
				++added;
			}
		}
		return added;
	}
}

bool GameDirector::is_violence_disallowed()
{
	if (auto* gd = GameDirector::get())
	{
		if (gd->state.game.forceNoViolenceTimeLeft.is_set() ||
			gd->state.game.objectiveRequiresNoViolence)
		{
			return true;
		}
		if (gd->is_active())
		{
			if ((gd->is_in_safe_space() || gd->is_narrative_requested()) &&
				! gd->is_narrative_trust_lost())
			{
				return true;
			}
		}
	}
	return false;
}

void GameDirector::narrative_mode_trust_lost()
{
	if (auto* gd = GameDirector::get())
	{
		if (gd->is_active())
		{
			if (gd->is_narrative_requested() &&
				! gd->is_narrative_trust_lost())
			{
				Concurrency::ScopedSpinLock lock(gd->state.lock);
				gd->state.narrativeTrustLost = true;
			}
		}
	}
}