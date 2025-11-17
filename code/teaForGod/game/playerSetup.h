#pragma once

#include "..\..\core\enums.h"
#include "..\..\core\tags\tag.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\types\option.h"
#include "..\..\core\types\optional.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\time\dateTime.h"
#include "..\..\framework\time\distance.h"
#include "..\..\framework\time\time.h"
#include "..\pilgrim\pilgrimSetup.h"
#include "..\library\gameDefinition.h"
#include "..\library\gameSubDefinition.h"
#include "..\teaEnums.h"
#include "gameSettings.h"
#include "gameState.h"
#include "missionResult.h"
#include "missionState.h"
#include "persistence.h"
#include "runSetup.h"

namespace Framework
{
	class TexturePart;
};

#ifdef small
#undef small
#endif

namespace TeaForGodEmperor
{
	struct PersistenceWeaponParts;
	struct SkipContentInfo;
	struct PlayerSetup;

	struct PlayerPreferencesColouredIcon
	{
		Framework::UsedLibraryStored<Framework::TexturePart> icon;
		Optional<Colour> colour;

		void lookup();
		void drop();
	};

	struct PlayerPreferencesIcons
	{
		Framework::UsedLibraryStored<Framework::TexturePart> small;
		Framework::UsedLibraryStored<Framework::TexturePart> big;

		void lookup();
		void drop();
	};

	struct PlayerPreferences
	{
	public:
		static float const nominalDoorHeight; // below that we use vr scaling

		static Colour defaultWeaponEnergyColour;
		static Colour defaultHealthEnergyColour;
		static Colour defaultTimeColour;

		static Colour defaultGameStatsDistColour;
		static Colour defaultGameStatsTimeColour;

		static PlayerPreferencesIcons defaultHealthIcons;
		static PlayerPreferencesIcons defaultWeaponIcons;
		static PlayerPreferencesIcons defaultItemIcons;

		static PlayerPreferencesColouredIcon defaultEnergyTransferUp;
		static PlayerPreferencesColouredIcon defaultEnergyTransferDown;

		static Colour defaultEnergyWithdrawColour;
		static Colour defaultEnergyDepositColour;

		static void lookup_defaults();
		static void drop_defaults();

	public:
		static bool are_currently_flickering_lights_allowed();
		static bool is_camera_rumble_allowed();
		static bool is_crouching_allowed();
		static bool should_show_in_hand_equipment_stats_at_glance();
		static bool should_show_saving_icon();
		static bool should_show_map();
		static bool should_allow_follow_guidance();
		static bool should_skip_content(SkipContentInfo const & _sci);
		static bool should_skip_content(Name const& _generalProgressTag, Optional<float> const & _slotAutoThreshold = NP, Optional<float> const & _profileAutoThreshold = NP);
		static bool does_use_sliding_locomotion();
		static bool should_hud_follow_pitch();
		static bool should_show_tips_during_game();
		static bool should_play_narrative_voiceovers();
		static bool should_play_pilgrim_overlay_voiceovers();
		static Option::Type get_pilgrim_overlay_locked_to_head();
		static DoorDirectionsVisible::Type get_door_directions_visible();
		static Optional<float> get_forced_environment_mix_pt();

	public:
		// colours
		Optional<Colour> weaponEnergyColour;
		Optional<Colour> healthEnergyColour;
		Optional<Colour> timeColour;

		Optional<Colour> gameStatsDistColour;
		Optional<Colour> gameStatsTimeColour;

		PlayerPreferencesIcons healthIcons;
		PlayerPreferencesIcons weaponIcons;
		PlayerPreferencesIcons itemIcons;

		PlayerPreferencesColouredIcon energyTransferUp;
		PlayerPreferencesColouredIcon energyTransferDown;
		Optional<Colour> energyWithdrawColour;
		Optional<Colour> energyDepositColour;

		// general
		Optional<MeasurementSystem::Type> measurementSystem; // auto/imperial/metric
		Optional<Name> language;
		bool subtitles = true; // subtitles on by default
		float subtitlesScale = 1.0f;
		bool showRealTime = true;
		bool hudFollowsPitch = true; // not changeable right now

		Option::Type turnCounter = Option::Allow; // off/on/allow

		Optional<bool> autoMainEquipment; // if not set, controller dependent (auto hide main equipment)
		AUTO_ bool controllerAutoMainEquipment = false; // controller sets it (every frame :/ )

