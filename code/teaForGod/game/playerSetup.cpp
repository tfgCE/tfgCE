#include "playerSetup.h"

#include "game.h"
#include "gameState.h"
#include "skipContentInfo.h"
#include "..\buildNOs.h"
#include "..\teaForGod.h"
#include "..\library\library.h"
#include "..\library\gameDefinition.h"
#include "..\roomGenerators\roomGenerationInfo.h"
#include "..\sound\subtitleSystem.h"
#include "..\tutorials\tutorial.h"
#include "..\tutorials\tutorialSystem.h"

#include "..\..\core\buildInformation.h"
#include "..\..\core\io\xml.h"
#include "..\..\framework\appearance\lightSource.h"
#include "..\..\framework\game\gameInputDefinition.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\text\localisedString.h"
#include "..\..\framework\world\door.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// game input definition
DEFINE_STATIC_NAME(game);

// game tags
DEFINE_STATIC_NAME(rotatingElevators);

// global references
DEFINE_STATIC_NAME_STR(grGameDefinitionExperience, TXT("game definition; experience"));
DEFINE_STATIC_NAME_STR(grGameDefinitionSimpleRules, TXT("game definition; simple rules"));
DEFINE_STATIC_NAME_STR(grGameDefinitionComplexRules, TXT("game definition; complex rules"));

DEFINE_STATIC_NAME_STR(grDemoGameDefinition, TXT("game definition; demo"));
DEFINE_STATIC_NAME_STR(grDemoGameSubDefinition, TXT("game sub definition; demo"));

//

using namespace TeaForGodEmperor;

//

bool PlayerStats::load_from_xml(IO::XML::Node const* _parentNode)
{
	bool result = true;

	for_every(node, _parentNode->children_named(TXT("stats")))
	{
		result &= timeActive.load_from_xml(node, TXT("timeActive"));
		result &= timeRun.load_from_xml(node, TXT("timeRun"));
		result &= distance.load_from_xml(node, TXT("distance"));
		freshStarts = node->get_int_attribute(TXT("freshStarts"), freshStarts);
		runs = node->get_int_attribute(TXT("runs"), runs);
		kills = node->get_int_attribute(TXT("kills"), kills);
		deaths = node->get_int_attribute(TXT("deaths"), deaths);
		experience.load_from_xml(node, TXT("experience"));
		allEXMs = node->get_int_attribute(TXT("allEXMs"), allEXMs);
	}

	return result;
}

bool PlayerStats::save_to_xml(IO::XML::Node * _parentNode) const
{
	bool result = true;

	auto* node = _parentNode->add_node(TXT("stats"));
	node->set_attribute(TXT("timeActive"), timeActive.get_for_parse());
	node->set_attribute(TXT("timeRun"), timeRun.get_for_parse());
	node->set_attribute(TXT("distance"), distance.get_for_parse());
	node->set_int_attribute(TXT("freshStarts"), freshStarts);
	node->set_int_attribute(TXT("runs"), runs);
	node->set_int_attribute(TXT("kills"), kills);
	node->set_int_attribute(TXT("deaths"), deaths);
	node->set_attribute(TXT("experience"), experience.as_string_auto_decimals());
	node->set_int_attribute(TXT("allEXMs"), allEXMs);

	return result;
}

//

PlayerGameSlot::PlayerGameSlot()
: pilgrimSetup(&persistence)
{
	teaBoxSeed = Random::next_int();
	update_build_no_to_current(); // if we're loading, we should load 0
}

GameState* PlayerGameSlot::get_game_state_to_continue() const
{
	if (startUsingGameState.is_set())
	{
		return startUsingGameState.get();
	}
	if (lastMoment.is_set())
	{
		return lastMoment.get();
	}
	return nullptr;
}

bool PlayerGameSlot::has_any_game_state() const
{
	return
		lastMoment.is_set() ||
		checkpoint.is_set() ||
		atLeastHalfHealth.is_set() ||
		chapterStart.is_set();
}

GameDefinition const* PlayerGameSlot::get_game_definition() const
{
	return gameDefinition.get();
}

void PlayerGameSlot::force_game_definitions_for_demo()
{
	if (TeaForGodEmperor::is_demo())
	{
		TeaForGodEmperor::GameDefinition* demoGameDefinition = nullptr;
		Array<TeaForGodEmperor::GameSubDefinition*> demoGameSubDefinitions;
		auto* lib = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>();
		demoGameDefinition = lib->get_global_references().get< TeaForGodEmperor::GameDefinition>(NAME(grDemoGameDefinition), true);
		demoGameSubDefinitions.push_back(lib->get_global_references().get< TeaForGodEmperor::GameSubDefinition>(NAME(grDemoGameSubDefinition), true));
		if (demoGameDefinition)
		{
			set_game_definition(demoGameDefinition, demoGameSubDefinitions, true /* restart for test room */, true);
		}
	}
}

void PlayerGameSlot::fill_default_game_sub_definitions()
{
	if (auto* gd = gameDefinition.get())
	{
		gameSubDefinitions.clear();
		fill_game_sub_definitions_if_empty();
	}
}

void PlayerGameSlot::fill_game_sub_definitions_if_empty()
{
	if (gameSubDefinitions.is_empty())
	{
		if (auto* gd = gameDefinition.get())
		{
			if (!gd->get_default_game_sub_definitions().is_empty())
			{
				for_every_ref(gsd, gd->get_default_game_sub_definitions())
				{
					an_assert(gameSubDefinitions.has_place_left());
					gameSubDefinitions.push_back(Framework::UsedLibraryStored<GameSubDefinition>(gsd));
				}
			}
		}
	}
}

void PlayerGameSlot::set_game_definition(GameDefinition const* _gameDefinition, Array<GameSubDefinition*> const & _gameSubDefinitions, bool _newGameDefinition, bool _setAsCurrent)
{
	gameDefinition = _gameDefinition;
	gameSubDefinitions.clear();
	for_every_ptr(gsd, _gameSubDefinitions)
	{
		gameSubDefinitions.push_back(Framework::UsedLibraryStored<GameSubDefinition>(gsd));
	}
	fill_game_sub_definitions_if_empty();
	if (_newGameDefinition)
	{
		clear_persistence_and_use_starting_gear();
	}
	if (_setAsCurrent)
	{
		choose_game_definition();
		prepare_missions_definition();
		update_mission_state();
	}
}

void PlayerGameSlot::choose_game_definition()
{
	Array<GameSubDefinition*> gameSubDefinitionsAsPtrs;
	Framework::UsedLibraryStored<GameSubDefinition>::change_to_pointer_array(gameSubDefinitions, gameSubDefinitionsAsPtrs);
	GameDefinition::choose(get_game_definition(), gameSubDefinitionsAsPtrs);
}

void PlayerGameSlot::prepare_missions_definition()
{
	if (missionsDefinition.is_name_valid())
	{
		if (!missionsDefinition.get())
		{
			missionsDefinition.find_may_fail(Library::get_current());
		}
	}

	if (!missionsDefinition.get())
	{
		if (auto* gd = get_game_definition())
		{
			missionsDefinition = gd->get_starting_missions_definition();
		}
	}
}

MissionsDefinition const* PlayerGameSlot::get_missions_definition() const
{
	return missionsDefinition.get();
}

void PlayerGameSlot::update_mission_state()
{
	MissionState::find_current(); // so it is easier to access

	if (auto * ms = MissionState::get_current())
	{
		ms->ready_for(this);
	}
}

void PlayerGameSlot::create_new_mission_state()
{
	Concurrency::ScopedSpinLock lock(missionStateLock);

	missionState = new MissionState();
	
	MissionState::find_current(); // so it is easier to access

	if (auto * ms = MissionState::get_current())
	{
		ms->prepare_for(this);
	}
}

bool PlayerGameSlot::has_any_game_states() const
{
	return lastMoment.is_set()
		|| checkpoint.is_set()
		|| atLeastHalfHealth.is_set()
		|| chapterStart.is_set()
		|| ! unlockedPilgrimages.is_empty();
	// don't check reachedPilgrimages
}

void PlayerGameSlot::clear_mission_state()
{
	missionState.clear();
	update_mission_state();
}

void PlayerGameSlot::clear_game_states()
{
	startUsingGameState.clear();
	startedUsingGameState.clear();
	lastMoment.clear();
	checkpoint.clear();
	atLeastHalfHealth.clear();
	chapterStart.clear();
}

void PlayerGameSlot::clear_game_states_and_mission_state_and_pilgrimages()
{
	clear_game_states();
	clear_mission_state();

	unlockedPilgrimages.clear();
	reachedPilgrimages.clear();
	restartAtPilgrimage = Framework::LibraryName::invalid();
}

MissionResult* PlayerGameSlot::create_new_last_mission_result()
{
	lastMissionResult = new MissionResult();
	ignoreLastMissionResultForSummary = false;
	if (auto* ms = missionState.get())
	{
		lastMissionResult->set_mission(ms->get_mission(), ms->get_difficulty_modifier());
	}
	return lastMissionResult.get();
}

void PlayerGameSlot::clear_last_mission_result()
{
	lastMissionResult.clear();
	ignoreLastMissionResultForSummary = false;
}

void PlayerGameSlot::update_build_no_to_current()
{
	buildNo = get_build_version_build_no();
	buildAcknowledgedChangesFlags = 0;
}

