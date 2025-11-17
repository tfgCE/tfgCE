#include "game.h"

#include "gameMode.h"
#include "gameSceneLayer.h"

#include "poiManager.h"

#include "..\debugSettings.h"
#include "..\debug\debugPanel.h"
#include "..\debug\previewGame.h"
#include "..\debug\testConfig.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\mainConfig.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\concurrency\asynchronousJobList.h"
#include "..\..\core\concurrency\threadSystemUtils.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\io\assetDir.h"
#include "..\..\core\io\assetFile.h"
#include "..\..\core\io\outputWriter.h"
#include "..\..\core\memory\softPooledObjectMT.h"
#include "..\..\core\types\names.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\input.h"
#include "..\..\core\system\recentCapture.h"
#ifdef AN_ANDROID
#include "..\..\core\system\android\androidApp.h"
#endif
#include "..\..\core\system\sound\soundSystem.h"
#include "..\..\core\system\video\renderTargetUtils.h"
#include "..\..\core\system\video\shaderProgramCache.h"
#include "..\..\core\system\video\video3dPrimitives.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"
#include "..\..\core\types\storeInScope.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\vr\vrOffsets.h"

#include "..\..\framework\library\usedLibraryStored.inl"

#include "..\library\library.h"

#include "..\jobSystem\jobSystem.h"

#include "..\pipelines\displayPipeline.h"
#include "..\pipelines\postProcessPipeline.h"
#include "..\pipelines\renderingPipeline.h"

#include "..\postProcess\postProcessRenderTargetManager.h"
#include "..\postProcess\chain\postProcessChain.h"

#include "..\loaders\iLoader.h"

#include "..\object\actor.h"
#include "..\object\item.h"
#include "..\object\scenery.h"
#include "..\object\temporaryObject.h"

#include "..\module\moduleAppearance.h"
#include "..\module\moduleCollision.h"
#include "..\module\modulePresence.h"

#include "..\nav\navSystem.h"

#include "..\world\doorInRoom.h"
#include "..\world\room.h"

#include "..\render\stats.h"

#include "delayedObjectCreation.h"
#include "gameConfig.h"
#include "gameInputDefinition.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "bullshotSystem.h"
#endif

#ifdef WITH_IN_GAME_EDITOR
#include "..\editor\editors\editorBase.h"
#include "..\editor\editorContext.h"
#include "..\video\basicDrawRenderingBuffer.h"
#include "..\video\spriteTextureProcessor.h"
#include "..\loaders\loaderCircles.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef AN_OUTPUT_WORLD_GENERATION
		#define OUTPUT_GENERATION
	#endif
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		//#define AN_DEBUG_WORLD_MANAGER_JOBS
	#endif
#endif

//#define INVESTIGATE_LOAD_LIBRARY_STALLING
//#define INSPECT_FADING

//

using namespace Framework;

//

DEFINE_STATIC_NAME(resetImmobileOriginVR);
DEFINE_STATIC_NAME(leftVRDebugPanel);
DEFINE_STATIC_NAME(saveFramePerformanceInfo);

// execution tags
DEFINE_STATIC_NAME(createCodeInfo);
DEFINE_STATIC_NAME(loadLibraryAndExit);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsNoDevelopmentInput, TXT("no development input"));

// input usage/option
DEFINE_STATIC_NAME(allowSavingFramePerformanceInfo);

// system tags
DEFINE_STATIC_NAME(lowGraphics);

// custom uniforms
DEFINE_STATIC_NAME(externalViewClipPlanes);
DEFINE_STATIC_NAME(externalViewClipPlanesCount);
DEFINE_STATIC_NAME(externalViewClipPlanesExclude);
DEFINE_STATIC_NAME(externalViewClipPlanesExcludeCount);

// global references
DEFINE_STATIC_NAME_STR(grGenericScenery, TXT("generic scenery"));

// library stored tags
DEFINE_STATIC_NAME(defaultColourPalette);

//

GameStaticInfo* GameStaticInfo::s_gameInfo = nullptr;
void GameStaticInfo::initialise_static()
{
	an_assert(!s_gameInfo);
	s_gameInfo = new GameStaticInfo();
}

void GameStaticInfo::close_static()
{
	an_assert(s_gameInfo);
	delete_and_clear(s_gameInfo);
}

//

CustomRenderContext::~CustomRenderContext()
{
	assert_rendering_thread();
	sceneRenderTarget = nullptr;
	finalRenderTarget = nullptr;
}

//

REGISTER_FOR_FAST_CAST(Game);

Game* Game::s_game = nullptr;

Game::Game()
: initialised(false)
, postProcessRenderTargetManager(nullptr)
, postProcessChain(nullptr)
, deltaTime(0.0f)
, destructionPendingObject(nullptr)
, runningSingleThreaded(false)
, mouseGrabbed(true)
, jobSystem(nullptr)
, systemJobs(TXT("system"))
, renderJobs(TXT("render"))
, soundJobs(TXT("sound"))
, prepareRenderJobs(TXT("prepareRenderJobs"))
, gameJobs(TXT("game"))
, backgroundJobs(TXT("background"))
, gameBackgroundJobs(TXT("gameBackground"))
, workerJobs(TXT("worker"))
, asyncWorkerJobs(TXT("asyncWorker"))
, loadingWorkerJobs(TXT("loadingWorker"))
, preparingWorkerJobs(TXT("preparingWorker"))
, defaultFontName(Names::default_, Names::default_)
, defaultFont(nullptr)
, worldManager(this)
, debugPanel(new DebugPanel(this))
, debugLogPanel(new DebugPanel(this, 3.0f))
, debugPerformancePanel(new DebugPanel(this, 3.0f))
{
	an_assert(GameStaticInfo::get());
	an_assert(s_game == nullptr);
	s_game = this;

	inContinuous.reset();
#ifdef FPS_INFO
	fpsTime = 0.0f;
	fpsCount = 0.0f;
	fpsImmediate = 0.0f;
	fpsAveraged = 0.0f;
	fpsLastSecond = 1.0f;
	fpsFrameTime = 0.0f;
	fpsIgnoreFirstSecond = true;
	fpsRange = Range::empty;
	fpsRange5 = Range::empty;
	fpsRange5show = Range::empty;
	fpsRange5Seconds = 0;
	fpsTotalFrames = 0.0f;
	fpsTotalTime = 0.0f;
#endif
#ifdef AN_USE_FRAME_INFO
#ifdef RENDER_HITCH_FRAME_INDICATOR
#ifdef BUILD_PREVIEW_RELEASE
	showHitchFrameIndicator = false;
#else
#ifdef BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE
	showHitchFrameIndicator = false;
#else
#ifdef BUILD_PUBLIC_RELEASE
	showHitchFrameIndicator = false;
#else
	showHitchFrameIndicator = false; // development, testing, disabled for time being, enable it here or hold both grips on when system library is loaded and main library loads
#endif
#endif
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
	showHitchFrameIndicator = true; // for profiler, always enable to give a hint of what's going on
#endif
#ifdef AN_PERFORMANCE_MEASURE
	showHitchFrameIndicator = true; // show info if measuring performance, even if not saving
#endif
#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH
	showHitchFrameIndicator = true; // if we're looking for FPI, show hitch frame indicator
#endif
#ifdef FORCE_RENDER_HITCH_FRAME_INDICATOR
	showHitchFrameIndicator = true; // forced, right?
#endif
#endif
#endif

	navSystem = new Nav::System(this);

	fadingVertexFormat.with_location_2d().no_padding().calculate_stride_and_offsets();

	generateMoreDetailsBySystem = false;

	debugLogPanel->keep_position(true);
	debugLogPanel->follow_camera(true);
	debugLogPanel->allow_change_orientation();
	debugLogPanel->set_default_panel_placement(Transform(Vector3(0.0f, 1.7f, 0.6f), Rotator3(30.0f, 0.0f, 0.0f).to_quat()));
	debugLogPanel->setup_show_log(40);

	debugPerformancePanel->auto_orientate();
	debugPerformancePanel->set_default_panel_placement(Transform(Vector3(0.0f, 0.8f, 0.0f), Rotator3(0.0f, 0.0f, 0.0f).to_quat()));
	debugPerformancePanel->show_performance();
}

Game::~Game()
{
	an_assert(!sceneLayerStack.is_set());

	delete_and_clear(debugPanel);
	delete_and_clear(debugLogPanel);
	delete_and_clear(debugPerformancePanel);
	delete_and_clear(navSystem);

	an_assert(s_game == this);
	s_game = nullptr;
}

String Game::get_game_name() const
{
	return String(::System::Core::get_app_name());
}

AI::Message* Game::create_ai_message(Name const& _name)
{
	return world ? world->create_ai_message(_name) : nullptr;
}

bool Game::is_generated(Framework::Room* _room) const
{
	if (!_room)
	{
		return false;
	}

	if (!_room->is_world_active())
	{
		return false;
	}

	for_every_ref(doc, delayedObjectCreations)
	{
		if (doc->roomType &&
			doc->inRoom == _room)
		{
			return false;
		}
	}

	if (auto* doc = delayedObjectCreationInProgress.get())
	{
		if (doc->roomType &&
			doc->inRoom == _room)
		{
			return false;
		}
	}

	return true;
}

bool Game::is_generated(Framework::Region* _region) const
{
	if (!_region)
	{
		return false;
	}

	for_every_ref(doc, delayedObjectCreations)
	{
		if (doc->roomType &&
			doc->inRoom.is_set() && doc->inRoom->is_in_region(_region))
		{
			return false;
		}
	}

	if (auto* doc = delayedObjectCreationInProgress.get())
	{
		if (doc->roomType &&
			doc->inRoom.is_set() && doc->inRoom->is_in_region(_region))
		{
			return false;
		}
	}

	return true;
}

void Game::start_mode(GameModeSetup* _modeSetup)
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("before we start mode, end current mode"));
#endif
	end_mode();
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("start mode"));
#endif
	_modeSetup->fill_missing(Random::Generator().temp_ref());
	mode = _modeSetup->create_mode();
	endModeRequested = false;
	mode->set_game(this);
	mode->on_start();
}

void Game::end_mode(bool _abort)
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("end mode%S"), _abort ? TXT(" (abort)") : TXT(""));
#endif
	if (mode.is_set())
	{
		mode->on_end(_abort);
	}
	mode = nullptr;
	endModeRequested = false;
}

void Game::setup_game_advance_context(REF_ GameAdvanceContext& _gameAdvanceContext)
{
	_gameAdvanceContext.set_delta_time(get_delta_time());
}

void Game::clear_scene_layer_stack()
{
	while (sceneLayerStack.is_set())
	{
		sceneLayerStack->pop();
	}
}

void Game::push_scene_layer(GameSceneLayer* _scene)
{
	_scene->set_game(this);
	_scene->push_onto(sceneLayerStack);
}

void Game::update_on_system_change()
{
	// resetup variables that depend on core
	generateMoreDetailsBySystem = true;
	if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
	{
		generateMoreDetailsBySystem = false;
	}
}

void Game::request_ready_and_activate_all_inactive(Framework::ActivationGroup* _activationGroup, World* _world)
{
#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("request ready and activate all inactive"));
#endif
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("[ActivationGroup] Game::request_ready_and_activate_all_inactive [%i]"), _activationGroup ? _activationGroup->get_id() : NONE);
#endif
	Framework::ActivationGroupPtr activationGroup;
	activationGroup = _activationGroup;
	add_async_world_job(TXT("ready all inactive"), [this, activationGroup, _world]() {async_ready_all_inactive(activationGroup.get(), _world); });
	add_sync_world_job(TXT("activate all inactive"), [this, activationGroup, _world]() {sync_activate_all_inactive(activationGroup.get(), _world); });
}

void Game::async_ready_all_inactive(Framework::ActivationGroup* _activationGroup, World* _world)
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("[ActivationGroup] Game::async_ready_all_inactive [%i]"), _activationGroup ? _activationGroup->get_id() : NONE);
#endif
	MEASURE_PERFORMANCE(game_readyAllInactive);
	Framework::ActivationGroupPtr activationGroup;
	activationGroup = _activationGroup;
	World* useWorld = _world ? _world : world;
	while (useWorld && !useWorld->ready_to_activate_all_inactive(_activationGroup)) {}
}

void Game::sync_activate_all_inactive(Framework::ActivationGroup* _activationGroup, World* _world)
{
	ASSERT_SYNC;

	World* useWorld = _world ? _world : world;
	if (useWorld)
	{
		useWorld->activate_all_inactive(_activationGroup);
	}
}

void Game::reload_input_config_files()
{
#ifndef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_ANDROID
	if (IO::AssetFile::does_exist(IO::get_asset_data_directory() + get_base_input_config_file())) // check if exists, we don't want a trace in a log file
#else
	if (IO::File::does_exist(IO::get_user_game_data_directory() + get_base_input_config_file())) // check if exists, we don't want a trace in a log file
#endif
#endif
	{
		GameInputDefinitions::clear();
		::Library::load_global_config_from_file(get_base_input_config_file(), ::LibrarySource::Assets); // always load base (even in final, we might be testing a final build or creating a main config file)
	}
}

void Game::load_system_tags_file()
{
	an_assert(!::System::Core::get(), TXT("core should not exist when we load system tags!"));
	// first load systemTags as they may change completely how the game behaves
	::System::Core::clear_extra_requested_system_tags();
	if (IO::File::does_exist(IO::get_user_game_data_directory() + get_system_tags_file()))
	{
		IO::File f;
		f.open(IO::get_user_game_data_directory() + get_system_tags_file());
		f.set_type(IO::InterfaceType::Text);
		List<String> systemTagsLines;
		f.read_lines(systemTagsLines);
		for_every(st, systemTagsLines)
		{
			::System::Core::set_extra_requested_system_tags(st->to_char(), true /*add*/);
		}
	}
	::System::Core::loaded_extra_requested_system_tags();
}

void Game::load_config_files()
{
	// load config
	// main < development < user (for development builds, user is not loaded unless config file states so)
	// - "main" contains ALL
	// - "base" contains ALL and is used to redefine things <- this is not included in the build but mainConfig is built on this (and only on this)
	// - "user" contains vr, video, sound setups/configs <- this is not included in the build and will be kept local to any player
	// - "development" contains extra things that can be changed <- this is not included in the build
#ifdef AN_DEVELOPMENT_OR_PROFILER
	// load main only if base does not exist
#ifdef AN_ANDROID
	if (!IO::AssetFile::does_exist(IO::get_asset_data_directory() + get_base_config_file()) &&
		!IO::AssetFile::does_exist(IO::get_asset_data_directory() + get_base_input_config_file()))
#else
	if (! IO::File::does_exist(IO::get_user_game_data_directory() + get_base_config_file()) &&
		! IO::File::does_exist(IO::get_user_game_data_directory() + get_base_input_config_file()))
#endif
#endif
	if (!wantsToCreateMainConfigFile)
	{
		::Library::load_global_config_from_file(get_main_config_file(), ::LibrarySource::Assets);
	}
#ifndef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_ANDROID
	if (IO::AssetFile::does_exist(IO::get_asset_data_directory() + get_base_config_file())) // check if exists, we don't want a trace in a log file
#else
	if (IO::File::does_exist(IO::get_user_game_data_directory() + get_base_config_file())) // check if exists, we don't want a trace in a log file
#endif
#endif
	{
		::Library::load_global_config_from_file(get_base_config_file(), ::LibrarySource::Assets); // always load base (even in final, we might be testing a final build or creating a main config file)
	}
#ifndef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_ANDROID
	if (IO::AssetFile::does_exist(IO::get_asset_data_directory() + get_base_input_config_file())) // check if exists, we don't want a trace in a log file
#else
	if (IO::File::does_exist(IO::get_user_game_data_directory() + get_base_input_config_file())) // check if exists, we don't want a trace in a log file
#endif
#endif
	{
		::Library::load_global_config_from_file(get_base_input_config_file(), ::LibrarySource::Assets); // always load base (even in final, we might be testing a final build or creating a main config file)
	}
	if (!wantsToCreateMainConfigFile)
	{
		MainConfig::store_defaults();
		{
			::Library::load_global_config_from_file(get_user_config_file(), ::LibrarySource::Files);
			save_config_file(); // resave it now
		}
		{
			todo_hack(TXT("we always want to use thread setup from main config (this is sort of a hack for an older version, where it was stored in user config)"));
			MainConfig::access_global().set_thread_setup_from(MainConfig::defaults()); 
		}
		{
			for_count(int, src, 2)
			{
				::LibrarySource::Type srcType = src == 0 ? ::LibrarySource::Assets : ::LibrarySource::Files;
				bool loadedDevConfig = false;
				if (src == 0)
				{
					IO::AssetDir dir;
					dir.list(IO::get_asset_data_directory());
					auto fileNames = dir.get_files();
					auto devConfigPrefix = get_development_config_file_prefix();
					for_every(fileName, fileNames)
					{
						if (fileName->get_left(devConfigPrefix.get_length()) == devConfigPrefix)
						{
							::Library::load_global_config_from_file(*fileName, srcType);
							loadedDevConfig = true;
						}
					}
				}
				else
				{
					IO::Dir dir;
					dir.list(IO::get_user_game_data_directory());
					auto dirEntries = dir.get_infos();
					auto devConfigPrefix = get_development_config_file_prefix();
					for_every(dirEntry, dirEntries)
					{
						if (dirEntry->filename.get_left(devConfigPrefix.get_length()) == devConfigPrefix)
						{
							::Library::load_global_config_from_file(dirEntry->filename, srcType);
							loadedDevConfig = true;
						}
					}
				}
#ifdef AN_DEVELOPMENT_OR_PROFILER
				// reload user config file in case we want to test it, by default we load user config but override it with dev
				if (loadedDevConfig)
				{
					DEFINE_STATIC_NAME(testUserConfigFile);
					if (TestConfig::get_params().get_value<bool>(NAME(testUserConfigFile), false))
					{
						::Library::load_global_config_from_file(get_user_config_file(), srcType);
					}
				}
#endif
			}
		}
	}
}

void Game::use_world(bool _use)
{
	if (worldInUse != _use)
	{
		worldInUse = _use;
	}
}

void Game::post_use_world(bool _use)
{
	add_sync_world_job(TXT("use world"), [this, _use]() { use_world(_use); });
}

void Game::request_world(Random::Generator _withGenerator)
{
	add_async_world_job(TXT("create world"), [this, _withGenerator]()
		{
			MEASURE_PERFORMANCE(createWorld);
			async_create_world(_withGenerator);
		});
}

void Game::async_create_world(Random::Generator _withGenerator)
{
	MEASURE_PERFORMANCE(game_createWorld);

#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("create world"));
#endif

	an_assert(!world);
	Framework::World* pendingWorld = new Framework::World();
	Framework::SubWorld* pendingSubWorld = nullptr;

	//set_wants_to_cancel_creation(false); // we want to create since now

	perform_sync_world_job(TXT("create pending subworld"), [&pendingSubWorld, pendingWorld]()
		{
			pendingSubWorld = new Framework::SubWorld(pendingWorld);
		});

	pendingSubWorld->set_individual_random_generator(_withGenerator);
	pendingSubWorld->init();

	perform_sync_world_job(TXT("pending to current"), [this, &pendingWorld, &pendingSubWorld]()
		{
			world = pendingWorld;
			subWorld = pendingSubWorld;
		});

#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("world = w%p"), world);
	output(TXT("subWorld = sw'%p"), subWorld);
#endif
}

void Game::request_destroy_world()
{
	output(TXT("request destroy world"));
	set_wants_to_cancel_creation(true);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("(cancel nav system tasks)"));
#endif
	navSystem->cancel_all();
	add_sync_world_job(TXT("destroy world"), [this]() {sync_destroy_world(true); });
}

void Game::sync_destroy_world(bool _allowAsync)
{
	ASSERT_SYNC;

	if ((navSystem && !navSystem->advance_till_done()) ||
		is_destroying_world_blocked() ||
		is_destruction_suspended())
	{
#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
		if ((navSystem && !navSystem->advance_till_done()))
		{
			output(TXT("postpone destroy world due to nav system"));
		}
		if (is_destroying_world_blocked())
		{
			output(TXT("postpone destroy world due world destruction blocked"));
		}
#endif
		add_sync_world_job(TXT("postponed destroy world"), [this, _allowAsync]() { sync_destroy_world(_allowAsync); });
	}
	else
	{
		drop_all_delayed_object_creations();

		// delete whole world
		if (_allowAsync)
		{
			add_async_world_job_top_priority(TXT("async destroy world"), [this]() { delete_and_clear(world); });
		}
		else
		{
			delete_and_clear(world);
		}

		worldInUse = false;
	}
}

void Game::start(bool _loadConfigAndExit)
{
	an_assert(!initialised);

	load_system_tags_file();

	initialise_system_core();
	
	update_on_system_change();

	load_config_files();

	if (_loadConfigAndExit)
	{
		return;
	}

#ifdef AN_DEVELOPMENT
	// we should have test config loaded right now
	if (auto* gdm = TestConfig::get_params().get_existing<int>(Name(TXT("generateDevMeshes"))))
	{
		generateDevMeshes = *gdm;
	}
#endif
#ifdef AN_PROFILER
	// we should have test config loaded right now
	if (auto* gdm = TestConfig::get_params().get_existing<int>(Name(TXT("generateDevMeshesProfiler"))))
	{
		generateDevMeshes = *gdm;
	}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	// we should have test config loaded right now
	if (auto * gal = TestConfig::get_params().get_existing<int>(Name(TXT("generateAtLOD"))))
	{
		generateAtLOD = *gal;
	}

	if (auto * gal = TestConfig::get_params().get_existing<bool>(Name(TXT("generateMoreDetails"))))
	{
		generateMoreDetails = *gal;
	}
#endif

	initialise_system();

	initialise_job_system();

	::System::Core::on_quick_exit(TXT("save game config"), [this]() { this->closingGame = true; this->save_config_file(); });
	::System::Core::set_quick_exit_handler([this]() {this->quick_exit(); });

	post_start();
}

