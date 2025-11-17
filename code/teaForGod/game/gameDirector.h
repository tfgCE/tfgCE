#pragma once

#include "gameDirectorDefinition.h"

#include "..\story\storyExecution.h"

#include "..\..\core\functionParamsStruct.h"

#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	interface_class IModulesOwner;
	class Door;
	class Room;
	class Sample;
};

namespace TeaForGodEmperor
{
	class ModuleAI;
	class PilgrimageInstance;
	struct GameDirectorDefinition;

	namespace GameScript
	{
		namespace Elements
		{
			class GameDirector;
		};
	};

	class GameDirector
	{
	public:
		static GameDirector* get() { return s_director; }
		static GameDirector* get_active() { return s_director && s_director->is_active()? s_director : nullptr; }

		GameDirector();
		~GameDirector();

		void reset();

		void activate();
		void deactivate();
		bool is_active() const { return isActive; }

		void update(float _deltaTime);

		Story::Execution& access_story_execution() { return storyExecution; }

		void do_clean_start_for(PilgrimageInstance const* _instance);

	public:
		void mark_auto_storing_persistence_required(bool _autoStoringPersistenceRequired = true) { playerInfo.autoStoringPersistenceRequired = _autoStoringPersistenceRequired; }

	public:
		static bool is_violence_disallowed(); // narrative, safe space etc

		static void narrative_mode_trust_lost(); // set only if in forced narrative

		bool is_game_menu_blocked() const { return state.gameMenuBlocked; }
		
		bool is_storing_intermediate_game_states_blocked() const { return state.storingIntermediateGameStatesBlocked; }

		bool is_narrative_active() const { return state.narrativeActive; }
		bool is_narrative_requested() const { return state.narrativeRequested; }
		bool is_narrative_trust_lost() const { return state.narrativeTrustLost; }
		bool is_active_scene() const { return state.activeScene; }
		bool is_in_safe_space() const { return state.inSafeSpace; }
		bool should_block_hostile_spawns() const { return state.blockHostileSpawns || state.blockHostileSpawnsDueToNarrative || state.blockHostileSpawnsDueToBlockedAwayCells || state.narrativeActive; }
		bool are_hostile_spawns_forced() const { return state.forceHostileSpawns; }
		bool should_hide_immobile_hostiles() const { return state.hideImmobileHostiles; }
		bool are_doors_blocked() const { return state.blockDoors; }
		bool are_elevators_blocked() const { return state.blockElevators; }

		bool is_accidental_music_blocked() const { return state.accidentalMusicBlocked; }

		bool is_map_blocked() const { return state.mapBlocked; }
		bool is_reveal_map_on_upgrade_blocked() const { return state.revealMapOnUpgradeBlocked; }
		bool are_door_directions_blocked() const { return state.doorDirectionsBlocked; }
		bool is_forearm_navigation_blocked() const { return state.forearmNavigationBlocked; }
		bool is_upgrade_canister_info_blocked() const { return state.upgradeCanisterInfoBlocked; }
		bool is_real_movement_input_tip_blocked() const { return state.realMovementInputTipBlocked; }

		bool is_pilgrimage_devices_unobfustication_on_hold() const { return state.pilgrimageDevicesUnobfusticationOnHold; }

		bool is_pilgrim_overlay_blocked() const { return state.pilgrimOverlayBlocked; }
		bool is_pilgrim_overlay_in_hand_stats_blocked() const { return state.pilgrimOverlayInHandStatsBlocked; }
		bool is_follow_guidance_blocked() const { return state.followGuidanceBlocked; }
		bool are_weapons_blocked() const { return state.weaponsBlocked; }
		bool is_health_status_blocked() const { return state.healthStatusBlocked; }
		bool is_ammo_status_blocked() const { return state.ammoStatusBlocked; }
		bool is_guidance_blocked() const { return state.guidanceBlocked; }
		bool is_guidance_status_blocked() const { return state.guidanceStatusBlocked || state.guidanceBlocked; }
		bool is_guidance_simplified() const { return state.guidanceSimplified; }
		float get_health_status() const { return state.healthStatus; }
		float get_ammo_status() const { return state.ammoStatus; }
		bool are_wide_stats_forced() const { return state.forceWideStats; }
		bool are_time_adjustments_blocked() const { return state.timeAdjustmentBlocked; }

