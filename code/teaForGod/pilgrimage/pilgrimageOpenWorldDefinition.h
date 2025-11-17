#pragma once

#include "..\game\skipContentInfo.h"

#include "..\..\core\types\dirFourClockwise.h"
#include "..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class DoorType;
	class EnvironmentType;
	class Mesh;
	class MeshGenerator;
	class RegionType;
	class RoomType;
	class SceneryType;
	class TexturePart;
};

namespace TeaForGodEmperor
{
	class LineModel;
	class Pilgrimage;
	class PilgrimageDevice;
	class PilgrimageEnvironmentMix;
	class PilgrimageInstanceOpenWorld;

	namespace DoorDirectionCombination
	{
		// if other dirs required, has to be rotated
		// some are not covered as they can be just mirrored
		enum Type
		{
			Up,
			UpD,
			UpR, // mirrored -> UpL
			UpRD, // mirrored -> UpDL
			UpRDL,
			UpRL,

			NoUp,
			NoUpD,
			NoUpR, // mirrored -> UpL
			NoUpRD, // mirrored -> UpDL
			NoUpRDL,
			NoUpRL,

			UpExitTo,
			UpEnteredFrom,

			NUM
		};
	}
	/**
	 *	Open world
	 *
	 *	Map is composed of cells. Cell may define/force adjacent cells to be of a specific type (if this cannot be fulfilled, they should be reverted)
	 *
	 *	The game starts in a starting room. The starting room should be a smart one, it should know where the doors to the starting cell are and should be built in such a way,
	 *	that there's little or no corridor. The starting door might be closed and auto open. It doesn't affect the world generation, it's just placed in a random room in the starting cell.
	 *	Starting cell type should be able to accept starting room, it has to contain a POI "startingDoor".
	 *
	 *	The doors to other cells should always exist but might be not active in-game (not visible, not passable)
	 *	They always connect A left/down, B right/up
	 *	They are always placed in VR space properly
	 *	They have variables set
	 *		bool		entrance	is this entrance
	 *		VectorInt2	aCell		where "A" side goes to
	 *		VectorInt2	bCell		where "B" side goes to
	 *
	 *	===
	 *
	 *	Open world cells are defined as rooms or regions.
	 *	If there's an issue, it is resolved in favour of the cell closer to 0,0
	 *	LibraryStored's custom parameters are used:
	 *		float					cellProbCoef					to make the cell being used more or less often, relative to 1
	 *		bool					cellNotFirstChoice				cannot be chosen during initial iteration(s)
	 *		int						cellPriority					higher priorities may override neighbour cells
	 *																when choosing a set, a cell looks up around it to check
	 *																if there is a cell of a higher priority that may modify it
	 *		Tags					cellType
	 *		bool					avoidSameNeighbours				will try to not have same kind of cell as a neighbour (try not to mix with rules)
	 *		float					cellRemove1ExitChance			chance to remove one exit
	 *		float					cellRemove2ExitsChance			chance to remove two exits
	 *		float					cellRemove3ExitsChance			chance to remove three exits
	 *		float					cellRemove4ExitsChance			chance to remove four exits (do not use if no way to make sure all are reachable)
	 *		float					cellIgnoreRemovingExitChance	if we're to remove exit but we ignore this
	 *		bool					noLeftExit						\
	 *		bool					noRightExit						 \ this cell may have less doors
	 *		bool					noUpExit						 /
	 *		bool					noDownExit						/
	 *		bool					sideExitsAtUp					\  
	 *		bool					sideExitsAtDown					 \ side exits are to be considered to lie closer
	 *		bool					sideExitsAtLeft					 / to one of the edges
	 *		bool					sideExitsAtRight				/ 
	 *		bool					leftExitRequired				\														\
	 *		bool					rightExitRequired				 \ overrides random exits, if can't fulfill exits,		 \
	 *		bool					upExitRequired					 / recreates a cell - all exits required even if no cell  \
	 *		bool					downExitRequired				/														   \ abide to priorities/neighbours
	 *		bool					allExitsRequired				overrides random exits, if can't fulfill exits,			   /
	 *																recreates a cell - all exits required even if no cell	  / 
	 *		bool					allPossibleExitsRequired		as above but ignores exits no to cell,					 /
	 *																won't cut them out randomly								/
	 *		int						forceDir						direction is fixed, 0-up 1-right 2-down 3-left
	 *		int						specialLocalDir					special exit is to appear as it would in in this LOCAL direction, 0-up 1-right 2-down 3-left
	 *		bool					dirFixed						direction is fixed, default/up
	 *		bool					dirImportant					it is important for this cell, where the dir is facing
	 *																if both cells share exit and it is marked as dir important
	 *																and both of them are dir important, if they mismatch dir
	 *																there's no connection between
	 *		bool					dirImportantLeftExit			\
	 *		bool					dirImportantRightExit			 \ this exit depends on dir
	 *		bool					dirImportantUpExit				 /
	 *		bool					dirImportantDownExit			/
	 *
	 *																note: dependency works if the one above has a higher priority
	 *		bool					dirAlignDependent				|   cells   | same dir
	 *		bool					axisAlignDependent				|   align   | same axis, but different dir
	 *		bool					alignDependentTowards			| that  are | towards us
	 *		bool					alignDependentAway				| dependent | away from us
	 *		TagCondition			dependsOnCellType				has to match condition
	 *		TagCondition			dependentCellTypes				has to match condition
	 *		int						maxDependentRadius				\
	 *		int						maxDependentLeft				 \
	 *		int						maxDependentRight				  >	this cell influences other cells
	 *		int						maxDependentUp					 /	maxDependent* tells how far does it stretch
	 *		int						maxDependentDown				/
	 *
	 *		Framework::LibraryName	closeSceneryMeshGenerator		\ check instance for more info
	 *		Framework::LibraryName	farSceneryMeshGenerator			/
	 *
	 *		Framework::LibraryName	cellMainRoomType				main room in the cell - important if we need data in it
	 * 
	 *		TagCondition			additionalCellInfoTagged		this is to get additional cell info that can be used to display unique line model (doesn't work for noCells)
	 *																additionalCellInfo is LibraryBasedParameters (uses standard probCoef)
	 *																if this is chosen, it stores as additionalCellInfo + adds custom parameters to the parameters list of cell/region
	 *																this is decided after the cell has been already created
	 *
	 *	===
	 *
	 *	High-level regions fall into various patterns:
	 *
	 *	Crosses
	 *			.....d....
	 *			....ddde..
	 *			....adeee.		They go into a higher level 
	 *			...aaabe..		of dx=2,-1 dy=1,2
	 *			....abbb..
	 *			......b...
	 *			..........
	 *
	 *	Cross + Square
	 *			..........
	 *			....bb....
	 *			...abb....		The go into a higher level
	 *			..aaa.....		of dx=3,0, dy=0,3
	 *			...a......
	 *			..........
	 *
	 *	Single Lines
	 *
	 *			..........
	 *			bbbbbbbbbb
	 *			aaaaaaaaaa
	 *			..........
	 * 
	 *	Double Lines
	 *
	 *			..........
	 *			bbbbbbbbbb
	 *			bbbbbbbbbb
	 *			aaaaaaaaaa
	 *		  0	aaaaaaaaaa
	 *			..........
	 * 
	 *	Double Lines Offset
	 *
	 *			..........
	 *			bbbbbbbbbb
	 *			bbbbbbbbbb
	 *			aaaaaaaaaa
	 *			aaaaaaaaaa
	 *		  0	..........
	 *			..........
	 * 
	 *	They are chosen out of different region types that have different spawn sets, corridor looks etc.
	 *	This should make regions of the world look more coherent.
	 *
	 *	Each high-level region has a specific room to hold a region manager (auto generated).
	 *
	 *	Each high-level may create one sub region that holds a different set of spawn sets, region managers and/or slight variations (this is done via custom parameter highLevelSubRegionTagged
	 *
	 *	===
	 *
	 *	Starting room should have "outDoor" or "door" POI to place exit door.
	 *	If has "inDoor" POI, it is used for entrance door (so there are two doors).
	 *	Door for starting room are created post generation and placed via POIs.
	 *	Therefore the generators should not rely on door placements.
	 *
	 *	===
	 * 
	 *	For noCells it may have just starting room
	 *	Door for noCells rooms are created post generation and placed via POIs.
	 *	Therefore the generators should not rely on door placements.
	 * 
	 */
	class PilgrimageOpenWorldDefinition
	: public RefCountObject
	{
	public:
		enum Pattern
		{
			Crosses,
			CrossSqure,
			SingleLines,
			DoubleLines,
			DoubleLinesOffset,
			SquaresAligned,
			SquaresMovedX,
		};

		struct SpecialRules
		{
			bool upgradeMachinesProvideMap = true;
			bool pilgrimOverlayInformsAboutNewMap = true;
			bool silentAutomaps = false;
			bool startingRoomWithoutDirection = false;
			Optional<DirFourClockwise::Type> startingRoomDoorDirection;
			bool transitionToNextRoomWithoutDirection = false;
			Optional<DirFourClockwise::Type> transitionToNextRoomDoorDirection;
			bool transitionToNextRoomWithObjectiveDirection = false;
			Optional<int> detectLongDistanceDetectableDevicesAmount;
			Optional<int> detectLongDistanceDetectableDevicesRadius;
		};

		struct AttemptToCellsHave
		{
			int exitsCount = 0;
			float exitsCountAttemptChance = 1.0f;
		};

		static const int maxDependentDist = 4;

		struct CellSet
		{
			template <typename Element>
			struct IncludeExplicit
			{
				Framework::UsedLibraryStored<Element> element;
				TagCondition forGameDefinitionTagged;
			};
			struct IncludeTagged
			{
				TagCondition tagged;
				TagCondition forGameDefinitionTagged;
			};
			Array<IncludeExplicit<Framework::RoomType>> includeRoomTypes;
			Array<IncludeExplicit<Framework::RegionType>> includeRegionTypes;
			Array<IncludeTagged> includeRoomTypesTagged;
			Array<IncludeTagged> includeRegionTypesTagged;
			struct Available
			{
				float probCoef = 1.0f;
				bool crouch = false;
				bool anyCrouch = false;
				bool notFirstChoice = false;
				int cellPriority = 0;
				Framework::UsedLibraryStored<Framework::LibraryStored> libraryStored;
				TagCondition forGameDefinitionTagged;
				Available() {}
				explicit Available(Framework::LibraryStored* ls, TagCondition const & _forGameDefinitionTagged);

				static inline int compare(void const* _a, void const* _b)
				{
					Available const& a = *plain_cast<Available>(_a);
					Available const& b = *plain_cast<Available>(_b);
					return Framework::LibraryName::compare(&a.libraryStored->get_name(), &b.libraryStored->get_name());
				}
			};
			Array<Available> available;
			int maxAvailableCellPriority = NONE;

			bool is_defined() const;
			bool is_empty() const;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		};

		struct HighLevelRegionSet
		: public RefCountObject
		{
			struct Element
			{
				TagCondition forGameDefinitionTagged;
				Framework::UsedLibraryStored<Framework::RegionType> includeRegionType;
				TagCondition includeRegionTypesTagged;
			};
			Array<Element> include;
			struct Available
			{
				float probCoef = 1.0f;
				Framework::UsedLibraryStored<Framework::LibraryStored> libraryStored;
				RefCountObjectPtr<HighLevelRegionSet> highLevelSubRegionSet;
				TagCondition forGameDefinitionTagged;
				Available() {}
				explicit Available(Framework::LibraryStored* ls, TagCondition const & _forGameDefinitionTagged, Framework::Library* _library, int _depth,
					Name const& _regionProbCoefVarName, Name const& _subRegionProbCoefVarName, Name const& _subRegionTaggedVarName
				);
				~Available();

				static inline int compare(void const* _a, void const* _b)
				{
					Available const& a = *plain_cast<Available>(_a);
					Available const& b = *plain_cast<Available>(_b);
					return Framework::LibraryName::compare(&a.libraryStored->get_name(), &b.libraryStored->get_name());
				}
			};
			Array<Available> available;

			bool is_empty() const;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext,
				Name const& _regionProbCoefVarName, Name const& _subRegionProbCoefVarName, Name const& _subRegionTaggedVarName);
		};

