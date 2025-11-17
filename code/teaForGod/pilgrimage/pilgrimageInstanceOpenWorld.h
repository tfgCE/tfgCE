#pragma once

#include "pilgrimageDevice.h"
#include "pilgrimageInstance.h"
#include "pilgrimageOpenWorldDefinition.h"

#include "..\teaForGod.h"

#include "..\game\airshipObstacle.h"

#include "..\..\framework\video\texturePartUse.h"

#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\types\dirFourClockwise.h"
#include "..\..\core\system\video\renderTargetPtr.h"
#include "..\..\core\wheresMyPoint\simpleWMPOwner.h"

//

#ifdef BUILD_NON_RELEASE
	#define DUMP_CALL_STACK_INFO_IF_CREATING_CELL_TOO_LONG
#endif

//

struct Tags;
class TagCondition;

//

namespace Framework
{
	class Door;
	class LibraryBasedParameters;
	class LibraryStored;
	class Mesh;
	class RegionType;
	class RoomType;
};

namespace TeaForGodEmperor
{
	class LineModel;

	namespace GameScript
	{
		namespace Elements
		{
			class Pilgrimage;
		};
	};

	namespace PilgrimageInstanceOpenWorldConnectorTagDistribution
	{
		enum Type
		{
			Clockwise,
			CounterClockwise
		};
	};

