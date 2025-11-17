#pragma once

#include "..\debugSettings.h"

#include "gameCustomisation.h"
#include "gameInput.h"
#include "..\framework.h"
#include "..\library\libraryName.h"
#include "..\library\libraryLoadLevel.h"
#include "..\render\renderCamera.h"
#include "..\world\activationGroup.h"
#include "..\world\world.h"
#include "..\..\core\casts.h"
#include "..\..\core\concurrency\synchronisationLounge.h"
#include "..\..\core\concurrency\asynchronousJobFunc.h"
#include "..\..\core\concurrency\counter.h"
#include "..\..\core\concurrency\gate.h"
#include "..\..\core\concurrency\semaphore.h"
#include "..\..\core\containers\map.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\system\video\renderTargetPtr.h"
#include "..\..\core\system\video\vertexFormat.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\systemInfo.h"
#include "..\..\core\vr\vrZone.h"

#ifdef AN_TWEAKS
#include "..\..\core\io\dir.h"
#else
#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\core\io\dir.h"
#endif
#endif

#ifdef AN_USE_FRAME_INFO
#define FPS_INFO
#endif

// for non windows we don't use it, it runs better
// for windows it works as expected
#ifdef AN_WINDOWS
#define GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
#endif

#include "..\..\core\system\timeStamp.h"

#include <functional>

//

// it's better to suspend destruction if we might be destroying objects during async
//#define WITH_GAME_DESTUTION_PENDING_OBJECT_LOCK

//

class DebugVisualiser;

namespace Concurrency
{
	class AsynchronousJobData;
};

namespace Loader
{
	interface_class ILoader;
};

namespace System
{
	class RenderTarget;
	class Video3D;
};

namespace Framework
{
	interface_class IModulesOwner;
	interface_class IDestroyable;
	class Font;
	class Game;
	class GameMode;
	class GameModeSetup;
	class GameSceneLayer;
	class JobSystem;
	class LibraryStored;
	class POIManager;
	class PostProcessRenderTargetManager;
	class PostProcessChain;
	class Module;
	class Room;
	class TexturePart;
	struct DebugPanel;
	struct DelayedObjectCreation;
	struct LibraryLoadSummary;

	namespace AI
	{
		class Message;
	};

	namespace Nav
	{
		class System;
	};

	namespace GameJob
	{
		enum Type
		{
			NotExecuted,
			InProgress,
			Done,
		};
	};

	namespace Render
	{
		class Scene;
	};

	struct GameJobSystemInfo
	{
		bool doSyncWorldJobsAsAsynchronousGameJobs = true;
		int workerJobsCount = 0;
		int asyncWorkerJobsCount = 0;
		int loadingWorkerJobsCount = 0;
		int preparingWorkerJobsCount = 0;
		int prepareRenderJobsCount = 0;
		int gameJobsCount = 0;
		int backgroundJobsCount = 0;
		int gameBackgroundJobsCount = 0;
		int renderJobsCount = 0;
		int soundJobsCount = 0;
		int systemJobsCount = 0;
	};

	struct GameSettings
	{
		bool itemsRequireNoPostMove = false; // by default but may override_
	};

	struct GameLoadingLibraryContext
	{
		Game* game;
		LibraryLoadLevel::Type libraryLoadLevel = LibraryLoadLevel::Init;
		Array<String> fromDirectories;
		Array<String> loadFiles;
		LibraryLoadSummary* summary = nullptr;
	};

	struct GameClosingLibraryContext
	{
		Game* game;
		LibraryLoadLevel::Type libraryLoadLevel = LibraryLoadLevel::Close;
	};

	/* this is not game config as it could be changed and adjusted to environment per run */
	struct GameStaticInfo
	{
		static GameStaticInfo* get() { return s_gameInfo; }
		static void initialise_static();
		static void close_static();

		VR::Zone get_vr_zone() const { an_assert(vrTileSize != 0.0f); return vrZone; }
		float get_vr_tile_size() const { an_assert(vrTileSize != 0.0f); return vrTileSize; }
		void set_vr_zone_and_tile_size(VR::Zone const & _vrZone, float _vrTileSize) { vrZone = _vrZone; vrTileSize = _vrTileSize; }

		bool is_sensitive_to_vr_anchor() const { return vrAnchorSensitive; }
		void set_sensitive_to_vr_anchor(bool _vrAnchorSensitive) { vrAnchorSensitive = _vrAnchorSensitive; }
	private:
		static GameStaticInfo* s_gameInfo;

		VR::Zone vrZone;
		float vrTileSize = 0.0f;

		bool vrAnchorSensitive = false; // if is sensitive to vr anchor, will try to set vr placement etc
	};

	struct GameAdvanceContext
	{
	public:
		virtual ~GameAdvanceContext() {};

	public:
		void set_delta_time(float _deltaTime) { mainDeltaTime = _deltaTime; otherDeltaTime = _deltaTime; worldDeltaTime = _deltaTime; }
		float get_main_delta_time() const { return mainDeltaTime; }
		void set_main_delta_time(float _mainDeltaTime) { mainDeltaTime = _mainDeltaTime; }
		float get_other_delta_time() const { return otherDeltaTime; }
		void set_other_delta_time(float _otherDeltaTime) { otherDeltaTime = _otherDeltaTime; }
		float get_world_delta_time() const { return worldDeltaTime; }
		void set_world_delta_time(float _worldDeltaTime) { worldDeltaTime = _worldDeltaTime; }

	private:
		float mainDeltaTime = 0.0f; // main menu
		float otherDeltaTime = 0.0f; // ui, round time
		float worldDeltaTime = 0.0f; // world advancement
	};

	struct CustomRenderContext
	{
		RefCountObjectPtr<::System::RenderTarget> sceneRenderTarget;
		RefCountObjectPtr<::System::RenderTarget> finalRenderTarget;
		Array<RefCountObjectPtr<Framework::Render::Scene> > renderScenes;
		Optional<Framework::Render::Camera> renderCamera; // in case we don't have scenes

		Optional<Vector2> relViewCentreOffsetForCamera; // -1 to 1

		virtual ~CustomRenderContext();
	};

	class Game
	{
		/*
		 *		Game system
		 *
		 *		Threads and jobs:
		 *			main thread does have to handle these jobs:
		 *				system (40)
		 *				renderer (30)
		 *			any thread (including maing) can do this (although * prefer not to be on main thread):
		 *				prepare render job (30)
		 *				sound (25)*
		 *				game background (10)*
		 *				background (10)*
		 *				game (5)*
		 *				worker (0)*
		 *			jobs try to populate threads evenly
		 *
		 *		Advance breaks into:
		 *			pre advance (this update controls, may store some imporant stuff that couldn't be done in finalise frame etc)
		 *			actual advance happening on different threads:
		 *				rendering on main thread
		 *				sound on sound thread
		 *				game loop on game thread
		 *					stuff that can be done WHILE rendering and sound is being handled
		 *					stuff after rendering and sound have finished (including finalising frame that gathers all data)
		 *
		 *		with such approach, pre advance advances VR device, control information from that
		 *		goes into game thread - it will rotate character for next frame to render
		 *		and goes into render thread - to keep camera updated immediately (with timewarp it should be even more accurate!)
		 *		
		 *		here's scenario:
		 *			frame n
		 *				VR head yaw is 55'
		 *				character is at 20'
		 *				relative yaw is 0'
		 *				rendered at 20'
		 *			frame n+1
		 *				VR head yaw is 60'
		 *				difference is 5'
		 *				character is at 20' - will get rotated by 5'
		 *				relative yaw is set to 5'
		 *				rendered at 25'
		 *			frame n+2
		 *				VR head yaw is 60'
		 *				difference is 0'
		 *				character is at 25' - will be kept, because there was no rotation
		 *				relative yaw is set to 0' (relative yaw was consumed in last gameplay update
		 *					- note that it doesn't have to be consumed like that - relative yaw might take time to be consumed)
		 *				rendered at 25'
		 *			this of course means that if we want to have character facing always same direction as camera (for example ship) it might be better to fake ship rotation for rendering purposes
		 */

		FAST_CAST_DECLARE(Game);
		FAST_CAST_END();
	public:
		struct WorldJob
		{
			enum TypeFlags
			{
				Sync  = 1,
				Async = 2,

				// flags
				All = Sync | Async
			};
			typedef std::function<void()> Func;
			tchar const * jobName = nullptr;
			Func func = nullptr;
			TypeFlags type = Sync;
			bool topPriority = false;
			WorldJob() {}
			WorldJob(Func _func, TypeFlags _type = Sync, tchar const * _jobName = nullptr, bool _topPriority = false) : jobName(_jobName), func(_func), type(_type), topPriority(_topPriority){}
		};