		bool is_respawn_and_continue_blocked() const { return state.respawnAndContinueBlocked; }
		bool is_immortal_health_regen_blocked() const { return state.immortalHealthRegenBlocked; }
		bool is_infinite_ammo_blocked() const { return state.infiniteAmmoBlocked; }
		bool are_ammo_storages_unavailable() const { return state.ammoStoragesUnavailable; }
		
		void set_respawn_and_continue_blocked(bool _v);
		void set_immortal_health_regen_blocked(bool _v);
		void set_infinite_ammo_blocked(bool _v);
		void set_ammo_storages_unavailable(bool _v);

		void set_real_movement_input_tip_blocked(bool _v);

	public:
		// works even if game director is inactive
		void game_clear_force_no_violence() { state.game.forceNoViolenceTimeLeft.clear(); }
		void game_force_no_violence_for(float _time) { state.game.forceNoViolenceTimeLeft = _time; }
		bool game_is_no_violence_forced() const { return state.game.forceNoViolenceTimeLeft.is_set(); }

		void game_set_objective_requires_no_voilence(bool _v) { state.game.objectiveRequiresNoViolence = _v; }
		bool game_does_objective_require_no_violence() const { return state.game.objectiveRequiresNoViolence; }

	public:
		// works even if game director is inactive
		void game_set_elavators_follow_player_only(bool _follow) { state.game.elevatorsFollowPlayerOnly = _follow; }
		bool game_do_elavators_follow_player_only() const { return state.game.elevatorsFollowPlayerOnly; }

	public:

	public:
		struct AirProhibitedPlace
		{
			Name id;
			Segment place;
			float radius;
		};
		
		void add_air_prohibited_place(Name const& _id, Segment const& _prohibitedPlace, float _radius, bool _relativeToPlayersTileLoc = false);
		void remove_air_prohibited_place(Name const& _id);
		void remove_all_air_prohibited_places();
		int get_air_prohibited_places(OUT_ ArrayStack<AirProhibitedPlace>* _prohibitedPlaces = nullptr) const; // if not set, will only return the size required

	public:
		struct DoorState
		{
			bool opening = false;
			bool overridingLock = false;
			bool overridingLockHighlight = false;
			float timeToOpen = 0.0f;
			float timeToOpenPeriod = 0.0f;
		};
		DoorState get_door_state(Framework::Door const* _door) const;
		struct DoorClosingParams
		{
			ADD_FUNCTION_PARAM_PLAIN(DoorClosingParams, Framework::Sample*, closedSound, with_closed_sound);

			DoorClosingParams()
			: closedSound(nullptr)
			{}
		};
		void set_door_closing(Framework::Door const* _door, float _timeToClose = 0.0f, DoorClosingParams const & _params = DoorClosingParams());
		struct DoorOpeningParams
		{
			ADD_FUNCTION_PARAM(DoorOpeningParams, float, timeToOpenPeriod, with_period);
			ADD_FUNCTION_PARAM_PLAIN(DoorOpeningParams, Framework::Sample*, openingSound, with_opening_sound);
			ADD_FUNCTION_PARAM_PLAIN(DoorOpeningParams, Framework::Sample*, openSound, with_open_sound);

			DoorOpeningParams()
			: openingSound(nullptr)
			, openSound(nullptr)
			{}
		};
		void set_door_opening(Framework::Door const* _door, float _timeToOpen = 0.0f, DoorOpeningParams const& _params = DoorOpeningParams());
		void set_door_override_lock(Framework::Door const* _door, bool _overrideLock);
		void extend_door_opening(Framework::Door const* _door, int _extendByPeriodNum = 1, Optional<float> const & _maxTimeToExtend = NP);
		bool are_lock_indicators_disabled() const { return state.lockIndicatorsDisabled; }
		bool is_keep_in_world_temporarily_disabled() const { return state.keepInWorldTemporarilyDisabled; }