		struct Rule
		{
			enum Type
			{
				None,
				BorderNear,
				BorderFar,
				BorderLeft,
				BorderRight
			} type = None;
			RangeInt depth = RangeInt(1); // general depth

			TagCondition cellTypes;

			bool allowHighLevelRegion = false;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
		};

		struct Objective
		{
			Name id;

			bool notActualObjective = false;
			TagCondition forGameDefinitionTagged;
			TagCondition forSkipContentTags; // they have to be defined in "skipContent"
			
			Optional<DirFourClockwise::Type> forceTowardsDir; // will use this when learning where to go

			bool addTransitionRoom = false;

			bool allowHighLevelRegion = false;

			Name pilgrimageDeviceGroupId; // to get to a device of this group
			bool requiresToDepletePilgrimageDevice = false; // if set to true, will ignore depleted pilgrimage devices
			TagCondition forceRandomPilgrimageDeviceTagged;
			
			bool allowRandomPilgrimageDevices = true;
			bool allowDistancedPilgrimageDevices = true;

			Framework::UsedLibraryStored<Framework::RoomType> forceSpecialRoomType; // will be connected in place of special
			Tags regionGenerationTags; // to be used in region generation, this can be used to modify content of region

			Framework::UsedLibraryStored<LineModel> lineModel; // displayed always
			Framework::UsedLibraryStored<LineModel> lineModelIfKnown; // displayed only if known
			float lineModelIfKnownDistance = 0.0f; // if 0 only here, if 1 neighbours and so on