void Game::post_start()
{
	// always add it but leave empty if not required
	{
		String func;
		func += TXT("void handle_external_view_planes(vec3 loc)\n");
		func += TXT("{\n");
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		func += TXT("  float bias = 0.001;\n");
		func += TXT("  for (int i = 0; i < externalViewClipPlanesCount; ++i)\n");
		func += TXT("  {\n");
		func += TXT("    vec4 plane = externalViewClipPlanes[i];\n");
		func += TXT("    if (loc.x * plane.x + loc.y * plane.y + loc.z * plane.z + plane.w < -bias)\n");
		func += TXT("    {\n");
		func += TXT("      discard;\n");
		func += TXT("    }\n");
		func += TXT("  }\n");
		func += TXT("  if (externalViewClipPlanesExcludeCount > 0)\n");
		func += TXT("  {\n");
		func += TXT("    bool inside = true;\n");
		func += TXT("    int insideCount = 0;\n");
		func += TXT("    for (int i = 0; i < externalViewClipPlanesExcludeCount; ++i)\n");
		func += TXT("    {\n");
		func += TXT("      for (; i < externalViewClipPlanesExcludeCount; ++i)\n");
		func += TXT("      {\n");
		func += TXT("        vec4 plane = externalViewClipPlanesExclude[i];\n");
		func += TXT("        if (plane.x == 0.0 && plane.y == 0.0 && plane.z == 0.0)\n");
		func += TXT("        {\n");
		func += TXT("          if (inside && insideCount > 0)\n");
		func += TXT("          {\n");
		func += TXT("            break;\n");
		func += TXT("          }\n");
		func += TXT("          /* next plane */\n");
		func += TXT("          inside = true;\n");
		func += TXT("          insideCount = 0;\n");
		func += TXT("        }\n");
		func += TXT("        else if (loc.x * plane.x + loc.y * plane.y + loc.z * plane.z + plane.w > bias)\n");
		func += TXT("        {\n");
		func += TXT("          /* still inside this one */\n");
		func += TXT("          ++insideCount;\n");
		func += TXT("        }\n");
		func += TXT("        else\n");
		func += TXT("        {\n");
		func += TXT("          /* outside this one, just continue till the next one */\n");
		func += TXT("          inside = false;\n");
		func += TXT("        }\n");
		func += TXT("      }\n");
		func += TXT("      if (inside && insideCount > 0)\n");
		func += TXT("      {\n");
		func += TXT("        discard;\n");
		func += TXT("      }\n");
		func += TXT("    }\n");
		func += TXT("  }\n");
#endif
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functionsFragmentShaderOnly.push_back(func);

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		::System::ShaderSourceCustomisation::get()->dataFragmentShaderOnly.push_back(String(TXT("uniform int externalViewClipPlanesCount;")));
		::System::ShaderSourceCustomisation::get()->dataFragmentShaderOnly.push_back(String(TXT("uniform vec4 externalViewClipPlanes[32];")));
		// these are planes that are grouped to exclude something that is beyond all of them
		// zero plane breaks groups
		::System::ShaderSourceCustomisation::get()->dataFragmentShaderOnly.push_back(String(TXT("uniform int externalViewClipPlanesExcludeCount;")));
		::System::ShaderSourceCustomisation::get()->dataFragmentShaderOnly.push_back(String(TXT("uniform vec4 externalViewClipPlanesExclude[128];")));
#endif
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	Array<Name> uniforms;
	uniforms.push_back(NAME(externalViewClipPlanes));
	uniforms.push_back(NAME(externalViewClipPlanesCount));
	uniforms.push_back(NAME(externalViewClipPlanesExclude));
	uniforms.push_back(NAME(externalViewClipPlanesExcludeCount));
	::System::Video3D::get()->add_custom_set_build_in_uniforms(NAME(externalViewClipPlanes), uniforms,
		[this](::System::ShaderProgram* _shader)
		{
			int externalViewClipPlanesUniformID = _shader->find_uniform_index(NAME(externalViewClipPlanes));
			if (externalViewClipPlanesUniformID != NONE)
			{
				int externalViewClipPlanesCountUniformID = _shader->find_uniform_index(NAME(externalViewClipPlanesCount));
				if (externalViewClipPlanesCountUniformID != NONE)
				{
					if (externalViewClipPlanesReadyForRendering.is_empty() &&
						!externalViewClipPlanes.planes.is_empty())
					{
						::System::VideoMatrixStackMode::Type matrixStackMode = ::System::Video3D::get()->get_model_view_matrix_stack().get_mode();
						for_every(p, externalViewClipPlanes.planes)
						{
							Plane planeToReady = *p;
							if (matrixStackMode == ::System::VideoMatrixStackMode::xRight_yForward_zUp)
							{
								::System::Video3D::ready_plane_for_video_x_right_y_forward_z_up(REF_ planeToReady);
							}
							externalViewClipPlanesReadyForRendering.push_back(planeToReady.to_vector4_for_rendering());
						}
					}
					_shader->set_uniform(externalViewClipPlanesCountUniformID, externalViewClipPlanes.planes.get_size());
					_shader->set_uniform(externalViewClipPlanesUniformID, externalViewClipPlanesReadyForRendering.get_data(), externalViewClipPlanesReadyForRendering.get_size());
				}
			}
			int externalViewClipPlanesExcludeUniformID = _shader->find_uniform_index(NAME(externalViewClipPlanesExclude));
			if (externalViewClipPlanesExcludeUniformID != NONE)
			{
				int externalViewClipPlanesExcludeCountUniformID = _shader->find_uniform_index(NAME(externalViewClipPlanesExcludeCount));
				if (externalViewClipPlanesExcludeCountUniformID != NONE)
				{
					if (externalViewClipPlanesExcludeReadyForRendering.is_empty() &&
						!externalViewClipPlanesExclude.planes.is_empty())
					{
						::System::VideoMatrixStackMode::Type matrixStackMode = ::System::Video3D::get()->get_model_view_matrix_stack().get_mode();
						for_every(p, externalViewClipPlanesExclude.planes)
						{
							Plane planeToReady = *p;
							if (! planeToReady.is_zero())
							{
								if (matrixStackMode == ::System::VideoMatrixStackMode::xRight_yForward_zUp)
								{
									::System::Video3D::ready_plane_for_video_x_right_y_forward_z_up(REF_ planeToReady);
								}
								externalViewClipPlanesExcludeReadyForRendering.push_back(planeToReady.to_vector4_for_rendering());
							}
							else
							{
								externalViewClipPlanesExcludeReadyForRendering.push_back(Vector4::zero);
							}
						}
					}
					_shader->set_uniform(externalViewClipPlanesExcludeCountUniformID, externalViewClipPlanesExclude.planes.get_size());
					_shader->set_uniform(externalViewClipPlanesExcludeUniformID, externalViewClipPlanesExcludeReadyForRendering.get_data(), externalViewClipPlanesExcludeReadyForRendering.get_size());
				}
			}
		});
#endif
}

void Game::pre_close()
{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	::System::Video3D::get()->remove_custom_set_build_in_uniforms(NAME(externalViewClipPlanes));
#endif
}

void Game::close_and_delete()
{
	pre_close();

	closingGame = true;

	::System::Core::set_quick_exit_handler(nullptr);

	navSystem->end();

	// try to end all sub systems
	bool allDone = false;
	while (!allDone)
	{
		allDone = true;
		allDone &= navSystem->advance_till_done();

		jobSystem->clean_up_finished_off_system_jobs();
		jobSystem->execute_main_executor();
		System::Core::sleep(0.001f);
	}

	// no saving game config as things are already at least partially unloaded

	close_job_system();

	close_system();

	output_colour(0, 0, 0, 1);
	output(TXT("game ended"));
	output_colour();
	delete this;
}

bool Game::does_use_vr() const
{
	todo_note(TXT("for time being, it always uses vr if can be used (or forced)"));
	return VR::IVR::can_be_used() || 
		(MainConfig::global().should_force_vr());
}

bool Game::should_render_to_main_for_vr() const
{
#ifdef AN_ANDROID
	return false;
#else
	return true;
#endif
}

bool Game::can_use_vr() const
{
	return VR::IVR::can_be_used() ||
		(MainConfig::global().should_force_vr());
}

void Game::initialise_system_core()
{
	output_colour_system();
	output(TXT("initialising core..."));
	output_colour();
	::System::Core::create();
}

void Game::load_game_settings()
{
	::Library::load_global_settings_from_directory(get_settings_directory(), ::LibrarySource::Assets);
}

void Game::initialise_system()
{
	::System::Input::create();

	output_colour_system();
	output(TXT("loading settings..."));
	output_colour();
	
	// load settings before we start - first main settings
	load_game_settings();

	// check if everything is fine with settings
	Collision::DefinedFlags::check_and_cache();
	Framework::Nav::DefinedFlags::check_and_cache();

#ifdef AN_VR
	bool vrFault = false;
	// create vr (to get preferred full screen resolution - before we initialise video)
	if (MainConfig::global().should_allow_vr() ||
		MainConfig::global().should_force_vr())
	{
		output_colour_system();
		output(TXT("initialising VR..."));
		output_colour();
		VR::IVR::create();

		if (!VR::IVR::get() || ! VR::IVR::get()->is_ok(true))
		{
			vrFault = true;
		}
	}
#ifdef AN_RELEASE
	else
	{
		vrFault = true;
	}
#endif

	if (!get_execution_tags().has_tag(NAME(createCodeInfo)) &&
		!get_execution_tags().has_tag(NAME(loadLibraryAndExit)))
	{
		if (vrFault)
		{
			output(TXT("VR device required"));
#ifdef AN_WINDOWS
			MessageBox(nullptr, TXT("VR device required"), TXT("Problem!"), MB_ICONEXCLAMATION | MB_OK);
#endif
			::System::Core::quick_exit();
		}
		else if (auto* vr = VR::IVR::get())
		{
			if (!vr->get_error_list().is_empty())
			{
				String info(vr->get_error_list().get_size() > 1 ? TXT("Problems encountered during VR init:") : TXT("Problem encountered during VR init:"));
				String errors;
				for_every(err, vr->get_error_list())
				{
					if (!errors.is_empty())
					{
						errors += TXT(",\n");
					}
					errors += *err;
				}
				info += TXT("\n");
				info += errors;
				output(info.to_char());
#ifdef AN_WINDOWS
				MessageBox(nullptr, info.to_char(), TXT("Problem!"), MB_ICONEXCLAMATION | MB_OK);
#endif
				::System::Core::quick_exit();
			}
		}
	}
#endif

	output_colour_system();
	output(TXT("initialising video..."));
	output_colour();
	// create video 3d, but first make sure everything is set
	MainConfig::access_global().access_video().fill_missing();
	::System::Video3D::create(&MainConfig::global().get_video());
	::System::Video3D* v3d = ::System::Video3D::get();

	::System::Core::set_title(get_game_name());

	::System::Input::get()->init_cursor();

	// create vr (after video3d because we need to create shaders and render targets)
	// it also enters vr mode if required
	if (VR::IVR::get())
	{
		output_colour_system();
		output(TXT("activating VR..."));
		output_colour();
		VR::IVR::get()->init(v3d);
		mouseGrabbed = !VR::IVR::get()->can_be_used();
	}

	output_colour_system();
	output(TXT("creating post process render target manager..."));
	output_colour();

	// create post process render target manager
	postProcessRenderTargetManager = new PostProcessRenderTargetManager();

	output_colour_system();
	output(TXT("default background..."));
	output_colour();

	// default background
	v3d->clear_colour_depth_stencil(Colour::black);
	{
		MEASURE_PERFORMANCE(v3d_displayBuffer);
		v3d->display_buffer();
	}

	// hide cursor and grab mouse input
	if (::System::Input::get())
	{
		output_colour_system();
		output(TXT("grab input"));
		output_colour();
		::System::Input::get()->grab(is_input_grabbed());
	}

	output_colour_system();
	output(TXT("initialising sound..."));
	output_colour();
	MainConfig::access_global().access_sound().fill_missing();
#ifdef AN_DEVELOPMENT_OR_PROFILER
	MainConfig::access_global().access_sound().decide_development_volume(VR::IVR::get() != nullptr);
#endif
	::System::Sound::create(&MainConfig::global().get_sound());

	::System::Core::post_init_all_subsystems();

	output_colour_system();
	output(TXT("systems initialised"));
	output_colour();

	::System::Core::output_system_info();

	create_render_buffers();
}

void Game::reinitialise_video(bool _softReinitialise, bool _newRenderTargetsNeeded)
{
	_newRenderTargetsNeeded |= ! _softReinitialise;
	output_colour_system();
	output(TXT("reinitialising video"));
	output(_softReinitialise? TXT("  soft") : TXT("  reinitialise system video"));
	output(_newRenderTargetsNeeded ? TXT("  new render targets required") : TXT("  no new render targets required"));
	output_colour();

	an_assert(::System::Video3D::get());

	if (!_softReinitialise)
	{
		::System::Video3D::get()->init(MainConfig::global().get_video());
	}

	// soft
	{
		::System::Video3D::get()->skip_display_buffer_for_VR(MainConfig::global().get_video().skipDisplayBufferForVR);

		if (_newRenderTargetsNeeded)
		{
			create_render_buffers();
		}

		if (postProcessChain)
		{
			postProcessChain->mark_linking_required();
		}
	}

	output_colour_system();
	output(TXT("video reinitialised"));
	output_colour();
}

void Game::prepare_render_buffer_resolutions(REF_ VectorInt2 & _mainResolution, REF_ VectorInt2 & _secondaryViewResolution)
{
	// keep them as they are
}

void Game::create_render_buffers()
{
	// create render targets for output first, we need to know their size to adjust other values
	if (can_use_vr())
	{
		// don't create if not needed
		Vector2 fallbackEyeResolution = MainConfig::access_global().access_video().resolution.to_vector2() * Vector2(0.5f, 1.0f);
		// use htc vive
		fallbackEyeResolution.x = 1080;
		fallbackEyeResolution.y = 1200;
		VR::IVR::get()->create_render_targets_for_output(RenderingPipeline::get_default_output_definition(), fallbackEyeResolution.to_vector_int2());
	}

	VectorInt2 mainResolution = MainConfig::global().get_video().resolution;
	VectorInt2 secondaryViewResolution = MainConfig::global().get_video().resolution;

	prepare_render_buffer_resolutions(REF_ mainResolution, REF_ secondaryViewResolution);

	// create render buffer (create it always as we could switch from vr to nonvr)
	{
		float aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef;
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
			apply_aspect_ratio_coef(mainResolution, aspectRatioCoef),
			MainConfig::global().get_video().get_msaa_samples(),
			::System::DepthStencilFormat::defaultFormat);
		rtSetup.set_aspect_ratio_coef(aspectRatioCoef);
		rtSetup.copy_output_textures_from(RenderingPipeline::get_default_output_definition());
		RENDER_TARGET_CREATE_INFO(TXT("main render buffer"));
		mainRenderBuffer = new ::System::RenderTarget(rtSetup);
	}

	// main render output (used as source when combining stuff together, only used if external view resolution is in use (otherwise it is already rendered to main output), has the same resolution as main render))
	mainRenderOutputRenderBuffer = nullptr;
	if (!secondaryViewResolution.is_zero())
	{
		float aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef;
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
			apply_aspect_ratio_coef(mainResolution, aspectRatioCoef),
			MainConfig::global().get_video().get_msaa_samples(),
			::System::DepthStencilFormat::defaultFormat);
		rtSetup.set_aspect_ratio_coef(aspectRatioCoef);
		rtSetup.copy_output_textures_from(RenderingPipeline::get_default_output_definition());
		RENDER_TARGET_CREATE_INFO(TXT("main render buffer"));
		mainRenderOutputRenderBuffer = new ::System::RenderTarget(rtSetup);
	}

	secondaryViewRenderBuffer = nullptr;
	if (!secondaryViewResolution.is_zero())
	{
		float aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef;
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
			apply_aspect_ratio_coef(secondaryViewResolution, aspectRatioCoef),
			MainConfig::global().get_video().get_msaa_samples(),
			::System::DepthStencilFormat::defaultFormat);
		rtSetup.set_aspect_ratio_coef(aspectRatioCoef);
		rtSetup.copy_output_textures_from(RenderingPipeline::get_default_output_definition());
		RENDER_TARGET_CREATE_INFO(TXT("main render buffer"));
		secondaryViewRenderBuffer = new ::System::RenderTarget(rtSetup);
	}

	secondaryViewOutputRenderBuffer = nullptr;
	if (!secondaryViewResolution.is_zero())
	{
		float aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef;
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
			apply_aspect_ratio_coef(secondaryViewResolution, aspectRatioCoef),
			MainConfig::global().get_video().get_msaa_samples(),
			::System::DepthStencilFormat::defaultFormat);
		rtSetup.set_aspect_ratio_coef(aspectRatioCoef);
		rtSetup.copy_output_textures_from(RenderingPipeline::get_default_output_definition());
		RENDER_TARGET_CREATE_INFO(TXT("main render buffer"));
		secondaryViewOutputRenderBuffer = new ::System::RenderTarget(rtSetup);
	}
}

int count_jobs(JobListDefinition const & _jobListDefinition, float _processorCount)
{
	return max(1, clamp((int)(clamp(_jobListDefinition.percentage, 0.0f, 1.0f) * (_jobListDefinition.allowOnMainThread ? _processorCount : _processorCount - 1)), _jobListDefinition.cores.min, _jobListDefinition.cores.max));
}

bool add_to_executor(JobSystemExecutor* _executor, REF_ int & _jobsLeft, JobListDefinition const & _jobListDefinition, Name const & _name, int _priority, Optional<bool> const & _mainExecutor = NP)
{
	bool mainExecutor = _mainExecutor.get(false);
	if (_jobsLeft > 0 && (_jobListDefinition.allowOnMainThread || !mainExecutor))
	{
		_executor->add_list(_name, _priority);
		--_jobsLeft;
		return true;
	}
	return false;
}

void add_extra_executors(JobSystem* _jobSystem, Array<JobSystemExecutor*> & _extraExecutors, REF_ int & _jobsLeft, Name const & _name)
{
	// create executor
	while (_jobsLeft > 0)
	{
		JobSystemExecutor* extraExecutor = _jobSystem->create_executor();
		extraExecutor->add_list(_name); // priority doesn't matter
		_extraExecutors.push_back(extraExecutor);
		--_jobsLeft;
	}
}

namespace PopulateExecutors
{
	enum Result
	{
		NothingDone,
		CantDoYet,
		Handled,
	};
}

PopulateExecutors::Result populate_executors(bool _allowBundlingNow, bool _allowAvoidanceNow, JobSystemExecutor* _mainExecutor, Array<JobSystemExecutor*> & _executors, REF_ int & _jobsLeft, JobListDefinition const & _jobListDefinition, Name const & _name, int _priority)
{
	if (_jobsLeft == 0)
	{
		return PopulateExecutors::NothingDone;
	}

	{
		Array<JobSystemExecutor*> allExecutors;
		for_every_ptr(executor, _executors)
		{
			allExecutors.push_back(executor);
		}

		{
			// already added to these executors, no need to add more/again
			for_every_ptr(executor, allExecutors)
			{
				if (executor->does_handle(_name))
				{
					return PopulateExecutors::NothingDone;
				}
			}
		}

		if (!_jobListDefinition.bundleWithJobList.is_empty())
		{
			if (!_allowBundlingNow)
			{
				return PopulateExecutors::CantDoYet;
			}
			for_every(a, _jobListDefinition.bundleWithJobList)
			{
				bool present = false;
				for_every_ptr(executor, allExecutors)
				{
					if (executor->does_handle(*a))
					{
						present = true;
						break;
					}
				}
				if (_mainExecutor->does_handle(*a))
				{
					present = true;
				}
				if (!present)
				{
					// wait till it is there
					return PopulateExecutors::CantDoYet;
				}
			}
		}
		if (_jobListDefinition.avoidBeingWithJobList.is_empty())
		{
			if (!_allowAvoidanceNow)
			{
				return PopulateExecutors::CantDoYet;
			}
			for_every(a, _jobListDefinition.avoidBeingWithJobList)
			{
				bool present = false;
				for_every_ptr(executor, allExecutors)
				{
					if (executor->does_handle(*a))
					{
						present = true;
						break;
					}
				}
				if (_mainExecutor->does_handle(*a))
				{
					present = true;
				}
				if (!present)
				{
					// wait till it is there
					return PopulateExecutors::CantDoYet;
				}
			}
		}
	}

	// sort executors from least occupied to most occupied
	Array<JobSystemExecutor*> allExecutors;
	Array<JobSystemExecutor*> sortedExecutors;
	for (int chooseVariantIdx = 0; chooseVariantIdx < 3; ++chooseVariantIdx)
	{
		// create copy
		allExecutors.clear();
		for_every_ptr(executor, _executors)
		{
			allExecutors.push_back(executor);
		}

		while (!allExecutors.is_empty())
		{
			JobSystemExecutor* leastOccupiedExecutor = nullptr;
			for_every_ptr(executor, allExecutors)
			{
				if (!leastOccupiedExecutor || executor->get_list_count() < leastOccupiedExecutor->get_list_count())
				{
					leastOccupiedExecutor = executor;
				}
			}
			bool okToUse = true;
			if (chooseVariantIdx <= 0)
			{
				if (! _jobListDefinition.bundleWithJobList.is_empty())
				{
					okToUse = false;
					for_every(a, _jobListDefinition.bundleWithJobList)
					{
						if (leastOccupiedExecutor->does_handle(*a))
						{
							okToUse = true;
						}
					}
				}
			}
			if (chooseVariantIdx <= 1)
			{
				for_every(a, _jobListDefinition.avoidBeingWithJobList)
				{
					if (leastOccupiedExecutor->does_handle(*a))
					{
						okToUse = false;
					}
				}
			}
			if (okToUse)
			{
				sortedExecutors.push_back(leastOccupiedExecutor);
			}
			allExecutors.remove_fast(leastOccupiedExecutor);
		}

		if (!sortedExecutors.is_empty())
		{
			break;
		}
	}

	int jobsLeftPrev = _jobsLeft;

	// add jobs
	for_every_ptr(executor, sortedExecutors)
	{
		add_to_executor(executor, REF_ _jobsLeft, _jobListDefinition, _name, _priority, false);
	}

	return jobsLeftPrev == _jobsLeft? PopulateExecutors::NothingDone : PopulateExecutors::Handled;
}

void complain_about_jobs_left(int _jobsLeft, Name const & _name)
{
	if (_jobsLeft > 0)
	{
		if (_jobsLeft == 1)
		{
			error(TXT("there was one \"%S\" job left - and no information where it should go"), _name.to_char());
		}
		else
		{
			error(TXT("there were %i \"%S\" jobs left - and no information where they should go"), _jobsLeft, _name.to_char());
		}
	}
}

#define HANDLE_POPULATE_EXECUTORS_RESULT(result) \
	{ \
		PopulateExecutors::Result r = (result); \
		if (r == PopulateExecutors::CantDoYet) ok = false; \
		if (r == PopulateExecutors::Handled) doneSomething = true; \
	}

void Game::initialise_job_system()
{
	an_assert(jobSystem == nullptr);

	jobSystem = new JobSystem();

	::System::Core::on_quick_exit(TXT("terminate job system"), [this]() {terminate_job_system(); });

	// add all lists
	jobSystem->add_immediate_list(workerJobs);
	jobSystem->add_immediate_list(prepareRenderJobs);
	jobSystem->add_asynchronous_list(asyncWorkerJobs);
	jobSystem->add_asynchronous_list(loadingWorkerJobs);
	jobSystem->add_asynchronous_list(preparingWorkerJobs);
	jobSystem->add_asynchronous_list(gameJobs);
	jobSystem->add_asynchronous_list(backgroundJobs);
	jobSystem->add_asynchronous_list(gameBackgroundJobs);
	jobSystem->add_asynchronous_list(renderJobs);
	jobSystem->add_asynchronous_list(soundJobs);
	jobSystem->add_asynchronous_list(systemJobs);

	// leave some threads for the system
	output(TXT("all cores available: %i"), Concurrency::ThreadSystemUtils::get_number_of_cores(true));
	int cpuCount = Concurrency::ThreadSystemUtils::get_number_of_cores();
	int systemReservedThreads = MainConfig::global().get_system_reserved_threads();
	if (systemReservedThreads < 0)
	{
		systemReservedThreads = (cpuCount - 4) / 4;
	}
	int32 numberOfCores = max(1, min(Concurrency::ThreadSystemUtils::get_number_of_cores() - systemReservedThreads, MainConfig::global().get_thread_limit()));
	if (numberOfCores == 1)
	{
		runningSingleThreaded = true;
	}
	output(TXT("system reserved: %i"), systemReservedThreads);
	output(TXT("cores to be used: %i"), numberOfCores);

	if (runningSingleThreaded)
	{
		// add everything
		JobSystemExecutor* mainExecutor = jobSystem->create_executor();
		mainExecutor->add_list(systemJobs, 40);
		mainExecutor->add_list(prepareRenderJobs, 35);
		mainExecutor->add_list(renderJobs, 30);
		mainExecutor->add_list(soundJobs, 25);
		mainExecutor->add_list(gameJobs, 15);
		mainExecutor->add_list(backgroundJobs, 10);
		mainExecutor->add_list(gameBackgroundJobs, 10);
		mainExecutor->add_list(workerJobs, 0);
		mainExecutor->add_list(asyncWorkerJobs, 0);

		jobSystem->set_main_executor(mainExecutor);
	}
	else
	{
		JobSystemDefinition const & jobSystemDefinition = MainSettings::global().get_job_system_definition();

		float numberOfCoresFloat = (float)numberOfCores;
		
		// count how many jobs do we need
		jobSystemInfo.workerJobsCount = count_jobs(jobSystemDefinition.workerJobs, numberOfCoresFloat);
		jobSystemInfo.asyncWorkerJobsCount = count_jobs(jobSystemDefinition.asyncWorkerJobs, numberOfCoresFloat);
		jobSystemInfo.loadingWorkerJobsCount = count_jobs(jobSystemDefinition.loadingWorkerJobs, numberOfCoresFloat);
		jobSystemInfo.preparingWorkerJobsCount = count_jobs(jobSystemDefinition.preparingWorkerJobs, numberOfCoresFloat);
		jobSystemInfo.prepareRenderJobsCount = count_jobs(jobSystemDefinition.prepareRenderJobs, numberOfCoresFloat);
		jobSystemInfo.gameJobsCount = count_jobs(jobSystemDefinition.gameJobs, numberOfCoresFloat);
		jobSystemInfo.backgroundJobsCount = count_jobs(jobSystemDefinition.backgroundJobs, numberOfCoresFloat);
		jobSystemInfo.gameBackgroundJobsCount = count_jobs(jobSystemDefinition.gameBackgroundJobs, numberOfCoresFloat);
		jobSystemInfo.renderJobsCount = count_jobs(jobSystemDefinition.renderJobs, numberOfCoresFloat);
		jobSystemInfo.soundJobsCount = count_jobs(jobSystemDefinition.soundJobs, numberOfCoresFloat);
		jobSystemInfo.systemJobsCount = count_jobs(jobSystemDefinition.systemJobs, numberOfCoresFloat);

		jobSystemInfo.doSyncWorldJobsAsAsynchronousGameJobs = jobSystemDefinition.doSyncWorldJobsAsAsynchronousGameJobs;
		if (jobSystemInfo.doSyncWorldJobsAsAsynchronousGameJobs)
		{
			jobSystemInfo.gameJobsCount = 1;
		}

		// we should ideally run to zero with this
		GameJobSystemInfo populateJobs = jobSystemInfo;

		output(TXT("jobs:"));
		output(TXT(" game             : %3i"), populateJobs.gameJobsCount);
		output(TXT(" render           : %3i"), populateJobs.renderJobsCount);
		output(TXT(" system           : %3i"), populateJobs.systemJobsCount);
		output(TXT(" prepare render   : %3i"), populateJobs.prepareRenderJobsCount);
		output(TXT(" sound            : %3i"), populateJobs.soundJobsCount);
		output(TXT(" background       : %3i"), populateJobs.backgroundJobsCount);
		output(TXT(" game background  : %3i"), populateJobs.gameBackgroundJobsCount);
		output(TXT(" worker           : %3i"), populateJobs.workerJobsCount);
		output(TXT(" async worker     : %3i"), populateJobs.asyncWorkerJobsCount);
		output(TXT(" loading worker   : %3i"), populateJobs.loadingWorkerJobsCount);
		output(TXT(" preparing worker : %3i"), populateJobs.preparingWorkerJobsCount);

		// populate main executor, system and renderer go here - this is hardcoded assumption!
		JobSystemExecutor* mainExecutor = jobSystem->create_executor();
		add_to_executor(mainExecutor, REF_ populateJobs.systemJobsCount, jobSystemDefinition.systemJobs, systemJobs, 40, true);
		add_to_executor(mainExecutor, REF_ populateJobs.renderJobsCount, jobSystemDefinition.renderJobs, renderJobs, 30, true);

		// although it is among the rest, it is treated as "second thread"
		if (jobSystemDefinition.gameJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.gameJobsCount, jobSystemDefinition.gameJobs, gameJobs, 15, true);

		// others, if preferred, will be added now, if not, they will be threated as left overs and will be added later on
		if (jobSystemDefinition.prepareRenderJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.prepareRenderJobsCount, jobSystemDefinition.prepareRenderJobs, prepareRenderJobs, 30, true);
		if (jobSystemDefinition.soundJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.soundJobsCount, jobSystemDefinition.soundJobs, soundJobs, 25, true);
		if (jobSystemDefinition.backgroundJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.backgroundJobsCount, jobSystemDefinition.backgroundJobs, backgroundJobs, 10, true);
		if (jobSystemDefinition.gameBackgroundJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.gameBackgroundJobsCount, jobSystemDefinition.gameBackgroundJobs, gameBackgroundJobs, 10, true);
		if (jobSystemDefinition.workerJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.workerJobsCount, jobSystemDefinition.workerJobs, workerJobs, 0, true);
		if (jobSystemDefinition.asyncWorkerJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.asyncWorkerJobsCount, jobSystemDefinition.asyncWorkerJobs, asyncWorkerJobs, 0, true);
		if (jobSystemDefinition.loadingWorkerJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.loadingWorkerJobsCount, jobSystemDefinition.loadingWorkerJobs, loadingWorkerJobs, 0, true);
		if (jobSystemDefinition.preparingWorkerJobs.preferMainThread) add_to_executor(mainExecutor, REF_ populateJobs.preparingWorkerJobsCount, jobSystemDefinition.preparingWorkerJobs, preparingWorkerJobs, 0, true);

		if (populateJobs.systemJobsCount > 0)
		{
			error(TXT("there are no more threads that can handle system job!"));
		}

		if (populateJobs.renderJobsCount > 0)
		{
			error(TXT("there are no more threads that can handle renderer job!"));
		}

		// create executors for other jobs
		Array<JobSystemExecutor*> executors;
		executors.set_size(numberOfCores - 1);
		for_every(executor, executors)
		{
			*executor = jobSystem->create_executor();
		}

		Array<JobSystemExecutor*> extraExecutors;
		if (MainConfig::global().does_allow_extra_threads())
		{
			if (jobSystemDefinition.prepareRenderJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.prepareRenderJobsCount, prepareRenderJobs);
			if (jobSystemDefinition.soundJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.soundJobsCount, soundJobs);
			if (jobSystemDefinition.backgroundJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.backgroundJobsCount, backgroundJobs);
			if (jobSystemDefinition.gameBackgroundJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.gameBackgroundJobsCount, gameBackgroundJobs);
			if (jobSystemDefinition.gameJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.gameJobsCount, gameJobs);
			if (jobSystemDefinition.workerJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.workerJobsCount, workerJobs);
			if (jobSystemDefinition.asyncWorkerJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.asyncWorkerJobsCount, asyncWorkerJobs);
			if (jobSystemDefinition.loadingWorkerJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.loadingWorkerJobsCount, loadingWorkerJobs);
			if (jobSystemDefinition.preparingWorkerJobs.preferExtraSeparateThread) add_extra_executors(jobSystem, REF_ extraExecutors, REF_ populateJobs.preparingWorkerJobsCount, prepareRenderJobs);
		}

		// populate executors with jobs - if something was preferred to be on main thread, it was already added there
		{
			bool ok = false;
			int tryIdx = 0;
			// we try to add something in each pass but step by step, first populate without dependencies, then try populating bundled and only then avoid
			// we step up only if nothing has been done in a recent step
			bool bundleNow = false;
			bool avoidNow = false;
			while (! ok)
			{
				ok = true;
				bool doneSomething = false;
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.prepareRenderJobsCount, jobSystemDefinition.prepareRenderJobs, prepareRenderJobs, 30));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.soundJobsCount, jobSystemDefinition.soundJobs, soundJobs, 25));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.backgroundJobsCount, jobSystemDefinition.backgroundJobs, backgroundJobs, 10));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.gameBackgroundJobsCount, jobSystemDefinition.gameBackgroundJobs, gameBackgroundJobs, 10));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.gameJobsCount, jobSystemDefinition.gameJobs, gameJobs, 5));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.workerJobsCount, jobSystemDefinition.workerJobs, workerJobs, 0));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.asyncWorkerJobsCount, jobSystemDefinition.asyncWorkerJobs, asyncWorkerJobs, 0));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.loadingWorkerJobsCount, jobSystemDefinition.loadingWorkerJobs, loadingWorkerJobs, 0));
				HANDLE_POPULATE_EXECUTORS_RESULT(populate_executors(bundleNow, avoidNow, mainExecutor, executors, REF_ populateJobs.preparingWorkerJobsCount, jobSystemDefinition.preparingWorkerJobs, preparingWorkerJobs, 0));
				if (!doneSomething)
				{
					if (!bundleNow) bundleNow = true;
					else if (!avoidNow) avoidNow = true;
				}
				if (tryIdx > 100)
				{
					error_stop(TXT("unable to populate executors, circular or invalid dependency"));
				}
				++tryIdx;
			}
		}

		// add left overs to main executor - if they were not preferred to be on main thread but there are so many jobs we have to use them
		add_to_executor(mainExecutor, REF_ populateJobs.prepareRenderJobsCount, jobSystemDefinition.prepareRenderJobs, prepareRenderJobs, 30, true);
		add_to_executor(mainExecutor, REF_ populateJobs.soundJobsCount, jobSystemDefinition.soundJobs, soundJobs, 25, true);
		add_to_executor(mainExecutor, REF_ populateJobs.backgroundJobsCount, jobSystemDefinition.backgroundJobs, backgroundJobs, 10, true);
		add_to_executor(mainExecutor, REF_ populateJobs.gameBackgroundJobsCount, jobSystemDefinition.gameBackgroundJobs, gameBackgroundJobs, 10, true);
		add_to_executor(mainExecutor, REF_ populateJobs.gameJobsCount, jobSystemDefinition.gameJobs, gameJobs, 5, true);
		add_to_executor(mainExecutor, REF_ populateJobs.workerJobsCount, jobSystemDefinition.workerJobs, workerJobs, 0, true);
		add_to_executor(mainExecutor, REF_ populateJobs.asyncWorkerJobsCount, jobSystemDefinition.asyncWorkerJobs, asyncWorkerJobs, 0, true);
		add_to_executor(mainExecutor, REF_ populateJobs.loadingWorkerJobsCount, jobSystemDefinition.loadingWorkerJobs, loadingWorkerJobs, 0, true);
		add_to_executor(mainExecutor, REF_ populateJobs.preparingWorkerJobsCount, jobSystemDefinition.preparingWorkerJobs, preparingWorkerJobs, 0, true);

		{
			LogInfoContext lic;
			{
				LOG_INDENT(lic);
				lic.log(TXT("main thread"));
				{
					LOG_INDENT(lic);
					mainExecutor->log(lic);
				}
			}
			lic.output_to_output();
		}

		// setup executors within threads
		jobSystem->set_main_executor(mainExecutor);
		output(TXT("create threads"));
		for_every_ptr(executor, executors)
		{
			LogInfoContext lic;
			{
				LOG_INDENT(lic);
				lic.log(TXT("create thread #%i"), for_everys_index(executor));
				{
					LOG_INDENT(lic);
					executor->log(lic);
				}
				Optional<ThreadPriority::Type> threadPriority;
				if (executor->does_handle(gameJobs))
				{
					threadPriority = MainConfig::global().get_main_thread_priority(); // behave as main
					lic.log(TXT(" should have the same priority as main thread (game jobs)"));
				}
				jobSystem->create_thread(executor, threadPriority);
			}
			lic.output_to_output();
		}

		// extra threads have lowest priorities
		for_every_ptr(executor, extraExecutors)
		{
			output(TXT(" create extra thread #%i"), for_everys_index(executor));
			jobSystem->create_thread(executor, ThreadPriority::Lowest);
		}

		jobSystem->wait_for_all_threads_to_be_up();
		output(TXT("all threads created"));

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		jobSystem->start_lock_checker();
		output(TXT("lock checker started"));