	class PilgrimageInstanceOpenWorld
	: public PilgrimageInstance
	{
		FAST_CAST_DECLARE(PilgrimageInstanceOpenWorld);
		FAST_CAST_BASE(PilgrimageInstance);
		FAST_CAST_END();
		
		typedef PilgrimageInstance base;
	public:
#ifdef AN_ALLOW_AUTO_TEST
		static void check_all_cells(bool _check = true, bool _reset = true, bool _processAllJobs = false);
#endif

		static Name const& dir_to_connector_tag(DirFourClockwise::Type _dir);
		static Optional<DirFourClockwise::Type> connector_tag_to_dir(Name const& _connectorTag);
		static Name const& dir_to_layout_cue_tag(DirFourClockwise::Type _dir);
		static Name const& dir_to_layout_cue_cap_tag(DirFourClockwise::Type _dir);

	public:
		struct CellSetFunctionality
		{
			virtual ~CellSetFunctionality() {}

			virtual Framework::LibraryStored* get_as_any() const = 0;

			int get_cell_priority() const;
			float get_cell_distance_visibility_coef() const;
			bool should_avoid_same_neighbours() const;
			Tags const* get_cell_type() const;
			bool should_dir_align_dependent() const;
			bool should_axis_align_dependent() const;
			bool should_align_dependent_towards() const;
			bool should_align_dependent_away() const;
			Range2 get_local_centre_for_map() const;
			void get_local_side_exits_info_for_map(bool* _sideSensitiveExits, Range& _sideExitsOffset) const;
			Framework::LibraryBasedParameters const* choose_additional_cell_info(Random::Generator const& _seed) const;
			Optional<Colour> get_map_colour_for_map() const;
			LineModel const* get_line_model_for_map() const;
			LineModel const* get_line_model_for_map_additional(Framework::LibraryStored const * _additionalInfo) const;
			LineModel const* get_line_model_always_otherwise_for_map() const;
			TagCondition const* get_depends_on_cell_type() const;
			TagCondition const* get_dependent_cell_types() const;
			void fill_local_dependents(OUT_ int* _localDependents, OUT_ int& _dependentsRadius, Random::Generator const& _rg) const;
		};

		struct CellSet
		: public CellSetFunctionality
		{
			Framework::UsedLibraryStored<Framework::RoomType> roomType;
			Framework::UsedLibraryStored<Framework::RegionType> regionType;

			void set(Framework::RoomType* rt);
			void set(Framework::RegionType* rt);

			implement_ Framework::LibraryStored* get_as_any() const;
		};

		struct CellSetAny
		: public CellSetFunctionality
		{
			Framework::LibraryStored* any = nullptr;

			implement_ Framework::LibraryStored* get_as_any() const { return any; }
		};

		struct KnownPilgrimageDevice
		{
			Framework::UsedLibraryStored<PilgrimageDevice> device;
			int subPriority = 0;
			bool depleted = false;
			bool visited = false; // if we were in the same room and seen it
			Name groupId; // to auto deplete beyond a certain limit
			int useLimit = NONE; // copied from original
			SimpleVariableStorage stateVariables;

			static int compare(void const* _a, void const* _b);
		};

		struct CellQueryResult
		{
			VectorInt2 at = VectorInt2::zero;
			DirFourClockwise::Type dir = DirFourClockwise::Up;
			bool knownExits = false;
			bool knownDevices = false;
			bool knownSpecialDevices = false;
			bool known = false;
			bool visited = false;
			Random::Generator randomSeed;
			CellSetAny cellSet;
			Optional<Colour> mapColour;
			LineModel const* lineModel = nullptr;
			LineModel const* lineModelAdditional = nullptr; // only if main model exists
			LineModel const* lineModelAlwaysOtherwise = nullptr; // that is if not seen the normal
			ArrayStatic<LineModel const*, 5> objectiveLineModels; // always rendered forward, not aligned to the cell, these are map landmarks from objectives, if known or not is handled by code
			bool exits[DirFourClockwise::NUM]; // general, valid only at the lowest priority
			int sideExits[DirFourClockwise::NUM]; // of cells in dir, shows in which dir does the neighbour cell prefers side exit (eg. right exit might be side-down or side-up, will be at the lower or upper end)
			int rotateSlots = 0;
			ArrayStatic<KnownPilgrimageDevice, 16> knownPilgrimageDevices;

			CellQueryResult();

			Vector2 calculate_rel_centre() const;
		};

		struct CellQueryTexturePartsResult
		{
			struct Element
			{
				Vector2 offset = Vector2::zero;
				Framework::TexturePartUse texturePartUse;
				Framework::TexturePartUse texturePartUseEntrance;
				Optional<VectorInt2> cellDir; // direction to other cell
			};
			ArrayStatic<Element, 16> elements;

			CellQueryTexturePartsResult()
			{
				SET_EXTRA_DEBUG_INFO(elements, TXT("PilgrimageInstanceOpenWorld.CellQueryTexturePartsResult.elements"));
			}
		};

		struct LongDistanceDetection
		{
			static const int MAX_DETECTIONS = 6;
			static const int MAX_ELEMENTS = 5;
			struct Element
			{
				float radius = NONE; // if none, won't be used
				Optional<Vector2> directionTowards; // true location, if set, will render as line, otherwise as circle
				VectorInt2 deviceAt = VectorInt2::zero;
			};
			struct Detection
			{
				VectorInt2 at = VectorInt2::zero;
				ArrayStatic<Element, LongDistanceDetection::MAX_ELEMENTS> elements;

				void clear();
				void add(float _radius, VectorInt2 const& _deviceAt, Optional<Vector2> const & _directionTowards);
			};
			ArrayStatic<Detection, LongDistanceDetection::MAX_DETECTIONS> detections;

			mutable Concurrency::SpinLock lock;

			LongDistanceDetection() {}
			LongDistanceDetection(LongDistanceDetection const& _other) { copy_from(_other); }
			LongDistanceDetection& operator=(LongDistanceDetection const& _other) { copy_from(_other); return *this; }

			void clear();
			Detection & get_for(VectorInt2 const& _at);

			void copy_from(LongDistanceDetection const& _source);
		};

	public:
		static void distribute_connector_tags_evenly(Array<Name>& _connectorTags, OUT_ Array<Name>& _distributedConnectorTags, OUT_ Array<DirFourClockwise::Type>& _distributedDirs, bool _looped,
			Optional<DirFourClockwise::Type> const& _worldBreakerDir, Optional<DirFourClockwise::Type> const& _preferredFirstDir,
			int _count, PilgrimageInstanceOpenWorldConnectorTagDistribution::Type _distribution, Random::Generator const & _seed);
		static void have_connector_tags_at_ends(Array<Name>& _distributedConnectorTags);

		static Framework::DoorInRoom* get_linked_door_in(Framework::Door* _door, VectorInt2 const& _cellAt);

		static bool fill_direction_texture_parts_for(Framework::DoorInRoom const* _dir, OUT_ ArrayStack<Framework::TexturePartUse>& _dtp);
		static bool does_door_lead_in_dir(Framework::DoorInRoom const* _dir, DirFourClockwise::Type _inDir);
		static int get_door_distance_in_dir(Framework::DoorInRoom const* _dir, DirFourClockwise::Type _inDir);
		struct DoorLeadsTowardParams
		{
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(DoorLeadsTowardParams, bool, toObjective, to_objective, false, true);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(DoorLeadsTowardParams, bool, toAmmo, to_ammo, false, true);
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(DoorLeadsTowardParams, bool, toHealth, to_health, false, true);
		};
		static void keep_possible_door_leads(Framework::Room * _room, REF_ DoorLeadsTowardParams & _params);
		static Optional<bool> does_door_lead_towards(Framework::DoorInRoom const* _dir, DoorLeadsTowardParams const & _params, OPTIONAL_ OUT_ int * _dist = nullptr);

		static void rotate_slot(REF_ Vector2 & _slot, int _rotateSlots);

	public:
		static PilgrimageInstanceOpenWorld* get() { return fast_cast<PilgrimageInstanceOpenWorld>(s_current); }

		PilgrimageInstanceOpenWorld(Game* _game, Pilgrimage* _pilgrimage);
		~PilgrimageInstanceOpenWorld();

		DirFourClockwise::Type dir_to_world(DirFourClockwise::Type _localDir, Framework::Region* _region) const;
		DirFourClockwise::Type dir_to_world(DirFourClockwise::Type _localDir, Framework::Room* _room) const;

		DirFourClockwise::Type dir_to_local(DirFourClockwise::Type _worldDir, Framework::Region* _region) const;
		DirFourClockwise::Type dir_to_local(DirFourClockwise::Type _worldDir, Framework::Room* _room) const;

		override_ void store_game_state(GameState* _gameState, StoreGameStateParams const& _params);
		override_ void create_and_start(GameState* _fromGameState = nullptr, bool _asNextPilgrimageStart = false); // will add async job
		override_ bool has_started() const { return generatedPilgrimAndStarted; }

		override_ void will_continue(PilgrimageInstance const* _pi, Framework::Room* _transitionRoom, Framework::Door* _transitionRoomExitDoor);

	public:
		void add_scenery_in_room(Framework::Room* _room, Optional<bool> const & _vistaScenery = NP /*false*/, Optional<Transform> const& _relToPlacement = NP);

	public:
		VectorInt2 const& get_start_at() const { an_assert(startAt.is_set()); return startAt.get(); }
		float get_cell_size() const;
		float get_cell_size_inner() const;

	public:
		LineModel* get_pilgrimage_device_line_model_for_map(PilgrimageDevice* _pd, KnownPilgrimageDevice const* kpd) const;
		Framework::TexturePart* get_pilgrimage_device_direction_texture_part(PilgrimageDevice* _pd, KnownPilgrimageDevice const * kpd) const;
		bool is_pilgrimage_device_direction_known(PilgrimageDevice* _pd) const;
		void mark_pilgrimage_device_direction_known(PilgrimageDevice const* _pd);
		void mark_pilgrimage_device_direction_known(Framework::IModulesOwner* _imo);
		void mark_all_pilgrimage_device_directions_known(); // test/dev
		void clear_pilgrimage_device_directions_known_on_hold(); // all
		void update_pilgrimage_device_directions_known_on_hold(); // all
	private:
		void create_obfuscated_pilgrimage_device_line_model_for_map(PilgrimageDevice* _pd);
		void create_obfuscated_pilgrimage_device_directions();

	public:
		void force_update_pilgrims_destination() { forceUpdatePilgrimsDestination = true; }
		void force_add_open_world_destination_at(Framework::Room* _room, PilgrimageDevice* _device); // call tag_open_world_directions_for_cell afterwards
		void tag_open_world_directions_for_cell(Framework::Room* _roomInsideCell, bool _pilgrimageDevicesOnly = false);
		void get_clockwise_ordered_doors_in(Framework::Room* _room, REF_ Array<Framework::DoorInRoom*>& _doors);
		Framework::DoorInRoom* find_best_door_in_dir(Framework::Room* _room, DirFourClockwise::Type _inDir);
		bool is_door_in_dir(Framework::DoorInRoom* _dir, DirFourClockwise::Type _inDir);
		Optional<DirFourClockwise::Type> get_dir_for_door(Framework::DoorInRoom* _dir); // the strongest one

		void set_last_visited_haven(VectorInt2 const & _at);
		void clear_last_visited_haven();
		void update_last_visited_haven();

	public:
		LongDistanceDetection& access_long_distance_detection() { return longDistanceDetection; }
		LongDistanceDetection const & get_long_distance_detection() const { return longDistanceDetection; }

	public:
		Framework::Room* get_starting_room() const { return startingRoom.get(); } // if exists
		Framework::Door* get_starting_room_exit_door() const { return startingRoomExitDoor.get(); } // if exists
		Framework::Room* get_any_transition_room() const;
		void set_leave_starting_room_exit_door_as_is(bool _leaveExitDoorAsIs = true) { leaveExitDoorAsIs = _leaveExitDoorAsIs; }
		void set_manage_starting_room_exit_door(bool _manageExitDoor = true) { manageExitDoor = _manageExitDoor; }
		void set_starting_door_requested_to_be_closed(bool _startingDoorRequestedToBeClosed = true) { startingDoorRequestedToBeClosed = _startingDoorRequestedToBeClosed; }
		void keep_starting_room_exit_door_open(bool _keepExitDoorOpen = true) { keepExitDoorOpen = _keepExitDoorOpen; }
		void set_trigger_game_script_execution_trap_on_open_exit_door(Name const& _trap) { triggerGameScriptExecutionTrapOnOpenExitDoor = _trap; }
		bool can_exit_starting_room() const { return canExitStartingRoom; }
		bool is_starting_door_advised_to_be_open() const { return startingDoorAdvisedToBeOpen; }
		Framework::Room* get_vista_scenery_room(VectorInt2 const& _at) const;

	public:
		virtual void pre_advance();
		virtual void advance(float _deltaTime);

		virtual void on_end(PilgrimageResult::Type _result);

	protected:
		implement_ Framework::RegionType* get_main_region_type() const;

		implement_ void post_create_next_pilgrimage();

	public: // used for cell that is being generated/of interest
		bool is_cell_context_set() const { return cellContext.is_set(); }
		VectorInt2 get_cell_context() const;
		int get_dir_exits_count_for_cell_context() const;
		int get_pilgrimage_devices_count_for_cell_context() const;
		DirFourClockwise::Type get_dir_for_cell_context() const;
		DirFourClockwise::Type get_dir_to_world_for_cell_context(DirFourClockwise::Type _localDir) const;
		Random::Generator get_random_seed_for_cell_context() const;
		bool has_same_depends_on_in_local_dir_for_cell_context(DirFourClockwise::Type _localDir) const;
		CellSet const * get_cell_set_for_cell_context() const;
		Framework::RoomType const * get_main_room_type_for_cell_context() const;

	public:
		virtual void log(LogInfoContext & _log) const;

		void debug_visualise_map(VectorInt2 const& _at, bool _generate = false, Optional<VectorInt2> const & _highlightAt = NP) const;
		void debug_visualise_map(RangeInt2 const& _range, bool _generate = false, Optional<VectorInt2> const& _highlightAt = NP) const;

		void debug_render_mini_map(Framework::IModulesOwner* _imo, Range2 const& _at, float _alpha = 0.8f, int _minDist = 2, Optional<Colour> const& _forceColour = NP, Framework::Region const* _highlightRegion = nullptr);
		void debug_render_mini_map(VectorInt2 const& _cellAt, Range2 const& _at, float _alpha = 0.8f, int _minDist = 2, Optional<Colour> const & _forceColour = NP, Framework::Region const* _highlightRegion = nullptr);

		Optional<Vector3> find_cell_at_for_map(Framework::IModulesOwner* _imo) const; // will also check if room or region have forced "visibleMapCellAt" (or offset) set
		Optional<VectorInt2> find_cell_at(Framework::IModulesOwner* _imo) const;
		Optional<VectorInt2> find_cell_at(Framework::Room* _room) const;
		Optional<VectorInt2> find_cell_at(Framework::Region* _region) const;

		Framework::Region* get_main_region_for_cell(VectorInt2 const& _cell) const;

		Optional<DirFourClockwise::Type> query_towards_objective_at(VectorInt2 const& _at) const;
		CellQueryResult query_cell_at(VectorInt2 const& _at) const;
		CellQueryTexturePartsResult query_cell_texture_parts_at(VectorInt2 const& _at) const; // we assume we know everything about the cell
		Vector2 calculate_rel_centre_for(VectorInt2 const& _at) const;
		Vector2 calculate_random_small_offset_for(VectorInt2 const& _at, float radius, int rgOffset = 0) const;
		void mark_cell_visited(VectorInt2 const& _at);
		bool mark_cell_known(VectorInt2 const& _at, int _radius = 0);
		bool mark_cell_known_exits(VectorInt2 const& _at, int _radius = 0);
		bool mark_cell_known_devices(VectorInt2 const& _at, int _radius = 0);
		bool mark_cell_known_special_devices(VectorInt2 const& _at, int _radius = 0, int _amount = 1, Name const& _pdWithTag = Name::invalid(), OPTIONAL_ OUT_ Array<VectorInt2> * _markedAt = nullptr);
		
		void add_long_distance_detection(VectorInt2 const& _at, int _radius = 0, int _amount = 1); // detects devices at max radius and adds to the stored

		void mark_all_cells_known(); // only if the map is limited

		void make_sure_cells_are_ready(VectorInt2 const& _at, int _radius = 0);

		void async_get_airship_obstacles(VectorInt2 const& _at, float _radius, OUT_ Array<AirshipObstacle>& _obstacles);

		bool should_blocked_away_cells_distance_block_hostiles() const { return blockedAwayCellsDistanceBlockHostiles; }

		PilgrimageOpenWorldDefinition::SpecialRules const& get_special_rules() const;

		void set_door_direction_objective_override(Optional<DoorDirectionObjectiveOverride::Type> const& _doorDirectionObjectiveOverride = NP);

		bool are_there_no_door_markers_pending() const { return noDoorMarkersPending; }

	public:
		void restore_pilgrim_device_state_variables(VectorInt2 const& _at, Framework::IModulesOwner* _object = nullptr); // if not provided, all
		void store_pilgrim_device_state_variables(VectorInt2 const& _at, Framework::IModulesOwner* _object = nullptr); // if not provided, all
		void mark_pilgrim_device_state_depleted(VectorInt2 const& _at, Framework::IModulesOwner* _object);
		void mark_pilgrim_device_state_visited(VectorInt2 const& _at, Framework::IModulesOwner* _object);
		bool is_pilgrim_device_state_depleted(VectorInt2 const& _at, Framework::IModulesOwner* _object);
		bool is_pilgrim_device_state_depleted_long_distance_detectable(VectorInt2 const& _at); // will check which pilgrimage device is long distance detectable
		bool is_pilgrim_device_state_visited(VectorInt2 const& _at, Framework::IModulesOwner* _object);
		bool is_pilgrim_device_use_exceeded(VectorInt2 const& _at, Framework::IModulesOwner* _object);

	public:
		List<String> build_dev_info(Framework::IModulesOwner* _for = nullptr) const;

	public:
		bool done_objective(Name const & _id);
		bool next_objective();
		bool request_create_cell_for_objective(Name const & _id); // returns true if created

	protected:
		Optional<DoorDirectionObjectiveOverride::Type> doorDirectionObjectiveOverride; // this is not actual objective but just something we choose to show
		Optional<int> currentObjectiveIdx;
		Optional<bool> hasMoreObjectives;
		void async_invalidate_towards_objective();
		void async_update_towards_objective(VectorInt2 const & _cellAt, bool _updateExisting = true);
		void async_update_towards_objective_basing_on_existing_cells();

		bool next_objective_internal(bool _advance = true); // if not _advance, will just make sure we're starting properly

	protected:
		bool generatedPilgrimAndStarted = false;

	protected:
		struct HighLevelRegion
		{
			VectorInt2 hlIdx;
			// we may have a ladder made of regions with top and bottom regions
			SafePtr<Framework::Region> highLevelRegion; // actually created region (top one)
			SafePtr<Framework::Region> containerRegion; // container region (at the bottom)
			SafePtr<Framework::Room> aiManagerRoom; // an empty room only for ai room managers (in container region)
		};
		mutable Concurrency::SpinLock highLevelRegionsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.highLevelRegionsLock"));
		List<HighLevelRegion> highLevelRegions;

		HighLevelRegion* async_get_or_create_high_level_region_for(VectorInt2 const& _cellAt);
		Framework::RegionType* async_get_high_level_region_type_for(VectorInt2 const& _cellAt); // will get region type even if high level region should not exist, will get it always!
		
	private:
		struct HighLevelRegionProcessContext
		{
			bool justGetRegionType = false;
			HighLevelRegion* hlr = nullptr;
			Framework::RegionType* hlRegionType = nullptr;
		};
		bool async_process_high_level_region_for(VectorInt2 const& _cellAt, REF_ HighLevelRegionProcessContext & _context);
		bool should_use_high_level_region_for(VectorInt2 const& _cellAt) const;

	protected:
		Tags skipContentTags;

		Optional<VectorInt2> startAt; // updated from pilgrimage
		SafePtr<Framework::Room> startInRoom; // if loading a game state, it might be different than startingRoom
		SafePtr<Framework::Room> startingRoom;
		Array<SafePtr<Framework::Room>> noCellsRooms; // all no cell rooms, including starting room (startingRoom is the first one in the array)
		Array<SafePtr<Framework::Door>> noCellsRoomsConnector; // door - this to next
		int noCellsRoomsPrevDoorsDoneFor = NONE;
		bool noCellsRoomsAllGenerated = false; // if all generated, will connect last room to transition room in next pilgrimage
		SafePtr<Framework::IModulesOwner> startingRoomUpgradeMachine;
		Framework::UsedLibraryStored<PilgrimageDevice> startingRoomUpgradeMachinePilgrimageDevice;
		SafePtr<Framework::Door> startingRoomExitDoor;
		bool leaveExitDoorAsIs = false; // set by game script only - orders pilgrimage instance to leave starting room's exit door alone, don't close, don't open, leave it for anything else to handle
		bool manageExitDoor = true; // this is "open exit door", if set to false, will close them, no idea why this works this way, it shouldn't be
		bool keepExitDoorOpen = true;
		Name triggerGameScriptExecutionTrapOnOpenExitDoor; // if set, won't open door but will trigger trap
		bool startingDoorAdvisedToBeOpen = false;
		bool startingDoorRequestedToBeOpen = false;
		bool startingDoorRequestedToBeClosed = false;

		bool canExitStartingRoom = false;
		bool upgradeInStartingRoomUnlocked = false;
		bool leftStartingRoom = false;
		
		bool allowManagingCells = false; // has to be explicitly allowed, this is used to issue cell creation ONLY after the next pilgrimage has been created

		Optional<VectorInt2> lastVisitedHavenAt;
		SafePtr<Framework::Room> pilgrimInRoom;
		Optional<VectorInt2> pilgrimAt;
		Optional<VectorInt2> pilgrimComingFrom;
		
		Optional<VectorInt2> cellsHandledAt; // management, creating new

		TagCondition okRoomToSpawnTagCondition;

		struct Cell
		{
			DirFourClockwise::Type dir = DirFourClockwise::Up;
			
			//

			int doneForMinPriority = NONE;
			VectorInt2 at;

			Random::Generator randomSeed; // spawned from the main one
			CellSet set;
			Framework::UsedLibraryStored<Framework::RoomType> mainRoomType;
			Framework::UsedLibraryStored<Framework::LibraryBasedParameters> additionalCellInfo;
			Framework::UsedLibraryStored<LineModel> lineModel;
			Framework::UsedLibraryStored<LineModel> lineModelAdditional; // only if line model is shown
			Framework::UsedLibraryStored<LineModel> lineModelAlwaysOtherwise; // always if line model can't be shown
			ArrayStatic<Framework::UsedLibraryStored<LineModel>, 5> objectiveLineModels; // always, doesn't have to be known
			struct ObjectiveLineModelIfKnown
			{
				Framework::UsedLibraryStored<LineModel> lineModel;
				float distance = 0.0f;
			};
			ArrayStatic<ObjectiveLineModelIfKnown, 5> objectiveLineModelsIfKnown;
			Optional<Colour> mapColour;
			int localDependents[4] = { 0, 0, 0, 0 };
			int dependentsRadius = 0;
			bool exits[DirFourClockwise::NUM]; // general, valid only at the lowest priority
			// side exits are used by neighbour cells 
			int sideExitsUpDown = 0; // if has to choose between up and down (0 - any, but it should defined via fill_preferred_side_exits)
			int sideExitsLeftRight = 0; // if has to choose between left and right (as above)
			Optional<VectorInt2> dependentOn;
			PilgrimageOpenWorldDefinition::Rule const * usesRule = nullptr;
			PilgrimageOpenWorldDefinition::Objective const * usesObjective = nullptr; // uses for room/region/pilgrimage device
			int orderNoStart = 0;
			int orderNoEnd = 0;
			int iteration = 0;
			DirFourClockwise::Type specialLocalDir = DirFourClockwise::Up; // this is to place the "special" properly between other rooms, ie when deciding how doors are arranged within a room

			bool knownPilgrimageDevicesAdded = false;
			int rotateSlots = 0;
			ArrayStatic<KnownPilgrimageDevice, 16> knownPilgrimageDevices;

			WORKER_ Optional<float> towardsObjectiveCostWorker;
			CACHED_ Optional<int> towardsObjectiveIdx; // for what objective it was set/created (if not set, not valid)
			CACHED_ Optional<DirFourClockwise::Type> towardsObjective; // so we don't have to redo

			WORKER_ bool reachable = false;

			RUNTIME_ TagCondition checkpointRoomTagged; // if we end up in a checkpoint room, we update this tag condition, it makes it easier to restart in the given checkpoint room

			//

			// there are two scenery meshes available
			// "close" is to be used when we are inside the cell, "far" if we're not
			// "close" should not include things that are generated by room generator
			Framework::UsedLibraryStored<Framework::Mesh> closeSceneryMesh;
			Framework::UsedLibraryStored<Framework::Mesh> vistaWindowsSceneryMesh;
			Framework::UsedLibraryStored<Framework::Mesh> farSceneryMesh;
			Framework::UsedLibraryStored<Framework::Mesh> medFarSceneryMesh;
			Framework::UsedLibraryStored<Framework::Mesh> veryFarSceneryMesh;

			//

			Cell();

			void init(VectorInt2 const& _at, PilgrimageInstanceOpenWorld const& _openWorld); // and random seed

			int get_as_world_address_id() const; // from "at"
			
			void fill_preferred_side_exits();
			void fill_cell_exit_chances(REF_ float & _cellRemove1ExitChance, REF_ float& _cellRemove2ExitsChance, REF_ float& _cellRemove3ExitsChance, REF_ float& _cellRemove4ExitsChance, REF_ float& _cellIgnoreRemovingExitChance);
			void fill_allowed_required_exits(OUT_ bool* _allowedExits, OUT_ bool * _requiredExits, OUT_ bool & _allPossibleExitsRequired) const;
			void fill_dir_important_exits(OUT_ bool* _dirDependentExits, OUT_ bool & _dirDependent) const;
			void get_dependents_world(OUT_ int * _dependents) const;
			static bool is_conflict_free(Cell const& _cell, Cell const& _against); // these are working cells

			void generate_scenery_meshes(PilgrimageInstanceOpenWorld* _piow, int _distIdx);
			static void get_or_generate_background_mesh(Framework::UsedLibraryStored<Framework::Mesh>& _outputMesh, VectorInt2 const& _at, PilgrimageOpenWorldDefinition::BackgroundCellBased const& _bcd, PilgrimageInstanceOpenWorld* _piow);

			KnownPilgrimageDevice* access_known_pilgrimage_device(PilgrimageDevice* _pd);
			KnownPilgrimageDevice const * get_known_pilgrimage_device(PilgrimageDevice* _pd) const;

		private:
			void generate_scenery_mesh(int _distIdx, Name const& _meshGeneratorParam, Framework::UsedLibraryStored<Framework::Mesh>& _sceneryMesh, PilgrimageInstanceOpenWorld* _piow) const;
		};

		mutable Concurrency::MRSWLock cellsLock = Concurrency::MRSWLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.cellsLock"), 0.0008f);
		mutable List<Cell> cells; // already created so we don't have to recreate them, this really helps to determine a set for a cell as we would be recreating lots of them

		struct ExistingCell
		: public Cell
		{
			bool beingGenerated = false;
			bool beingActivated = false;
			bool generated = false;
			float timeSinceImportant = 0.0f;
			SafePtr<Framework::Region> cellLevelRegion; // actually created region (top one) (if present)
			SafePtr<Framework::Region> containerRegion; // container region (at the bottom) (if present)
			SafePtr<Framework::Room> aiManagerRoom; // an empty room only for ai room managers (in container region)
			SafePtr<Framework::Region> region; // actually created region (cell most likely)
			SafePtr<Framework::Region> mainRegion; // this is either the region from set or the lowest available region
			SafePtr<Framework::Room> vistaSceneryRoom; // room containing only vista, used for rendering only! is added to region
			HighLevelRegion* belongsToHighLevelRegion = nullptr; // more as an information

			SafePtr<Framework::Room> transitionRoom; // if present
			SafePtr<Framework::Door> transitionEntranceDoor; // if present

			Array<PilgrimageDeviceInstance> pilgrimageDevices;

			// check one of two depending on pilgrimage's transitionToNextRoomWithoutDirection/transitionToNextRoomWithObjectiveDirection
			CACHED_ Optional<DirFourClockwise::Type> towardsLocalObjectiveDir; // if special door is within cell and it is objective, used if towardsObjective is empty
			CACHED_ bool towardsLocalObjectiveNoDir = false; // if special door is within cell and it is objective, used if towardsObjective is empty
			CACHED_ Framework::UsedLibraryStored<PilgrimageDevice> towardsPilgrimageDevice; // if we actually want to get towards pilgrimage device

			struct ForcedDestination
			{
				SafePtr<Framework::Room> room;
				Framework::UsedLibraryStored<PilgrimageDevice> device;
			};
			Array<ForcedDestination> forcedDestinations;

			struct Door
			{
				Optional<VectorInt2> toCellAt;
				SafePtr<Framework::Door> door;
				SafePtr<Framework::IModulesOwner> noDoorMarker; // when there is no connection between cells, this thing is made visible
				CACHED_ bool noDoorMarkerPending = false;
				bool cellWontBeCreated = false; // if cell this door is leading to, won't be created, it is marked accordingly
			};
			Array<Door> doors;
			Framework::Door* get_door_to(VectorInt2 const& _toCellAt, int _doorIdx) const;
			void add_door(Optional<VectorInt2> const& _toCellAt, int _doorIdx, Framework::Door* _door);

			ExistingCell();
		};
		bool extraCellExitsDone = false;
		mutable Concurrency::MRSWLock existingCellsLock = Concurrency::MRSWLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.existingCellsLock"), 0.0008f);
		List<ExistingCell> existingCells;
		bool moreExistingCellsNeeded = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		ArrayStatic<VectorInt2, 10> wantsToCreateCellAt;
		bool generatingSceneries = false;
		bool destroyingCell = false;
#endif
		VectorInt2 creatingOpenWorldCell = VectorInt2::zero;

		bool noDoorMarkersPending = false;

		struct KnownPilgrimageDeviceInTile
		: public KnownPilgrimageDevice
		{
			VectorInt2 at = VectorInt2::zero; // absolute, cell addr
		};
		struct PilgrimageDevicesTile
		{
			Name groupId;
			bool centreBased = false; // if centre based, tile is ignored and doesn't process "closer to 0"
			VectorInt2 centreAt = VectorInt2::zero;
			float radiusFromCentre;
			VectorInt2 tile = VectorInt2::zero; // centre starts at cell 0,0
			ArrayStatic<KnownPilgrimageDeviceInTile, 256> knownPilgrimageDevices; // for centre based we try to make sure the cells may handle them (tags etc) but if multiple devices overlap and their priorities do not fit, they may go wrong

			PilgrimageDevicesTile()
			{
				SET_EXTRA_DEBUG_INFO(knownPilgrimageDevices, TXT("PilgrimageInstanceOpenWorld.PilgrimageDevicesTile.knownPilgrimageDevices"));
			}
		};
		mutable Concurrency::SpinLock pilgrimageDevicesTilesLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.pilgrimageDevicesTilesLock"));
		mutable List<PilgrimageDevicesTile> pilgrimageDevicesTiles;

		struct ObfuscatedPilgrimageDevice
		{
			Framework::UsedLibraryStored<PilgrimageDevice> pilgrimageDevice;
			bool known = false;
			bool knownOnHold = false;
			Framework::UsedLibraryStored<LineModel> obfuscatedLineModelForMap;
			Framework::UsedLibraryStored<Framework::TexturePart> obfuscatedDirectionTexturePart;
		};
		bool obfuscatedPilgrimageDevicesAvailable = false;
		Array<ObfuscatedPilgrimageDevice> obfuscatedPilgrimageDevices; // should be safe to use as line models should be created when pilgrimages start
		::System::RenderTargetPtr obfuscatedDirectionsRT;
		struct InitObfuscatedDirectionsRT;
		friend struct InitObfuscatedDirectionsRT;

		struct PilgrimageDevicesUsageByGroup
		{
			Name groupId;
			int useCount = 0;
		};
		mutable Concurrency::SpinLock pilgrimageDevicesUsageByGroupLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.pilgrimageDevicesUsageByGroupLock"));
		Array<PilgrimageDevicesUsageByGroup> pilgrimageDevicesUsageByGroup; // only if group id is valid

		SimpleVariableStorage pilgrimageVariables; // all unique variables common for this particular pilgrimage, stored and restored with game state

		Optional<VectorInt2> cellContext; // which cell do we do right now

		float updatePilgrimsDestinationTimeLeft = 0.0f;
		bool forceUpdatePilgrimsDestination = false;
		SafePtr<Framework::Room> pilgrimsDestinationDidForRoom;

		Array<VectorInt2> knownExitsCells;
		Array<VectorInt2> knownDevicesCells;
		Array<VectorInt2> knownSpecialDevicesCells;
		Array<VectorInt2> knownCells;
		Array<VectorInt2> visitedCells; // if cell has been visited, all the devices in it are known (appear on the map unless stated otherwise by their setup)
		mutable Concurrency::SpinLock knownExitsCellsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.knownExitsCellsLock"));
		mutable Concurrency::SpinLock knownDevicesCellsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.knownDevicesCellsLock"));
		mutable Concurrency::SpinLock knownSpecialDevicesCellsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.knownSpecialDevicesCellsLock"));
		mutable Concurrency::SpinLock knownCellsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.knownCellsLock"));
		mutable Concurrency::SpinLock visitedCellsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.PilgrimageInstanceOpenWorld.visitedCellsLock"));

		LongDistanceDetection longDistanceDetection;

		// this is done in the background to make sure all cells we're supposed to known are readied
		struct ReadiedCells
		{
			int knownExitsCells;
			int knownDevicesCells;
			int knownSpecialDevicesCells;
			int knownCells;

			Concurrency::Counter active;
		} readiedCells;

		ExistingCell* get_existing_cell(VectorInt2 const& _at) const;
		ExistingCell* get_existing_cell_already_locked(VectorInt2 const& _at) const;
		ExistingCell* get_existing_cell_for_already_locked(Framework::Room * _room) const;

		struct DoorForOrder
		{
			Framework::DoorInRoom* dir;
			Optional<DirFourClockwise::Type> inDir0;
			Optional<DirFourClockwise::Type> inDir1;
			int dirIdx;

			static int compare(void const* _a, void const* _b)
			{
				DoorForOrder const& a = *plain_cast<DoorForOrder>(_a);
				DoorForOrder const& b = *plain_cast<DoorForOrder>(_b);
				if (a.inDir0.is_set() && b.inDir0.is_set())
				{
					if (a.inDir0.get() < b.inDir0.get()) return A_BEFORE_B;
					if (a.inDir0.get() > b.inDir0.get()) return B_BEFORE_A;
				}
				if (a.inDir1.is_set() && b.inDir1.is_set())
				{
					if (a.inDir1.get() < b.inDir1.get()) return A_BEFORE_B;
					if (a.inDir1.get() > b.inDir1.get()) return B_BEFORE_A;
				}
				if (a.dirIdx < b.dirIdx) return A_BEFORE_B;
				if (a.dirIdx > b.dirIdx) return B_BEFORE_A;
				return A_AS_B;
			}
		};

		void fill_dir_for_order(DoorForOrder& _dfo, Framework::DoorInRoom* dir, ExistingCell* ec) const;

	protected:
		struct GetRightDoorDirectionTexturePartUse
		{
			PilgrimageOpenWorldDefinition const* owDef = nullptr;
			bool exits[DirFourClockwise::NUM];
			bool showDirectionsAsObjective = false;

			GetRightDoorDirectionTexturePartUse(PilgrimageOpenWorldDefinition const* owDef, Cell const* cell, bool _showDirectionsAsObjective = false);
			Framework::TexturePartUse get_for(DirFourClockwise::Type _dir) const;
			Framework::TexturePartUse get_exit_to(DirFourClockwise::Type _dir) const;
			Framework::TexturePartUse get_entered_from(DirFourClockwise::Type _dir) const;
		};

	protected:
		void async_create_and_start_begin(GameState* _fromGameState, bool _asNextPilgrimageStart);
		void async_create_and_start_end(GameState* _fromGameState, bool _asNextPilgrimageStart);

		void async_generate_starting_room(GameState* _fromGameState);
		void async_add_pilgrim_and_start(GameState* _fromGameState, Framework::Room* _inRoom, Transform const& _placement);

		void async_add_extra_cell_exits_if_required();
			void async_enforce_all_cells_reachable_if_required();
			void async_attemp_cells_have_enough_exits();

		void async_find_place_to_start(GameState* _fromGameState, bool _asNextPilgrimageStart);

	protected:
		friend class GameScript::Elements::Pilgrimage;
		float manageCellsTimeLeft = 0.0f;
		Optional<::System::TimeStamp> creatingCellTS;
		bool workingOnCells = false;
#ifdef DUMP_CALL_STACK_INFO_IF_CREATING_CELL_TOO_LONG
		float workingOnCellsTime = 0.0f;
#endif
		tchar workingOnCellsInfo[256];
		bool issue_create_cell(VectorInt2 const& _at, Optional<bool> const & _force = NP, Optional<bool> const & _ignoreLimit = NP); // will queue if possible, returns true if successfully added
		bool add_async_create_cell(VectorInt2 const& _at); // always add async world job
		void async_create_cell_at(VectorInt2 const& _at);
		void async_connect_no_cells_rooms_chain();
		void issue_no_cells_rooms_more_work();
		void issue_destroy_cell(VectorInt2 const& _at);

		void issue_unlink_door_to_cell(SafePtr<Framework::Door> const & _door, VectorInt2 const& _intoCell);
		void sync_unlink_door_to_cell(SafePtr<Framework::Door> const & _door, VectorInt2 const& _intoCell);
		
		Framework::DoorInRoom* find_door_entering_cell(VectorInt2 const& _at, VectorInt2 const& _from);

	protected:
		void setup_vr_placement_for_door(Framework::Door* _door, VectorInt2 const& _at, VectorInt2 const& _dir);

		enum PrimalSet
		{
			PrimalSet_Normal,
			PrimalSet_Rules,
			PrimalSet_Objectives,
		};

		Random::Generator get_random_seed_at(VectorInt2 const& _at) const;
		Random::Generator get_random_seed_at_tile(VectorInt2 const& _at) const;
		CellSet get_primal_set_at(VectorInt2 const& _at, PrimalSet _primalSet = PrimalSet_Normal, int _iteration = 0, int _minCellPriority = 0, bool _ignoreCellPriority = false) const; // if exceeds max available priority, returns none
		Cell & access_cell_at(VectorInt2 const& at, int _minCellPriority = 0);
		Cell const& get_cell_at(VectorInt2 const& at, int _minCellPriority = 0) const;
		PilgrimageDevicesTile const& get_pilgrimage_devices_tile_at(int _dpdIdx, VectorInt2 const& at, bool _atIsTileCoord = false) const;
		VectorInt2 cell_at_to_tile_at(VectorInt2 const& _cellAt) const;
		VectorInt2 get_tile_centre(VectorInt2 const& _tile) const;
		PilgrimageOpenWorldDefinition::Objective const* get_objective_for(VectorInt2 const& _cellAt, Optional<int> const& _onlyObjectiveIdx = NP) const;
		PilgrimageOpenWorldDefinition::Objective const* get_objective_to_create_cell(VectorInt2 const& _cellAt) const;
		bool is_objective_ok(PilgrimageOpenWorldDefinition::Objective const* _objective) const;
		bool is_objective_ok_at(PilgrimageOpenWorldDefinition::Objective const* _objective, VectorInt2 const& _cellAt, Optional<bool> const & _allowPilgrimageDeviceGroupId = NP) const;

		bool does_cell_have_special_device(VectorInt2 const& _at, Name const& _pdWithTag) const;
		bool does_cell_have_long_distance_detectable_device(VectorInt2 const& _at, OPTIONAL_ OUT_ bool * _detectableByDirection = nullptr) const;

		bool manage_cells_and_objectives_for(Framework::IModulesOwner* imo, bool _justObjectives = false); // returns false if out of map (starting room etc)
		void update_pilgrim_cell_at(Framework::IModulesOwner* imo);

		void output_dev_info(Framework::IModulesOwner* _for = nullptr) const;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool is_creating_or_wants_to_create_cell(Framework::IModulesOwner* imo);
#endif

	protected:
		// if filling for multiple doors and using same dist tag, use clear distances first
		static void clear_distances_for_doors(Framework::DoorInRoom* dir, Name const& distTag, bool _goThrough = false);
		static void fill_distances_for_doors(Framework::DoorInRoom* dir, Name const& distTag, bool _goThrough = false);
		static void fill_layout_cue_distances_for_doors(Framework::DoorInRoom* dir, Name const& distTag);

		void add_pilgrimage_devices(Cell* _cell);

		void spawn_pilgrimage_devices(ExistingCell* _cell);

		bool get_cell(VectorInt2 const& _at, OUT_ Cell*& c) const;
		bool get_cells(VectorInt2 const& _at, OUT_ Cell*& c, OUT_ ExistingCell*& ec) const;

		bool is_cell_address_valid(VectorInt2 const& _at) const;

		void async_make_sure_cells_are_ready(VectorInt2 const& _at, int _radius = 0);
		void issue_make_sure_known_cells_are_ready();
		void async_make_sure_known_cells_are_ready();

		void on_starting_room_obtained();

		override_ TransitionRoom async_create_transition_room_for_previous_pilgrimage(PilgrimageInstance* _previousPilgrimage);

		VectorInt2 validate_coordinates(VectorInt2 at) const;

		void set_vr_elevator_altitude_for(Framework::Door* _door, VectorInt2 const& _at) const;

		void async_update_no_door_markers(VectorInt2 const & _cellAt);

		override_ void post_setup_rgi();

	private:
		WheresMyPoint::SimpleWMPOwner simpleWMPOwnerForCellDoors;

		float timeSinceLastCellBlocked = 0.0f;
		int blockedAwayCells = -1; // allow going left/right from start
		int blockedAwayCellsDistance = 1000; // how far away are we from blocked away cells
		bool blockedAwayCellsDistanceBlockHostiles = false;

		void creating_cell_for_too_long();

		void async_hide_doors_that_wont_be_used(ExistingCell* _cell);
	};
};
