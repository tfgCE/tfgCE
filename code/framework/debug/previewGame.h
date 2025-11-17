#pragma once

#include <functional>

#include "..\..\core\concurrency\gate.h"
#include "..\..\core\math\math.h"
#include "..\..\core\io\dir.h"
#include "..\..\core\debug\logInfoContext.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\render\renderCamera.h"

namespace Framework
{
	class Environment;
	class Object;
	class Room;
	class SubWorld;
	class TemporaryObject;
	class World;

	namespace AI
	{
		class Locomotion;
	};

	namespace Render
	{
		class Scene;
	};

	namespace Sound
	{
		class Scene;
	};

	namespace PreviewGameMode
	{
		enum Type
		{
			Normal,
			Quick,
			//
			MAX,
			First = 0,
		};
	};

	struct PreviewVRControllers
	{
		bool inUse = true;
		bool absolutePlacement = false;
		Transform head = Transform(Vector3(0.0f, 0.0f, 1.5f), Quat::identity);
		Transform hands[2] = { Transform(Vector3(-0.2f, 0.5f, 1.0f), Rotator3(0.0f, 0.0f, -90.0f).to_quat())
							 , Transform(Vector3( 0.2f, 0.5f, 1.0f), Rotator3(0.0f, 0.0f,  90.0f).to_quat()) };
	};