#endif

		// check jobs left and complain if there are some left
		complain_about_jobs_left(populateJobs.systemJobsCount, systemJobs);
		complain_about_jobs_left(populateJobs.renderJobsCount, renderJobs);
		complain_about_jobs_left(populateJobs.soundJobsCount, soundJobs);
		complain_about_jobs_left(populateJobs.prepareRenderJobsCount, prepareRenderJobs);
		complain_about_jobs_left(populateJobs.backgroundJobsCount, backgroundJobs);
		complain_about_jobs_left(populateJobs.workerJobsCount, workerJobs);
		complain_about_jobs_left(populateJobs.asyncWorkerJobsCount, asyncWorkerJobs);
		complain_about_jobs_left(populateJobs.loadingWorkerJobsCount, loadingWorkerJobs);
		complain_about_jobs_left(populateJobs.preparingWorkerJobsCount, preparingWorkerJobs);
		complain_about_jobs_left(populateJobs.gameJobsCount, gameJobs);
	}

	worldManager.setup_after_system_job_is_active();

	{
		// this means that only the registered threads here get to work if activation group stacks
		int threadCount = ::Concurrency::ThreadManager::get_thread_count();
		activationGroupStacks.set_size(threadCount);
	}
}

void Game::close_job_system()
{
	::System::Core::on_quick_exit_no_longer(TXT("terminate job system"));
	output_colour_system();
	output(TXT("closing job system..."));
	output_colour();
	jobSystem->end();
	jobSystem->wait_for_all_threads_to_be_down();
	delete_and_clear(jobSystem);
}

void Game::terminate_job_system()
{
	output_colour_system();
	output(TXT("terminating job system..."));
	output_colour();
	if (jobSystem)
	{
		jobSystem->end();
		::System::Core::sleep(0.1f); // sleep a bit to allow threads ending on their own
		delete_and_clear(jobSystem);
	}
	output(TXT("terminated job system..."));
}

void Game::close_system()
{
	delete_and_clear(postProcessRenderTargetManager);

	// release main render buffer
	mainRenderBuffer = nullptr;
	mainRenderOutputRenderBuffer = nullptr;
	secondaryViewRenderBuffer = nullptr;
	secondaryViewOutputRenderBuffer = nullptr;

	// close VR
	output_colour_system();
	output(TXT("closing VR..."));
	output_colour();
	VR::IVR::terminate();

	output_colour_system();
	output(TXT("closing systems..."));
	output_colour();
	::System::Core::sleep(0.25f);
	::System::Core::terminate();
}

void Game::before_game_starts()
{
	output(TXT("init poi manager"));
	delete_and_clear(poiManager);
	poiManager = new POIManager();

	if (auto* gc = GameConfig::get())
	{
		gc->prepare_on_game_start();
	}

	// create post process chain
	postProcessChain = new PostProcessChain(postProcessRenderTargetManager);
	postProcessChain->define_output(PostProcessPipeline::get_default_output_definition());

	add_sync_world_job(TXT("will use shaders"), []() { DisplayPipeline::will_use_shaders(); });

#ifdef AN_TWEAKS
	checkReloadTweaks.allow();
#endif
#ifdef AN_ALLOW_BULLSHOTS
	checkReloadBullshots.allow();
#endif

#ifdef AN_ALLOW_BULLSHOTS
	if (BullshotSystem::is_setting_active(NAME(bsNoDevelopmentInput)))
	{
		reload_input_config_files();
	}
#endif

	update_use_sliding_locomotion();

	output(TXT("get icons"));
	gameIcons.savingIcon.icon = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(Name(String(TXT("saving icon"))), true);
	gameIcons.inputMouseKeyboardIcon.icon = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(Name(String(TXT("input mouse keyboard icon"))), true);
	gameIcons.inputGamepadIcon.icon = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(Name(String(TXT("input gamepad icon"))), true);
	gameIcons.screenshotIcon.icon = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(Name(String(TXT("screenshot icon"))), true);
}

void Game::after_game_ended()
{
	gameIcons.savingIcon.icon.clear();
	gameIcons.inputMouseKeyboardIcon.icon.clear();
	gameIcons.inputGamepadIcon.icon.clear();
	gameIcons.screenshotIcon.icon.clear();

	delete_and_clear(poiManager);

	// disallow later loading, otherwise we may try to load more
	allowLaterLoading = false;

	// we should not save game post this point as we start to unload stuff
	::System::Core::on_quick_exit_no_longer(TXT("save game config"));

	delete_and_clear(postProcessChain);
	DisplayPipeline::release_shaders();
}

void Game::do_sound(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	scoped_call_stack_info(TXT("game::do_sound"));
	MEASURE_PERFORMANCE_COLOURED(doSound, Colour::c64Orange);
	Game* game = (Game*)_data;
#ifdef AN_USE_FRAME_INFO
	System::ScopedTimeStamp ts(game->currentFrameInfo->threadSound);
#endif
	game->soundJobStatus = GameJob::InProgress;
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(game->currentFrameInfo->soundTime);
#endif
		MEASURE_PERFORMANCE_COLOURED(soundScene, Colour::whiteCold);
		((Game*)_data)->sound();
	}
	game->soundJobStatus = GameJob::Done;
#ifdef GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
	game->advanceSynchronisationLounge.wake_up_all();
#endif
}

void Game::do_render(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	scoped_call_stack_info(TXT("game::do_render"));
	MEASURE_PERFORMANCE_COLOURED(doRender, Colour::c64Violet);
	Game* game = (Game*)_data;
#ifdef AN_USE_FRAME_INFO
	System::ScopedTimeStamp ts(game->currentFrameInfo->threadRender);
#endif
	game->renderJobStatus = GameJob::InProgress;
	if (auto* vr = VR::IVR::get())
	{
		vr->I_am_perf_thread_render();
	}
	MEASURE_PERFORMANCE_FINISH_RENDERING();
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(game->currentFrameInfo->renderTime);
#endif
		MEASURE_PERFORMANCE_COLOURED(gameRender, Colour::whiteCold);
		((Game*)_data)->render();
 	}
	MEASURE_PERFORMANCE_FINISH_RENDERING();
	{
		MEASURE_PERFORMANCE_COLOURED(debugRendererRender, Colour::c64Cyan);
		todo_note(TXT("bind render target?"));
		debug_renderer_render(::System::Video3D::get());
	}
	{
		MEASURE_PERFORMANCE_COLOURED(renderScene_flush, Colour::blackCold);
		//glFlush(); // send everything to graphic card (before waiting for other threads)
	}
	game->renderJobStatus = GameJob::Done;
#ifdef GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
	game->advanceSynchronisationLounge.wake_up_all();
#endif
}

void Game::do_game_loop(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	Game* game = (Game*)_data;
#ifdef AN_USE_FRAME_INFO
	System::ScopedTimeStamp tsg(game->currentFrameInfo->threadSoundGameLoop);
#endif
	if (MainSettings::global().should_do_sound_on_game_thread())
	{
		do_sound(_executionContext, _data);
	}
#ifdef AN_USE_FRAME_INFO
	System::ScopedTimeStamp ts(game->currentFrameInfo->threadGameLoop);
#endif
	// sync vr frame
	if (VR::IVR::can_be_used())
	{
		MEASURE_PERFORMANCE(vrGameSync);
		game->do_vr_game_sync();
	}
	{
		// update delta time with the latest result (maybe has changed?)
		game->deltaTime = ::System::Core::get_delta_time();
		game->jobSystem->access_advance_context().set_delta_time(game->deltaTime);
	}
	{
		scoped_call_stack_info(TXT("game::do_game_loop"));
		MEASURE_PERFORMANCE_COLOURED(doGameLoop, Colour::c64Violet);
		an_assert(!game->is_on_short_pause(), TXT("game should be not advanced while on short pause"));
		game->gameLoopJobStatus = GameJob::InProgress;
		if (auto* vr = VR::IVR::get())
		{
			vr->I_am_perf_thread_main();
		}
		{
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(game->currentFrameInfo->gameLoopTime);
#endif
			((Game*)_data)->game_loop();
		}
		game->gameLoopJobStatus = GameJob::Done;
#ifdef GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
		game->advanceSynchronisationLounge.wake_up_all();
#endif
	}
}

#ifdef AN_USE_FRAME_INFO
void Game::next_frame_info()
{
	spentOnDebugTime = 0.0f;
	swap(currentFrameInfo, prevFrameInfo);
	// next frame, this way we will skip logging time
	currentFrameInfo->clear();
	++initialFrameHack;
}
#endif

void Game::request_initial_world_advance_required()
{
	add_sync_world_job(TXT("initial advance required"), [this]() {initialWorldAdvanceRequired = true; });
}

void Game::initial_world_advance()
{
	if (!world || !initialWorldAdvanceRequired)
	{
		return;
	}

	MEASURE_PERFORMANCE(initialAdvance);

	// build presence links
	world->advance_build_presence_links(get_job_system(), worker_jobs(), subWorld);

	// finalise frame, ready for rendering, handle sounds etc
	world->advance_finalise_frame(get_job_system(), worker_jobs(), subWorld);

	// mark advanced
	initialWorldAdvanceRequired = false;
}

void Game::advance_world(Optional<float> _withDeltaTimeOverride)
{
	MEASURE_PERFORMANCE(advanceWorld);

#ifdef AN_DEVELOPMENT
	if (world)
	{
		world->dev_check_if_ok();
	}
#endif

#ifdef AN_USE_FRAME_INFO
	System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvance);
#endif

	an_assert(worldInUse);

	Framework::AdvanceContext& ac = get_job_system()->access_advance_context();
	STORE_IN_SCOPE(Framework::AdvanceContext, ac);

	if (_withDeltaTimeOverride.is_set())
	{
		ac.set_delta_time(_withDeltaTimeOverride.get());
	}

	// do it pre world so we may get extra time and maybe we won't have to wait for block to end
	if (subWorld)
	{
		subWorld->manage_inactive_temporary_objects();
	}

	if (world && ac.get_delta_time() != 0.0f)
	{
		WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);
		advance_world__during_block();
		advance_world__post_block();
		advancedWorldLastFrame = true;
	}
	else
	{
		WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);
		skip_advancing_world();
	}

#ifdef AN_DEVELOPMENT
	if (world)
	{
		world->dev_check_if_ok();
	}
#endif
}

void Game::advance_world__during_block()
{
#ifdef AN_USE_FRAME_INFO
	System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvanceDuringBlock);
#endif

	if (! world)
	{
		return;
	}

	MEASURE_PERFORMANCE(advanceWorldDuringPreRender);

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvanceDuringBlock_readyToAdvance);
#endif
		// begin advancing
		world->ready_to_advance(subWorld);
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvanceDuringBlock_advanceLogic);
#endif
		// do non physical advancement of world
		world->advance_logic(get_job_system(), worker_jobs(), subWorld);
	}

	// wait for render and sound to unblock (it could have already ended, then we will skip it)
	renderBlockGate.wait();
	soundBlockGate.wait();
}

void Game::advance_world__post_block()
{
#ifdef AN_USE_FRAME_INFO
	System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvancePostBlock);
#endif
	if (!world)
	{
		return;
	}

	MEASURE_PERFORMANCE(advanceWorldPostBlock);

	an_assert(subWorld, TXT("you need to set sub world for the game!"));

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvancePostBlock_advancePhysically);
#endif
		// advance world physically
		world->advance_physically(get_job_system(), worker_jobs(), subWorld);
	}

#ifdef BREAK_BUILD_PRESENCE_LINKS
	// build presence links for normal objects only
	world->advance_build_presence_links(get_job_system(), worker_jobs(), subWorld, Framework::BuildPresenceLinks::Objects);
#endif

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvancePostBlock_advanceTemporaryObjects);
#endif
		// advance temporary objects, activate them, advance (collide using presence links!), deactivate
		world->advance_temporary_objects(get_job_system(), worker_jobs(), subWorld);
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvancePostBlock_buildPresenceLinks);
#endif
#ifdef BREAK_BUILD_PRESENCE_LINKS
		// build presence links for temporary objects
		world->advance_build_presence_links(get_job_system(), worker_jobs(), subWorld, Framework::BuildPresenceLinks::Add_TemporaryObjects);
#else
		// build all presence links - we will be one frame off for temporary objects advance because of that, but that's acceptable
		world->advance_build_presence_links(get_job_system(), worker_jobs(), subWorld);
#endif
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvancePostBlock_redistributePresenceLinks);
#endif
		MEASURE_PERFORMANCE_COLOURED(redistributePresenceLinks, Colour::blue);
		PERFORMANCE_GUARD_LIMIT(0.0005f, TXT("redistribute presence links"));
		SoftPoolMT<Framework::PresenceLink>::redistribute();
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvancePostBlock_advanceFinaliseFrame);
#endif
		// finalise frame, ready for rendering, handle sounds etc
		world->advance_finalise_frame(get_job_system(), worker_jobs(), subWorld);
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->glWorldAdvancePostBlock_finaliseAdvance);
#endif
		// finalise advance
		world->finalise_advance(subWorld);
	}

	//
}

void Game::skip_advancing_world()
{
	// just do the semaphore thing
	renderBlockGate.wait();
	soundBlockGate.wait();
}

void Game::ready_for_continuous_advancement()
{
	if (!Concurrency::ThreadManager::get() || Concurrency::ThreadManager::get_current_thread_id() == 0)
	{
		::System::Core::set_time_multiplier(); 
		::System::Core::advance();
	}
	deltaTime = 0.0f;
	// no need to reset immobile vr origin as we're showing loader and it should be handled by other means
	inContinuous.reset();
#ifdef FPS_INFO
	fpsTime = 0.0f;
	fpsCount = 0.0f;
	fpsImmediate = 0.0f;
	fpsAveraged = 0.0f;
	fpsLastSecond = 1.0f;
	fpsFrameTime = 0.0f;
	fpsIgnoreFirstSecond = true;
	fpsRange = Range::empty;
	fpsRange5 = Range::empty;
	fpsRange5show = Range::empty;
	fpsRange5Seconds = 0;
	fpsTotalFrames = 0.0f;
	fpsTotalTime = 0.0f;
#endif
#ifdef AN_USE_FRAME_INFO
	// double to clear current and prev
	next_frame_info();
	next_frame_info();
	frameStart.reset();
	initialFrameHack = 0;
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
	NameStorage::set_should_log_creating_names(true);
#endif
}

// allow 50% of frame, because... it happens?
#define LOW_HITCH_MULTIPLIER 1.5f