		Option::Type allowCrouch = Option::Auto; // allow/disallow/auto
		Option::Type flickeringLights = Option::Allow; // allow/disallow
		Option::Type cameraRumble = Option::Allow; // allow/disallow
		Option::Type showInHandEquipmentStatsAtGlance = Option::On; // on/off
		Option::Type rotatingElevators = Option::Allow; // allow/disallow
		Option::Type followGuidance = Option::Allow; // allow/disallow
		Option::Type skipContent = Option::Auto; // off (never skip) / auto (if seen a few times) / on (if seen at least once)
		Option::Type narrativeVoiceovers = Option::On; // narrative voiceovers, if off, no subtitles neither
		Option::Type pilgrimOverlayVoiceovers = Option::On; // if off, pilgrim overlay shows info, without sound/voiceovers

		Optional<float> forcedEnvironmentMixPt;

		float vrHapticFeedback = 1.0f; // might be overriden by player setup

		bool movedRoomScaleAtLeastOnce = false;

		bool immobileVRSnapTurn = false; // if 0, no snap turning, continuous
		float immobileVRSnapTurnAngle = 45; //
		float immobileVRContinuousTurnSpeed = 180.0f; // angles per second
		float immobileVRSpeed = 1.25f;
		PlayerVRMovementRelativeTo::Type immobileVRMovementRelativeTo = PlayerVRMovementRelativeTo::Head;

		bool showTipsOnLoading = true;
		bool showTipsDuringGame = true;

		Tags gameInputOptions;

		Optional<bool> rightHanded;
		bool swapJoysticks = false;

		bool showSavingIcon = false; // game should use should_show_in_hand_equipment_stats_at_glance to get this value
		
		bool mapAvailable = false; // only if not mapAlwaysAvailable in GameSettings.difficulty

		DoorDirectionsVisible::Type doorDirectionsVisible = DoorDirectionsVisible::Auto;

		Option::Type pilgrimOverlayLockedToHead = Option::Allow; // will be always relative to head (allow/on/off)

		bool useShaderVignetteForMovement = false;
		bool useVignetteForMovement = false;
		float vignetteForMovementStrength = 0.5f;

		bool cartsAutoMovement = false; // hardcoded
		bool cartsStopAndStay = true; // hardcoded
		float cartsTopSpeed = 0.0f;
		float cartsSpeedPct = 1.0f;

		float doorHeight; // because it depends on the player
		bool scaleUpPlayer = true; // if we want only ceiling to move up but maintain the scale of player and other characters
		bool keepInWorld = true; // pilgrim keeper is active
		float keepInWorldSensitivity = 1.0f;

		Option::Type slidingLocomotion = Option::Auto; // auto/on
		
		float pocketsVerticalAdjustment = 0.0f; 

		float sightsOuterRingRadius = 0.004f;
		Colour sightsColour = Colour::red;

		HitIndicatorType::Type hitIndicatorType = HitIndicatorType::SidesOnly;
		bool healthIndicator = true;
		float healthIndicatorIntensity = 1.0f;
		float hitIndicatorIntensity = 1.0f;
		bool highlightInteractives = true;

		PlayerPreferences();

		static float get_door_height_from_eye_level(OUT_ float* _height = nullptr);

		bool should_auto_main_equipment() const { return autoMainEquipment.get(controllerAutoMainEquipment); }

		// will auto load and save "preferences"
		bool load_from_xml(IO::XML::Node const * _parentNode);
		bool save_to_xml(IO::XML::Node * _parentNode) const;

		void apply() const; // language, rgi
		void apply_to_game_tags() const; // rotating elevators
		void apply_to_vr_and_room_generation_info() const; // door height, allow crouch
		void apply_controls_to_main_config() const; // door height, allow crouch

		MeasurementSystem::Type get_measurement_system() const;
		static MeasurementSystem::Type get_auto_measurement_system();
		Name get_language() const;

		bool is_option_set(Name const & _option) const;

		static float calculate_vr_scaling(float _doorHeight, bool _scaleUpPlayer, OPTIONAL_ OUT_ float * _doorHeightToUse = nullptr);
	};