	public:
		static Game* get() { return s_game; }
		template<typename Class>
		static Class* get_as() {
#ifdef AN_DEVELOPMENT
			return fast_cast<Class>(s_game);
#else
			return plain_cast<Class>(s_game);
#endif
		}

		Game();

		GameCustomisation & access_customisation() { return customisation; }
		GameCustomisation const & get_customisation() const { return customisation; }

		Tags & access_game_tags() { return gameTags; }
		Tags const& get_game_tags() const { return gameTags; }

		String get_system_library_directory() const { return String(TXT("system")); }
		String get_library_directory() const { return String(TXT("library")); }
		String get_settings_directory() const { return String(TXT("settings")); }
		// check start() to get order of loading
		String get_user_config_file() const { return String(TXT("userConfig.xml")); }
		String get_main_config_file() const { return String(TXT("mainConfig.xml")); }
		String get_base_config_file() const { return String(TXT("_baseConfig.xml")); }
		String get_system_tags_file() const { return String(TXT("system.tags")); }
		String get_base_input_config_file() const { return String(TXT("_baseInputConfig.xml")); }
		String get_development_config_file_prefix() const { return String(TXT("_devConfig")); }
		void set_to_create_main_config_only(bool _wantsToCreateMainConfigFile = true) { wantsToCreateMainConfigFile = _wantsToCreateMainConfigFile; }
		bool does_want_to_create_main_config_only() const { return wantsToCreateMainConfigFile; }
		void supress_saving_config_file(bool _supressSavingConfigFile = true) { supressedSavingConfigFile = _supressSavingConfigFile; }

		void add_execution_tags(Tags const & _executionTags) { executionTags.set_tags_from(_executionTags); }
		Tags const & get_execution_tags() const { return executionTags; }

		void start(bool _loadConfigAndExit = false); // initialise threads, system, start
		void advance();
		void close_and_delete(); // close game and delete

		virtual void reinitialise_video(bool _softReinitialise = false, bool _newRenderTargetsNeeded = true); // if _softReinitialise, won't reinitialise window

		void quick_exit() { fadeOutAndQuit = true; }

		void cancel_world_managers_jobs(Loader::ILoader* _loader); // while displaying a loader
		void cancel_world_managers_jobs(bool _andForceAdvanceTillTheEnd = false); // cancels all pending (and forces to end them all)
		void force_advance_world_manager(Loader::ILoader* _loader); // while displaying a loader
		void force_advance_world_manager(bool _justOnce = false); // do all jobs world manager has to perform or do just one

		virtual void save_config_file();
		void save_system_tags_file();

		void ready_for_continuous_advancement();

		void there_is_longer_frame_to_ignore(); // we were not advancing for a while, we should ignore a longer frame

		// kind of second initalisation (after library is loaded etc)
		virtual void before_game_starts();
		virtual void after_game_ended();

		virtual bool is_option_set(Name const & _option) const;

		::System::RenderTarget* get_main_render_buffer() { return mainRenderBuffer.get(); }
		::System::RenderTarget* get_main_render_output_render_buffer() { return mainRenderOutputRenderBuffer.get(); }
		::System::RenderTarget* get_secondary_view_render_buffer() { return secondaryViewRenderBuffer.get(); }
		::System::RenderTarget* get_secondary_view_output_render_buffer() { return secondaryViewOutputRenderBuffer.get(); }

		PostProcessRenderTargetManager* get_post_process_render_target_manager() const { return postProcessRenderTargetManager; }

		int get_frame_no() const { return frameNo; }

		bool is_running_single_threaded() const { return runningSingleThreaded; }
		JobSystem* get_job_system() { return jobSystem; }
		Name const & system_jobs() const { return systemJobs; }
		Name const & render_jobs() const { return renderJobs; }
		Name const & sound_jobs() const { return soundJobs; }
		Name const & prepare_render_jobs() const { return prepareRenderJobs; }
		Name const & game_jobs() const { return gameJobs; }
		Name const & background_jobs() const { return backgroundJobs; }
		Name const & game_background_jobs() const { return gameBackgroundJobs; }
		Name const & worker_jobs() const { return workerJobs; }
		Name const & async_worker_jobs() const { return asyncWorkerJobs; }
		Name const & loading_worker_jobs() const { return loadingWorkerJobs; }
		Name const & preparing_worker_jobs() const { return preparingWorkerJobs; }
		GameJobSystemInfo const& get_job_system_info() const { return jobSystemInfo; }

		// will do the
		void do_in_background_while_showing_loader(Loader::ILoader& _showLoader, std::function<void()> _do);

		float get_delta_time() const { return deltaTime; }

		void setup_game_advance_context(REF_ GameAdvanceContext& _gameAdvanceContext);

		GameSettings const & get_settings() const { return settings; }

		TagCondition const & get_world_channel_tag_condition() const { return worldChannelTagCondition; }
		TagCondition const & get_sound_fade_channel_tag_condition() const { return soundFadeChannelTagCondition; }
		TagCondition const & get_audio_duck_on_voiceover_channel_tag_condition() const { return audioDuckOnVoiceoverChannelTagCondition; }

		bool does_use_vr() const;
		bool can_use_vr() const;
		bool should_render_to_main_for_vr() const;
		virtual VR::Zone get_vr_zone(Room* _room = nullptr) const { return GameStaticInfo::get()->get_vr_zone(); }
		float get_vr_tile_size() const { return GameStaticInfo::get()->get_vr_tile_size(); }
		void set_vr_zone_and_tile_size(VR::Zone const & _vrZone, float _vrTileSize) { GameStaticInfo::get()->set_vr_zone_and_tile_size (_vrZone, _vrTileSize); }
		bool is_sensitive_to_vr_anchor() const { return GameStaticInfo::get()->is_sensitive_to_vr_anchor(); }

		bool does_want_to_exit() const { return wantsToExit; }
		void set_wants_to_exit() { wantsToExit = true; }

		bool does_want_to_cancel_creation() const { return wantsToCancelCreation || does_want_to_exit(); }
		void set_wants_to_cancel_creation(bool _wantsToCancelCreation);
		
		bool is_destroying_world_blocked() const { return blockDestroyingWorld; }
		void block_destroying_world(bool _blockDestroyingWorld) { blockDestroyingWorld = _blockDestroyingWorld; }

		void clear_generation_issues() { generationIssues.clear(); }
		void add_generation_issue(Name const & _issue) { generationIssues.push_back(_issue); }
		Array<Name> const & get_generation_issues() const { return generationIssues; }

#ifdef WITH_IN_GAME_EDITOR
	public:
		bool does_want_to_switch_editor() const;
		bool show_editor_if_requested(); // checks if requested and shows only then, returns true if editor was visible
		void show_editor(); // switches into editor mode and shows stuff, exits only when everythign is done
		virtual void background_post_switch_editor(); // this is called when coming back from the editor in an asynchronous background job
#endif

	public: // modes
		virtual void start_mode(GameModeSetup* _modeSetup);
		virtual void end_mode(bool _abort = false);
		GameMode* get_mode() { return mode.get(); }
		void request_end_mode() { endModeRequested = true; }

	protected:
		RefCountObjectPtr<GameMode> mode;
		bool endModeRequested = false;

	public: // scenes
		void clear_scene_layer_stack();
		void push_scene_layer(GameSceneLayer* _scene);
		GameSceneLayer* get_scene_layer() const { return sceneLayerStack.get(); }

	protected:
		friend class GameSceneLayer;
		RefCountObjectPtr<GameSceneLayer> sceneLayerStack;

	public: // loader utils
		void advance_and_display_loader(Loader::ILoader* _loader); // this is basically entire game loop inside, it advances System::Core, executes JobSystem's main executor, renders, displays, lock, stock and barrel
		void wait_till_loader_ends(Loader::ILoader* _loader, Optional<float> const& _deactivateAfter = NP); // loader has to be activated beforehand
		void show_loader_and_perform_background(Loader::ILoader* _loader, std::function<void()> _perform);

	public:
		virtual void on_newly_created_library_stored_for_editor(LibraryStored*); // this is used for objects that are created for editing or something else that may require some initial setup to make things easier

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	public:
		void clear_external_view_clip_planes();
		void set_external_view_clip_planes(int _planeCount, Plane const* _planes, int _planeExcludeCount = 0, Plane const* _planesExclude = nullptr);

	private:
		::System::CustomVideoClipPlaneSet<64> externalViewClipPlanes; // in camera space!
		ArrayStatic<Vector4, 64> externalViewClipPlanesReadyForRendering;
		::System::CustomVideoClipPlaneSet<256> externalViewClipPlanesExclude; // in camera space!, groups, broken with zero planes
		ArrayStatic<Vector4, 256> externalViewClipPlanesExcludeReadyForRendering;
#endif