			bool atStartAt = false; // will use pilgrimage's startAt
			Optional<VectorInt2> at; // if not set, it is not placed, it might be just being used for certain group
			Optional<DirFourClockwise::Type> atDir; // this is to rotate the cell accordingly
			VectorInt2 repeatOffset = VectorInt2::zero;
			RangeInt2 randomOffset = RangeInt2::zero;
			Array<RangeInt2> notInRange; // ignore if in range
			bool ignoreIfBeyondRange = false; // if objective would be outside range it should be ignored

			bool beingInCellRequiresNoViolence = false;

			struct SkipCompleteOn
			{
				Optional<int> reachedX;
				Optional<int> reachedY;
			} skipCompleteOn;

			// min of two
			Optional<int> visibilityDistance;
			Optional<float> visibilityDistanceCoef;

			TagCondition cellTypes; // if not defined, will just try to get to "at"
			Array<Framework::UsedLibraryStored<Framework::RegionType>> regionTypes;
			Array<Framework::UsedLibraryStored<Framework::RoomType>> roomTypes;
			Array<TagCondition> regionTypesTagged;
			Array<TagCondition> roomTypesTagged;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
			bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

			int calculate_visibility_distance(int _veryFarDistance) const;

			bool does_apply_at(VectorInt2 const& _at, PilgrimageInstanceOpenWorld const* _forPilgrimage) const;
		};