	struct PlayerStats
	{
		Framework::Time timeActive = Framework::Time::zero(); // overall time active
		Framework::Time timeRun = Framework::Time::zero(); // time in runs (not in in-game menu, but game HUBs count)
		Framework::Distance distance = Framework::Distance::zero();
		int freshStarts = 0;
		int runs = 0;
		int kills = 0;
		int deaths = 0;
		Energy experience = Energy::zero();
		int allEXMs = 0;
		int meritPoints = 0;

		bool load_from_xml(IO::XML::Node const* _parentNode);
		bool save_to_xml(IO::XML::Node * _parentNode) const;
	};

	struct PlayerGameSlot
	: public RefCountObject
	{
		Framework::DateTime lastTimePlayed = Framework::DateTime::none();
		int id = NONE;
		int buildNo = 0; // updated when starting a game (or creating a slot)
		int buildAcknowledgedChangesFlags = 0; // if buildNo differs from current build, we should acknowledge changes 
		int teaBoxSeed;
		PlayerStats stats;
		Persistence persistence;
		PilgrimSetup pilgrimSetup; // this is setup for game slot, not the one currently being used by a pilgrim (this one is in pilgrim)
		RunSetup runSetup;
		Framework::UsedLibraryStored<GameDefinition> gameDefinition;
		Array<Framework::UsedLibraryStored<GameSubDefinition>> gameSubDefinitions;
		Framework::UsedLibraryStored<MissionsDefinition> missionsDefinition; // current missions definition, it may change (change means big irreversible change)

		Tags gameModifiers;
		
		Tags generalProgress; // used for features, to show stuff, prepare in a specific way

		Array<Framework::LibraryName> unlockedPilgrimages;
		Array<Framework::LibraryName> reachedPilgrimages; // all that have been ever reached

		Array<Framework::LibraryName> playedRecentlyPilgrimages; // all that've been played since pilgrimage start/load
		Array<Framework::LibraryName> unlockablePlayedRecentlyPilgrimages; // on death, played ale cleared and become unlockable

		GameSlotMode::Type gameSlotMode = GameSlotMode::Story; // when is switched, game states are reset, when switching from missions you lose progress, use set_game_slot_mode
		Array<GameSlotMode::Type> gameSlotModesAvailable;
		Array<GameSlotMode::Type> gameSlotModesConfirmed;

		Framework::LibraryName restartAtPilgrimage;

		RefCountObjectPtr<GameState> startUsingGameState; // this gets updated when new last checkpoint is created
		RefCountObjectPtr<GameState> startedUsingGameState;
		RefCountObjectPtr<GameState> lastMoment; // saving automatically or when interrupted
		RefCountObjectPtr<GameState> checkpoint; // in haven/chapter start
		RefCountObjectPtr<GameState> atLeastHalfHealth; // automatically but when health is at least half health
		RefCountObjectPtr<GameState> chapterStart;

		Concurrency::SpinLock missionStateLock;
		RefCountObjectPtr<MissionState> missionState; // if not present, no mission active
		RefCountObjectPtr<MissionResult> lastMissionResult; // created on mission end and destroyed when mission starts, used to show by (de)briefer the results and also for summary when ended
		bool ignoreLastMissionResultForSummary = false; // this is used to ignore last mission result when starting task and exiting before it actually started, this is not saved as it is used only when starting the game and if we exit before actually starting a mission

		PlayerGameSlot();

		String get_ui_name() const { return id != NONE? String::printf(TXT("#%i"), id) : String(TXT("--")); }

		Tags& access_general_progress() { return generalProgress; }
		Tags const& get_general_progress() const { return generalProgress; }

		GameDefinition const* get_game_definition() const;
		Array< Framework::UsedLibraryStored<GameSubDefinition>> const& get_game_sub_definitions() const { return gameSubDefinitions; }
		void set_game_definition(GameDefinition const* _gameDefinition, Array<GameSubDefinition*> const& _gameSubDefinitions, bool _newGameDefinition, bool _setAsCurrent);
		void choose_game_definition(); // to make it current/chosen/active
		void update_mission_state(); // to make sure it points at valid values

		void prepare_missions_definition();
		MissionsDefinition const* get_missions_definition() const;

		void force_game_definitions_for_demo();
		void fill_game_sub_definitions_if_empty();
		void fill_default_game_sub_definitions();

		GameState* get_game_state_to_continue() const;
		bool has_any_game_state() const;

		// this should be called only when created a new game slot (not setup, created as through the constructor) or when pilgrimage has started
		// if any particular difference has been handled, we will acknowledge it
		void update_build_no_to_current(); 

		void clear_persistence_and_use_starting_gear(PilgrimGear const* _startingGear = nullptr); // will clear persistence (only weapon parts and gear related)
		
		bool has_any_game_states() const;
		void clear_game_states();
		void clear_game_states_and_mission_state_and_pilgrimages();
		void clear_mission_state();

		void create_new_mission_state();
		
		MissionResult* get_last_mission_result() { return lastMissionResult.get(); }
		MissionResult* create_new_last_mission_result();
		void clear_last_mission_result();
		void ignore_last_mission_result_for_summary(bool _ignore = true) { ignoreLastMissionResultForSummary = _ignore; }
		bool should_ignore_last_mission_result_for_summary() const { return ignoreLastMissionResultForSummary; }

		void update_last_time_played();

		bool sync_unlock_pilgrimage(Framework::LibraryName const& _pilgrimage);
		bool ignore_sync_unlock_pilgrimage(Framework::LibraryName const& _pilgrimage); // if we can ignore sync
		void sync_reached_pilgrimage(Framework::LibraryName const& _pilgrimage, Energy const& _experienceForReachingForFirstTime);

		void sync_update_unlocked_pilgrimages_from_game_definition(PlayerSetup const& _setup, bool _updateRestartAtPilgrimage = false);

		void update_unlocked_pilgrimages(PlayerSetup const & _setup, Array<Framework::UsedLibraryStored<Pilgrimage>> const & _pilgrimages, Array<ConditionalPilgrimage> const & _conditionalPilgrimages, bool _updateRestartAtPilgrimage = false); // in UI, no need to lock, sync

	public:
		void clear_played_recently_pilgrimages(); // this clears both, should be done on each start (not from game state!) / death
		void store_played_recently_pilgrimages_as_unlockables(); // this should be done on death or when entering unlocks menu

	public:
		void set_game_slot_mode(GameSlotMode::Type _newMode);
		GameSlotMode::Type get_game_slot_mode() const { return gameSlotMode; }

		void make_game_slot_mode_available(GameSlotMode::Type _newMode);
		bool is_game_slot_mode_available(GameSlotMode::Type _mode) const;
		int get_game_slot_mode_available_count() const;

		void make_game_slot_mode_confirmed(GameSlotMode::Type _newMode);
		bool is_game_slot_mode_confirmed(GameSlotMode::Type _mode) const;
	};