void PlayerGameSlot::clear_persistence_and_use_starting_gear(PilgrimGear const* _startingGear)
{
	if (!_startingGear && gameDefinition.get())
	{
		_startingGear = gameDefinition->get_default_starting_gear();
	}
	if (!_startingGear)
	{
		return;
	}

	for_count(int, hIdx, Hand::MAX)
	{
		Hand::Type hand = (Hand::Type)hIdx;
		auto& handSrc = _startingGear->hands[hIdx];
		auto& handDst = pilgrimSetup.access_hand(hand);
		//
		{
			Concurrency::ScopedSpinLock lock(handDst.exmsLock);
			handDst.copy_exms_from(handSrc.passiveEXMs, handSrc.activeEXM);
		}
		handDst.weaponSetup.clear();
		if (!handSrc.weaponSetup.is_empty())
		{
			for_every(wp, handSrc.weaponSetup.get_parts())
			{
				auto* wpi = wp->part->create_copy_for_different_persistence(&persistence);
				handDst.weaponSetup.add_part(wp->at, wpi);
			}
		}
		else if (!handSrc.weaponSetupTemplate.is_empty())
		{
			handDst.weaponSetup.setup_with_template(handSrc.weaponSetupTemplate, &persistence);
		}
	}
}

void PlayerGameSlot::update_last_time_played()
{
	lastTimePlayed = Framework::DateTime::get_current_from_system();
}

void PlayerGameSlot::set_game_slot_mode(GameSlotMode::Type _newMode)
{
	if (gameSlotMode != _newMode)
	{
		gameSlotMode = _newMode;
		restartAtPilgrimage = Framework::LibraryName::invalid();
		MissionState::async_abort(false);
		clear_game_states();
		clear_mission_state();
	}
}

void PlayerGameSlot::make_game_slot_mode_available(GameSlotMode::Type _mode)
{
	if (_mode == GameSlotMode::Story)
	{
		// no need to unlock it
		return;
	}
	gameSlotModesAvailable.push_back_unique(_mode);
}

bool PlayerGameSlot::is_game_slot_mode_available(GameSlotMode::Type _mode) const
{
	if (_mode == GameSlotMode::Story)
	{
		return true;
	}
	return gameSlotModesAvailable.does_contain(_mode);
}

int PlayerGameSlot::get_game_slot_mode_available_count() const
{
	int count = gameSlotModesAvailable.get_size();
	if (!gameSlotModesAvailable.does_contain(GameSlotMode::Story))
	{
		++count;
	}
	return count;
}

void PlayerGameSlot::make_game_slot_mode_confirmed(GameSlotMode::Type _mode)
{
	gameSlotModesConfirmed.push_back_unique(_mode);
}

bool PlayerGameSlot::is_game_slot_mode_confirmed(GameSlotMode::Type _mode) const
{
	return gameSlotModesConfirmed.does_contain(_mode);
}

void PlayerGameSlot::sync_update_unlocked_pilgrimages_from_game_definition(PlayerSetup const& _setup, bool _updateRestartAtPilgrimage)
{
	ASSERT_SYNC; // to avoid issues with locking
	if (auto* gd = GameDefinition::get_chosen())
	{
		update_unlocked_pilgrimages(_setup, gd->get_pilgrimages(), gd->get_conditional_pilgrimages(), _updateRestartAtPilgrimage);
	}
}

bool PlayerGameSlot::sync_unlock_pilgrimage(Framework::LibraryName const& _pilgrimage)
{
	ASSERT_SYNC; // to avoid issues with locking
	return ignore_sync_unlock_pilgrimage(_pilgrimage);
}

bool PlayerGameSlot::ignore_sync_unlock_pilgrimage(Framework::LibraryName const& _pilgrimage)
{
	if (!unlockedPilgrimages.does_contain(_pilgrimage))
	{
		unlockedPilgrimages.push_back(_pilgrimage);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT("restartAtPilgrimage set to \"%S\""), _pilgrimage.to_string().to_char());
#endif
		restartAtPilgrimage = _pilgrimage;
		return true;
	}
	else
	{
		return false;
	}
}

void PlayerGameSlot::sync_reached_pilgrimage(Framework::LibraryName const& _pilgrimage, Energy const & _experienceForReachingForFirstTime)
{
	ASSERT_SYNC; // to avoid issues with locking
	if (!playedRecentlyPilgrimages.does_contain(_pilgrimage))
	{
		playedRecentlyPilgrimages.push_back(_pilgrimage);
	}
	if (!reachedPilgrimages.does_contain(_pilgrimage))
	{
		reachedPilgrimages.push_back(_pilgrimage);
		if (_experienceForReachingForFirstTime.is_positive())
		{
			PlayerSetup::access_current().stats__experience(_experienceForReachingForFirstTime);
			GameStats::get().add_experience(_experienceForReachingForFirstTime);
			Persistence::access_current().provide_experience(_experienceForReachingForFirstTime);
		}
	}
}

void PlayerGameSlot::update_unlocked_pilgrimages(PlayerSetup const& _setup, Array<Framework::UsedLibraryStored<Pilgrimage>> const& _pilgrimages, Array<ConditionalPilgrimage> const& _conditionalPilgrimages, bool _updateRestartAtPilgrimage)
{
	{
		Tags overallGeneralProgress = get_general_progress();
		overallGeneralProgress.set_tags_from(_setup.get_general_progress());

		for_every_ref(p, _pilgrimages)
		{
			auto& tc = p->get_auto_unlock_on_profile_general_progress();
			if (!tc.is_empty() &&
				tc.check(_setup.get_general_progress()))
			{
				if (!unlockedPilgrimages.does_contain(p->get_name()))
				{
					unlockedPilgrimages.push_back(p->get_name());
					if (_updateRestartAtPilgrimage)
					{
						restartAtPilgrimage = p->get_name();
					}
				}
			}
		}
		for_every(cp, _conditionalPilgrimages)
		{
			if (auto* p = cp->pilgrimage.get())
			{
				if (cp->generalProgressRequirement.check(overallGeneralProgress) ||
					cp->gameSlotGeneralProgressRequirement.check(generalProgress) ||
					cp->profileGeneralProgressRequirement.check(_setup.get_general_progress()))
				{
					if (!unlockedPilgrimages.does_contain(p->get_name()))
					{
						unlockedPilgrimages.push_back(p->get_name());
						if (_updateRestartAtPilgrimage)
						{
							restartAtPilgrimage = p->get_name();
						}
					}
				}
			}
		}
	}
}

void PlayerGameSlot::clear_played_recently_pilgrimages()
{
	output(TXT("clear played recently pilgrimages"));
	playedRecentlyPilgrimages.clear();
	unlockablePlayedRecentlyPilgrimages.clear();
}

void PlayerGameSlot::store_played_recently_pilgrimages_as_unlockables()
{
	output(TXT("store played recently pilgrimages as unlockables"));
	for_every(p, playedRecentlyPilgrimages)
	{
		unlockablePlayedRecentlyPilgrimages.push_back_unique(*p);
	}
}

//--

PlayerSetup* PlayerSetup::s_current = nullptr;
bool PlayerSetup::s_storeGameSlotStats = false;

PlayerSetup::PlayerSetup()
{
}

PlayerSetup::~PlayerSetup()
{
	if (s_current == this)
	{
		s_current = nullptr;
	}
}

PilgrimSetup& PlayerSetup::access_pilgrim_setup()
{
	if (TutorialSystem::check_active())
	{
		return TutorialSystem::get()->access_pilgrim_setup();
	}
	an_assert(gameSlots.is_index_valid(activeGameSlotIdx)); return gameSlots[activeGameSlotIdx]->pilgrimSetup;
}

PilgrimSetup const& PlayerSetup::get_pilgrim_setup() const
{
	if (TutorialSystem::check_active())
	{
		return TutorialSystem::get()->get_pilgrim_setup();
	}
	an_assert(gameSlots.is_index_valid(activeGameSlotIdx)); return gameSlots[activeGameSlotIdx]->pilgrimSetup;
}

PlayerGameSlot const* PlayerSetup::get_active_game_slot() const
{
	if (TutorialSystem::check_active())
	{
		return TutorialSystem::get()->get_game_slot();
	}
	return gameSlots.is_index_valid(activeGameSlotIdx) ? gameSlots[activeGameSlotIdx].get() : nullptr;
}

PlayerGameSlot* PlayerSetup::access_active_game_slot()
{
	if (TutorialSystem::check_active())
	{
		return TutorialSystem::get()->access_game_slot();
	}
	return gameSlots.is_index_valid(activeGameSlotIdx) ? gameSlots[activeGameSlotIdx].get() : nullptr;
}

void PlayerSetup::reset()
{
	stats = PlayerStats();
	preferences = PlayerPreferences();

	gameSlots.clear();
	gameSlotsCreated = 0;
	activeGameSlotIdx = NONE;

	gameUnlocks.clear();
	generalProgress.clear();

	tutorialsDone.clear();
	lastTutorialStarted = Framework::LibraryName::invalid();
}

void PlayerSetup::setup_for_quick_game_player_profile()
{
	if (gameSlots.is_empty())
	{
		gameSlots.push_back(RefCountObjectPtr<PlayerGameSlot>(new PlayerGameSlot()));
		setup_auto_for_game_slots();
		setup_defaults();
	}
	activeGameSlotIdx = 0;
}

void PlayerSetup::add_game_slot_and_make_it_active()
{
	gameSlots.push_back(RefCountObjectPtr<PlayerGameSlot>(new PlayerGameSlot()));
	setup_auto_for_game_slots();
	setup_defaults();
	activeGameSlotIdx = gameSlots.get_size() - 1;
}