	public:
		Framework::World* get_world() const { return world; }
		Framework::SubWorld* get_sub_world() const { return subWorld; }

		bool is_world_in_use() const { return worldInUse; }
		void use_world(bool _use = true);
		void post_use_world(bool _use = true); // will create job that world is about to be used

		bool is_generated(Framework::Room* _room) const;
		bool is_generated(Framework::Region* _region) const;

	public:
		void request_world(Random::Generator _withGenerator);
		void request_destroy_world();

	protected:
		virtual void async_create_world(Random::Generator _withGenerator);
		virtual void sync_destroy_world(bool _allowAsync = false);

	public:
		struct AddGenericObjectParams
		{
			ADD_FUNCTION_PARAM(AddGenericObjectParams, Random::Generator, randomGenerator, use_random_generator);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(AddGenericObjectParams, ObjectType*, objectType, use_object_type, nullptr);
			ADD_FUNCTION_PARAM_PLAIN(AddGenericObjectParams, String, name, named);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(AddGenericObjectParams, Room*, room, to_room, nullptr);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(AddGenericObjectParams, Transform, placement, at, Transform::identity);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(AddGenericObjectParams, Mesh*, mesh, use_mesh, nullptr);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(AddGenericObjectParams, MeshGenerator*, meshGenerator, use_mesh_generator, nullptr);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(AddGenericObjectParams, SimpleVariableStorage*, variables, use_variables, nullptr);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL(AddGenericObjectParams, Tags*, tags, tagged, nullptr);
		};
		void async_add_generic_object(AddGenericObjectParams const & _params); // if no objectType provided, uses global reference "generic scenery"
	protected:
		virtual ObjectType* get_object_type_for(AddGenericObjectParams const& _params) { return nullptr; }

	protected:
		virtual void on_advance_and_display_loader(Loader::ILoader* _loader) {}

	protected:
		virtual void load_game_settings();

	protected:
		virtual void post_start();
		virtual void pre_close();

	protected:
		virtual void pre_update_loader() {}
		virtual void post_render_loader() {}

	public:
		void show_loader(Loader::ILoader* _loader); // has to deactivate on its own
		void show_loader_waiting_for_world_manager(Loader::ILoader* _loader, std::function<void()> _asyncTaskToFinish = nullptr);

	public:
		Nav::System* get_nav_system() { return navSystem; }

	public:
		struct GameIcon;
		GameIcon& access_saving_icon() { return gameIcons.savingIcon; }
		GameIcon& access_input_mouse_keyboard_icon() { return gameIcons.inputMouseKeyboardIcon; }
		GameIcon& access_input_gamepad_icon() { return gameIcons.inputGamepadIcon; }

	public:
		struct GameIcon
		{
			bool is_active() const { return active || keepActiveFor > 0.0f; }
			void activate(float _keepActiveFor = 0.0f, bool _autoDeactivate = false) { active = true; keepActiveFor = _keepActiveFor; autoDeactivate = _autoDeactivate; activeFor = 0.0f; }
			void deactivate(bool _immediate = false) { active = false; if (_immediate) keepActiveFor = 0.0f; }
			void deactivate_if_at_least(float _atLeastActiveFor) { if (active) { autoDeactivate = true; keepActiveFor = max(0.0f, _atLeastActiveFor - activeFor); } }

			Framework::TexturePart* get_icon() const { return icon.get(); }

		private: friend class Game;
			bool active = false;
			bool autoDeactivate = false;
			float keepActiveFor = 0.0f; // if this is positive, it will remain active
			float activeFor = 0.0f; // how long is active
			float alpha = 0.0f;
			float offset = 0.0f;
			float inPlaceOffset = 0.0f; // 0 - in place, 1 incoming, -1 going away
			Framework::UsedLibraryStored<Framework::TexturePart> icon;
			void advance(float _offset, float _deltaTime);
		};

	protected:
		struct GameIcons
		{
			GameIcon savingIcon;
			GameIcon inputMouseKeyboardIcon;
			GameIcon inputGamepadIcon;
			GameIcon screenshotIcon;

			float currentInputActiveTime = 0.0f;
			bool currentInputInfoPending = false; // do not show if too close to each other
			bool mouseKeyboardActive = true;
			Vector2 mouseMovementSinceLongerInactive = Vector2::zero;
			float mouseInactiveTime = 0.0f;
		} gameIcons;

		void check_input_for_icons();

		// this is on vr's rts or main render output
		void game_icon_render_ui_on_main_render_output();

	public: // fading
		float get_fading_state() const { return fading.current; }
		bool is_fading(Optional<Name> const & _reason = NP) const { return fading.target != fading.current && (! _reason.is_set() || fading.reason == _reason.get()); }
		bool is_fading_in(Optional<Name> const & _reason = NP) const { return fading.target < fading.current && (!_reason.is_set() || fading.reason == _reason.get()); }
		bool is_fading_out(Optional<Name> const & _reason = NP) const { return fading.target > fading.current && (!_reason.is_set() || fading.reason == _reason.get()); }
		bool has_faded_in(Optional<Name> const & _reason = NP) const { return fading.current == 0.0f && (!_reason.is_set() || fading.reason == _reason.get()); }
		bool has_faded_out(Optional<Name> const & _reason = NP) const { return fading.current == 1.0f && (!_reason.is_set() || fading.reason == _reason.get()); }
		void fade_in(float _time = 1.0f /* default long fade in */, Optional<Name> const & _reason = NP, Optional<float> const & _delay = NP, Optional<bool> const & _useRawDeltaTime = NP); // will advance in pre_advance
		void fade_out(float _time = 0.2f, Optional<Name> const & _reason = NP, Optional<float> const & _delay = NP, Optional<bool> const & _useRawDeltaTime = NP, Optional<Colour> const & _fadeToColour = NP, Optional<bool> const & _fadeSounds = NP); // will advance in pre_advance
		void fade_in_immediately();
		void fade_out_immediately(Optional<Colour> const & _fadeToColour = NP);

		float get_fade_out() const;
		float get_fade_out_target() const { return fading.target; }

		void apply_fading_to(System::RenderTarget* _rt) const;
		void advance_fading(Optional<float> const & _deltaTime = NP);

	public: // library management
		bool is_loading_library() const { return isLoadingLibrary; }
		virtual bool load_library(String const & _fromDir, LibraryLoadLevel::Type _libraryLoadLevel, Loader::ILoader* _loader, OPTIONAL_ LibraryLoadSummary * _summary = nullptr); // _loader is memory managed
		virtual bool load_library(Array<String> const & _fromDirectories, Array<String> const & _files, LibraryLoadLevel::Type _libraryLoadLevel, Loader::ILoader* _loader, OPTIONAL_ LibraryLoadSummary* _summary = nullptr); // _loader is memory managed
		virtual void close_library(LibraryLoadLevel::Type _upToLibraryLoadLevel, Loader::ILoader* _loader); // _loader is memory managed

	private:
		bool isLoadingLibrary = false; // an information for loaders that are in sequence, deep inside other loaders, etc.

	protected: // library utils
		bool loadLibraryResult;
		bool prepareLibraryResult;

		static void load_library_and_prepare_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		static void close_library_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data);

		virtual void load_library_and_prepare_worker(Concurrency::JobExecutionContext const * _executionContext, GameLoadingLibraryContext* _gameLoadingLibraryContext);
		virtual void close_library_worker(Concurrency::JobExecutionContext const * _executionContext, GameClosingLibraryContext* _gameClosingLibraryContext);

		virtual void create_library();
		virtual void setup_library();

	public:
		void set_auto_removing_unused_temporary_objects(float _interval) { autoRemoveUnusedTemporaryObjectsFromLibraryInterval = _interval; }
		void set_no_auto_removing_unused_temporary_objects() { autoRemoveUnusedTemporaryObjectsFromLibraryInterval.clear(); }
		void request_removing_unused_temporary_objects(Optional<int> const & _batchSize = NP);
		bool is_removing_unused_temporary_objects() const { return removeUnusedTemporaryObjectsFromLibrary; }

	private:
		static void remove_unused_temporary_objects_batch(Concurrency::JobExecutionContext const* _executionContext, void* _data);

	protected:
		Optional<float> autoRemoveUnusedTemporaryObjectsFromLibraryInterval;
		float timeSinceLastRemoveUnusedTemporaryObjectsFromLibrary = 0.0f;
		int removeUnusedTemporaryObjectsFromLibraryBatchSize = 1;
		bool removeUnusedTemporaryObjectsFromLibraryRequested = false;
		bool removeUnusedTemporaryObjectsFromLibrary = false;
		bool removeUnusedTemporaryObjectsFromLibraryAsyncInProgress = false;
		int removedUnusedTemporaryObjectsFromLibrary = 0;