		struct BackgroundSceneryObject
		{
			Framework::UsedLibraryStored<Framework::SceneryType> sceneryType;
			Transform placement = Transform::identity;
			// without cell offset the placement will be relative always
			bool applyCellOffsetX = false;
			bool applyCellOffsetY = false;
			Optional<bool> forBullshots;
		};

		struct BackgroundCellBased
		{
			Framework::UsedLibraryStored<Framework::Mesh> mesh;
			Framework::UsedLibraryStored<Framework::MeshGenerator> meshGenerator;
			VectorInt2 at = VectorInt2::zero;
			VectorInt2 repeatOffset = VectorInt2::zero;
			ArrayStatic<VectorInt2, 4> repeatOffsetNotAxisAligned;
			Transform placement = Transform::identity;

			float visibilityRange = 0.0f; // if 0, only if cell is visible
			int extraVisibilityRangeCellRadius = 0; // applies to cell visibility range
		};

		struct NoCellsRoom
		{
			Framework::UsedLibraryStored<Framework::RoomType> roomType;
			Framework::UsedLibraryStored<Framework::DoorType> toNextDoorType;
			Tags tags;
		};

		struct LineModelOnMap
		{
			Framework::UsedLibraryStored<LineModel> lineModel;
			TagCondition storyFactsRequired;
			Vector3 at = Vector3::zero;
		};

