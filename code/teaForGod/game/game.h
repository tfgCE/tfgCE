#pragma once

#include "gameStats.h"
#include "player.h"
#include "playerSetup.h"

#include "..\teaForGod.h"
#include "..\loaders\hub\loaderHub.h"
#include "..\pilgrim\pilgrimInfoToRender.h"

#include "..\..\core\concurrency\gate.h"
#include "..\..\core\latent\latentFrameInspection.h"
#include "..\..\framework\game\game.h"
#include "..\..\framework\render\renderCamera.h"
#include "..\..\framework\render\renderScene.h"
#include "..\..\framework\render\renderContext.h"

namespace Framework
{
	class Door;
	class Region;
	class Item;
	class LibraryBasedParameters;
	class Object;
	class Display;
	class TexturePart;
	namespace AI
	{
		class Message;
	};
	namespace Sound
	{
		class Scene;
		class Camera;
	};
	namespace Render
	{
		struct SceneBuildRequest;
	};
};

namespace Loader
{
	interface_class ILoader;
};

namespace TeaForGodEmperor
{
	class EnvironmentPlayer;
	class GameDirector;
	class GameLog;
	class GameSceneLayer;
	class MusicPlayer;
	class SubtitleSystem;
	class TutorialSystem;
	class VoiceoverSystem;
	struct GameAdvanceContext;
	struct ScreenShotContext;

	namespace CustomModules
	{
		class Health;
	};

	namespace OverlayInfo
	{
		class System;
	};

	namespace GameMetaState
	{
		enum Type
		{
			JustStarted,
			SelectPilgrimageSetup,
			//
			SelectPlayerProfile,
			SetupNewPlayerProfile,
			SelectGameSlot,
			SetupNewGameSlot,
			PilgrimageSetup,
			QuickPilgrimageSetup,
			UseTestConfig,
			Pilgrimage,
			TutorialSelection,
			Tutorial,
			Debriefing,
			TutorialDebriefing,
		};
	};

	namespace InGameMenuRequest
	{
		enum Type
		{
			No,
			Yes,
			Fast
		};
	};

	namespace LoadingHubScene
	{
		enum Type
		{
			Undefined,
			InitialLoading
		};
	};

	struct StoreGameStateParams
	{
		ADD_FUNCTION_PARAM_PLAIN(StoreGameStateParams, TagCondition, restartInRoomTagged, restart_in_room_tagged); // overrides checkpointRoom mechanism
		ADD_FUNCTION_PARAM_PLAIN_DEF(StoreGameStateParams, bool, wasInterrupted, interrupted, true);
	};