		void advance_remove_unused_temporary_objects_from_library(bool _avoidAddingNew = false);

	public:
		virtual void change_show_debug_panel(DebugPanel* _change, bool _vr);
		void change_show_log_panel(DebugPanel* _change);

	public: // defaults
		virtual void pre_setup_module(IModulesOwner* _owner, Module* _module);

	public: // default assets
		void update_default_assets(); // called after library is loaded

		void set_default_font(LibraryName const & _fontName);

		Font* get_default_font() const { return defaultFont; }

	public:
		static void async_system_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data); // utility function to add and execute system job or do it on main thread
		static void async_system_job(Game* _game, std::function<void()> _do); // utility function to add and execute system job or do it on main thread
		static void async_worker_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data); // utility function to add and execute worker job or do it on main thread
		static void async_background_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data); // utility function to add and execute background job or do it on main thread
		static void async_background_job(Game* _game, std::function<void()> _do); // utility function to add and execute game background job or do it on main thread
		static void async_game_background_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data); // utility function to add and execute game background job or do it on main thread
		static void async_game_background_job(Game* _game, std::function<void()> _do); // utility function to add and execute game background job or do it on main thread
		static void loading_worker_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data); // utility function to add and execute worker job, not on main thread
		static void preparing_worker_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data); // utility function to add and execute worker job, not on main thread

	protected:
		virtual ~Game();

		virtual void initialise_job_system();
		void initialise_system_core();
		virtual void initialise_system();
		virtual void create_render_buffers();
		virtual void prepare_render_buffer_resolutions(REF_ VectorInt2 & _mainResolution, REF_ VectorInt2 & _secondaryViewResolution);

		virtual void terminate_job_system();
		virtual void close_job_system();
		virtual void close_system();

		virtual void pre_advance();
		virtual void advance_controls();
		virtual String get_game_name() const;

		// job methods
		virtual void pre_game_loop(); // job that is happening before render+sound+game loop, everything that can be done while vr and controls are advanced
		//
		virtual void render();
		virtual void sound(); // required as it updates sound system
		virtual void game_loop();

	public: // world
		virtual void advance_world(Optional<float> _withDeltaTimeOverride = NP);

		//
		void initial_world_advance(); // initial advance for world
		void request_initial_world_advance_required();

	protected:
		virtual void advance_world__during_block();
		virtual void advance_world__post_block();
		virtual void skip_advancing_world();

	public:
		void request_ready_and_activate_all_inactive(Framework::ActivationGroup* _activationGroup, World* _world = nullptr);

		// ai communication
		AI::Message* create_ai_message(Name const& _name);

	protected:
		void async_ready_all_inactive(Framework::ActivationGroup* _activationGroup, World* _world);
		void sync_activate_all_inactive(Framework::ActivationGroup* _activationGroup, World* _world);

	protected:
		void advance_later_loading();

	public:
		void render_nothing_to_output();
		virtual void render_vr_output_to_output(); // just vr output with anything else on the way
		virtual void render_main_render_output_to_output(); 

	public: // pausing and unpausing
		bool is_on_short_pause() const { return shortPauseCount; }
		bool was_recently_short_paused() const { return timeSinceShortPause < 0.2f; }
	protected:
		void start_short_pause() { if (shortPauseCount == 0) on_short_pause(); ++shortPauseCount; }
		void end_short_pause() { --shortPauseCount;  if (shortPauseCount == 0) on_short_unpause(); }
		virtual void on_short_pause() {}
		virtual void on_short_unpause() { timeSinceShortPause = 0.0f; }
	private:
		int shortPauseCount = 0;
		float timeSinceShortPause = 0.0f;

	public:
		void setup_to_render_to_main_render_target(::System::Video3D* _v3d);
		void setup_to_render_to_render_target(::System::Video3D* _v3d, ::System::RenderTarget* _rtOverride = nullptr);

	protected: friend interface_class IDestroyable;
		void sync_process_destruction_pending_objects();

	public:
		void suspend_destruction(Name const & _reason);
		void resume_destruction(Name const & _reason);
		void resume_destruction(bool _forceAll);
		bool is_destruction_suspended() const { return destructionSuspended; }

	public: // activation groups
		ActivationGroup* get_current_activation_group();
		ActivationGroup* get_current_activation_group_if_present();
		void push_activation_group(ActivationGroup* _ag);
		ActivationGroupPtr pop_activation_group(OPTIONAL_ ActivationGroup* _ag = nullptr);
		ActivationGroupPtr push_new_activation_group();

	public:
		virtual bool is_ok_to_use_for_region_generation(LibraryStored const* _libraryStored) const { return true; }

	private:
		Array<ActiviationGroupStack> activationGroupStacks; // per thread

	protected:
		bool allowLaterLoading = true;

	public: // world manager
		bool is_world_manager_busy() const { return worldManager.is_busy() || worldManager.has_something_to_do(); } // we will know whether we are waiting for hall or job or not
		float get_world_manager_time_unoccupied() const { return worldManager.get_time_unoccupied(); } // to allow gaps
		int get_number_of_world_jobs() const { return worldManager.get_number_of_world_jobs(); }

		void set_performance_mode(bool _inPerformanceMode);
		void post_set_performance_mode(bool _inPerformanceMode); // will create job to do that

		/**
		 *	Word on jobs:
		 *		add_sync and add_async - will add to the queue
		 *			they can be added from any place, although it might be tricky to share some data between, unless you're using some external class stored somewhere
		 *			example:
		 *				game->add_async_world_job(... /- do some async stuff -/ );
		 *				game->add_sync_world_job(... /- do some async stuff -/ );
		 *				game->add_async_world_job(... /- do some async stuff -/ );
		 *				... // repeat/mix as required
		 *			although instead of this approach, it is better to add async world job with perform sync job inside
		 *			and use adding sync jobs only when we want to do something but we do not care about result
		 *		add_async_top_priority - will add to the front of the queue
		 *			use this in some extremely rare cases, when changing player's stuff, to not make player waiting for other async jobs to come to an end
		 *		perform_sync - immediate
		 *			this should be used from some async worker or when world is not yet created (otherwise we will get stuck!)
		 *			this is especially useful when there is lots of code that doesn't require to be synced and just some bits requiring syncing
		 *			it can also be used from both sync and async world jobs (!)
		 *			this means that instead of adding bunch of sync and async world jobs it is much better to add one async world job and put perform sync inside
		 *			example:
		 *				game->add_async_world_job(...
		 *				{
		 *					// do some async stuff
		 *					game->perform_sync_world_job(... /- do some sync stuff -/ ); 
		 *					// do some more async stuff
		 *					... // repeat/mix as required
		 *				});
		 *		add_immediate_sync - will add to the immediate queue
		 *			they will be all done before next frame starts, before "perform sync" is done
		 *			should be done only by jobs that are super simple or require immediate action
		 */
		void add_sync_world_job(tchar const * _jobName, WorldJob::Func _func); // adds to queue
		void perform_sync_world_job(tchar const * _jobName, WorldJob::Func _func); // doesn't add to queue, performs immediately (actually the gameJobs asynchronous do that if called not on render or game thread)
		void perform_sync_world_job_on_this_thread(tchar const * _jobName, WorldJob::Func _func); // forces to be performed on this thread
		void add_immediate_sync_world_job(tchar const * _jobName, WorldJob::Func _func); // adds to immediate queue - will be done before next frame! don't hog it!
		void add_async_world_job(tchar const * _jobName, WorldJob::Func _func);
		void add_async_world_job_top_priority(tchar const* _jobName, WorldJob::Func _func); // will land at the beginning of the queue

		bool is_performing_sync_job() const;
#ifdef AN_DEVELOPMENT
		bool is_performing_sync_job_on_this_thread() const;
#endif
		bool is_performing_async_job_on_this_thread() const;
		bool is_performing_async_job_and_last_world_job_on_this_thread(int _leftBesidesThis = 0) const;

		bool does_allow_async_jobs() const;

		struct WorldManagerImmediateSyncJobsScopedLock
		{
			WorldManagerImmediateSyncJobsScopedLock(Game* _game);
			~WorldManagerImmediateSyncJobsScopedLock();
		private:
			Game* game;
		};

	public:
		void update_on_system_change();
		void load_system_tags_file();
		void load_config_files();
		void reload_input_config_files(); // base input config

	public:
		bool should_generate_more_details() const;
		void set_generate_more_details(Optional<bool> const & _generateMoreDetails = NP) { generateMoreDetails = _generateMoreDetails; }
	protected:
		Optional<bool> generateMoreDetails; // if not set, will be depending on system
		bool generateMoreDetailsBySystem;

	public:
		bool should_generate_at_lod(int _level) const { return generateAtLOD <= _level; }
		void set_generate_at_lod(int _generateAtLOD) { generateAtLOD = _generateAtLOD; }
		int get_generate_at_lod() const { return generateAtLOD; }
	protected:
		int generateAtLOD = 0; // if LOD is higher, less details we have