		struct RestartInRoomTagged
		{
			Optional<VectorInt2> cellAt;
			TagCondition roomTagged;
			TagCondition onStoryFacts;
		};

		PilgrimageOpenWorldDefinition();

	public:
		Framework::RegionType* get_main_region_type() const { return mainRegionType.get(); }

		Name const& get_background_scenery_anchor_variable_name() const { return backgroundSceneryAnchorVariableName; }
		Framework::SceneryType* get_background_scenery_type() const { return backgroundSceneryType.get(); }
		Framework::MeshGenerator* get_background_scenery_default_mesh_generator() const { return defaultBackgroundSceneryMeshGenerator.get(); }
		Array<BackgroundSceneryObject> const* get_background_scenery_objects(VectorInt2 const& _cellAt) const { if (auto* bd = find_background_definition(_cellAt)) return &bd->backgroundSceneryObjects; return nullptr; }
		Array<BackgroundCellBased> const& get_backgrounds_cell_based() const { return backgroundsCellBased; }
		int get_combine_scenery_cells_size() const { return combineSceneryCellsSize; }
		int get_background_scenery_cell_distance_visibility() const { return backgroundSceneryCellDistanceVisibility; }
		int get_background_scenery_cell_distance_visibility_very_far() const { return backgroundSceneryCellDistanceVisibilityVeryFar; }
		bool has_no_cells() const { return noCells; }
		bool no_starting_room_door_out_connection() const { return noStartingRoomDoorOutConnection; }
		bool no_transition_door_out_required() const { return noTransitionDoorOutRequired; }
		bool does_require_directions_towards_objective() const { return requiresDirectionsTowardsObjective; }
		bool has_door_directions(bool _objectives) const;
		bool has_tile_change_notifications() const { return !noTileChangeNotification; }
		bool is_follow_guidance_blocked() const { return followGuidanceBlocked; }
		bool is_map_available() const { return !noMap; }
		bool is_whole_map_known() const { return wholeMapKnown; }
		float get_cell_size() const { return cellSize; }
		float get_cell_size_inner() const { return cellSizeInner; }

		Framework::RoomType* get_vista_scenery_room_type() const { return vistaSceneryRoomType.get(); }

		Framework::RoomType* get_starting_room_type() const { return startingRoomType.get(); }
		Framework::DoorType* get_starting_entrance_door_type() const { return startingEntranceDoorType.get(); }
		Framework::DoorType* get_starting_exit_door_type() const { return startingExitDoorType.get(); }
		bool get_starting_door_connect_with_previous_pilgrimage() const { return startingDoorConnectWithPreviousPilgrimage; }
		
		bool does_allow_delayed_object_creation_on_start() const { return allowDelayedObjectCreationOnStart; }

		Framework::SceneryType* get_no_door_marker_scenery_type() const { return noDoorMarkerSceneryType.get(); }

		Array<NoCellsRoom> const & get_no_cells_rooms() const { return noCellsRooms; }
		Optional<int> const & get_limit_no_cells_rooms_pregeneration() const { return limitNoCellRoomsPregeneration; }

		Framework::RegionType* get_cell_region_type() const { return cellRegionType.get(); }

		Pattern get_pattern() const;

		bool should_allow_high_level_region_at_start() const { return allowHighLevelRegionAtStart; }
		HighLevelRegionSet const & get_high_level_region_set() const { return highLevelRegionSet; }
		HighLevelRegionSet const & get_cell_level_region_set() const { return cellLevelRegionSet; }
		