void Game::advance()
{
	scoped_call_stack_info(TXT("game::advance"));

	PERFORMANCE_MARKER(Colour::white);
	
	inContinuous.time += (double)::System::Core::get_delta_time();

	{
#ifdef AN_USE_FRAME_INFO
		currentFrameInfo->frameTime = frameStart.get_time_since();
		currentFrameInfo->deltaTime = ::System::Core::get_delta_time();
		currentFrameInfo->end_frame();

		PERFORMANCE_MARKER(Colour::orange);

		// no point to log while in debug mode
#ifndef AN_DEVELOPMENT
#ifdef AN_PERFORMANCE_MEASURE
		blockAutoStopFor = max(0.0f, blockAutoStopFor - ::System::Core::get_raw_delta_time());
#endif

#ifdef AN_USE_FRAME_INFO
		{
			ADDITIONAL_PERFORMANCE_INFO(TXT("50 render 0 - draw calls"),		String::printf(TXT("draw calls       : %6i"), currentFrameInfo->systemInfo.get_draw_calls()));
			ADDITIONAL_PERFORMANCE_INFO(TXT("50 render 1 - triangles"),			String::printf(TXT("draw triangles   : %6i"), currentFrameInfo->systemInfo.get_draw_triangles()));
			ADDITIONAL_PERFORMANCE_INFO(TXT("50 render 2 - rt - draw calls"),	String::printf(TXT("rt | draw calls  : %6i"), currentFrameInfo->systemInfo.get_draw_calls_render_targets()));
			ADDITIONAL_PERFORMANCE_INFO(TXT("50 render 3 - rt - other calls"),	String::printf(TXT("rt | other calls : %6i"), currentFrameInfo->systemInfo.get_other_calls_render_targets()));
			ADDITIONAL_PERFORMANCE_INFO(TXT("50 render 4 - direct gl calls"),	String::printf(TXT("direct gl calls  : %6i"), currentFrameInfo->systemInfo.get_direct_gl_calls()));
		}
#endif

#ifdef AN_PERFORMANCE_MEASURE
		if (!showPerformance)
#endif
		{
			PERFORMANCE_MARKER(Colour::blue);

			float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
			if (expectedFrameTime == 0.0f)
			{
				// something went really wrong...
				expectedFrameTime = 1.0f / 30.0f;
			}

#ifdef AN_PERFORMANCE_MEASURE
#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH
			if (auto* input = ::System::Input::get())
			{
				input->set_usage_tag(NAME(allowSavingFramePerformanceInfo));
			}
#endif
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (fast_cast<PreviewGame>(this) == nullptr)
#endif
			if (expectedFrameTime > 0.0f && initialFrameHack > 5 && !ignoreLongerFrame)
			{
				float frameTime = currentFrameInfo->frameTime - spentOnDebugTime;
				if (does_use_vr() && VR::IVR::get()->is_game_sync_blocking())
				{
					frameTime = currentFrameInfo->gameSyncFrameTime - spentOnDebugTime;
				}
				float const lowHitch = expectedFrameTime * LOW_HITCH_MULTIPLIER;
				float const highHitch = 0.5f;
				bool logHitchDetailsNow = false;
				bool logAndForceToStopNow = false;

				if (frameTime > highHitch)
				{
					error(TXT("SOMETHING WENT REALLY BAD %.3fms -> %.3fms (e:%.3fms)"), currentFrameInfo->frameTime * 1000.0f, (frameTime) * 1000.0f, expectedFrameTime * 1000.0f);
#ifdef AN_PERFORMANCE_MEASURE
					if (allowToStopToShowPerformanceOnHighHitch && blockAutoStopFor <= 0.0f)
					{
						logAndForceToStopNow = true;
					}
#endif
				}
				bool exceededLowHitch = frameTime > lowHitch;
#ifdef PROFILE_PERFORMANCE_CAPTURE_EVERY
				exceededLowHitch = mod(currentFrameInfo->frameNo, PROFILE_PERFORMANCE_CAPTURE_EVERY) == 0;
#endif
#ifdef AN_PERFORMANCE_MEASURE
#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH
				if (allTimeControlsInput.has_button_been_pressed(NAME(saveFramePerformanceInfo)))
				{
					exceededLowHitch = true;
				}
#endif
#endif
				if (exceededLowHitch)
				{
					++hitchFrameCount;
					++inContinuous.hitchFrameCount;
					bool glHitch = false;
					{
						float frameTimeBudgetGiven = expectedFrameTime - 0.003f;
						glHitch = (prevFrameInfo->gameLoopTime > frameTimeBudgetGiven ||
							currentFrameInfo->gameLoopTime > frameTimeBudgetGiven);
						if (glHitch)
						{
							++gameLoopHitchFrameCount;
						}
					}
#ifdef AN_PERFORMANCE_MEASURE
#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH
					bool store = true;
					{
#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH_GAME_ONLY
						store = glHitch;
						/*
						if (store)
						{
							//output(TXT("!@# would store game loop hitch %.3f v %.3f > %.3f [%.3f]"), prevFrameInfo->gameLoopTime * 1000.0f, currentFrameInfo->gameLoopTime * 1000.0f, frameTimeBudgetGiven * 1000.0f, expectedFrameTime * 1000.0f);
							store = false;
						}
						*/
#endif
					}
#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH_IGNORE_RENDER
					if (prevFrameInfo->lastRenderToRenderTime > expectedFrameTime * 1.5f)
					{
						store = false;
					}
#endif
#ifdef AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH_LONG_ONLY
					if (frameTime <= expectedFrameTime * 2.5f)
					{
						store = false;
					}
#endif
					if (allTimeControlsInput.has_button_been_pressed(NAME(saveFramePerformanceInfo)))
					{
						store = true;
					}
					if (store)
					{
						if (! delayedStorePerformanceFrameInfo.is_set())
						{
							ADDITIONAL_PERFORMANCE_INFO(TXT("90 frame info - use frame time"),			String::printf(TXT("use frame time         : %.3fms"), 1000.0f * frameTime));
							ADDITIONAL_PERFORMANCE_INFO(TXT("90 frame info - frame time"),				String::printf(TXT("frame time             : %.3fms"), 1000.0f * currentFrameInfo->frameTime));
							ADDITIONAL_PERFORMANCE_INFO(TXT("90 frame info - game sync frame time"),	String::printf(TXT("game sync frame time   : %.3fms"), 1000.0f * currentFrameInfo->gameSyncFrameTime));
							ADDITIONAL_PERFORMANCE_INFO(TXT("90 frame info - expected frame time"),		String::printf(TXT("expected frame time    : %.3fms"), 1000.0f * expectedFrameTime));
							ADDITIONAL_PERFORMANCE_INFO(TXT("90 frame info - render to render"),		String::printf(TXT("render to render time  : %.3fms"), 1000.0f * currentFrameInfo->lastRenderToRenderTime));
							delayedStorePerformanceFrameInfoDelayUsed = 0;
							if (::Performance::System::get_max_prev_frames() > 0)
							{
								// wait at least one, prefer to save more old frames
								delayedStorePerformanceFrameInfoDelayUsed = max(1, (::Performance::System::get_max_prev_frames()) * 2 / 5);
							}
							delayedStorePerformanceFrameInfo = delayedStorePerformanceFrameInfoDelayUsed;
							delayedStorePerformanceFrameInfoSuffix = String::printf(TXT("%.3fms"), frameTime * 1000.0f);
							output(TXT("[fpi] delay saving performance frame info (%i)"), delayedStorePerformanceFrameInfoDelayUsed);
						}
					}
#endif
#endif
					if (VR::IVR::can_be_used())
					{
#ifdef AN_PERFORMANCE_MEASURE
						if (allowToStopToShowPerformanceOnAnyHitch && blockAutoStopFor <= 0.0f)
						{
							logAndForceToStopNow = true;
						}
						else
#endif
						{
#ifdef AN_VIVE
							//if (false)
#endif
							{
								LogInfoContext log;
#ifdef SHORT_HITCH_FRAME_INFO
								log.set_colour(Colour::zxYellowBright);
								log.log(TXT("[hitch] HITCH FRAME TIME [%i] %.3fms/%.3fms -> %.3fms (e:%.3fms) : %S"),
									/*          */ hitchFrameCount,
									/*          */ currentFrameInfo->frameTime * 1000.0f, currentFrameInfo->gameSyncFrameTime * 1000.0f, (frameTime) * 1000.0f, expectedFrameTime * 1000.0f,
									/*          */ get_additional_hitch_info()
								);
								log.set_colour();
								{
									LOG_INDENT(log);
									float fpsFromHitches;
									float timeTotal = ::System::Core::get_time_since_core_started();
									struct CalculateFPS
									{
										static float calc(float expectedFrameTime, float timeTotal, int hitchFrameCount)
										{
											int fpsFull = TypeConversions::Normal::f_i_closest(1.0f / expectedFrameTime);
											float framesTotal = (float)fpsFull * timeTotal;
											float framesWithoutHitches = framesTotal - (float)hitchFrameCount;
											return framesWithoutHitches / timeTotal;
										}
									};
									fpsFromHitches = CalculateFPS::calc(expectedFrameTime, timeTotal, hitchFrameCount);
									log.log(TXT("hitches per minute: %.2f -> fps :%.2f"),
										(float)hitchFrameCount / max(0.01f, timeTotal / 60.0f), fpsFromHitches);
									log.log(TXT("in current continuous: hf:%i in %.1fmin -> per min %.2f -> fps :%.2f"),
										inContinuous.hitchFrameCount, (float)inContinuous.time / 60.0f,
										(float)inContinuous.hitchFrameCount / max(0.01f, (float)inContinuous.time / 60.0f),
										CalculateFPS::calc(expectedFrameTime, (float)inContinuous.time, inContinuous.hitchFrameCount));
#ifdef FPS_INFO
									log.log(TXT("frames per second: averaged:%.2f (range: %.2f->%.2f, 5s range: %.2f->%.2f)"), fpsAveraged, fpsRange.min, fpsRange.max, fpsRange5show.min, fpsRange5show.max);
#endif
									for_count(int, iFrame, 2)
									{
										auto* frameInfo = iFrame == 0 ? prevFrameInfo : currentFrameInfo;
										if (MainSettings::global().should_do_sound_on_game_thread())
										{
											log.log(TXT("%S threads: pre:%3fms -> mt:%3fms (r:%.3fms, s+gl:%.3fms (s:%.3fms + gl:%.3fms)) -> post:%3fms"),
												iFrame == 0 ? TXT("PR") : TXT("CR"),
												/* pre t    */ (frameInfo->preThreads) * 1000.0f,
												/* mt/thrds */ (frameInfo->threads) * 1000.0f,
												/* render   */ (frameInfo->threadRender) * 1000.0f,
												/* snd+gmlp */ (frameInfo->threadSoundGameLoop) * 1000.0f,
												/* sound    */ (frameInfo->threadSound) * 1000.0f,
												/* gameloop */ (frameInfo->threadGameLoop) * 1000.0f,
												/* post t   */ (frameInfo->postThreads) * 1000.0f
											);
										}
										else
										{
											log.log(TXT("%S threads: pre:%3fms -> mt:%3fms (r:%.3fms, s:%.3fms, gl:%.3fms) -> post:%3fms"),
												iFrame == 0 ? TXT("PR") : TXT("CR"),
												/* pre t    */ (frameInfo->preThreads) * 1000.0f,
												/* mt/thrds */ (frameInfo->threads) * 1000.0f,
												/* render   */ (frameInfo->threadRender) * 1000.0f,
												/* sound    */ (frameInfo->threadSound) * 1000.0f,
												/* gameloop */ (frameInfo->threadGameLoop) * 1000.0f,
												/* post t   */ (frameInfo->postThreads) * 1000.0f
											);
										}
										log.log(TXT("   r    rend:%.3fms(%.3fms+%.3fms+%.3fms(e&d:%.3fms+br:%.3fms)+%.3fms+%.3fms), imjobs:%.3fms"),
											/* rend     render time */ (frameInfo->renderTime) * 1000.0f,
											/* part1    */ (frameInfo->renderTime_part1) * 1000.0f,
											/* part2    */ (frameInfo->renderTime_part2) * 1000.0f,
											/* part3    */ (frameInfo->renderTime_part3) * 1000.0f,
											/* p3 e&d   */ (frameInfo->duringRenderPart3_endAndDisplay) * 1000.0f,
											/* p3 br    */ (frameInfo->duringRenderPart3_beginRender) * 1000.0f,
											/* part4    */ (frameInfo->renderTime_part4) * 1000.0f,
											/* part5    */ (frameInfo->renderTime_part5) * 1000.0f,
											/* imjobs   */ (frameInfo->immediateJobsPostRenderTime) * 1000.0f
										);
										log.log(TXT("        +- (r->r:%.3fms),e->sr:%.3fms,prer:%.3fms,actr:%.3fms,e->dj:%.3fms,dj(->pe):%.3fms,e->pe:%.3fms,pe(->e):%.3fms"),
											/* (r->r)   */ (frameInfo->lastRenderToRenderTime) * 1000.0f,
											/* e->sr    */ (frameInfo->lastRenderToStartRenderTime) * 1000.0f,
											/* prer     */ (frameInfo->prepareToRender) * 1000.0f,
											/* actr     */ (frameInfo->actualRenderTime) * 1000.0f,
											/* e->dj    */ (frameInfo->lastRenderToDoJobsOnRenderTime) * 1000.0f,
											/* dj(->pe) */ (frameInfo->lastRenderToPreEndRenderTime - frameInfo->lastRenderToDoJobsOnRenderTime) * 1000.0f,
											/* e->pe    */ (frameInfo->lastRenderToPreEndRenderTime) * 1000.0f,
											/* pe(->e)  */ (frameInfo->lastRenderToRenderTime - frameInfo->lastRenderToPreEndRenderTime) * 1000.0f
										);
										log.log(TXT("   gl   gsync:%.3fms  +  gamlop:%.3fms(db:%.3fms(r2a:%.3fms+adv:%.3fms) + pb:%.3fms)"),
											/* gsync    */ (frameInfo->gameSync) * 1000.0f,
											/* gamlop   */ (frameInfo->gameLoopTime) * 1000.0f,
											/* +wadb    */ (frameInfo->glWorldAdvanceDuringBlock) * 1000.0f,
											/*  +r2adv  */ (frameInfo->glWorldAdvanceDuringBlock_readyToAdvance) * 1000.0f,
											/*  +advlog */ (frameInfo->glWorldAdvanceDuringBlock_advanceLogic) * 1000.0f,
											/* +wapb    */ (frameInfo->glWorldAdvancePostBlock) * 1000.0f
										);
										log.log(TXT("   post preadv:%.3fms, nav:%.3fms, wm-sync:%.3fms"),
											/* preadv   */ (frameInfo->preAdvanceTime) * 1000.0f,
											/* nav      */ (frameInfo->navSystemAdvanceTime) * 1000.0f,
											/* wm-sync  */ (frameInfo->worldManagerSyncAdvanceTime) * 1000.0f
										);
									}
									if (auto* vr = VR::IVR::get())
									{
										vr->log_perf_threads(log);
									}
								}
								log.output_to_output();
							}
#ifdef SHORT_HITCH_FRAME_INFO_DETAILS
							{
								LogInfoContext log;
								currentFrameInfo->log(log, false, false);
								log.output_to_output();
							}
#endif
#else // not SHORT_HITCH_FRAME_INFO
#ifdef AN_RENDERER_STATS
							Framework::Render::Stats const & stats = Framework::Render::Stats::get();
							warn(TXT("HITCH FRAME TIME [%i] %.3fms -> %.3fms (e:%.3fms) (tris:%i dc:%i r-r:%i r-p:%i) : %S"), hitchFrameCount, currentFrameInfo->frameTime * 1000.0f, (frameTime) * 1000.0f, expectedFrameTime * 1000.0f,
								prevFrameInfo->systemInfo.get_draw_triangles(), prevFrameInfo->systemInfo.get_draw_calls(),
								stats.renderedRoomProxies.get(), stats.renderedPresenceLinkProxies.get(),
								get_additional_hitch_info()
								);
#else // not AN_RENDERER_STATS
							warn(TXT("HITCH FRAME TIME [%i] %.3fms -> %.3fms (e:%.3fms) (tris:%i dc:%i) : %S"), hitchFrameCount, currentFrameInfo->frameTime * 1000.0f, (frameTime) * 1000.0f, expectedFrameTime * 1000.0f,
								prevFrameInfo->systemInfo.get_draw_triangles(), prevFrameInfo->systemInfo.get_draw_calls(),
								get_additional_hitch_info()
							);
#endif
#endif
#ifdef RENDER_HITCH_FRAME_INDICATOR
							{
								hitchFrameIndicators.hitchFrame.reset();
								if (expectedFrameTime < max(prevFrameInfo->renderTime, currentFrameInfo->renderTime))
								{
									hitchFrameIndicators.renderThread.reset();
								}
								if (expectedFrameTime < max(prevFrameInfo->lastRenderToPreEndRenderTime, currentFrameInfo->lastRenderToPreEndRenderTime))
								{
									hitchFrameIndicators.renderEndToPreEnd.reset();
								}
								if (expectedFrameTime < max(prevFrameInfo->lastRenderToRenderTime - prevFrameInfo->lastRenderToPreEndRenderTime, currentFrameInfo->lastRenderToRenderTime - currentFrameInfo->lastRenderToPreEndRenderTime))
								{
									hitchFrameIndicators.renderPreEndToEnd.reset();
								}
								if (expectedFrameTime < max(prevFrameInfo->gameLoopTime, currentFrameInfo->gameLoopTime))
								{
									hitchFrameIndicators.gameLoop.reset();
								}
							}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	//#ifndef AN_CLANG
							if (!logDeltaTimes)
							{
#ifdef AN_PERFORMANCE_MEASURE
								logHitchDetailsNow = logHitchDetails;
#else
								logHitchDetailsNow = true;
#endif
							}
							logDeltaTimes = 3;
							warnLogDeltaTime = true;
	//#endif
#else
							logHitchDetailsNow = true;
#endif
						}
					}
					else if (!logDeltaTimes)
					{
#ifdef AN_PERFORMANCE_MEASURE
						logHitchDetailsNow = logHitchDetails;
						if (allowToStopToShowPerformanceOnAnyHitch && blockAutoStopFor <= 0.0f)
						{
							logAndForceToStopNow = true;
						}
#else
						logHitchDetailsNow = true;
#endif
					}
				}

				if (logHitchDetailsNow ||
					logAndForceToStopNow)
				{
					System::TimeStamp ts;
#ifndef SHORT_HITCH_FRAME_INFO
#ifdef AN_DEVELOPMENT
					bool withSystemInfo = true;
#else
					bool withSystemInfo = false;
#endif
					warn(TXT("=HITCH FRAME========================================"));
					warn(TXT("HITCH FRAME COUNT %i"), hitchFrameCount);
					warn(TXT("DELTA TIME %.3fms"), currentFrameInfo->deltaTime * 1000.0f);
					warn(TXT("USE FRAME TIME %.3fms"), frameTime * 1000.0f);
					warn(TXT("FRAME TIME %.3fms"), currentFrameInfo->frameTime * 1000.0f);
					warn(TXT("GAME SYNC FRAME TIME %.3fms"), currentFrameInfo->gameSyncFrameTime * 1000.0f);
					warn(TXT("EXPECTED T %.3fms [%.3fms]"), expectedFrameTime * 1000.0f, lowHitch * 1000.0f);
					{
						LogInfoContext log;
						log.log(TXT("PREVIOUS FRAME"));
						log.increase_indent();
						prevFrameInfo->log(log, true, withSystemInfo);
						log.decrease_indent();
						log.log(TXT("CURRENT FRAME $$$"));
						log.increase_indent();
						currentFrameInfo->log(log, true, withSystemInfo);
						log.decrease_indent();
						log.output_to_output();
					}
#endif
#ifdef AN_PERFORMANCE_MEASURE
					if (logAndForceToStopNow)
					{
						if (postProcessRenderTargetManager)
						{
							LogInfoContext log;
							log.log(TXT("POST PROCESS RENDER TARGETS"));
							postProcessRenderTargetManager->log(log);
							log.output_to_output();
						}
						if (should_render_to_main_for_vr())
						{
							Performance::System::display(nullptr, false, true);
						}
						stopToShowPerformance = true;
					}
#endif
#ifndef SHORT_HITCH_FRAME_INFO
					spentOnDebugTime = ts.get_time_since() + 0.1f; // to prevent getting here again
					logDeltaTimes = 3;
					warnLogDeltaTime = true;
#endif
				}
			}
			PERFORMANCE_MARKER(Colour::blue);
		}

		if (logDeltaTimes > 0)
		{
			if (warnLogDeltaTime)
			{
				output_colour(1, 1, 0, 1);
			}
			output(TXT("%06i dt %8.3fms ft %8.3fms <- "), currentFrameInfo->frameNo, currentFrameInfo->deltaTime * 1000.0f, currentFrameInfo->frameTime * 1000.0f);
			output_colour();
			warnLogDeltaTime = false;
			--logDeltaTimes;
		}
#endif
	}
	PERFORMANCE_MARKER(Colour::c64Grey2);

#ifdef AN_USE_FRAME_INFO
	::System::TimeStamp preThreadsTS;
#endif

	next_frame_info();
#else // AN_USE_FRAME_INFO
	if (! does_use_vr() || ! VR::IVR::get()->is_game_sync_blocking())
	{
		static bool lastFrameHitch = false;
		float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
		if (::System::Core::get_raw_delta_time() > expectedFrameTime * LOW_HITCH_MULTIPLIER)
		{
			++hitchFrameCount;
			if (!lastFrameHitch)
			{
				lastFrameHitch = true;
				if (VR::IVR::can_be_used())
				{
					warn(TXT("HITCH FRAME TIME [%i] %.3fms (expected %.3fms)"), hitchFrameCount, ::System::Core::get_raw_delta_time() * 1000.0f, expectedFrameTime * 1000.0f);
				}
			}
		}
		else
		{
			lastFrameHitch = false;
		}
	}
#endif

	{
		if (frameStart.get_time_since() > 0.1f || ignoreLongerFrame)
		{
			ModulePresence::ignore_auto_velocities();
		}
		else
		{
			ModulePresence::advance_ignore_auto_velocities();
		}
		frameStart.reset();
		ignoreLongerFrame = false;
	}

#ifdef AN_PERFORMANCE_MEASURE
	// start performance measure
	Performance::System::start_frame();
	PERFORMANCE_MARKER(Colour::whiteWarm);
#endif

	advance_remove_unused_temporary_objects_from_library();

	// advance frame number
	++frameNo;

#ifdef FPS_INFO
	// frames
	{
		MEASURE_PERFORMANCE_COLOURED(fpsInfo, Colour::zxBlack);
		fpsTime += ::System::Core::get_raw_delta_time();
		fpsCount += 1.0f;
		fpsTotalFrames += 1.0f;
		fpsTotalTime += ::System::Core::get_raw_delta_time();
		if (fpsTime >= 1.0f)
		{
			fpsLastSecond = fpsCount;
			fpsCount = 0.0f;
			fpsTime = mod(fpsTime, 1.0f);
			if (!fpsIgnoreFirstSecond)
			{
				fpsRange.include(fpsLastSecond);
				++fpsRange5Seconds;
				if (fpsRange5Seconds > 5)
				{
					fpsRange5show = fpsRange5;
					fpsRange5Seconds = 0;
					fpsRange5 = Range::empty;
				}
				fpsRange5.include(fpsLastSecond);
			}
			fpsIgnoreFirstSecond = false;
		}
		fpsImmediate = ::System::Core::get_raw_delta_time() != 0.0f ? 1.0f / ::System::Core::get_raw_delta_time() : 0.0f;
		fpsAveraged = fpsAveraged * 0.9f + fpsImmediate * 0.1f;
		fpsFrameTime = ::System::Core::get_raw_delta_time();
	}
#endif

	{
		MEASURE_PERFORMANCE_COLOURED(advanceSystem, Colour::zxBlackBright);
		// advance system
		::System::Core::set_time_multiplier(get_adjusted_time_multiplier());
		::System::Core::advance();
		deltaTime = ::System::Core::get_delta_time();
		jobSystem->access_advance_context().set_delta_time(deltaTime);
	}

#ifdef WITH_IN_GAME_EDITOR
	// it has to be done here, as otherwise we won't catch tilde being pressed as Core::advance might be called more than one time during "advance_frame"
	if (show_editor_if_requested())
	{
		deltaTime = 0.0f;
	}
#endif

	{
		jobSystem->clean_up_finished_off_system_jobs();
	}

	if (shortPauseCount == 0)
	{
		timeSinceShortPause += deltaTime;
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->preAdvanceTime);
#endif
		MEASURE_PERFORMANCE_COLOURED(preAdvance, Colour::zxGreenBright);
		pre_advance();
	}

	{
		// check if we require initial advance, this should happen only when was marked as required and was not advanced during last frame
		if (world &&
			initialWorldAdvanceRequired && !advancedWorldLastFrame)
		{
			initial_world_advance();
		}

		advancedWorldLastFrame = false;
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->worldManagerSyncAdvanceTime);
#endif
		MEASURE_PERFORMANCE_COLOURED(manageWorldSync, Colour::zxBlueBright);
		// manage world
		worldManager.advance_immediate_jobs();
		worldManager.advance_world_jobs(WorldJob::Sync);
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->preGameLoopTime);
#endif

		MEASURE_PERFORMANCE_COLOURED(preGameLoop, Colour::greenDark);

		preGameLoopJobStatus = GameJob::NotExecuted;

		if (runningSingleThreaded)
		{
			do_advance_vr_and_controls();
			do_pre_game_loop(nullptr, this);
		}
		else
		{
			MEASURE_PERFORMANCE_COLOURED(preGameLoopJobs, Colour::greenDark);

			jobSystem->do_asynchronous_job(gameJobs, do_pre_game_loop, this);

			do_advance_vr_and_controls();

			if (preGameLoopJobStatus != GameJob::Done)
			{
				MEASURE_PERFORMANCE(waitForPreGameLoop);
				while (preGameLoopJobStatus != GameJob::Done)
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					while (DebugVisualiser::get_active())
					{
						DebugVisualiserPtr dv(DebugVisualiser::get_active());
						if (dv.is_set())
						{
							::System::Core::set_time_multiplier();
							::System::Core::advance();
							if (MainSettings::global().should_render_fence_be_at_end_of_frame())
							{
								if (does_use_vr())
								{
									VR::IVR::get()->end_render(::System::Video3D::get());
								}
							}
							assert_rendering_thread();
							dv->advance_and_render_if_on_render_thread();
						}
					}
#endif
#ifdef GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
					advanceSynchronisationLounge.rest(0.001f);
#else
					::System::Core::sleep_u(10);
					::Concurrency::memory_fence();
#endif
				}
			}
		}
	}

#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->preThreads = preThreadsTS.get_time_since();
	::System::TimeStamp threadsTS;
#endif

#ifdef PROFILE_PERFORMANCE
	Concurrency::ThreadManager::store_current_thread_on_cpu();
#endif

	// mark that game loop and renderer has not finished
	gameLoopJobStatus = GameJob::NotExecuted;
	renderJobStatus = GameJob::NotExecuted;
	soundJobStatus = GameJob::NotExecuted;

	requiresRenderGateToAllow = true;
	requiresSoundGateToAllow = true;

	bool doAsyncWorldJobs = true;

	if (runningSingleThreaded)
	{
		// for single threaded - do render first and then, when command buffer is executed, do game loop
		do_render(nullptr, this);
		do_sound(nullptr, this);
		do_game_loop(nullptr, this);
	}
	else
	{
		MEASURE_PERFORMANCE_COLOURED(gameLoopJobs, Colour::greenDark);
		// add jobs
		// do sound and renderer before game loop to not end up inside game loop - as other thread could wake up, start doing game loop while sound would be not done yet
		jobSystem->do_asynchronous_job(renderJobs, do_render, this);
		if (! MainSettings::global().should_do_sound_on_game_thread())
		{
			jobSystem->do_asynchronous_job(soundJobs, do_sound, this);
		}
		jobSystem->do_asynchronous_job(gameJobs, do_game_loop, this);

		// focus on finishing rendering jobs first, as we're on renderer thread
		// although we can't choose which jobs to do, we just hope that priority for rendering is highest
		if (renderJobStatus != GameJob::Done)
		{
			MEASURE_PERFORMANCE_COLOURED(executeRenderJobsOnMainThread, Colour::zxBlue);

			// do renderer jobs
			while (renderJobStatus != GameJob::Done)
			{
				jobSystem->execute_main_executor();
			}
		}

		if (gameLoopJobStatus != GameJob::Done ||
			renderJobStatus != GameJob::Done ||
			soundJobStatus != GameJob::Done)
		{
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(currentFrameInfo->followUpTime);
			currentFrameInfo->followUpRender = renderJobStatus != GameJob::Done;
			currentFrameInfo->followUpSound = soundJobStatus != GameJob::Done;
			currentFrameInfo->followUpGameLoop = gameLoopJobStatus != GameJob::Done;
#endif
			MEASURE_PERFORMANCE_COLOURED(executePendingJobsOnMainThread, Colour::zxBlueBright);
			// wait for both game loop and renderer to finish and help as much as you can in meanwhile
			while (gameLoopJobStatus != GameJob::Done ||
				   renderJobStatus != GameJob::Done ||
				   soundJobStatus != GameJob::Done)
			{
				if (gameLoopJobStatus == GameJob::Done &&
					doAsyncWorldJobs)
				{
					doAsyncWorldJobs = false;
					{
#ifdef AN_USE_FRAME_INFO
						System::ScopedTimeStamp ts(currentFrameInfo->worldManagerAsyncAdvanceTime);
#endif
						MEASURE_PERFORMANCE_COLOURED(manageWorldAsyncDuringOthers, Colour::zxBlueBright);
						// manage world
						// do async jobs here, when we did everything for a frame, many jobs start with sync sections and there's no point in waiting whole game loop to perform them
						worldManager.advance_world_jobs(WorldJob::Async);
					}
				}
				{
					jobSystem->execute_main_executor();
				}
				if (gameLoopJobStatus != GameJob::Done ||
					renderJobStatus != GameJob::Done ||
					soundJobStatus != GameJob::Done)
				{
					// wait if there is still something running
					//MEASURE_PERFORMANCE_COLOURED(executePendingJobsOnMainThread_job, Colour::zxCyanBright);
					todo_future(TXT("put that on setting? if we won't wait, it runs much faster"));
					todo_future(TXT("2022-02-02 commented out to check performance"));
#ifdef GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
					advanceSynchronisationLounge.rest(0.001f);
#else
					::System::Core::sleep_u(10);
					::Concurrency::memory_fence();
#endif
				}
			}
		}
	}

#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->threads = threadsTS.get_time_since();
	::System::TimeStamp postThreadsTS;
#endif

#ifndef AN_PERFORMANCE_MEASURE
	advance_later_loading();
#endif

	if (doAsyncWorldJobs)
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->worldManagerAsyncAdvanceTime);
#endif
		MEASURE_PERFORMANCE_COLOURED(manageWorldAsyncPost, Colour::zxBlueBright);
		// manage world
		worldManager.advance_world_jobs(WorldJob::Async);
	}

/*
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::L))
	{
		Framework::GameConfig::switch_allow_lod();
	}
#endif
#endif
*/