	class PreviewGame
	: public Game
	{
		FAST_CAST_DECLARE(PreviewGame);
		FAST_CAST_BASE(Game);
		FAST_CAST_END();

		typedef Game base;
	public:
		typedef std::function< void() > CreateLibraryFunc;

		template <typename LibraryClass>
		static bool run_if_requested(String const & _byLookingIntoConfigFile, std::function<void(Game*)> _on_game_started = nullptr);

		PreviewGame(CreateLibraryFunc _createLibraryFunc);
		virtual ~PreviewGame();

		void set_started_using_preview_config_file(String const & _configFile) { startedUsingPreviewConfigFile = _configFile; }

		bool does_want_to_quit() const { return wantsToQuit; }

		bool load_preview_library(LibraryLoadLevel::Type _startAtLibraryLoadLevel);
		void close_preview_library(LibraryLoadLevel::Type _closeAtLibraryLoadLevel);
		bool reload_preview_library();
		override_ void create_library();

		bool reload_as_needed(Optional<bool> _quick = NP);

		void create_world() {}
		void destroy_world() {}

		override_ void initialise_system();
		override_ void close_system();

		override_ void save_config_file() { /* do not save */ }

		override_ void before_game_starts();

		override_ void sound();
		override_ void render();

		override_ void pre_advance();

		override_ void game_loop();

		override_ String get_game_name() const
		{
			return String(TXT("preview"));
		}

		void clear_library_files() { useLibraryFiles.clear(); }
		void add_library_file(String const & _useTestLibraryFile) { useLibraryFiles.push_back(_useTestLibraryFile); }
		void clear_library_directories() { useLibraryDirectories.clear(); }
		void add_library_directory(String const & _useTestLibrary) { useLibraryDirectories.push_back(_useTestLibrary); }
		void no_library(bool _noLibrary) { noLibrary = _noLibrary; }

		void show_object(LibraryName const & _name, Name const & _type);
		void add_to_show_object_list(LibraryName const & _name, Name const & _type);

		void show_next_object_from_list();
		void show_previous_object_from_list();

		void store_lod_info(String const& _aiName, int _lodLevel, float _size, float _dist, float _aggressiveLOD, float _aggressiveLODStartOffset, int _triCount);

		PreviewVRControllers const& get_preview_vr_controllers() const { return previewVRControllers; }

	public:
		bool should_optimise_vertices() const { return optimiseVerticesAlways || !quickReloading; }

	private:
		CreateLibraryFunc createLibraryFunc;

		String startedUsingPreviewConfigFile;

		Array<String> useLibraryFiles;
		Array<String> useLibraryDirectories;
		bool noLibrary = false;

		bool loadLibraryResult;
		bool prepareLibraryResult;

		bool wantsToQuit;

		bool optimiseVerticesAlways = false;
		bool quickReloading = false;

		bool droppedTools = false;

		Concurrency::SpinLock dirEntriesLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		Array<IO::DirEntry> dirEntries;
		bool reloadNeeded = true;
		bool reloadOnlyEnvironments = false;
		Optional<bool> quickReloadNeeded;
		int reloadStarting = 0; // to leave one frame to display info
		int blockAutoReloadsFor = 0;
		bool reloadInProgress = false;
		Concurrency::Semaphore checkReload;
		::System::TimeStamp checkReloadTS;

		Concurrency::Gate renderBlockGate; // means pre rendering is in progress and physical changes to world should not be allowed
		Concurrency::Gate soundBlockGate; // means pre rendering is in progress and physical changes to world should not be allowed

		int showInfo = 2; // the higher the number, the more info is displayed
		bool blockFilters = false;
		bool blockFilterNoise = false;
		int showGrid = 1; // 1xy 2xz 3yz
		int cameraMode = 0;
		Vector3 pivot;
		Vector3 pivotFollow;
		float distance;
		Range zRange;
		Rotator3 rotation;
		Transform cameraPlacement = Transform::identity;
		Render::Camera camera;
		float fov;

		Random::Generator mainRandomGenerator = Random::Generator(123123,546456);

		Vector2 textScale = Vector2::one;

		struct PreviewEnvironment
		{
			LibraryName useEnvironmentType;
			Array<LibraryName> addEnvironmentOverlays;
			EnvironmentType* environmentType = nullptr;
			Array<EnvironmentOverlay*> environmentOverlays;
			PreviewEnvironment(){}
			PreviewEnvironment(LibraryName const & _useEnvironmentType) { useEnvironmentType = _useEnvironmentType; }
		};
		Array<PreviewEnvironment> environments;
		int environmentIndex = 0;

		struct ShowObject
		{
			LibraryStored* object = nullptr;
			LibraryName name;
			Name type;
		};
		LibraryStored* prevShowObject = nullptr;
		ShowObject showObject;
		Array<ShowObject> showObjectList;
		LibraryName showObjectName;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		int prevPreviewLOD = NONE;
#endif

		Concurrency::SpinLock logInfoContextLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
		LogInfoContext logInfoContext;
		int logInfoLineIdx = 0;
		int logInfoLineStartIdx = 0;
		float linesStartAtY = 0.0f;
		float linesSeparation = 0.0f;
		float linesCharacterWidth = 0.0f;
		int visibleLogInfoLines = NONE;
		bool selectLogInfoLine = false;
		PreviewGameMode::Type mode = PreviewGameMode::Normal;
		bool autoReloads = false;
		Array<IO::DirEntry> quickInterestingDirEntries;
		Array<IO::DirEntry> environmentInterestingDirEntries;
		
		void process_controls();

		void close_post_process_chain();
		void create_post_process_chain();

		void update_interesting_dirs();

		override_ void load_library_and_prepare_worker(Concurrency::JobExecutionContext const * _executionContext, GameLoadingLibraryContext* _gameLoadingLibraryContext);
		static void check_library_change_worker_static(Concurrency::JobExecutionContext const * _executionContext, void* _data);

		void store_dir_entries();
		static void store_dir_entries(REF_ Array<IO::DirEntry> & _dirEntries, String const & _loadFromDir);

		PreviewEnvironment* add_environment_type(LibraryName const& _useEnvironmentType, bool _default = false) { environments.push_back(PreviewEnvironment(_useEnvironmentType)); if (_default) environmentIndex = environments.get_size() - 1;  return &environments.get_last();  }

		void post_reload();

	private:
		RefCountObjectPtr<Framework::Render::Scene> renderScene;

		Framework::Sound::Scene* soundScene = nullptr;

		Room* testRoom = nullptr;
		Room* testRoomForCustomPreview = nullptr;
		LibraryStored* testRoomForCustomPreviewForObject = nullptr;
		Room* testRoomForTemporaryObjects = nullptr;
		Room* testRoomForActualObjects = nullptr;
		int testEnvironmentMesh = 0;
		int noRoomMeshes = 0;
		SubWorld* testSubWorld = nullptr;
		World* testWorld = nullptr;
		Environment* testEnvironment = nullptr;
		Transform showObjectPlacement = Transform::identity;
		TemporaryObject* showTemporaryObject = nullptr;
		bool showTemporaryObjectSpawnCompanions = false;
		float showTemporaryObjectInactive = 0.0f;
		SafePtr<IModulesOwner> showActualObject;
		bool showActualObjectBeingGenerated = false;
		bool usePreviewAILogic = true;
		
		enum ShowControlPanel
		{
			None,
			Locomotion,
			VRControllers
		};
		ShowControlPanel showControlPanel = ShowControlPanel::None;
		ShowControlPanel visibleControlPanel = ShowControlPanel::None;

		PreviewVRControllers previewVRControllers;
		int previewVRBone = 0;

		int controllerTransformIdx = 0;

		struct DebugDraw
		{
			bool allObjects = true;

			bool presence = false;
			bool POIs = false;

			bool walker = false;

			bool skeleton = false;

			void draw(Framework::IModulesOwner* imo);
		} debugDraw;
		bool forceUpdateBlockFilters = false;

		struct UsedLODInfo
		{
			String aiName;
			int lod;
			float size;
			float dist;
			float aggressiveLOD;
			float aggressiveLODStartOffset;
			int triCount;

			static int compare(void const* _a, void const* _b)
			{
				UsedLODInfo const* a = (UsedLODInfo const*)(_a);
				UsedLODInfo const* b = (UsedLODInfo const*)(_b);
				return String::compare_icase_sort(&a->aiName, &b->aiName);
			}
		};
		Array<UsedLODInfo> usedLODInfo;
		Concurrency::SpinLock usedLODInfoLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

		struct PreviewWidget
		: public RefCountObject
		{
			Vector2 at; // centre - in pixels
			Vector2 size = Vector2(10.0f, 10.0f); // in pixels
			Colour colour = Colour::c64Red;
			String text;
			std::function< void(Vector2 const & _relClick) > onClick = nullptr;
		};
		Array<RefCountObjectPtr<PreviewWidget>> previewWidgets;
#ifdef AN_STANDARD_INPUT
		PreviewWidget* clickedPreviewWidget = nullptr;
#endif

		void destroy_test();

		ModuleController* get_show_actual_objects_controller() const;

		void find_environment_index(Framework::LibraryName const& _ln);
	};

};

#include "previewGame.inl"