		CellSet const & get_generic_cell_set() const { return genericCellSet; }
		CellSet const & get_for_rules_cell_set() const { return forRulesCellSet; }
		CellSet const & get_for_objectives_cell_set() const { return forObjectivesCellSet; }
		CellSet const & get_starting_cell_set() const { return startingCellSet; }

		Array<LineModelOnMap> const& get_line_models_always_on_map() const { return lineModelsAlwaysOnMap; }
		bool should_always_shop_whole_map() const { return alwaysShowWholeMap; }

		List<RestartInRoomTagged> const& get_restart_in_room_tagged() const { return restartInRoomTagged; }

		SpecialRules const& get_special_rules() const { return specialRules; }

		SkipContentSet const& get_skip_content_set() const { return skipContentSet; }

	public:
		struct RandomPilgrimageDevices
		{
			TagCondition tagged;
			TagCondition forGameDefinitionTagged;
			Array<Framework::UsedLibraryStored<PilgrimageDevice>> devices; // further filtered by game definition
		};
		Array<RandomPilgrimageDevices> const& get_random_pilgrimage_devices() const { return randomPilgrimageDevices; }

		struct DistancedPilgrimageDevices
		{
			Name groupId;
			int useLimit = NONE;
			TagCondition tagged;
			TagCondition forGameDefinitionTagged;
			Optional<Range> distance; // overrides pilgrimage devices distance
			
			// for non centre based only!
			Optional<int> atXLine;
			Optional<int> atYLine;
			Array<VectorInt2> forcedAt; // if we want some devices to be at the precise locations

			struct CentreBased
			{
				// if centre based, will ignore tile setup, will always process this
				bool use = false;
				Optional<Range> distanceCentreToStartAt; // where centre is placed relatively to start
				Optional<Range> distanceFromCentre;
				int minCount = 1; // there always has to be at least 1 (or more, this will try to fulfill this requirement but if it fails due to pilrigrage devices' distances, it fails, ultimate fail-safe is to place one in the centre)
				Optional<int> maxCount; // will remove all extra
				Optional<RangeInt> xRange;
				Optional<RangeInt> yRange;
			} centreBased;

			Array<Framework::UsedLibraryStored<PilgrimageDevice>> devices;
		};
		Array<DistancedPilgrimageDevices> const& get_distanced_pilgrimage_devices() const { return distancedPilgrimageDevices; }
		int get_distanced_pilgrimage_devices_tile_size() const { return distancedPilgrimageDevicesTileSize; }

		Framework::TexturePart* get_direction_texture_part(DoorDirectionCombination::Type _dir) const { return directionTextureParts.is_index_valid(_dir)? directionTextureParts[_dir].get() : nullptr; }
		Framework::TexturePart* get_towards_objective_texture_part() const { return towardsObjectiveTextuerPart.get(); }

	public:
		struct CellDoorVRPlacement
		{
			VectorInt2 from = VectorInt2::zero;
			VectorInt2 to = VectorInt2::zero;
			WheresMyPoint::ToolSet wheresMyPoint; // 0,0 is centre of play area, forward is from->to
		};

		CellDoorVRPlacement const* get_cell_door_vr_placement(VectorInt2 const& _from, VectorInt2 const& _to) const;

		bool should_alternate_cell_door_vr_placement() const { return alternateCellDoorVRPlacement; }
		Optional<VectorInt2> const & get_cell_door_tile_range() const { return cellDoorTileRange; }
		Optional<Vector2> const & get_cell_door_range() const { return cellDoorRange; }

	private:
		List<CellDoorVRPlacement> cellDoorVRPlacements;
		bool alternateCellDoorVRPlacement = true;
		Optional<VectorInt2> cellDoorTileRange;
		Optional<Vector2> cellDoorRange;

	public:
		struct BlockAwayFromObjective
		{
			TagCondition forGameDefinitionTagged;
			Optional<float> timePerCell;
			Optional<int> blockBehind; // if we moved in certain distance, block cells at this behind (0, at the same level)
			Optional<int> startBlockedAwayCells; // if 0, we won't create cells to sides
			Optional<bool> shouldBlockHostileSpawnsAtBlockedAwayCells; // if blocked away cells distance <= 0
			Framework::UsedLibraryStored<Framework::SceneryType> noDoorMarkerSceneryType; // placed when there is no door (yet or never going to be)
		};