#ifdef AN_PERFORMANCE_MEASURE
	bool outputPerformance = false;
	bool justShowedPerformance = false;
	if (doesAllowPerformanceOutput)
	{
		if (showPerformanceAfterFrames.is_set())
		{
			if (showPerformanceAfterFrames.get() <= 1 || showPerformanceAfterFrames.get() == showPerformanceAfterFramesStartedAt - 3)
			{
				// unpause vr! we won't be displaying debug panel vr when waiting for performance to display
				if (does_use_vr())
				{
					change_show_debug_panel(nullptr, true);
				}
				change_show_debug_panel(nullptr, false);
			}
			showPerformanceAfterFrames = showPerformanceAfterFrames.get() - 1;
			if (showPerformanceAfterFrames.get() <= 0)
			{
				showPerformanceAfterFrames.clear();
				stopToShowPerformance = true;
			}
		}
		if (stopToShowPerformanceEachFrame)
		{
			stopToShowPerformance = true;
		}
		if (stopToShowPerformance
#ifdef AN_STANDARD_INPUT
			|| ::System::Input::get()->has_key_been_pressed(::System::Key::O)
#endif
			)
		{
#ifdef AN_STANDARD_INPUT
			if ((::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
				::System::Input::get()->is_key_pressed(::System::Key::RightCtrl)))
			{
				stopToShowPerformanceEachFrame = !stopToShowPerformanceEachFrame;
			}
			else if ((::System::Input::get()->is_key_pressed(::System::Key::LeftShift) ||
				::System::Input::get()->is_key_pressed(::System::Key::RightShift)) &&
				!stopToShowPerformance)
			{
				allowToStopToShowPerformanceOnAnyHitch = !allowToStopToShowPerformanceOnAnyHitch;
			}
			else
#endif
			{
				showPerformance = !showPerformance || stopToShowPerformance;
				if (!stopToShowPerformanceEachFrame
#ifdef AN_STANDARD_INPUT
					|| ::System::Input::get()->has_key_been_pressed(::System::Key::O)
#endif
					)
				{
					stopToShowPerformance = false;
					stopToShowPerformanceEachFrame = false;
					allowToStopToShowPerformanceOnAnyHitch = false;
					outputPerformance = false;
				}
				justShowedPerformance = true;
#ifdef AN_STANDARD_INPUT
				// only for standard input as we may unpause
				System::Core::pause(showPerformance);
#endif
			}
		}
#ifndef AN_STANDARD_INPUT
		// if no standard input, handle pausing automatically
		if (does_use_vr())
		{
			if (showPerformance)
			{
				System::Core::pause_vr(bit(4));
			}
			else
			{
				System::Core::unpause_vr(bit(4));
			}
		}
#endif
		if (showPerformance
#ifdef AN_STANDARD_INPUT
			|| ::System::Input::get()->is_key_pressed(::System::Key::P)
#endif
			)
		{
			noRenderUntilShowPerformance = false;
			noGameLoopUntilShowPerformance = false;
#ifdef AN_STANDARD_INPUT
			if (((::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) || ::System::Input::get()->is_key_pressed(::System::Key::RightCtrl)) && justShowedPerformance) ||
				((::System::Input::get()->has_key_been_pressed(::System::Key::LeftCtrl) || ::System::Input::get()->has_key_been_pressed(::System::Key::RightCtrl))))
			{
				outputPerformance = true;
			}
#endif
			if (should_render_to_main_for_vr())
			{
				MEASURE_PERFORMANCE_COLOURED(performanceSystemDisplay, Colour::black);
				allowLaterLoading = false; // do not do anything in background (if possible) no need to disable that, we're already in dev/debug
				Performance::System::display(defaultFont ? defaultFont->get_font() : nullptr, true, outputPerformance);
			}
			if (outputPerformance)
			{
				Performance::System::output_long_jobs();
			}
		}
	}
#endif

#ifdef AN_DEVELOPMENT
	DebugVisualiserPtr dv(DebugVisualiser::get_active());
	if (dv.is_set())
	{
		if (MainSettings::global().should_render_fence_be_at_end_of_frame())
		{
			if (does_use_vr())
			{
				VR::IVR::get()->end_render(::System::Video3D::get());
			}
		}
		assert_rendering_thread();
		dv->advance_and_render_if_on_render_thread();
	}
	else
#endif
	{	
		// debug panel etc
		{
			todo_note(TXT("for time being, allow in final too"));
			if (showDebugPanel)
			{
#ifdef AN_PERFORMANCE_MEASURE
				blockAutoStopFor = 1.0f;
#endif
				showDebugPanel->advance(false);
			}
			if (showDebugPanel)
			{
				showDebugPanel->render_2d();
			}
			if (showDebugPanelVR)
			{
#ifdef AN_PERFORMANCE_MEASURE
				blockAutoStopFor = 1.0f;
#endif
				showDebugPanelVR->advance(true);
			}
			if (showLogPanelVR && showLogPanelVR != showDebugPanelVR)
			{
#ifdef AN_PERFORMANCE_MEASURE
				blockAutoStopFor = 1.0f;
#endif
				showLogPanelVR->advance(true);
			}

#ifdef AN_PERFORMANCE_MEASURE
			showPerformancePanelVR = showPerformance ? debugPerformancePanel : nullptr;
#else
			showPerformancePanelVR = nullptr;
#endif
			if (showPerformancePanelVR)
			{
				showPerformancePanelVR->advance(true);
			}
		}

		// display buffer
		{
			MEASURE_PERFORMANCE_COLOURED(displayBuffer, Colour::zxBlue);
			if (MainSettings::global().should_render_fence_be_at_end_of_frame())
			{
				System::RecentCapture::store_pixel_data(); // do it just before ending render as we will get stuck on CPU

				if (does_use_vr())
				{
					MEASURE_PERFORMANCE(vr_endRender_endOfFrame);
					VR::IVR::get()->end_render(::System::Video3D::get());
				}
				MEASURE_PERFORMANCE(v3d_displayBuffer);
				::System::Video3D::get()->display_buffer();
			}
			else
			{
				// just mark to display at a better time
				::System::Video3D::get()->mark_buffer_ready_to_display();
			}
		}
	}

	{
		MEASURE_PERFORMANCE_COLOURED(processDestructionPendingObjects, Colour::blue);
		perform_sync_world_job_on_this_thread(TXT("destruct pending objects"), [this](){ sync_process_destruction_pending_objects(); });
	}

#ifdef AN_PERFORMANCE_MEASURE
	// store if delayed
	if (delayedStorePerformanceFrameInfo.is_set())
	{
		if (delayedStorePerformanceFrameInfo.get() <= 0)
		{
			output(TXT("[fpi] order to save performance frame info"));
			delayedStorePerformanceFrameInfo.clear();
			Performance::System::save_frame(NP, delayedStorePerformanceFrameInfoSuffix, delayedStorePerformanceFrameInfoDelayUsed);
		}
		else
		{
			delayedStorePerformanceFrameInfo = delayedStorePerformanceFrameInfo.get() - 1;
			output(TXT("updated delayed saving performance frame info to (%i)"), delayedStorePerformanceFrameInfo.get());
		}
	}

	// end performance measure
	PERFORMANCE_MARKER(Colour::whiteCold);
	Performance::System::end_frame(showPerformance && (! (stopToShowPerformanceEachFrame && deltaTime > 0.0f)));
#endif

#ifdef AN_DEBUG_RENDERER
	// ready renderer for next frame
	if (deltaTime != 0.0f)
	{
		debug_renderer_next_frame(deltaTime);
	}
	else
	{
		debug_renderer_keep_frame();
	}
#endif

#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->postThreads = postThreadsTS.get_time_since();
#endif

#ifndef BUILD_PUBLIC_RELEASE
	lastGameFrameNo = ::System::Core::get_frame();
#endif
}

#undef LOW_HITCH_MULTIPLIER

void Game::do_pre_game_loop(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	Game* game = (Game*)_data;

	game->preGameLoopJobStatus = GameJob::InProgress;

	if (game->navSystem)
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(game->currentFrameInfo->navSystemAdvanceTime);
#endif
		MEASURE_PERFORMANCE_COLOURED(navSystemAdvance, Colour::zxYellowBright);
		// trigger tasks etc
		game->navSystem->advance();
	}

	game->pre_game_loop();

	game->preGameLoopJobStatus = GameJob::Done;

#ifdef GAME_USES_ADVANCE_SYNCHRONISATION_LOUNGE
	game->advanceSynchronisationLounge.wake_up_all();
#endif
}

void Game::do_advance_vr_and_controls()
{
	MEASURE_PERFORMANCE_COLOURED(gameAdvanceVRAndControls, Colour::zxMagenta);
	// has to be done before we break into preparation of scenes
	{
		// advance vr here, not in pre advance
		if (VR::IVR::can_be_used())
		{
			MEASURE_PERFORMANCE_COLOURED(gameVRAdvance, Colour::zxMagentaBright);

			VR::IVR::get()->advance();

			handle_vr_all_time_controls_input();
		}

		advance_controls();
	}
}

void Game::advance_later_loading()
{
	scoped_call_stack_info(TXT("Game::advance_later_loading"));

	if (allowLaterLoading &&
		! isLoadingLibrary &&
		get_world_manager_time_unoccupied() > 0.2f) // allow for gaps for loader to interrupt/stop
	{
		static float waitForLaterLoading = 0.0f;
		bool onlyOnDemand = false;
		if (!worldManager.is_busy() &&
			LibraryStored::has_something_for_later_loading())
		{
			waitForLaterLoading = onlyOnDemand ? max(0.0f, waitForLaterLoading - deltaTime) : 0.0f;
			if (waitForLaterLoading <= 0.0f)
			{
				add_async_world_job(TXT("later-loading"), []() {LibraryStored::load_one_of_later_loading(); });
			}
		}
		else
		{
			waitForLaterLoading = 0.5f; // delay
		}
	}
}

void Game::advance_remove_unused_temporary_objects_from_library(bool _avoidAddingNew)
{
	if (_avoidAddingNew)
	{
		removeUnusedTemporaryObjectsFromLibrary = false;
		removeUnusedTemporaryObjectsFromLibraryRequested = false;
	}
	else
	{
		timeSinceLastRemoveUnusedTemporaryObjectsFromLibrary += ::System::Core::get_raw_delta_time();

		if (autoRemoveUnusedTemporaryObjectsFromLibraryInterval.is_set())
		{
			if (!removeUnusedTemporaryObjectsFromLibraryRequested)
			{
				if (timeSinceLastRemoveUnusedTemporaryObjectsFromLibrary > autoRemoveUnusedTemporaryObjectsFromLibraryInterval.get())
				{
					timeSinceLastRemoveUnusedTemporaryObjectsFromLibrary = -autoRemoveUnusedTemporaryObjectsFromLibraryInterval.get() * 2.0f;
					removeUnusedTemporaryObjectsFromLibraryRequested = true;
				}
			}
		}

		if (removeUnusedTemporaryObjectsFromLibraryRequested)
		{
			if (!worldManager.is_performing_async_job() &&
				!navSystem->is_performing_task_or_queued())
			{
				if (!removeUnusedTemporaryObjectsFromLibrary)
				{
#ifndef INVESTIGATE_SOUND_SYSTEM
					output_colour(1, 1, 0, 0);
					output(TXT("started removal of unused temporary objects"));
					output_colour();
#endif
					removedUnusedTemporaryObjectsFromLibrary = 0;
					removeUnusedTemporaryObjectsFromLibrary = true;
					removeUnusedTemporaryObjectsFromLibraryRequested = false;
				}
			}
		}
	}

	if (removeUnusedTemporaryObjectsFromLibrary && !removeUnusedTemporaryObjectsFromLibraryAsyncInProgress)
	{
		MEASURE_PERFORMANCE_COLOURED(removeUnusedTemporaryObjectsFromLibrary, Colour::blackCold);
		removeUnusedTemporaryObjectsFromLibraryAsyncInProgress = true;
		get_job_system()->do_asynchronous_job(background_jobs(), remove_unused_temporary_objects_batch, this);
	}
}

void Game::pre_advance()
{
	scoped_call_stack_info(TXT("game::pre_advance"));

#ifdef AN_DEVELOPMENT
	if (world)
	{
		world->dev_check_if_ok();
	}
#endif

#ifdef AN_TWEAKS
	if (!checkReloadTweaks.should_wait() && checkReloadTweaksAllowedTimeStamp.get_time_since() > 1.0f) // leave 1 second pause
	{
		checkReloadTweaks.stop();
		get_job_system()->do_asynchronous_job(background_jobs(), check_tweaks_change_worker_static, this);
	}
	if (reloadTweaksNeeded)
	{
		reload_tweaks();
	}
#endif
#ifdef AN_ALLOW_BULLSHOTS
	if (!checkReloadBullshots.should_wait() && checkReloadBullshotsAllowedTimeStamp.get_time_since() > 1.0f) // leave 1 second pause
	{
		checkReloadBullshots.stop();
		get_job_system()->do_asynchronous_job(background_jobs(), check_bullshots_change_worker_static, this);
	}
	if (reloadBullshotsNeeded)
	{
		reload_bullshots();
	}
#endif

	{
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		jobSystem->pause_lock_checker();
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!DebugVisualiser::get_active())
#endif
		{
			MEASURE_PERFORMANCE(updateInputGrab);
			// update input grab
			::System::Input::get()->grab(should_input_be_grabbed());
		}
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		jobSystem->resume_lock_checker();
#endif
	}

	{
		MEASURE_PERFORMANCE(advanceFading);
		advance_fading();
	}

	process_delayed_object_creation();

#ifdef AN_DEVELOPMENT
	if (world)
	{
		world->dev_check_if_ok();
	}
#endif
}

void Game::process_delayed_object_creation()
{
	scoped_call_stack_info(TXT("process_delayed_object_creation"));
	if (does_allow_async_jobs())
	{
		MEASURE_PERFORMANCE(delayedObjectCreationInProgress);
		bool processNow = false;
		{
			Concurrency::ScopedSpinLock lock(delayedObjectCreationsLock, true);
			int goThroughLeft = 50;
			while (!delayedObjectCreations.is_empty() &&
				   !delayedObjectCreationInProgress.is_set() &&
				   !does_want_to_cancel_creation() &&
				   goThroughLeft > 0)
			{
				--goThroughLeft;
				if (delayedObjectCreations.get_first()->can_be_processed())
				{
					delayedObjectCreationInProgress = delayedObjectCreations.get_first();
					delayedObjectCreations.pop_front();
					processNow = true;
				}
				else if (!delayedObjectCreations.get_first()->can_ever_be_processed())
				{
					// we will never be able to do it
					delayedObjectCreations.pop_front();
				}
			}
		}
		if (processNow)
		{
#ifdef OUTPUT_GENERATION
			output(TXT("processing delayed creation from the queue (%p)"), delayedObjectCreationInProgress.get());
#endif
			delayedObjectCreationInProgress->process(this);
		}
	}
}

void Game::process_delayed_object_creation_immediately(Room* _room)
{
	ASSERT_ASYNC;
	{
		MEASURE_PERFORMANCE(delayedObjectCreationInProgress);
		while (true)
		{
			bool processNow = false;
			{
				Concurrency::ScopedSpinLock lock(delayedObjectCreationsLock, true);
				for (int i = 0; i < delayedObjectCreations.get_size() && !does_want_to_cancel_creation(); ++i)
				{
					auto* doc = delayedObjectCreations[i].get();
					if (doc->inRoom == _room)
					{
						delayedObjectCreationInProgress = doc;
						delayedObjectCreations.remove_at(i);
						processNow = true;
						break;
					}
				}
			}
			if (processNow)
			{
#ifdef OUTPUT_GENERATION
				output(TXT("processing delayed creation from the queue (immediately) (%p)"), delayedObjectCreationInProgress.get());
#endif
				delayedObjectCreationInProgress->process(this, true);
				delayedObjectCreationInProgress.clear();
			}
			else
			{
				break;
			}
		}
	}
}

void Game::remove_unused_temporary_objects_batch(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	Game* game = plain_cast<Game>(_data);
	{
		int removed = Library::get_current()->remove_unused_temporary_objects(game->removeUnusedTemporaryObjectsFromLibraryBatchSize);
		game->removeUnusedTemporaryObjectsFromLibrary = removed != 0;
		game->removedUnusedTemporaryObjectsFromLibrary += removed;
		if (!game->removeUnusedTemporaryObjectsFromLibrary)
		{
			game->timeSinceLastRemoveUnusedTemporaryObjectsFromLibrary = 0.0f;
#ifndef INVESTIGATE_SOUND_SYSTEM
			output_colour(1, 1, 0, 0);
			output(TXT("ended removal of unused temporary objects (removed: %i)"), game->removedUnusedTemporaryObjectsFromLibrary);
			output_colour();
#endif
#ifdef AN_DEVELOPMENT
			{
				LogInfoContext log;
				Library::get_current()->log_short_info(log);
				log.output_to_output();
			}
#endif
		}

		game->removeUnusedTemporaryObjectsFromLibraryAsyncInProgress = false;
	};
}

#ifdef AN_TWEAKS
#define TWEAKS_FILE_NAME TXT("_tweaks.xml")

void Game::check_tweaks_change_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	Game* game = plain_cast<Game>(_data);
	if (game->reloadTweaksNeeded)
	{
		return;
	}

	Concurrency::ScopedSpinLock reloadTweaksLockScoped(game->reloadTweaksLock);

	IO::DirEntry tweaksFileDirEntry;
	IO::Dir dir;
	dir.list(String(TXT(".")));
	auto dirEntries = dir.get_infos();
	int filenameLen = (int)tstrlen(TWEAKS_FILE_NAME);
	for_every(dirEntry, dirEntries)
	{
		if (dirEntry->path.get_right(filenameLen) == TWEAKS_FILE_NAME)
		{
			tweaksFileDirEntry = *dirEntry;
		}
	}

	if (game->reloadTweaksFileInfo != tweaksFileDirEntry)
	{
		game->reloadTweaksNeeded = true;
		game->reloadTweaksFileInfo = tweaksFileDirEntry;
	}
	else
	{
		game->checkReloadTweaksAllowedTimeStamp.reset();
		game->checkReloadTweaks.allow();
	}
}

void Game::reload_tweaks()
{
	Library::get_current()->load_tweaks_from_file(String(TWEAKS_FILE_NAME), LibrarySource::Files);
	reloadTweaksNeeded = false;
	checkReloadTweaksAllowedTimeStamp.reset();
	checkReloadTweaks.allow();
}
#endif

#ifdef AN_ALLOW_BULLSHOTS
#define BULLSHOTS_FILE_NAME TXT("_bullshots.xml")

void Game::check_bullshots_change_worker_static(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	Game* game = plain_cast<Game>(_data);
	if (game->reloadBullshotsNeeded)
	{
		return;
	}

	Concurrency::ScopedSpinLock reloadBullshotsLockScoped(game->reloadBullshotsLock);

	IO::DirEntry bullshotsFileDirEntry;
	IO::Dir dir;
	dir.list(String(TXT(".")));
	auto dirEntries = dir.get_infos();
	int filenameLen = (int)tstrlen(BULLSHOTS_FILE_NAME);
	for_every(dirEntry, dirEntries)
	{
		if (dirEntry->path.get_right(filenameLen) == BULLSHOTS_FILE_NAME)
		{
			bullshotsFileDirEntry = *dirEntry;
		}
	}

	if (game->reloadBullshotsFileInfo != bullshotsFileDirEntry)
	{
		game->reloadBullshotsNeeded = true;
		game->reloadBullshotsFileInfo = bullshotsFileDirEntry;
	}
	else
	{
		game->checkReloadBullshotsAllowedTimeStamp.reset();
		game->checkReloadBullshots.allow();
	}
}

void Game::reload_bullshots()
{
	reloadBullshotsNeeded = false;
	add_async_world_job_top_priority(TXT("reload bullshots"),
		[this]()
		{
			Library::get_current()->load_bullshots_from_file(String(BULLSHOTS_FILE_NAME), LibrarySource::Files);
			checkReloadBullshotsAllowedTimeStamp.reset();
			checkReloadBullshots.allow();

			post_reload_bullshots();

			BullshotSystem::update_objects(); // if no world (game not running), won't do anything
		});
}

void Game::post_reload_bullshots()
{
}

#endif

void Game::pre_game_loop()
{
	poiManager->update(deltaTime);
}

void Game::advance_controls()
{
}

void Game::render_block_gate_allow_one()
{
	if (requiresRenderGateToAllow)
	{
		requiresRenderGateToAllow = false;

		// mark rendering unblocked/allowed and wake up waiting for pre render sync
		renderBlockGate.allow_one();
	}
}

void Game::sound_block_gate_allow_one()
{
	if (requiresSoundGateToAllow)
	{
		requiresSoundGateToAllow = false;

		// mark sound unblocked/allowed and wake up waiting for pre render sync
		soundBlockGate.allow_one();
	}
}

void Game::sound()
{
	sound_block_gate_allow_one();

	// update sound now
	if (auto * ss = ::System::Sound::get())
	{
		ss->update(deltaTime);
	}
}

void Game::render()
{
	render_block_gate_allow_one();

	::System::Video3D* v3d = ::System::Video3D::get();
	// update (debug keys?)
	v3d->update();
	if (v3d->is_buffer_ready_to_display())
	{
		System::RecentCapture::store_pixel_data(); // do it just before ending render as we will get stuck on CPU

		if (does_use_vr())
		{
			VR::IVR::get()->end_render(v3d);
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!DebugVisualiser::get_active())
#endif
		{
			v3d->display_buffer_if_ready();
		}
	}
	if (does_use_vr())
	{
		VR::IVR::get()->begin_render(v3d);
	}
}

void Game::game_loop()
{
	check_input_for_icons();

#ifdef AN_DEVELOPMENT
	if (world)
	{
		world->dev_check_if_ok();
	}
#endif
}

void Game::async_system_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as system job
		_game->get_job_system()->do_asynchronous_job(_game->system_jobs(), _jobFunc, _data);
	}
	else
	{
		_jobFunc(nullptr, _data);
		_data->release_job_data();
	}
}

void Game::async_system_job(Game* _game, std::function<void()> _do)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as system job
		_game->get_job_system()->do_asynchronous_job(_game->system_jobs(), _do);
	}
	else
	{
		_do();
	}
}

void Game::async_worker_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as worker job
		_game->get_job_system()->do_asynchronous_job(_game->async_worker_jobs(), _jobFunc, _data);
	}
	else
	{
		_jobFunc(nullptr, _data);
		_data->release_job_data();
	}
}

void Game::async_background_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as background job
		_game->get_job_system()->do_asynchronous_job(_game->background_jobs(), _jobFunc, _data);
	}
	else
	{
		_jobFunc(nullptr, _data);
		_data->release_job_data();
	}
}

void Game::async_background_job(Game* _game, std::function<void()> _do)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as game background job
		_game->get_job_system()->do_asynchronous_job(_game->background_jobs(), _do);
	}
	else
	{
		_do();
	}
}

void Game::async_game_background_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as game background job
		_game->get_job_system()->do_asynchronous_job(_game->game_background_jobs(), _jobFunc, _data);
	}
	else
	{
		_jobFunc(nullptr, _data);
		_data->release_job_data();
	}
}

void Game::async_game_background_job(Game* _game, std::function<void()> _do)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as game background job
		_game->get_job_system()->do_asynchronous_job(_game->game_background_jobs(), _do);
	}
	else
	{
		_do();
	}
}

void Game::loading_worker_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as worker job
		_game->get_job_system()->do_asynchronous_job(_game->loading_worker_jobs(), _jobFunc, _data);
	}
	else
	{
		_jobFunc(nullptr, _data);
		_data->release_job_data();
	}
}

void Game::preparing_worker_job(Game* _game, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data)
{
	if (_game && !_game->is_running_single_threaded())
	{
		// run as worker job
		_game->get_job_system()->do_asynchronous_job(_game->preparing_worker_jobs(), _jobFunc, _data);
	}
	else
	{
		_jobFunc(nullptr, _data);
		_data->release_job_data();
	}
}

void Game::setup_to_render_to_main_render_target(::System::Video3D* _v3d)
{
	setup_to_render_to_render_target(_v3d);
}

void Game::setup_to_render_to_render_target(::System::Video3D* _v3d, ::System::RenderTarget* _rtOverride)
{
	_v3d->setup_for_2d_display();

	if (_rtOverride)
	{
		_v3d->set_viewport(VectorInt2::zero, _rtOverride->get_size());
	}
	else
	{
		_v3d->set_default_viewport();
	}
	_v3d->set_near_far_plane(0.02f, 100.0f);

	_v3d->set_2d_projection_matrix_ortho();
	_v3d->access_model_view_matrix_stack().clear();
}

void Game::set_default_font(LibraryName const & _fontName)
{
	defaultFontName = _fontName;
	update_default_assets();
}

void Game::update_default_assets()
{
	defaultFont = ::Library::get_current()->get_fonts().find(defaultFontName);
	debug_renderer_use_font(get_default_font() ? get_default_font()->get_font() : nullptr);
	if (auto * font = get_default_font())
	{
		DebugVisualiser::set_default_font(font->get_font());
	}
}

void Game::create_library()
{
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("create library"));
#endif
	Library::initialise_static<Library>();
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("create library [done]"));
#endif
}

void Game::setup_library()
{
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("setup library"));
#endif
	DEFINE_STATIC_NAME(allTimeControls);
	allTimeControlsInput.use(NAME(allTimeControls));
	allTimeControlsInput.use(GameInputIncludeVR::All, true);

	// at the moment we don't have any default game-specific components to setup here
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("setup library [done]"));
#endif
}

bool Game::load_library(String const & _fromDir, LibraryLoadLevel::Type _libraryLoadLevel, Loader::ILoader* _loader, OPTIONAL_ LibraryLoadSummary* _summary)
{
	Array<String> fromDirectories;
	if (!_fromDir.is_empty())
	{
		fromDirectories.push_back(_fromDir);
	}
	Array<String> noFiles;
	return load_library(fromDirectories, noFiles, _libraryLoadLevel, _loader, _summary);
}

bool Game::load_library(Array<String> const & _fromDirectories, Array<String> const & _loadFiles, LibraryLoadLevel::Type _libraryLoadLevel, Loader::ILoader* _loader, OPTIONAL_ LibraryLoadSummary* _summary)
{
	isLoadingLibrary = true;

	::System::TimeStamp loadingTime;

	if (::System::Video3D::get()->access_shader_program_cache().is_empty())
	{
		::System::Video3D::get()->access_shader_program_cache().load_from_files();
	}

	String fromDirs;

	// this sets up loading library and executes tasks on system job queue (loading shaders etc)
	if (_fromDirectories.get_size() == 1 && _loadFiles.is_empty())
	{
		output_colour(0, 1, 1, 0);
		output(TXT("load library \"%S\"..."), _fromDirectories.get_first().to_char());
		output_colour();
	}
	else
	{
		for_every(fromDir, _fromDirectories)
		{
			if (!fromDirs.is_empty())
			{
				fromDirs += TXT(", ");
			}
			fromDirs += TXT("\"");
			fromDirs += *fromDir;
			fromDirs += TXT("\"");
		}
		for_every(loadFile, _loadFiles)
		{
			if (!fromDirs.is_empty())
			{
				fromDirs += TXT(", ");
			}
			fromDirs += TXT("\"");
			fromDirs += *loadFile;
			fromDirs += TXT("\"");
		}
		output_colour(0, 1, 1, 0);
		output(TXT("load libraries %S..."), fromDirs.to_char());
		output_colour();
	}

	create_library();

	setup_library();

	GameLoadingLibraryContext gameLoadingLibraryContext;
	gameLoadingLibraryContext.game = this;
	gameLoadingLibraryContext.libraryLoadLevel = _libraryLoadLevel;
	gameLoadingLibraryContext.fromDirectories = _fromDirectories;
	gameLoadingLibraryContext.loadFiles = _loadFiles;
	gameLoadingLibraryContext.summary = _summary;

	bool result = true;

	loadLibraryResult = true;
	prepareLibraryResult = true;

#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("start loading library"));
#endif

	Framework::Library::get_current()->connect_to_game(this);
	if (!is_running_single_threaded())
	{
#ifdef AN_DEBUG_LOAD_LIBRARY
		output(TXT("use threads"));
#endif
		auto backgroundJobsList = get_job_system()->get_asynchronous_list(background_jobs());
		auto systemJobsList = get_job_system()->get_asynchronous_list(system_jobs());
		get_job_system()->do_asynchronous_job(background_jobs(), load_library_and_prepare_worker_static, &gameLoadingLibraryContext);
		bool loaderDeactivated = false;
		_loader->activate();
		while (!backgroundJobsList->has_finished() || !systemJobsList->has_finished() || (_loader && _loader->is_active()))
		{
			if (_loader && !loaderDeactivated &&
				backgroundJobsList->has_finished() && systemJobsList->has_finished())
			{
				_loader->deactivate();
				loaderDeactivated = true;
				isLoadingLibrary = false; // here is the proper point for marking when we finished loading

				result = update_library_loading_result(_fromDirectories, _loadFiles, fromDirs, loadingTime);
			}
			force_advance_world_manager(true);
			advance_and_display_loader(_loader);
		}
		output_waiting_done();
	}
	else
	{
#ifdef AN_DEBUG_LOAD_LIBRARY
		output(TXT("single threaded"));
#endif
		load_library_and_prepare_worker_static(nullptr, &gameLoadingLibraryContext);

		result = update_library_loading_result(_fromDirectories, _loadFiles, fromDirs, loadingTime);
	}

	update_default_assets();

	// all shaders should be compiled at this point
	::System::Video3D::get()->access_shader_program_cache().save_to_file();

#ifdef AN_ALLOW_BULLSHOTS
	reload_bullshots();
#endif

	isLoadingLibrary = false;

	return result;
}