#ifdef AN_DEVELOPMENT_OR_PROFILER
	public:
		bool should_generate_dev_meshes(int _level = 1) const { return generateDevMeshes >= _level; }
		void set_generate_dev_meshes(int _generateDevMeshes) { generateDevMeshes = _generateDevMeshes; }
	protected:
		int generateDevMeshes = 1000; // switch it off with testconfig's "generateDevMeshes"

	public:
		int get_preview_lod() const { return previewLOD; }
		void set_preview_lod(int _previewLOD = 0) { previewLOD = _previewLOD; }
	protected:
		int previewLOD = 0;
#endif

	public:
		virtual bool kill_by_gravity(IModulesOwner* _imo) { return false; } // return true if killed

	public:
		void force_instant_object_creation(bool _force = true) { forceInstantObjectCreation = _force; }
		bool is_forced_instant_object_creation() const { return forceInstantObjectCreation; }
		void queue_delayed_object_creation(DelayedObjectCreation* _delayed, bool _allowInstantObjectCreation = false);
		void drop_all_delayed_object_creations();
		bool should_process_object_creation(DelayedObjectCreation const* _doc) const;
		
		bool has_any_delayed_object_creation_pending() const;
		bool has_any_delayed_object_creation_pending(Room* _room);

		void process_delayed_object_creation();
		void process_delayed_object_creation_immediately(Room* _room);

		// this method is called on all objects created with DOC and should be called for any other object
		virtual void on_newly_created_object(IModulesOwner* _object) {}
		virtual void on_newly_created_door_in_room(DoorInRoom* _dir) {}
		virtual void on_changed_door_type(Door* _door) {}
		virtual void on_generated_room(Room* _room) {} // at the end of the generation

	private: friend struct DelayedObjectCreation;
		void done_delayed_object_creation(DelayedObjectCreation* _delayed);
	protected:
		virtual bool should_skip_delayed_objects_with_negative_priority() const { return false; }

	public:
		GameInput& access_all_time_controls_input() { return allTimeControlsInput; }

		virtual bool is_input_grabbed() const;

	public:
		POIManager& access_poi_manager() { an_assert(poiManager); return *poiManager; }

	public:
		static bool is_using_sliding_locomotion();
		static void update_use_sliding_locomotion(); // from main config or anywhere else
	protected:
		bool usingSlidingLocomotion = false; // every time we enter the world we should check this
		static void set_use_sliding_locomotion(bool _use);
		virtual void use_sliding_locomotion_internal(bool _use);

		virtual bool reset_immobile_origin_in_vr_space_for_sliding_locomotion(); // if used

	protected:
		virtual bool should_input_be_grabbed() const;

		void handle_vr_all_time_controls_input();

	protected:
		virtual float get_adjusted_time_multiplier() const { return 1.0f; }

	protected:
		bool requiresRenderGateToAllow = false;
		bool requiresSoundGateToAllow = false;
		Concurrency::Gate renderBlockGate; // means pre rendering is in progress and physical changes to world should not be allowed
		Concurrency::Gate soundBlockGate; // means pre rendering is in progress and physical changes to world should not be allowed

		void render_block_gate_allow_one();
		void sound_block_gate_allow_one();

	protected:
		static Game* s_game;

		GameInput allTimeControlsInput;

		Tags gameTags;

		GameCustomisation customisation;

		Framework::World* world = nullptr;
		bool worldInUse = false; // mark that world is in use or not
		Framework::SubWorld* subWorld = nullptr;

		bool initialWorldAdvanceRequired = true;
		bool advancedWorldLastFrame = false;

		bool wantsToCreateMainConfigFile = false; // will load only base and save as main
		bool supressedSavingConfigFile = false; // to allow temporarily supressing config file

		bool initialised;
		bool wantsToExit = false;
		bool wantsToCancelCreation = false; // to cancel any creation process that is on
		bool blockDestroyingWorld = false; // if true, will wait with destroying world
		bool closingGame = false;
		Tags executionTags;

		POIManager* poiManager = nullptr;

		Array<Name> generationIssues;

		GameSettings settings;
		VR::Zone vrZone;
		float vrTileSize = 0.0f;

		int frameNo = 0;

		// system
		// sound
		TagCondition worldChannelTagCondition; // applies only to world only related sounds
		TagCondition soundFadeChannelTagCondition; // applies only to sound scene
		TagCondition audioDuckOnVoiceoverChannelTagCondition; // channels other than voiceover

		// video
		::System::RenderTargetPtr mainRenderBuffer; // this is where we render the world
		::System::RenderTargetPtr mainRenderOutputRenderBuffer; // this is where we render mainRenderBuffer (using post process or directly), ui, icons, etc. might be null, meaning we're rendering directly to output
		::System::RenderTargetPtr secondaryViewRenderBuffer;
		::System::RenderTargetPtr secondaryViewOutputRenderBuffer;

		PostProcessRenderTargetManager* postProcessRenderTargetManager;
		PostProcessChain* postProcessChain;

		float deltaTime;

		Nav::System* navSystem = nullptr;

		// destroy
#ifdef WITH_GAME_DESTUTION_PENDING_OBJECT_LOCK
		Concurrency::SpinLock destructionPendingObjectLock = Concurrency::SpinLock(TXT("Framework.Game.destructionPendingObjectLock"));
#endif
		IDestroyable* destructionPendingObject; // managed in IDestroyable interface class
		bool destructionPendingWait = false; // wait one frame - we could issue destruction just after presence links were built - we may want to wait a little with that
		Concurrency::SpinLock destructionSuspendedLock;
		Array<Name> destructionSuspendedReasons;
		bool destructionSuspended = false; // allow suspending destruction

		// options
		bool runningSingleThreaded;
		bool mouseGrabbed;

		// jobs
		JobSystem* jobSystem;
		Name systemJobs;
		Name renderJobs;
		Name soundJobs;
		Name prepareRenderJobs;
		Name gameJobs;
		Name backgroundJobs;
		Name gameBackgroundJobs; // during the game
		Name workerJobs;
		Name asyncWorkerJobs;
		Name loadingWorkerJobs;
		Name preparingWorkerJobs;
		GameJobSystemInfo jobSystemInfo;

#ifdef GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
		Concurrency::SynchronisationLounge advanceSynchronisationLounge;