		BlockAwayFromObjective const* get_block_away_from_objective() const;

	private:
		Array<BlockAwayFromObjective> blockAwayFromObjectives;

	public:
		bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		RangeInt2 const& get_start_at() const { return startAt; }
		Array<VectorInt2> const& get_auto_generate_at_start() const { return autoGenerateAtStart; }
		Array<VectorInt2> const& get_always_generate_at() const { return alwaysGenerate; }
		RangeInt2 const& get_size() const;

		bool should_create_all_cells() const { return createAllCells; }
		
		bool are_authorised_cells_unreachable() const { return authorisedCellsUnreachable; }

		bool should_enforce_all_cells_reachable() const { return enforceAllCellsReachable; }
		AttemptToCellsHave const & get_attempt_to_cells_have() const { return attemptToCellsHave; }

		void fill_cell_exit_chances(OUT_ float& _cellRemove1ExitChance, OUT_ float& _cellRemove2ExitsChance, OUT_ float& _cellRemove3ExitsChance, OUT_ float& _cellRemove4ExitsChance, OUT_ float& _cellIgnoreRemovingExitChance) const;

		Vector2 const& get_vr_elevator_altitude_gradient() const { return vrElevatorAltitudeGradient; }

		Array<Rule> const& get_rules() const { return rules; }

		Array<Objective> const& get_objectives() const { return objectives; }
		bool are_objectives_sequential() const { return sequentialObjectives; }

		Optional<DirFourClockwise::Type> get_objectives_general_dir() const { return objectivesGeneralDir; }

	private:
		bool noCells = false; // instead of creating cells we create rooms that connect to each other
		bool noStartingRoomDoorOutConnection = false; // if we don't want to connect starting room to the region (or anything else)
		bool noTransitionDoorOutRequired = false; // sometimes we don't want to connect cells/no-cell-rooms/starting-room to transition (we will be teleporting!)
		bool requiresDirectionsTowardsObjective = false; // requires preparation, even if not displayed
		bool noDoorDirections = false;
		bool noDoorDirectionsUnlessObjectives = false;
		bool noTileChangeNotification = false;
		bool followGuidanceBlocked = false; // won't be saying "follow guidance"
		bool noMap = false;
		bool wholeMapKnown = false; // just the layout, not exits
		struct Size
		{
			RangeInt2 size = RangeInt2::empty; // if empty - unlimited
			TagCondition forGameDefinitionTagged;
		};
		Array<Size> sizes;
		RangeInt2 startAt = RangeInt2::zero; // might be outside
		Array<VectorInt2> autoGenerateAtStart; // auto generate cells if in starting/transition room - should help with having a few cells in advance
		Array<VectorInt2> alwaysGenerate; // always generate cells
		Vector2 vrElevatorAltitudeGradient = Vector2::zero; // should be high enough for longer elevators - depends on levels
		struct PatternElement
		{
			Pattern pattern = Pattern::Crosses;
			TagCondition forGameDefinitionTagged;
		};
		Array<PatternElement> patterns;
		
		Array<LineModelOnMap> lineModelsAlwaysOnMap;
		bool alwaysShowWholeMap = false;

		List<RestartInRoomTagged> restartInRoomTagged; // chooses the first match!

		bool createAllCells = false;
		
		bool authorisedCellsUnreachable = false; // in some cases we want them to be unreachable (not creating a valid path)

		bool enforceAllCellsReachable = false;
		AttemptToCellsHave attemptToCellsHave;

		float cellRemove1ExitChance = 0.3f;
		float cellRemove2ExitsChance = 0.06f;
		float cellRemove3ExitsChance = 0.0f;
		float cellRemove4ExitsChance = 0.0f;
		float cellIgnoreRemovingExitChance = 0.6f;