void PlayerSetup::have_at_least_one_game_slot()
{
	if (gameSlots.is_empty())
	{
		gameSlots.push_back(RefCountObjectPtr<PlayerGameSlot>(new PlayerGameSlot()));
		setup_auto_for_game_slots();
		setup_defaults();
	}
	if (activeGameSlotIdx == NONE)
	{
		activeGameSlotIdx = gameSlots.get_size() - 1;
	}
}

bool PlayerSetup::legacy_load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	reset();

	result &= load_player_common_from_xml(_node, nullptr);

	gameSlots.push_back(RefCountObjectPtr<PlayerGameSlot>(new PlayerGameSlot()));
	gameSlotsCreated = 1;
	auto& gs = *(gameSlots.get_last().get());
	gs.pilgrimSetup.load_from_xml_child_node(_node, TXT("pilgrim"));

	setup_auto_for_game_slots();

	return result;
}

bool PlayerSetup::load_player_stats_and_preferences_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	reset();

	result &= stats.load_from_xml(_node);

	for_every(nodeCommon, _node->children_named(TXT("common")))
	{
		result &= preferences.load_from_xml(nodeCommon);
	}

	return result;
}

bool PlayerSetup::load_from_xml(IO::XML::Node const* _node, Array<IO::XML::Node const*> const& _gameSlotNodes)
{
	bool result = true;

	reset();

	activeGameSlotId = NONE;

	result &= stats.load_from_xml(_node);

	result &= load_player_common_from_xml(_node, TXT("common"));
	result &= load_game_slots_info_from_xml(_node);

	result &= gameUnlocks.load_from_xml_attribute_or_child_node(_node, TXT("gameUnlocks"));
	result &= generalProgress.load_from_xml_attribute_or_child_node(_node, TXT("generalProgress"));

	{
		Array<IO::XML::Node const*> gameSlotNodes;
		gameSlotNodes.add_from(_gameSlotNodes);
		for_every(node, _node->children_named(TXT("gameSlots")))
		{
			for_every(gsNode, node->children_named(TXT("gameSlot")))
			{
				gameSlotNodes.push_back(gsNode);
			}
		}
		result &= load_game_slots_from_xml(gameSlotNodes);

		for_every_ref(gs, gameSlots)
		{
			if (gs->id == activeGameSlotId)
			{
				activeGameSlotIdx = for_everys_index(gs);
			}
		}

		if (activeGameSlotIdx == NONE &&
			!gameSlots.is_empty())
		{
			warn_loading_xml(_node, TXT("couldn't read active game slot info, assuming last one [slots %i]"), gameSlots.get_size());
			activeGameSlotIdx = gameSlots.get_size() - 1;
		}
	}

	if (activeGameSlotIdx != NONE)
	{
		activeGameSlotId = NONE;
	}

	{
		int totalAllEXMs = 0;
		for_every_ref(gs, gameSlots)
		{
			totalAllEXMs += gs->stats.allEXMs;
		}
		stats.allEXMs = max(stats.allEXMs, totalAllEXMs);
	}

	return result;
}

bool PlayerSetup::load_player_common_from_xml(IO::XML::Node const* _node, tchar const* _childNode)
{
	bool result = true;

	Array<IO::XML::Node const*> loadFromNodes;
	
	if (_childNode)
	{
		for_every(nodeCommon, _node->children_named(_childNode))
		{
			loadFromNodes.push_back(nodeCommon);
		}
	}
	else
	{
		loadFromNodes.push_back(_node);
	}

	tutorialsDone.clear();

	for_every_ptr(nodeCommon, loadFromNodes)
	{
		result &= preferences.load_from_xml(nodeCommon);

		allowNonGameMusic = nodeCommon->get_bool_attribute_or_from_child_presence(TXT("allowNonGameMusic"), allowNonGameMusic);

		for_every(node, nodeCommon->children_named(TXT("tutorials")))
		{
			{
				Framework::LibraryName tutorial;
				if (tutorial.load_from_xml(node, TXT("lastTutorialStarted")))
				{
					lastTutorialStarted = tutorial;
				}
			}
			for_every(tdNode, node->children_named(TXT("done")))
			{
				Framework::LibraryName tutorial;
				Framework::DateTime when;
				if (tutorial.load_from_xml(tdNode, TXT("tutorial")) &&
					when.load_from_xml(tdNode))
				{
					int howMany = tdNode->get_int_attribute(TXT("howMany"), 1);
					if (!when.is_valid())
					{
						when = Framework::DateTime::get_current_from_system();
					}
					bool tutorialAdded = false;
					if (!tutorialAdded)
					{
						for_every(tut, tutorialsDone)
						{
							if (tut->tutorial == tutorial)
							{
								tut->when = when;
								tut->howMany = howMany;
								tutorialAdded = true;
								break;
							}
						}
					}
					if (!tutorialAdded)
					{
						tutorialsDone.push_back(TutorialDone(tutorial, when, howMany));
						tutorialAdded = true;
					}
				}
			}
		}
	}

	if (s_current == this)
	{
		preferences.apply();
	}

	return result;
}

bool PlayerSetup::load_game_slots_info_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	
	activeGameSlotIdx = NONE;
	gameSlotsCreated = 0;
	for_every(node, _node->children_named(TXT("gameSlots")))
	{
		gameSlotsCreated = node->get_int_attribute(TXT("created"), gameSlotsCreated);
		activeGameSlotId = node->get_int_attribute(TXT("activeId"), activeGameSlotId);
	}

	return result;
}