#endif
		volatile GameJob::Type preGameLoopJobStatus;
		//
		volatile GameJob::Type gameLoopJobStatus;
		volatile GameJob::Type renderJobStatus;
		volatile GameJob::Type soundJobStatus;

		// job methods
		static void do_pre_game_loop(Concurrency::JobExecutionContext const* _executionContext, void* _data);
		void do_advance_vr_and_controls(); // but used locally
		//
		static void do_render(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		static void do_sound(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		static void do_game_loop(Concurrency::JobExecutionContext const * _executionContext, void* _data);

		void do_vr_game_sync();

		// defaults
		LibraryName defaultFontName;
		Font* defaultFont;

		// does one job at a time, at proper time during advance it does sync jobs, while async jobs are triggered every frame and done in background
		// it also allows to perform sync jobs that will execute whenever immediate sync jobs are allowed (useful to add stuff to the world during world advance)
		struct WorldManager
		{
			WorldManager(Game* _game);
			~WorldManager();

			void setup_after_system_job_is_active();

			void advance_immediate_jobs();
			void advance_world_jobs(WorldJob::TypeFlags _flags);
			// these two above or this
			void advance_jobs();

			void advance_time_unoccupied(); // also called from above, call it explicitly only when certain it won't get called otherwise

			void cancel_all();

			bool has_something_to_do() const { return is_busy() || ! worldJobs.is_empty(); }
			int get_number_of_world_jobs() const { return worldJobs.get_size(); }
			bool is_busy() const { return isBusy; }
			float get_time_unoccupied() const { return has_something_to_do() || is_busy() || !timeUnoccupied.is_set()? 0.0f : timeUnoccupied.get().get_time_since(); }

#ifdef AN_DEVELOPMENT_OR_PROFILER
			String get_current_sync_world_job_info() const;
			String get_current_async_world_job_info() const;
			void get_async_world_job_infos(Array<tchar const*>& _infos, Optional<int>  const& _max = NP) const;
#else
			String const & get_current_sync_world_job_info() const { return String::empty(); }
			String const & get_current_async_world_job_info() const { return String::empty(); }
			void get_async_world_job_infos(Array<tchar const*>& _infos, Optional<int>  const & _max = NP) const {}
#endif

			void add_sync_world_job(tchar const * _jobName, WorldJob::Func _func);
			void perform_sync_world_job(tchar const * _jobName, WorldJob::Func _func); // doesn't add to queue, performs immediately
			void add_immediate_sync_world_job(tchar const * _jobName, WorldJob::Func _func); // adds to immediate queue - will be done before next frame! don't hog it!
			void add_async_world_job(tchar const * _jobName, WorldJob::Func _func);
			void add_async_world_job_top_priority(tchar const* _jobName, WorldJob::Func _func); // will land at the beginning of the queue

			bool is_performing_sync_job() const;
#ifdef AN_DEVELOPMENT
			bool is_performing_sync_job_on_this_thread() const;
#endif
			bool is_performing_async_job() const;
			bool is_performing_async_job_on_this_thread() const;
			bool is_performing_async_job_and_last_world_job_on_this_thread(int _leftBesidesThis = 0) const;

			void allow_immediate_sync_jobs();
			void disallow_immediate_sync_jobs();

			void set_performance_mode(bool _inPerformanceMode) { inPerformanceMode = _inPerformanceMode; }
			bool is_performance_mode() const { return inPerformanceMode; }

			bool is_nested_sync_job() const;

		private:
			Game* game;

			Optional<::System::TimeStamp> timeUnoccupied;

			bool inPerformanceMode = false; // if true, only one job per frame will be done (this also applies for queued sync jobs)

			bool isBusy = false;
			Concurrency::SpinLock asyncJob = Concurrency::SpinLock(SYSTEM_SPIN_LOCK); // if there is an async job in progress
			Concurrency::Semaphore syncJob; // if there is a sync job in progress
			Concurrency::Semaphore immediateSyncJobsSempahore; // to allow/disallow immediate sync job - allows if counter is 0, disallows if counter is non 0
			Concurrency::Counter immediateSyncJobsLockCounter;
			Concurrency::Semaphore immediateSyncJobsManager;
			int32 syncJobThreadId = NONE;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			mutable Concurrency::SpinLock currentAsyncJobInfoLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
			mutable Concurrency::SpinLock currentSyncJobInfoLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
			String currentAsyncJobInfo;
			String currentSyncJobInfo;
#endif
#ifdef AN_DEVELOPMENT
			Array<bool> syncJobOnThread; // to check whether we are now doing sync job
			int32 disallowedImmediateSyncJobsOnThread = NONE; // which thread has disallowed immediate sync jobs
#endif
			Array<bool> asyncJobOnThread; // to check whether we are now doing async job

			mutable Concurrency::SpinLock worldJobsLock = Concurrency::SpinLock(TXT("Framework.Game.WorldManager.worldJobsLock"));
			Array<WorldJob> worldJobs;
			
			Concurrency::SpinLock immediateJobsLock = Concurrency::SpinLock(TXT("Framework.Game.WorldManager.immediateJobsLock"));
			Array<WorldJob> immediateJobs;

			static void perform_async_job(Concurrency::JobExecutionContext const * _executionContext, void* _data);
			void perform_sync_job(tchar const * _jobName, WorldJob::Func _func);
		};
		WorldManager worldManager;

		bool forceInstantObjectCreation = false;
		Concurrency::SpinLock delayedObjectCreationsLock = Concurrency::SpinLock(TXT("Framework.Game.delayedObjectCreationsLock"));
		Array<RefCountObjectPtr<DelayedObjectCreation>> delayedObjectCreations;
		RefCountObjectPtr<DelayedObjectCreation> delayedObjectCreationInProgress;

		struct Fading
		{
			Name reason;
			// 0 not faded, 1 fully faded out
			float target = 0.0f;
			float current = 0.0f;
			float fadingTime = 0.1f; // fading time as requested by user
			float blendTime = 0.0f; // blend time from current to target
			float pauseFadingFor = 0.0f;

			Colour currentFadeOutColour = Colour::black;
			Colour targetFadeOutColour = Colour::black;

			bool useRawDeltaTime = false; // to allow exit

			bool fadeSoundNow = false; // fade to target

			bool should_be_applied() const { return current != 0.0f; }
		} fading;
		bool fadeOutAndQuit = false; // quick_exit is done when faded out - in advance_fading
		::System::VertexFormat fadingVertexFormat;

	protected:
		bool ignoreLongerFrame = false;
		::System::TimeStamp frameStart;
#ifdef FPS_INFO
		float fpsTime;
		float fpsCount;
		float fpsImmediate;
		float fpsAveraged;
		float fpsLastSecond;
		float fpsFrameTime;
		bool fpsIgnoreFirstSecond;
		Range fpsRange;
		Range fpsRange5;
		Range fpsRange5show;
		int fpsRange5Seconds;
		float fpsTotalFrames;
		float fpsTotalTime;
#endif
#ifdef AN_USE_FRAME_INFO
		struct FrameInfo
		{
			int frameNo = 0;
			bool hitchFrame = false;
			float deltaTime = 0.0f;
			float frameTime = 0.0f;
			float gameSyncFrameTime = 0.0f;
			float preAdvanceTime = 0.0f;
			float preGameLoopTime = 0.0f;
			float navSystemAdvanceTime = 0.0f;
			float extraAdvancesTime = 0.0f;
			float worldManagerSyncAdvanceTime = 0.0f;
			float worldManagerAsyncAdvanceTime = 0.0f;
			// pre + threads + post
			float preThreads = 0.0f;
			float threads = 0.0f;
			float threadSound = 0.0f;
			float threadRender = 0.0f;
			float threadGameLoop = 0.0f;
			float threadSoundGameLoop = 0.0f;
			float postThreads = 0.0f;
			// details
			float renderTime = 0.0f; // total
			float renderTime_part1 = 0.0f;
			float duringRenderPart1_screenShotContextPrepare = 0.0f;
			float duringRenderPart1_fading = 0.0f;
			float duringRenderPart1_takingControls = 0.0f;
			float duringRenderPart1_vignetteForDead = 0.0f;
			float duringRenderPart1_vignetteForMovement = 0.0f;
			float duringRenderPart1_allowGate = 0.0f;
			float renderTime_part2 = 0.0f;
			float duringRenderPart2_videoUpdate = 0.0f;
			float renderTime_part3 = 0.0f;
			float duringRenderPart3_endAndDisplay = 0.0f;
			float duringRenderPart3_beginRender = 0.0f;
			float renderTime_part4 = 0.0f;
			float renderTime_part5 = 0.0f;
			float gameSync = 0.0f; // time spent on game_sync
			float prepareToRender = 0.0f;
			float actualRenderTime = 0.0f;
			float actualRenderWorldTime = 0.0f;
			float buildRenderScenesTime = 0.0f;
			float startEndRenderSinceLastEndRender = 0.0f; 
			float immediateJobsPostRenderTime = 0.0f;
			float lastRenderToStartRenderTime = 0.0f;
			float lastRenderToDoJobsOnRenderTime = 0.0f;
			float lastRenderToPreEndRenderTime = 0.0f;
			float lastRenderToRenderTime = 0.0f;
			float soundTime = 0.0f;
			float gameLoopTime = 0.0f;
			float glWorldAdvance = 0.0f;
			float glWorldAdvanceDuringBlock = 0.0f;
			float glWorldAdvanceDuringBlock_readyToAdvance = 0.0f;
			float glWorldAdvanceDuringBlock_advanceLogic = 0.0f;
			float glWorldAdvancePostBlock = 0.0f;
			float glWorldAdvancePostBlock_advancePhysically = 0.0f;
			float glWorldAdvancePostBlock_advanceTemporaryObjects = 0.0f;
			float glWorldAdvancePostBlock_buildPresenceLinks = 0.0f;
			float glWorldAdvancePostBlock_redistributePresenceLinks = 0.0f;
			float glWorldAdvancePostBlock_advanceFinaliseFrame = 0.0f;
			float glWorldAdvancePostBlock_finaliseAdvance = 0.0f;
			float followUpTime = 0.0f;
			bool followUpRender = false;
			bool followUpSound = false;
			bool followUpGameLoop = false;
			System::SystemInfo systemInfo;

			void clear()
			{
				frameNo = 0;
				deltaTime = 0.0f;
				frameTime = 0.0f;
				gameSyncFrameTime = 0.0f;
				preAdvanceTime = 0.0f;
				preGameLoopTime = 0.0f;
				navSystemAdvanceTime = 0.0f;
				extraAdvancesTime = 0.0f;
				worldManagerSyncAdvanceTime = 0.0f;
				worldManagerAsyncAdvanceTime = 0.0f;
				preThreads = 0.0f;
				threads = 0.0f;
				threadSound = 0.0f;
				threadRender = 0.0f;
				threadGameLoop = 0.0f;
				threadSoundGameLoop = 0.0f;
				postThreads = 0.0f;
				renderTime = 0.0f;
				renderTime_part1 = 0.0f;
				duringRenderPart1_screenShotContextPrepare = 0.0f;
				duringRenderPart1_fading = 0.0f;
				duringRenderPart1_takingControls = 0.0f;
				duringRenderPart1_vignetteForDead = 0.0f;
				duringRenderPart1_vignetteForMovement = 0.0f;
				duringRenderPart1_allowGate = 0.0f;
				renderTime_part2 = 0.0f;
				duringRenderPart2_videoUpdate = 0.0f;
				renderTime_part3 = 0.0f;
				duringRenderPart3_endAndDisplay = 0.0f;
				duringRenderPart3_beginRender = 0.0f;
				renderTime_part4 = 0.0f;
				renderTime_part5 = 0.0f;
				gameSync = 0.0f;
				prepareToRender = 0.0f;
				actualRenderTime = 0.0f;
				actualRenderWorldTime = 0.0f;
				buildRenderScenesTime = 0.0f;
				startEndRenderSinceLastEndRender = 0.0f;
				immediateJobsPostRenderTime = 0.0f;
				lastRenderToDoJobsOnRenderTime = 0.0f;
				lastRenderToPreEndRenderTime = 0.0f;
				lastRenderToRenderTime = 0.0f;
				soundTime = 0.0f;
				gameLoopTime = 0.0f;
				glWorldAdvance = 0.0f;
				glWorldAdvanceDuringBlock = 0.0f;
				glWorldAdvanceDuringBlock_readyToAdvance = 0.0f;
				glWorldAdvanceDuringBlock_advanceLogic = 0.0f;
				glWorldAdvancePostBlock = 0.0f;
				glWorldAdvancePostBlock_advancePhysically = 0.0f;
				glWorldAdvancePostBlock_advanceTemporaryObjects = 0.0f;
				glWorldAdvancePostBlock_buildPresenceLinks = 0.0f;
				glWorldAdvancePostBlock_redistributePresenceLinks = 0.0f;
				glWorldAdvancePostBlock_advanceFinaliseFrame = 0.0f;
				glWorldAdvancePostBlock_finaliseAdvance = 0.0f;
				followUpTime = 0.0f;
				followUpRender = false;
				followUpSound = false;
				followUpGameLoop = false;
				systemInfo.clear_temporary();
			}
			void end_frame()
			{
				System::SystemInfo::store_in(systemInfo);
				System::SystemInfo::clear_temporary();
				frameNo = ::System::Core::get_frame();
			}

			void log(LogInfoContext & _log, bool _withSystemInfo, bool _withSysteminfoDetailed) const
			{
				_log.log(TXT("FRAME                    : %012i"), frameNo);
				_log.log(TXT("FRAME TIME               : %12.3fms"), frameTime * 1000.0f);
				_log.log(TXT("+- pre threads           : %12.3fms"), preThreads * 1000.0f);
				_log.log(TXT(" +- pre advance          : %12.3fms"), preAdvanceTime * 1000.0f);
				_log.log(TXT(" +- wrld mgr sync        : %12.3fms"), worldManagerSyncAdvanceTime * 1000.0f);
				_log.log(TXT(" +- pre game loop        : %12.3fms"), preGameLoopTime * 1000.0f);
				_log.log(TXT("   +- nav system         : %12.3fms"), navSystemAdvanceTime * 1000.0f);
				_log.log(TXT("   +- extra advances     : %12.3fms"), extraAdvancesTime * 1000.0f);
				_log.log(TXT("+- threads (not sum!)    : %12.3fms"), threads * 1000.0f);
				_log.log(TXT(" +- thread render        : %12.3fms"), threadRender * 1000.0f);
				_log.log(TXT("  +- render              : %12.3fms"), renderTime * 1000.0f);
				_log.log(TXT("   +- part 1             : %12.3fms"), renderTime_part1 * 1000.0f);
				_log.log(TXT("     +- screenshot ctxt  : %12.3fms"), duringRenderPart1_screenShotContextPrepare * 1000.0f);
				_log.log(TXT("     +- fading           : %12.3fms"), duringRenderPart1_fading * 1000.0f);
				_log.log(TXT("     +- taking controls  : %12.3fms"), duringRenderPart1_takingControls * 1000.0f);
				_log.log(TXT("     +- vignetter for dd : %12.3fms"), duringRenderPart1_vignetteForDead * 1000.0f);
				_log.log(TXT("     +- vignetter for mv : %12.3fms"), duringRenderPart1_vignetteForMovement * 1000.0f);
				_log.log(TXT("     +- prepare          : %12.3fms"), prepareToRender * 1000.0f);
				_log.log(TXT("       +- build rendscns : %12.3fms"), buildRenderScenesTime * 1000.0f);
				_log.log(TXT("     +- allow gate       : %12.3fms"), duringRenderPart1_allowGate * 1000.0f);
				_log.log(TXT("   +- part 2             : %12.3fms"), renderTime_part2 * 1000.0f);
				_log.log(TXT("     +- video update     : %12.3fms"), duringRenderPart2_videoUpdate * 1000.0f);
				_log.log(TXT("     +- im.jobs post rndr: %12.3fms"), immediateJobsPostRenderTime * 1000.0f);
				_log.log(TXT("   +- part 3             : %12.3fms"), renderTime_part3 * 1000.0f);
				_log.log(TXT("     +- end and display  : %12.3fms"), duringRenderPart3_endAndDisplay * 1000.0f);
				_log.log(TXT("     +- begin render     : %12.3fms"), duringRenderPart3_beginRender * 1000.0f);
				_log.log(TXT("   +- part 4             : %12.3fms"), renderTime_part4 * 1000.0f);
				_log.log(TXT("     +- actual render tm : %12.3fms"), actualRenderTime * 1000.0f);
				_log.log(TXT("       +- world          : %12.3fms"), actualRenderWorldTime * 1000.0f);
				_log.log(TXT("   +- part 5             : %12.3fms"), renderTime_part5 * 1000.0f);
				_log.log(TXT("   +- prev end -> submit : %12.3fms"), startEndRenderSinceLastEndRender * 1000.0f);
				_log.log(TXT("   END->"));
				_log.log(TXT("    +- end -> im.jobs    : %12.3fms"), lastRenderToDoJobsOnRenderTime * 1000.0f);
				_log.log(TXT("     | diff (do jobs)    : %12.3fms"), (lastRenderToPreEndRenderTime - lastRenderToDoJobsOnRenderTime) * 1000.0f);
				_log.log(TXT("    +- end -> pre end    : %12.3fms"), lastRenderToPreEndRenderTime * 1000.0f);
				_log.log(TXT("     | diff (submit?)    : %12.3fms"), (lastRenderToRenderTime - lastRenderToPreEndRenderTime) * 1000.0f);
				_log.log(TXT("    +- end -> end        : %12.3fms"), lastRenderToRenderTime * 1000.0f);
				_log.log(TXT(" +- thread sound + game  : %12.3fms"), threadSoundGameLoop * 1000.0f);
				_log.log(TXT(" +- thread sound         : %12.3fms"), threadSound * 1000.0f);
				_log.log(TXT("  +- sound               : %12.3fms"), soundTime * 1000.0f);
				_log.log(TXT(" +- thread game loop     : %12.3fms"), threadGameLoop * 1000.0f);
				_log.log(TXT("  +- game sync           : %12.3fms"), gameSync * 1000.0f);
				_log.log(TXT("  +- game loop           : %12.3fms"), gameLoopTime * 1000.0f);
				_log.log(TXT("   +- world advance      : %12.3fms"), glWorldAdvance * 1000.0f);
				_log.log(TXT("    +- during block      : %12.3fms"), glWorldAdvanceDuringBlock * 1000.0f);
				_log.log(TXT("     +- ready to advance : %12.3fms"), glWorldAdvanceDuringBlock_readyToAdvance * 1000.0f);
				_log.log(TXT("     +- advance logic    : %12.3fms"), glWorldAdvanceDuringBlock_advanceLogic * 1000.0f);
				_log.log(TXT("    +- post block        : %12.3fms"), glWorldAdvancePostBlock * 1000.0f);
				_log.log(TXT("     +- adv physically   : %12.3fms"), glWorldAdvancePostBlock_advancePhysically * 1000.0f);
				_log.log(TXT("     +- adv temp.obj.    : %12.3fms"), glWorldAdvancePostBlock_advanceTemporaryObjects * 1000.0f);
				_log.log(TXT("     +- build preslinks  : %12.3fms"), glWorldAdvancePostBlock_buildPresenceLinks * 1000.0f);
				_log.log(TXT("     +- redist preslinks : %12.3fms"), glWorldAdvancePostBlock_redistributePresenceLinks * 1000.0f);
				_log.log(TXT("     +- adv finalise frm : %12.3fms"), glWorldAdvancePostBlock_advanceFinaliseFrame * 1000.0f);
				_log.log(TXT("     +- finalise advance : %12.3fms"), glWorldAdvancePostBlock_finaliseAdvance * 1000.0f);
				_log.log(TXT("+- post threads          : %12.3fms"), postThreads * 1000.0f);
				_log.log(TXT(" +- wrld mgr async (a)   : %12.3fms"), worldManagerAsyncAdvanceTime * 1000.0f);
				_log.log(TXT("+- follow up %c%c%c      : %12.3fms"),
													followUpRender? 'R' : '-',
													followUpSound ? 'S' : '-',
													followUpGameLoop ? 'G' : '-',
													followUpTime * 1000.0f);
				if (_withSystemInfo)
				{
					LOG_INDENT(_log);
					systemInfo.log(_log, _withSysteminfoDetailed);
				}
			}
		};
		FrameInfo frameInfoA;
		FrameInfo frameInfoB;
		FrameInfo* currentFrameInfo = &frameInfoA;
		FrameInfo* prevFrameInfo = &frameInfoB;
		System::TimeStamp lastVRGameSyncTime;

		int initialFrameHack = 0; // first initial frames may be bonkers for time being. it is faded out anyway so it should not matter
		void next_frame_info();

#ifdef RENDER_HITCH_FRAME_INDICATOR
		struct HitchFrameIndicators
		{
			::System::TimeStamp hitchFrame; // render thread in general
			::System::TimeStamp renderThread; // render thread in general
			::System::TimeStamp renderPreEndToEnd; // if we've done everything and we wait too long for frame to submit
			::System::TimeStamp renderEndToPreEnd; // if we took to long to manage to aim at pre end
			::System::TimeStamp gameLoop; // game loop taking too much time
		} hitchFrameIndicators;
	public:
		bool showHitchFrameIndicator = false;
		bool allowShowHitchFrameIndicator = true;
#endif
#endif
		int hitchFrameCount = 0;
		int gameLoopHitchFrameCount = 0;
		struct InContinuous
		{
			int hitchFrameCount = 0;
			double time = 0.0;
			void reset()
			{
				hitchFrameCount = 0;
				time = 0.0;
			}
		} inContinuous;

#ifndef BUILD_PUBLIC_RELEASE
		int lastGameFrameNo = 0;
#endif

#ifdef AN_PERFORMANCE_MEASURE
	public:
		void stop_to_show_performance(bool _eachFrame = false) { stopToShowPerformance = true; stopToShowPerformanceEachFrame = _eachFrame; }
		void show_performance_in_frames(int _frames = 5);

	protected:
		bool showPerformance = false;
		bool logHitchDetails = false;
		bool allowToStopToShowPerformanceOnAnyHitch = false;
		bool allowToStopToShowPerformanceOnHighHitch = false;
		bool stopToShowPerformance = false;
		bool stopToShowPerformanceEachFrame = false;
		bool doesAllowPerformanceOutput = true;
		float blockAutoStopFor = 0.0f;
		Optional<int> showPerformanceAfterFrames;
		int showPerformanceAfterFramesStartedAt;
		bool noRenderUntilShowPerformance = false;
		bool noGameLoopUntilShowPerformance = false;
		Optional<int> delayedStorePerformanceFrameInfo;
		int delayedStorePerformanceFrameInfoDelayUsed = 0;
		String delayedStorePerformanceFrameInfoSuffix;
#endif
#ifdef AN_USE_FRAME_INFO
	protected:
		int logDeltaTimes = 0;
		bool warnLogDeltaTime = false;
		float spentOnDebugTime = 0.0f;
#endif
	protected:
		virtual tchar const* get_additional_hitch_info() const { return TXT(""); }

	protected:
		// the game has to decide how and when to show the panels
		DebugPanel* showDebugPanel = nullptr; // currently active
		DebugPanel* showDebugPanelVR = nullptr; // currently active
		DebugPanel* showLogPanelVR = nullptr; // currently active
		DebugPanel* showPerformancePanelVR = nullptr; // active if showing performance
#ifdef AN_RELEASE
		bool allowVRDebugPanel = false;
#else
		bool allowVRDebugPanel = true;
#endif

		DebugPanel* debugPanel;
		DebugPanel* debugLogPanel;
		DebugPanel* debugPerformancePanel;

#ifdef AN_TWEAKS
	private:
		bool reloadTweaksNeeded;
		::System::TimeStamp checkReloadTweaksAllowedTimeStamp;
		Concurrency::Semaphore checkReloadTweaks;
		Concurrency::SpinLock reloadTweaksLock = Concurrency::SpinLock(TXT("Framework.Game.reloadTweaksLock"));
		IO::DirEntry reloadTweaksFileInfo;

		static void check_tweaks_change_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		void reload_tweaks();
#endif

#ifdef AN_ALLOW_BULLSHOTS
	private:
		bool reloadBullshotsNeeded;
		::System::TimeStamp checkReloadBullshotsAllowedTimeStamp;
		Concurrency::Semaphore checkReloadBullshots;
		Concurrency::SpinLock reloadBullshotsLock = Concurrency::SpinLock(TXT("Framework.Game.reloadBullshotsLock"));
		IO::DirEntry reloadBullshotsFileInfo;

		static void check_bullshots_change_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		void reload_bullshots();
	protected:
		virtual void post_reload_bullshots();
#endif

		bool update_library_loading_result(Array<String> const & _fromDirectories, Array<String> const & _loadFiles, String const & _fromDirs, ::System::TimeStamp & _loadingTime);

	public:
		// this is to allow identification of objects in game, to connect various objects by same id, etc
		int get_unique_id();

	public:
		virtual bool send_quick_note_in_background(String const& _text, String const& _additionalText = String::empty()) { return false; }
		virtual bool send_full_log_info_in_background(String const& _text, String const& _additionalText = String::empty()) { return false; }

	private:
		Concurrency::SpinLock nextUniqueIDLock = Concurrency::SpinLock(TXT("Framework.Game.nextUniqueIDLock"));
		int nextUniqueID = 1; // should never be 0 as it means not set

	private:
		friend struct WorldManager;
		friend struct WorldManagerImmediateSyncJobsScopedLock;
	};

};