		Name backgroundSceneryAnchorVariableName; // has to be the same for every place
		Framework::UsedLibraryStored<Framework::SceneryType> backgroundSceneryType; // scenery used to represent other cells
		Framework::UsedLibraryStored<Framework::MeshGenerator> defaultBackgroundSceneryMeshGenerator; // if not provided, this is used to generate per cell - it's better to use it this way than to 
		struct BackgroundDefinition
		{
			RangeInt2 forCell = RangeInt2::empty; // empty - all
			Array<BackgroundSceneryObject> backgroundSceneryObjects; // always placed
		};
		Array<BackgroundDefinition> backgroundDefinitions;
		Array<BackgroundCellBased> backgroundsCellBased;
		mutable CACHED_ Optional<VectorInt2> backgroundDefinitionIdxForCellAt;
		mutable CACHED_ int backgroundDefinitionIdx = NONE;

		int combineSceneryCellsSize = 3; // used to combine meshes into a single mesh to sped up rendering
		int backgroundSceneryCellDistanceVisibility = 4;
		int backgroundSceneryCellDistanceVisibilityVeryFar = 0;
		float cellSize = 48.0f;
		float cellSizeInner = 40.0f;

		Framework::UsedLibraryStored<Framework::RoomType> vistaSceneryRoomType; // to start in a room - starting room should be a clever one that connects to the entrance door

		Framework::UsedLibraryStored<Framework::RegionType> mainRegionType; // main one, to contain default settings

		// generators for these room types should not rely on door placements as there are none provided
		Framework::UsedLibraryStored<Framework::RoomType> startingRoomType; // to start in a room - starting room should be a clever one that connects to the entrance door
		Framework::UsedLibraryStored<Framework::DoorType> startingEntranceDoorType; // rather an optional one (inDoor/door)
		Framework::UsedLibraryStored<Framework::DoorType> startingExitDoorType; // to our pilgrimage (outDoor/door)
		bool startingDoorConnectWithPreviousPilgrimage = true; // if false, allows for teleporting etc
		Array<NoCellsRoom> noCellsRooms; // for noCells, includes startingRoom, might be empty if no other than starting room should be used, will always auto include starting room and set starting room from this
		Optional<int> limitNoCellRoomsPregeneration; // if set, only this certain number will be generated initially, the rest will be generated when there's time
		// if you want to have starting room using a different exit door, set it up as both startingRoomType and first of noCellsRooms

		bool allowDelayedObjectCreationOnStart = false; // if set to true, won't be creating all DOC objects immediately (we may get sudden bumps unless it is managed specifically)

		Framework::UsedLibraryStored<Framework::SceneryType> noDoorMarkerSceneryType; // placed when there is no door (yet or never going to be)

		Framework::UsedLibraryStored<Framework::RegionType> cellRegionType; // cell region type that works as a container for the actual room/region
	
		bool allowHighLevelRegionAtStart = false;
		HighLevelRegionSet highLevelRegionSet;
		HighLevelRegionSet cellLevelRegionSet;

		// for cells, will use info provided via tags/variables to generate map
		CellSet genericCellSet;
		CellSet forRulesCellSet;
		CellSet forObjectivesCellSet;
		CellSet startingCellSet;

		Array<Rule> rules;
		Array<Objective> objectives;
		bool sequentialObjectives = false;
		Optional<DirFourClockwise::Type> objectivesGeneralDir; // general direction in which objectives move

		SkipContentSet skipContentSet;

		Array<RandomPilgrimageDevices> randomPilgrimageDevices;
		Array<DistancedPilgrimageDevices> distancedPilgrimageDevices;
		int distancedPilgrimageDevicesTileSize = 20;

		ArrayStatic<Framework::UsedLibraryStored<Framework::TexturePart>, DoorDirectionCombination::NUM> directionTextureParts;
		Framework::UsedLibraryStored<Framework::TexturePart> towardsObjectiveTextuerPart;

		BackgroundDefinition const * find_background_definition(VectorInt2 const& _at) const;

		SpecialRules specialRules;

		friend class Pilgrimage;
	};

};