	/**
	 *	Holds information about preferred player setup:
	 *		1. in game settings, like FADs
	 *		2. unlocked items
	 *		3. preferences (eg. preferred control schemes)
	 */
	struct PlayerSetup
	{
	public:
		static PlayerSetup * access_current_if_exists() { return s_current; }
		static PlayerSetup const * get_current_if_exists() { return s_current; }
		static PlayerSetup & access_current() { an_assert(s_current); return *s_current; }
		static PlayerSetup const & get_current() { an_assert(s_current); return *s_current; }
	private:
		static PlayerSetup* s_current;

	public:
		PlayerSetup();
		~PlayerSetup();

		String const& get_profile_name() const; // there is no profile name stored here, it is accessed from game

		void reset();
		void setup_for_quick_game_player_profile(); // has to have one game slot
		void have_at_least_one_game_slot(); // this is until we get proper game slot management
		void add_game_slot_and_make_it_active();

		void setup_defaults(); // default pilgrim setup
		
		Tags const& get_game_unlocks() const { return gameUnlocks; }
		
		Tags & access_general_progress() { return generalProgress; }
		Tags const& get_general_progress() const { return generalProgress; }

		bool is_option_set(Name const & _option) const;

		void make_current() { s_current = this; }

	public:	// stats
		static void stats__store_game_slot_stats(bool _storeGameSlotStats) { s_storeGameSlotStats = _storeGameSlotStats; }
		void stats__death();
		void stats__run();
		void stats__run_revert(); // if we started the game and immediately went back
		void stats__kill();
		void stats__unlock_all_exms();
		void stats__experience(Energy const& _experience);
		void stats__merit_points(int _meritPoints);
		void stats__increase_run_time(float _time);
		void stats__increase_profile_time(float _time);
		void stats__increase_distance(float _distance);

		void stats__fresh_start();

	private:
		Concurrency::SpinLock statsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PlayerSetup.statsLock"));