bool PlayerSetup::load_game_slots_from_xml(Array<IO::XML::Node const*> const& _gameSlotNodes)
{
	bool result = true;
	
	activeGameSlotIdx = NONE;
	for_every_ptr(gsNode, _gameSlotNodes)
	{
		gameSlots.push_back(RefCountObjectPtr<PlayerGameSlot>(new PlayerGameSlot()));
		auto& gs = *(gameSlots.get_last().get());
		gs.lastTimePlayed.load_from_xml(gsNode->first_child_named(TXT("lastTimePlayed")));
		gs.id = gsNode->get_int_attribute(TXT("id"), gs.id);
		gs.teaBoxSeed = gsNode->get_int_attribute(TXT("teaBoxSeed"), gs.teaBoxSeed);
		gs.buildNo = gsNode->get_int_attribute(TXT("buildNo"), 0 /* assume not provided at all */);
#ifdef STORE_BUILD_ACKNOWLEDGMENTS_FOR_GAME_SLOTS
		gs.buildAcknowledgedChangesFlags = gsNode->get_int_attribute(TXT("buildAcknowledgedChangesFlags"), 0 /* read only if present */);
#endif
		gs.stats.load_from_xml(gsNode);
		gs.persistence.load_from_xml_child_node(gsNode, TXT("persistence"));
		gs.stats.allEXMs = max(gs.stats.allEXMs, gs.persistence.get_all_exms().get_size()); // fix in case we're not having it
		gs.persistence.resolve_library_links();
		/* we use game definition only */ //gs.pilgrimSetup.load_from_xml_child_node(gsNode, TXT("pilgrim"));
		gs.runSetup.load_from_xml_child_node(gsNode, TXT("run"));
		gs.gameDefinition.load_from_xml(gsNode, TXT("gameDefinition"));
		for_every(n, gsNode->children_named(TXT("gameDefinition")))
		{
			gs.gameDefinition.load_from_xml(n);
		}
		gs.gameDefinition.find_may_fail(Library::get_current());
		gs.gameSubDefinitions.clear();
		for_every(n, gsNode->children_named(TXT("gameSubDefinition")))
		{
			Framework::UsedLibraryStored<GameSubDefinition> gsd;
			gsd.load_from_xml(n);
			gsd.find_may_fail(Library::get_current());
			gs.gameSubDefinitions.push_back(gsd);
		}
		gs.fill_game_sub_definitions_if_empty();
		for_every(n, gsNode->children_named(TXT("missionsDefinition")))
		{
			gs.missionsDefinition.load_from_xml(n);
		}		
		gs.missionsDefinition.find_may_fail(Library::get_current());
		
		gs.force_game_definitions_for_demo();

		gs.gameModifiers.load_from_xml_attribute_or_child_node(gsNode, TXT("gameModifiers"));
		
		gs.generalProgress.load_from_xml_attribute_or_child_node(gsNode, TXT("generalProgress"));

		if (is_demo())
		{
			gs.gameSlotMode = GameSlotMode::Story;
		}
		else
		{
			gs.gameSlotMode = GameSlotMode::parse(gsNode->get_string_attribute_or_from_child(TXT("gameSlotMode")));
		}

		gs.gameSlotModesAvailable.clear();
		for_every(n, gsNode->children_named(TXT("gameSlotModeAvailable")))
		{
			GameSlotMode::Type gsm = GameSlotMode::Story;
			gsm = GameSlotMode::parse(n->get_text());
			gs.gameSlotModesAvailable.push_back_unique(gsm);
		}

		gs.gameSlotModesConfirmed.clear();
		for_every(n, gsNode->children_named(TXT("gameSlotModeConfirmed")))
		{
			GameSlotMode::Type gsm = GameSlotMode::Story;
			gsm = GameSlotMode::parse(n->get_text());
			gs.gameSlotModesConfirmed.push_back_unique(gsm);
		}

		gs.missionState.clear();
		for_every(n, gsNode->children_named(TXT("missionState")))
		{
			gs.missionState = new MissionState();
			result &= gs.missionState->load_from_xml(n);
		}

		gs.lastMissionResult.clear();
		for_every(n, gsNode->children_named(TXT("lastMissionResult")))
		{
			gs.lastMissionResult = new MissionResult();
			result &= gs.lastMissionResult->load_from_xml(n);
			if (!gs.lastMissionResult->resolve_library_links())
			{
				error_loading_xml_dev_ignore(gsNode, TXT("could not resolve links for last mission result, removing"));
				gs.lastMissionResult.clear();
			}
		}

		gs.restartAtPilgrimage = Framework::LibraryName::invalid();
		for_every(raNode, gsNode->children_named(TXT("restartAt")))
		{
			gs.restartAtPilgrimage.load_from_xml(raNode, TXT("pilgrimage"));
		}

		gs.unlockedPilgrimages.clear();
		for_every(upNode, gsNode->children_named(TXT("unlockedPilgrimages")))
		{
			for_every(up1Node, upNode->children_named(TXT("unlocked")))
			{
				Framework::LibraryName up;
				if (up.load_from_xml(up1Node, TXT("pilgrimage")))
				{
					gs.unlockedPilgrimages.push_back(up);
				}
				else
				{
					error_loading_xml_dev_ignore(up1Node, TXT("could not load unlocked pilgrimage"));
				}
			}
		}

		gs.reachedPilgrimages.clear();
		for_every(upNode, gsNode->children_named(TXT("reachedPilgrimages")))
		{
			for_every(up1Node, upNode->children_named(TXT("reached")))
			{
				Framework::LibraryName up;
				if (up.load_from_xml(up1Node, TXT("pilgrimage")))
				{
					gs.reachedPilgrimages.push_back(up);
				}
				else
				{
					error_loading_xml_dev_ignore(up1Node, TXT("could not load reached pilgrimage"));
				}
			}
		}

		gs.playedRecentlyPilgrimages.clear();
		for_every(upNode, gsNode->children_named(TXT("playedRecentlyPilgrimages")))
		{
			for_every(up1Node, upNode->children_named(TXT("played")))
			{
				Framework::LibraryName up;
				if (up.load_from_xml(up1Node, TXT("pilgrimage")))
				{
					gs.playedRecentlyPilgrimages.push_back(up);
				}
				else
				{
					error_loading_xml_dev_ignore(up1Node, TXT("could not load reached pilgrimage"));
				}
			}
		}

		gs.unlockablePlayedRecentlyPilgrimages.clear();
		for_every(upNode, gsNode->children_named(TXT("unlockablePlayedRecentlyPilgrimages")))
		{
			for_every(up1Node, upNode->children_named(TXT("unlockable")))
			{
				Framework::LibraryName up;
				if (up.load_from_xml(up1Node, TXT("pilgrimage")))
				{
					gs.unlockablePlayedRecentlyPilgrimages.push_back(up);
				}
				else
				{
					error_loading_xml_dev_ignore(up1Node, TXT("could not load reached pilgrimage"));
				}
			}
		}

		for_every(n, gsNode->children_named(TXT("lastMoment")))
		{
			gs.lastMoment = new GameState();
			gs.lastMoment->load_from_xml(n);
			if (!gs.lastMoment->resolve_library_links())
			{
				error_loading_xml_dev_ignore(gsNode, TXT("could not resolve links for last moment, removing"));
				gs.lastMoment.clear();
			}
		}

		for_every(n, gsNode->children_named(TXT("checkpoint")))
		{
			gs.checkpoint = new GameState();
			gs.checkpoint->load_from_xml(n);
			if (!gs.checkpoint->resolve_library_links())
			{
				error_loading_xml_dev_ignore(gsNode, TXT("could not resolve links for checkpoint, removing"));
				gs.checkpoint.clear();
			}
		}

		for_every(n, gsNode->children_named(TXT("atLeastHalfHealth")))
		{
			gs.atLeastHalfHealth = new GameState();
			gs.atLeastHalfHealth->load_from_xml(n);
			if (!gs.atLeastHalfHealth->resolve_library_links())
			{
				error_loading_xml_dev_ignore(gsNode, TXT("could not resolve links for at least half health, removing"));
				gs.atLeastHalfHealth.clear();
			}
		}

		for_every(n, gsNode->children_named(TXT("chapterStart")))
		{
			gs.chapterStart = new GameState();
			gs.chapterStart->load_from_xml(n);
			if (!gs.chapterStart->resolve_library_links())
			{
				error_loading_xml_dev_ignore(gsNode, TXT("could not resolve links for chapter start, removing"));
				gs.chapterStart.clear();
			}
		}

		if (gs.buildNo < BUILD_NO__INVALIDATE_GAME_SLOTS)
		{
			// remove older game slots
			gameSlots.pop_back();
		}
	}

	setup_auto_for_game_slots();

	return result;
}

bool PlayerSetup::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	result &= stats.save_to_xml(_node);
	if (auto* n = _node->add_node(TXT("gameUnlocks")))
	{
		n->add_text(gameUnlocks.to_string());
	}
	if (auto* n = _node->add_node(TXT("generalProgress")))
	{
		n->add_text(generalProgress.to_string());
	}
	result &= save_player_common_to_xml(_node);
	result &= save_game_slots_info_to_xml(_node);

	return result;
}

bool PlayerSetup::save_player_common_to_xml(IO::XML::Node * _node) const
{
	bool result = true;

	auto* nodeCommon = _node->add_node(TXT("common"));

	result &= preferences.save_to_xml(nodeCommon);

	if (allowNonGameMusic)
	{
		nodeCommon->add_node(TXT("allowNonGameMusic"));
	}

	{
		auto* node = nodeCommon->add_node(TXT("tutorials"));
		if (lastTutorialStarted.is_valid())
		{
			node->set_attribute(TXT("lastTutorialStarted"), lastTutorialStarted.to_string());
		}
		for_every(td, tutorialsDone)
		{
			auto* tdNode = node->add_node(TXT("done"));
			tdNode->set_attribute(TXT("tutorial"), td->tutorial.to_string());
			td->when.save_to_xml(tdNode);
			tdNode->set_int_attribute(TXT("howMany"), td->howMany);
		}
	}

	return result;
}

bool PlayerSetup::save_game_slots_info_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	auto* node = _node->add_node(TXT("gameSlots"));
	node->set_int_attribute(TXT("created"), gameSlotsCreated);
	if (gameSlots.is_index_valid(activeGameSlotIdx))
	{
		node->set_int_attribute(TXT("activeId"), gameSlots[activeGameSlotIdx]->id);
	}
	else if (activeGameSlotId != NONE) // if we have a backup
	{
		node->set_int_attribute(TXT("activeId"), activeGameSlotId);
	}

	return result;
}

bool PlayerSetup::save_game_slot_to_xml(IO::XML::Node* _node, PlayerGameSlot const* _gameSlot) const
{
	bool result = true;
	auto* gs = _gameSlot;
	auto* gsNode = _node;

	{
		gsNode->set_int_attribute(TXT("id"), gs->id);
		gsNode->set_int_attribute(TXT("teaBoxSeed"), gs->teaBoxSeed);
		gsNode->set_int_attribute(TXT("buildNo"), gs->buildNo);
#ifdef STORE_BUILD_ACKNOWLEDGMENTS_FOR_GAME_SLOTS
		if (gs->buildAcknowledgedChangesFlags != 0)
		{
			gsNode->set_int_attribute(TXT("buildAcknowledgedChangesFlags"), gs->buildAcknowledgedChangesFlags);
		}
#endif
		gs->lastTimePlayed.save_to_xml(gsNode->add_node(TXT("lastTimePlayed")));
		result &= gs->stats.save_to_xml(gsNode);
		result &= gs->persistence.save_to_xml_child_node(gsNode, TXT("persistence"));
		/* we use gamedefinition only */ //result &= gs->pilgrimSetup.save_to_xml_child_node(gsNode, TXT("pilgrim"));
		result &= gs->runSetup.save_to_xml_child_node(gsNode, TXT("run"));
		if (gs->gameDefinition.get())
		{
			gsNode->add_node(TXT("gameDefinition"))->add_text(gs->gameDefinition->get_name().to_string());
		}
		for_every(gsd, gs->gameSubDefinitions)
		{
			gsNode->add_node(TXT("gameSubDefinition"))->add_text(gsd->get_name().to_string());
		}
		if (gs->missionsDefinition.get())
		{
			gsNode->add_node(TXT("missionsDefinition"))->add_text(gs->missionsDefinition->get_name().to_string());
		}
		gsNode->add_node(TXT("gameModifiers"))->add_text(gs->gameModifiers.to_string());
		gsNode->add_node(TXT("generalProgress"))->add_text(gs->generalProgress.to_string());
		gsNode->add_node(TXT("gameSlotMode"))->add_text(GameSlotMode::to_char(gs->gameSlotMode));
		for_every(gsm, gs->gameSlotModesAvailable)
		{
			gsNode->add_node(TXT("gameSlotModeAvailable"))->add_text(GameSlotMode::to_char(*gsm));
		}
		for_every(gsm, gs->gameSlotModesConfirmed)
		{
			gsNode->add_node(TXT("gameSlotModeConfirmed"))->add_text(GameSlotMode::to_char(*gsm));
		}

		if (auto* ms = gs->missionState.get())
		{
			ms->save_to_xml(gsNode->add_node(TXT("missionState")));
		}

		if (auto* mr = gs->lastMissionResult.get())
		{
			mr->save_to_xml(gsNode->add_node(TXT("lastMissionResult")));
		}

		if (gs->restartAtPilgrimage.is_valid())
		{
			auto* raNode = gsNode->add_node(TXT("restartAt"));
			raNode->set_attribute(TXT("pilgrimage"), gs->restartAtPilgrimage.to_string().to_char());
		}
		{
			auto* upNode = gsNode->add_node(TXT("unlockedPilgrimages"));
			for_every(up, gs->unlockedPilgrimages)
			{
				auto* up1Node = upNode->add_node(TXT("unlocked"));
				up1Node->set_attribute(TXT("pilgrimage"), up->to_string().to_char());
			}
		}
		{
			auto* upNode = gsNode->add_node(TXT("reachedPilgrimages"));
			for_every(up, gs->reachedPilgrimages)
			{
				auto* up1Node = upNode->add_node(TXT("reached"));
				up1Node->set_attribute(TXT("pilgrimage"), up->to_string().to_char());
			}
		}
		{
			auto* upNode = gsNode->add_node(TXT("playedRecentlyPilgrimages"));
			for_every(up, gs->playedRecentlyPilgrimages)
			{
				auto* up1Node = upNode->add_node(TXT("played"));
				up1Node->set_attribute(TXT("pilgrimage"), up->to_string().to_char());
			}
		}
		{
			auto* upNode = gsNode->add_node(TXT("unlockablePlayedRecentlyPilgrimages"));
			for_every(up, gs->unlockablePlayedRecentlyPilgrimages)
			{
				auto* up1Node = upNode->add_node(TXT("unlockable"));
				up1Node->set_attribute(TXT("pilgrimage"), up->to_string().to_char());
			}
		}
		if (auto* lastMoment = _gameSlot->lastMoment.get())
		{
			if (auto* node = gsNode->add_node(TXT("lastMoment")))
			{
				if (!lastMoment->save_to_xml(node))
				{
					result = false;
					gsNode->remove_node(node);
				}
			}
		}
		if (auto* checkpoint = _gameSlot->checkpoint.get())
		{
			if (auto* node = gsNode->add_node(TXT("checkpoint")))
			{
				if (!checkpoint->save_to_xml(node))
				{
					result = false;
					gsNode->remove_node(node);
				}
			}
		}
		if (auto* atLeastHalfHealth = _gameSlot->atLeastHalfHealth.get())
		{
			if (auto* node = gsNode->add_node(TXT("atLeastHalfHealth")))
			{
				if (!atLeastHalfHealth->save_to_xml(node))
				{
					result = false;
					gsNode->remove_node(node);
				}
			}
		}
		if (auto* chapterStart = _gameSlot->chapterStart.get())
		{
			if (auto* node = gsNode->add_node(TXT("chapterStart")))
			{
				if (!chapterStart->save_to_xml(node))
				{
					result = false;
					gsNode->remove_node(node);
				}
			}
		}
	}

	an_assert(result);
	return result;
}