	class Game
		: public Framework::Game
	{
		FAST_CAST_DECLARE(Game);
		FAST_CAST_BASE(Framework::Game);
		FAST_CAST_END();

		typedef Framework::Game base;
	public:
		static float s_renderingNearZ;
		static float s_renderingFarZ;

	public:
		Game();

		override_ void create_library();
		override_ void setup_library();
		override_ void close_library(Framework::LibraryLoadLevel::Type _upToLibraryLoadLevel, Loader::ILoader* _loader);

		override_ void before_game_starts();
		override_ void after_game_ended();

		override_ bool is_option_set(Name const& _option) const;

		void end_demo() { endDemoRequestedFor.set_if_not_set(0.0f); }

		bool does_allow_screenshots() const { return allowScreenShots; }

	public:
		Framework::Region* get_utility_region() const { return utilityRegion.get(); }
		Framework::Room* get_locker_room() const { return lockerRoom.get(); }

		void draw_vr_controls(bool _draw = true) { debugDrawVRControls = _draw ? 1 : 0; }

		OverlayInfo::System* access_overlay_info_system() const { return overlayInfoSystem; }

		override_ VR::Zone get_vr_zone(Framework::Room* _room) const;

	public:
		static float calculate_health_pt_for_effects(CustomModules::Health const* h);
		static float calculate_sound_dullness_for(Framework::Object const* _usingObjectAsOwner);

	private:
		float calculate_sound_dullness_for_consider_camera(Framework::Object const* _usingObjectAsOwner);

	public:
		void set_scripted_sound_dullness(float _dullness, float _blendTime = 0.2f) { scriptedSoundDullness.target = _dullness; scriptedSoundDullness.blendTime = _blendTime; if (_blendTime == 0.0f) scriptedSoundDullness.current = _dullness; }

	public:
		bool does_debug_control_player() const;
		Player& access_player() { return player; }
		Player& access_player_taken_control() { return playerTakenControl; } // temporarily

		void set_taking_controls_target(float _target, Optional<float> const& _blendTime) { takingControls.target = _target; takingControls.blendTime = _blendTime; }
		float get_taking_controls_target() const { return takingControls.target; }
		void set_taking_controls_at(float _at, float _target = 0.0f) { takingControls.at = _at; takingControls.target = _target; }
		float get_taking_controls_at() const { return takingControls.at; }
		float is_taking_controls() const { return takingControls.at != 0.0f && takingControls.target == 0.0f; }

	public:
		void set_vignette_for_dead_target(float _target, Optional<float> const& _blendTime) { vignetteForDead.target = _target; vignetteForDead.blendTime = _blendTime.get(vignetteForDead.blendTime); }
		float get_vignette_for_dead_target() const { return vignetteForDead.target; }
		void set_vignette_for_dead_at(float _at) { vignetteForDead.at = _at; vignetteForDead.target = _at; }
		float get_vignette_for_dead_at() const { return vignetteForDead.at; }
		void clear_vignette_for_dead_target() { set_vignette_for_dead_at(0.0f); }

	public:
		void set_scripted_tint_at(Colour const& _colour, float _active) { scriptedTint.current = _colour; scriptedTint.active = _active; scriptedTint.target = _colour; scriptedTint.targetActive = _active; scriptedTint.blendTime = 0.0f; }
		void set_scripted_tint_target(Colour const& _target, float _targetActive, float _blendTime) { scriptedTint.target = _target; scriptedTint.targetActive = _targetActive; scriptedTint.blendTime = _blendTime; }
		void clear_scripted_tint() { scriptedTint.current = scriptedTint.target = Colour::none; scriptedTint.active = scriptedTint.targetActive = 0.0f;  scriptedTint.blendTime = 0.0f; }
		float get_scripted_tint_active() const { return scriptedTint.active; }
		Colour const& get_scripted_tint_current() const { return scriptedTint.current; }
		float get_scripted_tint_target_active() const { return scriptedTint.targetActive; }
		Colour const& get_scripted_tint_target() const { return scriptedTint.target; }
		float get_scripted_tint_blend_time() const { return scriptedTint.blendTime; }

	public:
		// from .xml to .player
		bool legacy__does_require_updating_player_profiles() const;
		void legacy__update_player_profiles();

		Array<String> const& get_player_profiles() const { return playerProfiles; }
		void update_player_profiles_list(bool _selectPlayerProfileAndStoreInConfig);
		void choose_player_profile(String const& _profile, bool _addIfDoesntExist = false, bool _loadGameSlots = true);
		void use_quick_game_player_profile(bool _useQuickGameProfile, Optional<String> const& _andChooseProfile = NP);
		bool is_using_quick_game_player_profile() const { return useQuickGameProfile; }
		void create_and_choose_default_player(bool _loadGameSlots = true);

		void set_allow_saving_player_profile(bool _allowSavingPlayerProfile) { allowSavingPlayerProfile = _allowSavingPlayerProfile; }
		bool load_existing_player_profile(bool _loadGameSlots = true); // if doesn't exist, will clear profile name
		bool load_player_profile(bool _loadGameSlots = true);
		void save_player_profile(bool _saveActiveGameSlot = true); // only if exists
		void save_player_profile_to_file(Optional<String> const& _profile = NP, bool _saveAllGameSlots = false, bool _saveActiveGameSlot = true);
		void add_async_save_player_profile(bool _saveActiveGameSlot = true);
		void may_need_to_save_player_profile();
		void no_need_to_save_player_profile();
		void switch_to_no_player_profile(); // only if existswhen we're creating a new profile
		bool load_player_profile_stats_and_preferences_into(PlayerSetup& _ps, Optional<String> const& _profile = NP);
		String get_player_profile_directory() const { return String(TXT("_players")) + IO::get_directory_separator(); }
		tchar const* get_player_profile_filename_suffix() const { return TXT(".player"); }
		tchar const* get_game_slot_filename_suffix() const { return TXT(".gameslot"); }
		String get_player_profile_file(Optional<String> const& _profile = NP) const;
		String get_player_profile_file_without_suffix(Optional<String> const& _profile = NP) const;
		String get_whole_path_to_player_profile_file(Optional<String> const& _profile = NP) const;
		String get_whole_path_to_game_slot_file(Optional<String> const& _profileName, PlayerGameSlot const* _gameSlot = nullptr) const;
		String const& get_player_profile_name() const;
		void rename_player_profile(String const& _previousName, String const& _newName);
		void remove_player_profile();

		void show_empty_hub_scene_and_save_player_profile(Optional<Loader::Hub*> _usingHubLoader = NP); // this is a helper function

		// _savePlayerProfile is actual saving, we save player profile and game slot with it
		void add_async_store_persistence(Optional<bool> const& _savePlayerProfile = NP); // won't create game state, will only store persistence/game slot
		void add_async_store_game_state(Optional<bool> const& _savePlayerProfile = NP, Optional<GameStateLevel::Type> const& _gameStateLevel = NP, Optional<StoreGameStateParams> const& _params = NP); // save/checkpoint
		float get_time_since_pilgrimage_started() const { return pilgrimageStartedTS.get_time_since(); }

	protected:
		override_ void on_advance_and_display_loader(Loader::ILoader* _loader);

	public:
		void start_with_welcome_scene() { startWithWelcomeScene = true; }
		void set_meta_state(GameMetaState::Type _metaState);
		void request_post_game_summary(PostGameSummary::Type _postGameSummary);
		void request_post_game_summary_if_none(PostGameSummary::Type _postGameSummary);
		bool is_post_game_summary_requested() const;

		void store_post_tutorial_meta_state(GameMetaState::Type _metaState) { postTutorialMetaState = _metaState; }
		GameMetaState::Type get_post_tutorial_meta_state() const { return postTutorialMetaState; }

		void store_post_tutorial_selection_meta_state(GameMetaState::Type _metaState) { postTutorialSelectionMetaState = _metaState; }
		GameMetaState::Type get_post_tutorial_selection_meta_state() const { return postTutorialSelectionMetaState; }

		void set_post_run_tutorial(bool _postRunTutorial) { postRunTutorial = _postRunTutorial; }
		bool is_post_run_tutorial() const { return postRunTutorial; }

		void update_game_stats();

	private:
		void on_ready_for_post_game_summary();

	private:
		GameMetaState::Type metaState = GameMetaState::JustStarted;
		GameMetaState::Type postTutorialMetaState = GameMetaState::JustStarted; // to know where to exit from tutorial
		GameMetaState::Type postTutorialSelectionMetaState = GameMetaState::JustStarted; // to know where to exit from tutorial selection
		bool startWithWelcomeScene = false;
		bool tutorialFromProfile = false;
		bool postRunTutorial = false;
		PostGameSummary::Type showPostGameSummary = PostGameSummary::None;
		PostGameSummary::Type debriefingPostGameSummary = PostGameSummary::None; // in case we came back to it

	public:
		int consume_run_time_seconds() { int runTimeSeconds = TypeConversions::Normal::f_i_cells(consumableRunTime); consumableRunTime -= (float)runTimeSeconds; return runTimeSeconds; }

	public: // modes
		override_ void start_mode(Framework::GameModeSetup* _modeSetup);
		override_ void end_mode(bool _abort = false);

	public: // in-game menu
		void request_in_game_menu(bool _request = true, bool _requestHold = true) { if (_request) inGameMenuForced = true;  inGameMenuRequested = _request ? InGameMenuRequest::Fast : (_requestHold ? InGameMenuRequest::Yes : InGameMenuRequest::No); }
		bool is_in_game_menu_active() const { return inGameMenuActive; }

	public: // hub loader
		void init_main_hub_loader();
		void init_other_hub_loaders();
		void update_hub_scene_meshes();
		void reinit_profile_hub_loader(); // create new one
		void close_hub_loaders();
		Loader::Hub& access_main_hub_loader() { an_assert(mainHubLoader); return *mainHubLoader; }
		Loader::Hub* get_main_hub_loader() { an_assert(mainHubLoader); return mainHubLoader; }
		Loader::Hub* get_recent_hub_loader() { return recentHubLoader ? recentHubLoader : mainHubLoader; }
		Loader::Hub* get_hub_loader_for_tutorial() { return mainHubLoader; /* tutorialFromProfile ? profileHubLoader : mainHubLoader */; }
		Loader::Hub* get_hub_loader_for_pilgrimage() { return mainHubLoader; /* useQuickGameProfile ? mainHubLoader : profileHubLoader */; }

		// first will wait for async world jobs to end, second will use special function unless none provided, then will do as first
		void show_hub_scene_waiting_for_world_manager(Loader::HubScene* _scene, Optional<Loader::Hub*> _usingHubLoader = NP, std::function<void()> _asyncTaskToFinish = nullptr);
		void show_hub_scene(Loader::HubScene* _scene, Optional<Loader::Hub*> _usingHubLoader = NP, std::function<bool()> _conditionToFinish = nullptr, bool _allowWorldManagerAdvancement = true); // _conditionToFinish returns true if should finish

		Loader::HubScene* create_loading_hub_scene(String const& _message, Optional<float> const& _delay = NP, Optional<Name> const& _idToReuse = NP, Optional<LoadingHubScene::Type> const& _loadingHubScene = NP, Optional<bool> const& _allowLoadingTips = NP);

	public:
		implement_ bool kill_by_gravity(Framework::IModulesOwner* _imo);

	protected:
		override_ bool should_skip_delayed_objects_with_negative_priority() const;

	protected:
		override_ void load_game_settings();

	protected:
		override_ void post_start();
		override_ void pre_close();

	protected:
		override_ void pre_update_loader();
		override_ void post_render_loader();

	private:
		void ready_to_use_hub_loader(Loader::Hub* _loader = nullptr); // if recent is not this one, fade out (if none provided, assumes mainHubLoader
		bool make_sure_hub_loader_is_valid(Loader::Hub*& _loader);

	protected:
		~Game();

		override_ void initialise_system();
		override_ void close_system();
		override_ void create_render_buffers();
		override_ void prepare_render_buffer_resolutions(REF_ VectorInt2 & _mainResolution, REF_ VectorInt2 & _secondaryViewResolution);

		override_ void pre_advance();
		override_ void advance_controls();

		override_ void pre_game_loop();

		override_ void render();
		override_ void sound();
		override_ void game_loop();

	public:
		override_ void render_vr_output_to_output();
		override_ void render_main_render_output_to_output();

		override_ void change_show_debug_panel(Framework::DebugPanel* _change, bool _vr);

	public:
		void force_vignette_for_movement(float _vignette) { forcedVignetteForMovementAmount = _vignette; }
		void clear_forced_vignette_for_movement() { forcedVignetteForMovementAmount.clear(); }

	public:
		override_ bool is_ok_to_use_for_region_generation(Framework::LibraryStored const* _libraryStored) const;

	public:
		void prepare_world_to_sound(float _deltaTime, Framework::Object const* _usingObjectAsOwner);
		void prepare_world_to_sound(float _deltaTime, Framework::Sound::Camera const& _usingCamera, Framework::Object const* _usingObjectAsOwner = nullptr);
		void no_world_to_sound();
		Framework::Sound::Scene* access_sound_scene() { return soundScene; }

		void end_all_sounds();

		MusicPlayer* access_music_player() { return musicPlayer; }
		MusicPlayer* access_non_game_music_player() { return nonGameMusicPlayer; }

		SubtitleSystem* access_subtitle_system() { return subtitleSystem; }
		VoiceoverSystem* access_voiceover_system() { return voiceoverSystem; }
		EnvironmentPlayer* access_environment_player() { return environmentPlayer; }

		// make some things much easier
		void play_loading_music_via_non_game_music_player(Optional<float> const& _fadeTime = NP);
		void play_global_reference_music_via_non_game_music_player(Name const& _gsRef, Optional<float> const& _fadeTime = NP);
		void stop_non_game_music_player(Optional<float> const& _fadeTime = NP);

	public:
		// preparation
		void prepare_world_to_render(Framework::Object const* _usingObjectAsOwner, Framework::CustomRenderContext* _customRC = nullptr);
		void prepare_world_to_render(Framework::Render::Camera const& _usingCamera, Framework::CustomRenderContext* _customRC = nullptr);
		void prepare_world_to_render(Framework::Render::Camera const& _usingCamera, Framework::CustomRenderContext* _customRC, bool _allowVROffset);
		void prepare_world_to_render_as_recently(Framework::CustomRenderContext* _customRC = nullptr, Optional<bool> _allowVROffset = NP);
		void no_world_to_render();
		//bool was_recently_rendered() const { return recentlyRendered.wasRecentlyRendered; }

		// rendering of world or empty background (to vr's rts or main render buffer)
		void render_prepared_world_on_render(Framework::CustomRenderContext* _customRC = nullptr);
		void render_empty_background_on_render(Colour const& _colour, Framework::CustomRenderContext* _customRC = nullptr, bool _renderToMain = false);

		// render process - from vr's rts to themselves or from main render buffer to main render target
		void render_post_process(Framework::CustomRenderContext* _customRC = nullptr);

		void render_bullshot_logos_on_main_render_output(Framework::CustomRenderContext* _customRC = nullptr);

		// render on vr's rts or on main render target(!)
		void render_display_2d(Framework::Display* _display, Vector2 const& _relPos, Framework::CustomRenderContext* _customRC = nullptr, bool _renderToMain = false, System::BlendOp::Type _srcBlend = System::BlendOp::One, System::BlendOp::Type _destBlend = System::BlendOp::OneMinusSrcAlpha); // relPos 0 to 1 - 0 on left/bottom edge, 0.5 centre, 1 on right/top edge

		// this is on main output / display
		void temp_render_ui_on_output(bool _justInfo = false);

		// frame around and helping lines
		void render_bullshot_frame_on_main_render_output();

		// done before rendering as maybe it is handled through a character
		bool apply_prerender_fading() const;

	public:
		override_ void advance_world(Optional<float> _withDeltaTimeOverride = NP);

	protected:
		override_ void advance_world__post_block();

	private:
		void do_extra_advances(float _deltaTime); // advnace other systems or do some work while they should be paused

	public: // utils
		// workshop (level)
		void remove_actors_from_pilgrimage();

	public: // directing the game
		GameDirector& access_game_director() { an_assert(gameDirector); return *gameDirector; }

	public: // global camera rumble
		Transform get_current_camera_rumble_placement() const { return cameraRumble.currentOffset; }
		void reset_camera_rumble();

		struct CameraRumbleParams
		{
			ADD_FUNCTION_PARAM(CameraRumbleParams, Range3, maxLocationOffset, with_max_location_offset);
			ADD_FUNCTION_PARAM(CameraRumbleParams, Range3, maxOrientationOffset, with_max_orientation_offset);
			ADD_FUNCTION_PARAM(CameraRumbleParams, Range, interval, with_interval);
			ADD_FUNCTION_PARAM(CameraRumbleParams, float, blendTime, with_blend_time);
			ADD_FUNCTION_PARAM(CameraRumbleParams, float, blendInTime, with_blend_in_time);
			ADD_FUNCTION_PARAM(CameraRumbleParams, float, blendOutTime, with_blend_out_time);
		};
		void set_camera_rumble(CameraRumbleParams const& _params);
	private:
		struct CameraRumble
		{
			Random::Generator rg;

			Transform currentOffset = Transform::identity;
			Transform targetOffset = Transform::identity;
			Range3 currentMaxLocationOffset = Range3::zero;
			Range3 currentMaxOrientationOffset = Range3::zero;
			Range3 targetMaxLocationOffset = Range3::zero;
			Range3 targetMaxOrientationOffset = Range3::zero;
			Range interval = Range(0.02f, 0.8f);
			float blendTime = 0.1f;

			Optional<float> blendToTargetMaxTimeLeft;
			Optional<float> autoBlendOutTime; // if set, blendToTargetMaxTimeLeft will use it and target offsets will be set to 0
			float intervalTimeLeft = 0.0f;

			void reset();
			void set(CameraRumbleParams const& _params);
			void advance(float _deltaTime);

		} cameraRumble;

		void advance_camera_rumble(float _deltaTime);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		bool is_autopilot_on() const { return autopilot; }
	private:
		bool autopilot = false;
#endif

	protected:
		override_ bool should_input_be_grabbed() const;

	public:
		void start_game_sounds_pause() { if (gameSoundsPauseCount == 0) on_game_sounds_pause(); ++gameSoundsPauseCount; }
		void end_game_sounds_pause() { --gameSoundsPauseCount;  if (gameSoundsPauseCount == 0) on_game_sounds_unpause(); }
	protected:
		virtual void on_game_sounds_pause();
		virtual void on_game_sounds_unpause();
	private:
		int gameSoundsPauseCount = 0;

	public:
		void manage_game_sounds_pause(bool _shouldBePaused);
	private:
		bool gameSoundsShouldBePaused = false;

	public:
		void start_non_game_sounds_pause() { if (nonGameSoundsPauseCount == 0) on_game_sounds_pause(); ++nonGameSoundsPauseCount; }
		void end_non_game_sounds_pause() { --nonGameSoundsPauseCount;  if (nonGameSoundsPauseCount == 0) on_game_sounds_unpause(); }
	protected:
		virtual void on_non_game_sounds_pause();
		virtual void on_non_game_sounds_unpause();
	private:
		int nonGameSoundsPauseCount = 0;

	public:
		void manage_non_game_sounds_pause(bool _shouldBePaused);
	private:
		bool nonGameSoundsShouldBePaused = false;

	protected:
		override_ void on_short_pause();
		override_ void on_short_unpause();

	public:
		override_ void on_newly_created_object(Framework::IModulesOwner* _object);
		override_ void on_newly_created_door_in_room(Framework::DoorInRoom* _dir);
		override_ void on_changed_door_type(Framework::Door* _door);
		override_ void on_generated_room(Framework::Room* _room);

	public:
		bool is_forced_camera_in_use() const { return useForcedCamera; }
		void clear_forced_camera() { useForcedCamera = false; }
		void set_forced_camera(Transform const& _p, Framework::Room* _r, Optional<float> const& _fov, Framework::IModulesOwner* _attachedTo);
		Transform const& get_forced_camera_placement() const { return forcedCameraPlacement; }
		Framework::Room* get_forced_camera_in_room() const { return forcedCameraInRoom.get(); }
		float get_forced_camera_fov() const { return forcedCameraFOV; }

	private:
		bool useForcedCamera = false;
		Transform forcedCameraPlacement = Transform::identity;
		float forcedCameraFOV = 90.0f;
		SafePtr<Framework::Room> forcedCameraInRoom;
		SafePtr<Framework::IModulesOwner> forcedCameraAttachedTo;

	public:
		bool is_forced_sound_camera_in_use() const { return useForcedSoundCamera; }
		void clear_forced_sound_camera() { useForcedSoundCamera = false; }
		void set_forced_sound_camera(Transform const& _p, Framework::Room* _r);
		Transform const& get_forced_sound_camera_placement() const { return forcedSoundCameraPlacement; }
		Framework::Room* get_forced_sound_camera_in_room() const { return forcedSoundCameraInRoom.get(); }

	public:
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		bool should_use_external_controls() const { return useExternalControls; }
		bool does_use_external_view() const { return useExternalRenderScene || onlyExternalRenderScene; }
		bool does_use_external_view_only() const { return onlyExternalRenderScene; }
		Optional<Transform> const& get_external_camera_vr_anchor_offset() const { return externalCameraVRAnchorOffset; }
#endif

	protected:
		override_ void use_sliding_locomotion_internal(bool _use);
		override_ bool reset_immobile_origin_in_vr_space_for_sliding_locomotion(); // if used

	private:
		bool useForcedSoundCamera = false;
		Transform forcedSoundCameraPlacement = Transform::identity;
		SafePtr<Framework::Room> forcedSoundCameraInRoom;

	protected:
		override_ float get_adjusted_time_multiplier() const;

	private:
		TutorialSystem* tutorialSystem = nullptr;
		OverlayInfo::System* overlayInfoSystem = nullptr;
		GameDirector* gameDirector = nullptr;
		EnvironmentPlayer* environmentPlayer = nullptr;
		MusicPlayer* musicPlayer = nullptr;
		MusicPlayer* nonGameMusicPlayer = nullptr; // menus/hub/metastates
		SubtitleSystem* subtitleSystem = nullptr;
		VoiceoverSystem* voiceoverSystem = nullptr;

		SafePtr<Framework::Region> utilityRegion; // when emptying utility region from rooms that are no longer used, remember to keep locker room
		SafePtr<Framework::Room> lockerRoom; // room where we may create objects, leave them suspended there (exists inside utility region)

		GameLog* gameLog = nullptr;

		InGameMenuRequest::Type inGameMenuRequested = InGameMenuRequest::No;
		bool inGameMenuForced = false;
		float inGameMenuRequestedFor = 0.0f;
		bool inGameMenuActive = false;

		Loader::Hub* mainHubLoader = nullptr;
		Loader::Hub* profileHubLoader = nullptr;

		Loader::Hub* recentHubLoader = nullptr; // just for reference

		bool allowSavingPlayerProfile = true;
		bool addedAsyncSavePlayerProfile = false;
		bool savingPlayerProfile = false;
		bool useQuickGameProfile = false; // for quick game, override
		Array<String> playerProfiles;
		PlayerSetup playerSetup; // current profile, we access it as the current one (through PlayerSetup's static methods)
		bool playerProfileRequiresSaving = false;
		bool playerProfileLoadedWithGameSlots = false;
		bool playerProfileExists = false;
		System::TimeStamp playerProfileSaveTimeStamp;

		Player player;
		Player playerTakenControl; // temporarily, if we're still in our body but we control some device
		struct TakingControls
		{
			// if >0 entering; <0 leaving
			float at = 0.0f;
			float target = 0.0f;
			Optional<float> blendTime; // this is for 0 to 1 not the whole thing but also tells in what time will we get the controls
			bool overlaySystemSupressed = false;
		} takingControls;
		struct VignetteForDead
		{
			// 0 - alive, 1 - fully dead
			float at = 0.0f;
			float target = 0.0f;
			float blendTime = 2.0f;
		} vignetteForDead;
		Optional<float> playerDeadFor;
		float keepInWorldPenetrationPt = 0.0f;
		float consumableRunTime = 0.0f; // that can be consumed
		bool blockRunTime = false; // allow only if in actual run, not loading etc
		Optional<float> endDemoRequestedFor; // #demo
		Optional<float> endDemoKeyPressedFor; // #demo

		struct ScriptedSoundDullness
		{
			float current = 0.0f;
			float blendTime = 0.1f;
			float target = 0.0f;
		} scriptedSoundDullness;

		struct ScriptedTint
		{
			Colour current = Colour::none;
			float active = 0.0f;
			Colour target = Colour::none;
			float targetActive = 0.0f;
			float blendTime = 0.0f;
		} scriptedTint;

		Optional<float> forcedVignetteForMovementAmount;
		float vignetteForMovementAmount = 0.0f; // calculated every frame
		float vignetteForMovementActive = 0.0f; // calculated every frame

		bool preparedRenderScenes = false;
		// if we're rendering using vr, we don't use actual main, it is safer to store these value and use them when deciding what to render
		VectorInt2 secondaryViewMainResolution;
		VectorInt2 secondaryViewViewResolution;
		VectorInt2 secondaryViewPIPResolution;
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		bool useExternalControls = false; // depends on what is rendered/has greater focus
		bool overrideExternalRenderScene = false; // on ctrl+F2
		bool useExternalRenderScene = false;
		bool onlyExternalRenderScene = false;
		int externalRenderSceneIdx = NONE;
		Optional<Transform> externalCameraVRAnchorOffset;
		struct ExternalRenderPlayArea
		{
			bool render = false;
			float active = 0.0f;
			ExternalRenderPlayArea() {}
			ExternalRenderPlayArea(bool _on) : render(_on), active(_on ? 1.0f : 0.0f) {}
		};
		ExternalRenderPlayArea externalRenderPlayArea;
		ExternalRenderPlayArea externalRenderPlayAreaWalls = ExternalRenderPlayArea(true);
		ExternalRenderPlayArea externalRenderPlayAreaFrontWalls = ExternalRenderPlayArea(true); // walls in front
		float externalRenderPlayerHeight = 1.0f;
		Framework::IModulesOwner* externalRenderPlayerHeightDoneFor = nullptr;
#endif
		Array<RefCountObjectPtr<Framework::Render::Scene> > renderScenes;

		bool preparedSoundScene = false;
		Framework::Sound::Scene* soundScene;
		//
		// ...

		::Framework::GameInput ingameMenuInput;

#ifdef FPS_INFO
		float get_ideal_expected_frame_per_second() const;
#endif

	protected:
		override_ tchar const* get_additional_hitch_info() const;

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		bool is_player_controls_allowed() const { return debugControlsMode == 0; }

	private:
		Framework::Render::ContextDebug rcDebug;
		bool debugRenderingSuspended;
		bool debugSelectDebugSubject;
		bool debugQuickSelectDebugSubject;
		SafePtr<Collision::ICollidableObject> debugSelectDebugSubjectHit;
		bool debugInspectMovement;
		int debugDrawCollision;
		int debugDrawCollisionMode;
		bool debugLogRoomVariables;
		bool debugDrawDoors;
		bool debugRoomInfo;
		bool debugDrawVolumetricLimit;
		bool debugDrawSkeletons;
		bool debugDrawSockets;
		int debugDrawPOIs;
		bool debugDrawMeshNodes;
		int debugTestCollisions;
		SafePtr<Collision::ICollidableObject> debugTestCollisionsHit;
		Collision::Model const * debugTestCollisionsHitModel = nullptr;
		Collision::ICollidableShape const * debugTestCollisionsHitShape = nullptr;
		int debugDrawPresence;
		int debugDrawPresenceBaseInfo;
		int debugDrawCollisionGradient;
		int debugDrawRGI;
		int debugDrawVRAnchor;
		int debugDrawVRPlayArea;
		int debugDrawSpaceBlockers;
		int debugLogNavTasks;
		int debugFindLocationOnNavMesh;
		int debugFindPathOnNavMesh;
		float debugFindPathOnNavMeshBlockedFor = 0.0f;
		int debugVariables;
		int debugAILog;
		int debugAILogic;
		int debugAIHunches;
		int debugAICommonVariables;
		int debugAILatent;
		Latent::FramesInspection debugAILatentFrameInspection;
		int debugACList;
		int debugLocomotion;
		int debugLocomotionPath;
		int debugAIState;
		Framework::Nav::TaskPtr findPathTask;
		Framework::Nav::TaskPtr foundPathTask;
		int debugOutputRenderDetails;
		int debugOutputRenderDetailsLocked;
		FrameInfo debugOutputRenderDetailsLockedFrame;
		int debugOutputRenderScene;
		int debugOutputSoundSystem;
		int debugOutputSoundScene;
		int debugOutputMusicPlayer;
		int debugOutputEnvironmentPlayer;
		int debugOutputVoiceoverSystem;
		int debugOutputSubtitleSystem;
		int debugOutputPilgrimageInfo;
		int debugOutputGameDirector;
		int debugOutputLockerRoom;
		int debugOutputPilgrim;
		int debugOutputGameplayStuff;
#ifdef AN_ALLOW_BULLSHOTS
		int debugOutputBullshotSystem;
#endif
		void select_next_ai_to_debug(bool _next);
		void select_next_ai_in_room_to_debug(bool _next);
		void select_next_in_room_to_debug(bool _next);
		void select_next_ai_manager_to_debug(bool _next);
		void select_next_region_manager_to_debug(bool _next);
		void select_next_dynamic_spawn_manager_to_debug(bool _next);
		void select_next_game_script_executor_to_debug(bool _next);
		int debugShowWorldLog;
		int debugShowMiniMap;
		int debugShowGameJobs;
		LogInfoContext worldLog;
		LogInfoContext debugLog;
		LogInfoContext performanceLog;
		struct DebugHighLevelRegion
		{
			int outputInfo;
			Framework::Region const* highLevelRegion = nullptr;
			Framework::Room const* highLevelRegionInfoFor = nullptr;
			SafePtr<Framework::IModulesOwner> highLevelRegionInfoManager;
			LogInfoContext highLevelRegionInfo;
		};
		DebugHighLevelRegion debugHighLevelRegion;
		DebugHighLevelRegion debugCellLevelRegion;
		void log_high_level_info_region(LogInfoContext& _log, DebugHighLevelRegion& _debugHighLevelRegion, Name const& _highLevelRegionTag, Name const& _highLevelRegionInfoTag);
		struct DebugRoomRegion
		{
			int outputInfo;
			Framework::Room const* forRoom = nullptr;
			LogInfoContext info;
		} debugRoomRegion;
		void log_room_region(LogInfoContext& _log, DebugRoomRegion& _debugRoomRegion);
#endif
		int debugDrawVRControls;
		int debugControlsMode;
		int debugCameraMode;
		float debugCameraSpeed = 1.0f;
		int debugSoundCamera = 0; // 0 at camera, 1 dropped where camera was, 2 at player (to debug sounds)
		bool resetDebugCamera = true;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		int lastInRoomPresenceLinksCount = 0;
		bool lastInRoomPresenceLinksCountRequired = false;
#endif
		Transform debugCameraPlacement = Transform::identity;
		SafePtr<Framework::Room> debugCameraInRoom;
		Optional<Transform> debugSoundCameraPlacement;
		SafePtr<Framework::Room> debugSoundCameraInRoom;

		struct RecentlyRenderedInfo
		{
			bool wasRecentlyRendered = false;
			Framework::Object const * usingObjectAsOwner = nullptr;
			Framework::Render::Camera usingCamera;
			bool allowVROffset;
		} recentlyRendered;

		bool process_debug_controls(bool _forLoaderOnly = false);
		void debug_draw();

		friend class GameSceneLayer;

		void store_recent_capture(bool _fullInGame);

		override_ void async_create_world(Random::Generator _withGenerator);

		void sync_remove_actors_from_pilgrimage();

	private:
		struct OverridePostProcessParam
		{
			Name param;
			Optional<float> value;
			float strength = 0.0f;
			Optional<float> target;
		};
		Array<OverridePostProcessParam> overridePostProcessParams;

	public:
		void make_pilgrim_info_to_render_visible(bool _visible) { pilgrimInfoToRender.isVisible = _visible; }

	protected:
		PilgrimInfoToRender pilgrimInfoToRender;
		::System::TimeStamp pilgrimInfoToRenderUpdateTS;

	protected:
		bool allowScreenShots = true; // allow screen shots in general
		bool cleanScreenShots = true; // screenshots without foreground or additional effects
		bool noOverlayScreenShots = true; // screenshots without overlay

		VectorInt2 preferredScreenShotResolution[2] = { VectorInt2(3840, 2160) /* 4K */, VectorInt2(1920, 1080) /* hd */ };
		Framework::CustomRenderContext screenShotCustomRenderContext[2];

		void make_sure_screen_shot_custom_render_context_is_valid(int _idx);

		friend struct ScreenShotContext;

	protected:
		Optional<Framework::Render::SceneBuildRequest> get_scene_build_request() const;
		void setup_scene_build_context(Framework::Render::SceneBuildContext& _sbc) const;

	private:
		::System::TimeStamp endedRenderTS;
		float lastFrameTimeRenderToRender = 0.0f;

		inline float get_time_to_next_end_render() const;
		inline void mark_render_start();
		inline void mark_render_pushed_do_jobs();
		inline void mark_pre_ended_render();
		inline void mark_render_end();
#ifdef AN_DEVELOPMENT_OR_PROFILER
		inline void mark_start_end_render();
#endif
	private:
		void do_immediate_jobs_while_waiting(float _timeLeft);

	private:
		::System::TimeStamp pilgrimageStartedTS;

		void before_pilgrimage_setup();
		void post_pilgrimage_setup();
		void before_pilgrimage_starts(); // still generating and may break
		void before_pilgrimage_starts_ready(); // about to start!
		void post_pilgrimage_ends();

	public:
		void use_test_scenario_for_pilgimage(Framework::LibraryBasedParameters* _testScenario);
		Framework::LibraryBasedParameters* get_test_scenario_for_pilgimage() const { return testScenario; }

	private:
		Framework::LibraryBasedParameters* testScenario = nullptr;

#ifdef AN_ALLOW_BULLSHOTS
	private:
		Name setupForBullshotId;

		void hard_copy_bullshot(bool _all);
	protected:
		override_ void post_reload_bullshots();
#endif

#ifdef AN_ALLOW_AUTO_TEST
	private:
		Array<Vector2> testMultiplePlayAreaRectSizes;
		bool testingMultiplePlayAreaRectSizes = false;
		
		float autoTestTimeLeft = 0.0f;
		float autoTestInterval = 0.0f;
		int autoTestIdx = 0;
		bool autoTestProcessAllJobs = false;

		bool is_auto_test_active() const { return autoTestInterval > 0.0f; }
	public:
		void mark_nothing_to_auto_test() { autoTestTimeLeft = min(0.0f, autoTestTimeLeft); }
#endif

#ifdef WITH_DRAWING_BOARD
	public:
		::System::RenderTarget* get_drawing_board_rt() const { return drawingBoardRT.get(); }
		void drop_drawing_board_pixels();
		void store_drawing_board_pixels();
		bool read_drawing_board_pixel_data(OUT_ VectorInt2& _resolution, OUT_ Array<byte>& _upsideDownPixels, OUT_ int& _bytesPerPixel, OUT_ bool& _convertSRGB);

	private:
		Concurrency::SpinLock drawingBoardLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.Game.drawingBoardLock"));
		::System::RenderTargetPtr drawingBoardRT;
		Array<byte> drawingBoardUpsideDownPixels;
		VectorInt2 drawingBoardUpsideDownPixelsSize;
#endif

	public:
		override_ bool send_quick_note_in_background(String const& _text, String const& _additionalText = String::empty());
		override_ bool send_full_log_info_in_background(String const& _text, String const& _additionalText = String::empty());
	};
};