bool Game::update_library_loading_result(Array<String> const & _fromDirectories, Array<String> const & _loadFiles, String const & _fromDirs, ::System::TimeStamp & _loadingTime)
{
	bool result = true;
	if (!loadLibraryResult)
	{
		error(TXT("error loading library"));
		result = false;
	}
	if (!prepareLibraryResult)
	{
		error(TXT("error preparing library"));
		result = false;
	}
	if (result)
	{
		output_colour(0, 1, 1, 0);
		if (_fromDirectories.get_size() == 1 && _loadFiles.is_empty())
		{
			output(TXT("library \"%S\" loaded and prepared for game (%.1fs)"), _fromDirectories.get_first().to_char(), _loadingTime.get_time_since());
		}
		else
		{
			output(TXT("libraries %S loaded and prepared for game (%.1fs)"), _fromDirs.to_char(), _loadingTime.get_time_since());
		}
		output_colour();
	}

	return result;
}

void Game::do_vr_game_sync()
{
	an_assert(does_use_vr());

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->gameSync);
#endif
		VR::IVR::get()->game_sync();
	}
#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->gameSyncFrameTime = lastVRGameSyncTime.get_time_since();
	lastVRGameSyncTime.reset();
	if (currentFrameInfo->gameSyncFrameTime > 1.0f)
	{
		currentFrameInfo->gameSyncFrameTime = 0.0f;
	}
#endif
}

void Game::advance_and_display_loader(Loader::ILoader* _loader)
{
	scoped_call_stack_info(TXT("advance and display loader"));

	{
		scoped_call_stack_info(TXT("-> on_advance_and_display_loader"));
		on_advance_and_display_loader(_loader);
	}


	if (does_use_vr())
	{
		scoped_call_stack_info(TXT("-> end render"));
		// end any excess frame, if we haven't started one, we will bail out anyway
		VR::IVR::get()->end_render(::System::Video3D::get());
	}

	// advance system
	::System::Core::set_time_multiplier();
	::System::Core::advance();

#ifdef WITH_IN_GAME_EDITOR
	// it has to be done here, as otherwise we won't catch tilde being pressed as Core::advance might be called more than one time during "advance_frame"
	show_editor_if_requested();
#endif

	if (jobSystem)
	{
		scoped_call_stack_info(TXT("-> clean up system jobs"));
		jobSystem->clean_up_finished_off_system_jobs();
		jobSystem->access_advance_context().set_delta_time(::System::Core::get_delta_time());
#ifdef INVESTIGATE_LOAD_LIBRARY_STALLING
		::System::TimeStamp executeOneJob;
		int executed = 
#endif
		jobSystem->execute_main_executor(1); // execute one job to update screen
#ifdef INVESTIGATE_LOAD_LIBRARY_STALLING
		if (executed)
		{
			float timePassed = executeOneJob.get_time_since();
			output(TXT("in %0.3fms"), timePassed * 1000.0f);
			if (timePassed > 0.015f)
			{
				output(TXT("advance_and_display_loader stalling"));
			}
		}
#endif
	}

	if (auto * ss = ::System::Sound::get())
	{
		scoped_call_stack_info(TXT("-> sound update"));
		ss->update(::System::Core::get_raw_delta_time());
	}

	// advance vr, if can be used
	if (does_use_vr())
	{
		scoped_call_stack_info(TXT("-> sync vr"));
		VR::IVR::get()->advance();
		
		do_vr_game_sync(); // before begin_render!

		handle_vr_all_time_controls_input();
	}

	// process controls post vr advance to have the latest values
	advance_controls();

	advance_fading();

	advance_later_loading();

	// allow for gaps for loader to interrupt/stop
	if (get_world_manager_time_unoccupied() > 0.2f)
	{
		if (navSystem)
		{
			scoped_call_stack_info(TXT("-> navSystem->advance"));

			// trigger tasks etc
			navSystem->advance();
		}

		scoped_call_stack_info(TXT("-> process_delayed_object_creation"));

		process_delayed_object_creation();
	}

	// display loader
	{
		scoped_call_stack_info(TXT("-> display loader"));

		WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

		debug_renderer_next_frame(::System::Core::get_raw_delta_time());
		debug_renderer_not_yet_rendered();
		::System::Video3D * v3d = ::System::Video3D::get();
		//v3d->clear_colour_depth_stencil(Colour(((float)(rand() % 256)) / 255.0f, ((float)(rand() % 256)) / 255.0f, ((float)(rand() % 256)) / 255.0f));
		if (_loader)
		{
			pre_update_loader();
			_loader->update(::System::Core::get_raw_delta_time());
		}
		debug_gather_in_renderer();
		bool renderingAllowed = true;
		if (does_use_vr())
		{
			renderingAllowed = VR::IVR::get()->begin_render(v3d);
		}
		if (renderingAllowed)
		{
			// loaders will render to main output always
			if (_loader)
			{
				_loader->display(v3d, does_use_vr());
				an_assert(!::System::Video3D::get()->is_frame_buffer_bound());
				post_render_loader();
				an_assert(!::System::Video3D::get()->is_frame_buffer_bound());
			}
			else
			{
				v3d->set_default_viewport();
				v3d->set_near_far_plane(0.02f, 100.0f);

				Colour currentBackgroundColour = MainConfig::global().get_video().forcedBlackColour.get(Colour::black);

				if (does_use_vr())
				{
					for (int rtIdx = 0; rtIdx < 2; ++rtIdx)
					{
						if (System::RenderTarget* rt = VR::IVR::get()->get_output_render_target((VR::Eye::Type)rtIdx))
						{
							rt->bind();

							v3d->set_viewport(rt->get_size());

							v3d->clear_colour_depth_stencil(currentBackgroundColour);

							rt->bind_none();
						}
					}
				}
				else
				{
					System::RenderTarget::bind_none();
					v3d->set_default_viewport();

					v3d->clear_colour_depth_stencil(currentBackgroundColour);
				}
			}
			// apply fading
			{
				if (does_use_vr())
				{
					for (int rtIdx = 0; rtIdx < 2; ++rtIdx)
					{
						if (System::RenderTarget* rt = VR::IVR::get()->get_output_render_target((VR::Eye::Type)rtIdx))
						{
							apply_fading_to(rt);
						}
					}
				}
				else
				{
					apply_fading_to(nullptr);
				}
			}

			an_assert(!::System::Video3D::get()->is_frame_buffer_bound());
			System::RecentCapture::store_pixel_data(); // do it just before ending render as we will get stuck on CPU
			an_assert(!::System::Video3D::get()->is_frame_buffer_bound());
		}

		if (does_use_vr())
		{
			// this is a backup to render from vr output to the screen (loaders don't do that)
			// demo #demo - for demo always render to output, we want to see what player is doing
			render_vr_output_to_output();

			VR::IVR::get()->end_render(v3d);
		}
		{
			MEASURE_PERFORMANCE(debug);
			debug_gather_restore();
			todo_note(TXT("bind render target?"));
			debug_renderer_render(v3d);
			debug_renderer_undefine_contexts();
			debug_renderer_already_rendered();
		}
		{
			MEASURE_PERFORMANCE(v3d_displayBuffer);
			v3d->display_buffer();
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
		todo_note(TXT("this is temporary, this is only to allow checking for memory leaks"));
		if (System::Input::get()->is_key_pressed(System::Key::Esc))
		{
			set_wants_to_exit();
			output_colour_system();
			output(TXT("exit requested"));
			output_colour();
			if (_loader)
			{
				_loader->force_deactivate();
			}
		}
#endif
#endif
	}

	/*
#ifdef AN_DEVELOPMENT
	static int frame = 0;
	if (frame == 0)
	{
		// we're outputting waiting even if we're in hub, menu, etc
		output_waiting();
	}
	frame = (frame + 1) % 10;
#endif
	*/
}

void Game::show_loader_waiting_for_world_manager(Loader::ILoader* _loader, std::function<void()> _asyncTaskToFinish)
{
	start_short_pause();
	if (_asyncTaskToFinish)
	{
		add_async_world_job(TXT("async task for hub scene"), _asyncTaskToFinish);
	}
	force_advance_world_manager(_loader);
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
}

void Game::show_loader(Loader::ILoader* _loader)
{
	start_short_pause();
	if (!_loader->is_active())
	{
		_loader->activate();
	}
	while (_loader->is_active())
	{
		advance_and_display_loader(_loader);
	}
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
}

void Game::close_library(LibraryLoadLevel::Type _upToLibraryLoadLevel, Loader::ILoader* _loader)
{
	output_colour(0, 1, 1, 0);
	output(TXT("closing library..."));
	output_colour();
	defaultFont = nullptr; // we should no longer be using it!
	GameClosingLibraryContext gameClosingLibraryContext;
	gameClosingLibraryContext.game = this;
	gameClosingLibraryContext.libraryLoadLevel = _upToLibraryLoadLevel;
	if (!is_running_single_threaded())
	{
		auto backgroundJobsList = get_job_system()->get_asynchronous_list(background_jobs());
		auto systemJobsList = get_job_system()->get_asynchronous_list(system_jobs());
		get_job_system()->do_asynchronous_job(background_jobs(), close_library_worker_static, &gameClosingLibraryContext);
		bool loaderDeactivated = false;
		_loader->activate();
		while (!backgroundJobsList->has_finished() || !systemJobsList->has_finished() || (_loader && _loader->is_active()))
		{
			if (_loader && !loaderDeactivated &&
				backgroundJobsList->has_finished() && systemJobsList->has_finished())
			{
				_loader->deactivate();
				loaderDeactivated = true;
			}

			force_advance_world_manager(true); 
			advance_and_display_loader(_loader);
		}
		output_waiting_done();
	}
	else
	{
		close_library_worker(nullptr, &gameClosingLibraryContext);
	}
}

void Game::load_library_and_prepare_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	GameLoadingLibraryContext* gameLoadingLibraryContext = (GameLoadingLibraryContext*)_data;
	Game* self = gameLoadingLibraryContext->game;
	self->load_library_and_prepare_worker(_executionContext, gameLoadingLibraryContext);
}

void Game::load_library_and_prepare_worker(Concurrency::JobExecutionContext const * _executionContext, Framework::GameLoadingLibraryContext* gameLoadingLibraryContext)
{
	scoped_call_stack_info(TXT("load library worker"));
	Game* self = gameLoadingLibraryContext->game;
	self->loadLibraryResult = true;
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("load from directiories?"));
#endif
	// IMPLEMENT Framework::Library::get_current()->reset_progress();
	// IMPLEMENT Framework::Library::get_current()->provide_progress(loading);
	// IMPLEMENT Framework::Library::get_current()->count_to_load_from_directories(gameLoadingLibraryContext->fromDirectories, LibrarySource::Assets, gameLoadingLibraryContext->libraryLoadLevel);
	if (!Framework::Library::get_current()->load_from_directories(gameLoadingLibraryContext->fromDirectories, LibrarySource::Assets, gameLoadingLibraryContext->libraryLoadLevel, gameLoadingLibraryContext->summary))
	{
		error(TXT("problem while loading from directiores"));
		self->loadLibraryResult = false;
	}
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("load from files?"));
#endif
	for_every(file, gameLoadingLibraryContext->loadFiles)
	{
		self->loadLibraryResult &= Framework::Library::get_current()->load_from_file(*file, LibrarySource::Assets, gameLoadingLibraryContext->libraryLoadLevel, gameLoadingLibraryContext->summary);
	}
	Framework::Library::get_current()->wait_till_loaded();
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("add defaults"));
#endif
	Framework::Library::get_current()->add_defaults();
	self->prepareLibraryResult = true;
#ifdef AN_DEBUG_LOAD_LIBRARY
	output(TXT("prepare for game"));
#endif
	// IMPLEMENT Framework::Library::get_current()->provide_progress(preparing);
	// IMPLEMENT Framework::Library::get_current()->count_prepare_for_game();
	if (! Framework::Library::get_current()->prepare_for_game())
	{
		error(TXT("problem while preparing for game"));
		self->prepareLibraryResult = false;
	}
	// IMPLEMENT Framework::Library::get_current()->provide_progress(done);
}

void Game::close_library_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	GameClosingLibraryContext* gameClosingLibraryContext = (GameClosingLibraryContext*)_data;
	Game* self = gameClosingLibraryContext->game;
	self->close_library_worker(_executionContext, gameClosingLibraryContext);
}

void Game::close_library_worker(Concurrency::JobExecutionContext const * _executionContext, GameClosingLibraryContext* gameClosingLibraryContext)
{
	Framework::Library::get_current()->unload(gameClosingLibraryContext->libraryLoadLevel);
	if (gameClosingLibraryContext->libraryLoadLevel == LibraryLoadLevel::Close)
	{
		Framework::Library::close_static();
	}
}

void Game::pre_setup_module(IModulesOwner* _owner, Module* _module)
{
	if (ModuleCollision* mc = fast_cast<ModuleCollision>(_module))
	{
		an_assert(GameConfig::get());
		if (Item* item = fast_cast<Item>(_owner))
		{
			mc->access_collision_flags() = GameConfig::item_collision_flags();
			mc->access_collides_with_flags() = GameConfig::item_collides_with_flags();
			mc->update_collidable_object();
		}
		else if (Actor* actor = fast_cast<Actor>(_owner))
		{
			mc->access_collision_flags() = GameConfig::actor_collision_flags();
			mc->access_collides_with_flags() = GameConfig::actor_collides_with_flags();
			mc->update_collidable_object();
		}
		else if (Scenery* scenery = fast_cast<Scenery>(_owner))
		{
			mc->access_collision_flags() = GameConfig::scenery_collision_flags();
			mc->access_collides_with_flags() = GameConfig::scenery_collides_with_flags();
			mc->update_collidable_object();
		}
		else if (TemporaryObject* to = fast_cast<TemporaryObject>(_owner))
		{
			mc->access_collision_flags() = GameConfig::temporary_object_collision_flags();
			mc->access_collides_with_flags() = GameConfig::temporary_object_collides_with_flags();
			mc->update_collidable_object();
		}
	}
}

void Game::suspend_destruction(Name const & _reason)
{
	Concurrency::ScopedSpinLock lock(destructionSuspendedLock);
	// do not check for multiple occurrences. it is possible and we should have the same amount of calls here and back
	// if we want to clear it (if we removed jobs altogether), we have resume_destruction(true)
	destructionSuspendedReasons.push_back(_reason);
	destructionSuspended = true;
}

void Game::resume_destruction(Name const& _reason)
{
	Concurrency::ScopedSpinLock lock(destructionSuspendedLock);
	destructionSuspendedReasons.remove_fast(_reason);
	destructionSuspended = ! destructionSuspendedReasons.is_empty();
}

void Game::resume_destruction(bool _forceAll)
{
	if (_forceAll)
	{
		Concurrency::ScopedSpinLock lock(destructionSuspendedLock);
		destructionSuspendedReasons.clear();
		destructionSuspended = false;
	}
}

void Game::sync_process_destruction_pending_objects()
{
	if (destructionPendingWait)
	{
		destructionPendingWait = false;
		return;
	}
	if (destructionSuspended)
	{
		return;
	}
	{
#ifdef WITH_GAME_DESTUTION_PENDING_OBJECT_LOCK
		Concurrency::ScopedSpinLock lock(destructionPendingObjectLock, true);
#endif
		if (destructionPendingObject)
		{
			int noMoreThan = 5;
			while (destructionPendingObject && noMoreThan > 0)
			{
				destructionPendingObject->destruct_and_delete();
				--noMoreThan;
			}
		}
	}
}

void Game::fade_in(float _time, Optional<Name> const & _reason, Optional<float> const & _delay, Optional<bool> const & _useRawDeltaTime)
{
#ifdef INVESTIGATE_MISSING_INPUT
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# fade in %S"), _reason.is_set() ? _reason.get().to_char() : TXT("--"));
#endif
#ifdef INSPECT_FADING
	output(TXT("[fading] fade in %.3f delay %.3f"), _time, _delay.get(0.0f));
#endif
	fading.reason = _reason.get(Name::invalid());
	fading.target = 0.0f;
	fading.fadingTime = fading.target < fading.current ? _time : 0.0f;
	fading.blendTime = fading.target < fading.current ? _time / (fading.current - fading.target) : 0.0f;
	fading.pauseFadingFor = max(0.0f, _delay.get(0.0f));
	fading.fadeSoundNow = true;
	fading.useRawDeltaTime = _useRawDeltaTime.get(false);
}

void Game::fade_out(float _time, Optional<Name> const & _reason, Optional<float> const & _delay, Optional<bool> const & _useRawDeltaTime, Optional<Colour> const & _fadeToColour, Optional<bool> const& _fadeSounds)
{
#ifdef INVESTIGATE_MISSING_INPUT
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# fade out %S"), _reason.is_set() ? _reason.get().to_char() : TXT("--"));
#endif
#ifdef INSPECT_FADING
	output(TXT("[fading] fade out %.3f delay %.3f"), _time, _delay.get(0.0f));
#endif
	fading.reason = _reason.get(Name::invalid());
	fading.targetFadeOutColour = _fadeToColour.get(Colour::black);
	if (fading.current == 0.0f)
	{
		fading.currentFadeOutColour = fading.targetFadeOutColour;
	}
	fading.target = 1.0f;
	fading.fadingTime = fading.target > fading.current ? _time : 0.0f;
	fading.blendTime = fading.target > fading.current ? _time / (fading.target - fading.current) : 0.0f;
	fading.pauseFadingFor = max(0.0f, _delay.get(0.0f));
	fading.fadeSoundNow = _fadeSounds.get(true);
	fading.useRawDeltaTime = _useRawDeltaTime.get(false);
}

void Game::fade_in_immediately()
{
#ifdef INVESTIGATE_MISSING_INPUT
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# fade in immediately"));
#endif
#ifdef INSPECT_FADING
	output(TXT("[fading] fade in immediately"));
#endif
	fading.current = fading.target = 0.0f;
	fading.pauseFadingFor = 0.0f;
	fading.fadingTime = 0.0f;
	fading.blendTime = 0.0f;
	fading.fadeSoundNow = true;
	fading.useRawDeltaTime = false;
}

void Game::fade_out_immediately(Optional<Colour> const & _fadeToColour)
{
#ifdef INVESTIGATE_MISSING_INPUT
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# fade out immediately"));
#endif
#ifdef INSPECT_FADING
	output(TXT("[fading] fade out immediately"));
#endif
	if (fading.current == 0.0f)
	{
		fading.targetFadeOutColour = Colour::black;
	}
	fading.currentFadeOutColour = fading.targetFadeOutColour = _fadeToColour.get(fading.targetFadeOutColour);
	fading.current = fading.target = 1.0f;
	fading.pauseFadingFor = 0.0f;
	fading.fadingTime = 0.0f;
	fading.blendTime = 0.0f;
	fading.fadeSoundNow = true;
	fading.useRawDeltaTime = false;
}

float Game::get_fade_out() const
{
	float fadeOut = fading.current;

	if (auto* ss = ::System::Sound::get())
	{
		float ssFade = ss->get_fade(soundFadeChannelTagCondition);
		float ssFadeOut = 1.0f - ssFade;
		fadeOut = min(fadeOut, ssFadeOut); // both have to be faded out
	}

	return fadeOut;
}

void Game::advance_fading(Optional<float> const & _deltaTime)
{
	scoped_call_stack_info(TXT("Game::advance_fading"));
	if (fadeOutAndQuit)
	{
		fading.target = 1.0f;
		fading.pauseFadingFor = 0.0f;
		fading.targetFadeOutColour = Colour::black;
		fading.fadingTime = 0.2f;
		fading.blendTime = 0.2f;
		fading.fadeSoundNow = true;
		if (fading.current > 0.99f)
		{
			::System::Core::set_quick_exit_handler(nullptr); // to allow actual quick exit to happen
			::System::Core::quick_exit();
		}
	}
	if (fading.current != fading.target)
	{
		float deltaTimeToUse = fading.useRawDeltaTime || System::Core::is_vr_paused()? System::Core::get_raw_delta_time() : (_deltaTime.is_set() ? _deltaTime.get() : deltaTime);
#ifdef INSPECT_FADING
		output(TXT("[fading] delta time %.3f"), deltaTimeToUse);
#endif
		if (deltaTimeToUse > 0.1f)
		{
			deltaTimeToUse = 0.03f;
#ifdef INSPECT_FADING
			output(TXT("[fading] delta time too big, limited to %.3f"), deltaTimeToUse);
#endif
		}
		if (deltaTimeToUse > 0.0f)
		{
			if (fading.pauseFadingFor > 0.0f)
			{
				fading.pauseFadingFor -= deltaTimeToUse;
				if (fading.pauseFadingFor < 0.0f)
				{
					deltaTimeToUse += fading.pauseFadingFor;
					fading.pauseFadingFor = 0.0f;
				}
				else
				{
#ifdef INSPECT_FADING
					output(TXT("[fading] fading paused, left %.3f"), fading.pauseFadingFor);
#endif
				}
			}
			if (fading.pauseFadingFor == 0.0f)
			{
#ifdef INSPECT_FADING
				output(TXT("[fading] fade %.3f -> %.3f, blend time %.3f"), fading.current, fading.target, fading.blendTime);
#endif
				float prevCurrent = fading.current;
				fading.current = blend_to_using_speed_based_on_time(fading.current, fading.target, fading.blendTime, deltaTimeToUse);
				if (fading.current < 0.001f)
				{
					fading.current = 0.0f;
					fading.currentFadeOutColour = fading.targetFadeOutColour;
				}
				if (fading.current > 0.999f && fading.target == 1.0f)
				{
					fading.current = 1.0f;
					fading.currentFadeOutColour = fading.targetFadeOutColour;
				}
				else if (fading.current > prevCurrent)
				{
					fading.currentFadeOutColour = Colour::lerp((fading.current - prevCurrent) / (1.0f - prevCurrent), fading.currentFadeOutColour, fading.targetFadeOutColour);
				}
			}
		}
#ifdef INSPECT_FADING
		output(TXT("[fading] fading %.3f"), fading.current);
#endif
	}
	else
	{
		fading.currentFadeOutColour = fading.targetFadeOutColour;
	}

	if (fading.fadeSoundNow)
	{
		if (fading.pauseFadingFor == 0.0f)
		{
			if (auto * ss = ::System::Sound::get())
			{
				ss->fade_to(1.0f - fading.target, fading.fadingTime, soundFadeChannelTagCondition);
			}
			fading.fadeSoundNow = false;
		}
	}
}

void Game::apply_fading_to(System::RenderTarget* _rt) const
{
	if (fading.should_be_applied())
	{
		System::Video3D* v3d = System::Video3D::get();
		if (_rt)
		{
			_rt->bind();
			v3d->set_viewport(VectorInt2::zero, _rt->get_size());
		}
		else
		{
			System::RenderTarget::bind_none();
			v3d->set_default_viewport();
		}
		v3d->setup_for_2d_display();
		v3d->set_2d_projection_matrix_ortho();
		v3d->set_near_far_plane(0.02f, 100.0f);
		v3d->mark_enable_blend(System::BlendOp::SrcAlpha, System::BlendOp::OneMinusSrcAlpha);
		Vector2 size = v3d->get_viewport_size().to_vector2();
		v3d->set_fallback_colour(fading.currentFadeOutColour.with_alpha(fading.current));

		Vector2 points[4];
		points[0] = Vector2(0.0f, 0.0f);
		points[1] = Vector2(0.0f, 0.0f + size.y);
		points[2] = Vector2(0.0f + size.x, 0.0f + size.y);
		points[3] = Vector2(0.0f + size.x, 0.0f);
		Meshes::Mesh3D::render_data(v3d, nullptr, nullptr, Meshes::Mesh3DRenderingContext(), points, ::Meshes::Primitive::TriangleFan, 2, fadingVertexFormat);

		v3d->set_fallback_colour();
		v3d->mark_disable_blend();

		System::RenderTarget::unbind_to_none();
	}
}

void Game::save_system_tags_file()
{
	IO::File f;
	f.create(IO::get_user_game_data_directory() + get_system_tags_file());
	f.set_type(IO::InterfaceType::Text);
	f.write_text(::System::Core::get_extra_requested_system_tags());
}

void Game::save_config_file()
{
	if (supressedSavingConfigFile)
	{
		return;
	}
	for_count(int, userConfigFile, 2)
	{
		if (userConfigFile || wantsToCreateMainConfigFile)
		{
			IO::XML::Document doc;
			IO::XML::Node* libraryNode = doc.get_root()->add_node(TXT("library"));

			IO::XML::Node* mainConfigNode = libraryNode->add_node(TXT("mainConfig"));
			MainConfig::global().save_to_xml(mainConfigNode, userConfigFile);
			//
			if (!userConfigFile && wantsToCreateMainConfigFile)
			{
				MainConfig::save_to_be_copied_to_main_config_xml(libraryNode);
			}

			GameConfig::save_to_xml(libraryNode, userConfigFile);
			//
			if (!userConfigFile && wantsToCreateMainConfigFile)
			{
				GameConfig::save_to_be_copied_to_main_config_xml(libraryNode);
			}

			if (!userConfigFile)
			{
				VR::Offsets::save_to_xml(libraryNode);
				VR::Input::Devices::save_to_xml(libraryNode);
				GameInputDefinitions::save_to_xml(libraryNode);
			}

			IO::File file;
			if (file.create(IO::get_user_game_data_directory() + (userConfigFile ? get_user_config_file() : get_main_config_file())))
			{
				file.set_type(IO::InterfaceType::Text);
				doc.save_xml(&file);
				output_colour_system();
				output(TXT("config saved"));
				output_colour();
				if (userConfigFile && closingGame) // only user config and only when closing the game
				{
					IO::OutputWriter ow;
					doc.save_xml(&ow);
				}
			}
		}
	}
}