bool PlayerSetup::is_option_set(Name const & _option) const
{
	return preferences.is_option_set(_option);
}

void PlayerSetup::setup_defaults()
{
}

void PlayerSetup::stats__death()
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	++stats.deaths;
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			++gs->stats.deaths;
		}
	}
}

void PlayerSetup::stats__unlock_all_exms()
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	++stats.allEXMs;
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			++gs->stats.allEXMs;
		}
	}
}

void PlayerSetup::stats__fresh_start()
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	++stats.freshStarts;
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			++gs->stats.freshStarts;
		}
	}
}

void PlayerSetup::stats__run()
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	++stats.runs;
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			++gs->stats.runs;
		}
	}
}

void PlayerSetup::stats__run_revert()
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	--stats.runs;
	stats.runs = max(0, stats.runs);
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			--gs->stats.runs;
			gs->stats.runs = max(0, gs->stats.runs);
		}
	}
}

void PlayerSetup::stats__kill()
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	++stats.kills;
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			++gs->stats.kills;
		}
	}
}

void PlayerSetup::stats__experience(Energy const & _experience)
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	stats.experience += _experience;
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			gs->stats.experience += _experience;
		}
	}
}

void PlayerSetup::stats__merit_points(int _meritPoints)
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	stats.meritPoints += _meritPoints;
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			gs->stats.meritPoints += _meritPoints;
		}
	}
}

void PlayerSetup::stats__increase_run_time(float _time)
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	stats.timeRun.advance_ms(_time);
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			gs->stats.timeRun.advance_ms(_time);
		}
	}
}

void PlayerSetup::stats__increase_profile_time(float _time)
{
	// profile time is always increased, even in tutorial
	Concurrency::ScopedSpinLock lock(statsLock);
	stats.timeActive.advance_ms(_time);
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			gs->stats.timeActive.advance_ms(_time);
		}
	}
}

void PlayerSetup::stats__increase_distance(float _distance)
{
	RETURN_IF_IN_TUTORIAL;
	Concurrency::ScopedSpinLock lock(statsLock);
	stats.distance.add_meters(_distance);
	if (s_storeGameSlotStats)
	{
		if (auto* gs = access_active_game_slot())
		{
			gs->stats.distance.add_meters(_distance);
		}
	}
}

void PlayerSetup::remove_active_game_slot(bool _allowUpdatingID)
{
	if (gameSlots.is_index_valid(activeGameSlotIdx))
	{
		if (_allowUpdatingID)
		{
			if (gameSlotsCreated == gameSlots[activeGameSlotIdx]->id)
			{
				--gameSlotsCreated;
			}
		}
		gameSlots.remove_at(activeGameSlotIdx);
	}
	activeGameSlotIdx = gameSlots.is_empty()? NONE : gameSlots.get_size() - 1;
}

String const& PlayerSetup::get_profile_name() const
{
	if (auto* g = Game::get_as<Game>())
	{
		return g->get_player_profile_name();
	}
	return String::empty();
}

void PlayerSetup::setup_auto_for_game_slots()
{
	for_every_ref(gs, gameSlots)
	{
		if (gs->id == NONE)
		{
			gs->id = gameSlotsCreated + 1;
			++gameSlotsCreated;
		}
		gameSlotsCreated = max(gameSlotsCreated, gs->id);
		gs->persistence.setup_auto();
	}

	// check if game definition is defined, if not, choose one depending on other game definitions
	// in all (valid) cases, we should have only ONE game definition missing
	// let's shamelessly ignore other cases
	GameDefinition* defaultGameDefinition = nullptr;
	{
		Array<GameDefinition*> tryGameDefinitions;
		Array<Name> gameDefinitionGRNames;
		gameDefinitionGRNames.push_back(NAME(grGameDefinitionExperience));
		gameDefinitionGRNames.push_back(NAME(grGameDefinitionSimpleRules));
		gameDefinitionGRNames.push_back(NAME(grGameDefinitionComplexRules));
		for_every(gdgr, gameDefinitionGRNames)
		{
			Framework::LibraryName gdName = Library::get_current()->get_global_references().get_name(*gdgr,
				Library::get_current_as<Library>()->get_game_definitions().is_empty() /* may fail, especially when we're loading the game and we haven't loaded game definitions yet */);
			if (gdName.is_valid())
			{
				if (GameDefinition* gd = Library::get_current_as<Library>()->get_game_definitions().find(gdName))
				{
					tryGameDefinitions.push_back(gd);
				}
			}
		}
		if (!tryGameDefinitions.is_empty())
		{
			int lowestFound = NONE;
			for_every_ref(gs, gameSlots)
			{
				if (auto* gd = gs->gameDefinition.get())
				{
					int at = tryGameDefinitions.find_index(gd);
					lowestFound = max(lowestFound, at);
				}
			}
			int useIdx = clamp(lowestFound, 0, tryGameDefinitions.get_size() - 1);
			defaultGameDefinition = tryGameDefinitions[useIdx];
			for_every_ref(gs, gameSlots)
			{
				if (! gs->gameDefinition.get())
				{
					gs->set_game_definition(defaultGameDefinition, Array<GameSubDefinition*>(), true /* reset */, false /* do not set as current, we will do so separately */);
				}
			}
		}
	}

	for_every_ref(gs, gameSlots)
	{
		gs->fill_game_sub_definitions_if_empty();
	}
}

void PlayerSetup::redo_tutorials()
{
	tutorialsDone.clear();
}

void PlayerSetup::mark_tutorial_done(Framework::LibraryName const& _tutorial)
{
	if (auto* game = Game::get_as<Game>())
	{
		if (game->is_using_quick_game_player_profile())
		{
			return;
		}
	}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("tutorial done \"%S\""), _tutorial.get_name().to_string().to_char());
#endif
	for_every(tut, tutorialsDone)
	{
		if (tut->tutorial == _tutorial)
		{
			tut->when = Framework::DateTime::get_current_from_system();
			++tut->howMany;
			return;
		}
	}
	tutorialsDone.push_back(TutorialDone(_tutorial, Framework::DateTime::get_current_from_system()));
}

bool PlayerSetup::was_tutorial_done(Framework::LibraryName const& _tutorial, OPTIONAL_ OUT_ int* _minutesSince, OPTIONAL_ OUT_ int* _howManyTimes) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (game->is_using_quick_game_player_profile())
		{
			return false;
		}
	}
	for_every(tut, tutorialsDone)
	{
		if (tut->tutorial == _tutorial)
		{
			if (OPTIONAL_ OUT_ _minutesSince)
			{
				Framework::Time timeSince = Framework::DateTime::get_current_from_system() - tut->when;
				assign_optional_out_param(_minutesSince, TypeConversions::Normal::f_i_cells(timeSince.to_minutes()));
			}
			assign_optional_out_param(_howManyTimes, tut->howMany);
			return true;
		}
	}
	assign_optional_out_param(_minutesSince, 0);
	assign_optional_out_param(_howManyTimes, 0);
	return false;
}