#ifdef AN_DEVELOPMENT
#define ASSERT_SYNC an_assert(Framework::Game::get()->is_performing_sync_job_on_this_thread(), TXT("we should be called from within sync job"))
#define ASSERT_NOT_SYNC an_assert(! Framework::Game::get()->is_performing_sync_job_on_this_thread(), TXT("we should not be called from within sync job"))
#define ASSERT_ASYNC an_assert(Framework::Game::get()->is_performing_async_job_on_this_thread(), TXT("we should be called from within async job"))
#define ASSERT_SYNC_OR(orCondition) an_assert(Framework::Game::get()->is_performing_sync_job_on_this_thread() || orCondition, TXT("we should be called from within async job or condition should be fulfilled"))
#define ASSERT_NOT_ASYNC_OR(orCondition) an_assert(Framework::Game::get()->is_performing_sync_job_on_this_thread() || ! Framework::Game::get()->is_performing_async_job_on_this_thread() || orCondition, TXT("we should be called from within sync job, not async job (unless it is synced part) or condition should be fulfilled"))
#define ASSERT_SYNC_OR_ASYNC an_assert(Framework::Game::get()->is_performing_async_job_on_this_thread() || Framework::Game::get()->is_performing_sync_job_on_this_thread())
#define ASSERT_NOT_SYNC_NOR_ASYNC an_assert(! Framework::Game::get()->is_performing_async_job_on_this_thread() && ! Framework::Game::get()->is_performing_sync_job_on_this_thread())
#define ASSERT_ASYNC_OR_LOADING_OR_PREVIEW an_assert(Framework::Game::get()->is_performing_async_job_on_this_thread() \
											  || Framework::Library::get_current()->get_state() != Framework::LibraryState::Idle \
											  || fast_cast<Framework::PreviewGame>(Framework::Game::get()) \
												 , TXT("we should be called from within async job or when loading a library"))
#else
#define ASSERT_SYNC
#define ASSERT_NOT_SYNC
#define ASSERT_ASYNC
#define ASSERT_SYNC_OR(...)
#define ASSERT_NOT_ASYNC_OR(...)
#define ASSERT_SYNC_OR_ASYNC
#define ASSERT_NOT_SYNC_NOR_ASYNC
#define ASSERT_ASYNC_OR_LOADING_OR_PREVIEW 
#endif