	private: friend class ModuleAI;
		void register_hostile_ai(Framework::IModulesOwner* _imo);
		void unregister_hostile_ai(Framework::IModulesOwner* _imo);
		//
		void mark_hostile_ai_non_hostile(Framework::IModulesOwner* _imo, bool _nonHostile);
		
		void register_immobile_hostile_ai(Framework::IModulesOwner* _imo);
		void unregister_immobile_hostile_ai(Framework::IModulesOwner* _imo);
		//
		void mark_immobile_hostile_ai_hidden(Framework::IModulesOwner* _imo, bool _hidden);
		
	private: friend class GameScript::Elements::GameDirector;
	public:
		void take_care_of_orphaned(Framework::IModulesOwner* _imo);

	public:
		void log(LogInfoContext& _log) const;

	private:
		static GameDirector* s_director;
		
	private:
		Story::Execution storyExecution;

		Random::Generator rg;

		PilgrimageInstance const* forPilgrimageInstance = nullptr;
		GameDirectorDefinition usingDefinition;

		bool isActive = false;

		struct Doors
		{
			mutable Concurrency::MRSWLock lock;

			struct Door
			{
				SafePtr<Framework::Door> door;
				Optional<float> timeToOpen;
				Optional<float> timeToClose;
				Optional<float> overridingLock; // if set, will randomly reset to show override state
				Optional<float> overridingLockHighlight; // if set, will randomly reset to show override state
				//
				float timeToOpenPeriod = 0.5f;
				Framework::UsedLibraryStored<Framework::Sample> openingSound;
				Framework::UsedLibraryStored<Framework::Sample> openSound;
				Framework::UsedLibraryStored<Framework::Sample> closedSound;

			};
			Array<Door> doors;
		} doors;

		struct State
		{
			Concurrency::SpinLock lock;

			bool gameMenuBlocked = false;
			
			bool storingIntermediateGameStatesBlocked = false;
			
			bool narrativeActive = false; // currently in narrative mode
			bool narrativeRequested = false; // ordered to force narrative
			bool narrativeTrustLost = false; // if narrative is forced but we kill someone, trust is lost and everyone may attack
			bool activeScene = false; // something is happening and we want other scenes to not happen at the same time, there might be enemies invovled, etc. this is just to check by scripts what we're doing
			
			bool autoHostilesAllowed = true; // allowed by game director, set by pilgrimage

			bool inSafeSpace = false; // when in safe space, no more enemies get spawned and existing ones run away
			bool blockHostileSpawns = false; // only dynamic, turrets etc may still spawn
			bool blockHostileSpawnsDueToNarrative = false; // when narrative is active
			bool blockHostileSpawnsDueToBlockedAwayCells = false; // as above, this happens when we're in open world and we're at the bring of being blocked away
			bool forceHostileSpawns = false; // only dynamic
			bool hideImmobileHostiles = false; // if an immobile hostile is not visible, it is hidden if this is requested
			bool blockDoors = false;
			bool blockElevators = false; // implement support!

			// anything game related (respawn, logic etc)
			struct Game
			{
				Optional<float> forceNoViolenceTimeLeft;
				bool objectiveRequiresNoViolence = false;
				bool elevatorsFollowPlayerOnly = false;
			} game;