void PlayerSetup::set_last_tutorial_started(Framework::LibraryName const& _tutorial)
{
	lastTutorialStarted = _tutorial;
}

Framework::LibraryName const& PlayerSetup::get_last_tutorial_started() const
{
	return lastTutorialStarted;
}

//

void PlayerPreferencesColouredIcon::lookup()
{
	if (auto* library = Framework::Library::get_current())
	{
		icon.find_may_fail(library);
	}
}

void PlayerPreferencesColouredIcon::drop()
{
	icon.clear();
}

//

void PlayerPreferencesIcons::lookup()
{
	if (auto* library = Framework::Library::get_current())
	{
		small.find_may_fail(library);
		big.find_may_fail(library);
	}
}

void PlayerPreferencesIcons::drop()
{
	small.clear();
	big.clear();
}

//

float const PlayerPreferences::nominalDoorHeight = 2.0f;

Colour PlayerPreferences::defaultWeaponEnergyColour = Colour::white;
Colour PlayerPreferences::defaultHealthEnergyColour = Colour::white;
Colour PlayerPreferences::defaultTimeColour = Colour::white;
Colour PlayerPreferences::defaultGameStatsDistColour = Colour::white;
Colour PlayerPreferences::defaultGameStatsTimeColour = Colour::white;

PlayerPreferencesIcons PlayerPreferences::defaultHealthIcons;
PlayerPreferencesIcons PlayerPreferences::defaultWeaponIcons;
PlayerPreferencesIcons PlayerPreferences::defaultItemIcons;

PlayerPreferencesColouredIcon PlayerPreferences::defaultEnergyTransferUp;
PlayerPreferencesColouredIcon PlayerPreferences::defaultEnergyTransferDown;

Colour PlayerPreferences::defaultEnergyDepositColour = Colour::white;
Colour PlayerPreferences::defaultEnergyWithdrawColour = Colour::white;

bool PlayerPreferences::are_currently_flickering_lights_allowed()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().flickeringLights == Option::Allow;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::is_camera_rumble_allowed()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().cameraRumble == Option::Allow;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::is_crouching_allowed()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().allowCrouch != Option::Disallow;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::should_show_in_hand_equipment_stats_at_glance()
{
#ifdef FORCE_SAVING_ICON
	return true;
#endif
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().showInHandEquipmentStatsAtGlance == Option::On;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::should_show_saving_icon()
{
#ifdef BUILD_NON_RELEASE
	return true; // always show during development
#endif
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().showSavingIcon;
	}
	else
	{
		return false;
	}
}

bool PlayerPreferences::should_show_map()
{
	if (GameSettings::get().difficulty.mapAlwaysAvailable)
	{
		return true;
	}
	else
	{
		if (auto* ps = PlayerSetup::access_current_if_exists())
		{
			return ps->get_preferences().mapAvailable;
		}
		else
		{
			return false;
		}
	}
}

Option::Type PlayerPreferences::get_pilgrim_overlay_locked_to_head()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().pilgrimOverlayLockedToHead;
	}
	else
	{
		return Option::Allow;
	}
}

DoorDirectionsVisible::Type PlayerPreferences::get_door_directions_visible()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().doorDirectionsVisible;
	}
	else
	{
		return DoorDirectionsVisible::Auto;
	}
}

Optional<float> PlayerPreferences::get_forced_environment_mix_pt()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().forcedEnvironmentMixPt;
	}
	else
	{
		return NP;
	}
}

bool PlayerPreferences::should_allow_follow_guidance()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().followGuidance == Option::Allow;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::should_skip_content(SkipContentInfo const& _sci)
{
	return should_skip_content(_sci.generalProgressTag, _sci.gameSlotThreshold, _sci.profileThreshold);
}

bool PlayerPreferences::should_skip_content(Name const & _generalProgressTag, Optional<float> const& _slotAutoThreshold, Optional<float> const& _profileAutoThreshold)
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		if (ps->get_preferences().skipContent == Option::Off)
		{
			return false;
		}
		float profileGP = ps->get_general_progress().get_tag(_generalProgressTag);
		float slotGP = ps->get_active_game_slot()? ps->get_active_game_slot()->get_general_progress().get_tag(_generalProgressTag) : 0.0f;
		if (ps->get_preferences().skipContent == Option::On)
		{
			return profileGP > 0.0f || slotGP > 0.0f;
		}
		return profileGP >= _profileAutoThreshold.get(3.0f) - 0.01f ||
			   slotGP >= _slotAutoThreshold.get(3.0f) - 0.01f;
	}
	else
	{
		return false;
	}
}

bool PlayerPreferences::does_use_sliding_locomotion()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().slidingLocomotion == Option::On;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::should_hud_follow_pitch()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().hudFollowsPitch;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::should_show_tips_during_game()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().showTipsDuringGame;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::should_play_narrative_voiceovers()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().narrativeVoiceovers != Option::Off;
	}
	else
	{
		return true;
	}
}

bool PlayerPreferences::should_play_pilgrim_overlay_voiceovers()
{
	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		return ps->get_preferences().pilgrimOverlayVoiceovers != Option::Off;
	}
	else
	{
		return true;
	}
}

PlayerPreferences::PlayerPreferences()
: doorHeight(Framework::Door::get_default_nominal_door_height())
{
}