void Game::set_performance_mode(bool _inPerformanceMode)
{
	worldManager.set_performance_mode(_inPerformanceMode);
}

void Game::post_set_performance_mode(bool _inPerformanceMode)
{
	add_sync_world_job(TXT("set performance mode"), [this, _inPerformanceMode]() { set_performance_mode(_inPerformanceMode); });
}

void Game::add_sync_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	MEASURE_PERFORMANCE(game_addSyncWorldJob);
	worldManager.add_sync_world_job(_jobName, _func);
}

void Game::perform_sync_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	scoped_call_stack_info(TXT("perform_sync_world_job"));
	MEASURE_PERFORMANCE(game_performSyncWorldJob);
#ifdef AN_LINUX_OR_ANDROID
	bool doAsynchronousJob = false;
	if (! worldManager.is_nested_sync_job() &&
		  worldManager.is_performance_mode() &&
		! Concurrency::ThreadManager::is_main_thread())
	{
		if (jobSystemInfo.doSyncWorldJobsAsAsynchronousGameJobs)
		{
			if (! jobSystem->is_on_thread_that_handles(gameJobs, renderJobs))
			{
				doAsynchronousJob = true;
			}
		}
	}

	// when dealing with multiple threads on linux/android we get some threads that are real-time and some that are normal
	// normal threads may have context switched and stall for a few milliseconds making them unreliable at performing sync jobs
	// it is better to do some extra work and offset the job to a faster thread that does it when it has time
	// there shouldn't be multiple perform sync jobs happening anyway
	if (doAsynchronousJob)
	{
		scoped_call_stack_info(TXT("create async game job for this sync world job"));
		MEASURE_PERFORMANCE_STR_COLOURED(_jobName, Colour::zxRed);

		// do the job by offsetting perform sync world job to other thread
		// but we're still going through the whole process
		// we're waiting here till it's done
		auto* ag = get_current_activation_group_if_present();
		::Concurrency::Gate gate;
		jobSystem->do_asynchronous_job(gameJobs,
			[this, _jobName, _func, ag, &gate]()
			{
				// offset activation group to this thread, otherwise we won't have access to it and we will be unable to tell what we're doing
				if (ag)
				{
					push_activation_group(ag);
				}
				perform_sync_world_job_on_this_thread(_jobName, _func);
				if (ag)
				{
					pop_activation_group(ag);
				}
				gate.allow_one();
			});
		gate.wait(); // this is effectively a memory barrier
	}
	else
#endif
	{
		scoped_call_stack_info(TXT("execute sync world job now"));
		perform_sync_world_job_on_this_thread(_jobName, _func);
	}
}

void Game::perform_sync_world_job_on_this_thread(tchar const * _jobName, WorldJob::Func _func)
{
	MEASURE_PERFORMANCE(game_performSyncWorldJob_onThisThread);
	worldManager.perform_sync_world_job(_jobName, _func);

	/*
	// use this to stop and see a longer task
	static int count = 0;
	++count;
	if (count == 6000)
	{
		stopToShowPerformance = true;
	}
	*/
}

void Game::add_immediate_sync_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	MEASURE_PERFORMANCE(addImmediateSyncWorldJob);
	worldManager.add_immediate_sync_world_job(_jobName, _func);
}

void Game::add_async_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	MEASURE_PERFORMANCE(addAsyncWorldJob);
	worldManager.add_async_world_job(_jobName, _func);
}

void Game::add_async_world_job_top_priority(tchar const * _jobName, WorldJob::Func _func)
{
	MEASURE_PERFORMANCE(addAsyncWorldJob);
	worldManager.add_async_world_job_top_priority(_jobName, _func);
}

bool Game::is_performing_sync_job() const
{
	return worldManager.is_performing_sync_job();
}

#ifdef AN_DEVELOPMENT
bool Game::is_performing_sync_job_on_this_thread() const
{
	return worldManager.is_performing_sync_job_on_this_thread();
}
#endif

bool Game::does_allow_async_jobs() const
{
	return ! removeUnusedTemporaryObjectsFromLibrary &&
		   ! removeUnusedTemporaryObjectsFromLibraryRequested;
}

bool Game::is_performing_async_job_on_this_thread() const
{
	return worldManager.is_performing_async_job_on_this_thread();
}

bool Game::is_performing_async_job_and_last_world_job_on_this_thread(int _leftBesidesThis) const
{
	return worldManager.is_performing_async_job_and_last_world_job_on_this_thread(_leftBesidesThis);
}

void Game::cancel_world_managers_jobs(Loader::ILoader* _loader)
{
	start_short_pause();
	if (!_loader->is_active())
	{
		_loader->activate();
	}
	while (_loader->is_active())
	{
		if (is_world_manager_busy())
		{
			cancel_world_managers_jobs();
			advance_remove_unused_temporary_objects_from_library(true);
			force_advance_world_manager(true);
		}
		else
		{
			_loader->deactivate();
		}
		advance_and_display_loader(_loader);
	}
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
}

void Game::cancel_world_managers_jobs(bool _andForceAdvanceTillTheEnd)
{
	if (_andForceAdvanceTillTheEnd)
	{
		while (is_world_manager_busy())
		{
			cancel_world_managers_jobs(false);
			advance_remove_unused_temporary_objects_from_library(true);
			force_advance_world_manager();
		}
	}
	else
	{
		worldManager.cancel_all();
	}
	resume_destruction(true); // some of the jobs could already suspend it, we shall resume
}

void Game::force_advance_world_manager(Loader::ILoader* _loader)
{
	start_short_pause();
	if (!_loader->is_active())
	{
		_loader->activate();
	}
	while (_loader->is_active())
	{
		worldManager.advance_time_unoccupied(); // we need to call it explicitly, as if world manager is not busy, it won't get called
		if (is_world_manager_busy())
		{
			advance_remove_unused_temporary_objects_from_library(true);
			force_advance_world_manager(true);
		}
		else
		{
			_loader->deactivate();
		}
		advance_and_display_loader(_loader);
	}
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
}

void Game::wait_till_loader_ends(Loader::ILoader* _loader, Optional<float> const & _deactivateAfter)
{
	start_short_pause();
	Optional<float> deactivateAfter = _deactivateAfter;
	while (_loader->is_active())
	{
		if (deactivateAfter.is_set())
		{
			deactivateAfter = deactivateAfter.get() - ::System::Core::get_raw_delta_time();
			if (deactivateAfter.get() <= 0.0f)
			{
				_loader->deactivate();
			}
		}
		force_advance_world_manager(true);
		advance_and_display_loader(_loader);
	}
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
}

struct BackgroundJob
: public Concurrency::AsynchronousJobData
{
	std::function<void()> func;
	Loader::ILoader* loader;
	BackgroundJob(std::function<void()> _func, Loader::ILoader* _loader) :func(_func), loader(_loader) {}

	static void perform(Concurrency::JobExecutionContext const* _executionContext, void* _data)
	{
		BackgroundJob* data = plain_cast<BackgroundJob>(_data);
		scoped_call_stack_info(TXT("async background job"));
		data->func();
		data->loader->deactivate();
		delete data;
	}
};

void Game::show_loader_and_perform_background(Loader::ILoader* _loader, std::function<void()> _perform)
{
	_loader->activate();
	get_job_system()->do_asynchronous_job(background_jobs(), BackgroundJob::perform, new BackgroundJob(_perform, _loader));

	start_short_pause();
	while (_loader->is_active())
	{
		force_advance_world_manager(true);
		advance_and_display_loader(_loader);
	}
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
}

void Game::force_advance_world_manager(bool _justOnce)
{
	worldManager.advance_time_unoccupied(); // we need to call it explicitly, as without "has_something_to_do" it won't get called
	while (worldManager.has_something_to_do())
	{
		worldManager.advance_jobs();
#ifdef AN_DEVELOPMENT_OR_PROFILER
		while (DebugVisualiser::get_active())
		{
			DebugVisualiserPtr dv(DebugVisualiser::get_active());
			if (dv.is_set())
			{
				::System::Core::set_time_multiplier(); 
				::System::Core::advance();
				if (MainSettings::global().should_render_fence_be_at_end_of_frame())
				{
					if (does_use_vr())
					{
						VR::IVR::get()->end_render(::System::Video3D::get());
					}
				}
				assert_rendering_thread();
				dv->advance_and_render_if_on_render_thread();
			}
		}
#endif
		if (_justOnce)
		{
			return;
		}
		::System::Core::sleep(0.001f);
	}
}

//

Game::WorldManagerImmediateSyncJobsScopedLock::WorldManagerImmediateSyncJobsScopedLock(Game* _game)
: game(_game)
{
	game->worldManager.disallow_immediate_sync_jobs();
}

Game::WorldManagerImmediateSyncJobsScopedLock::~WorldManagerImmediateSyncJobsScopedLock()
{
	game->worldManager.allow_immediate_sync_jobs();
}

//

Game::WorldManager::WorldManager(Game* _game)
: game(_game)
{
	syncJob.allow();
	immediateSyncJobsSempahore.allow();
	immediateSyncJobsManager.allow();
}

Game::WorldManager::~WorldManager()
{
	an_assert(worldJobs.is_empty(), TXT("you should force do all world jobs before deletion"));
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
void Game::WorldManager::get_async_world_job_infos(Array<tchar const*>& _infos, Optional<int> const& _max) const
{
	{
		Concurrency::ScopedSpinLock lock(worldJobsLock);
		for_every(wj, worldJobs)
		{
			if (!_max.is_set() ||
				_infos.get_size() < _max.get())
			{
				_infos.push_back(wj->jobName);
			}
			else
			{
				break;
			}
		}
	}
}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
String Game::WorldManager::get_current_async_world_job_info() const
{
	String info;
	{
		Concurrency::ScopedSpinLock lock(currentAsyncJobInfoLock);
		info = currentAsyncJobInfo;
	}
	return info;
}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
String Game::WorldManager::get_current_sync_world_job_info() const
{
	String info;
	{
		Concurrency::ScopedSpinLock lock(currentSyncJobInfoLock);
		info = currentSyncJobInfo;
	}
	return info;
}
#endif

void Game::WorldManager::setup_after_system_job_is_active()
{
	int threadCount = ::Concurrency::ThreadManager::get_thread_count();
#ifdef AN_DEVELOPMENT
	syncJobOnThread.set_size(threadCount);
	for_every(s, syncJobOnThread)
	{
		*s = false;
	}
#endif
	asyncJobOnThread.set_size(threadCount);
	for_every(s, asyncJobOnThread)
	{
		*s = false;
	}
}

void Game::WorldManager::allow_immediate_sync_jobs()
{
	immediateSyncJobsManager.wait_and_occupy();
#ifdef AN_DEVELOPMENT
	disallowedImmediateSyncJobsOnThread = NONE;
#endif
	immediateSyncJobsLockCounter.decrease();
	if (! immediateSyncJobsLockCounter.get())
	{
		immediateSyncJobsSempahore.allow();
	}
	immediateSyncJobsManager.allow();
}

void Game::WorldManager::disallow_immediate_sync_jobs()
{
	syncJob.wait(); // wait for any existing sync job to finish, this should only matter for allowed immediate sync jobs
	immediateSyncJobsManager.wait_and_occupy();
	if (! immediateSyncJobsLockCounter.get())
	{
		immediateSyncJobsSempahore.stop();
	}
	immediateSyncJobsLockCounter.increase();
	immediateSyncJobsManager.allow();
#ifdef AN_DEVELOPMENT
	disallowedImmediateSyncJobsOnThread = Concurrency::ThreadManager::get_current_thread_id();
#endif
}

void Game::WorldManager::perform_sync_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	scoped_call_stack_info(TXT("Game::WorldManager::perform_sync_world_job"));
#ifdef AN_DEVELOPMENT
	an_assert(disallowedImmediateSyncJobsOnThread == NONE || disallowedImmediateSyncJobsOnThread != Concurrency::ThreadManager::get_current_thread_id(), TXT("we're stuck here!"));
#endif
	if (is_nested_sync_job())
	{
		scoped_call_stack_info(TXT("nested sync job"));
		_func();
		return;
	}
	immediateSyncJobsManager.wait_and_occupy();
	while (immediateSyncJobsSempahore.should_wait())
	{
		immediateSyncJobsManager.allow();
		immediateSyncJobsSempahore.wait(); // wait till is allowed
		immediateSyncJobsManager.wait_and_occupy();
	}
	an_assert(!immediateSyncJobsSempahore.should_wait());
	PERFORMANCE_LIMIT_GUARD_START();
	perform_sync_job(_jobName, _func);
	PERFORMANCE_LIMIT_GUARD_STOP(0.001f, TXT("world sync job [%S]"), _jobName);
	an_assert(!immediateSyncJobsSempahore.should_wait());
	immediateSyncJobsManager.allow();
}

bool Game::WorldManager::is_nested_sync_job() const
{
	return syncJobThreadId == Concurrency::ThreadManager::get_current_thread_id();
}

void Game::WorldManager::add_sync_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	Concurrency::ScopedSpinLock lock(worldJobsLock);
	worldJobs.push_back(WorldJob(_func, WorldJob::Sync, _jobName));
	timeUnoccupied.clear();
}

void Game::WorldManager::add_async_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	Concurrency::ScopedSpinLock lock(worldJobsLock);
	worldJobs.push_back(WorldJob(_func, WorldJob::Async, _jobName));
	timeUnoccupied.clear();
}

void Game::WorldManager::add_async_world_job_top_priority(tchar const * _jobName, WorldJob::Func _func)
{
	Concurrency::ScopedSpinLock lock(worldJobsLock);
	int atIdx = 0;
	while (atIdx < worldJobs.get_size())
	{
		if (worldJobs[atIdx].type == WorldJob::Async &&
			!worldJobs[atIdx].topPriority)
		{
			break;
		}
		++atIdx;
	}
	worldJobs.insert_at(atIdx, WorldJob(_func, WorldJob::Async, _jobName, true));
}

void Game::WorldManager::add_immediate_sync_world_job(tchar const * _jobName, WorldJob::Func _func)
{
	Concurrency::ScopedSpinLock lock(immediateJobsLock);
	immediateJobs.push_back(WorldJob(_func, WorldJob::Sync, _jobName));
}

bool Game::WorldManager::is_performing_sync_job() const
{
	return syncJob.should_wait();
}

#ifdef AN_DEVELOPMENT
bool Game::WorldManager::is_performing_sync_job_on_this_thread() const
{
	return syncJobOnThread[Concurrency::ThreadManager::get_current_thread_id()] || Concurrency::ThreadManager::get_thread_count() == 1;
}
#endif

bool Game::WorldManager::is_performing_async_job() const
{
	return asyncJob.is_locked();
}

bool Game::WorldManager::is_performing_async_job_on_this_thread() const
{
	return asyncJobOnThread[Concurrency::ThreadManager::get_current_thread_id()] || Concurrency::ThreadManager::get_thread_count() == 1;
}

bool Game::WorldManager::is_performing_async_job_and_last_world_job_on_this_thread(int _leftBesidesThis) const
{
	if (is_performing_async_job_on_this_thread())
	{
		Concurrency::ScopedSpinLock lock(worldJobsLock);
		return worldJobs.get_size() == _leftBesidesThis; // there are no other world jobs right now pending, we're doing the last one
	}
	return false;
}

struct WorldAsyncJob
: public PooledObject<WorldAsyncJob>
{
	Game* game;
	Game::WorldJob::Func func;
	tchar const * jobName;
	WorldAsyncJob(Game* _game, Game::WorldJob::Func _func, tchar const * _jobName) : game(_game), func(_func), jobName(_jobName) {}
};

void Game::WorldManager::perform_sync_job(tchar const * _jobName, WorldJob::Func _func)
{
	if (is_nested_sync_job())
	{
		_func();
		return;
	}

	MEASURE_PERFORMANCE(worldManagerPerformSyncJob);
	scoped_call_stack_info(TXT("sync world job"));
	scoped_call_stack_info(_jobName);

#ifdef AN_DEBUG_WORLD_MANAGER_JOBS
	output(TXT("[world manager sync job] %S"), _jobName ? _jobName : TXT("[unnamed]"));
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	{
		Concurrency::ScopedSpinLock lock(currentSyncJobInfoLock);
		currentSyncJobInfo = _jobName;
	}
#endif

	syncJob.wait_and_occupy();
	syncJobThreadId = Concurrency::ThreadManager::get_current_thread_id();

#ifdef AN_DEVELOPMENT
	int currentThreadId = ::Concurrency::ThreadManager::get_current_thread_id();
	syncJobOnThread[currentThreadId] = true;
#endif

	{
		scoped_call_stack_info(TXT("perform sync world job"));
		MEASURE_PERFORMANCE_STR_COLOURED(_jobName, Colour::zxRedBright);
		MEASURE_PERFORMANCE_LONG_JOB(_jobName);
		PERFORMANCE_GUARD_LIMIT_2(0.001f, TXT("[sync job]"), _jobName);
		_func();
	}

#ifdef AN_DEVELOPMENT
	syncJobOnThread[currentThreadId] = false;
#endif

	syncJobThreadId = NONE;
	syncJob.allow();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	{
		Concurrency::ScopedSpinLock lock(currentSyncJobInfoLock);
		currentSyncJobInfo = String::empty();
	}
#endif
}

void Game::WorldManager::perform_async_job(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	MEASURE_PERFORMANCE(worldManagerPerformAsyncJob);
	WorldAsyncJob* data = plain_cast<WorldAsyncJob>(_data);
	//
	int currentThreadId = ::Concurrency::ThreadManager::get_current_thread_id();
	data->game->worldManager.asyncJobOnThread[currentThreadId] = true;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	{
		Concurrency::ScopedSpinLock lock(data->game->worldManager.currentAsyncJobInfoLock);
		data->game->worldManager.currentAsyncJobInfo = data->jobName;
	}
#endif
	//
	scoped_call_stack_info(TXT("async world job"));
	scoped_call_stack_info(data->jobName);

#ifdef AN_DEBUG_WORLD_MANAGER_JOBS
	output(TXT("[world manager async job] %S"), data->jobName ? data->jobName : TXT("[unnamed]"));
#endif

	{
#ifdef AN_PERFORMANCE_MEASURE
		tchar const * jobName = data->jobName;
		MEASURE_PERFORMANCE_STR_COLOURED(jobName, Colour::zxMagenta);
#endif
		data->func();
	}
	//
	data->game->worldManager.asyncJobOnThread[currentThreadId] = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	{
		Concurrency::ScopedSpinLock lock(data->game->worldManager.currentAsyncJobInfoLock);
		data->game->worldManager.currentAsyncJobInfo = String::empty();
	}
#endif
	//
	if (!data->game->runningSingleThreaded)
	{
		data->game->worldManager.asyncJob.release();
	}
	delete data;
}

void Game::WorldManager::cancel_all()
{
	Concurrency::ScopedSpinLock lock(worldJobsLock);
	worldJobs.clear();
}

void Game::WorldManager::advance_jobs()
{
	advance_immediate_jobs();
	advance_world_jobs(WorldJob::All);
}

void Game::WorldManager::advance_immediate_jobs()
{
	scoped_call_stack_info(TXT("Game::WorldManager::advance_immediate_jobs"));

	PERFORMANCE_GUARD_LIMIT(0.002f, TXT("world manager advance - immediate jobs"));
	// do all immediate jobs, no matter if async jobs are in progress
	while (!immediateJobs.is_empty())
	{
		WorldJob nextJob;
		{
			Concurrency::ScopedSpinLock lock(immediateJobsLock);
			if (!immediateJobs.is_empty())
			{
				nextJob = immediateJobs[0];
				immediateJobs.remove_at(0);
			}
		}
		if (!nextJob.func)
		{
			break;
		}

		an_assert(nextJob.type == WorldJob::Sync);

		{
			// sync
			scoped_call_stack_info(TXT("Game::WorldManager::advance_immediate_jobs"));
			perform_sync_job(nextJob.jobName, nextJob.func);
		}
	}
}

void Game::WorldManager::advance_time_unoccupied()
{
	if (is_busy() || has_something_to_do())
	{
		timeUnoccupied.clear();
	}
	else if (! timeUnoccupied.is_set())
	{
		timeUnoccupied = ::System::TimeStamp();
	}
}

void Game::WorldManager::advance_world_jobs(WorldJob::TypeFlags _flags)
{
	scoped_call_stack_info(TXT("Game::WorldManager::advance_world_jobs"));
	PERFORMANCE_GUARD_LIMIT(0.002f, TXT("world manager advance - advance"));

	advance_time_unoccupied(); // always call it on world jobs advancing

	if (asyncJob.is_locked() && !game->runningSingleThreaded)
	{
		timeUnoccupied.clear();
		// async job in progress
		return;
	}

	{
		Concurrency::ScopedSpinLock lock(worldJobsLock);
		if (worldJobs.is_empty())
		{
			isBusy = false;
			return;
		}
	}

	isBusy = true;
	timeUnoccupied.clear();

	if (!game->does_allow_async_jobs())
	{
		// try to remove temporary objects first
		return;
	}

	int syncJobsLeft = 3;
	while (true)
	{
		WorldJob nextJob;
		{
			Concurrency::ScopedSpinLock lock(worldJobsLock);
			if (!worldJobs.is_empty())
			{
				nextJob = worldJobs[0];
				if (!(nextJob.type & _flags))
				{
					// don't do it now
					break;
				}
				worldJobs.remove_at(0);
			}
		}
		if (!nextJob.func)
		{
			break;
		}

		isBusy = true;
		if (nextJob.type == WorldJob::Async)
		{
			// async, stop for a while
			if (game->runningSingleThreaded)
			{
				nextJob.func();
			}
			else
			{
				asyncJob.acquire();
				NAME_POOLED_OBJECT_IF_NOT_NAMED(WorldAsyncJob);
				game->get_job_system()->do_asynchronous_job(game->game_background_jobs(), perform_async_job, new WorldAsyncJob(game, nextJob.func, nextJob.jobName));
			}
			break;
		}
		else
		{
			--syncJobsLeft;
			// sync
			scoped_call_stack_info(TXT("perform_sync_job via advance world jobs"));
			perform_sync_job(nextJob.jobName, nextJob.func);
			// in performance mode there is a limit of sync jobs
			if (inPerformanceMode && syncJobsLeft > 0)
			{
				break;
			}
		}
	}
}

bool Game::is_option_set(Name const & _option) const
{
	if (_option == TXT("vr")) return does_use_vr();

	return MainConfig::global().is_option_set(_option) ||
		   GameConfig::get()->is_option_set(_option);
}

int Game::get_unique_id()
{
	Concurrency::ScopedSpinLock lock(nextUniqueIDLock);

	if (nextUniqueID == 0)
	{
		++nextUniqueID;
	}
	int result = nextUniqueID;
	++nextUniqueID;
	return result;
}

void Game::request_removing_unused_temporary_objects(Optional<int> const & _batchSize)
{
	if (! removeUnusedTemporaryObjectsFromLibrary)
	{
		if (!removeUnusedTemporaryObjectsFromLibraryRequested)
		{
			removeUnusedTemporaryObjectsFromLibraryRequested = true;
			output_colour(1, 1, 0, 0);
			output(TXT("request removal of unused temporary objects"));
			output_colour();
		}
	}
	removeUnusedTemporaryObjectsFromLibraryBatchSize = _batchSize.is_set() && _batchSize.get() >= 1 ? _batchSize.get() : 10;
	removeUnusedTemporaryObjectsFromLibraryRequested = true;
}

#ifdef AN_PERFORMANCE_MEASURE
void Game::show_performance_in_frames(int _frames)
{
	showPerformanceAfterFrames = _frames;
	showPerformanceAfterFramesStartedAt = _frames;
}
#endif

void Game::change_show_debug_panel(DebugPanel* _change, bool _vr)
{
	if (_vr)
	{
		showDebugPanelVR = showDebugPanelVR != _change ? _change : nullptr;
		if (showDebugPanelVR)
		{
			::System::Core::pause_vr(bit(3));
		}
		else
		{
			::System::Core::unpause_vr(bit(3));
		}
	}
	else
	{
		showDebugPanel = showDebugPanel != _change ? _change : nullptr;
	}
#ifdef AN_PERFORMANCE_MEASURE
	blockAutoStopFor = 1.0f;
#endif
}

void Game::change_show_log_panel(DebugPanel* _change)
{
	showLogPanelVR = showLogPanelVR != _change ? _change : nullptr;
}

ActivationGroup* Game::get_current_activation_group()
{
	auto* ag = get_current_activation_group_if_present();
	an_assert(ag, TXT("did you forget to create/push new activation group first?"));
	return ag;
}

ActivationGroup* Game::get_current_activation_group_if_present()
{
	auto & agStack = activationGroupStacks[Concurrency::ThreadManager::get_current_thread_id()];
	return agStack.activationGroups.is_empty() ? nullptr : agStack.activationGroups.get_first().get();
}

void Game::push_activation_group(ActivationGroup* _ag)
{
	an_assert(_ag);
	auto & agStack = activationGroupStacks[Concurrency::ThreadManager::get_current_thread_id()];
#ifdef INSPECT_ACTIVATION_GROUP
	output(TXT("[ActivationGroup] Game::push_activation_group [%i]"), _ag ? _ag->get_id() : NONE);
#endif
	agStack.activationGroups.push_back(	ActivationGroupPtr(_ag));
}

ActivationGroupPtr Game::pop_activation_group(OPTIONAL_ ActivationGroup* _ag)
{
#ifdef INSPECT_ACTIVATION_GROUP
	output(TXT("[ActivationGroup] Game::pop_activation_group [%i]"), _ag ? _ag->get_id() : NONE);
#endif
	auto & agStack = activationGroupStacks[Concurrency::ThreadManager::get_current_thread_id()];
	ActivationGroupPtr popped = agStack.activationGroups.get_last();
	an_assert(!_ag || popped == _ag);
	agStack.activationGroups.pop_back();
	return popped;
}