	public:
		// general
		PlayerStats const & get_stats() const { return stats; }

		PlayerPreferences& access_preferences() { return preferences; }
		PlayerPreferences const & get_preferences() const { return preferences; }

		// this is general for the player profile
		void redo_tutorials();
		void mark_tutorial_done(Framework::LibraryName const& _tutorial);
		bool was_tutorial_done(Framework::LibraryName const& _tutorial, OPTIONAL_ OUT_ int * _minutesSince = nullptr, OPTIONAL_ OUT_ int * _howManyTimes = nullptr) const;
		void set_last_tutorial_started(Framework::LibraryName const& _tutorial);
		Framework::LibraryName const& get_last_tutorial_started() const;

		bool does_allow_non_game_music() const { return allowNonGameMusic; }
		void allow_non_game_music(bool _allow = true) { allowNonGameMusic = _allow; }

	public:
		// game or tutorial

		// it might be from tutorial system
		PilgrimSetup& access_pilgrim_setup();
		PilgrimSetup const& get_pilgrim_setup() const;

		Array<RefCountObjectPtr<PlayerGameSlot>> const& get_game_slots() const { return gameSlots; }
		// it might be from tutorial system
		PlayerGameSlot const* get_active_game_slot() const;
		PlayerGameSlot* access_active_game_slot();
		void set_active_game_slot(int _idx) { if (gameSlots.is_index_valid(_idx)) activeGameSlotIdx = _idx; else activeGameSlotIdx = NONE; }
		int get_active_game_slot_idx() const { return gameSlots.is_index_valid(activeGameSlotIdx) ? activeGameSlotIdx : NONE; }

		void remove_active_game_slot(bool _allowUpdatingID = false); // _allowUpdatingID=true if we're mid through creating a game slot

	public:
		bool legacy_load_from_xml(IO::XML::Node const * _node); // this is only for backward compatibility

		bool load_player_stats_and_preferences_from_xml(IO::XML::Node const* _node);

		// wrapped together
		bool load_from_xml(IO::XML::Node const* _node, Array<IO::XML::Node const*> const & _gameSlotNodes);
		bool save_to_xml(IO::XML::Node* _node) const;

		bool save_game_slot_to_xml(IO::XML::Node* _node, PlayerGameSlot const* _gameSlot) const;

	private:
		// this is general for player's profile
		bool load_player_common_from_xml(IO::XML::Node const * _node, tchar const* _childNode);
		bool save_player_common_to_xml(IO::XML::Node * _node) const;

		// this is only related to particular game slot
		bool load_game_slots_info_from_xml(IO::XML::Node const * _node);
		bool load_game_slots_from_xml(Array<IO::XML::Node const*> const& _gameSlotNodes);
		bool save_game_slots_info_to_xml(IO::XML::Node * _node) const;

	private:
		// profile-wise
		PlayerStats stats;
		PlayerPreferences preferences;
		Tags gameUnlocks;
		bool allowNonGameMusic =
#ifdef MUSIC_SINCE_START
			true;
#else
			false;
#endif			
		Tags generalProgress; // used for features, to show stuff, prepare in a specific way
		
		// tutorials
		struct TutorialDone
		{
			Framework::LibraryName tutorial;
			Framework::DateTime when = Framework::DateTime::none();
			int howMany = 1; // by default it should be done one time
			TutorialDone() {}
			TutorialDone(Framework::LibraryName const& _tutorial, Framework::DateTime const& _when, int _howMany = 1) : tutorial(_tutorial), when(_when), howMany(_howMany) {}

			bool operator == (TutorialDone const& _other) const
			{
				return tutorial == _other.tutorial &&
					   when == _other.when &&
					   howMany == _other.howMany;
			}
		};
		Array<TutorialDone> tutorialsDone;
		Framework::LibraryName lastTutorialStarted;

		// game slots
		static bool s_storeGameSlotStats;
		int activeGameSlotIdx = NONE;
		int activeGameSlotId = NONE; // this is backup when loading and saving data without loading and saving gameslots
		Array<RefCountObjectPtr<PlayerGameSlot>> gameSlots;
		int gameSlotsCreated = 0;

		void setup_auto_for_game_slots();

	private:
		PlayerSetup(PlayerSetup const& _other); // unavailable
		PlayerSetup& operator=(PlayerSetup const& _other); // unavailable

	};
};