			// all custom stuff
			bool accidentalMusicBlocked = false;
			bool mapBlocked = false;
			bool revealMapOnUpgradeBlocked = false;
			bool doorDirectionsBlocked = false;
			bool forearmNavigationBlocked = false;
			bool upgradeCanisterInfoBlocked = false;
			bool realMovementInputTipBlocked = false;
			bool pilgrimageDevicesUnobfusticationOnHold = false;
			bool pilgrimOverlayBlocked = false;
			bool pilgrimOverlayInHandStatsBlocked = false;
			bool followGuidanceBlocked = false;
			bool weaponsBlocked = false;
			bool healthStatusBlocked = false;
			bool ammoStatusBlocked = false;
			bool guidanceBlocked = false; // blocked at all
			bool guidanceStatusBlocked = false; // just info popping on forearm (assumed blocked if guidance blocked)
			bool guidanceSimplified = false; // replace directions with objective, show only directions
			float statusBlendTime = 2.0f;
			float healthStatus = 0.0f;
			float ammoStatus = 0.0f;
			bool timeAdjustmentBlocked = false;
			bool forceWideStats = false;
			bool respawnAndContinueBlocked = false;
			bool immortalHealthRegenBlocked = false;
			bool infiniteAmmoBlocked = false;
			bool ammoStoragesUnavailable = false;
			bool lockIndicatorsDisabled = false; // just visual
			bool keepInWorldTemporarilyDisabled = false;

			float temporaryFoveationLevelOffset = 0.0f;
		} state;

		struct AutoHostiles
		{
			bool active = false; // in general, comes from the requestedActive and is used to monitor changes
			bool requestedActive = false; // based on pilgrimage

			bool hostilesActive = false;
			bool fullHostiles = false;
			float timeLeft = 0.0f;
		} autoHostiles;

		struct PlayerInfo
		{
			bool inSafeSpace = false;
			CACHED_ Framework::Room* checkedForRoom = nullptr;
			Optional<VectorInt2> cellAt;

			bool playerInGameStateSafePlace = false;
			float playerInGameStateSafePlaceTime = 0.0f;
			float playerOutGameStateSafePlaceTime = 0.0f;

			Optional<float> autoCreateLastMomentGameStateTimeLeft;
			Optional<VectorInt2> autoCreateLastMomentGameStateWhenEnteredCellAt;

			bool autoStoringPersistenceRequired = false;
			Optional<float> lastAutoStoringPersistenceTimeLeft; // cool down to store less often

			void update(GameDirector* gd, float _deltaTime);
		} playerInfo;
		friend struct PlayerInfo;

		struct AirProhibitedPlaces
		{
			mutable Concurrency::MRSWLock placesLock = Concurrency::MRSWLock(TXT("TeaForGodEmperor.GameDirector.airProhibitedPlaces.placesLock"));

			Array<AirProhibitedPlace> places;
		} airProhibitedPlaces;
		friend struct AirProhibitedPlaces;

		mutable Concurrency::SpinLock registeredHostileAIsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameDirector.registeredHostileAIsLock"));
		Array<SafePtr<Framework::IModulesOwner>> registeredHostileAIs;
		mutable Concurrency::SpinLock nonHostileAIsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameDirector.nonHostileAIsLock"));
		Array<SafePtr<Framework::IModulesOwner>> nonHostileAIs; // may include suspended too

		mutable Concurrency::SpinLock registeredImmobileHostileAIsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameDirector.registeredImmobileHostileAIsLock"));
		Array<SafePtr<Framework::IModulesOwner>> registeredImmobileHostileAIs;
		mutable Concurrency::SpinLock hiddenImmobileHostileAIsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameDirector.hiddenImmobileHostileAIsLock"));
		Array<SafePtr<Framework::IModulesOwner>> hiddenImmobileHostileAIs; // may include suspended too

		// orphans are left by spawn managers and when they venture off, they cease to exist (unless they're friendly to the player)
		// they should be a rarity as you have to be far from the tile they were spawned in
		mutable Concurrency::SpinLock orphansLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameDirector.orphansLock"));
		Array<SafePtr<Framework::IModulesOwner>> orphans;
		int orphanIdx = 0;

		GameDirectorDefinition::AutoHostiles prepare_auto_hostiles_to_use() const;
	};


};