#define LOAD_COLOUR(colour) \
	for_every(child, node->children_named(TXT(#colour))) \
	{ \
		colour = Colour::black; \
		colour.load_from_xml(child); \
	}

#define SAVE_COLOUR(colour) \
	colour.save_to_xml_child_node(node, TXT(#colour)); \

#define LOAD_OPTIONAL_COLOUR(colour) \
	for_every(child, node->children_named(TXT(#colour))) \
	{ \
		colour.set(Colour::black); \
		colour.access().load_from_xml(child); \
	}

#define SAVE_OPTIONAL_COLOUR(colour) \
	if (colour.is_set()) \
	{ \
		colour.get().save_to_xml_child_node(node, TXT(#colour)); \
	}

#define LOAD_OPTIONAL_FLOAT(variable) \
	{ \
		variable.load_from_xml(node, TXT(#variable)); \
	}

#define SAVE_OPTIONAL_FLOAT(variable) \
	{ \
		if (variable.is_set()) \
		{ \
			node->set_float_attribute(TXT(#variable), variable.get()); \
		} \
	}

#define LOAD_OPTIONAL_BOOL(variable, notSetValue) \
	{ \
		Option::Type opt = notSetValue; \
		if (variable.is_set()) \
		{ \
			opt = variable.get() ? Option::True : Option::False; \
		} \
		opt = Option::parse(node->get_string_attribute(TXT(#variable)).to_char(), opt, Option::True | Option::False | notSetValue); \
		if (opt == Option::True) \
		{ \
			variable = true; \
		} \
		else if (opt == Option::True) \
		{ \
			variable = true; \
		} \
		else \
		{ \
			variable.clear(); \
		} \
	}

#define SAVE_OPTIONAL_BOOL(variable, notSetValue) \
	{ \
		Option::Type opt = notSetValue; \
		if (variable.is_set()) \
		{ \
			opt = variable.get() ? Option::True : Option::False; \
		} \
		node->set_attribute(TXT(#variable), String(Option::to_char(opt))); \
	}

#define LOAD_BOOL(value) \
	value = node->get_bool_attribute(TXT(#value), value);

#define SAVE_BOOL(value) \
	node->set_bool_attribute(TXT(#value), value);

#define LOAD_FLOAT(value) \
	value = node->get_float_attribute(TXT(#value), value);

#define SAVE_FLOAT(value) \
	node->set_float_attribute(TXT(#value), value);

#define LOAD_LIBRARY_NAME(value) \
	value.set_name(node->get_string_attribute(TXT(#value), value.get_name().to_string()));

#define SAVE_LIBRARY_NAME(value) \
	if (value.is_name_valid()) node->set_attribute(TXT(#value), value.get_name().to_string());

#define LOAD_ENUM(Type, variable) \
	{ \
		String attr = node->get_string_attribute(TXT(#variable)); \
		if (!attr.is_empty()) \
		{ \
			variable = Type::parse(attr); \
		} \
	}

#define SAVE_ENUM(Type, variable) \
	{ \
		node->set_attribute(TXT(#variable), Type::to_char(variable)); \
	}

#define LOAD_OPTIONAL_ENUM(Type, variable) \
	{ \
		String attr = node->get_string_attribute(TXT(#variable)); \
		if (!attr.is_empty()) \
		{ \
			if (attr == TXT("auto")) \
			{ \
				variable.clear(); \
			} \
			else \
			{ \
				variable = Type::parse(attr); \
			} \
		} \
	}

#define SAVE_OPTIONAL_ENUM(Type, variable) \
	{ \
		if (variable.is_set()) \
		{ \
			node->set_attribute(TXT(#variable), Type::to_char(variable.get())); \
		} \
		else \
		{ \
			node->set_attribute(TXT(#variable), TXT("auto")); \
		} \
	}

#define LOAD_OPTIONAL_NAME(variable) \
	{ \
		String attr = node->get_string_attribute(TXT(#variable)); \
		if (!attr.is_empty()) \
		{ \
			if (attr == TXT("auto")) \
			{ \
				variable.clear(); \
			} \
			else \
			{ \
				variable = Name(attr); \
			} \
		} \
	}

#define SAVE_OPTIONAL_NAME(variable) \
	{ \
		if (variable.is_set()) \
		{ \
			node->set_attribute(TXT(#variable), variable.get().to_char()); \
		} \
		else \
		{ \
			node->set_attribute(TXT(#variable), TXT("auto")); \
		} \
	}

bool PlayerPreferences::load_from_xml(IO::XML::Node const * _parentNode)
{
	bool result = true;

	for_every(node, _parentNode->children_named(TXT("preferences")))
	{
		LOAD_OPTIONAL_COLOUR(weaponEnergyColour);
		LOAD_OPTIONAL_COLOUR(healthEnergyColour);
		LOAD_OPTIONAL_COLOUR(timeColour);
		LOAD_OPTIONAL_COLOUR(gameStatsDistColour);
		LOAD_OPTIONAL_COLOUR(gameStatsTimeColour);
		//
		LOAD_LIBRARY_NAME(healthIcons.small);
		LOAD_LIBRARY_NAME(healthIcons.big);
		LOAD_LIBRARY_NAME(weaponIcons.small);
		LOAD_LIBRARY_NAME(weaponIcons.big);
		LOAD_LIBRARY_NAME(itemIcons.small);
		LOAD_LIBRARY_NAME(itemIcons.big);
		LOAD_LIBRARY_NAME(energyTransferUp.icon);
		LOAD_OPTIONAL_COLOUR(energyTransferUp.colour);
		LOAD_LIBRARY_NAME(energyTransferDown.icon);
		LOAD_OPTIONAL_COLOUR(energyTransferDown.colour);
		LOAD_OPTIONAL_COLOUR(energyWithdrawColour);
		LOAD_OPTIONAL_COLOUR(energyDepositColour);
		//
		turnCounter = Option::parse(node->get_string_attribute(TXT("turnCounter")).to_char(), turnCounter, Option::On | Option::Off | Option::Allow);
		allowCrouch = Option::parse(node->get_string_attribute(TXT("allowCrouch")).to_char(), allowCrouch, Option::Allow | Option::Disallow | Option::Auto);
		flickeringLights = Option::parse(node->get_string_attribute(TXT("flickeringLights")).to_char(), flickeringLights, Option::Allow | Option::Disallow);
		cameraRumble = Option::parse(node->get_string_attribute(TXT("cameraRumble")).to_char(), cameraRumble, Option::Allow | Option::Disallow);
		showInHandEquipmentStatsAtGlance = Option::parse(node->get_string_attribute(TXT("showInHandEquipmentStatsAtGlance")).to_char(), showInHandEquipmentStatsAtGlance, Option::On | Option::Off);
		rotatingElevators = Option::parse(node->get_string_attribute(TXT("rotatingElevators")).to_char(), rotatingElevators, Option::Allow | Option::Disallow);
		followGuidance = Option::parse(node->get_string_attribute(TXT("followGuidance")).to_char(), followGuidance, Option::Allow | Option::Disallow);
		skipContent = Option::parse(node->get_string_attribute(TXT("skipContent")).to_char(), skipContent, Option::Auto | Option::Off | Option::On);
		narrativeVoiceovers = Option::parse(node->get_string_attribute(TXT("narrativeVoiceovers")).to_char(), narrativeVoiceovers, Option::On | Option::Off);
		pilgrimOverlayVoiceovers = Option::parse(node->get_string_attribute(TXT("pilgrimOverlayVoiceovers")).to_char(), pilgrimOverlayVoiceovers, Option::On | Option::Off);
		LOAD_OPTIONAL_FLOAT(forcedEnvironmentMixPt);
		LOAD_FLOAT(vrHapticFeedback);
		LOAD_BOOL(movedRoomScaleAtLeastOnce);
		LOAD_BOOL(immobileVRSnapTurn);
		LOAD_FLOAT(immobileVRSnapTurnAngle);
		LOAD_FLOAT(immobileVRContinuousTurnSpeed);
		LOAD_FLOAT(immobileVRSpeed);
		LOAD_ENUM(PlayerVRMovementRelativeTo, immobileVRMovementRelativeTo);
		LOAD_BOOL(showTipsOnLoading);
		LOAD_BOOL(showTipsDuringGame);
		LOAD_BOOL(swapJoysticks);
		LOAD_BOOL(showSavingIcon);
		LOAD_BOOL(mapAvailable);
		LOAD_ENUM(DoorDirectionsVisible, doorDirectionsVisible);
		pilgrimOverlayLockedToHead = Option::parse(node->get_string_attribute(TXT("pilgrimOverlayLockedToHead")).to_char(), pilgrimOverlayLockedToHead, Option::Allow | Option::On | Option::Off);
		LOAD_OPTIONAL_ENUM(MeasurementSystem, measurementSystem);
		LOAD_OPTIONAL_NAME(language);
		LOAD_BOOL(subtitles);
		LOAD_FLOAT(subtitlesScale);
		LOAD_BOOL(showRealTime);
		LOAD_OPTIONAL_BOOL(autoMainEquipment, Option::Auto);
		LOAD_OPTIONAL_BOOL(rightHanded, Option::Auto);
		LOAD_FLOAT(doorHeight);
		LOAD_BOOL(scaleUpPlayer);
		LOAD_BOOL(keepInWorld);
		LOAD_FLOAT(keepInWorldSensitivity);
		slidingLocomotion = Option::parse(node->get_string_attribute(TXT("slidingLocomotion")).to_char(), slidingLocomotion, Option::Auto | Option::On);
		LOAD_FLOAT(pocketsVerticalAdjustment);
		LOAD_FLOAT(sightsOuterRingRadius);
		LOAD_COLOUR(sightsColour);
		LOAD_ENUM(HitIndicatorType, hitIndicatorType);
		LOAD_BOOL(healthIndicator);
		LOAD_FLOAT(healthIndicatorIntensity);
		LOAD_FLOAT(hitIndicatorIntensity);
		LOAD_BOOL(highlightInteractives);
		LOAD_BOOL(useShaderVignetteForMovement);
		LOAD_BOOL(useVignetteForMovement);
		LOAD_FLOAT(vignetteForMovementStrength);
		LOAD_FLOAT(cartsTopSpeed);
		LOAD_FLOAT(cartsSpeedPct);
		gameInputOptions.load_from_xml_attribute_or_child_node(node, TXT("gameInputOptions"));

		healthIcons.lookup();
		weaponIcons.lookup();
		itemIcons.lookup();

		energyTransferUp.lookup();
		energyTransferDown.lookup();
	}

	if (subtitlesScale == 0.0f)
	{
		subtitlesScale = 1.0f;
	}

	return result;
}

bool PlayerPreferences::save_to_xml(IO::XML::Node * _parentNode) const
{
	bool result = true;

	IO::XML::Node* node = _parentNode->add_node(TXT("preferences"));

	SAVE_OPTIONAL_COLOUR(weaponEnergyColour);
	SAVE_OPTIONAL_COLOUR(healthEnergyColour);
	SAVE_OPTIONAL_COLOUR(timeColour);
	SAVE_OPTIONAL_COLOUR(gameStatsDistColour);
	SAVE_OPTIONAL_COLOUR(gameStatsTimeColour);
	//
	SAVE_LIBRARY_NAME(healthIcons.small);
	SAVE_LIBRARY_NAME(healthIcons.big);
	SAVE_LIBRARY_NAME(weaponIcons.small);
	SAVE_LIBRARY_NAME(weaponIcons.big);
	SAVE_LIBRARY_NAME(itemIcons.small);
	SAVE_LIBRARY_NAME(itemIcons.big);
	SAVE_LIBRARY_NAME(energyTransferUp.icon);
	SAVE_OPTIONAL_COLOUR(energyTransferUp.colour);
	SAVE_LIBRARY_NAME(energyTransferDown.icon);
	SAVE_OPTIONAL_COLOUR(energyTransferDown.colour);
	SAVE_OPTIONAL_COLOUR(energyWithdrawColour);
	SAVE_OPTIONAL_COLOUR(energyDepositColour);
	//
	node->set_attribute(TXT("turnCounter"), String(Option::to_char(turnCounter)));
	node->set_attribute(TXT("allowCrouch"), String(Option::to_char(allowCrouch)));
	node->set_attribute(TXT("flickeringLights"), String(Option::to_char(flickeringLights)));
	node->set_attribute(TXT("cameraRumble"), String(Option::to_char(cameraRumble)));
	node->set_attribute(TXT("showInHandEquipmentStatsAtGlance"), String(Option::to_char(showInHandEquipmentStatsAtGlance)));
	node->set_attribute(TXT("rotatingElevators"), String(Option::to_char(rotatingElevators)));
	node->set_attribute(TXT("followGuidance"), String(Option::to_char(followGuidance)));
	node->set_attribute(TXT("skipContent"), String(Option::to_char(skipContent)));
	node->set_attribute(TXT("narrativeVoiceovers"), String(Option::to_char(narrativeVoiceovers)));
	node->set_attribute(TXT("pilgrimOverlayVoiceovers"), String(Option::to_char(pilgrimOverlayVoiceovers)));
	SAVE_OPTIONAL_FLOAT(forcedEnvironmentMixPt);
	SAVE_FLOAT(vrHapticFeedback);
	SAVE_BOOL(movedRoomScaleAtLeastOnce);
	SAVE_BOOL(immobileVRSnapTurn);
	SAVE_FLOAT(immobileVRSnapTurnAngle);
	SAVE_FLOAT(immobileVRContinuousTurnSpeed);
	SAVE_FLOAT(immobileVRSpeed);
	SAVE_ENUM(PlayerVRMovementRelativeTo, immobileVRMovementRelativeTo);
	SAVE_BOOL(showTipsOnLoading);
	SAVE_BOOL(showTipsDuringGame);
	SAVE_BOOL(swapJoysticks);
	SAVE_BOOL(showSavingIcon);
	SAVE_BOOL(mapAvailable);
	SAVE_ENUM(DoorDirectionsVisible, doorDirectionsVisible);
	node->set_attribute(TXT("pilgrimOverlayLockedToHead"), String(Option::to_char(pilgrimOverlayLockedToHead)));
	SAVE_OPTIONAL_ENUM(MeasurementSystem, measurementSystem);
	SAVE_OPTIONAL_NAME(language);
	SAVE_BOOL(subtitles);
	SAVE_FLOAT(subtitlesScale);
	SAVE_BOOL(showRealTime);
	SAVE_OPTIONAL_BOOL(autoMainEquipment, Option::Auto);
	SAVE_OPTIONAL_BOOL(rightHanded, Option::Auto);
	SAVE_FLOAT(doorHeight);
	SAVE_BOOL(scaleUpPlayer);
	SAVE_BOOL(keepInWorld);
	SAVE_FLOAT(keepInWorldSensitivity);
	node->set_attribute(TXT("slidingLocomotion"), String(Option::to_char(slidingLocomotion)));
	SAVE_FLOAT(pocketsVerticalAdjustment);
	SAVE_FLOAT(sightsOuterRingRadius);
	SAVE_COLOUR(sightsColour);
	SAVE_ENUM(HitIndicatorType, hitIndicatorType);
	SAVE_BOOL(healthIndicator);
	SAVE_FLOAT(healthIndicatorIntensity);
	SAVE_FLOAT(hitIndicatorIntensity);
	SAVE_BOOL(highlightInteractives);
	SAVE_BOOL(useShaderVignetteForMovement);
	SAVE_BOOL(useVignetteForMovement);
	SAVE_FLOAT(vignetteForMovementStrength);
	SAVE_FLOAT(cartsTopSpeed);
	SAVE_FLOAT(cartsSpeedPct);
	node->set_attribute(TXT("gameInputOptions"), gameInputOptions.to_string());

	return result;
}

void PlayerPreferences::lookup_defaults()
{
	defaultWeaponEnergyColour.parse_from_string(String(TXT("handWeapon")));
	defaultHealthEnergyColour.parse_from_string(String(TXT("handHealth")));
	defaultTimeColour = Colour::greyLight;
	defaultGameStatsDistColour = Colour::c64LightGreen;
	defaultGameStatsTimeColour = Colour::c64LightBlue;

	defaultHealthIcons.small.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - health - indicator"))));
	defaultHealthIcons.big.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - health - big"))));
	defaultHealthIcons.lookup();

	defaultWeaponIcons.small.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - weapon - indicator"))));
	defaultWeaponIcons.big.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - weapon - big"))));
	defaultWeaponIcons.lookup();

	defaultItemIcons.small.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - item - indicator"))));
	defaultItemIcons.big.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - item - big"))));
	defaultItemIcons.lookup();

	defaultEnergyTransferDown.icon.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - energy deposit x2"))));
	defaultEnergyTransferDown.colour = Colour::white;
	defaultEnergyTransferDown.lookup();

	defaultEnergyTransferUp.icon.set_name(hardcoded Framework::LibraryName(String(TXT("pilgrims:hand - pure - energy withdraw x2"))));
	defaultEnergyTransferUp.colour = Colour::white;
	defaultEnergyTransferUp.lookup();

	defaultEnergyDepositColour.parse_from_string(String(TXT("energy_deposit")));
	defaultEnergyWithdrawColour.parse_from_string(String(TXT("energy_withdraw")));
}

void PlayerPreferences::drop_defaults()
{
	defaultHealthIcons.drop();
	defaultWeaponIcons.drop();
	defaultItemIcons.drop();
	defaultEnergyTransferDown.drop();
	defaultEnergyTransferUp.drop();
}

MeasurementSystem::Type PlayerPreferences::get_measurement_system() const
{
	if (measurementSystem.is_set())
	{
		return measurementSystem.get();
	}

	return get_auto_measurement_system();
}

MeasurementSystem::Type PlayerPreferences::get_auto_measurement_system()
{
#ifdef AN_WINDOWS
	DWORD dwMSys;
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE | LOCALE_RETURN_NUMBER, (LPWSTR)&dwMSys, sizeof(DWORD));

	return dwMSys == 1 ? MeasurementSystem::Imperial : MeasurementSystem::Metric;
#else
	todo_note(TXT("implement measurement"));
	return MeasurementSystem::Metric;
#endif
}

Name PlayerPreferences::get_language() const
{
	if (language.is_set())
	{
		if (Framework::LocalisedStrings::find_language_index(language.get()) != NONE)
		{
			return language.get();
		}
	}

	if (Framework::LocalisedStrings::get_suggested_default_language().is_valid())
	{
		return Framework::LocalisedStrings::get_suggested_default_language();
	}

	Name const & systemLanguage = Framework::LocalisedStrings::get_system_language();
	if (systemLanguage.is_valid())
	{
		return systemLanguage;
	}

	return Framework::LocalisedStrings::get_default_language();
}

void PlayerPreferences::apply() const
{
	Name useLanguage = get_language();
	if (Framework::LocalisedStrings::find_language_index(useLanguage) != NONE)
	{
		Framework::LocalisedStrings::set_language(useLanguage);
	}

	if (auto* gid = Framework::GameInputDefinitions::get_definition(NAME(game))) // hardcoded
	{
		if (::System::Input* input = ::System::Input::get())
		{
			for_every(opt, gid->get_options())
			{
				if (gameInputOptions.get_tag(opt->name))
				{
					input->set_usage_tag(opt->name);
				}
				else
				{
					input->remove_usage_tag(opt->name);
				}
			}
		}
	}

	SubtitleSystem::withSubtitles = subtitles;
	SubtitleSystem::subtitlesScale = subtitlesScale;

	Framework::LightSource::allowFlickeringLights = flickeringLights == Option::Allow;

	apply_to_vr_and_room_generation_info();

	apply_to_game_tags();

	apply_controls_to_main_config();
}

void PlayerPreferences::apply_to_game_tags() const
{
	if (auto* g = Game::get())
	{
		auto& gameTags = g->access_game_tags();
		if (rotatingElevators == Option::Disallow)
		{
			gameTags.remove_tag(NAME(rotatingElevators));
		}
		else
		{
			gameTags.set_tag(NAME(rotatingElevators));
		}
	}
}

float PlayerPreferences::calculate_vr_scaling(float _doorHeight, bool _scaleUpPlayer, OPTIONAL_ OUT_ float* _doorHeightToUse)
{
	float vrScaling = 1.0f;
	float rgiDoorHeight = _doorHeight;
	float minNominalDoorHeight = nominalDoorHeight - 0.1f;
	float maxNominalDoorHeight = nominalDoorHeight + 0.1f;
	if (_doorHeight < minNominalDoorHeight)
	{
		rgiDoorHeight = minNominalDoorHeight;
	}
	else if (_doorHeight > maxNominalDoorHeight)
	{
		rgiDoorHeight = maxNominalDoorHeight + (_doorHeight - maxNominalDoorHeight) * 0.3f; // scale a little bit with the person, but rely more on actual scaling
		if (_scaleUpPlayer)
		{
			rgiDoorHeight = _doorHeight;
		}
	}
	vrScaling = rgiDoorHeight / _doorHeight;
	assign_optional_out_param(_doorHeightToUse, rgiDoorHeight);
	return vrScaling;
}

void PlayerPreferences::apply_to_vr_and_room_generation_info() const
{
	MainConfig::access_global().set_vr_haptic_feedback(vrHapticFeedback);
	if (auto* vr = VR::IVR::get())
	{
		if (rightHanded.is_set())
		{
			vr->set_dominant_hand_override(rightHanded.get() ? Hand::Right : Hand::Left);
		}
		else
		{
			vr->set_dominant_hand_override(NP);
		}
	}
	{
		float rgiDoorHeight;
		float vrScaling = calculate_vr_scaling(doorHeight, scaleUpPlayer, &rgiDoorHeight);
		RoomGenerationInfo::access().set_door_height(rgiDoorHeight);
		MainConfig::access_global().set_vr_scaling(vrScaling);
	}
	RoomGenerationInfo::access().set_allow_crouch(allowCrouch); // game modifiers should setup allowCrouch through RunSetup!
}

void PlayerPreferences::apply_controls_to_main_config() const
{
	MainConfig::access_global().set_joystick_input_swapped(swapJoysticks);
}

bool PlayerPreferences::is_option_set(Name const & _option) const
{
	if (_option == TXT("vignetteForMovement")) return useShaderVignetteForMovement && useVignetteForMovement && vignetteForMovementStrength != 0.0f; // this is for shaders
	return false;
}

float PlayerPreferences::get_door_height_from_eye_level(OUT_ float* _height)
{
	float eyeLevel = 1.67f;

	if (auto* vr = VR::IVR::get())
	{
		if (vr->is_controls_view_valid())
		{
			eyeLevel = vr->get_controls_raw_view().get_translation().z;
		}
	}

	// the values here are based on my height
	float const myHeight = 1.75f;
	float const myEyeLevel = 1.69f;
	float height = 0.7f * (eyeLevel * myHeight / myEyeLevel) + 0.3f * (eyeLevel + (myHeight - myEyeLevel));
	float doorHeight = clamp(height * 2.0f / 1.75f, 1.0f, 3.0f);

	doorHeight = round_to(doorHeight, 0.05f);

	assign_optional_out_param(_height, height);

	return doorHeight;
}