ActivationGroupPtr Game::push_new_activation_group()
{
	auto & agStack = activationGroupStacks[Concurrency::ThreadManager::get_current_thread_id()];
	ActivationGroupPtr newAG;
	newAG = new ActivationGroup();
	agStack.activationGroups.push_back(newAG);
#ifdef INSPECT_ACTIVATION_GROUP
	output(TXT("[ActivationGroup] Game::push_new_activation_group [%i]"), newAG->get_id());
#endif
	return newAG;
}

bool Game::should_process_object_creation(DelayedObjectCreation const* _doc) const
{
	if ((_doc->priority < 0 && should_skip_delayed_objects_with_negative_priority())
#ifdef AN_DEVELOPMENT
#ifndef AN_OVERRIDE_DEV_SKIPPING_DELAYED_OBJECT_CREATIONS
		|| _doc->devSkippable
#endif
#endif
		)
	{
		return false;
	}
	return true;
}

bool Game::has_any_delayed_object_creation_pending() const
{
	if (!delayedObjectCreations.is_empty())
	{
		return true;
	}
	if (delayedObjectCreationInProgress.is_set())
	{
		return true;
	}
	return false;
}

bool Game::has_any_delayed_object_creation_pending(Room* _room)
{
	if (!delayedObjectCreations.is_empty())
	{
		Concurrency::ScopedSpinLock lock(delayedObjectCreationsLock, true);
		for (int i = 0; i < delayedObjectCreations.get_size() && !does_want_to_cancel_creation(); ++i)
		{
			auto* doc = delayedObjectCreations[i].get();
			if (doc->inRoom == _room)
			{
				return true;
			}
		}
	}
	if (delayedObjectCreationInProgress.is_set())
	{
		// use RefCountObjectPtr as it may disappear
		RefCountObjectPtr<DelayedObjectCreation> docptr;
		docptr = delayedObjectCreationInProgress.get();
		if (auto* doc = docptr.get())
		{
			if (doc->inRoom == _room)
			{
				return true;
			}
		}
	}
	return false;
}

void Game::queue_delayed_object_creation(DelayedObjectCreation* _delayed, bool _allowInstantObjectCreation)
{
	if (!should_process_object_creation(_delayed))
	{
		RefCountObjectPtr<DelayedObjectCreation> doc(_delayed);
#ifdef OUTPUT_GENERATION
		if (_delayed->objectType)
		{
			output(TXT("ignored delayed creation (%p) of \"%S\" named \"%S\""), _delayed, _delayed->objectType->get_name().to_string().to_char(), _delayed->name.to_char());
		}
		else
		{
			output(TXT("ignored delayed creation (%p)"), _delayed);
		}
#endif
		return;
	}
	an_assert(_delayed->inRoom.is_set());
	if (forceInstantObjectCreation && _allowInstantObjectCreation)
	{
		// use ptr to auto destroyed if not held outside
		RefCountObjectPtr<DelayedObjectCreation> doc(_delayed);
		doc->process(this, true);
	}
	else
	{
#ifdef OUTPUT_GENERATION
		if (_delayed->objectType)
		{
			output(TXT("queueing delayed creation (%p) of \"%S\" named \"%S\""), _delayed, _delayed->objectType->get_name().to_string().to_char(), _delayed->name.to_char());
		}
		else
		{
			output(TXT("queueing delayed creation (%p)"), _delayed);
		}
#endif
		Concurrency::ScopedSpinLock lock(delayedObjectCreationsLock, true);
		int insertAt = NONE;
		for_every_ref(doc, delayedObjectCreations)
		{
			if (doc->priority < _delayed->priority)
			{
				insertAt = for_everys_index(doc);
				break;
			}
		}
		if (insertAt != NONE)
		{
			delayedObjectCreations.insert_at(insertAt, RefCountObjectPtr<DelayedObjectCreation>(_delayed));
		}
		else
		{
			delayedObjectCreations.push_back(RefCountObjectPtr<DelayedObjectCreation>(_delayed));
		}
	}
}

void Game::drop_all_delayed_object_creations()
{
	Concurrency::ScopedSpinLock lock(delayedObjectCreationsLock, true);
	delayedObjectCreations.clear();
}

void Game::done_delayed_object_creation(DelayedObjectCreation* _delayed)
{
	an_assert(delayedObjectCreationInProgress.get() == _delayed);
	delayedObjectCreationInProgress = nullptr;
}

bool Game::should_input_be_grabbed() const
{
#ifdef AN_PERFORMANCE_MEASURE
	return !showPerformance && is_input_grabbed() && showDebugPanel == nullptr;
#else
	return is_input_grabbed();
#endif
}

void Game::handle_vr_all_time_controls_input()
{
	bool resetImmobileOrigin = false;
	if (does_use_vr())
	{
		if (allTimeControlsInput.has_button_been_pressed(NAME(resetImmobileOriginVR)))
		{
			resetImmobileOrigin = true;
		}
	}
	if (MainConfig::global().should_be_immobile_vr())
	{
		if (auto* vr = VR::IVR::get())
		{
			if (vr->get_controls().requestRecenter)
			{
				resetImmobileOrigin = true;
			}
		}
	}
	if (resetImmobileOrigin)
	{
		reset_immobile_origin_in_vr_space_for_sliding_locomotion();
	}
}

void Game::there_is_longer_frame_to_ignore()
{
	ModulePresence::ignore_auto_velocities();
	ignoreLongerFrame = true;
}

bool Game::should_generate_more_details() const
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (generateMoreDetails.is_set())
	{
		return generateMoreDetails.get();
	}
#endif
	return generateMoreDetailsBySystem;
}

void Game::render_nothing_to_output()
{
	auto* v3d = ::System::Video3D::get();

	::System::RenderTarget::bind_none();

	v3d->setup_for_2d_display();
	v3d->set_default_viewport();
	v3d->set_near_far_plane(0.02f, 100.0f);

	v3d->set_2d_projection_matrix_ortho();
	v3d->access_model_view_matrix_stack().clear();
	
	v3d->clear_colour(Colour::black);
}

void Game::render_main_render_output_to_output()
{
	if (does_use_vr())
	{
		render_vr_output_to_output();
	}
	else
	{
		if (auto* rt = get_main_render_output_render_buffer()) // if not available, means we don't use it and already rendered stuff
		{
			auto* v3d = ::System::Video3D::get();

			System::RenderTarget::bind_none();

			v3d->setup_for_2d_display();
			v3d->set_default_viewport();
			v3d->set_near_far_plane(0.02f, 100.0f);
			v3d->clear_colour(Colour::black);

			// restore projection matrix
			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			Vector2 leftBottom;
			Vector2 size;
			VectorInt2 targetSize = MainConfig::global().get_video().resolution;
			::System::RenderTargetUtils::fit_into(rt->get_full_size_for_aspect_ratio_coef(), targetSize, OUT_ leftBottom, OUT_ size);
			rt->render(0, ::System::Video3D::get(), leftBottom, size);

			System::RenderTarget::unbind_to_none();
		}
	}
}

void Game::render_vr_output_to_output()
{
	if (!does_use_vr() || !should_render_to_main_for_vr())
	{
		return;
	}
	if (VR::IVR::get()->get_available_functionality().renderToScreen)
	{
		VR::IVR::get()->render_one_output_to_main(::System::Video3D::get(), VR::Eye::OnMain);
	}
}

bool Game::is_using_sliding_locomotion()
{
#ifdef TEST_NOT_SLIDING_LOCOMOTION
	return false;
#endif
	if (!VR::IVR::get()->can_be_used()) return true;
	if (auto* g = Game::get()) return g->usingSlidingLocomotion;
	return false;
}

void Game::update_use_sliding_locomotion()
{
	bool slidingLocomotion = MainConfig::global().should_be_immobile_vr();
	if (!VR::IVR::get()->can_be_used()) slidingLocomotion = true;
#ifdef TEST_NOT_SLIDING_LOCOMOTION
	slidingLocomotion = false;
#endif
	set_use_sliding_locomotion(slidingLocomotion);
}

void Game::set_use_sliding_locomotion(bool _use)
{
	if (auto* g = Game::get_as<Game>())
	{
		g->use_sliding_locomotion_internal(_use);
		g->usingSlidingLocomotion = _use;
	}
}

void Game::use_sliding_locomotion_internal(bool _use)
{
	usingSlidingLocomotion = _use;
}

bool Game::reset_immobile_origin_in_vr_space_for_sliding_locomotion()
{
	if (is_using_sliding_locomotion())
	{
		if (auto* vr = VR::IVR::get())
		{
			vr->reset_immobile_origin_in_vr_space();
			return true;
		}
	}
	return false;
}

void Game::set_wants_to_cancel_creation(bool _wantsToCancelCreation)
{
	if (_wantsToCancelCreation)
	{
		output(TXT("[wants to cancel] request cancelling creation"));
	}
	else
	{
		output(TXT("[wants to cancel] allow creation"));
	}
	if (wantsToCancelCreation != _wantsToCancelCreation)
	{
		output(TXT("[wants to cancel] changed to: %S"), _wantsToCancelCreation? TXT("cancel creation") : TXT("allow creation"));
	}
	wantsToCancelCreation = _wantsToCancelCreation;
}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
void Game::clear_external_view_clip_planes()
{
	externalViewClipPlanes.clear();
	externalViewClipPlanesReadyForRendering.clear();
	externalViewClipPlanesExclude.clear();
	externalViewClipPlanesExcludeReadyForRendering.clear();
}

void Game::set_external_view_clip_planes(int _planeCount, Plane const* _planes, int _planeExcludeCount, Plane const* _planesExclude)
{
	externalViewClipPlanes.clear();
	externalViewClipPlanesExclude.clear();
	if (_planeCount && _planes)
	{
		Plane const* p = _planes;
		for_count(int, i, _planeCount)
		{
			externalViewClipPlanes.add_as_is(*p);
			++p;
		}
	}
	if (_planeExcludeCount && _planesExclude)
	{
		Plane const* p = _planesExclude;
		for_count(int, i, _planeExcludeCount)
		{
			externalViewClipPlanesExclude.add_as_is(*p);
			++p;
		}
	}
	externalViewClipPlanesReadyForRendering.clear();
	externalViewClipPlanesExcludeReadyForRendering.clear();
}
#endif

void Game::GameIcon::advance(float _offset, float _deltaTime)
{
	if (alpha >= 1.0f)
	{
		activeFor += _deltaTime;
		keepActiveFor = max(0.0f, keepActiveFor - _deltaTime);
	}
	if (active && autoDeactivate && keepActiveFor == 0.0f)
	{
		if (alpha >= 1.0f)
		{
			active = false;
		}
	}
	if (alpha <= 0.001f)
	{
		offset = _offset;
		alpha = 0.0f;
		inPlaceOffset = 1.0f;
	}
	else
	{
		offset = blend_to_using_time(offset, _offset, 0.2f, _deltaTime);
	}
	inPlaceOffset = blend_to_using_speed_based_on_time(inPlaceOffset, is_active() ? 0.0f : -1.0f, 0.2f, _deltaTime);
	alpha = blend_to_using_speed_based_on_time(alpha, is_active() ? 1.0f : 0.0f, 0.2f, _deltaTime);
}

void Game::game_icon_render_ui_on_main_render_output()
{
	MEASURE_PERFORMANCE(render_gameIcons);

	// get all icons
	float const thresholdActive = 0.001f;
	ARRAY_STACK(GameIcon*, icons, 4);
	icons.push_back(&gameIcons.savingIcon);
	icons.push_back(&gameIcons.inputMouseKeyboardIcon);
	icons.push_back(&gameIcons.inputGamepadIcon);
	icons.push_back(&gameIcons.screenshotIcon);

	for (int i = 0; i < icons.get_size(); ++i)
	{
		auto * icon = icons[i];
		if (!icon->icon.is_set() || (!icon->is_active() && icon->alpha <= thresholdActive))
		{
			icons.remove_at(i);
			--i;
		}
	}

	if (icons.is_empty())
	{
		return;
	}

	float offset = 0.0f;
	for_every_ptr(icon, icons)
	{
		if (icon->is_active())
		{
			offset += 1.0f;
		}
	}
	offset = -(max(0.0f, offset - 1.0f) * 0.5f);

	{
		float const deltaTime = ::System::Core::get_delta_time();
		for_every_ptr(icon, icons)
		{
			icon->advance(icon->is_active() ? offset : icon->offset, deltaTime);
			if (icon->is_active())
			{
				offset += 1.0f;
			}
		}
	}

	System::Video3D * v3d = System::Video3D::get();

	for_count(int, renderForVR, does_use_vr() ? 2 : 1)
	{
		if (!should_render_to_main_for_vr() && does_use_vr() &&
			renderForVR == 0)
		{
			// skip normal rendering, go straight to vr
			continue;
		}
		for (int eyeIdx = 0; eyeIdx < (renderForVR ? 2 : 1); ++eyeIdx)
		{
			Vector2 rtSize = v3d->get_screen_size().to_vector2();
			VR::Eye::Type eye = (VR::Eye::Type)eyeIdx;
			Vector2 atScale(Vector2::one * 0.5f); // to make render place be from -1,-1 to 1,1
			Vector2 atAnchor = rtSize * 0.5f;
			float atEdge = 0.8f;
			bool forceBottom = false;
			System::RenderTarget* rt = get_main_render_output_render_buffer();
			if (renderForVR && VR::IVR::get()->get_output_render_target(eye))
			{
				rt = VR::IVR::get()->get_output_render_target(eye);
			}
			if (rt)
			{
				rt->bind();
				rtSize = rt->get_size().to_vector2();
			}
			else
			{
				System::RenderTarget::bind_none();
			}

			if (renderForVR)
			{
				atScale = Vector2::one * 0.5f;
				auto& vrri = VR::IVR::get()->get_render_info();
				atAnchor = rtSize * 0.5f + vrri.lensCentreOffset[eyeIdx] * rtSize * 0.5f;
				atAnchor.x += rtSize.x * ((tan_deg(5.0f / 2.0f) / tan_deg(vrri.fov[eyeIdx] / 2.0f)) * vrri.aspectRatio[eyeIdx]) * (eyeIdx == 0 ? 1.0f : -1.0f); // pretend to be closer to eyes
				atEdge = 0.3f;
				forceBottom = true;
			}


			// ready to draw icons

			v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2());
			v3d->setup_for_2d_display();
			v3d->set_2d_projection_matrix_ortho(rtSize);
			v3d->access_model_view_matrix_stack().clear();

			Vector2 availableSize = rtSize * atScale * 0.5f; // to get only available size

			atScale *= rtSize; // bring it into proper 

			todo_future(TXT("we use saving icon as reference for icon size"));
			float iconSize = gameIcons.savingIcon.icon.is_set() ? (availableSize.x > availableSize.y ? gameIcons.savingIcon.icon->get_size().x : gameIcons.savingIcon.icon->get_size().y) : 32.0f;
			float iconSpacing = round(iconSize * 0.25f);
			float iconOffset = iconSize + iconSpacing;

			float scale = 1.0f;
			float size = iconSize;
			while (size * scale < min(availableSize.x, availableSize.y) * 0.2f)
			{
				scale += 1.0f;
			}

			// draw them!
			for_every_ptr(icon, icons)
			{
				Vector2 at = atAnchor;
				Framework::TexturePartDrawingContext dc;
				dc.scale = Vector2(scale, scale);
				//dc.colour.r = Random::get_chance(0.5f) ? 1.0f : 0.0f;
				//dc.colour.g = Random::get_chance(0.5f) ? 1.0f : 0.0f;
				//dc.colour.b = Random::get_chance(0.5f) ? 1.0f : 0.0f;
				dc.colour.a = icon->alpha;
				dc.blend = true;
				if (availableSize.x > availableSize.y || forceBottom)
				{
					// bottom centre
					at += atScale * Vector2(0.0f, -atEdge);
					at.x -= scale * icon->icon->get_size().x * 0.5f; // do not use anchor here as anchor is pixel aligned
					at.x += scale * icon->offset * iconOffset;
					at.y += scale * icon->inPlaceOffset * iconSize;
				}
				else
				{
					// left middle
					at += atScale * Vector2(-atEdge, 0.0f);
					at.y -= scale * icon->icon->get_size().y * 0.5f; // do not use anchor here as anchor is pixel aligned
					at.y += scale * icon->offset * iconOffset;
					at.x += scale * icon->inPlaceOffset * iconSize;
				}
				icon->icon->draw_at(v3d, at, dc);
			}

			System::RenderTarget::unbind_to_none();
		}
	}
}

void Game::check_input_for_icons()
{
	bool newMouseKeyboardActive = gameIcons.mouseKeyboardActive;
	if (!gameIcons.mouseKeyboardActive)
	{
		if (System::Input::get()->is_keyboard_active() ||
			System::Input::get()->is_mouse_button_active())
		{
			newMouseKeyboardActive = true;
		}
		else
		{
			gameIcons.mouseInactiveTime += get_delta_time();
			Vector2 mouseMovementThisFrame = System::Input::get()->get_mouse_relative_location();
			gameIcons.mouseMovementSinceLongerInactive += mouseMovementThisFrame;
			float const mouseActiveThreshold = 1.0f;
			if (mouseMovementThisFrame.length() > mouseActiveThreshold ||
				gameIcons.mouseMovementSinceLongerInactive.length() > mouseActiveThreshold)
			{
				newMouseKeyboardActive = true;
			}
			if (gameIcons.mouseInactiveTime > 0.5f)
			{
				// with this timer we require movement to be fast, not short one
				gameIcons.mouseMovementSinceLongerInactive = Vector2::zero;
			}
		}
	}
	else
	{
		if (System::Input::get()->is_gamepad_active())
		{
			newMouseKeyboardActive = false;
		}
	}
	gameIcons.currentInputActiveTime += get_delta_time();
	if (newMouseKeyboardActive != gameIcons.mouseKeyboardActive || gameIcons.currentInputInfoPending)
	{
		if (gameIcons.currentInputActiveTime > 0.2f)
		{
			gameIcons.mouseKeyboardActive = newMouseKeyboardActive;
			gameIcons.currentInputActiveTime = 0.0f;
			if (gameIcons.mouseKeyboardActive)
			{
				gameIcons.inputMouseKeyboardIcon.activate(1.0f, true);
				gameIcons.inputGamepadIcon.deactivate(true);
			}
			else
			{
				gameIcons.inputGamepadIcon.activate(1.0f, true);
				gameIcons.inputMouseKeyboardIcon.deactivate(true);
			}
			gameIcons.currentInputInfoPending = false;
		}
		else
		{
			gameIcons.currentInputInfoPending = true;
		}
	}
}

void Game::async_add_generic_object(AddGenericObjectParams const& _params)
{
	Object* object = nullptr;
	ObjectType* objectType = _params.objectType;

	if (!objectType)
	{
		objectType = get_object_type_for(_params);
	}

	if (!objectType)
	{
		objectType	= Library::get_current()->get_global_references().get<SceneryType>(NAME(grGenericScenery));
	}

	objectType->load_on_demand_if_required();

	perform_sync_world_job(TXT("spawn generic object"), [&object, objectType, &_params]()
		{
			object = objectType->create(! _params.name.is_empty()? _params.name : String::printf(TXT("generic object")));
			object->init(_params.room->get_in_sub_world());
		});

	Transform placement = _params.placement;

	if (_params.randomGenerator.is_set())
	{
		object->access_individual_random_generator() = _params.randomGenerator.get();
	}
	else
	{
		object->access_individual_random_generator() = Random::Generator();
	}

	if (_params.tags)
	{
		object->access_tags().set_tags_from(*_params.tags);
	}

	_params.room->collect_variables(object->access_variables());

	if (_params.variables)
	{
		object->access_variables().set_from(*_params.variables);
	}

	object->initialise_modules([&_params](Framework::Module* _module)
		{
			if (_params.mesh)
			{
				if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
				{
					moduleAppearance->use_mesh(_params.mesh);
				}
			}
			else if (_params.meshGenerator)
			{
				if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
				{
					moduleAppearance->use_mesh_generator_on_setup(_params.meshGenerator);
				}
			}
		});

	perform_sync_world_job(TXT("place generic object"), [&object, &_params, &placement]()
		{
			object->get_presence()->place_in_room(_params.room, placement);
		});

	on_newly_created_object(object);
}

bool Game::is_input_grabbed() const
{
	if (auto* sl = sceneLayerStack.get())
	{
		Optional<bool> mg = sl->is_input_grabbed();
		if (mg.is_set())
		{
			return mg.get();
		}
	}
	return mouseGrabbed;
}

#ifdef WITH_IN_GAME_EDITOR
bool Game::does_want_to_switch_editor() const
{
	if (auto* input = ::System::Input::get())
	{
		if (input->is_key_pressed(System::Key::LeftCtrl) || input->is_key_pressed(System::Key::RightCtrl))
		{
			if (input->has_key_been_pressed(System::Key::Tilde))
			{
				return true;
			}
		}
	}

	return false;
}

bool Game::show_editor_if_requested()
{
	if (does_want_to_switch_editor())
	{
		show_editor();
		return true;
	}
	return false;
}

void Game::show_editor()
{
	start_short_pause();

	EditorContext editorContext;

	editorContext.edit(nullptr);
	editorContext.load_last_workspace_or_default();

	debug_renderer_mode(DebugRendererMode::OnlyActiveFilters);

	while (true)
	{
		// advance system
		::System::Core::set_time_multiplier();
		::System::Core::advance();

		float const deltaTime = ::System::Core::get_delta_time();

		debug_renderer_next_frame(::System::Core::get_raw_delta_time());

		if (jobSystem)
		{
			scoped_call_stack_info(TXT("-> clean up system jobs"));
			jobSystem->clean_up_finished_off_system_jobs();
			jobSystem->access_advance_context().set_delta_time(deltaTime);
			jobSystem->execute_main_executor(1); // execute one job to update screen
		}

		if (auto* ss = ::System::Sound::get())
		{
			scoped_call_stack_info(TXT("-> sound update"));
ss->update(::System::Core::get_raw_delta_time());
		}

		advance_controls();

		// grab input if editor wants to do so
		if (auto* editor = editorContext.get_editor())
		{
			::System::Input::get()->grab(editor->is_input_grabbed().get(false));
			RefCountObjectPtr<Framework::Editor::Base> keepEditor(editor);
			editor->process_controls();
		}

		advance_fading();

		advance_later_loading();

		debug_renderer_not_yet_rendered();

		// display things
		{
			WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

			if (auto* editor = editorContext.get_editor())
			{
				editor->pre_render(nullptr);
			}

			System::Video3D* v3d = System::Video3D::get();

			{	// display buffer

				// update
				{
					v3d->update();
				}
			}

			if (auto* editor = editorContext.get_editor())
			{
				// to get_main_render_buffer
				editor->render_main(nullptr);
				editor->render_debug(nullptr);
			}

			::System::RenderTarget::unbind_to_none();

			// do post process if required, instead of copying
			{
				System::RenderTarget* rtSrc = get_main_render_buffer();
				System::RenderTarget* rtDest = get_main_render_output_render_buffer();
				if (rtSrc && rtSrc != rtDest)
				{
					System::Video3D* v3d = System::Video3D::get();
					rtSrc->resolve_forced_unbind();
					if (rtDest)
					{
						rtDest->bind();
					}
					else
					{
						System::RenderTarget::bind_none();
					}
					setup_to_render_to_render_target(v3d, rtDest);

					rtSrc->render(0, v3d, Vector2::zero, v3d->get_viewport_size().to_vector2());

					::System::RenderTarget::unbind_to_none();
				}
			}

			if (auto* editor = editorContext.get_editor())
			{
				// to get_main_render_output_render_buffer
				editor->render_ui(nullptr);
			}

			System::RenderTarget::unbind_to_none();

			render_main_render_output_to_output();

			debug_renderer_render(::System::Video3D::get());	// ready renderer for next frame

			apply_fading_to(nullptr);

			{
				MEASURE_PERFORMANCE(v3d_displayBuffer);
				v3d->display_buffer();
			}
		}

		debug_renderer_undefine_contexts();
		debug_renderer_already_rendered();

		if (does_want_to_switch_editor())
		{
			break;
		}
	}

	{
		Loader::Circles loaderCircles;
		do_in_background_while_showing_loader(loaderCircles,
			[this, &editorContext]()
			{
				editorContext.save_workspace();

				background_post_switch_editor();
			});


		while (true)
		{
			advance_and_display_loader(&loaderCircles);

			if (jobSystem->get_asynchronous_list(system_jobs())->has_finished() &&
				jobSystem->get_asynchronous_list(background_jobs())->has_finished() &&
				jobSystem->get_asynchronous_list(loading_worker_jobs())->has_finished() &&
				jobSystem->get_asynchronous_list(preparing_worker_jobs())->has_finished() &&
				jobSystem->get_asynchronous_list(async_worker_jobs())->has_finished())
			{
				break;
			}
		}
	}

#ifdef SPRITE_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING
	BasicDrawRenderingBuffer::force_rerendering_of_all();
#endif

	end_short_pause();
}

void Game::background_post_switch_editor()
{
	Library::get_current()->access_sprite_texture_processor().add_job_update_and_load_if_required();
#ifdef BASIC_DRAW_RENDERING_BUFFER__ALLOW_FORCED_RERENDERING
	BasicDrawRenderingBuffer::force_rerendering_of_all();
#endif
}

#endif

void Game::on_newly_created_library_stored_for_editor(LibraryStored* _libraryStored)
{
	if (auto* vm = fast_cast<VoxelMold>(_libraryStored))
	{
		for_every_ptr(cp, Library::get_current()->get_colour_palettes().get_tagged(TagCondition(NAME(defaultColourPalette).to_string())))
		{
			VoxelMoldContentAccess::set_colour_palette(vm, cp);
			break; // first one is enough
		}
	}
	if (auto* vm = fast_cast<Sprite>(_libraryStored))
	{
		for_every_ptr(cp, Library::get_current()->get_colour_palettes().get_tagged(TagCondition(NAME(defaultColourPalette).to_string())))
		{
			SpriteContentAccess::set_colour_palette(vm, cp);
			break; // first one is enough
		}
	}
}

void Game::do_in_background_while_showing_loader(Loader::ILoader& _showLoader, std::function<void()> _do)
{
	async_background_job(this, _do);

	while (true)
	{
		advance_and_display_loader(&_showLoader);

		if (jobSystem->get_asynchronous_list(system_jobs())->has_finished() &&
			jobSystem->get_asynchronous_list(background_jobs())->has_finished() &&
			jobSystem->get_asynchronous_list(loading_worker_jobs())->has_finished() &&
			jobSystem->get_asynchronous_list(preparing_worker_jobs())->has_finished() &&
			jobSystem->get_asynchronous_list(async_worker_jobs())->has_finished())
		{
			break;
		}
	}
}

