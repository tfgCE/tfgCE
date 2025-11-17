#include "pilgrimageInstanceOpenWorld.h"

#include "pilgrimage.h"
#include "pilgrimageOpenWorldDefinition.h"

#include "..\reportBug.h"

#include "..\game\game.h"
#include "..\game\gameDirector.h"
#include "..\game\gameState.h"
#include "..\library\library.h"
#include "..\loaders\hub\scenes\lhs_beAtRightPlace.h"
#include "..\modules\custom\mc_pilgrimageDevice.h"
#include "..\modules\gameplay\moduleMultiplePilgrimageDevice.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\roomGenerators\roomGenerationInfo.h"
#include "..\utils\overridePilgrimType.h"

#include "..\..\core\collision\model.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\pieceCombiner\pieceCombinerDebugVisualiser.h"
#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#include "..\..\core\random\randomUtils.h"
#include "..\..\core\system\faultHandler.h"
#include "..\..\core\system\video\video3dPrimitives.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

#include "..\..\framework\debug\testConfig.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\moduleAppearanceImpl.inl"
#include "..\..\framework\module\moduleController.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\nav\navSystem.h"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\object\scenery.h"
#include "..\..\framework\world\pseudoRoomRegion.h"
#include "..\..\framework\world\regionType.h"
#include "..\..\framework\world\regionGeneratorInfo.h"
#include "..\..\framework\world\roomRegionVariables.inl"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define COMBINED_CELLS_COUNT 50

//

//#define DEBUG_TOWARDS_OBJECT_SHOW_ON_MAP
//#define DEBUG_TOWARDS_OBJECT_PROPAGATION
//#define DEBUG_TOWARDS_OBJECT_SET_DIRS

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifdef AN_OUTPUT
		#define AN_OUTPUT_DEV_INFO
		#define AN_IMPORTANT_INFO

		#define AN_TAG_OPEN_WORLD_ROOMS

		#ifdef AN_ALLOW_EXTENSIVE_LOGS
			#define AN_OUTPUT_PILGRIMAGE_DEVICES
			#define AN_OUTPUT_PILGRIMAGE_GENERATION
			//#define AN_OUTPUT_PILGRIMAGE_GENERATION_DETAILED
			//#define AN_OUTPUT_TOWARDS_OBJECTIVE
			//#define AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
			#define AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_JUST_ADD_INFO
			//#define AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_DETAILED_ITERATION
			//#define INSPECT_FILL_DISTANCES_FOR_DOORS
		#endif

		REMOVE_AS_SOON_AS_POSSIBLE_
		#ifndef AN_OUTPUT_PILGRIMAGE_GENERATION
			#define AN_OUTPUT_PILGRIMAGE_GENERATION
		#endif
	#endif

	#ifdef AN_OUTPUT_WORLD_GENERATION
		// use this one to show generation process
		// regions/rooms
		//#define OUTPUT_GENERATION_DEBUG_VISUALISER
		// pilgimage devices tiles
		//#define OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
		#define OUTPUT_GENERATION_EXTRA_CELL_EXITS
	#endif
#endif

#ifdef AN_DEVELOPMENT
// to generate only cell at distance 1
#define SHORT_VISIBILITY
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define TEST_CELL 0,0
#define TEST_CELL_DIST 10
//#define RANDOM_WANDERER
//#define CHECK_ALL_CELLS
//#define DONT_OBFUSCATE_MAP_SYMBOLS
//#define PRE_GENERATE_MAP
//#define PRE_GENERATE_MAP_MARK_KNOWN
//#define VISUALISE_CELL_GENERATION
//#define VISUALISE_CELL_GENERATION_PRE
//#define TEST_CELL_GENERATION_BASIC
//#define TEST_CELL_GENERATION_SMALL
//#define TEST_CELL_GENERATION_BIG
//#define TEST_CELL_GENERATION_RANDOM
//#define SHOW_MAP_BEFORE_WE_START
//#define OUTPUT_CELL_GENERATION_DETAILS
#endif

#define MAX_OVERRIDE_DIST PilgrimageOpenWorldDefinition::maxDependentDist

//

using namespace TeaForGodEmperor;

//

// POIs
DEFINE_STATIC_NAME(door);
DEFINE_STATIC_NAME(inDoor);
DEFINE_STATIC_NAME(outDoor);
DEFINE_STATIC_NAME_STR(vrAnchor, TXT("vr anchor"));
DEFINE_STATIC_NAME_STR(pilgrimSpawnPoint, TXT("pilgrim spawn point"));
DEFINE_STATIC_NAME_STR(noCellsExitDoor, TXT("noCellsExitDoor"));
DEFINE_STATIC_NAME_STR(noCellsPrevDoor, TXT("noCellsPrevDoor"));
DEFINE_STATIC_NAME_STR(noCellsNextDoor, TXT("noCellsNextDoor"));

// room/region variables
DEFINE_STATIC_NAME(visibleMapCellAt);
DEFINE_STATIC_NAME(visibleMapCellAtOffset);

// room variables
DEFINE_STATIC_NAME(handleDoorOperation); // if set to true, pilgrimage won't be opening and closing doors
DEFINE_STATIC_NAME(handleExitDoorOperation); // if set to true, pilgrimage won't be opening and closing doors
DEFINE_STATIC_NAME(allowDelayedObjectCreationForStartingRoom); // if set to true for starting room will allow delayed object creation (won't wait)
DEFINE_STATIC_NAME(checkpointRoomTagged); // will store in game state where we should restart

// room tags
DEFINE_STATIC_NAME(startingRoom);
DEFINE_STATIC_NAME(noCellsRoom);
DEFINE_STATIC_NAME(vrCorridor);
DEFINE_STATIC_NAME(towardsObjectiveCost);
DEFINE_STATIC_NAME(vrElevatorAltitude);
DEFINE_STATIC_NAME(openWorldX);
DEFINE_STATIC_NAME(openWorldY);
DEFINE_STATIC_NAME(cellRoomIdx);
DEFINE_STATIC_NAME(openWorldExterior);
DEFINE_STATIC_NAME(checkpointRoom);
DEFINE_STATIC_NAME(noSafeSpawn);
DEFINE_STATIC_NAME(inaccessible);

// upgrade machine tags+variables
DEFINE_STATIC_NAME(upgradeMachine);
DEFINE_STATIC_NAME(chosenOption);
DEFINE_STATIC_NAME(upgradeMachineShouldRemainDisabled);

// door in room tags
DEFINE_STATIC_NAME(openWorldCellDistFromUp);		// within room one has N
DEFINE_STATIC_NAME(openWorldCellDistFromRight);		// other have N+1
DEFINE_STATIC_NAME(openWorldCellDistFromDown);		// 
DEFINE_STATIC_NAME(openWorldCellDistFromLeft);		// 
DEFINE_STATIC_NAME(openWorldCellDistFromSpecial);	// used for special (to door marked "special")
DEFINE_STATIC_NAME(openWorldCellDistFromObjective);	// used for custom objectives (transition room, etc)
DEFINE_STATIC_NAME(layoutCueUp);					// helpful when telling in which direction does it go
DEFINE_STATIC_NAME(layoutCueRight);					// propagation is blocked by cue or cell separators
DEFINE_STATIC_NAME(layoutCueDown);					// this is only used when creating levels, not used for navigation
DEFINE_STATIC_NAME(layoutCueLeft);					// 
DEFINE_STATIC_NAME(layoutCueCapUp);					// creates start on the other side of the door
DEFINE_STATIC_NAME(layoutCueCapRight);				//
DEFINE_STATIC_NAME(layoutCueCapDown);				//
DEFINE_STATIC_NAME(layoutCueCapLeft);				// 
DEFINE_STATIC_NAME(layoutCueStartUp);				// auto from layoutCue or layoutCueCap
DEFINE_STATIC_NAME(layoutCueStartRight);			//
DEFINE_STATIC_NAME(layoutCueStartDown);				//
DEFINE_STATIC_NAME(layoutCueStartLeft);				// 
DEFINE_STATIC_NAME(layoutCueDistFromUp);			// 
DEFINE_STATIC_NAME(layoutCueDistFromRight);			// 
DEFINE_STATIC_NAME(layoutCueDistFromDown);			// 
DEFINE_STATIC_NAME(layoutCueDistFromLeft);			// 
DEFINE_STATIC_NAME_STR(dt_entranceDoor, TXT("entranceDoor"));
DEFINE_STATIC_NAME_STR(dt_exitDoor, TXT("exitDoor"));
DEFINE_STATIC_NAME_STR(dt_startingRoomOutDoor, TXT("startingRoomOutDoor"));
DEFINE_STATIC_NAME_STR(dt_startingRoomInDoor, TXT("startingRoomInDoor"));
DEFINE_STATIC_NAME_STR(dt_noCellsExitDoor, TXT("noCellsExitDoor"));
DEFINE_STATIC_NAME_STR(dt_noCellsPrevDoor, TXT("noCellsPrevDoor"));
DEFINE_STATIC_NAME_STR(dt_noCellsNextDoor, TXT("noCellsNextDoor"));

// slot connectors
DEFINE_STATIC_NAME(left);
DEFINE_STATIC_NAME(right);
DEFINE_STATIC_NAME(down);
DEFINE_STATIC_NAME(up);
DEFINE_STATIC_NAME_STR(dirLeft, TXT("dir left"));
DEFINE_STATIC_NAME_STR(dirLeftSideDown, TXT("dir left side down"));
DEFINE_STATIC_NAME_STR(dirLeftSideUp, TXT("dir left side up"));
DEFINE_STATIC_NAME_STR(dirRight, TXT("dir right"));
DEFINE_STATIC_NAME_STR(dirRightSideDown, TXT("dir right side down"));
DEFINE_STATIC_NAME_STR(dirRightSideUp, TXT("dir right side up"));
DEFINE_STATIC_NAME_STR(dirDown, TXT("dir down"));
DEFINE_STATIC_NAME_STR(dirDownSideLeft, TXT("dir down side left"));
DEFINE_STATIC_NAME_STR(dirDownSideRight, TXT("dir down side right"));
DEFINE_STATIC_NAME_STR(dirUp, TXT("dir up"));
DEFINE_STATIC_NAME_STR(dirUpSideLeft, TXT("dir up side left"));
DEFINE_STATIC_NAME_STR(dirUpSideRight, TXT("dir up side right"));
DEFINE_STATIC_NAME(special);
DEFINE_STATIC_NAME_STR(dirSpecial, TXT("dir special"));

// no door marker tags
DEFINE_STATIC_NAME(noDoorMarkerPending);

// connector tags
DEFINE_STATIC_NAME(openWorldLeft);
DEFINE_STATIC_NAME(openWorldRight);
DEFINE_STATIC_NAME(openWorldDown);
DEFINE_STATIC_NAME(openWorldUp);
DEFINE_STATIC_NAME(openWorldSpecial);

// door variables
DEFINE_STATIC_NAME(aCell);
DEFINE_STATIC_NAME(bCell);

// game tags
DEFINE_STATIC_NAME(linear);

// region tags
DEFINE_STATIC_NAME(cellLevelRegion);
DEFINE_STATIC_NAME(highLevelRegion);

// region variables
DEFINE_STATIC_NAME(hlRegionPriority);
DEFINE_STATIC_NAME(cellLevelAt);
DEFINE_STATIC_NAME(cellLevelAt2);
DEFINE_STATIC_NAME(cellLevelNotAt);
DEFINE_STATIC_NAME(cellLevelNotAt2);

// region tags, region generator tags and variables
DEFINE_STATIC_NAME(openWorld); // bool
DEFINE_STATIC_NAME(openWorldDir); // name
DEFINE_STATIC_NAME(openWorldDirUp); // bool
DEFINE_STATIC_NAME(openWorldDirDown); // bool
DEFINE_STATIC_NAME(openWorldDirRight); // bool
DEFINE_STATIC_NAME(openWorldDirLeft); // bool
DEFINE_STATIC_NAME(openWorldCellContext); // VectorInt2 (access via wmp tool openWorld get="cellContext"

// generation tags
DEFINE_STATIC_NAME(containsPilgrimageDevice);
DEFINE_STATIC_NAME(containsRoomBasedPilgrimageDevice);
DEFINE_STATIC_NAME(containsSceneryPilgrimageDevice);

// cell custom parameters
DEFINE_STATIC_NAME(cellMainRoomType);
DEFINE_STATIC_NAME(cellDistanceVisibilityCoef);
DEFINE_STATIC_NAME(cellPriority);
DEFINE_STATIC_NAME(avoidSameNeighbours);
DEFINE_STATIC_NAME(cellType);
DEFINE_STATIC_NAME(cellRemove1ExitChance);
DEFINE_STATIC_NAME(cellRemove2ExitsChance);
DEFINE_STATIC_NAME(cellRemove3ExitsChance);
DEFINE_STATIC_NAME(cellRemove4ExitsChance);
DEFINE_STATIC_NAME(cellIgnoreRemovingExitChance);
DEFINE_STATIC_NAME(noLeftExit);
DEFINE_STATIC_NAME(noRightExit);
DEFINE_STATIC_NAME(noUpExit);
DEFINE_STATIC_NAME(noDownExit);
DEFINE_STATIC_NAME(sideExitsAtUp);
DEFINE_STATIC_NAME(sideExitsAtDown);
DEFINE_STATIC_NAME(sideExitsAtLeft);
DEFINE_STATIC_NAME(sideExitsAtRight);
DEFINE_STATIC_NAME(leftExitRequired);
DEFINE_STATIC_NAME(rightExitRequired);
DEFINE_STATIC_NAME(upExitRequired);
DEFINE_STATIC_NAME(downExitRequired);
DEFINE_STATIC_NAME(allExitsRequired);
DEFINE_STATIC_NAME(allPossibleExitsRequired);
DEFINE_STATIC_NAME(forceDir);
DEFINE_STATIC_NAME(specialLocalDir);
DEFINE_STATIC_NAME(dirFixed);
DEFINE_STATIC_NAME(dirImportant);
DEFINE_STATIC_NAME(dirImportantUpExit);
DEFINE_STATIC_NAME(dirImportantRightExit);
DEFINE_STATIC_NAME(dirImportantDownExit);
DEFINE_STATIC_NAME(dirImportantLeftExit);
DEFINE_STATIC_NAME(dirAlignDependent);
DEFINE_STATIC_NAME(axisAlignDependent);
DEFINE_STATIC_NAME(alignDependentTowards);
DEFINE_STATIC_NAME(alignDependentAway);
DEFINE_STATIC_NAME(dependsOnCellType);
DEFINE_STATIC_NAME(dependentCellTypes);
DEFINE_STATIC_NAME(maxDependentRadius);
DEFINE_STATIC_NAME(maxDependentUp);
DEFINE_STATIC_NAME(maxDependentRight);
DEFINE_STATIC_NAME(maxDependentDown);
DEFINE_STATIC_NAME(maxDependentLeft);
DEFINE_STATIC_NAME(sceneryMeshGenerator);
DEFINE_STATIC_NAME(closeSceneryMeshGenerator);
DEFINE_STATIC_NAME(farSceneryMeshGenerator);
DEFINE_STATIC_NAME(forMapCentreLocal);
DEFINE_STATIC_NAME(forMapColour); // read from cell, if fails, read from high level region
DEFINE_STATIC_NAME(additionalCellInfoTagged);
DEFINE_STATIC_NAME(forMapLineModel);
DEFINE_STATIC_NAME(forMapLineModelAdditional);
DEFINE_STATIC_NAME(forMapLineModelAlwaysOtherwise);
DEFINE_STATIC_NAME(forMapSideSensitiveUpExit);
DEFINE_STATIC_NAME(forMapSideSensitiveLeftExit);
DEFINE_STATIC_NAME(forMapSideSensitiveDownExit);
DEFINE_STATIC_NAME(forMapSideSensitiveRightExit);
DEFINE_STATIC_NAME(forMapSideExitsOffset);
DEFINE_STATIC_NAME(airshipObstacleHeight);

// background scenery variables/parameters
DEFINE_STATIC_NAME(backgroundScenery); // close or far
DEFINE_STATIC_NAME(closeBackgroundScenery);
DEFINE_STATIC_NAME(farBackgroundScenery);
DEFINE_STATIC_NAME(medFarBackgroundScenery);
DEFINE_STATIC_NAME(veryFarBackgroundScenery);
DEFINE_STATIC_NAME(noMovementCollision);
DEFINE_STATIC_NAME(addVistaWindows);

// shader params
DEFINE_STATIC_NAME(textureFloorScale);
DEFINE_STATIC_NAME(textureFloorOffset);
DEFINE_STATIC_NAME(textureFloorX);
DEFINE_STATIC_NAME(textureFloorY);
DEFINE_STATIC_NAME(connectionPending);

// general door type parameters
DEFINE_STATIC_NAME(withDoorDirections);

// game state
DEFINE_STATIC_NAME(pilgrimAt);
DEFINE_STATIC_NAME(pilgrimComingFrom);
DEFINE_STATIC_NAME(pilgrimInCheckpointRoomTagged);
DEFINE_STATIC_NAME(pilgrimInHavenAt);
DEFINE_STATIC_NAME(upgradeInStartingRoomUnlocked);
DEFINE_STATIC_NAME(currentObjectiveIdx);
DEFINE_STATIC_NAME(timeSinceLastCellBlocked);
DEFINE_STATIC_NAME(blockedAwayCells);

// test tags
DEFINE_STATIC_NAME(testObjectiveId);
DEFINE_STATIC_NAME(testObjectiveIdx);
DEFINE_STATIC_NAME(testPilgrimage);
DEFINE_STATIC_NAME(testOpenWorldCell);
DEFINE_STATIC_NAME(testStartInRoomTag);
DEFINE_STATIC_NAME(testCellRoomIdx);
DEFINE_STATIC_NAME(testCellRoomLocation);
DEFINE_STATIC_NAME(testCellRoomRotation);
DEFINE_STATIC_NAME(testCellRoomVRLocation);
DEFINE_STATIC_NAME(testCellRoomVRRotation);

//

#ifndef BUILD_PUBLIC_RELEASE
Optional<VectorInt2> testOpenWorldCell;
Optional<Name> testStartInRoomTag;
Optional<int> testCellRoomIdx;
Optional<Vector3> testCellRoomLocation;
Optional<Rotator3> testCellRoomRotation;
Optional<Vector3> testCellRoomVRLocation;
Optional<Rotator3> testCellRoomVRRotation;
#endif

#ifdef AN_ALLOW_AUTO_TEST
bool checkAllCells_active =
#ifdef CHECK_ALL_CELLS
	true;
#else
	false;
#endif
bool checkAllCells_processAllJobs = false;
bool checkAllCells_goingUp = true;
Optional<VectorInt2> checkAllCells_goingSideFrom;
VectorInt2 checkAllCells_moveBy = VectorInt2::zero;
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool showFailDetails = false;
#endif

//

int to_world_address_id(VectorInt2 const& _at)
{
	int result = 0;
	result |= (0xffff & _at.x);
	result |= (0xffff & _at.y) << 16;
	return result; // should be far enough
}

static VectorInt2 be_dir(VectorInt2 _dir)
{
	_dir.x = clamp(_dir.x, -1, 1);
	_dir.y = clamp(_dir.y, -1, 1);
	return _dir;
}

//

struct HighLevelRegionCreator
{
	struct Context
	{
		ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(Context, bool, justGetRegionType, just_get_region_type, false, true);

		Context() {}
	};
	// in
	VectorInt2 idx;
	Random::Generator randomSeed;
	bool baseOnIndex; // if true, will shuffle available set and will choose random from it by modding idx, also the randomSeed should be the same for all idx
	bool allowMissionDifficultyModifier; // if true, will use mission difficulty modifier to choose from available elements
	PilgrimageOpenWorldDefinition::HighLevelRegionSet const& highLevelRegionSet;
	SafePtr<Framework::Region> parentRegion;
	Optional<VectorInt2> cellAt;
	Name hlRegionPriorityVar;
	Name cellAtRangeVar;
	Name cellAtRange2Var;
	Name cellNotAtRangeVar;
	Name cellNotAtRange2Var;

	// auto
	Framework::SubWorld* subWorld;

	// out
	Framework::RegionType* regionType;
	SafePtr<Framework::Region> topRegion;
	SafePtr<Framework::Region> containerRegion;
	SafePtr<Framework::Room> aiManagerRoom;

	HighLevelRegionCreator(
		VectorInt2 const & _idx,
		PilgrimageOpenWorldDefinition::HighLevelRegionSet const& _highLevelRegionSet,
		Random::Generator const& _randomSeed,
		bool _baseOnIndex,
		bool _allowMissionDifficultyModifier,
		Framework::Region* _parentRegion,
		Optional<VectorInt2> const & _cellAt = NP,
		Name const & _hlRegionPriorityVar = Name::invalid(),
		Name const & _cellAtRangeVar = Name::invalid(),
		Name const & _cellAtRange2Var = Name::invalid(),
		Name const & _cellNotAtRangeVar = Name::invalid(),
		Name const & _cellNotAtRange2Var = Name::invalid()
	)
	: idx(_idx)
	, randomSeed(_randomSeed)
	, baseOnIndex(_baseOnIndex)
	, allowMissionDifficultyModifier(_allowMissionDifficultyModifier)
	, highLevelRegionSet(_highLevelRegionSet)
	, parentRegion(_parentRegion)
	, cellAt(_cellAt)
	, hlRegionPriorityVar(_hlRegionPriorityVar)
	, cellAtRangeVar(_cellAtRangeVar)
	, cellAtRange2Var(_cellAtRange2Var)
	, cellNotAtRangeVar(_cellNotAtRangeVar)
	, cellNotAtRange2Var(_cellNotAtRange2Var)
	{
		if (auto* r = parentRegion.get())
		{
			subWorld = r->get_in_sub_world();
		}
		else
		{
			subWorld = nullptr;
		}
	}

	void limit_available_using_mission_difficulty_modifier(REF_ Array<PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available>& _available)
	{
		if (!allowMissionDifficultyModifier) return;
		auto* ms = MissionState::get_current();
		if (!ms) return;
		auto* mdm = ms->get_difficulty_modifier();
		if (!mdm) return;

		int matchCount = 0;
		int noMatchCount = 0;
		for_every(a, _available)
		{
			if (auto* ls = a->libraryStored.get())
			{
				if (mdm->get_cell_region_tag_condition().check(ls->get_tags()))
				{
					++matchCount;
				}
				else
				{
					++noMatchCount;
				}
			}
		}

		// remove only if removing wouldn't remove all or nothing (if nothing, there's no point, if all, there would be no content and we need something!)
		if (matchCount != 0 && noMatchCount != 0)
		{
			for (int i = 0; i < _available.get_size(); ++i)
			{
				auto* a = &_available[i];
				if (auto* ls = a->libraryStored.get())
				{
					if (!mdm->get_cell_region_tag_condition().check(ls->get_tags()))
					{
						_available.remove_at(i);
						--i;
					}
				}
			}
		}
	}

	void run(Context const & _context = Context())
	{
		scoped_call_stack_info(TXT("HighLevelRegionCreator::run"));

		Game* game = Game::get_as<Game>();

		//

		auto const& hlrSet = highLevelRegionSet;

		if (hlrSet.is_empty())
		{
			return;
		}

		Random::Generator rg = randomSeed;
		Random::Generator userg = randomSeed;

		ARRAY_PREALLOC_SIZE(PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available, available, hlrSet.available.get_size());

		{
			scoped_call_stack_info(TXT("find available"));
			for_every(a, hlrSet.available)
			{
				if (GameDefinition::check_chosen(a->forGameDefinitionTagged))
				{
					available.push_back(*a);
				}
			}
		}

		limit_available_using_mission_difficulty_modifier(REF_ available);

		if (baseOnIndex)
		{
			scoped_call_stack_info(TXT("shuffle"));
			Random::Generator shuffleRG = randomSeed;
			for_count(int, i, available.get_size() * 3)
			{
				int a = shuffleRG.get_int(available.get_size());
				int b = shuffleRG.get_int(available.get_size());
				swap(available[a], available[b]);
			}

			rg.advance_seed(idx.x, idx.y);
			userg.advance_seed(idx.x, idx.y);
		}
		userg.advance_seed(23508, 2251);

		//
		Framework::RegionType* chosenRegionType = nullptr;
		Optional<int> highestHLRegionPriority;
		PilgrimageOpenWorldDefinition::HighLevelRegionSet const * chosenFromRegionSet = nullptr;

		int basedIdx = mod(idx.x + idx.y, available.get_size());

		while (! available.is_empty())
		{
			int hlrsIdx = 0;
			{
				scoped_call_stack_info(TXT("choose hlrsIdx"));
				if (baseOnIndex)
				{
					hlrsIdx = mod(basedIdx, available.get_size()); // if won't fit, we will remove it, effectively going to the next one
				}
				else
				{
					hlrsIdx = RandomUtils::ChooseFromContainer<Array<PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available>, PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available>::choose(userg,
						available, [](PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available const& a) { return a.probCoef; });
				}
			}

			{
				scoped_call_stack_info(TXT("get from available"));
				Framework::RegionType* hlRegionType = fast_cast<Framework::RegionType>(available[hlrsIdx].libraryStored.get());

				if (hlRegionType)
				{
					Optional<int> hlRegionPriority;
					if (hlRegionPriorityVar.is_valid())
					{
						if (auto* ex = hlRegionType->get_custom_parameters().get_existing<int>(hlRegionPriorityVar))
						{
							hlRegionPriority = *ex;
						}
					}
					if (!highestHLRegionPriority.is_set() || (hlRegionPriority.is_set() && hlRegionPriority.get() > highestHLRegionPriority.get()))
					{
						bool isOk = true;
						if (cellAt.is_set())
						{
							if (isOk && (cellAtRangeVar.is_valid() || cellAtRange2Var.is_valid()))
							{
								bool inside = false;
								bool anyVarPresent = false;
								for_count(int, i, 2)
								{
									Name useVar = i == 0 ? cellAtRangeVar : cellAtRange2Var;
									if (useVar.is_valid())
									{
										if (auto* ex = hlRegionType->get_custom_parameters().get_existing<RangeInt2>(useVar))
										{
											anyVarPresent = true;
											RangeInt2 cellLevelAt = *ex;
											bool insideThis = true;
											if (!cellLevelAt.x.is_empty() && !cellLevelAt.x.does_contain(cellAt.get().x))
											{
												insideThis = false;
											}
											if (!cellLevelAt.y.is_empty() && !cellLevelAt.y.does_contain(cellAt.get().y))
											{
												insideThis = false;
											}
											inside |= insideThis;
										}
									}
								}
								if (!inside && anyVarPresent)
								{
									isOk = false;
								}
							}
							if (isOk && cellNotAtRangeVar.is_valid())
							{
								if (auto* ex = hlRegionType->get_custom_parameters().get_existing<RangeInt2>(cellNotAtRangeVar))
								{
									RangeInt2 cellLevelNotAt = *ex;
									bool condX = cellLevelNotAt.x.is_empty() || cellLevelNotAt.x.does_contain(cellAt.get().x);
									bool condY = cellLevelNotAt.y.is_empty() || cellLevelNotAt.y.does_contain(cellAt.get().y);
									if (condX && condY)
									{
										isOk = false;
									}
								}
							}
							if (isOk && cellNotAtRange2Var.is_valid())
							{
								if (auto* ex = hlRegionType->get_custom_parameters().get_existing<RangeInt2>(cellNotAtRange2Var))
								{
									RangeInt2 cellLevelNotAt = *ex;
									bool condX = cellLevelNotAt.x.is_empty() || cellLevelNotAt.x.does_contain(cellAt.get().x);
									bool condY = cellLevelNotAt.y.is_empty() || cellLevelNotAt.y.does_contain(cellAt.get().y);
									if (condX && condY)
									{
										isOk = false;
									}
								}
							}
						}
						if (isOk)
						{
							highestHLRegionPriority = hlRegionPriority;
							chosenRegionType = hlRegionType;
							chosenFromRegionSet = available[hlrsIdx].highLevelSubRegionSet.get();
						}
					}
				}
			}

			available.remove_at(hlrsIdx);
		}

		if (chosenRegionType)
		{
			regionType = chosenRegionType;
			if (!_context.justGetRegionType)
			{
				an_assert(subWorld);

				scoped_call_stack_info(TXT("create region"));

				Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();

				ArrayStatic<Framework::Region*, 8> regionLadder; SET_EXTRA_DEBUG_INFO(regionLadder, TXT("PilgrimageInstanceOpenWorld::HighLevelRegionCreator::run.regionLadder"));

				// high-level region
				{
					scoped_call_stack_info(TXT("create high level region"));

					if (chosenRegionType) { chosenRegionType->load_on_demand_if_required(); }
					Game::get()->perform_sync_world_job(TXT("create high level region"), [this, chosenRegionType, rg]()
						{
							scoped_call_stack_info(TXT("create high level region (sync)"));
							topRegion = new Framework::Region(subWorld, parentRegion.get(), chosenRegionType, rg);
							topRegion->set_world_address_id(to_world_address_id(idx));
						});

					todo_note(TXT("this should be removed when the places are properly defined"));
					//topRegion->be_overriding_region();

					topRegion->run_wheres_my_point_processor_setup();
					topRegion->generate_environments();

					containerRegion = topRegion;

					regionLadder.push_back(topRegion.get());
				}

				// high-level sub region(s) - the whole ladder
				{
					scoped_call_stack_info(TXT("sub region"));

					Random::Generator userg = rg;
					userg.advance_seed(97835, 97235);

					PilgrimageOpenWorldDefinition::HighLevelRegionSet const* fromSet = chosenFromRegionSet;
					while (fromSet && !fromSet->is_empty())
					{
						scoped_call_stack_info(TXT("get from available"));
						ARRAY_PREALLOC_SIZE(PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available, available, fromSet->available.get_size());
						for_every(a, fromSet->available)
						{
							if (GameDefinition::check_chosen(a->forGameDefinitionTagged))
							{
								available.push_back(*a);
							}
						}

						limit_available_using_mission_difficulty_modifier(REF_ available);

						bool chosen = false;
						while (!available.is_empty() && !chosen)
						{
							int idx = RandomUtils::ChooseFromContainer<Array<PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available>, PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available>::choose(userg,
								available, [](PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available const& a) { return a.probCoef; });

							{
								if (Framework::RegionType* hlSubRegionType = fast_cast<Framework::RegionType>(available[idx].libraryStored.get()))
								{
									Framework::Region* subRegion;
									if (hlSubRegionType) { hlSubRegionType->load_on_demand_if_required(); }
									Game::get()->perform_sync_world_job(TXT("create high level sub region"), [this, hlSubRegionType, &subRegion, userg]()
										{
											subRegion = new Framework::Region(subWorld, containerRegion.get(), hlSubRegionType, userg);
											topRegion->set_world_address_id(0);
										});

									todo_note(TXT("this should be removed when the places are properly defined"));
									//subRegion->be_overriding_region();

									subRegion->run_wheres_my_point_processor_setup();
									subRegion->generate_environments();

									containerRegion = subRegion;

									regionLadder.push_back(subRegion);
								}

								// venture further
								fromSet = available[idx].highLevelSubRegionSet.get();

								chosen = true;
							}
						}
					}
				}

				{
					scoped_call_stack_info(TXT("create ai manager room"));
					Game::get()->perform_sync_world_job(TXT("create ai manager room"), [this]()
						{
							aiManagerRoom = new Framework::Room(subWorld, containerRegion.get(), nullptr, Random::Generator().be_zero_seed());
							ROOM_HISTORY(aiManagerRoom, TXT("ai manager room"));
						});
				}

				{
					scoped_call_stack_info(TXT("call post generate fro all"));
					// generate ai managers on the whole ladder
					for_every_ptr(region, regionLadder)
					{
						if (auto* rt = region->get_region_type())
						{
							if (auto* rgi = rt->get_region_generator_info())
							{
								scoped_call_stack_info(TXT("post generate"));
								rgi->post_generate(region);
							}
						}
					}
				}

				game->request_ready_and_activate_all_inactive(activationGroup.get());
				game->pop_activation_group(activationGroup.get());
			}
		}
	}
};

//

PilgrimageInstanceOpenWorld::GetRightDoorDirectionTexturePartUse::GetRightDoorDirectionTexturePartUse(PilgrimageOpenWorldDefinition const* _owDef, PilgrimageInstanceOpenWorld::Cell const* _cell, bool _showDirectionsAsObjective)
: owDef(_owDef)
, showDirectionsAsObjective(_showDirectionsAsObjective)
{
	an_assert(owDef);
	if (_cell)
	{
		memory_copy(exits, _cell->exits, sizeof(bool) * DirFourClockwise::NUM);
	}
	else
	{
		memory_zero(exits, sizeof(bool) * DirFourClockwise::NUM);
	}
}

Framework::TexturePartUse PilgrimageInstanceOpenWorld::GetRightDoorDirectionTexturePartUse::get_for(DirFourClockwise::Type _dir) const
{
	Framework::TexturePartUse result;

	if (showDirectionsAsObjective)
	{
		result.texturePart = owDef->get_towards_objective_texture_part();
		return result;
	}

	an_assert(owDef);
	// these indices refer to local dirs
	int lr = exits[mod<int>(_dir + 1, DirFourClockwise::NUM)];
	int ld = exits[mod<int>(_dir + 2, DirFourClockwise::NUM)];
	int ll = exits[mod<int>(_dir + 3, DirFourClockwise::NUM)];

	// up is always considered open, we wouldn't be doing a check
	DoorDirectionCombination::Type ddc = DoorDirectionCombination::Type::Up;
	if (lr)
	{
		if (ld)
		{
			if (ll)
			{
				ddc = DoorDirectionCombination::UpRDL;
			}
			else
			{
				ddc = DoorDirectionCombination::UpRD;
			}
		}
		else
		{
			if (ll)
			{
				ddc = DoorDirectionCombination::UpRL;
			}
			else
			{
				ddc = DoorDirectionCombination::UpR;
			}
		}
	}
	else
	{
		if (ld)
		{
			if (ll)
			{
				ddc = DoorDirectionCombination::UpRD;
				result.mirroredX = true;
			}
			else
			{
				ddc = DoorDirectionCombination::UpD;
			}
		}
		else
		{
			if (ll)
			{
				ddc = DoorDirectionCombination::UpR;
				result.mirroredX = true;
			}
			else
			{
				ddc = DoorDirectionCombination::Up;
			}
		}
	}
	if (!exits[_dir])
	{
		ddc = (DoorDirectionCombination::Type)(ddc + DoorDirectionCombination::NoUp - DoorDirectionCombination::Up);
	}
	result.texturePart = owDef->get_direction_texture_part(ddc);
	result.rotated90 = _dir;
	return result;
}

Framework::TexturePartUse PilgrimageInstanceOpenWorld::GetRightDoorDirectionTexturePartUse::get_exit_to(DirFourClockwise::Type _dir) const
{
	an_assert(owDef);

	Framework::TexturePartUse result;

	if (showDirectionsAsObjective)
	{
		result.texturePart = owDef->get_towards_objective_texture_part();
		return result;
	}

	result.texturePart = owDef->get_direction_texture_part(DoorDirectionCombination::UpExitTo);
	result.rotated90 = _dir;
	return result;
}

Framework::TexturePartUse PilgrimageInstanceOpenWorld::GetRightDoorDirectionTexturePartUse::get_entered_from(DirFourClockwise::Type _dir) const
{
	an_assert(owDef);

	Framework::TexturePartUse result;

	if (showDirectionsAsObjective)
	{
		result.texturePart = owDef->get_towards_objective_texture_part();
		return result;
	}

	result.texturePart = owDef->get_direction_texture_part(DoorDirectionCombination::UpEnteredFrom);
	result.rotated90 = _dir;
	return result;
}

//

REGISTER_FOR_FAST_CAST(PilgrimageInstanceOpenWorld);

#ifdef AN_ALLOW_AUTO_TEST
void PilgrimageInstanceOpenWorld::check_all_cells(bool _check, bool _reset, bool _processAllJobs)
{
	checkAllCells_active = _check;
	checkAllCells_processAllJobs = _processAllJobs;

	if (_reset)
	{
		checkAllCells_goingUp = true;
		checkAllCells_goingSideFrom.clear();
		checkAllCells_moveBy = VectorInt2::zero;
	}
}
#endif

PilgrimageInstanceOpenWorld::PilgrimageInstanceOpenWorld(Game* _game, Pilgrimage* _pilgrimage)
: base(_game, _pilgrimage)
{
	okRoomToSpawnTagCondition.load_from_string(String(TXT("(not vistaSceneryRoom) and (not noSafeSpawn)")));

#ifdef AN_DEVELOPMENT_OR_PROFILER
	SET_EXTRA_DEBUG_INFO(wantsToCreateCellAt, TXT("PilgrimageInstanceOpenWorld.wantsToCreateCellAt"));
#endif
}

PilgrimageInstanceOpenWorld::~PilgrimageInstanceOpenWorld()
{
}

float PilgrimageInstanceOpenWorld::get_cell_size() const
{
	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		return owDef->get_cell_size();
	}
	return 40.0f;
}

float PilgrimageInstanceOpenWorld::get_cell_size_inner() const
{
	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		return owDef->get_cell_size_inner();
	}
	return get_cell_size() * 0.9f;
}

void PilgrimageInstanceOpenWorld::set_last_visited_haven(VectorInt2 const& _at)
{
	output(TXT("remember last visited haven %ix%i"), _at.x, _at.y);
	lastVisitedHavenAt = _at;
}

void PilgrimageInstanceOpenWorld::clear_last_visited_haven()
{
	output(TXT("clear last visited haven"));
	lastVisitedHavenAt.clear();
}

void PilgrimageInstanceOpenWorld::update_last_visited_haven()
{
	todo_multiplayer_issue(TXT("we just get player here"));
	if (auto* pa = game->access_player().get_actor())
	{
		auto at = find_cell_at(pa);
		if (at.is_set())
		{
			set_last_visited_haven(at.get());
		}
	}
}

void PilgrimageInstanceOpenWorld::store_game_state(GameState* _gameState, StoreGameStateParams const & _params)
{
	base::store_game_state(_gameState, _params);

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("store game state"));
#endif

	_gameState->pilgrimageState.access<bool>(NAME(upgradeInStartingRoomUnlocked)) = upgradeInStartingRoomUnlocked;

	if (currentObjectiveIdx.is_set())
	{
		_gameState->pilgrimageState.access<int>(NAME(currentObjectiveIdx)) = currentObjectiveIdx.get();
	}

	_gameState->pilgrimageVariables.set_from(pilgrimageVariables);

	if (pilgrimage->open_world__get_definition()->has_no_cells())
	{
		if (!_params.restartInRoomTagged.is_empty())
		{
			output(TXT("store pilgrim in checkpoint room tagged \"%S\" (forced restart)"),
				_params.restartInRoomTagged.to_string().to_char());
			_gameState->pilgrimageState.access<TagCondition>(NAME(pilgrimInCheckpointRoomTagged)) = _params.restartInRoomTagged;
		}
	}
	else
	{
		if (pilgrimAt.is_set())
		{
			if (!_params.restartInRoomTagged.is_empty())
			{
				output(TXT("store last pilgrim at [%ix%i] in checkpoint room tagged \"%S\" (forced restart)"),
					pilgrimAt.get().x, pilgrimAt.get().y,
					_params.restartInRoomTagged.to_string().to_char());
				_gameState->pilgrimageState.access<VectorInt2>(NAME(pilgrimAt)) = pilgrimAt.get();
				_gameState->pilgrimageState.access<TagCondition>(NAME(pilgrimInCheckpointRoomTagged)) = _params.restartInRoomTagged;
			}
			else
			{
				if (auto* cell = get_existing_cell(pilgrimAt.get()))
				{
					if (!cell->checkpointRoomTagged.is_empty())
					{
						output(TXT("store last pilgrim at [%ix%i] in checkpoint room tagged \"%S\""),
							pilgrimAt.get().x, pilgrimAt.get().y,
							cell->checkpointRoomTagged.to_string().to_char());
						_gameState->pilgrimageState.access<VectorInt2>(NAME(pilgrimAt)) = pilgrimAt.get();
						_gameState->pilgrimageState.access<TagCondition>(NAME(pilgrimInCheckpointRoomTagged)) = cell->checkpointRoomTagged;
					}
				}
			}
		}
		if (pilgrimAt.is_set() &&
			pilgrimComingFrom.is_set())
		{
			output(TXT("store last pilgrim at [%ix%i] coming from [%ix%i] for game state"),
				pilgrimAt.get().x, pilgrimAt.get().y,
				pilgrimComingFrom.get().x, pilgrimComingFrom.get().y);
			_gameState->pilgrimageState.access<VectorInt2>(NAME(pilgrimAt)) = pilgrimAt.get();
			_gameState->pilgrimageState.access<VectorInt2>(NAME(pilgrimComingFrom)) = pilgrimComingFrom.get();
		}
		if (lastVisitedHavenAt.is_set())
		{
			output(TXT("store last visited haven [%ix%i] for game state"), lastVisitedHavenAt.get().x, lastVisitedHavenAt.get().y);
			_gameState->pilgrimageState.access<VectorInt2>(NAME(pilgrimInHavenAt)) = lastVisitedHavenAt.get();
		}
		_gameState->pilgrimageState.access<float>(NAME(timeSinceLastCellBlocked)) = timeSinceLastCellBlocked;
		_gameState->pilgrimageState.access<int>(NAME(blockedAwayCells)) = blockedAwayCells;
	}

	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("store pilgrim device states for all existing cells"));
		for_every(ec, existingCells)
		{
			store_pilgrim_device_state_variables(ec->at);
		}
	}

	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("store game state"));

		_gameState->openWorld.knownExitsCells = knownExitsCells;
		_gameState->openWorld.knownDevicesCells = knownDevicesCells;
		_gameState->openWorld.knownSpecialDevicesCells = knownSpecialDevicesCells;
		_gameState->openWorld.knownCells = knownCells;
		_gameState->openWorld.visitedCells = visitedCells;

		_gameState->openWorld.longDistanceDetection.copy_from(longDistanceDetection);

		_gameState->openWorld.unobfuscatedPilgrimageDevices.clear();
		for_every(opd, obfuscatedPilgrimageDevices)
		{
			if (opd->known)
			{
				_gameState->openWorld.unobfuscatedPilgrimageDevices.push_back(opd->pilgrimageDevice);
			}
		}

		_gameState->openWorld.pilgrimageDevicesUsageGroup.clear();
		{
			Concurrency::ScopedSpinLock lock(pilgrimageDevicesUsageByGroupLock, TXT("restore from game state"));
			for_every(pdubg, pilgrimageDevicesUsageByGroup)
			{
				for_count(int, i, pdubg->useCount)
				{
					_gameState->openWorld.pilgrimageDevicesUsageGroup.push_back(pdubg->groupId);
				}
			}
		}

#ifdef AN_ALLOW_EXTENSIVE_LOGS
		REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("cells we need %i"), cells.get_size());
#endif

		_gameState->openWorld.cells.make_space_for(cells.get_size());
		for_every(c, cells)
		{
			if (c->knownPilgrimageDevicesAdded &&
				!c->knownPilgrimageDevices.is_empty())
			{
				bool storeCell = false;
				_gameState->openWorld.cells.grow_size(1);
				auto& cDest = _gameState->openWorld.cells.get_last();
				cDest.at = c->at;
				// store only those that have something set
				for_every(kpd, c->knownPilgrimageDevices)
				{
					if (!kpd->stateVariables.is_empty() || kpd->depleted || kpd->visited)
					{
						storeCell = true;
						cDest.knownPilgrimageDevices.grow_size(1);
						auto& kpdDest = cDest.knownPilgrimageDevices.get_last();
						kpdDest.device = kpd->device;
						kpdDest.depleted = kpd->depleted;
						kpdDest.visited = kpd->visited;
						kpdDest.stateVariables = kpd->stateVariables;
					}
				}
				if (!storeCell)
				{
					// no need to store it
					_gameState->openWorld.cells.pop_back();
				}
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::will_continue(PilgrimageInstance const* _pi, Framework::Room* _transitionRoom, Framework::Door* _transitionRoomExitDoor)
{
	ASSERT_ASYNC;

	base::will_continue(_pi, _transitionRoom, _transitionRoomExitDoor);

	an_assert(!startingRoom.is_set());
	an_assert(!startingRoomExitDoor.is_set());

	startingRoom = _transitionRoom;
	startingRoomExitDoor = _transitionRoomExitDoor;

	on_starting_room_obtained();
}

void PilgrimageInstanceOpenWorld::create_and_start(GameState* _fromGameState, bool _asNextPilgrimageStart)
{
#ifdef AN_IMPORTANT_INFO
	output(TXT("create and start [open world] \"%S\" (%S)"), get_pilgrimage()->get_name().to_string().to_char(), is_current()? TXT("current") : TXT("not current / next"));
#endif
	// info will be output when rgi is setup

	if (is_current())
	{
		game->access_game_tags().remove_tag(NAME(linear));
		game->access_game_tags().set_tag(NAME(openWorld));

		game->set_auto_removing_unused_temporary_objects(10.0f);
	}

	isBusyAsync = true;

	base::create_and_start(_fromGameState, _asNextPilgrimageStart);

	{	// post rgi setup
		if (auto* owDef = pilgrimage->open_world__get_definition())
		{
			owDef->get_skip_content_set().process(OUT_ skipContentTags);
		}
	}

	// based on random seed, will be the same across pilgrimages
	{
		Random::Generator rg = randomSeed;
		rg.advance_seed(950834, 22591);
		noonYaw = rg.get_float(-180.0f, 180.0f);
		
		todo_hack(TXT("setting noon yaw for the very first game in the gameslot is hardcoded as it is the easiest way to perform this and it is a pretty custom thing"));
		if (auto* ps = PlayerSetup::access_current_if_exists())
		{
			if (ps->get_stats().freshStarts <= 1)
			{
				noonYaw = 78.9f;
			}
		}
	}

	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	RefCountObjectPtr<GameState> keepGameState(_fromGameState);
	game->add_async_world_job(TXT("create and start [begin]"), [this, keepThis, keepGameState, _asNextPilgrimageStart]()
		{
			async_create_and_start_begin(keepGameState.get(), _asNextPilgrimageStart);
		});
}

void PilgrimageInstanceOpenWorld::async_create_and_start_begin(GameState* _fromGameState, bool _asNextPilgrimageStart)
{
	ASSERT_ASYNC;

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	scoped_call_stack_info(TXT("PilgrimageInstanceOpenWorld::async_create_and_start_begin"));

#ifdef AN_IMPORTANT_INFO
	output(TXT("(async) create and start \"%S\" (%S)"), get_pilgrimage()->get_name().to_string().to_char(), is_current() ? TXT("current") : TXT("not current / next"));
#endif

	// we may get stuck for a bit longer because of this set to true
	// that's why we do it only when starting the game which is during loading screen
	// for time being this is overriden by game when loading a game state, to speed up the process
	// this is stored for this pilgrimage instance and will be refreshed when we're doing something
	{
		scoped_call_stack_info(TXT("force instant object creation"));
		// game states load long enough, we need something else here
		bool forceInstantObjectCreation = _fromGameState == nullptr && !_asNextPilgrimageStart;
		if (Framework::RoomType* roomType = pilgrimage->open_world__get_definition()->get_starting_room_type())
		{
			roomType->load_on_demand_if_required();
			if (roomType->get_custom_parameters().get_value<bool>(NAME(allowDelayedObjectCreationForStartingRoom), false))
			{
				forceInstantObjectCreation = false;
			}
		}
		if (pilgrimage->open_world__get_definition()->does_allow_delayed_object_creation_on_start())
		{
			forceInstantObjectCreation = false;
		}
		refresh_force_instant_object_creation(forceInstantObjectCreation);
	}

	startInRoom.clear();
	{
		Random::Generator rg = randomSeed;
		rg.advance_seed(36086, 9734);
		VectorInt2 sAt;
		sAt.x = rg.get(pilgrimage->open_world__get_definition()->get_start_at().x);
		sAt.y = rg.get(pilgrimage->open_world__get_definition()->get_start_at().y);
		startAt = sAt;
	}

	if (!_asNextPilgrimageStart) // this is called twice for next pilgrimage - run it just once
	{
#ifdef AN_IMPORTANT_INFO
		output(TXT("(async) create obfuscated pilgrimage device line models"));
#endif
		scoped_call_stack_info(TXT("create obfuscated pilgrimage device line models"));
		Library::get_current_as<Library>()->get_pilgrimage_devices().do_for_every([this](Framework::LibraryStored* _libraryStored)
			{
				if (auto* pd = fast_cast<PilgrimageDevice>(_libraryStored))
				{
					create_obfuscated_pilgrimage_device_line_model_for_map(pd);
				}
			});
		create_obfuscated_pilgrimage_device_directions();
		if (is_current() && _fromGameState)
		{
			for_every(xopd, obfuscatedPilgrimageDevices)
			{
				xopd->known = false;
				xopd->knownOnHold = false;
				for_every(uopd, _fromGameState->openWorld.unobfuscatedPilgrimageDevices)
				{
					if (xopd->pilgrimageDevice == *uopd)
					{
						xopd->known = true;
					}
				}
			}

			{
				Concurrency::ScopedSpinLock lock(pilgrimageDevicesUsageByGroupLock, TXT("check usage"));
				pilgrimageDevicesUsageByGroup.clear();
				for_every(g, _fromGameState->openWorld.pilgrimageDevicesUsageGroup)
				{
					bool found = false;
					for_every(pdubg, pilgrimageDevicesUsageByGroup)
					{
						if (pdubg->groupId == *g)
						{
							++pdubg->useCount;
							found = true;
							break;
						}
					}
					if (!found)
					{
						PilgrimageDevicesUsageByGroup pdubg;
						pdubg.groupId = *g;
						++pdubg.useCount;
						pilgrimageDevicesUsageByGroup.push_back(pdubg);
					}
				}
			}
		}
	}
	if (is_current())
	{
		scoped_call_stack_info(TXT("is current"));
		if (auto* gs = _fromGameState)
		{
			scoped_call_stack_info(TXT("load game state"));
			MEASURE_PERFORMANCE(createAndStart_loadGameState);

			pilgrimageVariables.set_from(gs->pilgrimageVariables);

			upgradeInStartingRoomUnlocked = gs->pilgrimageState.get_value<bool>(NAME(upgradeInStartingRoomUnlocked), false);
			currentObjectiveIdx.clear();
			hasMoreObjectives.clear();
			if (auto* v = gs->pilgrimageState.get_existing<int>(NAME(currentObjectiveIdx)))
			{
				currentObjectiveIdx = *v;
			}

			knownExitsCells = gs->openWorld.knownExitsCells;
			knownDevicesCells = gs->openWorld.knownDevicesCells;
			knownSpecialDevicesCells = gs->openWorld.knownSpecialDevicesCells;
			knownCells = gs->openWorld.knownCells;
			visitedCells = gs->openWorld.visitedCells;

			longDistanceDetection.copy_from(gs->openWorld.longDistanceDetection);

			cells.clear();
			for_every(c, gs->openWorld.cells)
			{
				if (!c->knownPilgrimageDevices.is_empty())
				{
					auto& cell = get_cell_at(c->at);
					for_every(kpd, c->knownPilgrimageDevices)
					{
						for_every(destKpd, cell.knownPilgrimageDevices)
						{
							if (destKpd->device == kpd->device)
							{
								auto* dest = cast_to_nonconst(destKpd);
								dest->device = kpd->device;
								dest->depleted = kpd->depleted;
								dest->visited = kpd->visited;
								dest->stateVariables = kpd->stateVariables;
							}
						}
					}
				}
			}
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
		{
			if (auto* toi = Framework::TestConfig::get_params().get_existing<int>(NAME(testObjectiveIdx)))
			{
				currentObjectiveIdx = *toi;
				output(TXT("[test] testObjectiveIdx is %i"), currentObjectiveIdx);
			}
			if (auto* toi = Framework::TestConfig::get_params().get_existing<Name>(NAME(testObjectiveId)))
			{
				output(TXT("[test] testObjectiveId: %S (find objective)"), toi->to_char());
				if (auto* owDef = pilgrimage->open_world__get_definition())
				{
					for_every(objective, owDef->get_objectives())
					{
						if (*toi == objective->id)
						{
							currentObjectiveIdx = for_everys_index(objective);
							output(TXT("[test] testObjectiveIdx is %i"), currentObjectiveIdx);
						}
					}
				}
			}
		}
#endif
	}
	// unveil map
	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		if (owDef->is_whole_map_known())
		{
			RangeInt2 size = owDef->get_size();
			if (size.x.is_empty() || size.y.is_empty())
			{
				make_sure_cells_are_ready(get_start_at(), 10);
			}
			else
			{
				for_range(int, y, size.y.min, size.y.max)
				{
					for_range(int, x, size.x.min, size.x.max)
					{
						get_cell_at(VectorInt2(x, y));
					}
				}
			}
		}
	}
	// handle centre-based that are known-from-start
	// they should be added/accessed already
	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		for_every(dpd, owDef->get_distanced_pilgrimage_devices())
		{
			if (!dpd->centreBased.use)
			{
				continue;
			}
			if (!GameDefinition::check_chosen(dpd->forGameDefinitionTagged))
			{
				continue;
			}

			bool anyKnownFromStart = false;
			for_every_ref(pd, dpd->devices)
			{
				if (pd->is_known_from_start())
				{
					anyKnownFromStart = true;
					break;
				}
			}

			if (anyKnownFromStart)
			{
				auto& tile = get_pilgrimage_devices_tile_at(for_everys_index(dpd), VectorInt2::zero); // cell-at is not important here

				for_every(pd, tile.knownPilgrimageDevices)
				{
					if (pd->device->is_known_from_start())
					{
						get_cell_at(pd->at);
					}
				}
			}
		}
	}
	MEASURE_PERFORMANCE(createAndStart);
	// make sure we have an objective (objectives are held inside instance, so we don't have to check whether it is current or not)
	{
		scoped_call_stack_info(TXT("next objective (actually just the first one, current)"));
		next_objective_internal(false);
	}
	Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();
	async_generate_base(_asNextPilgrimageStart);
	if (mainRegion.is_set())
	{
		mainRegion->access_tags().set_tag(NAME(openWorld));
		//
		{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("%c : auto map only [%S]"),
				!GameSettings::get().difficulty.autoMapOnly ? '+' : '-',
				GameSettings::get().difficulty.autoMapOnly ? TXT("yes") : TXT("no"));
			bool hasDoorDirections = pilgrimage->open_world__get_definition()->has_door_directions(GameSettings::get().difficulty.showDirectionsOnlyToObjective);
			output(TXT("%c : has door directions (%S)"),
				hasDoorDirections ? '+' : '-',
				GameSettings::get().difficulty.showDirectionsOnlyToObjective ? TXT("showDirectionsOnlyToObjective") : TXT("--"));
#endif
			bool newValue = !GameSettings::get().difficulty.autoMapOnly && pilgrimage->open_world__get_definition()->has_door_directions(GameSettings::get().difficulty.showDirectionsOnlyToObjective);
			mainRegion->access_variables().access<bool>(NAME(withDoorDirections)) = newValue;
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("with door directions = %S"), newValue ? TXT("true") : TXT("false"));
#endif
		}
	}
	if (is_current() && !_asNextPilgrimageStart) // create starting room only when current and not continuing pilgrimage, we're just starting and we need it here
	{
		scoped_call_stack_info(TXT("call async_generate_starting_room [current but not as a start of next pilgrimage, the very starting room, at least in this run]"));
		async_generate_starting_room(_fromGameState);
	}
	{
		scoped_call_stack_info(TXT("call game->request_ready_and_activate_all_inactive"));
		game->request_ready_and_activate_all_inactive(activationGroup.get());
	}
	game->pop_activation_group(activationGroup.get());

	if (auto* b = pilgrimage->open_world__get_definition()->get_block_away_from_objective())
	{
		blockedAwayCells = b->startBlockedAwayCells.get(0);
	}
	else
	{
		blockedAwayCells = 0;
	}

	if (is_current())
	{
		scoped_call_stack_info(TXT("is current - from game state?"));
		if (_fromGameState)
		{
#ifdef AN_IMPORTANT_INFO
			output(TXT("from game state"));
#endif
			if (pilgrimage->open_world__get_definition()->has_no_cells())
			{
#ifdef AN_IMPORTANT_INFO
				output(TXT("has no cells, don't create any cells"));
#endif
			}
			else
			{
				bool forceCreateAt = false; // if we don't force it, it will be created only if not at "startAt", and sometimes we want to have it created properly
				Array<VectorInt2> createAt;

				timeSinceLastCellBlocked = _fromGameState->pilgrimageState.get_value<float>(NAME(timeSinceLastCellBlocked), 0.0f);
				blockedAwayCells = _fromGameState->pilgrimageState.get_value<int>(NAME(blockedAwayCells), 0);

				if (auto* pilgrimInHavenAt = _fromGameState->pilgrimageState.get_existing<VectorInt2>(NAME(pilgrimInHavenAt)))
				{
#ifdef AN_IMPORTANT_INFO
					output(TXT("pilgrimInHavenAt %iX%i"), pilgrimInHavenAt->x, pilgrimInHavenAt->y);
#endif
					createAt.push_back(*pilgrimInHavenAt);
				}
				else
				{
					Optional<VectorInt2> pilgrimAt;
					if (auto* pat = _fromGameState->pilgrimageState.get_existing<VectorInt2>(NAME(pilgrimAt)))
					{
						pilgrimAt = *pat;
					}
					for_every(rirt, pilgrimage->open_world__get_definition()->get_restart_in_room_tagged())
					{
						if (rirt->onStoryFacts.check(_fromGameState->storyFacts))
						{
							if (rirt->cellAt.is_set())
							{
								forceCreateAt = true;
								pilgrimAt = rirt->cellAt;
							}
							break;
						}
					}
					if (pilgrimAt.is_set())
					{
						VectorInt2 pAt = pilgrimAt.get();
						if (auto* pilgrimComingFrom = _fromGameState->pilgrimageState.get_existing<VectorInt2>(NAME(pilgrimComingFrom)))
						{
							VectorInt2 pComingFrom = *pilgrimComingFrom;
							VectorInt2 fromDir = be_dir(pComingFrom - pAt);
							pAt = validate_coordinates(pAt);
							pComingFrom = pAt + fromDir;

#ifdef AN_IMPORTANT_INFO
							output(TXT("pAt %iX%i"), pAt.x, pAt.y);
							output(TXT("pComingFrom %iX%i"), pComingFrom.x, pComingFrom.y);
#endif
							createAt.push_back(pAt);
							createAt.push_back(pComingFrom);
							forceCreateAt = true;
						}
						else
						{
#ifdef AN_IMPORTANT_INFO
							output(TXT("pAt (only) %iX%i"), pAt.x, pAt.y);
#endif
							createAt.push_back(validate_coordinates(pAt));
						}
					}
				}

#ifdef AN_IMPORTANT_INFO
				output(TXT("createAt.size = %i"), createAt.get_size());
				for_every(c, createAt)
				{
					output(TXT("  %ix%i"), c->x, c->y);
				}
#endif
				for_every(c, createAt)
				{
					if (forceCreateAt ||
						(*c != get_start_at()))
					{
						auto* ec = get_existing_cell(*c);
						if (! ec || ! ec->generated)
						{
							// issue cell creation, we might need next pilgrimage which is still on the way to be created
#ifdef AN_IMPORTANT_INFO
							output(TXT("issue cell creation (from game state) %iX%i"), c->x, c->y);
#endif
							add_async_create_cell(*c);
							/*
#ifdef AN_IMPORTANT_INFO
							output(TXT("create cell for (from game state) %iX%i"), c->x, c->y);
#endif
							scoped_call_stack_info(TXT("call async_create_cell_at"));
							async_create_cell_at(*c);
							*/
						}
					}
				}
			}
			// here we just created the cells, we'll find the startInRoom later
		}
	}

	// will try to finalise, unless we're not the only async job
	async_create_and_start_end(_fromGameState, _asNextPilgrimageStart);
}

void PilgrimageInstanceOpenWorld::async_create_and_start_end(GameState* _fromGameState, bool _asNextPilgrimageStart)
{
	scoped_call_stack_info(TXT("PilgrimageInstanceOpenWorld::async_create_and_start_end"));

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	ASSERT_ASYNC;

	// process it only if it is the last one 
	if (!game->is_performing_async_job_and_last_world_job_on_this_thread(s_pilgrimageInstance_deferredEndJobs.get()))
	{
		// defer
		RefCountObjectPtr<PilgrimageInstance> keepThis(this);
		RefCountObjectPtr<GameState> keepGameState(_fromGameState);
		s_pilgrimageInstance_deferredEndJobs.increase();
		game->add_async_world_job(TXT("create and start [end]"), [this, keepThis, keepGameState, _asNextPilgrimageStart]()
			{
				s_pilgrimageInstance_deferredEndJobs.decrease();
				async_create_and_start_end(keepGameState.get(), _asNextPilgrimageStart);
			});
	}
	else
	{
		if (is_current())
		{
			async_find_place_to_start(_fromGameState, _asNextPilgrimageStart);
		}
		refresh_force_instant_object_creation(false); // clear
		isBusyAsync = false;
	}
}

void PilgrimageInstanceOpenWorld::async_add_extra_cell_exits_if_required()
{
	if (!extraCellExitsDone)
	{
		async_enforce_all_cells_reachable_if_required();
		async_attemp_cells_have_enough_exits();
		extraCellExitsDone = true;
	}
}

void PilgrimageInstanceOpenWorld::async_enforce_all_cells_reachable_if_required()
{
	auto* owDef = pilgrimage->open_world__get_definition();

#ifdef OUTPUT_GENERATION_EXTRA_CELL_EXITS
	output(TXT("[extra cell exits] (enforce reachable)"));
#endif

	if (owDef->has_no_cells() || !owDef->should_enforce_all_cells_reachable())
	{
		return;
	}

	RangeInt2 size = owDef->get_size();
	VectorInt2 startAt = get_start_at();

	bool allReachable = false;

	struct ChangeableExit
	{
		VectorInt2 at;
		DirFourClockwise::Type dir;
		ChangeableExit() {}
		ChangeableExit(VectorInt2 const& _at, DirFourClockwise::Type _dir) : at(_at), dir(_dir) {}
	};
	Array<ChangeableExit> changeableExits;

	Random::Generator rg = randomSeed;
	rg.advance_seed(9458989, 234978);

#ifdef OUTPUT_GENERATION_EXTRA_CELL_EXITS
	output(TXT("[extra cell exits] rg %S"), rg.get_seed_string().to_char());
#endif

	while (!allReachable)
	{
		for_range(int, y, size.y.min, size.y.max)
		{
			for_range(int, x, size.x.min, size.x.max)
			{
				auto& c = access_cell_at(VectorInt2(x, y));
				c.reachable = false;
			}
		}

		auto& c = access_cell_at(startAt);
		c.reachable = true;

		// do in many passes, propagate reachability
		bool workToDo = true;
		while (workToDo)
		{
			workToDo = false;
			for_range(int, y, size.y.min, size.y.max)
			{
				for_range(int, x, size.x.min, size.x.max)
				{
					auto& c = access_cell_at(VectorInt2(x, y));
					if (c.reachable)
					{
						for_count(int, d, DirFourClockwise::NUM)
						{
							if (c.exits[d])
							{
								auto& cd = access_cell_at(VectorInt2(x, y) + DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)d));
								if (!cd.reachable)
								{
									cd.reachable = true;
									workToDo = true;
								}
							}
						}
					}
				}
			}
		}

		allReachable = true;

		for_range(int, y, size.y.min, size.y.max)
		{
			for_range(int, x, size.x.min, size.x.max)
			{
				auto& c = access_cell_at(VectorInt2(x, y));
				if (! c.reachable)
				{
					allReachable = false;
					break;
				}
			}
			if (!allReachable)
			{
				break;
			}
		}

		if (!allReachable)
		{
			changeableExits.clear();

			for_range(int, y, size.y.min, size.y.max)
			{
				for_range(int, x, size.x.min, size.x.max)
				{
					auto& c = access_cell_at(VectorInt2(x, y));
					if (c.reachable)
					{
						bool allowedExits[4];
						bool requiredExits[4];
						bool allPossibleExitsRequired;
						c.fill_allowed_required_exits(allowedExits, requiredExits, allPossibleExitsRequired);

						for_count(int, dIdx, DirFourClockwise::NUM)
						{
							DirFourClockwise::Type d = (DirFourClockwise::Type)dIdx;
							if (!c.exits[d] && allowedExits[d])
							{
								VectorInt2 to = VectorInt2(x, y) + DirFourClockwise::dir_to_vector_int2(d);
								if (size.does_contain(to))
								{
									auto& cd = access_cell_at(to);
									if (!cd.reachable)
									{
										bool cd_allowedExits[4];
										bool cd_requiredExits[4];
										bool cd_allPossibleExitsRequired;
										cd.fill_allowed_required_exits(cd_allowedExits, cd_requiredExits, cd_allPossibleExitsRequired);
										if (cd_allowedExits[DirFourClockwise::opposite_dir(d)])
										{
											changeableExits.push_back(ChangeableExit(VectorInt2(x, y), d));
										}
									}
								}
							}
						}
					}
				}
			}

			if (!changeableExits.is_empty())
			{
				int idx = rg.get_int(changeableExits.get_size());
				auto& ce = changeableExits[idx];
				auto& c = access_cell_at(ce.at);
				auto to = ce.at + DirFourClockwise::dir_to_vector_int2(ce.dir);
				auto& cTo = access_cell_at(to);
#ifdef OUTPUT_GENERATION_EXTRA_CELL_EXITS
				output(TXT("[extra cell exits] (enforce reachable) add %ix%i -> %ix%i"), ce.at.x, ce.at.y, to.x, to.y);
#endif
				c.exits[ce.dir] = true;
				cTo.exits[DirFourClockwise::opposite_dir(ce.dir)] = true;
			}
			else
			{
				error(TXT("no connections that could be changed to make it reachable"));
				break;
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::async_attemp_cells_have_enough_exits()
{
	auto* owDef = pilgrimage->open_world__get_definition();

#ifdef OUTPUT_GENERATION_EXTRA_CELL_EXITS
	output(TXT("[extra cell exits] (make enough)"));
#endif

	if (owDef->has_no_cells() || owDef->get_attempt_to_cells_have().exitsCount == 0)
	{
		return;
	}

	int preferredExitCount = owDef->get_attempt_to_cells_have().exitsCount;
	float attemptChance = owDef->get_attempt_to_cells_have().exitsCountAttemptChance;

	RangeInt2 size = owDef->get_size();

	Random::Generator rg = randomSeed;
	rg.advance_seed(83457, 29235);

#ifdef OUTPUT_GENERATION_EXTRA_CELL_EXITS
	output(TXT("[extra cell exits] rg %S"), rg.get_seed_string().to_char());
#endif

	struct PossibleExit
	{
		VectorInt2 at;
		DirFourClockwise::Type dir;
		PossibleExit() {}
		PossibleExit(VectorInt2 const& _at, DirFourClockwise::Type _dir) : at(_at), dir(_dir) {}
	};

	bool allOk = false;
	while (! allOk)
	{
		bool changedSomething = false;

		for_range(int, y, size.y.min, size.y.max)
		{
			for_range(int, x, size.x.min, size.x.max)
			{
				auto& c = access_cell_at(VectorInt2(x, y));
				int exitCount = 0;
				for_count(int, i, DirFourClockwise::NUM)
				{
					if (c.exits[i]) ++exitCount;
				}
				if (exitCount < preferredExitCount && rg.get_chance(attemptChance))
				{
					bool allowedExits[4];
					bool requiredExits[4];
					bool allPossibleExitsRequired;
					c.fill_allowed_required_exits(allowedExits, requiredExits, allPossibleExitsRequired);

					ArrayStatic<PossibleExit, DirFourClockwise::NUM> possibleExits;
					for_count(int, dIdx, DirFourClockwise::NUM)
					{
						DirFourClockwise::Type d = (DirFourClockwise::Type)dIdx;
						if (!c.exits[d] && allowedExits[d])
						{
							VectorInt2 to = VectorInt2(x, y) + DirFourClockwise::dir_to_vector_int2(d);
							if (size.does_contain(to))
							{
								auto& cd = access_cell_at(to);
								{
									bool cd_allowedExits[4];
									bool cd_requiredExits[4];
									bool cd_allPossibleExitsRequired;
									cd.fill_allowed_required_exits(cd_allowedExits, cd_requiredExits, cd_allPossibleExitsRequired);
									if (cd_allowedExits[DirFourClockwise::opposite_dir(d)])
									{
										possibleExits.push_back(PossibleExit(VectorInt2(x, y), d));
									}
								}
							}
						}
					}

					if (!possibleExits.is_empty())
					{
						int idx = rg.get_int(possibleExits.get_size());
						auto& ce = possibleExits[idx];
						auto& c = access_cell_at(ce.at);
						auto to = ce.at + DirFourClockwise::dir_to_vector_int2(ce.dir);
						auto& cTo = access_cell_at(to);
						c.exits[ce.dir] = true;
						cTo.exits[DirFourClockwise::opposite_dir(ce.dir)] = true;

#ifdef OUTPUT_GENERATION_EXTRA_CELL_EXITS
						output(TXT("[extra cell exits] (make enough) add %ix%i -> %ix%i"), ce.at.x, ce.at.y, to.x, to.y);
#endif

						changedSomething = true;
					}
				}
			}
		}

		allOk = !changedSomething;
	}
}

void PilgrimageInstanceOpenWorld::async_find_place_to_start(GameState* _fromGameState, bool _asNextPilgrimageStart)
{
	ASSERT_ASYNC;

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	refresh_force_instant_object_creation();

	Optional<VectorInt2> beAt;
	Optional<Transform> beAtPlacementWS;

	TagCondition const* _pilgrimInCheckpointRoomTagged = nullptr;
	Optional<VectorInt2> _pilgrimAt;
	if (_fromGameState)
	{
		_fromGameState->pilgrimageState.get_existing<TagCondition>(NAME(pilgrimInCheckpointRoomTagged));
		auto const * pat = _fromGameState->pilgrimageState.get_existing<VectorInt2>(NAME(pilgrimAt));
		if (pat)
		{
			_pilgrimAt = *pat;
		}
		for_every(rirt, pilgrimage->open_world__get_definition()->get_restart_in_room_tagged())
		{
			if (rirt->onStoryFacts.check(_fromGameState->storyFacts))
			{
				_pilgrimInCheckpointRoomTagged = &rirt->roomTagged;
				if (rirt->cellAt.is_set())
				{
					_pilgrimAt = rirt->cellAt;
				}
				break;
			}
		}
	}

	if (_fromGameState &&
		pilgrimage->open_world__get_definition()->has_no_cells())
	{
		startInRoom = nullptr;

		// check if we can find checkpoint room tagged
#ifdef AN_IMPORTANT_INFO
		if (_pilgrimInCheckpointRoomTagged)
		{
			output(TXT("will start in checkpoint room tagged (\"%S\")"), _pilgrimInCheckpointRoomTagged->to_string().to_char());
		}
		else
		{
			output(TXT("will start in checkpoint without info about room tagged"));
		}
#endif
		{
			Framework::Room* result = nullptr;
			get_main_region()->for_every_room([&result, _pilgrimInCheckpointRoomTagged](Framework::Room* _room)
				{
					if (!result && !_room->get_all_doors().is_empty() &&
						!_room->get_tags().get_tag(NAME(noSafeSpawn)) &&
						_pilgrimInCheckpointRoomTagged &&
						_pilgrimInCheckpointRoomTagged->check(_room->get_tags()))
					{
						result = _room;
					}
				});
			if (result)
			{
#ifdef AN_IMPORTANT_INFO
				output(TXT("found a room to restart"));
#endif
				startInRoom = result;
				beAt = VectorInt2::zero; // anything
				beAtPlacementWS.clear(); // will look for anything useful (maybe even pilgrim spawn point?)
			}
		}
	}
	if (_fromGameState &&
		!pilgrimage->open_world__get_definition()->has_no_cells() /* this should be used only if there are cells */)
	{
		// before we call async_create_and_start_end, we issued creation of cells, so they have to exist by now
		startInRoom = nullptr;
		auto* _pilgrimComingFrom = _fromGameState->pilgrimageState.get_existing<VectorInt2>(NAME(pilgrimComingFrom));
		// haven has the greater priority
		if (auto* pilgrimInHavenAt = _fromGameState->pilgrimageState.get_existing<VectorInt2>(NAME(pilgrimInHavenAt)))
		{
			VectorInt2 tryAt = *pilgrimInHavenAt;

			set_last_visited_haven(tryAt);

			tryAt = validate_coordinates(tryAt);

			if (!startInRoom.get())
			{
#ifdef AN_IMPORTANT_INFO
				output(TXT("will start at last haven %ix%i"), tryAt.x, tryAt.y);
#endif
				if (auto* ec = get_existing_cell(tryAt))
				{
					if (ec->generated)
					{
						if (auto* r = ec->region.get())
						{
							TagCondition tc;
							todo_note(TXT("change hardcoded string to definable for whole game or just pilgrimage?"));
							tc.load_from_string(hardcoded String(TXT("haven")));
							if (auto* room = r->get_any_room(false, &tc))
							{
								startInRoom = room;
								beAt = tryAt;
							}
						}
					}
				}
				else
				{
					warn(TXT("cell at %ix%i was not created by now (haven)"), tryAt.x, tryAt.y);
				}
			}
		}
		if (_pilgrimAt.is_set() && _pilgrimComingFrom)
		{
			pilgrimAt = _pilgrimAt.get();
			pilgrimComingFrom = *_pilgrimComingFrom;

			{
				VectorInt2 fromDir = be_dir(pilgrimComingFrom.get() - pilgrimAt.get());
				pilgrimAt = validate_coordinates(pilgrimAt.get());
				pilgrimComingFrom = pilgrimAt.get() + fromDir;
			}

			if (!startInRoom.get())
			{
#ifdef AN_IMPORTANT_INFO
				output(TXT("will start at last cell %ix%i"), pilgrimAt.get().x, pilgrimAt.get().y);
#endif
				if (!get_existing_cell(pilgrimAt.get()))
				{
					warn(TXT("cell at %ix%i was not created by now (pilgrim at)"), pilgrimAt.get().x, pilgrimAt.get().y);
				}
				if (auto* dir = find_door_entering_cell(pilgrimAt.get(), pilgrimComingFrom.get()))
				{
					startInRoom = dir->get_in_room();
					beAt = pilgrimAt.get();
					// we should be placed inbound a half tile distance into the room (let's hope we don't have an elevator)
					beAtPlacementWS = dir->get_inbound_matrix().to_transform().to_world(Transform(Vector3::yAxis * 0.001f, Quat::identity));
				}
				else
				{
					// any room
					if (auto* ec = get_existing_cell(pilgrimAt.get()))
					{
						Framework::Room* result = nullptr;
						ec->region->for_every_room([&result](Framework::Room* _room)
							{
								if (!result && !_room->get_all_doors().is_empty() &&
									!_room->get_tags().get_tag(NAME(noSafeSpawn)))
								{
									result = _room;
								}
							});
						if (result)
						{
							if (auto* dir = result->get_all_doors().get_first())
							{
								startInRoom = result;
								beAt = pilgrimAt.get();
								beAtPlacementWS = dir->get_inbound_matrix().to_transform().to_world(Transform(Vector3::yAxis * 0.001f, Quat::identity));
							}
						}
					}
				}
			}
		}
		else if (_pilgrimAt.is_set())
		{
			pilgrimAt = _pilgrimAt.get();
			pilgrimComingFrom.clear();

			pilgrimAt = validate_coordinates(pilgrimAt.get());

			if (!startInRoom.get())
			{
#ifdef AN_IMPORTANT_INFO
				output(TXT("will start at last cell %ix%i"), pilgrimAt.get().x, pilgrimAt.get().y);
#endif
				if (!get_existing_cell(pilgrimAt.get()))
				{
					warn(TXT("cell at %ix%i was not created by now (pilgrim at)"), pilgrimAt.get().x, pilgrimAt.get().y);
				}
				{
					if (auto* ec = get_existing_cell(pilgrimAt.get()))
					{
						Framework::Room* result = nullptr;
						ec->region->for_every_room([&result](Framework::Room* _room)
							{
								if (!result && !_room->get_all_doors().is_empty() &&
									!_room->get_tags().get_tag(NAME(noSafeSpawn)))
								{
									result = _room;
								}
							});
						if (result)
						{
							if (auto* dir = result->get_all_doors().get_first())
							{
#ifdef AN_IMPORTANT_INFO
								output(TXT("found a room and door to restart"));
#endif
								startInRoom = result;
								beAt = pilgrimAt.get();
								beAtPlacementWS = dir->get_inbound_matrix().to_transform().to_world(Transform(Vector3::yAxis * 0.001f, Quat::identity));
							}
						}
					}
				}
			}
		}
		if (_pilgrimAt.is_set() && _pilgrimInCheckpointRoomTagged &&
			pilgrimAt.is_set() &&
			_pilgrimAt.get() == pilgrimAt.get())
		{
			// check if we can find checkpoint room tagged
#ifdef AN_IMPORTANT_INFO
			output(TXT("will start at last cell %ix%i in checkpoint room tagged (\"%S\")"), pilgrimAt.get().x, pilgrimAt.get().y, _pilgrimInCheckpointRoomTagged->to_string().to_char());
#endif
			if (!get_existing_cell(pilgrimAt.get()))
			{
				warn(TXT("cell at %ix%i was not created by now (pilgrim at)"), pilgrimAt.get().x, pilgrimAt.get().y);
			}
			{
				if (auto* ec = get_existing_cell(pilgrimAt.get()))
				{
					Framework::Room* result = nullptr;
					ec->region->for_every_room([&result, _pilgrimInCheckpointRoomTagged](Framework::Room* _room)
						{
							if (!result && !_room->get_all_doors().is_empty() &&
								!_room->get_tags().get_tag(NAME(noSafeSpawn)) &&
								_pilgrimInCheckpointRoomTagged->check(_room->get_tags()))
							{
								result = _room;
							}
						});
					if (result)
					{
#ifdef AN_IMPORTANT_INFO
						output(TXT("found a room to restart"));
#endif
						startInRoom = result;
						beAt = pilgrimAt.get();
						beAtPlacementWS.clear(); // will look for anything useful (maybe even pilgrim spawn point?)
					}
				}
			}
		}
	}

	if (!beAt.is_set())
	{
		if (startInRoom.is_set() && startInRoom != startingRoom)
		{
			error_dev_ignore(TXT("we don't know where to start and startInRoom is not startingRoom"));
			startInRoom.clear();
		}
		else if (startInRoom == startingRoom)
		{
			beAt = get_start_at();
#ifdef AN_IMPORTANT_INFO
			output(TXT("will start at pilgrimage start %ix%i (valid starting room)"), beAt.get().x, beAt.get().y);
#endif
		}
	}

	// get where we should be in the room
	{
		bool found = false;
		if (startInRoom.get())
		{
			if (beAtPlacementWS.is_set())
			{
				// we want vr placement
				found = true;
			}
			else
			{
				startInRoom->for_every_point_of_interest(NAME(pilgrimSpawnPoint), [&found, &beAtPlacementWS](Framework::PointOfInterestInstance* _poi)
					{
						// we want vr placement
						beAtPlacementWS = _poi->calculate_placement();
						found = true;
					});
			}
		}
		if (!found)
		{
			warn(TXT("could not find point of interest to tell where we should be (to start) let's start with usual startingRoom"));
			startInRoom = startingRoom;
			if (startInRoom.is_set())
			{
				startInRoom->for_every_point_of_interest(NAME(pilgrimSpawnPoint), [&found, &beAtPlacementWS](Framework::PointOfInterestInstance* _poi)
					{
						// we want vr placement
						beAtPlacementWS = _poi->calculate_placement();
						found = true;
					});
				if (!found)
				{
					beAtPlacementWS = startInRoom->get_vr_anchor();
					found = true;
				}
			}
		}
		if (!found)
		{
			warn(TXT("could not find point of interest even in the startingRoom"));

			if (pilgrimAt.is_set())
			{
				pilgrimAt = validate_coordinates(pilgrimAt.get());
#ifdef AN_IMPORTANT_INFO
				output(TXT("will start at last cell %ix%i"), pilgrimAt.get().x, pilgrimAt.get().y);
#endif
				if (!get_existing_cell(pilgrimAt.get()))
				{
					warn(TXT("cell at %ix%i was not created by now (pilgrim at)"), pilgrimAt.get().x, pilgrimAt.get().y);
				}
				{
					if (auto* ec = get_existing_cell(pilgrimAt.get()))
					{
						Framework::Room* result = nullptr;
						ec->region->for_every_room([&result](Framework::Room* _room)
							{
								if (!result && !_room->get_all_doors().is_empty() &&
									!_room->get_tags().get_tag(NAME(noSafeSpawn)))
								{
									result = _room;
								}
							});
						if (result)
						{
							if (auto* dir = result->get_all_doors().get_first())
							{
#ifdef AN_IMPORTANT_INFO
								output(TXT("found a room and door to restart"));
#endif
								startInRoom = result;
								beAt = pilgrimAt.get();
								beAtPlacementWS = dir->get_inbound_matrix().to_transform().to_world(Transform(Vector3::yAxis * 0.001f, Quat::identity));
							}
						}
					}
				}
			}
		}
	}

	{
#ifndef BUILD_PUBLIC_RELEASE
		testOpenWorldCell.clear();
		testStartInRoomTag.clear();
		testCellRoomIdx.clear();
		testCellRoomLocation.clear();
		testCellRoomRotation.clear();
		testCellRoomVRLocation.clear();
		testCellRoomVRRotation.clear();
#ifdef TEST_CELL
		testOpenWorldCell = VectorInt2(TEST_CELL);
#endif
		{
			bool tryTestOpenWorldCell = true;
			if (auto* tp = Framework::TestConfig::get_params().get_existing<Framework::LibraryName>(NAME(testPilgrimage)))
			{
				if (*tp != get_pilgrimage()->get_name())
				{
					tryTestOpenWorldCell = false;
				}
			}
			if (tryTestOpenWorldCell)
			{
				if (auto* towc = Framework::TestConfig::get_params().get_existing<VectorInt2>(NAME(testOpenWorldCell)))
				{
					testOpenWorldCell = *towc;
				}
				if (auto* tsir = Framework::TestConfig::get_params().get_existing<Name>(NAME(testStartInRoomTag)))
				{
					testStartInRoomTag = *tsir;
				}
				if (auto* tcri = Framework::TestConfig::get_params().get_existing<int>(NAME(testCellRoomIdx)))
				{
					testCellRoomIdx = *tcri;
				}
				if (auto* tcrl = Framework::TestConfig::get_params().get_existing<Vector3>(NAME(testCellRoomLocation)))
				{
					testCellRoomLocation = *tcrl;
				}
				if (auto* tcrr = Framework::TestConfig::get_params().get_existing<Rotator3>(NAME(testCellRoomRotation)))
				{
					testCellRoomRotation = *tcrr;
				}
				if (auto* tcrl = Framework::TestConfig::get_params().get_existing<Vector3>(NAME(testCellRoomVRLocation)))
				{
					testCellRoomVRLocation = *tcrl;
				}
				if (auto* tcrr = Framework::TestConfig::get_params().get_existing<Rotator3>(NAME(testCellRoomVRRotation)))
				{
					testCellRoomVRRotation = *tcrr;
				}
			}
		}
		if (testOpenWorldCell.is_set() &&
			!pilgrimage->open_world__get_definition()->has_no_cells())
		{
			testOpenWorldCell = validate_coordinates(testOpenWorldCell.get());
			beAt = testOpenWorldCell;
			// we need a different one, not from the starting room
			// will choose starting room later
			startInRoom.clear();
			beAtPlacementWS.clear();
		}
#endif
	}

	// make sure the cell at starting room exists if we start in starting room
	if (startInRoom.is_set() &&
		startInRoom == startingRoom)
	{
		auto startAt = get_start_at();
		if (!get_existing_cell(startAt))
		{
			issue_create_cell(startAt, true);
		}
	}

	if (beAt.is_set() && !pilgrimage->open_world__get_definition()->has_no_cells())
	{
#ifdef AN_IMPORTANT_INFO
		output(TXT("generate cells around %ix%i"), beAt.get().x, beAt.get().y);
#endif
		// get enough cells around the actual start
		int dist = 10; // should be enough?
		{
			for_range(int, y, -dist, dist)
			{
				for_range(int, x, -dist, dist)
				{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					::System::TimeStamp cellTS;
#endif
					get_cell_at(beAt.get() + VectorInt2(x, y));
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION_DETAILED
					output(TXT(" + cell %ix%i (%.3fs)"), beAt.get().x + x, beAt.get().y + y, cellTS.get_time_since());
#endif
					if (game->does_want_to_cancel_creation())
					{
						break;
					}
				}
				if (game->does_want_to_cancel_creation())
				{
					break;
				}
			}
		}
	}

	{
		Optional<VectorInt2> startAt = beAt;
		if (startAt.is_set())
		{
			if (!get_existing_cell(startAt.get()))
			{
				issue_create_cell(startAt.get(), true);
				mark_cell_visited(startAt.get());
			}
			{	// make sure the cells are ready, so we can show the map
				int radius = hardcoded 10;
				async_make_sure_cells_are_ready(startAt.get(), radius);
				issue_make_sure_known_cells_are_ready();
			}
		}
	}

	if (!beAtPlacementWS.is_set())
	{
		if (!beAtPlacementWS.is_set())
		{
			if (startInRoom.is_set())
			{
				startInRoom->for_every_point_of_interest(NAME(pilgrimSpawnPoint), [&beAtPlacementWS](Framework::PointOfInterestInstance* _fpoi)
					{
#ifdef AN_IMPORTANT_INFO
						output(TXT("be at pilgrimSpawnPoint"));
#endif
						beAtPlacementWS = _fpoi->calculate_placement();
					});
			}
		}
		if (!beAtPlacementWS.is_set())
		{
			if (startInRoom.is_set())
			{
				startInRoom->for_every_point_of_interest(NAME(vrAnchor), [&beAtPlacementWS](Framework::PointOfInterestInstance* _fpoi)
					{
#ifdef AN_IMPORTANT_INFO
						output(TXT("be at vr anchor"));
#endif
						beAtPlacementWS = _fpoi->calculate_placement();
					});
			}
		}
	}

#ifndef BUILD_PUBLIC_RELEASE
	{
		if (testOpenWorldCell.is_set())
		{
			if (Game* game = Game::get_as<Game>())
			{
				VectorInt2 atCell(testOpenWorldCell.get(VectorInt2::zero));
				if (auto* ec = get_existing_cell(atCell))
				{
					if (ec->generated)
					{
						if (auto* r = ec->region.get())
						{
							if (testCellRoomIdx.is_set())
							{
								r->for_every_room([this, &beAtPlacementWS](Framework::Room* room)
									{
										if (room->get_tags().has_tag(NAME(cellRoomIdx)))
										{
											int cellRoomIdx = room->get_tags().get_tag_as_int(NAME(cellRoomIdx));
											if (testCellRoomIdx.get() == cellRoomIdx)
											{
												Transform placement = Transform::identity;
												if (!room->get_doors().is_empty())
												{
													placement = room->get_doors().get_first()->get_inbound_matrix().to_transform();
													placement.set_translation(placement.get_translation() + placement.get_axis(Axis::Forward) * 0.25f);
												}
												if (testCellRoomLocation.is_set())
												{
													placement.set_translation(testCellRoomLocation.get());
												}
												if (testCellRoomRotation.is_set())
												{
													placement.set_orientation(testCellRoomRotation.get().to_quat());
												}
												startInRoom = room;
												beAtPlacementWS = placement;
											}
										}
									});
							}
							if (testStartInRoomTag.is_set())
							{
								if (auto* room = r->find_room_tagged(testStartInRoomTag.get()))
								{
									startInRoom.clear();
									beAtPlacementWS.clear();
									if (!startInRoom.get())
									{
										room->for_every_point_of_interest(NAME(pilgrimSpawnPoint), [room, this, &beAtPlacementWS](Framework::PointOfInterestInstance* _fpoi)
											{
												startInRoom = room;
												beAtPlacementWS = _fpoi->calculate_placement();
											});
									}
									if (!startInRoom.get())
									{
										for_every_ptr(door, room->get_doors())
										{
											if (door->is_visible() &&
												door->can_move_through() &&
												!door->may_ignore_vr_placement())
											{
												Transform placement = Transform::identity;
												placement = door->get_inbound_matrix().to_transform();
												placement.set_translation(placement.get_translation() + placement.get_axis(Axis::Forward) * 0.25f);
												startInRoom = room;
												beAtPlacementWS = placement;
												break;
											}
										}
									}
								}
							}
							if (!startInRoom.get())
							{
								if (auto* room = r->get_any_room(false, &okRoomToSpawnTagCondition))
								{
									if (!startInRoom.get())
									{
										room->for_every_point_of_interest(NAME(pilgrimSpawnPoint), [room, this, &beAtPlacementWS](Framework::PointOfInterestInstance* _fpoi)
											{
												startInRoom = room;
												beAtPlacementWS = _fpoi->calculate_placement();
											});
									}
									if (!startInRoom.get())
									{
										for_every_ptr(door, room->get_doors())
										{
											if (door->is_visible() &&
												door->can_move_through() &&
												!door->may_ignore_vr_placement())
											{
												Transform placement = Transform::identity;
												placement = door->get_inbound_matrix().to_transform();
												placement.set_translation(placement.get_translation() + placement.get_axis(Axis::Forward) * 0.25f);
												startInRoom = room;
												beAtPlacementWS = placement;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{
			if (testStartInRoomTag.is_set())
			{
				if (get_main_region())
				{
					if (auto* room = get_main_region()->find_room_tagged(testStartInRoomTag.get()))
					{
						startInRoom.clear();
						beAtPlacementWS.clear();
						if (!startInRoom.get())
						{
							room->for_every_point_of_interest(NAME(pilgrimSpawnPoint), [room, this, &beAtPlacementWS](Framework::PointOfInterestInstance* _fpoi)
								{
									startInRoom = room;
									beAtPlacementWS = _fpoi->calculate_placement();
								});
						}
						if (!startInRoom.get())
						{
							for_every_ptr(door, room->get_doors())
							{
								if (door->is_visible() &&
									door->can_move_through() &&
									!door->may_ignore_vr_placement())
								{
									Transform placement = Transform::identity;
									placement = door->get_inbound_matrix().to_transform();
									placement.set_translation(placement.get_translation() + placement.get_axis(Axis::Forward) * 0.25f);
									startInRoom = room;
									beAtPlacementWS = placement;
									break;
								}
							}
						}
					}
				}
				else
				{
					error(TXT("no main region"));
				}
			}
		}
	}
#endif

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	if (beAtPlacementWS.is_set() && ! _asNextPilgrimageStart)
	{
		if (auto* r = startInRoom.get())
		{
			Transform vrAnchor = r->get_vr_anchor(beAtPlacementWS.get());
			Transform beAtPlacementVRS = vrAnchor.to_local(beAtPlacementWS.get());
			Loader::HubScenes::BeAtRightPlace::be_at(beAtPlacementVRS, true);
		}
	}

#ifdef AN_IMPORTANT_INFO
	if (beAt.is_set())
	{
		output(TXT("beAt = %S"), beAt.get().to_string().to_char());
	}
	if (beAtPlacementWS.is_set())
	{
		output(TXT("beAtPlacementWS.translation = %S"), beAtPlacementWS.get().get_translation().to_string().to_char());
		output(TXT("beAtPlacementWS.orientation = %S"), beAtPlacementWS.get().get_orientation().to_rotator().to_string().to_char());
	}
#endif

	if (startInRoom.get())
	{
#ifndef BUILD_PUBLIC_RELEASE
		if (testCellRoomVRLocation.is_set() && !_asNextPilgrimageStart)
		{
			Transform beAt = Transform(testCellRoomVRLocation.get(), testCellRoomVRRotation.get(Rotator3::zero).to_quat());
			Loader::HubScenes::BeAtRightPlace::be_at(beAt, true);
		}
#endif
		Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();
		if (!_asNextPilgrimageStart)
		{
			async_add_pilgrim_and_start(_fromGameState, startInRoom.get(), beAtPlacementWS.get()); // activates all inactive
		}

		game->pop_activation_group(activationGroup.get());
		refresh_force_instant_object_creation(false); // clear - after we add pilgrimage and start
	}
	else
	{
		// try again, maybe after we have the right cell
		game->add_async_world_job(TXT("find place to start"),
			[this, _fromGameState, _asNextPilgrimageStart]()
			{
				async_find_place_to_start(_fromGameState, _asNextPilgrimageStart);
			}
		);
	}
}

VectorInt2 PilgrimageInstanceOpenWorld::validate_coordinates(VectorInt2 at) const
{
	{
		REMOVE_AS_SOON_AS_POSSIBLE_ // remove after a while, this is not possible - the game will crash before we do this
		if (at.x > 1000 || at.x < -1000) at.x = 0;
		if (at.y > 1000 || at.y < -1000) at.y = 0;
	}
	auto* owDef = pilgrimage->open_world__get_definition();
	if (!owDef->has_no_cells())
	{
		RangeInt2 size = owDef->get_size();
		auto& c = get_cell_at(at);
		if (!c.set.get_as_any())
		{
			while (true)
			{
				auto& c = get_cell_at(at);
				if (c.set.get_as_any())
				{
					break;
				}
				VectorInt2 toValid = VectorInt2::zero;
				if (!size.x.is_empty())
				{
					if (at.x > size.x.max) toValid.x = size.x.max - at.x;
					if (at.x < size.x.min) toValid.x = size.x.min - at.x;
				}
				if (!size.y.is_empty())
				{
					if (at.y > size.y.max) toValid.y = size.y.max - at.y;
					if (at.y < size.y.min) toValid.y = size.y.min - at.y;
				}
				if (toValid.is_zero())
				{
					an_assert(false, TXT("within valid zone but no valid content?"));
					break;
				}
				if (abs(toValid.x) > abs(toValid.y))
				{
					at.x += sign(toValid.x);
				}
				else
				{
					at.y += sign(toValid.y);
				}
			}
#ifdef AN_IMPORTANT_INFO
			output(TXT("wanted to start outside valid zone, moved inside to %ix%i"), at.x, at.y);
#endif
		}
	}

	return at;
}

void PilgrimageInstanceOpenWorld::on_end(PilgrimageResult::Type _result)
{
	base::on_end(_result);
	game->set_no_auto_removing_unused_temporary_objects();
}

static inline void crosses__move_hi_inner(REF_ VectorInt2& hi, REF_ VectorInt2& inner, VectorInt2 hiBy)
{
	while (hiBy.x < 0)
	{
		inner.x += 2;
		inner.y -= 1;
		--hi.x;
		++hiBy.x;
	}
	while (hiBy.x > 0)
	{
		inner.x -= 2;
		inner.y += 1;
		++hi.x;
		--hiBy.x;
	}
	while (hiBy.y < 0)
	{
		inner.x += 1;
		inner.y += 2;
		--hi.y;
		++hiBy.y;
	}
	while (hiBy.y > 0)
	{
		inner.x -= 1;
		inner.y -= 2;
		++hi.y;
		--hiBy.y;
	}
}

static inline VectorInt2 crosses__get_hl_idx(VectorInt2 const& _cellAt)
{
	// We just have crosses laid out

	/*
	 *
	 *	...X....X....
	 *	.....X.c..X..
	 *	..X...cXcd..X 
	 *	....X.acdXd..
	 *	.X...aXabd.X.
	 *	...X..abXb...
	 *	X....X..b.X..	
	 *	..X....X.....
	 *	....X....X...
	 */

	VectorInt2 hi;
	hi.x = floor_by(_cellAt.x * 2 - _cellAt.y, 5);
	hi.y = floor_by(_cellAt.y * 2 + _cellAt.x, 5);

	// calculate values in a square (we might be outside the square!
	VectorInt2 inner;
	inner.x = _cellAt.x - ( 2 * hi.x + 1 * hi.y);
	inner.y = _cellAt.y - (-1 * hi.x + 2 * hi.y);

	/*
	 *	...|.....
	 *	...|.....
	 *	..+---+..
	 *	..|ccc|..
	 *	..|acd|..
	 *	--|Xab|--
	 *	..+---+..
	 *	...|.....
	 *	...|.....
	 */ 
	
	// if we're outside the square, change square (squares do overlap!)
	{
		bool isOk = false;
		while (!isOk)
		{
			isOk = true;
			while (inner.x < 0)
			{
				isOk = false;
				crosses__move_hi_inner(REF_ hi, REF_ inner, VectorInt2(-1, 0));
			}
			while (inner.x > 4)
			{
				isOk = false;
				crosses__move_hi_inner(REF_ hi, REF_ inner, VectorInt2(1, 0));
			}
			while (inner.y < 0)
			{
				isOk = false;
				crosses__move_hi_inner(REF_ hi, REF_ inner, VectorInt2(0, -1));
			}
			while (inner.y > 4)
			{
				isOk = false;
				crosses__move_hi_inner(REF_ hi, REF_ inner, VectorInt2(0, 1));
			}
		}
	}

	// now inner is within square, but may actually belong to other higher, move there
	/*
	 *	...|.....
	 *	...|.....
	 *	..+---+..		X +0, +0
	 *	..|yYy|..		Y +0, +1
	 *	..|xyzZ..		Z +1, +1
	 *	--|Xxv|--		V +1, +0
	 *	..+--V+..
	 *	...|.....
	 *	...|.....
	 */ 

	if (inner.x == 2)
	{
		if (inner.y == 0)
		{
			// v
			crosses__move_hi_inner(REF_ hi, REF_ inner, VectorInt2(1, 0));
		}
		else if (inner.y == 1)
		{
			// z
			crosses__move_hi_inner(REF_ hi, REF_ inner, VectorInt2(1, 1));
		}
	}
	else if ((inner.x == 1 && inner.y == 1) ||
			 inner.y == 2)
	{
		// y
		crosses__move_hi_inner(REF_ hi, REF_ inner, VectorInt2(0, 1));
	}

	VectorInt2 hlIdx = hi;

	return hlIdx;
}

static inline VectorInt2 cross_square__get_hl_idx(VectorInt2 const& _cellAt)
{
	// each higher consists of a cross and a square

	/*
	 *
	 *	.............
	 *	......X..X...
	 *	.......bb.... 
	 *	......abb....
	 *	.....aXa.X...
	 *	......a......
	 *	.............
	 */

	VectorInt2 hi;
	hi.x = floor_by(_cellAt.x, 3);
	hi.y = floor_by(_cellAt.y, 3);

	// calculate values in a square (we might be outside the square!
	VectorInt2 inner;
	inner.x = _cellAt.x - 3 * hi.x;
	inner.y = _cellAt.y - 3 * hi.y;

	/*
	 *	...|.....
	 *	...|.....
	 *	..+---+..
	 *	..|?bb|..
	 *	..|abb|..
	 *	--|Xa?|--
	 *	..+---+..
	 *	...|.....
	 *	...|.....
	 */ 

	// we can't be outside the square

	// now inner is within square, but may actually belong to other higher, move there
	/*
	 *	.....|.....
	 *	.....|.....
	 *	....+---+..		X +0, +0
	 *	....|yxx|..		Y +0, +1
	 *	....|xxx|..		Z +1, +0
	 *	----|Xxz|--
	 *	....+---+..
	 *	.....|.....
	 *	.....|.....
	 */ 

	if (inner.x == 0 && inner.y == 2)
	{
		// y
		inner.y -= 3;
		++hi.y;
	}
	else if (inner.x == 2 && inner.y == 0)
	{
		// z
		inner.x -= 3;
		++hi.x;
	}

	// now we're at the actual four crosses, get lo
	/*
	 *	...|....
	 *	...|bb..
	 *	...abb..
	 *	--aXa---
	 *	...a....
	 *	...|....
	 */
	VectorInt2 lo;
	// check a
	if ((inner.x == 0 && inner.y >= -1 && inner.y <= 1) ||
		(inner.y == 0 && inner.x >= -1 && inner.x <= 1))
	{
		lo = VectorInt2(0, 0); // a
	}
	else
	{
		lo = VectorInt2(1, 0); // b
	}

	VectorInt2 hlIdx;
	hlIdx.x = 2 * hi.x + lo.x;
	hlIdx.y = hi.y;

	return hlIdx;
}

static inline VectorInt2 get_high_level_pattern_index(VectorInt2 const& _cellAt, PilgrimageOpenWorldDefinition::Pattern _pattern)
{
	scoped_call_stack_info(TXT("get_high_level_pattern_index"));

	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::CrossSqure)
	{
		return cross_square__get_hl_idx(_cellAt);
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::Crosses)
	{
		return crosses__get_hl_idx(_cellAt);
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::SingleLines)
	{
		return VectorInt2(0, _cellAt.y);
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::DoubleLines)
	{
		int y = _cellAt.y;
		return VectorInt2(0, (y - mod(y, 2)) / 2);
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::DoubleLinesOffset)
	{
		int y = _cellAt.y + 1;
		return VectorInt2(0, (y - mod(y, 2)) / 2);
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::SquaresAligned)
	{
		int x = _cellAt.x - mod(_cellAt.x, 2);
		int y = _cellAt.y - mod(_cellAt.y, 2);
		x = x / 2;
		y = y / 2;
		return VectorInt2(x, y);
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::SquaresMovedX)
	{
		int y = _cellAt.y - mod(_cellAt.y, 2);
		y = y / 2;
		int x = (_cellAt.x - mod(y, 2));
		x = x - mod(x, 2);
		x = x / 2;
		return VectorInt2(x, y);
	}
	return VectorInt2::zero;
}

static inline bool high_level_should_base_on_index_instead_of_random(VectorInt2 const& _cellAt, PilgrimageOpenWorldDefinition::Pattern _pattern)
{
	scoped_call_stack_info(TXT("high_level_should_base_on_index_instead_of_random"));
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::CrossSqure)
	{
		return false;
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::Crosses)
	{
		return false;
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::SingleLines)
	{
		return true;
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::DoubleLines)
	{
		return true;
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::DoubleLinesOffset)
	{
		return true;
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::SquaresAligned)
	{
		return false;
	}
	if (_pattern == PilgrimageOpenWorldDefinition::Pattern::SquaresMovedX)
	{
		return false;
	}
	return false;
}

bool PilgrimageInstanceOpenWorld::should_use_high_level_region_for(VectorInt2 const& _cellAt) const
{
	bool useHLR = true;
	if (!pilgrimage->open_world__get_definition()->should_allow_high_level_region_at_start() &&
		_cellAt == get_start_at())
	{
		useHLR = false;
	}
	else
	{
		auto const& c = get_cell_at(_cellAt);
		if ((c.usesRule && ! c.usesRule->allowHighLevelRegion) || (c.usesObjective && !c.usesObjective->allowHighLevelRegion))
		{
			useHLR = false;
		}
	}
	//
	return useHLR;
}

PilgrimageInstanceOpenWorld::HighLevelRegion* PilgrimageInstanceOpenWorld::async_get_or_create_high_level_region_for(VectorInt2 const& _cellAt)
{
	HighLevelRegionProcessContext context;
	if (async_process_high_level_region_for(_cellAt, REF_ context))
	{
		return context.hlr;
	}
	return nullptr;
}

Framework::RegionType* PilgrimageInstanceOpenWorld::async_get_high_level_region_type_for(VectorInt2 const& _cellAt)
{
	HighLevelRegionProcessContext context;
	context.justGetRegionType = true;
	if (async_process_high_level_region_for(_cellAt, REF_ context))
	{
		return context.hlRegionType;
	}
	return nullptr;
}

bool PilgrimageInstanceOpenWorld::async_process_high_level_region_for(VectorInt2 const& _cellAt, REF_ HighLevelRegionProcessContext& _context)
{
	ASSERT_ASYNC;

	scoped_call_stack_info(TXT("PilgrimageInstanceOpenWorld::async_process_high_level_region_for"));

	if (game->does_want_to_cancel_creation())
	{
		return false;
	}

	refresh_force_instant_object_creation();

	auto* owDef = pilgrimage->open_world__get_definition();
	if (owDef->get_high_level_region_set().is_empty())
	{
		return false;
	}

	if (! _context.justGetRegionType && !should_use_high_level_region_for(_cellAt))
	{
		return false;
	}

	VectorInt2 hlIdx = get_high_level_pattern_index(_cellAt, owDef->get_pattern());
	bool hlBaseOnIndex = high_level_should_base_on_index_instead_of_random(_cellAt, owDef->get_pattern());

	HighLevelRegion* hlr = nullptr;

	if (!_context.justGetRegionType) // for region type we want to ignore existing hlr (we really just want region type and sometimes it may be not set)
	{
		scoped_call_stack_info(TXT("!_context.justGetRegionType"));
		Concurrency::ScopedSpinLock lock(highLevelRegionsLock);

		for_every(phlr, highLevelRegions)
		{
			if (phlr->hlIdx == hlIdx)
			{
				hlr = phlr;
				break;
			}
		}

		if (!hlr)
		{
			highLevelRegions.push_back(HighLevelRegion());
			hlr = &highLevelRegions.get_last();
			hlr->hlIdx = hlIdx;
		}
	}

	// hlr at this point is either created or won't be created at all
	_context.hlr = hlr;

	if (!hlr || !hlr->highLevelRegion.is_set())
	{
		scoped_call_stack_info(TXT("find high level region"));

		auto const& hlrSet = owDef->get_high_level_region_set();

		Random::Generator rg = randomSeed;
		if (hlBaseOnIndex)
		{
			rg.advance_seed(88054, 87525);
		}
		else
		{
			rg.advance_seed(hlIdx.x * 54 + hlIdx.y * 8695, hlIdx.x * 8 + hlIdx.y * 341);
		}

		HighLevelRegionCreator chlr(hlIdx, hlrSet, rg, hlBaseOnIndex, false, mainRegion.get());
		chlr.run(HighLevelRegionCreator::Context().just_get_region_type(_context.justGetRegionType));
		if (_context.justGetRegionType)
		{
			_context.hlRegionType = chlr.regionType;
		}
		else
		{
			if (!hlr)
			{
				return false;
			}
			hlr->highLevelRegion = chlr.topRegion;
			hlr->containerRegion = chlr.containerRegion;
			hlr->aiManagerRoom = chlr.aiManagerRoom;

			if (hlr->highLevelRegion.is_set())
			{
				hlr->highLevelRegion->access_tags().set_tag(NAME(highLevelRegion));
			}
		}
	}

	return true;
}

PilgrimageInstance::TransitionRoom PilgrimageInstanceOpenWorld::async_create_transition_room_for_previous_pilgrimage(PilgrimageInstance* _previousPilgrimage)
{
	ASSERT_ASYNC;

	if (game->does_want_to_cancel_creation())
	{
		PilgrimageInstance::TransitionRoom result;
		return result;
	}

#ifdef AN_IMPORTANT_INFO
	output(TXT("(async) create transition room for previous pilgrimage (or starting room)"));
#endif

	refresh_force_instant_object_creation();

	PilgrimageInstance::TransitionRoom result;

	// if we have a case of a pilgrimage that has no cells and has no rooms for "no cells"
	// then it means that we should connect starting room's directly to each other
	// we should then not create door for transition (entrance door) but reuse starting room's exit door
	Framework::Door* useStartingRoomEntranceDoor = nullptr;
	if (pilgrimage->open_world__get_definition()->get_starting_door_connect_with_previous_pilgrimage())
	{
		if (auto* prevpiow = fast_cast<PilgrimageInstanceOpenWorld>(_previousPilgrimage))
		{
			if (prevpiow->pilgrimage->open_world__get_definition()->has_no_cells() &&
				(prevpiow->pilgrimage->open_world__get_definition()->get_no_cells_rooms().is_empty() ||
				 (prevpiow->pilgrimage->open_world__get_definition()->get_no_cells_rooms().get_size() == 1 &&
				  prevpiow->pilgrimage->open_world__get_definition()->get_no_cells_rooms()[0].roomType.get() == prevpiow->pilgrimage->open_world__get_definition()->get_starting_room_type())))
			{
				useStartingRoomEntranceDoor = prevpiow->startingRoomExitDoor.get();
			}
		}
	}

	// 1. create room
	Framework::Room* room = nullptr;
	Framework::RoomType* roomType = pilgrimage->open_world__get_definition()->get_starting_room_type();
	if (roomType) { roomType->load_on_demand_if_required(); }
	Game::get()->perform_sync_world_job(TXT("create starting/transition room"), [this, &room, roomType]()
		{
			Random::Generator rrg = randomSeed;
			rrg.advance_seed(80235, 97523);
			room = new Framework::Room(subWorld, mainRegion.get(), roomType, rrg);
			ROOM_HISTORY(room, TXT("transition for prev pilgr"));
		});
	// 2. generate room
	{
		// generate before any door is put in, as for stations we base door on POIs
		room->generate();
		room->access_tags().set_tag(NAME(startingRoom));
	}
	// 3. ready for vr before we place door
	{
		room->ready_for_game();
	}
	// 4. store pointer
	{
		result.room= room;
		startInRoom = room;
	}
	Transform outDoorPOIPlacement = Transform::identity;
	Transform outDoorVRAnchor = Transform::identity;
	{
		// get placements first, as we will need to check if we want to have a separate entrance door
		get_poi_placement(room, NAME(outDoor), NAME(door), OUT_ outDoorVRAnchor, OUT_ outDoorPOIPlacement);
	}
	bool shareExit = true;
	// 5. create entrance door (if available)
	if (auto* doorType = pilgrimage->open_world__get_definition()->get_starting_entrance_door_type())
	{
		Transform doorPlacement = Transform::identity;
		Transform vrAnchor = Transform::identity;
		if (get_poi_placement(room, NAME(inDoor), NAME(door), OUT_ vrAnchor, OUT_ doorPlacement))
		{
			if (!Transform::same_with_orientation(outDoorPOIPlacement, doorPlacement))
			{
				Framework::Door* door = useStartingRoomEntranceDoor;
				if (!door)
				{
					door = create_door(doorType);
				}
				door->access_variables().access<bool>(NAME(special)) = true;
				Framework::DoorInRoom* dir = create_door_in_room(door, room, Tags().set_tag(NAME(dt_startingRoomInDoor)).set_tag(NAME(dt_entranceDoor))); // linked as A

				door->be_important_vr_door();
				door->be_world_separator_door(NP, room);

				//dir->access_tags().set_tag(NAME(autoStationDoor));
				{
					Transform vrPlacement = vrAnchor.to_local(doorPlacement);
					if (!dir->is_vr_space_placement_set())
					{
						dir->set_vr_space_placement(vrPlacement);
					}
					else if (!Framework::DoorInRoom::same_with_orientation_for_vr(dir->get_vr_space_placement(), vrPlacement))
					{
						error(TXT("door placement should match!"));
					}
				}
				dir->make_vr_placement_immovable();
				dir->set_placement(doorPlacement);
				dir->init_meshes();

				result.entranceDoor = door;

				shareExit = false;
			}
		}
	}
	// 6. create exit door (starting for this pilgrimage)
	if (auto* doorType = pilgrimage->open_world__get_definition()->get_starting_exit_door_type())
	{
		Transform doorPlacement = outDoorPOIPlacement;
		Transform vrAnchor = outDoorVRAnchor;

		Framework::Door* door = nullptr;
		if (useStartingRoomEntranceDoor &&
			shareExit)
		{
			door = useStartingRoomEntranceDoor;
		}
		if (!door)
		{
			door = create_door(doorType);
		}
		door->access_variables().access<bool>(NAME(special)) = true;
		Framework::DoorInRoom* dir = create_door_in_room(door, room, Tags().set_tag(NAME(dt_startingRoomOutDoor)).set_tag(NAME(dt_exitDoor))); // linked as A

		door->be_important_vr_door();
		door->be_world_separator_door(NP, room);

		//dir->access_tags().set_tag(NAME(autoStationDoor));
		{
			Transform vrPlacement = vrAnchor.to_local(doorPlacement);
			if (!dir->is_vr_space_placement_set())
			{
				dir->set_vr_space_placement(vrPlacement);
			}
			else if (!Framework::DoorInRoom::same_with_orientation_for_vr(dir->get_vr_space_placement(), vrPlacement, 0.03f, NP, 0.01f))
			{
				error(TXT("door placement should match!"));
			}
		}
		dir->make_vr_placement_immovable();
		dir->set_placement(doorPlacement);
		dir->init_meshes();

		result.exitDoor = door;
	}
	// 7. if sharing exit door, mark it accordingly
	if (shareExit && result.exitDoor.get())
	{
		result.entranceDoor = result.exitDoor; // share
		an_assert(result.entranceDoor->get_linked_door_a());
		result.entranceDoor->get_linked_door_a()->access_tags().set_tag(NAME(dt_startingRoomInDoor)).set_tag(NAME(dt_entranceDoor));
	}

	room->keep_door_types();

	// we need a separate call as doors are added post
	room->check_if_doors_valid();

	return result;
}

void PilgrimageInstanceOpenWorld::async_generate_starting_room(GameState* _fromGameState)
{
	ASSERT_ASYNC;

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

#ifdef AN_IMPORTANT_INFO
	output(TXT("(async) create starting room"));
#endif

	refresh_force_instant_object_creation();

	an_assert(!startingRoom.is_set());

	// we always need starting room to always have the starting room door (I don't remember now why but it's useful when we fail on finding spot to continue the game)
	{
		auto tr = async_create_transition_room_for_previous_pilgrimage(nullptr);
		{
			startingRoom = tr.room;
			startInRoom = startingRoom;
			startingRoomExitDoor = tr.exitDoor;
		}
		// get upgrade machine
		{
			on_starting_room_obtained();
		}
	}
}

void PilgrimageInstanceOpenWorld::on_starting_room_obtained()
{
	ASSERT_ASYNC;

	startingRoomUpgradeMachine.clear();
	startingRoomUpgradeMachinePilgrimageDevice.clear();
	FOR_EVERY_ALL_OBJECT_IN_ROOM(obj, startingRoom.get())
	{
		if (obj->get_tags().get_tag(NAME(upgradeMachine)))
		{
			startingRoomUpgradeMachine = obj;
			if (upgradeInStartingRoomUnlocked)
			{
				startingRoomUpgradeMachine->access_variables().access<bool>(NAME(upgradeMachineShouldRemainDisabled)) = true;
			}
			{
				Framework::SceneryType const* objSceneryType = nullptr;
				if (auto* os = fast_cast<Framework::Scenery>(obj))
				{
					objSceneryType = os->get_scenery_type();
				}
				if (objSceneryType)
				{
					Library::get_current_as<Library>()->get_pilgrimage_devices().do_for_every([this, objSceneryType](Framework::LibraryStored* _libraryStored)
						{
							if (auto* pd = fast_cast<PilgrimageDevice>(_libraryStored))
							{
								if (pd->get_scenery_type() != objSceneryType)
								{
									return;
								}

								// make sure it is the device for the current game(we may have different versions
								if (!GameDefinition::get_chosen() ||
									!GameDefinition::get_chosen()->get_pilgrimage_devices_tagged().check(pd->get_tags()))
								{
									return;
								}

								// now check if any distanced or random pilgrimage device that is setup for this pilgrimage actually matches this pd's tags and choose only that one
								auto* owDef = pilgrimage->open_world__get_definition();
								for_every(opd, owDef->get_distanced_pilgrimage_devices())
								{
									if (GameDefinition::check_chosen(opd->forGameDefinitionTagged) &&
										opd->tagged.check(pd->get_tags()))
									{
										startingRoomUpgradeMachinePilgrimageDevice = pd;
										return;
									}
								}
								for_every(opd, owDef->get_random_pilgrimage_devices())
								{
									if (GameDefinition::check_chosen(opd->forGameDefinitionTagged) &&
										opd->tagged.check(pd->get_tags()))
									{
										startingRoomUpgradeMachinePilgrimageDevice = pd;
										return;
									}
								}
							}
						});
				}
			}
		}
	}

	set_manage_starting_room_exit_door(! (startInRoom->get_value<bool>(NAME(handleDoorOperation), false, false /* just this room */) ||
										  startInRoom->get_value<bool>(NAME(handleExitDoorOperation), false, false /* just this room */)));

}

void PilgrimageInstanceOpenWorld::async_add_pilgrim_and_start(GameState* _fromGameState, Framework::Room* _inRoom, Transform const & _placement)
{
	scoped_call_stack_info(TXT("PilgrimageInstanceOpenWorld::async_add_pilgrim_and_start"));

	ASSERT_ASYNC;

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	refresh_force_instant_object_creation();

#ifdef AN_IMPORTANT_INFO
	output(TXT("create pilgrim"));
#endif

	auto* pilgrimType = pilgrimage->get_pilgrim();
	override_pilgrim_type(REF_ pilgrimType, _fromGameState);
	an_assert(pilgrimType);
	pilgrimType->load_on_demand_if_required();

	startInRoom = _inRoom;
	Framework::Actor* playerActor;
	game->perform_sync_world_job(TXT("create pilgrim"), [this, &playerActor, pilgrimType]()
	{
		playerActor = new Framework::Actor(pilgrimType, String(TXT("player")));
		playerActor->init(subWorld);
	});

	{
		Random::Generator rg = randomSeed;
		rg.advance_seed(239782, 23978234);
		playerActor->access_individual_random_generator() = rg;
	}

#ifdef AN_IMPORTANT_INFO
	if (_fromGameState) _fromGameState->debug_log(TXT("usingGameState"));
#endif

	game->access_player().bind_actor(playerActor, Framework::Player::BindActorParams().lazy_bind());
	{
		ModulePilgrim::s_hackCreatingForGameState = _fromGameState;
		playerActor->initialise_modules();
		ModulePilgrim::s_hackCreatingForGameState = nullptr;
	}
	game->access_player().bind_actor(playerActor);
	{	// place at vr anchor, will shift to proper position
		game->perform_sync_world_job(TXT("place pilgrim"), [this, &playerActor, _placement]()
			{
				playerActor->get_presence()->place_in_room(startInRoom.get(), _placement);
			});
	}

	game->request_ready_and_activate_all_inactive(game->get_current_activation_group());
	
	game->request_initial_world_advance_required();

	if (startInRoom == startingRoom)
	{
		// this is more temporary solution until the room gets properly generated
		/*
		game->add_sync_world_job(TXT("change vr anchor for room"), [this, playerActor]()
		{
			// makes sense only if vr is active
			if (VR::IVR::get() &&
				VR::IVR::get()->can_be_used() &&
				VR::IVR::get()->is_controls_view_valid())
			{
				Vector3 vrViewLoc = VR::IVR::get()->get_controls_view().get_translation();
				Transform playerVRAnchor = playerActor->get_presence()->get_vr_anchor();
				Transform doorVRAnchor;
				Transform doorPlacement;
				get_poi_placement(startInRoom.get(), NAME(outDoor), NAME(door), OUT_ doorVRAnchor, OUT_ doorPlacement);

				Vector3 vrDoorLoc = playerVRAnchor.location_to_local(doorPlacement.get_translation()); // player VR!
				Vector3 vrRotatedDoorLoc = -vrDoorLoc;

				float distNormal = (vrViewLoc - vrDoorLoc).length();
				float distRotated = (vrViewLoc - vrRotatedDoorLoc).length();

				if (distRotated > distNormal)
				{
					// rotate player's vr to place it further from the door
					playerActor->get_presence()->turn_vr_anchor_by_180();
				}
			}
		});
		*/
	}

	if (GameSettings::get().difficulty.linearLevels)
	{
		for (int i = 0; ; ++i)
		{
			ExistingCell* ec = nullptr;
			{
				Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("tag open world directions for cell"));

				if (i < existingCells.get_size())
				{
					ec = &existingCells[i];
				}
			}
			if (ec)
			{
				async_hide_doors_that_wont_be_used(ec);
			}
			else
			{
				break;
			}
		}
	}

	// make sure everything in our room is done
	if (auto* r = startInRoom.get())
	{
#ifdef AN_IMPORTANT_INFO
		output(TXT("process all delayed objects for start in room \"%S\" and neighbours"), r->get_name().to_char());
#endif
		game->process_delayed_object_creation_immediately(r);

		for_every_ptr(door, r->get_doors())
		{
			if (auto* r = door->get_room_on_other_side())
			{
				game->process_delayed_object_creation_immediately(r);
			}
		}
	}

	// mark that we will be using world
	game->post_use_world(true);

	// switch into performance mode
	game->post_set_performance_mode(true);

	generatedPilgrimAndStarted = true;
}

void PilgrimageInstanceOpenWorld::pre_advance()
{
	base::pre_advance();

	if (!leaveExitDoorAsIs) // set by game script
	{
		// manageExitDoor is set via handleDoorOperation or handleExitDoorOperation variable for room
		canExitStartingRoom = false;
		if (auto* d = startingRoomExitDoor.get())
		{
			{
				auto* da = d->get_linked_door_a();
				auto* db = d->get_linked_door_b();
				if (da && db)
				{
					auto* ra = da->get_in_room();
					auto* rb = db->get_in_room();
					if (ra && rb)
					{
						if (ra->is_world_active() &&
							rb->is_world_active())
						{
							canExitStartingRoom = true;
						}
					}
				}
			}
			bool wasStartingDoorAdvisedToBeOpen = startingDoorAdvisedToBeOpen;
			if (!leftStartingRoom && canExitStartingRoom &&
				startingRoom.get() && AVOID_CALLING_ACK_ startingRoom->was_recently_seen_by_player() &&
				(upgradeInStartingRoomUnlocked || !startingRoomUpgradeMachine.get()))
			{
				startingDoorAdvisedToBeOpen = true;
			}
			else
			{
				startingDoorAdvisedToBeOpen = false;
				if (!leftStartingRoom && canExitStartingRoom &&
					d->is_closed() &&
					d->get_linked_door_b() &&
					d->get_linked_door_b()->get_in_room() &&
					AVOID_CALLING_ACK_ !d->get_linked_door_b()->get_in_room()->was_recently_seen_by_player() &&
					d->get_linked_door_a() &&
					d->get_linked_door_a()->get_in_room() &&
					AVOID_CALLING_ACK_ !d->get_linked_door_a()->get_in_room()->was_recently_seen_by_player())
				{
					leftStartingRoom = true;
				}
			}
			if (keepExitDoorOpen)
			{
				// force them to remain open if we already opened them and left
				startingDoorAdvisedToBeOpen |= wasStartingDoorAdvisedToBeOpen || leftStartingRoom;
			}
			if (manageExitDoor)
			{
				if (startingDoorRequestedToBeOpen != startingDoorAdvisedToBeOpen)
				{
					if (startingDoorAdvisedToBeOpen && triggerGameScriptExecutionTrapOnOpenExitDoor.is_valid())
					{
						Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptExecutionTrapOnOpenExitDoor);
					}
				}
				startingDoorRequestedToBeOpen = startingDoorAdvisedToBeOpen;
			}
			if (startingDoorRequestedToBeOpen && canExitStartingRoom && !startingDoorRequestedToBeClosed)
			{
				if (! triggerGameScriptExecutionTrapOnOpenExitDoor.is_valid())
				{
					d->set_operation(Framework::DoorOperation::StayOpen, Framework::Door::DoorOperationParams().allow_auto(!keepExitDoorOpen && leftStartingRoom)); // force open unless has left starting room then switch to auto
				}
			}
			else
			{
				d->set_operation(Framework::DoorOperation::StayClosed);
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::issue_destroy_cell(VectorInt2 const& _at)
{
	if (workingOnCells)
	{
		return;
	}

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	if (! game->does_allow_async_jobs())
	{
		return;
	}

	workingOnCells = true;
	game->block_destroying_world(true);
	isBusyAsync = true;
	VectorInt2 cellAt = _at;
	tsprintf(workingOnCellsInfo, 127, TXT("async destroy open world cell %ix%i"), _at.x, _at.y);
	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(workingOnCellsInfo,
		[keepThis, this, cellAt]()
		{
			MEASURE_PERFORMANCE(destroyCell);

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output_colour(0, 1, 1, 1);
			output(TXT("destroy open world cell %ix%i"), cellAt.x, cellAt.y);
			output_colour();
#endif
#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
			output(TXT("destroy open world cell %ix%i"), cellAt.x, cellAt.y);
#endif

			moreExistingCellsNeeded = false;

#ifdef AN_DEVELOPMENT_OR_PROFILER
			destroyingCell = true;
#endif

			game->perform_sync_world_job(TXT("unlink doors"),
				[keepThis, this, cellAt]()
				{
					if (cellAt == get_start_at())
					{
						sync_unlink_door_to_cell(startingRoomExitDoor, cellAt);
					}

					if (auto* ec = get_existing_cell(cellAt))
					{
						for_every(door, ec->doors)
						{
							sync_unlink_door_to_cell(door->door, cellAt);
						}
						if (ec->transitionEntranceDoor.is_set())
						{
							sync_unlink_door_to_cell(ec->transitionEntranceDoor, cellAt);
						}
					}
				});

			if (auto* ec = get_existing_cell(cellAt))
			{
				store_pilgrim_device_state_variables(ec->at);

				if (auto* room = ec->transitionRoom.get())
				{
					Game::get()->perform_sync_world_job(TXT("destroy room and issue crawling destruction of door in it"), [room]()
						{
#ifdef AN_OUTPUT_WORLD_GENERATION
							output(TXT("destruction of room (transition via open world cell destruction) \"%S\""), room->get_name().to_char());
#endif
							room->clear_inside();
#ifdef AN_OUTPUT_WORLD_GENERATION
							output(TXT("open world cell destruction - destroy actual room \"%S\""), room->get_name().to_char());
#endif
							// and destroy room
							delete room;
						});
				}

				if (ec->region.is_set())
				{
					// will issue destruction of everything inside
					ec->region->start_destruction();
				}
			}

			game->perform_sync_world_job(TXT("remove cell"),
				[keepThis, this, cellAt]()
				{
					Concurrency::ScopedMRSWLockWrite lock(existingCellsLock, TXT("remove cell"));
					for_every_iter(ec, existingCells)
					{
						if ((*ec).at == cellAt)
						{
							existingCells.remove(ec);
							break;
						}
					}
				});

			// we will remove things soon
			game->request_removing_unused_temporary_objects();

#ifdef AN_DEVELOPMENT_OR_PROFILER
			destroyingCell = false;
#endif
			async_update_no_door_markers(cellAt);

			workingOnCells = false;
			game->block_destroying_world(false);
			isBusyAsync = false;

#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
			output(TXT("destroyed open world cell"));
#endif
		});
}

void PilgrimageInstanceOpenWorld::post_setup_rgi()
{
	base::post_setup_rgi();
#ifdef AN_IMPORTANT_INFO
	output(TXT("rgi setup done"));
#endif
	output_dev_info();
}

void PilgrimageInstanceOpenWorld::output_dev_info(Framework::IModulesOwner* _for) const
{
#ifdef AN_OUTPUT_DEV_INFO
	List<String> lines = build_dev_info(_for);
	lock_output();
	::System::FaultHandler::store_custom_info(String(TXT("pilgrimage")));
	for_every(l, lines)
	{
		s_a_o(TXT("%S"), l->to_char());
	}
	unlock_output();
#endif
}

List<String> PilgrimageInstanceOpenWorld::build_dev_info(Framework::IModulesOwner* _for) const
{
	List<String> lines;
	{	// output in an easier to load way
		s_a_o_line(TXT("dev info (%S)"), _for? TXT("location of a character") : TXT("config test string"));
		s_a_o_line(TXT("    <bool name=\"testPilgrimageGameScript\">true</bool>"));
		if (_for)
		{
			if (auto* p = _for->get_presence())
			{
				if (auto* r = p->get_in_room())
				{
					s_a_o_line(TXT("    <vectorInt2 name=\"testOpenWorldCell\" x=\"%i\" y=\"%i\"/>"), r->get_tags().get_tag_as_int(NAME(openWorldX)), r->get_tags().get_tag_as_int(NAME(openWorldY)));
#ifdef AN_TAG_OPEN_WORLD_ROOMS
					s_a_o_line(TXT("    <int name=\"testCellRoomIdx\">%i</int>"), r->get_tags().get_tag_as_int(NAME(cellRoomIdx)));
#endif
					s_a_o_line(TXT("    room's name is \"%S\""), r->get_name().to_char());
					s_a_o_line(TXT("    room's tags are \"%S\""), r->get_tags().to_string().to_char());
					{
						Transform placement = p->get_placement();
						Vector3 pl = placement.get_translation();
						s_a_o_line(TXT("    <vector3 name=\"testCellRoomLocation\" x=\"%.3f\" y=\"%.3f\" z=\"%.3f\"/>"), pl.x, pl.y, pl.z);
						Rotator3 pr = placement.get_orientation().to_rotator();
						s_a_o_line(TXT("    <rotator3 name=\"testCellRoomRotation\" pitch=\"%.3f\" yaw=\"%.3f\" roll=\"%.3f\"/>"), pr.pitch, pr.yaw, pr.roll);
					}
					{
						Transform placement = p->get_vr_anchor().to_local(p->get_placement());
						Vector3 pl = placement.get_translation();
						s_a_o_line(TXT("    <vector3 name=\"testCellRoomVRLocation\" x=\"%.3f\" y=\"%.3f\" z=\"%.3f\"/>"), pl.x, pl.y, pl.z);
						Rotator3 pr = placement.get_orientation().to_rotator();
						s_a_o_line(TXT("    <rotator3 name=\"testCellRoomVRRotation\" pitch=\"%.3f\" yaw=\"%.3f\" roll=\"%.3f\"/>"), pr.pitch, pr.yaw, pr.roll);
					}
				}
			}
		}
		else
		{
			s_a_o_line(TXT("    <vectorInt2 name=\"testOpenWorldCell\" x=\"%i\" y=\"%i\"/>"), creatingOpenWorldCell.x, creatingOpenWorldCell.y);
		}
		if (currentObjectiveIdx.is_set())
		{
			s_a_o_line(TXT("    <int name=\"testObjectiveIdx\">%i</int>"), currentObjectiveIdx.get());
		}
		if (GameDefinition::get_chosen())
		{
			s_a_o_line(TXT("    <libraryName name=\"testGameDefinition\">%S</libraryName>"), GameDefinition::get_chosen()->get_name().to_string().to_char());
		}
		if (auto* gsd = GameDefinition::get_chosen_sub(0))
		{
			s_a_o_line(TXT("    <libraryName name=\"testGameSubDefinition\">%S</libraryName>"), gsd->get_name().to_string().to_char());
		}
		s_a_o_line(TXT("    <libraryName name=\"testPilgrimage\">%S</libraryName>"), get_pilgrimage()->get_name().to_string().to_char());
		if (randomSeed.get_seed_lo_number() == 0)
		{
			s_a_o_line(TXT("    <int name=\"testSeed\">%u</int>"), randomSeed.get_seed_hi_number());
		}
		else
		{
			s_a_o_line(TXT("    <vectorInt2 name=\"testSeed\" x=\"%u\" y=\"%u\"/>"), randomSeed.get_seed_hi_number(), randomSeed.get_seed_lo_number());
		}
		s_a_o_line(TXT("    <bool name=\"testImmobileVR\">%S</bool>"), Option::to_char(MainConfig::global().get_immobile_vr()));
		s_a_o_line(TXT("    <bool name=\"testImmobileVRAuto\">%S</bool>"), MainConfig::global().get_immobile_vr_auto() ? TXT("true") : TXT("false"));
		s_a_o_line(TXT("    <vector2 name=\"testPlayArea\" x=\"%.2f\" y=\"%.2f\"/>"), RoomGenerationInfo::get().get_play_area_rect_size().x, RoomGenerationInfo::get().get_play_area_rect_size().y);
		if (auto* vr = VR::IVR::get())
		{
			s_a_o_line(TXT("    <float name=\"testScaling\">%.9f</float>"), vr->get_scaling().general);
		}
		else
		{
			s_a_o_line(TXT("    <float name=\"testScaling\">1.0</float>"));
		}
		s_a_o_line(TXT("    <name name=\"testPreferredTileSize\">%S</name>"), PreferredTileSize::to_char(RoomGenerationInfo::get().get_preferred_tile_size()));
		s_a_o_line(TXT("    <float name=\"testTileSize\">%.9f</float>"), RoomGenerationInfo::get().get_tile_size());
		s_a_o_line(TXT("    <float name=\"testDoorHeight\">%.9f</float>"), RoomGenerationInfo::get().get_door_height());
		s_a_o_line(TXT("    <bool name=\"testAllowCrouch\">%S</bool>"), RoomGenerationInfo::get().does_allow_crouch() ? TXT("true") : TXT("false"));
		s_a_o_line(TXT("    <bool name=\"testAllowAnyCrouch\">%S</bool>"), RoomGenerationInfo::get().does_allow_any_crouch() ? TXT("true") : TXT("false"));
		s_a_o_line(TXT("    <string name=\"testGenerationTags\">%S</string>"), RoomGenerationInfo::get().get_generation_tags().to_string(true).to_char());
		{
			Tags difficultySettingsTags;
			TeaForGodEmperor::GameSettings::get().difficulty.store_as_tags(difficultySettingsTags);
			s_a_o_line(TXT("    <string name=\"testDifficultySettings\">%S</string>"), difficultySettingsTags.to_string(true).to_char());
		}
	}
	return lines;
}

bool PilgrimageInstanceOpenWorld::issue_create_cell(VectorInt2 const& _at, Optional<bool> const& _force, Optional<bool> const& _ignoreLimit)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (!wantsToCreateCellAt.has_place_left())
	{
		// it's just an information
		wantsToCreateCellAt.pop_front();
	}
	wantsToCreateCellAt.push_back_unique(_at);
#endif
	if (workingOnCells)
	{
		return false;
	}

#ifdef AN_DEVELOPMENT
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("check if generated (assert)"));
		for_every(c, existingCells)
		{
			an_assert(c->at != _at, TXT("cell already generated"));
		}
	}
#endif

	if (!_ignoreLimit.get(false))
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("check if there aren't too many cells"));
		if (existingCells.get_size() > 7)
		{
			moreExistingCellsNeeded = true;
			// we don't want to have too many cells
			return false;
		}
	}

	if (game->does_want_to_cancel_creation())
	{
		return false;
	}

	if (!_force.get(false))
	{
		if (! game->does_allow_async_jobs())
		{
			return false;
		}

		if (!game->is_on_short_pause())
		{
			if (auto* navSys = game->get_nav_system())
			{
				if (navSys->get_pending_build_tasks_count())
				{
					// as soon as we're done with this one
					return false;
				}
			}
		}
	}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output(TXT("issue create open world cell %ix%i"), _at.x, _at.y);
#endif

	return add_async_create_cell(_at);
}

bool PilgrimageInstanceOpenWorld::add_async_create_cell(VectorInt2 const& _at)
{
	if (workingOnCells)
	{
		if (game->does_want_to_cancel_creation())
		{
			return false;
		}
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output(TXT("postpone create cell %ix%i"), _at.x, _at.y);
#endif
		RefCountObjectPtr<PilgrimageInstance> keepThis(this);
		game->add_async_world_job(workingOnCellsInfo,
			[keepThis, this, _at]()
			{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output(TXT("postponed create cell %ix%i"), _at.x, _at.y);
#endif
				add_async_create_cell(_at);
			});
		return false;
	}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output(TXT("queue create open world cell %ix%i"), _at.x, _at.y);
#endif

	workingOnCells = true;
	game->block_destroying_world(true);
	isBusyAsync = true;
	VectorInt2 cellAt = _at;
	tsprintf(workingOnCellsInfo, 127, TXT("async create open world cell %ix%i"), cellAt.x, cellAt.y);
	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(workingOnCellsInfo,
		[keepThis, this, cellAt]()
		{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("via issue_create_cell %ix%i"), cellAt.x, cellAt.y);
#endif
			async_create_cell_at(cellAt);
		});

	return true;
}

void PilgrimageInstanceOpenWorld::set_door_direction_objective_override(Optional<DoorDirectionObjectiveOverride::Type> const& _doorDirectionObjectiveOverride)
{
	if (_doorDirectionObjectiveOverride.is_set() && _doorDirectionObjectiveOverride.get() != DoorDirectionObjectiveOverride::None)
	{
		doorDirectionObjectiveOverride = _doorDirectionObjectiveOverride;
	}
	else
	{
		doorDirectionObjectiveOverride.clear();
	}
}

bool PilgrimageInstanceOpenWorld::request_create_cell_for_objective(Name const& _id)
{
	Optional<VectorInt2> at;
	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		for_every(o, owDef->get_objectives())
		{
			if (o->id == _id)
			{
				if (o->at.is_set())
				{
					at = o->at;
					break;
				}
				if (o->atStartAt)
				{
					at = get_start_at();
				}
			}
		}
	}

	if (at.is_set())
	{
		bool existsAndAccessible = false;
		bool needToCreate = true;
		{
			Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("check if there aren't too many cells"));
			for_every(ec, existingCells)
			{
				if (ec->at == at.get())
				{
					needToCreate = false; // it already exists, maybe is active or just being generated
					if (ec->generated)
					{
						existsAndAccessible = true;
						break;
					}
				}
			}
		}
		if (needToCreate)
		{
			issue_create_cell(at.get(), true, true);
		}

		return existsAndAccessible;
	}
	else
	{
		warn_dev_investigate(TXT("no objective to create cell for"));
		return true;
	}
}

bool PilgrimageInstanceOpenWorld::done_objective(Name const& _id)
{
	auto* owDef = pilgrimage->open_world__get_definition();
	if (!owDef->are_objectives_sequential())
	{
		return false; // won't make sense
	}

	Optional<int> doneObjectiveIdx;
	int checkObjectiveIdx = currentObjectiveIdx.get();
	while (owDef->get_objectives().is_index_valid(checkObjectiveIdx))
	{
		auto& obj = owDef->get_objectives()[checkObjectiveIdx];
		if (!obj.notActualObjective &&
			is_objective_ok(&obj) &&
			obj.id == _id)
		{
			doneObjectiveIdx = checkObjectiveIdx;
			break;
		}
		checkObjectiveIdx = checkObjectiveIdx + 1;
	}

	if (!doneObjectiveIdx.is_set())
	{
#ifdef AN_IMPORTANT_INFO
		output(TXT("couldn't do objective (done or not existing) \"%S\" (at %i)"), _id.to_char(), currentObjectiveIdx.get());
#endif
		return false;
	}
	else
	{
#ifdef AN_IMPORTANT_INFO
		output(TXT("done objective \"%S\" (at %i)"), _id.to_char(), currentObjectiveIdx.get());
#endif
		currentObjectiveIdx = doneObjectiveIdx;
		return next_objective();
	}
}

bool PilgrimageInstanceOpenWorld::next_objective()
{
	return next_objective_internal();
}

bool PilgrimageInstanceOpenWorld::next_objective_internal(bool _advance)
{
#ifdef AN_IMPORTANT_INFO
	output(TXT("next objective internal from [%i]"), currentObjectiveIdx.get(NONE));
#endif
	int advanceIdx = _advance? 1 : 0;
	int prevIdx = currentObjectiveIdx.get(NONE);
	if (!currentObjectiveIdx.is_set())
	{
		currentObjectiveIdx = 0;
		advanceIdx = 0;
	}
	auto* owDef = pilgrimage->open_world__get_definition();
	if (!owDef->are_objectives_sequential())
	{
		// always find the first one
		currentObjectiveIdx = 0;
		advanceIdx = 0;
	}
	if (GameSettings::get().difficulty.linearLevels)
	{
		advanceIdx = owDef->get_objectives().get_size();
	}
	{
		hasMoreObjectives = true;
		int lastValid = currentObjectiveIdx.get();
		while (owDef->get_objectives().is_index_valid(currentObjectiveIdx.get()))
		{
			auto& obj = owDef->get_objectives()[currentObjectiveIdx.get()];
#ifdef AN_IMPORTANT_INFO
			output(TXT("check [%i] %S"), currentObjectiveIdx.get(), obj.id.to_char());
#endif
			if (!obj.notActualObjective &&
				is_objective_ok(&obj))
			{
				lastValid = currentObjectiveIdx.get();
				if (advanceIdx == 0)
				{
					break;
				}
				--advanceIdx;
			}
			currentObjectiveIdx = currentObjectiveIdx.get() + 1;
		}

		if (!owDef->get_objectives().is_index_valid(currentObjectiveIdx.get()))
		{
			currentObjectiveIdx = lastValid;
			hasMoreObjectives = false;
		}
	}

	// allow to recreate cells surrounding our cell in case we're missing something
	cellsHandledAt.clear();

	if (currentObjectiveIdx.get() != prevIdx)
	{
#ifdef AN_IMPORTANT_INFO
		output(TXT("next objective [%i]"), currentObjectiveIdx.get());
#endif
		game->add_async_world_job_top_priority(TXT("update towards objective"),
			[this]()
			{
				async_invalidate_towards_objective();
				async_update_towards_objective_basing_on_existing_cells();
			}
		);

		return true;
	}
	else
	{
		return false;
	}
}

void PilgrimageInstanceOpenWorld::async_invalidate_towards_objective()
{
	ASSERT_ASYNC;

	{
		Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("invalidate towards objective"));
		for_every(_c, cells)
		{
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
			if (_c->towardsObjectiveIdx.is_set())
			{
				output(TXT("[towardsObjective] clear at %S"), _c->at.to_string().to_char());
			}
#endif
			_c->towardsObjectiveIdx.clear();
			_c->towardsObjectiveCostWorker.clear();
		}
	}

	{
		Concurrency::ScopedMRSWLockWrite lock(existingCellsLock, TXT("invalidate towards objective"));
		for_every(_c, existingCells)
		{
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
			if (_c->towardsObjectiveIdx.is_set())
			{
				output(TXT("[towardsObjective] clear existing at %S"), _c->at.to_string().to_char());
			}
#endif
			_c->towardsObjectiveIdx.clear();
			_c->towardsObjectiveCostWorker.clear();
		}
	}
}

void PilgrimageInstanceOpenWorld::async_update_towards_objective_basing_on_existing_cells()
{
	ASSERT_ASYNC;

	Array<VectorInt2> coords;
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get existing cell coords"));
		for_every(_c, existingCells)
		{
			coords.push_back(_c->at);
		}
	}

	for_every(coord, coords)
	{
		async_update_towards_objective(*coord, for_everys_index(coord) == coords.get_size() - 1);
	}
}

void PilgrimageInstanceOpenWorld::async_update_towards_objective(VectorInt2 const& cellAt, bool _updateExisting)
{
	ASSERT_ASYNC;

	bool noCells = pilgrimage->open_world__get_definition()->has_no_cells();

	// find path towards objective
	if (!noCells && (GameSettings::get().difficulty.showDirectionsOnlyToObjective || pilgrimage->open_world__get_definition()->does_require_directions_towards_objective()))
	{
		auto const& cell = get_cell_at(cellAt);
		if (! cell.towardsObjectiveIdx.is_set())
		{
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
			output(TXT("[towardsObjective] update required for %S"), cellAt.to_string().to_char());
#endif
			// find towards closest objective
			{
				{
					Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("towards objective setup"));
					for_every(_c, cells)
					{
						if (_c->at == cellAt)
						{
							_c->towardsObjectiveCostWorker = 0.0f;
						}
						else
						{
							_c->towardsObjectiveCostWorker.clear();
						}
					}
				}

				int maxDistance = hardcoded magic_number 20; // put it into definition?

				Array<VectorInt2> closestObjectives;
				int radius = 0;
				while (closestObjectives.get_size() < 2 && radius < maxDistance)
				{
					for_range(int, oy, -radius, radius)
					{
						for_range(int, ox, -radius, radius)
						{
							if (oy == -radius || oy == radius ||
								ox == -radius || ox == radius)
							{
								VectorInt2 at = cellAt + VectorInt2(ox, oy);
								get_cell_at(at); // we may need to have cell there (known pilgrimage devices!)
								if (auto* obj = get_objective_for(at, currentObjectiveIdx))
								{
									if (!obj->notActualObjective)
									{
										closestObjectives.push_back(at);
									}
								}
							}
						}
					}
					++radius;
				}

				if (!closestObjectives.is_empty())
				{
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
					for_every(co, closestObjectives)
					{
						output(TXT("[towardsObjective] closest objective at %S"), co->to_string().to_char());
					}
#endif
					Optional<VectorInt2> foundAt;
					float foundAtCost = 0.0f;
					float toCost = -1.0f;
					while (!foundAt.is_set())
					{
#ifdef DEBUG_TOWARDS_OBJECT_PROPAGATION
						bool setSomething = false;
#endif
						{
							Optional<float> nextTOCostProposal;
							Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("towards objective next"));
							for_every(c, cells)
							{
								if (c->towardsObjectiveCostWorker.is_set() &&
									c->towardsObjectiveCostWorker.get() > toCost &&
									(!nextTOCostProposal.is_set() || c->towardsObjectiveCostWorker.get() < nextTOCostProposal.get()))
								{
									nextTOCostProposal = c->towardsObjectiveCostWorker.get();
								}
							}
							if (!nextTOCostProposal.is_set())
							{
								break;
							}
							toCost = nextTOCostProposal.get();
						}
						for (int i = 0; ; ++i)
						{
							{
								Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("towards objective next"));
								if (i >= cells.get_size())
								{
									break;
								}
							}
							bool exits[DirFourClockwise::NUM];
							VectorInt2 cAt = cellAt;
							{
								Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("towards objective next"));
								auto& _c = cells[i];
								cAt = _c.at;
								if (_c.towardsObjectiveCostWorker.is_set() &&
									_c.towardsObjectiveCostWorker.get() == toCost)
								{
									memory_copy(exits, _c.exits, sizeof(bool) * DirFourClockwise::NUM);
								}
								else
								{
									continue;
								}
							}
							{
								for_count(int, dirIdx, DirFourClockwise::NUM)
								{
									if (exits[dirIdx])
									{
										VectorInt2 dir = DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)dirIdx);
										VectorInt2 to = cAt + dir;
										auto& cTo = access_cell_at(to);
										float addTowardsObjectiveCost = 1.0f;
										if (auto* r = cTo.set.get_as_any())
										{
											if (r->get_tags().has_tag(NAME(towardsObjectiveCost)))
											{
												addTowardsObjectiveCost = r->get_tags().get_tag(NAME(towardsObjectiveCost));
											}
										}
										float setTOCost = toCost + addTowardsObjectiveCost;
										if (!cTo.towardsObjectiveCostWorker.is_set() ||
											cTo.towardsObjectiveCostWorker.get() > setTOCost)
										{
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
											output(TXT("[towardsObjective] set worker cost at %S to %.3f"), cTo.at.to_string().to_char(), setTOCost);
#endif
											cTo.towardsObjectiveCostWorker = setTOCost;
#ifdef DEBUG_TOWARDS_OBJECT_PROPAGATION
											setSomething = true;
#endif
										}
										{
											bool atObjective = false;// don't use existing one as we may have skip some non existent cTo.towardsObjective.is_set();
											if (!atObjective)
											{
												for_every(obj, closestObjectives)
												{
													if (to == *obj)
													{
														atObjective = true;
														break;
													}
												}
											}
											if (atObjective)
											{
												an_assert(cTo.towardsObjectiveCostWorker.is_set());
												float cost = cTo.towardsObjectiveCostWorker.get();
												if (!foundAt.is_set() ||
													foundAtCost > cost)
												{
													foundAt = to;
													foundAtCost = cost;
												}
											}
										}
									}
								}
							}
							if (foundAt.is_set())
							{
								break;
							}
						}
#ifdef DEBUG_TOWARDS_OBJECT_PROPAGATION
						if (setSomething)
						{
							debug_visualise_map(cellAt, true, cellAt); // !@#
						}
#endif
					}
					if (foundAt.is_set())
					{
						// go from this one to 0 to fill path
						VectorInt2 at = foundAt.get();
						while (true)
						{
							VectorInt2 prevAt = at;
							auto& cAt = access_cell_at(at);
							if (at == foundAt.get())
							{
								// mark valid without dir
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
								output(TXT("[towardsObjective] target at %S"), at.to_string().to_char());
#endif
								cAt.towardsObjectiveIdx = currentObjectiveIdx;
								cAt.towardsObjective.clear();
							}
							if (cAt.towardsObjectiveCostWorker.is_set())
							{
								float cost = cAt.towardsObjectiveCostWorker.get();
								if (cost == 0.0f)
								{
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
									output(TXT("[towardsObjective] start at %S"), at.to_string().to_char());
#endif
									cAt.towardsObjectiveIdx = currentObjectiveIdx;
									// it should be the best, quit now, doesn't matter if dir is set or not (we could start at the objective)
									break;
								}
								{
									Optional<VectorInt2> bestAt;
									Optional<DirFourClockwise::Type> bestAtSetDir;
									float bestAtCost = 0.0f;
									float bestAtDist = 0.0f;
									for_count(int, dirIdx, DirFourClockwise::NUM)
									{
										if (cAt.exits[dirIdx])
										{
											VectorInt2 dir = DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)dirIdx);
											VectorInt2 to = at + dir;
											auto& cTo = access_cell_at(to);
											if (cTo.towardsObjectiveCostWorker.is_set() &&
												cTo.towardsObjectiveCostWorker.get() < cost)
											{
												float cToCost = cTo.towardsObjectiveCostWorker.get();
												float cToDist = (cTo.at - cellAt).to_vector2().length()
													- (cTo.at - foundAt.get()).to_vector2().length();
												if (!bestAt.is_set() ||
													bestAtCost > cToCost || // the fastest way
													(bestAtCost == cToCost && bestAtDist > cToDist)) // we're looking for the closest to start as it may mean we move the fastest
												{
													bestAt = cTo.at;
													bestAtCost = cToCost;
													bestAtDist = cToDist;
													bestAtSetDir = DirFourClockwise::opposite_dir((DirFourClockwise::Type)dirIdx);
												}
											}
										}
									}
									if (bestAt.is_set())
									{
										auto& cTo = access_cell_at(bestAt.get());
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
										output(TXT("[towardsObjective] set at %S in %i (towards %S)"), cTo.at.to_string().to_char(), bestAtSetDir.get(), at.to_string().to_char());
#endif
										cTo.towardsObjectiveIdx = currentObjectiveIdx;
										cTo.towardsObjective = bestAtSetDir.get();
										if (!GameSettings::get().difficulty.linearLevels)
										{
											for_every(objective, pilgrimage->open_world__get_definition()->get_objectives())
											{
												if (objective->forceTowardsDir.is_set())
												{
													if (is_objective_ok_at(objective, cTo.at))
													{
														cTo.towardsObjective = objective->forceTowardsDir.get();
													}
												}
											}
										}
										at = bestAt.get();
										if (cellsHandledAt.is_set() && cellsHandledAt.get() == cTo.at)
										{
											cellsHandledAt.clear(); // check again if we changed "towardsObjective"
										}
#ifdef DEBUG_TOWARDS_OBJECT_SET_DIRS
										debug_visualise_map(cellAt, true, at);
#endif
									}
								}
							}
							else
							{
								an_assert(false);
								break;
							}
							if (prevAt == at)
							{
								an_assert(false);
								break;
							}
						}
					}
				}
			}
		}

		if (_updateExisting)
		{
			Concurrency::ScopedMRSWLockWrite lock(existingCellsLock, TXT("update existing cells"));
			for_every(_c, existingCells)
			{
				auto const& cell = get_cell_at(_c->at);
				if (cell.towardsObjectiveIdx.is_set())
				{
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
					output(TXT("[towardsObjective] copy for %S in %i"), _c->at.to_string().to_char(), cell.towardsObjective.is_set()? (int)cell.towardsObjective.get() : NONE);
#endif

					_c->towardsObjectiveIdx = cell.towardsObjectiveIdx;
					_c->towardsObjective = cell.towardsObjective;
					_c->towardsPilgrimageDevice.clear();
					if (!cell.towardsObjective.is_set()) // look for objective inside
					{
						// check if any of objectives may point at this pilgrimage device
						for_every(objective, pilgrimage->open_world__get_definition()->get_objectives())
						{
							if (objective->pilgrimageDeviceGroupId.is_valid())
							{
								if (is_objective_ok(objective))
								{
									for_every(kpd, _c->knownPilgrimageDevices)
									{
										if (kpd->groupId == objective->pilgrimageDeviceGroupId &&
											(!objective->requiresToDepletePilgrimageDevice || !kpd->depleted))
										{
											_c->towardsPilgrimageDevice = kpd->device;
										}
									}
								}
							}
						}
					}
				}
#ifdef AN_OUTPUT_TOWARDS_OBJECTIVE
				else
				{
					output(TXT("[towardsObjective] has no to %S"), _c->at.to_string().to_char());
				}
#endif
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::async_connect_no_cells_rooms_chain()
{
	ASSERT_ASYNC;

	{
		scoped_call_stack_info(TXT("connect noCellsRooms chain"));

		Framework::Door* doorToPrevious = nullptr;

		for_every_ref(noCellsRoom, noCellsRooms)
		{
			int noCellsRoomIdx = for_everys_index(noCellsRoom);

			if (noCellsRoomsConnector.is_index_valid(noCellsRoomIdx))
			{
				doorToPrevious = noCellsRoomsConnector[noCellsRoomIdx].get();
			}
			else
			{
				// previous
				if (noCellsRoomIdx > noCellsRoomsPrevDoorsDoneFor)
				{
					noCellsRoomsPrevDoorsDoneFor = noCellsRoomIdx;
					an_assert((doorToPrevious != nullptr) ^ (noCellsRoomIdx == 0));
					if (doorToPrevious)
					{
						create_and_place_door_in_room(doorToPrevious, noCellsRoom, Tags().set_tag(NAME(dt_noCellsPrevDoor)), NAME(noCellsPrevDoor));
					}
				}

				Framework::Door* doorToNext = nullptr;

				if (noCellsRoomIdx == noCellsRooms.get_size() - 1)
				{
					// this is the currently last available room...
					if (!noCellsRoomsAllGenerated)
					{
						// ...but we still need to create more, wait
						break;
					}
				}

				// next
				{
					if (noCellsRoomIdx == 0 && startingRoom == noCellsRoom && startingRoomExitDoor.get())
					{
						// first room should have door to next, our starting room's exit door
						doorToNext = startingRoomExitDoor.get();
						if (noCellsRoomIdx == noCellsRooms.get_size() - 1)
						{
							// if we're the first and the last, the transition room's entrance should be our starting room exit door, check it!
#ifdef AN_ASSERT
							if (!pilgrimage->open_world__get_definition()->no_transition_door_out_required())
							{
								// last room
								Framework::Door* door = nullptr;
								{
									// get transition room's door
									if (!existingCells.is_empty())
									{
										ExistingCell* newCell = nullptr;
										{
											Concurrency::ScopedMRSWLockWrite lock(existingCellsLock, TXT("make sure we have a cell"));
											newCell = &existingCells.get_first();
										}

										auto* ted = newCell->transitionEntranceDoor.get();
										if (ted)
										{
											door = ted;
										}
									}
									else
									{
										error(TXT("no required transition room entrance door"));
									}
								}
								if (!door)
								{
									error(TXT("no door available"));
								}

								an_assert(startingRoomExitDoor == door);
							}
#endif
						}
						else
						{
							auto* dir = startingRoomExitDoor->get_linked_door_in(noCellsRoom);
							an_assert(dir);
							if (!dir->is_vr_space_placement_set())
							{
								Transform vrAnchor = Transform::identity;
								Transform dirPlacement = Transform::identity;
								Name doorPOIToNext;
								bool found = false;
								{
									if (noCellsRoomIdx == noCellsRooms.get_size() - 1)
									{
										// last room
										doorPOIToNext = NAME(noCellsExitDoor);
										found = get_poi_placement(noCellsRoom, doorPOIToNext, NP, OUT_ vrAnchor, OUT_ dirPlacement, AllowToFail);
									}
									// last room's POI could default to NEXT
									if (!found)
									{
										doorPOIToNext = NAME(noCellsNextDoor);
										found = get_poi_placement(noCellsRoom, doorPOIToNext, NP, OUT_ vrAnchor, OUT_ dirPlacement);
									}
								}
								if (found)
								{
									Transform dirVRPlacement = vrAnchor.to_local(dirPlacement);

									// if it has a fixed vr placement already, grow into a vr corridor
									if (!dir->is_vr_space_placement_set() ||
										!Framework::DoorInRoom::same_with_orientation_for_vr(dir->get_vr_space_placement(), dirVRPlacement))
									{
										warn(TXT("try to place out door in the same spot"));
										if (dir->is_vr_placement_immovable() &&
											dir->is_relevant_for_vr())
										{
											RoomGenerationInfo const& roomGenerationInfo = RoomGenerationInfo::get();
											dir = dir->grow_into_vr_corridor(NP, dirVRPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
										}
										if (!dir->is_vr_placement_immovable())
										{
											dir->set_vr_space_placement(dirVRPlacement);
										}
									}
									dir->make_vr_placement_immovable();
									dir->set_placement(dirPlacement);
								}
							}
							dir->init_meshes();
						}
					}
					else
					{
						Name doorPOIToNext;
						Optional<Name> doorPOIToNextAlt;
						Tags doorTags;
						if (noCellsRoomIdx == noCellsRooms.get_size() - 1)
						{
							if (!pilgrimage->open_world__get_definition()->no_transition_door_out_required())
							{
								// last room
								Framework::Door* door = nullptr;
								{
									// get transition room's door
									if (!existingCells.is_empty())
									{
										ExistingCell* newCell = nullptr;
										{
											Concurrency::ScopedMRSWLockWrite lock(existingCellsLock, TXT("make sure we have a cell"));
											newCell = &existingCells.get_first();
										}

										auto* ted = newCell->transitionEntranceDoor.get();
										if (ted)
										{
											door = ted;
										}
									}
									else
									{
										error(TXT("no required transition room entrance door"));
									}
								}
								if (!door)
								{
									error(TXT("no door available"));
								}
								doorPOIToNext = NAME(noCellsExitDoor);
								doorPOIToNextAlt = NAME(noCellsNextDoor);
								doorToNext = door;
								doorTags.set_tag(NAME(dt_noCellsExitDoor));
								doorTags.set_tag(NAME(dt_noCellsNextDoor));
							}
						}
						else if (pilgrimage->open_world__get_definition()->get_no_cells_rooms().is_index_valid(noCellsRoomIdx))
						{
							doorPOIToNext = NAME(noCellsNextDoor);
							doorTags.set_tag(NAME(dt_noCellsNextDoor));
							Framework::DoorType* doorType = pilgrimage->open_world__get_definition()->get_no_cells_rooms()[noCellsRoomIdx].toNextDoorType.get();
							if (doorType)
							{
								game->perform_sync_world_job(TXT("create door"), [this, doorType, &doorToNext]()
									{
										doorToNext = new Framework::Door(subWorld, doorType);
									});
							}
							else
							{
								error(TXT("no door type provided for noCells room to next noCells room (and not the last one to use from transition"));
							}
						}

						if (doorToNext)
						{
							create_and_place_door_in_room(doorToNext, noCellsRoom, doorTags, doorPOIToNext, doorPOIToNextAlt);
						}
						else if (noCellsRoomIdx < noCellsRooms.get_size() - 1)
						{
							error(TXT("no door to next for noCells room to next noCells room"));
						}
						else if (!pilgrimage->open_world__get_definition()->no_transition_door_out_required())
						{
							error(TXT("no door to transition room and we require it"));
						}
					}
				}

				noCellsRoomsConnector.grow_size(1);
				noCellsRoomsConnector.get_last() = doorToNext;

				doorToPrevious = doorToNext;
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::issue_no_cells_rooms_more_work()
{
	if (Game* game = Game::get_as<Game>())
	{
		RefCountObjectPtr<PilgrimageInstance> keepThis(this);
		game->add_async_world_job(TXT("no cells rooms more work"),
			[keepThis, this, game]()
			{
				if (game->does_want_to_cancel_creation())
				{
					return;
				}

				Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();

				Framework::Room* workingOnRoom = nullptr;

				{
					scoped_call_stack_info(TXT("create noCellsRooms"));

					if (noCellsRooms.get_size() < pilgrimage->open_world__get_definition()->get_no_cells_rooms().get_size())
					{
						int newNoCellRoomIdx = noCellsRooms.get_size();
						auto & ncr = pilgrimage->open_world__get_definition()->get_no_cells_rooms()[newNoCellRoomIdx];
						Framework::Room* room = nullptr;
						Game::get()->perform_sync_world_job(TXT("create noCells chain room"), [this, &room, &ncr]()
							{
								Random::Generator rrg = randomSeed;
								rrg.advance_seed(80235 + noCellsRooms.get_size(), 97523 + noCellsRooms.get_size() * 235);
								room = new Framework::Room(subWorld, mainRegion.get(), ncr.roomType.get(), rrg);
								ROOM_HISTORY(room, TXT("no cells chain room"));
							});
						{
							room->access_tags().set_tag(NAME(noCellsRoom));
							room->access_tags().set_tags_from(ncr.tags);
						}
						noCellsRooms.push_back(SafePtr<Framework::Room>(room));

						room->generate();

						workingOnRoom = room;
					}
				}

				if (workingOnRoom)
				{
					async_connect_no_cells_rooms_chain();
					workingOnRoom->ready_for_game(); // rooms are generated, so we don't have to generate them

					issue_no_cells_rooms_more_work();
				}

				game->request_ready_and_activate_all_inactive(activationGroup.get());
				game->pop_activation_group(activationGroup.get());
			});
	}
}

void PilgrimageInstanceOpenWorld::async_create_cell_at(VectorInt2 const & cellAt)
{
	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

	// it's best to call it right before creating actual cells
	async_add_extra_cell_exits_if_required();

	scoped_call_stack_info_str_printf(TXT("create cell at %ix%i"), cellAt.x, cellAt.y);

	MEASURE_PERFORMANCE(createCell);

	Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output_colour(0, 1, 1, 1);
	output(TXT("create open world cell %ix%i"), cellAt.x, cellAt.y);
	output_colour();
#endif

	bool noCells = pilgrimage->open_world__get_definition()->has_no_cells();

	// tests
#ifdef AN_DEVELOPMENT
	static bool testOnce = true;
	if (testOnce)
	{
		testOnce = false;
#ifdef TEST_CELL_GENERATION_BASIC
		{	// test
			get_cell_at(cellAt + VectorInt2( 0,  0));
			{
				RangeInt2 range(cellAt, cellAt);
				range.expand_by(VectorInt2::one);
				debug_visualise_map(range, false);
			}
			debug_visualise_map(cellAt, false);
			get_cell_at(cellAt + VectorInt2(-1,  1));
			get_cell_at(cellAt + VectorInt2( 1,  1));
			get_cell_at(cellAt + VectorInt2( 0, -1));
			get_cell_at(cellAt + VectorInt2( 0,  1));
			get_cell_at(cellAt + VectorInt2( 0,  2));
			debug_visualise_map(cellAt, false);
			{
				Concurrency::ScopedMRSWLockWrite lock(cellsLock);
				cells.clear();
			}
		}
#endif
#ifdef TEST_CELL_GENERATION_SMALL
		{	// test
			get_cell_at(cellAt + VectorInt2( 0,  0));
			{
				RangeInt2 range(cellAt, cellAt);
				range.expand_by(VectorInt2::one);
			}
			get_cell_at(cellAt + VectorInt2(-1,  1));
			get_cell_at(cellAt + VectorInt2( 1,  1));
			get_cell_at(cellAt + VectorInt2( 0, -1));
			get_cell_at(cellAt + VectorInt2( 0,  1));
			get_cell_at(cellAt + VectorInt2( 0,  2));
			debug_visualise_map(cellAt, false);
			{
				Concurrency::ScopedMRSWLockWrite lock(cellsLock);
				cells.clear();
			}
		}
#endif
#ifndef AN_CLANG
		int dist = TEST_CELL_DIST;
#endif
#ifdef TEST_CELL_GENERATION_BIG
		{
			for_range(int, y, -dist, dist)
			{
				for_range(int, x, -dist, dist)
				{
					get_cell_at(cellAt + VectorInt2(x, y));
				}
			}
			debug_visualise_map(cellAt, false);
			{
				Concurrency::ScopedMRSWLockWrite lock(cellsLock);
				cells.clear();
			}
		}
#endif
#ifdef TEST_CELL_GENERATION_RANDOM
		{
			for_count(int, i, dist * dist * 2)
			{
				{
					Concurrency::ScopedMRSWLockWrite lock(cellsLock);
					cells.clear();
				}
				Array<VectorInt2> ats;
				for_range(int, y, -dist, dist)
				{
					for_range(int, x, -dist, dist)
					{
						ats.push_back(cellAt + VectorInt2(x, y));
					}
				}
				int amount = sqr(dist * 2) * 3 / 10;
				while (!ats.is_empty() && amount > 0)
				{
					int idx = Random::get_int(ats.get_size());
					get_cell_at(ats[idx]);
					ats.remove_fast_at(idx);
					--amount;
				}
				RangeInt2 r(cellAt, cellAt);
				r.expand_by(VectorInt2(dist * 4, dist * 4));
				debug_visualise_map(r, false);
			}
			{
				Concurrency::ScopedMRSWLockWrite lock(cellsLock);
				cells.clear();
			}
		}
#endif
#ifdef SHOW_MAP_BEFORE_WE_START
		if (existingCells.is_empty())
		{
			for_range(int, y, -dist, dist)
			{
				for_range(int, x, -dist, dist)
				{
					get_cell_at(cellAt + VectorInt2(x, y));
				}
			}
			debug_visualise_map(cellAt, false);
			{
				Concurrency::ScopedMRSWLockWrite lock(cellsLock);
				cells.clear();
			}
		}
#endif
#ifdef PRE_GENERATE_MAP
		{
			for_range(int, y, -dist, dist)
			{
				for_range(int, x, -dist, dist)
				{
					get_cell_at(cellAt + VectorInt2(x, y));
#ifdef PRE_GENERATE_MAP_MARK_KNOWN
					mark_cell_known_exits(VectorInt2(x, y));
					mark_cell_known_devices(VectorInt2(x, y));
#endif
				}
			}
		}
#endif
	}
#endif

	creatingOpenWorldCell = cellAt;
#ifdef AN_IMPORTANT_INFO
	output(TXT("create cell at %ix%i"), cellAt.x, cellAt.y);
#endif
	output_dev_info();

#ifdef AN_OUTPUT_DEV_INFO
	{	// output in an easier to load way
		s_a_o(TXT("dev info (pilgrimage and setup)"));
		s_a_o(TXT("  pilgrimage: %S"), pilgrimage.get() ? pilgrimage->get_name().to_string().to_char() : TXT("--"));
		s_a_o(TXT("  region variables:"));
		SimpleVariableStorage regionVariables;
		GameSettings::get().setup_variables(regionVariables);
		LogInfoContext log;
		log.increase_indent();
		log.increase_indent();
		regionVariables.log(log);
		log.decrease_indent();
		log.decrease_indent();
		log.output_to_output();
		{
			String info;
			log.output_to_string(info);
			System::FaultHandler::add_custom_info(info);
		}
	}
#endif

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	::System::TimeStamp timeStamp;
#endif
	
	creatingCellTS.set(::System::TimeStamp());

	game->clear_generation_issues();

	// remove scenery meshes that are far away
	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("remove far scenery meshes"));
		auto* owDef = pilgrimage->open_world__get_definition();
		int radius = owDef->get_background_scenery_cell_distance_visibility();
		int veryFarRadius = max(radius, owDef->get_background_scenery_cell_distance_visibility_very_far());
		int removeBeyond = min(veryFarRadius + 6, veryFarRadius * 2);
		for_every(cell, cells)
		{
			VectorInt2 dist = cell->at - cellAt;
			if (abs(dist.x) > removeBeyond ||
				abs(dist.x) > removeBeyond)
			{
				cell->vistaWindowsSceneryMesh.clear();
				cell->closeSceneryMesh.clear();
				cell->farSceneryMesh.clear();
				cell->medFarSceneryMesh.clear();
				cell->veryFarSceneryMesh.clear();
			}
		}
	}

	// create new region as a separate thing

	ExistingCell* newCell = nullptr;
	// create existing cell, we may need it during generation
	{
		Concurrency::ScopedMRSWLockWrite lock(existingCellsLock, TXT("make sure we have a cell"));
		for_every(c, existingCells)
		{
			if (c->at == cellAt)
			{
				newCell = c;
			}
		}
		an_assert(!newCell); // maybe we should just leave it then?
		if (!newCell)
		{
			existingCells.push_back(ExistingCell());
			newCell = &existingCells.get_last();
			newCell->init(cellAt, *this);
		}
	}

	newCell->beingGenerated = true;

	// get cell from the map
	{
		auto const& cell = get_cell_at(cellAt);
		newCell->Cell::operator =(cell); // copy as an ordinary cell
	}

	Array<Framework::Door*> addDoors;
	Array<Framework::RoomType*> addRoomTypes;

	Tags addRegionGenerationTags;
	{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output(TXT("check for transition room"));
#endif

		// add starting room and connect to dirSpecial (or add a room from a pilgrimage device)
		bool allowTransitionRoom = true;
		bool allowPilgrimageDevices = true;
		if ((! noCells && cellAt == get_start_at()) &&
			startingRoom.get() &&
			(startingRoomExitDoor.get() || pilgrimage->open_world__get_definition()->no_transition_door_out_required()) &&
			!pilgrimage->open_world__get_definition()->no_starting_room_door_out_connection())
		{
			if (startingRoomExitDoor.get())
			{
				set_vr_elevator_altitude_for(startingRoomExitDoor.get(), cellAt);
				addDoors.push_back(startingRoomExitDoor.get());
			}
			allowTransitionRoom = noCells;
			allowPilgrimageDevices = false;
		}
		{
			auto const& cell = get_cell_at(cellAt);
			if (allowTransitionRoom && (noCells || (cell.usesObjective && cell.usesObjective->addTransitionRoom)))
			{
				auto tr = async_create_transition_room_to_next_pilgrimage();
				if (tr.room.is_set())
				{
					newCell->transitionRoom = tr.room.get();

					if (tr.entranceDoor.get() && !pilgrimage->open_world__get_definition()->no_transition_door_out_required())
					{
						set_vr_elevator_altitude_for(tr.entranceDoor.get(), cellAt);
						addDoors.push_back(tr.entranceDoor.get());

						newCell->transitionEntranceDoor = addDoors.get_last();
					}
				}
				allowPilgrimageDevices = false;
			}
			if (allowPilgrimageDevices)
			{
				for_every(pd, cell.knownPilgrimageDevices)
				{
					if (auto* rt = pd->device->get_room_type())
					{
						if (rt->get_as_region_piece())
						{
							addRoomTypes.push_back(rt);
						}
					}
				}
			}
			if (cell.usesObjective)
			{
				if (auto* rt = cell.usesObjective->forceSpecialRoomType.get())
				{
					addRoomTypes.push_back(rt);
				}
				addRegionGenerationTags.set_tags_from(cell.usesObjective->regionGenerationTags);
			}
		}
	}

	if (noCells)
	{
		an_assert((addDoors.get_size() == 1) || pilgrimage->open_world__get_definition()->no_transition_door_out_required(), TXT("if we have more, implement such a case"));
		an_assert((addDoors.get_size() == 1 && addDoors[0] == newCell->transitionEntranceDoor.get()) || pilgrimage->open_world__get_definition()->no_transition_door_out_required(), TXT("this should be our transition door"));
		an_assert(startingRoom.get());

		bool requiresFurtherWork = false;

		// create noCellsRooms
		{
			scoped_call_stack_info(TXT("create noCellsRooms"));

			// first, we should add starting room to the chain
			an_assert(noCellsRooms.is_empty(), TXT("called twice?"));
			noCellsRooms.push_back(startingRoom);

			// now create other rooms
			Optional<int> limit = pilgrimage->open_world__get_definition()->get_limit_no_cells_rooms_pregeneration();
			int generated = -1; // starting room

			for_every(ncr, pilgrimage->open_world__get_definition()->get_no_cells_rooms())
			{
				if (for_everys_index(ncr) == 0)
				{
					// skip the first one as it is the starting room
					continue;
				}

				Framework::Room* room = nullptr;
				Game::get()->perform_sync_world_job(TXT("create noCells chain room"), [this, &room, &ncr]()
					{
						Random::Generator rrg = randomSeed;
						rrg.advance_seed(80235 + noCellsRooms.get_size(), 97523 + noCellsRooms.get_size() * 235);
						room = new Framework::Room(subWorld, mainRegion.get(), ncr->roomType.get(), rrg);
						ROOM_HISTORY(room, TXT("no cells chain room"));
					});
				{
					room->access_tags().set_tag(NAME(noCellsRoom));
					room->access_tags().set_tags_from(ncr->tags);
				}
				noCellsRooms.push_back(SafePtr<Framework::Room>(room));

				room->generate();

				++generated;
				if (limit.is_set() && generated >= limit.get())
				{
					requiresFurtherWork = true;
					break;
				}
			}
		}

		noCellsRoomsAllGenerated = !requiresFurtherWork;

		// connect noCellsRooms chain
		async_connect_no_cells_rooms_chain();

		// ready all rooms to vr
		{
			scoped_call_stack_info(TXT("ready rooms for vr"));

			{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output(TXT("ready for vr no-cells room %S \"%S\" [%S]"),
					startingRoom->get_individual_random_generator().get_seed_string().to_char(),
					startingRoom->get_name().to_char(),
					startingRoom->get_room_type() ? startingRoom->get_room_type()->get_name().to_string().to_char() : TXT("--"));
#endif
				startingRoom->ready_for_game(); // rooms are generated, so we don't have to generate them
			}
		}

		if (requiresFurtherWork)
		{
			issue_no_cells_rooms_more_work();
		}
	}
	// create new cell region
	else if (newCell->set.get_as_any())
	{
		scoped_call_stack_info(TXT("create new cell region"));

		SafePtr<Framework::Region> inHigherRegion = mainRegion;
		{
			if (auto* hlr = async_get_or_create_high_level_region_for(cellAt))
			{
				newCell->belongsToHighLevelRegion = hlr;
				if (hlr->containerRegion.get())
				{
					inHigherRegion = hlr->containerRegion;
				}
			}
		}

		{
			MEASURE_PERFORMANCE(createCellLevelRegion);
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output_colour(0, 1, 1, 0);
			output(TXT("create cell level region..."));
			output_colour();
			::System::TimeStamp ts;
#endif
			cellContext = cellAt;

			if (auto* owDef = pilgrimage->open_world__get_definition())
			{
				auto const& clrSet = owDef->get_cell_level_region_set();

				Random::Generator rg = newCell->randomSeed;

				HighLevelRegionCreator chlr(newCell->at, clrSet, rg, false, true, inHigherRegion.get(), cellAt, NAME(hlRegionPriority), NAME(cellLevelAt), NAME(cellLevelAt2), NAME(cellLevelNotAt), NAME(cellLevelNotAt2));
				chlr.run();
				newCell->cellLevelRegion = chlr.topRegion;
				newCell->containerRegion = chlr.containerRegion;
				newCell->aiManagerRoom = chlr.aiManagerRoom;
				newCell->mainRegion.clear();

				if (newCell->cellLevelRegion.is_set())
				{
					newCell->cellLevelRegion->access_tags().set_tag(NAME(cellLevelRegion));
				}

				if (newCell->containerRegion.get())
				{
					inHigherRegion = newCell->containerRegion;
				}
			}
		}

		MEASURE_PERFORMANCE(createCurrentRegion);
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output_colour(0, 1, 1, 0);
		output(TXT("create new cell region..."));
		output_colour();
		::System::TimeStamp ts;
#endif
		cellContext = cellAt;

		auto* openWorldCellRegionType = pilgrimage->open_world__get_definition()->get_cell_region_type();
		if (openWorldCellRegionType) { openWorldCellRegionType->load_on_demand_if_required(); }
		game->perform_sync_world_job(TXT("create region for new cell"), [this, newCell, openWorldCellRegionType, inHigherRegion]()
		{
			newCell->region = new Framework::Region(subWorld, inHigherRegion.get(), openWorldCellRegionType, newCell->randomSeed);
			newCell->region->set_world_address_id(newCell->get_as_world_address_id());
		});

		newCell->region->access_variables().access<VectorInt2>(NAME(openWorldCellContext)) = cellAt;
		
		if (auto* aci = newCell->additionalCellInfo.get())
		{
			newCell->region->access_variables().set_from(aci->get_parameters());
		}
		newCell->region->run_wheres_my_point_processor_setup();
		newCell->region->generate_environments();

		if (!game->does_want_to_cancel_creation())
		{
			if (auto* owDef = pilgrimage->open_world__get_definition())
			{
				if (auto* vsrt = owDef->get_vista_scenery_room_type())
				{
					vsrt->load_on_demand_if_required();
					Random::Generator useRG = newCell->randomSeed;
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output(TXT("generate vista room using rg %S"), useRG.get_seed_string().to_char());
#endif

					Game::get()->perform_sync_world_job(TXT("create vista room"), [&newCell, &useRG, vsrt]()
						{
							newCell->vistaSceneryRoom = new Framework::Room(newCell->region->get_in_sub_world(), newCell->region.get(),
								vsrt, useRG);
							ROOM_HISTORY(newCell->vistaSceneryRoom, TXT("vista room"));
						});
					newCell->vistaSceneryRoom->generate();
					add_scenery_in_room(newCell->vistaSceneryRoom.get(), true);
				}
			}

			scoped_call_stack_info(TXT("generate region"));

			cellContext = cellAt; // as could be cleared when generating background sceneries

			Random::Generator useRG = newCell->randomSeed;
					
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("generate region using rg %S"), useRG.get_seed_string().to_char());
#endif

			Framework::Region* newRegion = nullptr;
			if (Framework::RegionGenerator::generate_and_add_to_sub_world(subWorld, newCell->region.get(), openWorldCellRegionType, NP, &useRG,
				[this, &useRG, newCell, cellAt, openWorldCellRegionType, addDoors, addRoomTypes, noCells, addRegionGenerationTags](PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Framework::Region* _forRegion)
				{
					_forRegion->access_tags().set_tag(NAME(openWorld));
					_forRegion->set_world_address_id(newCell->get_as_world_address_id());
					_forRegion->access_variables().access<bool>(NAME(openWorld)) = true;
					_forRegion->access_variables().access<Name>(NAME(openWorldDir)) = DirFourClockwise::to_name(newCell->dir);
					_forRegion->access_variables().access<bool>(NAME(openWorldDirUp)) = newCell->dir == DirFourClockwise::Up;
					_forRegion->access_variables().access<bool>(NAME(openWorldDirDown)) = newCell->dir == DirFourClockwise::Down;
					_forRegion->access_variables().access<bool>(NAME(openWorldDirRight)) = newCell->dir == DirFourClockwise::Right;
					_forRegion->access_variables().access<bool>(NAME(openWorldDirLeft)) = newCell->dir == DirFourClockwise::Left;

					_generator.access_generation_context().generationTags = RoomGenerationInfo::get().get_generation_tags();
					_generator.access_generation_context().generationTags.set_tag(NAME(openWorld));
					_generator.access_generation_context().generationTags.set_tags_from(addRegionGenerationTags);

					// check what pilgrimage devices we're going to contain here
					an_assert(newCell->knownPilgrimageDevicesAdded);
					for_every(kpd, newCell->knownPilgrimageDevices)
					{
						if (auto* pd = kpd->device.get())
						{
							_generator.access_generation_context().generationTags.set_tag(NAME(containsPilgrimageDevice));
							if (pd->get_room_type())
							{
								_generator.access_generation_context().generationTags.set_tag(NAME(containsRoomBasedPilgrimageDevice));
							}
							else
							{
								_generator.access_generation_context().generationTags.set_tag(NAME(containsSceneryPilgrimageDevice));
							}
						}
					}

#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (auto* dv = _generator.access_debug_visualiser())
					{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
						dv->activate();
#endif
					}
#endif

					for_every_ptr(rt, addRoomTypes)
					{
						_generator.add_piece(rt->get_as_region_piece(), Framework::RegionGenerator::PieceInstanceData());
					}

					for_every_ptr(addDoor, addDoors)
					{
						newCell->add_door(NP, for_everys_index(addDoor), addDoor);
						if (auto* ldb = addDoor->get_linked_door_b())
						{
							warn(TXT("we should not be connected at this point, but if we are, destroy that door-in-room"));
							game->perform_sync_world_job(TXT("destroy starting/transition room door B"), [ldb]() {delete ldb; });
						}
						PieceCombiner::Connector<Framework::RegionGenerator> const* slot = openWorldCellRegionType->find_inside_slot_connector(NAME(special));
						PieceCombiner::Connector<Framework::RegionGenerator> const* slotDir = openWorldCellRegionType->find_inside_slot_connector(NAME(dirSpecial));
						if (slot && slotDir)
						{
							// addDoor is connected in startingRoom/transitionRoom as A;
							_generator.add_outer_connector(slot, nullptr, Framework::RegionGenerator::ConnectorInstanceData(addDoor), PieceCombiner::ConnectorSide::A);
							_generator.add_outer_connector(slotDir, nullptr, Framework::RegionGenerator::ConnectorInstanceData(addDoor), PieceCombiner::ConnectorSide::A);
						}
						else
						{
							if (!slot)
							{
								error(TXT("region type \"%S\" should handle slot \"%S\" (non directional, not using connectorTag)"), openWorldCellRegionType->get_name().to_string().to_char(), NAME(special).to_char());
							}
							if (!slotDir)
							{
								error(TXT("region type \"%S\" should handle slot \"%S\" (directional, using connectorTag)"), openWorldCellRegionType->get_name().to_string().to_char(), NAME(dirSpecial).to_char());
							}
						}
					}

					// get slots connecting to other cells
					if (!noCells)
					{
						for_count(int, dirIdx, 4)
						{
							if (!newCell->exits[dirIdx])
							{
								// skip, there is no exit in that dir
								continue;
							}
							VectorInt2 dir = DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)dirIdx);
							Name dirSlot;
							Name dirDirSlot;
							Name dirDirSideSlot;
							int side = -1;
							{	// side preferred by the neighbour
								Random::Generator rg = newCell->randomSeed;
								rg.advance_seed(9763 + dirIdx, 97239);
								side = rg.get_bool() ? 1 : -1; // although we should read it from cell
								if (is_cell_address_valid(cellAt))
								{
									auto const& cell = get_cell_at(cellAt + dir);
									if (dirIdx == DirFourClockwise::Up ||
										dirIdx == DirFourClockwise::Down)
									{
										if (cell.sideExitsLeftRight != 0)
										{
											side = cell.sideExitsLeftRight;
										}
									}
									if (dirIdx == DirFourClockwise::Left ||
										dirIdx == DirFourClockwise::Right)
									{
										if (cell.sideExitsUpDown != 0)
										{
											side = cell.sideExitsUpDown;
										}
									}
								}
							}

							switch (dirIdx)
							{
							case 0: dirSlot = NAME(up); dirDirSlot = NAME(dirUp); dirDirSideSlot = side < 0? NAME(dirUpSideLeft) : NAME(dirUpSideRight); break;
							case 1: dirSlot = NAME(right); dirDirSlot = NAME(dirRight); dirDirSideSlot = side < 0 ? NAME(dirRightSideDown) : NAME(dirRightSideUp); break;
							case 2: dirSlot = NAME(down); dirDirSlot = NAME(dirDown); dirDirSideSlot = side < 0 ? NAME(dirDownSideLeft) : NAME(dirDownSideRight); break;
							case 3: dirSlot = NAME(left); dirDirSlot = NAME(dirLeft); dirDirSideSlot = side < 0 ? NAME(dirLeftSideDown) : NAME(dirLeftSideUp); break;
							default: warn(TXT("invalid dir")); break;
							}
							VectorInt2 toCellAt = cellAt + dir;
							if (is_cell_address_valid(cellAt))
							{
								PieceCombiner::Connector<Framework::RegionGenerator> const* slot = openWorldCellRegionType->find_inside_slot_connector(dirSlot);
								PieceCombiner::Connector<Framework::RegionGenerator> const* slotDir = openWorldCellRegionType->find_inside_slot_connector(dirDirSlot);
								PieceCombiner::Connector<Framework::RegionGenerator> const* slotDirSide = openWorldCellRegionType->find_inside_slot_connector(dirDirSideSlot);
								if (slot && slotDir && slotDirSide)
								{
									ExistingCell* toCell = get_existing_cell(toCellAt);
									for (int doorIdx = 0; doorIdx < 1; ++doorIdx)
									{
										Framework::Door* slotDoor = nullptr;
										if (toCell)
										{
											// door to us
											slotDoor = toCell->get_door_to(cellAt, doorIdx);
										}
										if (!slotDoor)
										{
											slotDoor = create_door(slot->def.doorType.get());
										}
										// store slot door
										newCell->add_door(toCellAt, doorIdx, slotDoor);
										if (toCell)
										{
											toCell->add_door(cellAt, doorIdx, slotDoor);
										}

										PieceCombiner::ConnectorSide::Type connectorSideType = PieceCombiner::ConnectorSide::Any;
										{
											if (toCellAt.x < cellAt.x || toCellAt.y < cellAt.y) connectorSideType = PieceCombiner::ConnectorSide::A;
											if (toCellAt.x > cellAt.x || toCellAt.y > cellAt.y) connectorSideType = PieceCombiner::ConnectorSide::B;
											an_assert(connectorSideType != PieceCombiner::ConnectorSide::Any);
										}

										slotDoor->access_variables().access<VectorInt2>(connectorSideType == PieceCombiner::ConnectorSide::A ? NAME(aCell) : NAME(bCell)) = toCellAt;
										slotDoor->access_variables().access<VectorInt2>(connectorSideType == PieceCombiner::ConnectorSide::A ? NAME(bCell) : NAME(aCell)) = cellAt;

										// create both versions, region should decide which one to use
										_generator.add_outer_connector(slot, nullptr, Framework::RegionGenerator::ConnectorInstanceData(slotDoor), connectorSideType);
										_generator.add_outer_connector(slotDir, nullptr, Framework::RegionGenerator::ConnectorInstanceData(slotDoor), connectorSideType);
										_generator.add_outer_connector(slotDirSide, nullptr, Framework::RegionGenerator::ConnectorInstanceData(slotDoor), connectorSideType);
									}
								}
								else
								{
									if (!slot)
									{
										error(TXT("region type \"%S\" should handle slot \"%S\" (non directional, not using connectorTag)"), openWorldCellRegionType->get_name().to_string().to_char(), dirSlot.to_char());
									}
									if (!slotDir)
									{
										error(TXT("region type \"%S\" should handle slot \"%S\" (directional, using connectorTag)"), openWorldCellRegionType->get_name().to_string().to_char(), dirDirSlot.to_char());
									}
									if (!slotDirSide)
									{
										error(TXT("region type \"%S\" should handle slot \"%S\" (directional, using connectorTag)"), openWorldCellRegionType->get_name().to_string().to_char(), dirDirSideSlot.to_char());
									}
								}
							}
						}

						// add actual region or room
						{
							if (auto* rt = newCell->set.regionType.get())
							{
								_generator.add_piece(rt->get_as_region_piece(), Framework::RegionGenerator::PieceInstanceData(newCell->randomSeed));
							}
							else if (auto* rt = newCell->set.roomType.get())
							{
								Random::Generator rg = newCell->randomSeed;
								rg.advance_seed(800834, 108125);
								_generator.add_piece(rt->get_region_piece_for_region_generator(_generator, _forRegion, REF_ rg).get(), Framework::RegionGenerator::PieceInstanceData(newCell->randomSeed));
							}
							else
							{
								error(TXT("couldn't choose region/room for pilgrimage \"%S\""), pilgrimage->get_name().to_string().to_char());
							}
						}
					}

					// may require outer connectors
					{
						auto* library = openWorldCellRegionType->get_library();
						Framework::RegionGenerator::setup_generator(_generator, _forRegion, library, useRG);
					}
				},
				[](Framework::Region* region)
				{
					GameSettings::get().setup_variables(region->access_variables());
				},
				&newRegion, false))
			{
				if (newRegion)
				{
					if (auto* rt = newCell->set.regionType.get())
					{
						newRegion->for_every_region([&newCell, rt](Framework::Region* _region)
							{
								if (_region && _region->get_region_type() == rt)
								{
									newCell->mainRegion = _region;
								}
							});
					}
					if (!newCell->mainRegion.get())
					{
						auto* r = newRegion;
						while (r && !r->get_own_regions().is_empty())
						{
							auto* nr = newRegion->get_own_regions().get_first();
							if (!nr)
							{
								break;
							}
						}
						newCell->mainRegion = r;
					}
				}
			}
		}

		if (!newCell->mainRegion.get())
		{
			newCell->mainRegion = newCell->region;
		}

		cellContext = cellAt;

		if (!game->does_want_to_cancel_creation())
		{
			Framework::Room* startWithRoom = nullptr;
			if (noCells)
			{
				startWithRoom = startingRoom.get();
			}
			else
			{
				Framework::Door* startWithDoor = nullptr;
				{
					scoped_call_stack_info(TXT("setup vr doors and find door to start the generation with"));

					int doorDirMatch = NONE;
					// set vr placement for all doors
					// and find a door to start the generation with
					for_every(door, newCell->doors)
					{
						if (!door->toCellAt.is_set())
						{
							continue;
						}
						if (auto* d = door->door.get())
						{
							int match = NONE;
							if (door->toCellAt.get().y > cellAt.y) match = 4;
							if (door->toCellAt.get().x > cellAt.x) match = 3;
							if (door->toCellAt.get().y < cellAt.y) match = 2;
							if (door->toCellAt.get().x < cellAt.x) match = 1;
							setup_vr_placement_for_door(d, cellAt, door->toCellAt.get() - cellAt);
							if (match > doorDirMatch)
							{
								match = doorDirMatch;
								startWithDoor = door->door.get();
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
								output(TXT("choose door to cell %ix%i"), door->toCellAt.get().x, door->toCellAt.get().y);
#endif
							}
						}
					}

					if (!startWithDoor)
					{
						for_every(door, newCell->doors)
						{
							if (door->toCellAt.is_set())
							{
								continue;
							}
							if (auto* d = door->door.get())
							{
								for_count(int, i, 2)
								{
									if (auto* ld = d->get_linked_door(i))
									{
										if (ld->is_vr_space_placement_set())
										{
											startWithDoor = d;
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
											output(TXT("choose door not to cell"));
#endif
											break;
										}
									}
								}
								if (startWithDoor)
								{
									break;
								}
							}
						}
					}
				}

				an_assert(startWithDoor);
				if (startWithDoor && !game->does_want_to_cancel_creation())
				{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					{
						if (auto* d = startWithDoor)
						{
							output(TXT("choosen door d%p"), d);
							if (auto* da = d->get_linked_door_a())
							{
								if (auto* r = da->get_in_room())
								{
									output(TXT(" a: %S \"%S\" [%S]"),
										r->get_individual_random_generator().get_seed_string().to_char(),
										r->get_name().to_char(),
										r->get_room_type() ? r->get_room_type()->get_name().to_string().to_char() : TXT("--"));
								}
							}
							if (auto* db = d->get_linked_door_b())
							{
								if (auto* r = db->get_in_room())
								{
									output(TXT(" b: %S \"%S\" [%S]"),
										r->get_individual_random_generator().get_seed_string().to_char(),
										r->get_name().to_char(),
										r->get_room_type() ? r->get_room_type()->get_name().to_string().to_char() : TXT("--"));
								}
							}
						}
					}
#endif
					for_count(int, doorIdx, 2)
					{
						if (auto* dir = startWithDoor->get_linked_door(doorIdx))
						{
							if (auto* r = dir->get_in_room())
							{
								if (r->is_in_region(newCell->region.get()))
								{
									startWithRoom = r;
								}
							}
						}
					}
				}
			}

			{
				scoped_call_stack_info(TXT("ready rooms for vr"));

				an_assert(startWithRoom);
				if (startWithRoom)
				{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output(TXT("ready for vr cell %ix%i room %S \"%S\" [%S]"), newCell->at.x, newCell->at.y,
						startWithRoom->get_individual_random_generator().get_seed_string().to_char(),
						startWithRoom->get_name().to_char(),
						startWithRoom->get_room_type()? startWithRoom->get_room_type()->get_name().to_string().to_char() : TXT("--"));
#endif
					startWithRoom->ready_for_game();
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output(TXT("readied for vr cell %ix%i room %S \"%S\" [%S]"), newCell->at.x, newCell->at.y,
						startWithRoom->get_individual_random_generator().get_seed_string().to_char(),
						startWithRoom->get_name().to_char(),
						startWithRoom->get_room_type() ? startWithRoom->get_room_type()->get_name().to_string().to_char() : TXT("--"));
#endif
				}
			}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("spawn pilgrimage devices for %ix%i"), newCell->at.x, newCell->at.y);
#endif
			spawn_pilgrimage_devices(newCell);

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("fill directions/distances for %ix%i"), newCell->at.x, newCell->at.y);
#endif

			// and fill directions/distances
			an_assert(startWithRoom);
			tag_open_world_directions_for_cell(startWithRoom);
			if (GameSettings::get().difficulty.linearLevels)
			{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output(TXT("async hide doors that won't be used for %ix%i"), newCell->at.x, newCell->at.y);
#endif
				async_hide_doors_that_wont_be_used(newCell);
			}
		}

		cellContext.clear();

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output_colour(0, 1, 1, 0);
		output(TXT("generated new cell region in %.2fs"), ts.get_time_since());
		output_colour();
#endif
	}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output_colour(0, 1, 1, 1);
	output(TXT("request ready and activate for creation of cell %ix%i"), newCell->at.x, newCell->at.y);
	output_colour();
#endif

	game->request_ready_and_activate_all_inactive(activationGroup.get());
	game->pop_activation_group(activationGroup.get());

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output_colour(0, 1, 1, 1);
	output(TXT("ready to finish creation of cell %ix%i"), newCell->at.x, newCell->at.y);
	output_colour();
#endif

	{
		async_update_towards_objective(cellAt);
	}

	// issue finishing jobs, they will be done when we finish activation
	{
		newCell->beingGenerated = false;
		if (game->does_want_to_cancel_creation())
		{
			game->block_destroying_world(false);
		}
		else
		{
			RefCountObjectPtr<PilgrimageInstance> keepThis(this);
			newCell->beingActivated = true;

			// at this point we should have all jobs queued and awaiting to be performed
			// to do something right now, we should add async jobs that will do final stuff (like restoring pilgrim devices variables or creating nav mesh)

			game->add_async_world_job(TXT("create nav mesh"),
				[this, keepThis]()
			{
				if (game->is_on_short_pause())
				{
					// we're creating or loading game
					if (auto* navSys = game->get_nav_system())
					{
						navSys->async_perform_all();
					}
				}
			});

			game->add_async_world_job(TXT("restore pilgrim device state variables"),
				[this, keepThis, newCell]()
			{
				// do it post activate as we want to have all sub devices present
				restore_pilgrim_device_state_variables(newCell->at);
			});
			game->add_async_world_job(TXT("update no door markers"),
				[this, keepThis, newCell]()
			{
				async_update_no_door_markers(newCell->at);
			});
			game->add_sync_world_job(TXT("end cell creation"),
				[this, newCell, keepThis
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				, timeStamp
#endif
				]()
			{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output_colour(0, 1, 1, 1);
				output(TXT("created cell %ix%i in %.2fs"), newCell->at.x, newCell->at.y, timeStamp.get_time_since());
				output_colour();
#endif
#ifdef AN_TAG_OPEN_WORLD_ROOMS
				if (newCell->region.get())
				{
					int roomIdx = 0;
					newCell->region->for_every_room([newCell, &roomIdx](Framework::Room* room)
						{
							room->access_tags().set_tag(NAME(openWorldX), newCell->at.x);
							room->access_tags().set_tag(NAME(openWorldY), newCell->at.y);
							room->access_tags().set_tag(NAME(cellRoomIdx), roomIdx);
							++roomIdx;
						});
				}
#endif
#ifdef AN_ALLOW_BULLSHOTS
				if (Framework::BullshotSystem::is_active())
				{
					Framework::BullshotSystem::update_objects();
				}
#endif
				creatingCellTS.clear();
				newCell->beingActivated = false;
				newCell->generated = true;
				isBusyAsync = false;
				workingOnCells = false;
				game->block_destroying_world(false);

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output_colour(0, 1, 1, 1);
				output(TXT("finalised creating cell %ix%i"), newCell->at.x, newCell->at.y);
				output_colour();
#endif
			});
		}
	}
}

void PilgrimageInstanceOpenWorld::add_pilgrimage_devices(Cell* _cell)
{
	an_assert(_cell->knownPilgrimageDevices.is_empty());
	if (!_cell->set.get_as_any())
	{
		return;
	}

	scoped_call_stack_info_str_printf(TXT("add pilgrimage devices at %ix%i"), _cell->at.x, _cell->at.y);

	Random::Generator rg = _cell->randomSeed;
	rg.advance_seed(5078086, 10285);

	auto* owDef = pilgrimage->open_world__get_definition();

	bool allowRandomPilgrimageDevices = true;
	bool allowDistancedPilgrimageDevices = true;

	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		for_every(o, owDef->get_objectives())
		{
			if ((!o->allowRandomPilgrimageDevices || !o->allowDistancedPilgrimageDevices) &&
				is_objective_ok_at(o, _cell->at))
			{
				allowRandomPilgrimageDevices = o->allowRandomPilgrimageDevices;
				allowDistancedPilgrimageDevices = o->allowDistancedPilgrimageDevices;
			}
		}
	}

	// make a list of randomly chosen pilgrimage devices
	if (allowRandomPilgrimageDevices)
	{
		for_every(rpd, owDef->get_random_pilgrimage_devices())
		{
			if (! GameDefinition::check_chosen(rpd->forGameDefinitionTagged))
			{
				continue;
			}
			for_every_ref(pd, rpd->devices)
			{
				if (auto* gd = GameDefinition::get_chosen())
				{
					if (!gd->get_pilgrimage_devices_tagged().check(pd->get_tags()))
					{
						// does not match, skip
						continue;
					}
				}

				if (!pd->get_scenery_type())
				{
					error(TXT("pilgrimage device \"%S\" has no scenery type"), pd->get_name().to_string().to_char());
					// not a thing
					continue;
				}

				// check if forced
				bool forced = false;
				{
					if (auto* owDef = pilgrimage->open_world__get_definition())
					{
						for_every(o, owDef->get_objectives())
						{
							if (!o->forceRandomPilgrimageDeviceTagged.is_empty() &&
								is_objective_ok_at(o, _cell->at))
							{
								if (o->forceRandomPilgrimageDeviceTagged.check(pd->get_tags()))
								{
									forced = true;
								}
							}
						}
					}
				}

				if (forced ||
					(pd->get_open_world_cell_tag_condition().check(_cell->set.get_as_any()->get_tags()) &&
					 rg.get_chance(pd->get_chance())))
				{
					KnownPilgrimageDevice kpd;
					kpd.device = pd;
					kpd.subPriority = rg.get_int(10000);
					_cell->knownPilgrimageDevices.push_back(kpd);
					if (pd->is_special() &&
						pd->is_known_from_start())
					{
						Concurrency::ScopedSpinLock lock(knownSpecialDevicesCellsLock, true);
						knownSpecialDevicesCells.push_back_unique(_cell->at);
					}
				}
			}
		}
	}

	if (allowDistancedPilgrimageDevices)
	{
		for_every(dpd, owDef->get_distanced_pilgrimage_devices())
		{
			if (!GameDefinition::check_chosen(dpd->forGameDefinitionTagged))
			{
				continue;
			}
			auto& tile = get_pilgrimage_devices_tile_at(for_everys_index(dpd), _cell->at);

			for_every(pd, tile.knownPilgrimageDevices)
			{
				if (pd->at == _cell->at)
				{
					// only here, do not place in other cells as it will just result in random access mayhem
					if (pd->device->get_open_world_cell_tag_condition().check(_cell->set.get_as_any()->get_tags()))
					{
						KnownPilgrimageDevice kpd;
						kpd.device = pd->device;
						kpd.subPriority = rg.get_int(10000);
						kpd.groupId = dpd->groupId;
						kpd.useLimit = dpd->useLimit;
						_cell->knownPilgrimageDevices.push_back(kpd);
						if (pd->device->is_special() &&
							pd->device->is_known_from_start())
						{
							Concurrency::ScopedSpinLock lock(knownSpecialDevicesCellsLock, true);
							knownSpecialDevicesCells.push_back_unique(_cell->at);
						}
					}
				}
			}
		}
	}

	// subPriorities already set, sort using priority+subPriority
	sort(_cell->knownPilgrimageDevices);

	// remove doubled slot use
	for(int i = 0; i < _cell->knownPilgrimageDevices.get_size(); ++ i)
	{
		Name pd_i_slot = _cell->knownPilgrimageDevices[i].device->get_open_world_cell_slot();
		Name pd_i_slot_secondary = _cell->knownPilgrimageDevices[i].device->get_open_world_cell_slot_secondary();
		bool pd_i_special = _cell->knownPilgrimageDevices[i].device->is_special();
		bool pd_i_guidance = _cell->knownPilgrimageDevices[i].device->is_guidance_beacon();
		for (int j = i + 1; j < _cell->knownPilgrimageDevices.get_size(); ++j)
		{
			if (pd_i_slot == _cell->knownPilgrimageDevices[j].device->get_open_world_cell_slot() || // same slot
				(pd_i_slot_secondary.is_valid() && pd_i_slot_secondary == _cell->knownPilgrimageDevices[j].device->get_open_world_cell_slot_secondary()) || // same secondary slot
				(pd_i_special && _cell->knownPilgrimageDevices[j].device->is_special()) || // both are special
				(pd_i_guidance && _cell->knownPilgrimageDevices[j].device->is_guidance_beacon())) // both require guidance
			{
				_cell->knownPilgrimageDevices.remove_at(j);
				--j;
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::spawn_pilgrimage_devices(ExistingCell* _cell)
{
	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	an_assert(_cell->pilgrimageDevices.is_empty());
	an_assert(_cell->set.get_as_any());

	Random::Generator rg = _cell->randomSeed;
	rg.advance_seed(5078086, 10285);

	// copy from cell
	{
		Cell* cell = nullptr;
		if (get_cell(_cell->at, cell))
		{
			_cell->pilgrimageDevices.make_space_for(cell->knownPilgrimageDevices.get_size());
			for_every(kpd, cell->knownPilgrimageDevices)
			{
				PilgrimageDeviceInstance pdi;
				pdi.device = kpd->device;
				_cell->pilgrimageDevices.push_back(pdi);
			}
		}
		else
		{
			error(TXT("no cell info %ix%i to spawn pilgrimage devices"), _cell->at.x, _cell->at.y);
		}
	}


#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	output(TXT("pilgrimage device: %i"), _cell->pilgrimageDevices.get_size());
#endif

	Array<RefCountObjectPtr<Framework::PointOfInterestInstance>> pois; // use ref count as we might be dealing with temporary instances (see ModuleAppearance::for_every_point_of_interest)

	for (int pdiIdx = 0; pdiIdx < _cell->pilgrimageDevices.get_size(); ++pdiIdx)
	{
		auto& pdi = _cell->pilgrimageDevices[pdiIdx];
		auto* inRegion = _cell->region.get(); // we need to use region as pilgrimage device room may be created outside mainRegion (!), which makes sense as they are accessible via "special" doors
		auto* mainRegion = _cell->mainRegion.get(); // required to get the values properly
		an_assert(inRegion);
		an_assert(mainRegion);

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output(TXT("try to place pilgrimage device \"%S\""), pdi.device->get_name().to_string().to_char());
#endif

		// find proper POIs
		pois.clear();
		{
			for_every(pdiPOI, pdi.device->get_poi_definitions())
			{
				inRegion->for_every_point_of_interest(pdiPOI->name,
					[pdiPOI, &pois](Framework::PointOfInterestInstance* _fpoi)
					{
						if (pdiPOI->tagged->is_empty() || pdiPOI->tagged->check(_fpoi->get_tags()))
						{
							pois.push_back_unique(RefCountObjectPtr<Framework::PointOfInterestInstance>(_fpoi));
						}
					});
			}
		}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output(TXT("  choose from %i poi(s)"), pois.get_size());
#endif
		bool placedPilgrimageDevice = false;
		if (pois.is_empty())
		{
			if (!pdi.device->may_fail_placing())
			{
				error(TXT("no pois to place pilgrimage device \"%S\" for cell %ix%i, maybe you should disallow it here or provide larger rooms?"), pdi.device->get_name().to_string().to_char(), _cell->at.x, _cell->at.y);
			}
		}
		else
		{
			Framework::Object* spawned = nullptr;

			// spawn device
			{
				pdi.device->get_scenery_type()->load_on_demand_if_required();

				Game::get()->perform_sync_world_job(TXT("spawn via spawn manager"),
					[&pdi, &spawned, pdiIdx, _cell, inRegion]()
					{
						spawned = pdi.device->get_scenery_type()->create(String::printf(TXT("%S [pilgrimage device: %i for %ix%i]"),
							pdi.device->get_scenery_type()->create_instance_name().to_char(), pdiIdx, _cell->at.x, _cell->at.y));
						spawned->init(inRegion->get_in_sub_world());
					});

				{
					mainRegion->collect_variables(spawned->access_variables());
					spawned->access_variables().set_from(pdi.device->get_device_setup_variables());
					spawned->access_individual_random_generator() = rg.spawn();

					spawned->initialise_modules();
				}
			}

			// try to place for a specific number of tries (if fails, fail with just an information, not even a warning
			{
				bool isOk = false;
				int triesLeft = 50;
				while (!isOk && triesLeft > 0 && ! pois.is_empty())
				{
					--triesLeft;

					RefCountObjectPtr<Framework::PointOfInterestInstance> poi;
					{
						int poiIdx = rg.get_int(pois.get_size());
						poi = pois[poiIdx];
						pois.remove_fast_at(poiIdx);
					}

					Transform placeAt = poi->calculate_placement();
					Framework::Room* room = poi->get_room();

					{
						Optional<Transform> goodPlacement = poi->find_placement_for(spawned, room, rg.spawn());

						isOk = goodPlacement.is_set();
						placeAt = goodPlacement.get(placeAt);
					}

					if (isOk)
					{
						Game::get()->perform_sync_world_job(TXT("place spawned"),
						[spawned, room, placeAt]()
						{
							spawned->get_presence()->place_in_room(room, placeAt);
						});
					}
				}

				if (isOk)
				{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output_colour(0, 1, 1, 1);
					output(TXT("  placed pilgrimage device \"%S\" in \"%S\""), spawned->get_name().to_char(),
						spawned->get_presence()->get_in_room()->get_name().to_char());
					output_colour();
#endif
					pdi.object = spawned;

					if (auto* mpd = spawned->get_gameplay_as<ModuleMultiplePilgrimageDevice>())
					{
						mpd->spawn_pilgrimage_devices();
					}

					// as we used this pilgrimage device, remove awaiting devices that share the same slot
					{
						Name pd_i_slot = pdi.device->get_open_world_cell_slot();
						Name pd_i_slot_secondary = pdi.device->get_open_world_cell_slot_secondary();
						for (int j = pdiIdx + 1; j < _cell->pilgrimageDevices.get_size(); ++j)
						{
							if (pd_i_slot == _cell->pilgrimageDevices[j].device->get_open_world_cell_slot() ||
								(pd_i_slot_secondary.is_valid() && pd_i_slot_secondary == _cell->pilgrimageDevices[j].device->get_open_world_cell_slot_secondary()))
							{
								_cell->pilgrimageDevices.remove_at(j);
								--j;
							}
						}
					}
					placedPilgrimageDevice = true;
				}
				else
				{
					error(TXT("couldn't place pilgrimage device \"%S\", maybe you should disallow it here or provide larger rooms?"), spawned->get_name().to_char());
					spawned->destroy();
					spawned = nullptr;
				}
			}
		}
		if (! placedPilgrimageDevice)
		{
			// don't hold empty device
			_cell->pilgrimageDevices.remove_at(pdiIdx);
			--pdiIdx;
		}
	}
}

bool PilgrimageInstanceOpenWorld::get_cell(VectorInt2 const& _at, OUT_ Cell*& c) const
{
	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("get_cell"));
		for_every(_c, cells)
		{
			if (_c->at == _at)
			{
				c = _c;
				break;
			}
		}
	}

	return c;
}

bool PilgrimageInstanceOpenWorld::get_cells(VectorInt2 const& _at, OUT_ Cell*& c, OUT_ ExistingCell*& ec) const
{
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get cells"));
		for_every(_ec, existingCells)
		{
			if (_ec->at == _at)
			{
				ec = cast_to_nonconst(_ec);
				break;
			}
		}
	}
	get_cell(_at, c);

	return ec && c;
}

void PilgrimageInstanceOpenWorld::restore_pilgrim_device_state_variables(VectorInt2 const& _at, Framework::IModulesOwner* _object)
{
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			if (pd->object == _object || _object == nullptr)
			{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
				output(TXT("[PD] restore pilgrimage device state variables %ix%i %S"), _at.x, _at.y, pd->object.get()? pd->object->ai_get_name().to_char() : TXT("no pd"));
#endif
				if (auto* kpd = c->get_known_pilgrimage_device(pd->device.get()))
				{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
					output(TXT("[PD] known pilgrimage device data available"));
#endif
					for_every(varInfo, pd->device->get_state_variables().get_all())
					{
						if (auto* raw = kpd->stateVariables.get_raw(varInfo->get_name(), varInfo->type_id()))
						{
							RegisteredType::copy(varInfo->type_id(), pd->object->access_variables().access_raw(varInfo->get_name(), varInfo->type_id()), raw);
						}
					}

					if (pd->object.get())
					{
						if (auto* mmpd = pd->object->get_gameplay_as<ModuleMultiplePilgrimageDevice>()) // this is a gameplay module!
						{
							mmpd->restore_pilgrim_device_state_variables(kpd->stateVariables);
						}
						if (auto* mpd = pd->object->get_custom<CustomModules::PilgrimageDevice>()) // this is a custom module!
						{
							mpd->restore_pilgrim_device_state_variables(kpd->stateVariables);
						}
					}
				}
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
				else
				{
					output(TXT("[PD] no known pilgrimage device data available"));
				}
#endif
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::store_pilgrim_device_state_variables(VectorInt2 const& _at, Framework::IModulesOwner* _object)
{
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			if (pd->object == _object || _object == nullptr)
			{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
				output(TXT("[PD] store pilgrimage device state variables %ix%i %S"), _at.x, _at.y, pd->object.get() ? pd->object->ai_get_name().to_char() : TXT("no pd"));
#endif
				if (auto* kpd = c->access_known_pilgrimage_device(pd->device.get()))
				{
					if (pd->object.get())
					{
						if (auto* mpd = pd->object->get_custom<CustomModules::PilgrimageDevice>())
						{
							mpd->store_pilgrim_device_state_variables(kpd->stateVariables);
						}
						if (auto* mmpd = pd->object->get_gameplay_as<ModuleMultiplePilgrimageDevice>())
						{
							mmpd->store_pilgrim_device_state_variables(kpd->stateVariables);
						}

						for_every(varInfo, pd->device->get_state_variables().get_all())
						{
							if (auto* raw = pd->object->get_variables().get_raw(varInfo->get_name(), varInfo->type_id()))
							{
								RegisteredType::copy(varInfo->type_id(), kpd->stateVariables.access_raw(varInfo->get_name(), varInfo->type_id()), raw);
							}
						}
					}
				}
			}
		}
	}
}

bool PilgrimageInstanceOpenWorld::is_pilgrim_device_state_visited(VectorInt2 const& _at, Framework::IModulesOwner* _object)
{
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			if (pd->object == _object)
			{
				if (auto* kpd = c->get_known_pilgrimage_device(pd->device.get()))
				{
					return kpd->visited;
				}
				return false;
			}
		}
	}
	return false;
}

bool PilgrimageInstanceOpenWorld::is_pilgrim_device_state_depleted(VectorInt2 const& _at, Framework::IModulesOwner* _object)
{
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (startingRoomUpgradeMachine.get() && startingRoomUpgradeMachine == _object &&
		startingRoomUpgradeMachinePilgrimageDevice.get())
	{
		return upgradeInStartingRoomUnlocked;
	}
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			if (pd->object == _object)
			{
				if (auto* kpd = c->get_known_pilgrimage_device(pd->device.get()))
				{
					return kpd->depleted;
				}
				return false;
			}
		}
	}
	return false;
}

bool PilgrimageInstanceOpenWorld::is_pilgrim_device_state_depleted_long_distance_detectable(VectorInt2 const& _at)
{
	Cell* c = nullptr;
	if (get_cell(_at, c))
	{
		for_every(kpd, c->knownPilgrimageDevices)
		{
			if (auto* pdd = kpd->device.get())
			{
				if (pdd->is_long_distance_detectable())
				{
					return kpd->depleted;
				}
			}
		}
	}
	return false;
}

bool PilgrimageInstanceOpenWorld::is_pilgrim_device_use_exceeded(VectorInt2 const& _at, Framework::IModulesOwner* _object)
{
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			if (pd->object == _object)
			{
				if (auto* kpd = c->get_known_pilgrimage_device(pd->device.get()))
				{
					if (kpd->groupId.is_valid() &&
						kpd->useLimit >= 0)
					{
						Concurrency::ScopedSpinLock lock(pilgrimageDevicesUsageByGroupLock, TXT("check usage"));
						for_every(pdubg, pilgrimageDevicesUsageByGroup)
						{
							if (pdubg->groupId == kpd->groupId)
							{
								if (pdubg->useCount >= kpd->useLimit)
								{
									return true;
								}
							}
						}
					}
				}
				return false;
			}
		}
	}
	return false;
}

void PilgrimageInstanceOpenWorld::mark_pilgrim_device_state_visited(VectorInt2 const& _at, Framework::IModulesOwner* _object)
{
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			bool thisOne = pd->object == _object;
			if (!thisOne)
			{
				if (auto* mmpd = pd->object->get_gameplay_as<ModuleMultiplePilgrimageDevice>()) // this is a gameplay module!
				{
					if (mmpd->is_one_of_pilgrimage_devices(_object))
					{
						thisOne = true;
					}
				}
			}
			if (thisOne)
			{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
				output(TXT("[PD] mark pilgrimage device visited [%ix%i] \"%S\""), _at.x, _at.y, _object->ai_get_name().to_char());
#endif

				if (auto* kpd = c->access_known_pilgrimage_device(pd->device.get()))
				{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
					output(TXT("[PD] accessed device \"%S\""), pd->device.get_name().to_string().to_char());
#endif
					if (!kpd->visited)
					{
						todo_multiplayer_issue(TXT("we just get player here"));
						if (auto* g = Game::get_as<Game>())
						{
							if (auto* pa = g->access_player().get_actor())
							{
								if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
								{
									auto& poi = mp->access_overlay_info();
									if (poi.is_trying_to_show_map())
									{
										poi.new_map_available(); // to update visitation
									}
								}
							}
						}
					}

					kpd->visited = true;

					// store for existing cell too (useful for door directions)
					if (auto* ekpd = ec->access_known_pilgrimage_device(pd->device.get()))
					{
						ekpd->visited = true;
					}

					return;
				}
				else
				{
					an_assert(false, TXT("there should be a device of this name \"%S\""), pd->device.get_name().to_string().to_char());
				}

			}
		}
	}
}

void PilgrimageInstanceOpenWorld::mark_pilgrim_device_state_depleted(VectorInt2 const& _at, Framework::IModulesOwner* _object)
{
	if (startingRoomUpgradeMachine.get() && startingRoomUpgradeMachine == _object &&
		startingRoomUpgradeMachinePilgrimageDevice.get())
	{
		upgradeInStartingRoomUnlocked = true;
		return;
	}
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			bool thisOne = pd->object == _object;
			if (!thisOne)
			{
				if (auto* mmpd = pd->object->get_gameplay_as<ModuleMultiplePilgrimageDevice>()) // this is a gameplay module!
				{
					if (mmpd->is_one_of_pilgrimage_devices(_object))
					{
						thisOne = true;
					}
				}
			}
			if (thisOne)
			{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
				output(TXT("[PD] mark pilgrimage device depleted [%ix%i] \"%S\""), _at.x, _at.y, _object->ai_get_name().to_char());
#endif

				if (auto* kpd = c->access_known_pilgrimage_device(pd->device.get()))
				{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
					output(TXT("[PD] accessed device \"%S\""), pd->device.get_name().to_string().to_char());
#endif
					if (!kpd->depleted)
					{
						todo_multiplayer_issue(TXT("we just get player here"));
						if (auto* g = Game::get_as<Game>())
						{
							if (auto* pa = g->access_player().get_actor())
							{
								if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
								{
									auto& poi = mp->access_overlay_info();
									if (poi.is_trying_to_show_map())
									{
										poi.new_map_available(); // to update depletion
									}
								}
							}
						}
						if (kpd->groupId.is_valid() && kpd->useLimit >= 0)
						{
							Concurrency::ScopedSpinLock lock(pilgrimageDevicesUsageByGroupLock, TXT("store in game state"));
							bool found = false;
							for_every(pdubg, pilgrimageDevicesUsageByGroup)
							{
								if (pdubg->groupId == kpd->groupId)
								{
									++pdubg->useCount;
									found = true;
									break;
								}
							}
							if (!found)
							{
								PilgrimageDevicesUsageByGroup pdubg;
								pdubg.groupId = kpd->groupId;
								++pdubg.useCount;
								pilgrimageDevicesUsageByGroup.push_back(pdubg);
							}
						}
					}

					kpd->depleted = true;

					// store for existing cell too (useful for door directions)
					if (auto* ekpd = ec->access_known_pilgrimage_device(pd->device.get()))
					{
						ekpd->depleted = true;
					}

					Game::get()->add_async_world_job_top_priority(TXT("store variables"), [this, _at, _object]
						{
							store_pilgrim_device_state_variables(_at, _object);
						});
					return;
				}
				else
				{
					an_assert(false, TXT("there should be a device of this name \"%S\""), pd->device.get_name().to_string().to_char());
				}

			}
		}
	}
}

Framework::DoorInRoom* PilgrimageInstanceOpenWorld::find_door_entering_cell(VectorInt2 const& _at, VectorInt2 const& _from)
{
	ASSERT_ASYNC;

	if (auto* ec = get_existing_cell(_at))
	{
		Framework::DoorInRoom* result = nullptr;
		ec->region->for_every_room([_at, _from, &result](Framework::Room* _room)
			{
				if (! result)
				{
					FOR_EVERY_ALL_DOOR_IN_ROOM(dir, _room)
					{
						if (auto* door = dir->get_door())
						{
							auto* aCell = door->get_variables().get_existing<VectorInt2>(NAME(aCell));
							auto* bCell = door->get_variables().get_existing<VectorInt2>(NAME(bCell));
							if (aCell && bCell)
							{
								if ((*aCell == _at && *bCell == _from) ||
									(*bCell == _at && *aCell == _from))
								{
									result = dir;
									return;
								}
							}
						}
					}
				}
			});
		return result;
	}
	return nullptr;
}

void PilgrimageInstanceOpenWorld::issue_unlink_door_to_cell(SafePtr<Framework::Door> const& _door, VectorInt2 const& _intoCell)
{
	if (!_door.get())
	{
		return;
	}

	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_sync_world_job(TXT("unlink door to cell"),
		[keepThis, this, _door, _intoCell]()
		{
			sync_unlink_door_to_cell(_door, _intoCell);
		});
}


void PilgrimageInstanceOpenWorld::sync_unlink_door_to_cell(SafePtr<Framework::Door> const& _door, VectorInt2 const& _intoCell)
{
	ASSERT_SYNC;

	if (auto* d = _door.get())
	{
		Framework::DoorInRoom* unlinkDIR = nullptr;
		if (auto* ec = d->get_variables().get_existing<VectorInt2>(NAME(aCell)))
		{
			if (*ec == _intoCell)
			{
				unlinkDIR = d->get_linked_door_a();
			}
		}
		if (auto* ec = d->get_variables().get_existing<VectorInt2>(NAME(bCell)))
		{
			if (*ec == _intoCell)
			{
				unlinkDIR = d->get_linked_door_b();
			}
		}

		if (unlinkDIR)
		{
			// will unlink and remove from world
			delete unlinkDIR;
		}
		else
		{
			warn(TXT("no door in room marked to be in %ix%i"), _intoCell.x, _intoCell.y);
		}
	}
}

void PilgrimageInstanceOpenWorld::advance(float _deltaTime)
{
	base::advance(_deltaTime);

#ifdef DUMP_CALL_STACK_INFO_IF_CREATING_CELL_TOO_LONG
	if (workingOnCells)
	{
		workingOnCellsTime += _deltaTime;
	}
	else
	{
		workingOnCellsTime = 0.0f;
	}

	{
		static float showCallStackTimeLeft = 0.0f;
		showCallStackTimeLeft -= _deltaTime;
		if (workingOnCellsTime > 20.0f)
		{
			if (showCallStackTimeLeft <= 0.0f)
			{
				showCallStackTimeLeft = workingOnCellsTime > 40.0f? 1.0f : 5.0f;
				output(TXT("[working on cells for too long, periodical call stack info]"));
				output(TXT("%S"), get_call_stack_info().to_char());
			}
		}
	}
#endif

	timeSinceLastCellBlocked += _deltaTime;

	MEASURE_PERFORMANCE(pilgrimage_advance__openworld);
#ifndef BUILD_PUBLIC_RELEASE
#ifdef AN_STANDARD_INPUT
	if (auto* input = ::System::Input::get())
	{
		{
			Optional<VectorInt2> moveBy;
			{
				if (input->is_key_pressed(::System::Key::LeftAlt) &&
					(input->is_key_pressed(::System::Key::LeftCtrl) || input->is_key_pressed(::System::Key::LeftShift)))
				{
					if (input->has_key_been_pressed(::System::Key::UpArrow))
					{
						moveBy = VectorInt2(0, 1);
					}
					if (input->has_key_been_pressed(::System::Key::DownArrow))
					{
						moveBy = VectorInt2(0, -1);
					}
					if (input->has_key_been_pressed(::System::Key::RightArrow))
					{
						moveBy = VectorInt2(1, 0);
					}
					if (input->has_key_been_pressed(::System::Key::LeftArrow))
					{
						moveBy = VectorInt2(-1, 0);
					}
					if (input->has_key_been_pressed(::System::Key::M))
					{
						if (Game* game = Game::get_as<Game>())
						{
							todo_multiplayer_issue(TXT("we just get player here"));
							if (auto* pa = game->access_player().get_actor())
							{
								if (auto* p = pa->get_presence())
								{
									if (auto* r = p->get_in_room())
									{
										Optional<VectorInt2> atCell;
										{
											Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get cell for region"));
											for_every(cell, existingCells)
											{
												if (r->is_in_region(cell->region.get()))
												{
													atCell = cell->at;
												}
											}
										}
										if (atCell.is_set())
										{
											int r = 5;
											RangeInt2 range;
											range.x = RangeInt(atCell.get().x - r, atCell.get().x + r);
											range.y = RangeInt(atCell.get().y - r, atCell.get().y + r);
											debug_visualise_map(atCell.get(), true, atCell.get());
										}
									}
								}
							}
						}
					}
				}
			}
#ifdef RANDOM_WANDERER
			if (Game* game = Game::get_as<Game>())
			{
				todo_multiplayer_issue(TXT("we just get player here"));
				if (auto* pa = game->access_player().get_actor())
				{
					if (!is_creating_or_wants_to_create_cell(pa))
					{
						static VectorInt2 preferred = VectorInt2::zero;
						static int preferredFor = 0;
						static int preferredIdx = 0;
						--preferredFor;
						if (preferredFor <= 0)
						{
							preferredIdx = mod(preferredIdx + 1, 4);
							if (preferredIdx == 1) preferred = VectorInt2(0, 1);
							if (preferredIdx == 2) preferred = VectorInt2(1, 0);
							if (preferredIdx == 3) preferred = VectorInt2(0, -1);
							if (preferredIdx == 0) preferred = VectorInt2(Random::get_chance(0.6f)?1 : -1, 0);

							preferredFor = Random::get_int_from_range(2, preferred.x == 0? 10 : 4);
						}
						moveBy = preferred;
					}
				}
			}
#endif
#ifdef AN_ALLOW_AUTO_TEST
			if (checkAllCells_active)
			{
				moveBy = checkAllCells_moveBy;
				if (checkAllCells_processAllJobs)
				{
					if (Game* game = Game::get_as<Game>())
					{
						if (game->has_any_delayed_object_creation_pending())
						{
							moveBy.clear();
						}
						if (auto* ns = game->get_nav_system())
						{
							if (ns->get_pending_build_tasks_count() ||
								ns->is_performing_task_or_queued())
							{
								moveBy.clear();
							}
						}
					}
				}
			}
#endif
			if (moveBy.is_set() && !moveBy.get().is_zero())
			{
				if (Game* game = Game::get_as<Game>())
				{
					todo_multiplayer_issue(TXT("we just get player here"));
					if (auto* pa = game->access_player().get_actor())
					{
						if (auto* p = pa->get_presence())
						{
							if (auto* r = p->get_in_room())
							{
								Optional<VectorInt2> atCell;
								{
									Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get cell for region"));
									for_every(cell, existingCells)
									{
										if (r->is_in_region(cell->region.get()))
										{
											atCell = cell->at;
										}
									}
								}
								if (atCell.is_set())
								{
									if (auto* ec = get_existing_cell(atCell.get() + moveBy.get()))
									{
										if (ec->generated)
										{
#ifdef AN_ALLOW_AUTO_TEST
											checkAllCells_moveBy = VectorInt2::zero;
#endif
											bool placed = false;
											/*	this is accurate, but it's better to get any room really as it might be the first/most important one, so we can test it better
											if (!placed)
											{
												if (auto* d = ec->get_door_to(atCell.get(), 0))
												{
													auto* dir = (moveBy.get().x > 0 || moveBy.get().y > 0) ? d->get_linked_door_b() : d->get_linked_door_a();
													if (dir)
													{
														if (auto* room = dir->get_in_room())
														{
															Transform placement = dir->get_inbound_matrix().to_transform();
															placement.set_translation(placement.get_translation() + placement.get_axis(Axis::Forward) * 0.25f);
															p->request_teleport(room, placement);
															pa->get_controller()->set_requested_look_orientation(placement.get_orientation());
															placed = true;
														}
													}
												}
											}
											*/
											if (!placed)
											{
												// find within the first room as most likely it is the most interesting room
												// if we have that room, find a door from which we came
												if (auto* r = ec->region.get())
												{
													if (auto* room = r->get_any_room(false, &okRoomToSpawnTagCondition))
													{
														Optional<DirFourClockwise::Type> dirFrom = DirFourClockwise::vector_int2_to_dir(-moveBy.get());
														if (dirFrom.is_set())
														{
															Name distTag;
															switch (dirFrom.get())
															{
																case DirFourClockwise::Up: distTag = NAME(openWorldCellDistFromUp); break;
																case DirFourClockwise::Right: distTag = NAME(openWorldCellDistFromRight); break;
																case DirFourClockwise::Down: distTag = NAME(openWorldCellDistFromDown); break;
																case DirFourClockwise::Left: distTag = NAME(openWorldCellDistFromLeft); break;
																default: break;
															}
															if (distTag.is_valid())
															{
																Framework::DoorInRoom* bestDir = nullptr;
																float bestDist = 0.0f;
																for_every_ptr(dir, room->get_all_doors())
																{
																	// invisible doors are fine
																	float tagValue = dir->get_tags().get_tag(distTag);
																	if (tagValue > 0 &&
																		(!bestDir || tagValue < bestDist))
																	{
																		bestDir = dir;
																		bestDist = tagValue;
																	}
																}
																if (bestDir)
																{
																	Transform placement = bestDir->get_inbound_matrix().to_transform();
																	placement.set_translation(placement.get_translation() + placement.get_axis(Axis::Forward) * 0.25f);
																	p->request_teleport(room, placement);
																	pa->get_controller()->set_requested_look_orientation(placement.get_orientation());
																	placed = true;
																}
															}
														}
													}
												}
											}
											if (!placed)
											{
												if (auto* r = ec->region.get())
												{
													if (auto* room = r->get_any_room(false, &okRoomToSpawnTagCondition))
													{
														Transform placement = Transform::identity;
														if (!room->get_doors().is_empty())
														{
															placement = room->get_doors().get_first()->get_inbound_matrix().to_transform();
															placement.set_translation(placement.get_translation() + placement.get_axis(Axis::Forward) * 0.25f);
														}
														p->request_teleport(room, placement);
														pa->get_controller()->set_requested_look_orientation(placement.get_orientation());
														placed = true;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		/*
		if (input->is_key_pressed(::System::Key::F))
		{
			if (Game* game = Game::get_as<Game>())
			{
				if (auto* pa = game->access_player().get_actor())
				{
					if (auto* p = pa->get_presence())
					{
						if (auto* r = p->get_in_room())
						{
							for_every(cell, existingCells)
							{
								if (!r->is_in_region(cell->region.get()))
								{
									issue_destroy_cell(cell->at);
									return;
								}
							}
						}
					}
				}
			}
			return;
		}
		*/
	}
#endif
#endif

	if (auto* srum = startingRoomUpgradeMachine.get())
	{
		if (srum->get_variables().get_value<bool>(NAME(upgradeMachineShouldRemainDisabled), false))
		{
			upgradeInStartingRoomUnlocked = true;
		}
		else
		{
			upgradeInStartingRoomUnlocked = srum->get_variables().get_value<int>(NAME(chosenOption), 0) != 0;
		}
	}
	else
	{
		startingRoomUpgradeMachine.clear();
	}

	Optional<bool> onMap;
	manageCellsTimeLeft = max(0.0f, manageCellsTimeLeft - _deltaTime);
	if (manageCellsTimeLeft == 0.0f && allowManagingCells)
	{
		MEASURE_PERFORMANCE(manage_cells);
		manageCellsTimeLeft = 0.65f; // no need to do it more often
		// check if there are cells adjacent to the one the player is in
		// also check if we should update objectives
		// if we're working on cells, no need to manage them, but we still need to update objectives
		if (Game* game = Game::get_as<Game>())
		{
			if (auto* pa = game->access_player().get_actor())
			{
				onMap = manage_cells_and_objectives_for(pa, workingOnCells);
			}
		}
	}
	if (Game* game = Game::get_as<Game>())
	{
		if (auto* pa = game->access_player().get_actor())
		{
			update_pilgrim_cell_at(pa);
		}
	}

	if (onMap.is_set() &&
		onMap.get())
	{
		MEASURE_PERFORMANCE(findCellToDestroy);
		int eldestIdx = NONE;
		float eldestTime = 0.0f;
		{
			Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("find eldest cell to destroy"));
			for_every(ec, existingCells)
			{
				ec->timeSinceImportant += _deltaTime;
				float timeToDestroy = 30.0f;
#ifdef AN_DEVELOPMENT
				timeToDestroy = 10.0f;
#endif
				if (ec->timeSinceImportant > timeToDestroy)
				{
					if (existingCells.get_size() > 4)
					{
						issue_destroy_cell(ec->at);
						eldestIdx = NONE;
						break;
					}
				}
				else if (eldestIdx == NONE || eldestTime < ec->timeSinceImportant)
				{
					eldestIdx = for_everys_index(ec);
					eldestTime = ec->timeSinceImportant;
				}
			}
		}
		if (moreExistingCellsNeeded && eldestIdx != NONE)
		{
			issue_destroy_cell(existingCells[eldestIdx].at);
		}
	}

	// update pilgrim's destination
	{
		MEASURE_PERFORMANCE(updatePilgrimsDestination);
		updatePilgrimsDestinationTimeLeft -= _deltaTime;
		if (updatePilgrimsDestinationTimeLeft <= 0.0f || forceUpdatePilgrimsDestination)
		{
			updatePilgrimsDestinationTimeLeft = 0.7f;
			if (Game* game = Game::get_as<Game>())
			{
				// this is okay to get only the local player as we want to have the destination just for this one
				if (auto* pa = game->access_player().get_actor())
				{
					if (auto* p = pa->get_presence())
					{
						if (auto* r = p->get_in_room())
						{
							// update if in the same room to avoid going out to hear beep and it disappearing due to updatePilgrimsDestinationTimeLeft
							if (pilgrimsDestinationDidForRoom != r ||
								pilgrimsDestination.get() == r ||
								forceUpdatePilgrimsDestination)
							{
								forceUpdatePilgrimsDestination = false;
								pilgrimsDestinationDidForRoom = r;

								bool foundGuidanceBeacon = false;
								{
									Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("look for guidance beacon"));
									for_every(cell, existingCells)
									{
										if (r->is_in_region(cell->region.get()))
										{
											for_every(pdi, cell->pilgrimageDevices)
											{
												if (pdi->device->is_guidance_beacon())
												{
													bool validGuidance = true;
													Name pdSystem = pdi->device->get_system();
													if (pdi->device->is_guidance_beacon_only_if_not_depleted())
													{
														if (auto* kpd = cell->get_known_pilgrimage_device(pdi->device.get()))
														{
															if (kpd->depleted)
															{
																// skip
																validGuidance = false;
															}
														}
													}
													if (validGuidance)
													{
														auto* targetObject = pdi->object.get();
														if (targetObject)
														{
															if (auto* mpd = targetObject->get_custom<CustomModules::PilgrimageDevice>()) // this is a custom module!
															{
																if (auto* guideTowards = mpd->get_guide_towards())
																{
																	targetObject = guideTowards;
																}
															}
															if (auto* inRoom = targetObject->get_presence()->get_in_room())
															{
																pilgrimsDestination = inRoom;
																pilgrimsDestinationDevice = pdi->device.get();
																pilgrimsDestinationDeviceMainIMO = pdi->object.get();
																pilgrimsDestinationSystem = pdSystem;
																foundGuidanceBeacon = true;
																break;
															}
														}
													}
												}
											}
											break;
										}
									}
								}
								if (!foundGuidanceBeacon)
								{
									pilgrimsDestination.clear();
									pilgrimsDestinationDevice = nullptr;
									pilgrimsDestinationDeviceMainIMO.clear();
									pilgrimsDestinationSystem = Name::invalid();
								}
							}
						}
					}
				}
			}
		}
	}

#ifndef AN_DEVELOPMENT
	if (creatingCellTS.is_set())
	{
		if (creatingCellTS.get().get_time_since() > 60.0f)
		{
			creating_cell_for_too_long();
		}
	}
#endif
}

void PilgrimageInstanceOpenWorld::creating_cell_for_too_long()
{
	error_dev_investigate(TXT("creating cell for too long %.1fs"), creatingCellTS.is_set()? creatingCellTS.get().get_time_since() : 0.0f);
#ifdef AN_DEVELOPMENT_OR_PROFILER
	AN_BREAK;
#else
	static bool sentOnce = false;
	if (!sentOnce)
	{
		String shortText = String::printf(TXT("creating cell for too long %.1fs"), creatingCellTS.is_set() ? creatingCellTS.get().get_time_since() : 0.0f);
		String moreInfo;
		{
			moreInfo += TXT("creating cell");
			List<String> list = build_dev_info();
			for_every(line, list)
			{
				moreInfo += *line + TXT("\n");
			}
		}
		ReportBug::send_quick_note_in_background(shortText, moreInfo);
		sentOnce = true;
	}
#endif
	creatingCellTS.clear(); // no longer end here
}

void PilgrimageInstanceOpenWorld::post_create_next_pilgrimage()
{
	base::post_create_next_pilgrimage();

	// only after we create next cell
	allowManagingCells = true;
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool PilgrimageInstanceOpenWorld::is_creating_or_wants_to_create_cell(Framework::IModulesOwner* imo)
{
	if (workingOnCells)
	{
		return true;
	}

	Optional<VectorInt2> atCell;
	if (auto* p = imo->get_presence())
	{
		if (auto* r = p->get_in_room())
		{
			Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get cell by region [is_creating_or_wants_to_create_cell]"));
			for_every(cell, existingCells)
			{
				if (r->is_in_region(cell->region.get()))
				{
					atCell = cell->at;
				}
			}
		}
	}

	if (atCell.is_set())
	{
		if (auto* atec = get_existing_cell(atCell.get()))
		{
			atec->timeSinceImportant = 0.0f;
			for_count(int, d, 4)
			{
				if (atec->exits[d])
				{
					VectorInt2 at = atCell.get() + DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)d);
					if (auto* ec = get_existing_cell(at))
					{
						ec->timeSinceImportant = 0.0f;
					}
					else
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}
#endif

void PilgrimageInstanceOpenWorld::update_pilgrim_cell_at(Framework::IModulesOwner* imo)
{
	MEASURE_PERFORMANCE(updatePilgrimCellAt);

	Framework::Room* imoInRoom = nullptr;
	if (auto* p = imo->get_presence())
	{
		if (auto* r = p->get_in_room())
		{
			imoInRoom = r;
		}
	}

	if (imoInRoom && imoInRoom != pilgrimInRoom.get())
	{
		pilgrimInRoom = imoInRoom;
		Optional<VectorInt2> newCellAt = find_cell_at(imo);
		if (newCellAt.is_set())
		{
			if (!pilgrimAt.is_set() ||
				newCellAt.get() != pilgrimAt.get())
			{
				pilgrimComingFrom = pilgrimAt;
				pilgrimAt = newCellAt;
#ifdef AN_IMPORTANT_INFO
				if (pilgrimComingFrom.is_set())
				{
					output(TXT("pilgrim coming from %ix%i to %ix%i"),
						pilgrimComingFrom.get().x, pilgrimComingFrom.get().y,
						pilgrimAt.get().x, pilgrimAt.get().y);
				}
				else
				{
					output(TXT("pilgrim coming to %ix%i"),
						pilgrimAt.get().x, pilgrimAt.get().y);
				}
#endif
				clear_last_visited_haven(); // our new placement is more important
				// check skipCompleteOn for objective
				if (currentObjectiveIdx.is_set())
				{
					if (auto* owDef = pilgrimage->open_world__get_definition())
					{
						if (owDef->get_objectives().is_index_valid(currentObjectiveIdx.get()))
						{
							auto& obj = owDef->get_objectives()[currentObjectiveIdx.get()];
							if (is_objective_ok(&obj))
							{
								bool skipComplete = false;
								skipComplete |= obj.skipCompleteOn.reachedX.is_set() &&
												pilgrimAt.get().x == obj.skipCompleteOn.reachedX.get();
								skipComplete |= obj.skipCompleteOn.reachedY.is_set() &&
												pilgrimAt.get().y == obj.skipCompleteOn.reachedY.get();
								if (skipComplete)
								{
#ifdef AN_IMPORTANT_INFO
									output(TXT("skip complete"));
#endif
									next_objective();
								}
							}
						}
					}
				}
			}
		}
		if (imoInRoom->get_tags().get_tag(NAME(checkpointRoom)) &&
			pilgrimAt.is_set())
		{
			if (auto* crt = imoInRoom->get_variables().get_existing<TagCondition>(NAME(checkpointRoomTagged)))
			{
				if (auto* cell = get_existing_cell(pilgrimAt.get()))
				{
					cell->checkpointRoomTagged = *crt;
					clear_last_visited_haven();
				}
			}
		}
		{
			bool noViolenceRequired = false;
			if (pilgrimAt.is_set())
			{
				if (auto* cell = get_existing_cell(pilgrimAt.get()))
				{
					if (cell->usesObjective)
					{
						noViolenceRequired = cell->usesObjective->beingInCellRequiresNoViolence;
					}
				}
			}
			if (auto* gd = GameDirector::get()) // ignore being active
			{
				gd->game_set_objective_requires_no_voilence(noViolenceRequired);
			}
		}
	}
}

bool PilgrimageInstanceOpenWorld::manage_cells_and_objectives_for(Framework::IModulesOwner* imo, bool _justObjectives)
{
	if (!is_current())
	{
		// wait till we switch, the switch should be a quick one
		return false;
	}

	Optional<VectorInt2> atCell;
	Framework::Room* imoInRoom = nullptr;
	if (auto* p = imo->get_presence())
	{
		if (auto* r = p->get_in_room())
		{
			imoInRoom = r;
			Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get cell by region [manage_cells_and_objectives_for]"));
			for_every(cell, existingCells)
			{
				if (! cell->beingGenerated && r->is_in_region(cell->region.get()))
				{
					atCell = cell->at;
				}
			}
		}
	}

	bool onMap = false;
	if (atCell.is_set())
	{
#ifdef AN_ALLOW_AUTO_TEST
		if (!_justObjectives && checkAllCells_active)
		{
			if (! game->get_nav_system() ||
				game->get_nav_system()->get_pending_build_tasks_count() == 0)
			{
				VectorInt2 genAt = atCell.get();
				auto* owDef = pilgrimage->open_world__get_definition();
				RangeInt2 size = owDef->get_size();

				if (checkAllCells_goingSideFrom.is_set())
				{
					if (checkAllCells_goingSideFrom.get() == atCell.get())
					{
						if (checkAllCells_goingUp)
						{
							genAt.x = -abs(genAt.x); // switch from x to -x
						}
						else
						{
							genAt.x = abs(genAt.x) + 1; // switch from -x to x+1
						}
						if (!size.x.is_empty())
						{
							if (genAt.x > size.x.max ||
								genAt.x < size.x.min)
							{
								game->mark_nothing_to_auto_test();
							}
						}
					}
					else
					{
						checkAllCells_goingSideFrom.clear();
					}
				}
				else if (checkAllCells_goingUp)
				{
					if (! size.y.is_empty() && genAt.y >= size.y.max)
					{
						checkAllCells_goingUp = false;
						checkAllCells_goingSideFrom = atCell;
					}
					else
					{
						genAt.y += 1;
					}
				}
				else
				{
					if (!size.y.is_empty() && genAt.y <= size.y.min)
					{
						checkAllCells_goingUp = true;
						checkAllCells_goingSideFrom = atCell;
					}
					else
					{
						genAt.y -= 1;
					}
				}
				checkAllCells_moveBy = genAt - atCell.get();
				if (!checkAllCells_moveBy.is_zero())
				{
					while (true)
					{
						auto& c = get_cell_at(genAt);
						bool anyExits = false;
						for_count(int, i, DirFourClockwise::NUM)
						{
							anyExits |= c.exits[i];
						}
						if (anyExits)
						{
							break;
						}
						else
						{
							genAt.y += (checkAllCells_goingUp? 1 : -1);
							checkAllCells_moveBy = genAt - atCell.get();
						}
					}
					if (auto* atec = get_existing_cell(genAt))
					{
						// allow to move there
						return true;
					}
					issue_create_cell(genAt, true);
				}
			}
			return true;
		}
#endif
		if (auto* atec = get_existing_cell(atCell.get()))
		{
			atec->timeSinceImportant = 0.0f;
			if (hasMoreObjectives.get(true))
			{
				if (atec->towardsObjectiveIdx.is_set() &&
					currentObjectiveIdx.is_set() &&
					atec->towardsObjectiveIdx.get() == currentObjectiveIdx.get())
				{
					if (!atec->towardsObjective.is_set() ||
						atec->towardsPilgrimageDevice.is_set())
					{
						bool nextObjective = true;
						// we're in the final cell of our current objective
						if (atec->towardsPilgrimageDevice.is_set())
						{
							// check if we're in the right room
							nextObjective = false;
							if (imoInRoom)
							{
								PilgrimageOpenWorldDefinition::Objective const* objective = nullptr;
								auto* owDef = pilgrimage->open_world__get_definition();
								if (currentObjectiveIdx.is_set() && owDef->get_objectives().is_index_valid(currentObjectiveIdx.get()))
								{
									objective = &owDef->get_objectives()[currentObjectiveIdx.get()];
								}

								for_every(pd, atec->pilgrimageDevices)
								{
									if (pd->device == atec->towardsPilgrimageDevice &&
										pd->object.is_set())
									{
										if (pd->object->get_presence()->get_in_room() == imoInRoom)
										{
											// check if the objective requires the device to be depleted, if so, we need to have it depleted
											if (objective && objective->requiresToDepletePilgrimageDevice)
											{
												bool depleted = false;
												for_every(kpd, atec->knownPilgrimageDevices)
												{
													if (kpd->depleted &&
														kpd->device == pd->device)
													{
														depleted = true;
														break;
													}
												}
												if (! depleted) // we require it to be depleted to advance!
												{
													continue;
												}
											}
											nextObjective = true;
										}
									}
								}
							}
						}
						if (nextObjective)
						{
							output(TXT("[objective] in pilgrimage device room or at intermediate objective"));
							next_objective_internal();
						}
					}
				}
			}
			if (!_justObjectives)
			{
				if (cellsHandledAt.is_set() && cellsHandledAt.get() != atCell.get())
				{
					cellsHandledAt.clear();
				}
				if (! cellsHandledAt.is_set() && ! workingOnCells)
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					wantsToCreateCellAt.clear();
#endif
					bool requiresNoDoorMarkerUpdate = false;
					cellsHandledAt = atCell;
					Optional<DirFourClockwise::Type> dirTowardsObjective;
					ArrayStatic<VectorInt2, 8> wantsCellsAt;
					// 0 towards objective
					// 1-4 surrounding dirs
					// 5 one after objective, only if path marked
					for_count(int, id, 6)
					{
#ifdef AN_DEVELOPMENT
#ifdef AN_ALLOW_BULLSHOTS
						if (Framework::BullshotSystem::is_active())
						{
							// don't create neighbour cells
							break;
						}
#endif
#endif
						Optional<VectorInt2> at;
						if (id <= 4)
						{
							Optional<DirFourClockwise::Type> d;
							if (id == 0) // towards objective
							{
								// cached/existing
								{
									Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("find objective to generate [manage_cells_and_objectives_for]"));

									for_every(cell, cells)
									{
										if (cell->at == atCell.get())
										{
											d = cell->towardsObjective;
											dirTowardsObjective = d;
											break;
										}
									}
								}
							}
							else if (id >= 1 && id <= 4) // surrounding dirs
							{
								if (GameSettings::get().difficulty.linearLevels)
								{
									continue;
								}
								// all around
								d = (DirFourClockwise::Type)(id - 1);
							}
							if (d.is_set() &&
								atec->exits[d.get()])
							{
								at = atCell.get() + DirFourClockwise::dir_to_vector_int2(d.get());
							}
						}
						else
						{
							// the next one after the objective, only if path only marked
							if (! GameSettings::get().difficulty.showDirectionsOnlyToObjective ||
								! dirTowardsObjective.is_set())
							{
								continue;
							}
							VectorInt2 atTowardsObjective = atCell.get() + DirFourClockwise::dir_to_vector_int2(dirTowardsObjective.get());
							// cached/existing
							{
								Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("find objective to generate [manage_cells_and_objectives_for]"));

								for_every(cell, cells)
								{
									if (cell->at == atTowardsObjective)
									{
										auto const & nextD = cell->towardsObjective;
										if (nextD.is_set())
										{
											at = atTowardsObjective + DirFourClockwise::dir_to_vector_int2(nextD.get());
										}
										break;
									}
								}
							}
						}
						if (at.is_set())
						{
							if (auto* ec = get_existing_cell(at.get()))
							{
								// keep it!
								ec->timeSinceImportant = 0.0f;
							}
							else
							{
								wantsCellsAt.push_back(at.get());
							}
						}
					}
					for_every(pAt, wantsCellsAt)
					{
						VectorInt2 at = *pAt;
						bool shouldCreate = true;
						if (auto* owDef = pilgrimage->open_world__get_definition())
						{
							if (auto* blockAway = owDef->get_block_away_from_objective())
							{
								Optional<DirFourClockwise::Type> useDirTowardsObjective = dirTowardsObjective;
								/*
								bool const alwaysAllowForward = false; // allow only going towards the target
								if (alwaysAllowForward)
								{
									if (!useDirTowardsObjective.is_set() &&
										owDef->get_objectives_general_dir().is_set())
									{
										useDirTowardsObjective = owDef->get_objectives_general_dir();
									}
								}
								*/
								// so we always have a dir to follow
								if (useDirTowardsObjective.is_set())
								{
									VectorInt2 cellTowardsObjective = atCell.get() + DirFourClockwise::dir_to_vector_int2(useDirTowardsObjective.get());
									if (owDef->get_objectives_general_dir().is_set())
									{
										VectorInt2 fromStartAt = at - get_start_at();
										VectorInt2 fromStartNow = atCell.get() - get_start_at();
										VectorInt2 genObjDir = DirFourClockwise::dir_to_vector_int2(owDef->get_objectives_general_dir().get());
										int distAt = VectorInt2::dot(fromStartAt, genObjDir);
										int distNow = VectorInt2::dot(fromStartNow, genObjDir);

										if (blockAway->blockBehind.is_set())
										{
											if (distNow > blockedAwayCells + blockAway->blockBehind.get())
											{
												blockedAwayCells = distNow - blockAway->blockBehind.get();
												timeSinceLastCellBlocked = 0.0f;
											}
										}

										blockedAwayCellsDistance = distNow - blockedAwayCells;
										if (blockAway->shouldBlockHostileSpawnsAtBlockedAwayCells.get(false))
										{
											blockedAwayCellsDistanceBlockHostiles = blockedAwayCellsDistance <= 0;
										}
										else
										{
											blockedAwayCellsDistanceBlockHostiles = false;
										}

										// check only to sides or away
										if (distAt <= distNow)
										{
											auto timePerCell = blockAway->timePerCell;
											if (timePerCell.is_set())
											{
												while (timeSinceLastCellBlocked >= timePerCell.get())
												{
													if (blockedAwayCells < distNow)
													{
														++blockedAwayCells;
														timeSinceLastCellBlocked -= timePerCell.get();
														blockedAwayCellsDistance = distNow - blockedAwayCells;
													}
													else
													{
														// stay here for a bit
														timeSinceLastCellBlocked = timePerCell.get();
														break;
													}
												}
											}
											else
											{
												todo_implement;
											}
											if (distAt <= distNow &&
												distAt <= blockedAwayCells)
											{
												shouldCreate = false;
											}
										}
									}
									else
									{
										todo_implement;
									}

									if (at == cellTowardsObjective)
									{
										// this is checked here as we want to update blockedAwayCells and timeSinceLastCellBlocked as soon as possible
										// we also want to enforce creating cell in this dir
										shouldCreate = true;
									}
								}
								else
								{
									// wait!
									shouldCreate = false;
									cellsHandledAt.clear(); // wait till we get it
								}
							}
						}
						for_every(door, atec->doors)
						{
							if (door->toCellAt == at)
							{
								if (door->cellWontBeCreated != (! shouldCreate))
								{
									requiresNoDoorMarkerUpdate = true;
								}
								door->cellWontBeCreated = !shouldCreate;
							}
						}
						if (shouldCreate)
						{
							// check if we should generate this, if we're towards the objective and general direction
							issue_create_cell(at);
							cellsHandledAt.clear(); // we still should check one more time
						}
					}
					if (requiresNoDoorMarkerUpdate)
					{
						RefCountObjectPtr<PilgrimageInstance> keepThis(this);
						game->add_async_world_job(TXT("async_update_no_door_markers"), [this, keepThis, atCell]() {  async_update_no_door_markers(atCell.get()); });
					}
				}
			}
		}

		onMap = true;
	}
	else if (! _justObjectives &&
			 imo && imo->get_presence()->get_in_room() == get_starting_room()) // it's sort of a fail-safe for when the starting cell has not been triggered to generate and we are in this pilgrimage's starting room
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		wantsToCreateCellAt.clear();
#endif
		ArrayStatic<VectorInt2, 16> addAt;
		Optional<VectorInt2> startAt;
		{
			startAt = get_start_at();
			addAt.push_back(startAt.get());
		}
		for_every(at, pilgrimage->open_world__get_definition()->get_auto_generate_at_start())
		{
			addAt.push_back(*at);
		}
		for_every(at, addAt)
		{
			if (!get_existing_cell(*at))
			{
				issue_create_cell(*at, true);
				if (startAt.is_set() && startAt.get() == *at)
				{
					mark_cell_visited(*at);
				}
			}
		}
		onMap = false; // not on map
	}
	
	if (!_justObjectives) // it's sort of a fail-safe for when the starting cell has not been triggered to generate
	{
		ArrayStatic<VectorInt2, 16> addAt;
		if (is_current() &&
			((!atCell.is_set() && imo && imo->get_presence()->get_in_room() == get_starting_room()) || // we have to be in starting room, if no cell is provided
			 (atCell.is_set() && get_start_at() == atCell.get())))
		{
			// if in transition/starting room or first cell
			for_every(at, pilgrimage->open_world__get_definition()->get_auto_generate_at_start())
			{
				addAt.push_back(*at);
			}
		}
		// always have them generated (unless already generated)
		if (is_current())
		{
			for_every(at, pilgrimage->open_world__get_definition()->get_always_generate_at())
			{
				addAt.push_back(*at);
			}
		}
		for_every(at, addAt)
		{
			if (!get_existing_cell(*at))
			{
				issue_create_cell(*at, true);
			}
		}
	}

	// create all cells
	if (!_justObjectives && ! workingOnCells &&
		pilgrimage->open_world__get_definition()->should_create_all_cells())
	{
		RangeInt2 size = pilgrimage->open_world__get_definition()->get_size();
		if (!size.x.is_empty() &&
			!size.y.is_empty())
		{
			for_range(int, y, size.y.min, size.y.max)
			{
				for_range(int, x, size.x.min, size.x.max)
				{
					VectorInt2 at(x, y);
					if (!get_existing_cell(at))
					{
						issue_create_cell(at);
					}
				}
			}
		}
	}

	if (!onMap)
	{
		timeSinceLastCellBlocked = 0.0f;
	}

	return onMap;
}

void PilgrimageInstanceOpenWorld::log(LogInfoContext & _log) const
{
	base::log(_log);

	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		_log.log(TXT("current objective       : %i \"%S\""), currentObjectiveIdx.get(NONE), currentObjectiveIdx.is_set() && owDef->get_objectives().is_index_valid(currentObjectiveIdx.get())? owDef->get_objectives()[currentObjectiveIdx.get()].id.to_char() : TXT("not valid"));
	}

	_log.log(TXT("existing cells          : %i"), existingCells.get_size());
	{
		int generated = 0;
		{
			Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("log"));
			for_every(ec, existingCells)
			{
				if (ec->generated)
				{
					++generated;
				}
			}
		}
		_log.log(TXT("  generated             : %i"), generated);
	}
	if (auto* d = startingRoomExitDoor.get())
	{
		_log.log(TXT("starting room exit door : %S"), Framework::DoorOperation::to_char(d->get_operation()));
	}
	else
	{
		_log.log(TXT("starting room exit door : not present"));
	}
	_log.log(TXT("canExitStartingRoom     : %S"), canExitStartingRoom ? TXT("YES") : TXT("no"));
	_log.log(TXT("leftStartingRoom        : %S"), leftStartingRoom ? TXT("YES") : TXT("no"));
	_log.log(TXT("leaveExitDoorAsIs       : %S"), leaveExitDoorAsIs ? TXT("YES") : TXT("no"));
	_log.log(TXT("manageExitDoor          : %S"), manageExitDoor ? TXT("YES") : TXT("no"));
	_log.log(TXT("blockedAwayCells        : %i (%.0fs)"), blockedAwayCells, timeSinceLastCellBlocked);
}

#ifdef VISUALISE_CELL_GENERATION
Array<String> getCellStack;
Array<VectorInt2> getCellStackAt;
#endif

Framework::Region* PilgrimageInstanceOpenWorld::get_main_region_for_cell(VectorInt2 const& _cell) const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get cell by region [find_cell_at]"));
	for_every(cell, existingCells)
	{
		if (cell->at == _cell)
		{
			return cell->mainRegion.get();
		}
	}

	return nullptr;
}

Optional<Vector3> PilgrimageInstanceOpenWorld::find_cell_at_for_map(Framework::IModulesOwner* _imo) const
{
	Vector3 offset = Vector3::zero;
	if (_imo)
	{
		if (auto* p = _imo->get_presence())
		{
			if (auto* r = p->get_in_room())
			{
				Vector3 nonExistentVisibleMapCellAt = Vector3(0.0f, 0.0f, -10000.0f);
				Vector3 visibleMapCellAt = r->get_value<Vector3>(NAME(visibleMapCellAt), nonExistentVisibleMapCellAt);
				offset = r->get_value<Vector3>(NAME(visibleMapCellAtOffset), offset);
				if (visibleMapCellAt != nonExistentVisibleMapCellAt)
				{
					return visibleMapCellAt;
				}
			}
		}
	}
	Optional<VectorInt2> at = find_cell_at(_imo);
	if (at.is_set())
	{
		return at.get().to_vector_int3().to_vector3() + offset;
	}
	return NP;
}

Optional<VectorInt2> PilgrimageInstanceOpenWorld::find_cell_at(Framework::IModulesOwner* _imo) const
{
	if (_imo)
	{
		if (auto* p = _imo->get_presence())
		{
			if (auto* r = p->get_in_room())
			{
				Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get cell by region [find_cell_at]"));
				for_every(cell, existingCells)
				{
					if (r->is_in_region(cell->region.get()))
					{
						return cell->at;
					}
				}
				if (r == get_starting_room())
				{
					return get_start_at();
				}
			}
		}
	}
	return NP;
}

Optional<VectorInt2> PilgrimageInstanceOpenWorld::find_cell_at(Framework::Room* _room) const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("add scenery in room"));

	for_every(cell, existingCells)
	{
		if (_room->is_in_region(cell->region.get()))
		{
			return cell->at;
		}
	}

	return NP;
}

Optional<VectorInt2> PilgrimageInstanceOpenWorld::find_cell_at(Framework::Region* _region) const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("add scenery in region"));

	for_every(cell, existingCells)
	{
		if (_region->is_in_region(cell->region.get()))
		{
			return cell->at;
		}
	}

	return NP;
}

Framework::Room* PilgrimageInstanceOpenWorld::get_vista_scenery_room(VectorInt2 const& _at) const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("add scenery in room"));

	for_every(cell, existingCells)
	{
		if (cell->at == _at)
		{
			return cell->vistaSceneryRoom.get();
		}
	}

	return nullptr;
}

void PilgrimageInstanceOpenWorld::debug_render_mini_map(Framework::IModulesOwner* _imo, Range2 const& _at, float _alpha, int _minDist, Optional<Colour> const& _forceColour, Framework::Region const * _highlightRegion)
{
	Optional<VectorInt2> atCell = find_cell_at(_imo);
	bool found = atCell.is_set();
	if (startAt.is_set())
	{
		atCell.set_if_not_set(get_start_at());
	}
	atCell.set_if_not_set(VectorInt2::zero);
	debug_render_mini_map(atCell.get(), _at, _alpha, _minDist, !found && !_forceColour.is_set()? Colour::grey : _forceColour, _highlightRegion);
}

void PilgrimageInstanceOpenWorld::debug_render_mini_map(VectorInt2 const& _cellAt, Range2 const& _at, float _alpha, int _minDist, Optional<Colour> const& _forceColour, Framework::Region const* _highlightRegion)
{
	Vector2 centre = _at.centre();
	float size = min(_at.x.length(), _at.y.length()) / (float)(1 + _minDist * 2.0f);
	float offset = -size * 0.5f;

	int xDist = TypeConversions::Normal::f_i_cells(((_at.x.length() - size) * 0.5f) / size);
	int yDist = TypeConversions::Normal::f_i_cells(((_at.y.length() - size) * 0.5f) / size);

	::System::Video3D* v3d = ::System::Video3D::get();
	v3d->face_display(::System::FaceDisplay::Both);

	for_range(int, oy, -yDist, yDist)
	{
		for_range(int, ox, -xDist, xDist)
		{
			VectorInt2 at = _cellAt + VectorInt2(ox, oy);

			Range2 tile;
			tile.x.min = (float)ox * size + offset + centre.x;
			tile.x.max = (float)(ox + 1) * size + offset + centre.x;
			tile.y.min = (float)oy * size + offset + centre.y;
			tile.y.max = (float)(oy + 1) * size + offset + centre.y;
			Range2 r = tile.expanded_by(Vector2::one * (-size * 0.1f));
			Range2 wr = tile;// .expanded_by(Vector2::one * (-size * 0.01f));
			Range2 ex = tile.expanded_by(Vector2::one * (-size * 0.45f));
			Range2 td = tile.expanded_by(Vector2::one * (-size * 0.4f));

			Range2 exl(Range(wr.x.min, r.x.min), ex.y);
			Range2 exr(Range(r.x.max, wr.x.max), ex.y);
			Range2 exu(ex.x, Range(r.y.max, wr.y.max));
			Range2 exd(ex.x, Range(wr.y.min, r.y.min));

			Cell * cell = nullptr;
			ExistingCell * existingCell = nullptr;
			get_cells(at, cell, existingCell);

			bool fill = false;
			Colour colour = Colour::blue;
			if (cell)
			{
				if (cell->set.get_as_any())
				{
					colour = Colour::green;
				}
				if (_forceColour.is_set())
				{
					colour = _forceColour.get();
				}
				if (existingCell)
				{
					if (existingCell->beingGenerated)
					{
						float timePeriod = 0.2f;
						if (::System::Core::get_timer_mod(timePeriod) < timePeriod * 0.5f)
						{
							fill = true;
							colour = Colour::white;
						}
#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (generatingSceneries)
						{
							colour = Colour::cyan;
						}
#endif

					}
					else
					{
						fill = true;
						if (existingCell->beingActivated)
						{
							float timePeriod = 0.2f;
							if (::System::Core::get_timer_mod(timePeriod) >= timePeriod * 0.5f)
							{
								fill = false;
							}
						}
					}
				}
#ifdef AN_DEVELOPMENT_OR_PROFILER
				else if (wantsToCreateCellAt.does_contain(at))
				{
					float timePeriod = wantsToCreateCellAt[0] == at ? 0.5f : 1.0f;
					if (destroyingCell)
					{
						timePeriod = 0.25f;
					}
					if (::System::Core::get_timer_mod(timePeriod) < timePeriod * 0.5f)
					{
						colour = destroyingCell? Colour::red : (moreExistingCellsNeeded? Colour::orange : Colour::white);
					}
				}
#endif

				colour = colour.with_set_alpha(_alpha);
				bool blend = _alpha < 0.99f;

				bool drawExits = cell->doneForMinPriority <= cell->set.get_cell_priority();

				if (_highlightRegion &&
					existingCell &&
					existingCell->region.get() &&
					existingCell->region->is_in_region(_highlightRegion))
				{
					::System::Video3DPrimitives::fill_rect_2d(Colour::orange.with_alpha(0.5f), r, true);
				}

				Colour dirColour = colour;
				if (fill)
				{
					::System::Video3DPrimitives::fill_rect_2d(colour, r, blend);
					
					dirColour = dirColour.mul_rgb(0.0f);
				}
				else
				{
					// lt
					::System::Video3DPrimitives::line_2d(colour, Vector2(r.x.min, ex.y.max), Vector2(r.x.min, r.y.max), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(ex.x.min, r.y.max), Vector2(r.x.min, r.y.max), blend);
					// rt
					::System::Video3DPrimitives::line_2d(colour, Vector2(r.x.max, ex.y.max), Vector2(r.x.max, r.y.max), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(ex.x.max, r.y.max), Vector2(r.x.max, r.y.max), blend);
					// lb
					::System::Video3DPrimitives::line_2d(colour, Vector2(r.x.min, ex.y.min), Vector2(r.x.min, r.y.min), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(ex.x.min, r.y.min), Vector2(r.x.min, r.y.min), blend);
					// rb
					::System::Video3DPrimitives::line_2d(colour, Vector2(r.x.max, ex.y.min), Vector2(r.x.max, r.y.min), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(ex.x.max, r.y.min), Vector2(r.x.max, r.y.min), blend);
				}

				{	// exits
					if (drawExits && cell->set.get_as_any() && cell->exits[DirFourClockwise::Up])
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exu.x.min, exu.y.min), Vector2(exu.x.min, exu.y.max), blend);
						::System::Video3DPrimitives::line_2d(colour, Vector2(exu.x.max, exu.y.min), Vector2(exu.x.max, exu.y.max), blend);
					}
					else if (! fill)
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exu.x.min, exu.y.min), Vector2(exu.x.max, exu.y.min), blend);
					}
					if (drawExits && cell->set.get_as_any() && cell->exits[DirFourClockwise::Right])
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exr.x.min, exr.y.min), Vector2(exr.x.max, exr.y.min), blend);
						::System::Video3DPrimitives::line_2d(colour, Vector2(exr.x.min, exr.y.max), Vector2(exr.x.max, exr.y.max), blend);
					}
					else if (!fill)
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exr.x.min, exr.y.min), Vector2(exr.x.min, exr.y.max), blend);
					}
					if (drawExits && cell->set.get_as_any() && cell->exits[DirFourClockwise::Down])
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exd.x.min, exd.y.min), Vector2(exd.x.min, exd.y.max), blend);
						::System::Video3DPrimitives::line_2d(colour, Vector2(exd.x.max, exd.y.min), Vector2(exd.x.max, exd.y.max), blend);
					}
					else if (!fill)
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exd.x.min, exd.y.max), Vector2(exd.x.max, exd.y.max), blend);
					}
					if (drawExits && cell->set.get_as_any() && cell->exits[DirFourClockwise::Left])
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exl.x.min, exl.y.min), Vector2(exl.x.max, exl.y.min), blend);
						::System::Video3DPrimitives::line_2d(colour, Vector2(exl.x.min, exl.y.max), Vector2(exl.x.max, exl.y.max), blend);
					}
					else if (!fill)
					{
						::System::Video3DPrimitives::line_2d(colour, Vector2(exl.x.max, exl.y.min), Vector2(exl.x.max, exl.y.max), blend);
					}
				}

				if (cell->set.get_as_any())
				{
					if (cell->dir == DirFourClockwise::Up)
					{
						::System::Video3DPrimitives::triangle_2d(dirColour, Vector2(td.x.centre(), td.y.max), Vector2(td.x.max, td.y.min), Vector2(td.x.min, td.y.min), blend);
					}
					if (cell->dir == DirFourClockwise::Right)
					{
						::System::Video3DPrimitives::triangle_2d(dirColour, Vector2(td.x.max, td.y.centre()), Vector2(td.x.min, td.y.min), Vector2(td.x.min, td.y.max), blend);
					}
					if (cell->dir == DirFourClockwise::Down)
					{
						::System::Video3DPrimitives::triangle_2d(dirColour, Vector2(td.x.centre(), td.y.min), Vector2(td.x.max, td.y.max), Vector2(td.x.min, td.y.max), blend);
					}
					if (cell->dir == DirFourClockwise::Left)
					{
						::System::Video3DPrimitives::triangle_2d(dirColour, Vector2(td.x.min, td.y.centre()), Vector2(td.x.max, td.y.min), Vector2(td.x.max, td.y.max), blend);
					}
				}
				if (cell->towardsObjective.is_set())
				{
					Range2 to = td.expanded_by(Vector2::one * (size * 0.1f));

					Vector2 points[3];
					points[0] = Vector2(to.x.centre(), to.y.centre());
					points[1] = points[0];
					points[2] = points[0];

					if (cell->towardsObjective.get() == DirFourClockwise::Up)
					{
						points[0] = Vector2(to.x.max, to.y.centre());
						points[1] = Vector2(to.x.centre(), to.y.max);
						points[2] = Vector2(to.x.min, to.y.centre());
					}
					if (cell->towardsObjective.get() == DirFourClockwise::Right)
					{
						points[0] = Vector2(to.x.centre(), to.y.max);
						points[1] = Vector2(to.x.max, to.y.centre());
						points[2] = Vector2(to.x.centre(), to.y.min);
					}
					if (cell->towardsObjective.get() == DirFourClockwise::Down)
					{
						points[0] = Vector2(to.x.max, to.y.centre());
						points[1] = Vector2(to.x.centre(), to.y.min);
						points[2] = Vector2(to.x.min, to.y.centre());
					}
					if (cell->towardsObjective.get() == DirFourClockwise::Left)
					{
						points[0] = Vector2(to.x.centre(), to.y.max);
						points[1] = Vector2(to.x.min, to.y.centre());
						points[2] = Vector2(to.x.centre(), to.y.min);
					}
					::System::Video3DPrimitives::line_strip_2d(dirColour, points, 3, blend);
				}

				if (at == _cellAt)
				{
					Colour colour = Colour::red;
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.min, wr.y.min), Vector2(r.x.min, wr.y.min), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.min, wr.y.min), Vector2(wr.x.min, r.y.min), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.min, wr.y.max), Vector2(r.x.min, wr.y.max), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.min, wr.y.max), Vector2(wr.x.min, r.y.max), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.max, wr.y.min), Vector2(r.x.max, wr.y.min), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.max, wr.y.min), Vector2(wr.x.max, r.y.min), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.max, wr.y.max), Vector2(r.x.max, wr.y.max), blend);
					::System::Video3DPrimitives::line_2d(colour, Vector2(wr.x.max, wr.y.max), Vector2(wr.x.max, r.y.max), blend);
				}
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::debug_visualise_map(VectorInt2 const& _at, bool _generate, Optional<VectorInt2> const& _highlightAt) const
{
	RangeInt2 r(_at, _at);
	r.expand_by(_generate? VectorInt2(10, 10) : VectorInt2(32, 32));
	debug_visualise_map(r, _generate, _highlightAt);
}

void PilgrimageInstanceOpenWorld::debug_visualise_map(RangeInt2 const & _range, bool _generate, Optional<VectorInt2> const& _highlightAt) const
{
	DebugVisualiserPtr dv;
	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, true, TXT("debug visualise map"));
		dv = (new DebugVisualiser(_generate ? String::printf(TXT("map")) : String::printf(TXT("map (%i)"), cells.get_size())));
	}
	dv->activate();
	dv->start_gathering_data();
	dv->clear();

	float size = 10.0f;
	float textScale = 1.0;

	for_range(int, y, _range.y.min, _range.y.max)
	{
		for_range(int, x, _range.x.min, _range.x.max)
		{
			VectorInt2 at = VectorInt2(x, y);

			Range2 er;
			Range2 wr;
			Range2 r;
			float spacing = size * 0.1f;
			float offset = -size * 0.5f;
			r.x.min = (float)x * size + spacing + offset;
			r.x.max = (float)(x + 1) * size - spacing + offset;
			r.y.min = (float)y * size + spacing + offset;
			r.y.max = (float)(y + 1) * size - spacing + offset;
			float wrSpacing = size * 0.01f;
			wr.x.min = (float)x * size + wrSpacing + offset;
			wr.x.max = (float)(x + 1) * size - wrSpacing  + offset;
			wr.y.min = (float)y * size + wrSpacing + offset;
			wr.y.max = (float)(y + 1) * size - wrSpacing + offset;
			er = wr.expanded_by(Vector2::one * (-size * 0.25f));

			Cell const* cell = nullptr;
			{
				if (_generate)
				{
					get_cell_at(at);
				}
				{
					Concurrency::ScopedMRSWLockRead lock(cellsLock, true, TXT("get cell [visualise]"));
					for_every(c, cells)
					{
						if (c->at == at)
						{
							cell = c;
						}
					}
				}
			}

			if (cell)
			{
				Colour colour = Colour::blue;
				if (cell->set.get_as_any())
				{
					colour = Colour::green;
					int useCellPriority = cell->set.get_cell_priority();
					if (cell->dependentOn.is_set())
					{
						useCellPriority = get_cell_at(cell->dependentOn.get()).set.get_cell_priority();
					}
					if (useCellPriority == 1)
					{
						colour = Colour::yellow;
					}
					if (useCellPriority == 2)
					{
						colour = Colour::cyan;
					}
					if (useCellPriority == 3)
					{
						colour = Colour::orange;
					}
					if (useCellPriority == 4)
					{
						colour = Colour::mint;
					}
					int exitsCount = (cell->exits[0] ? 1 : 0) + (cell->exits[1] ? 1 : 0) + (cell->exits[2] ? 1 : 0) + (cell->exits[3] ? 1 : 0);
					if (exitsCount == 0)
					{
						colour = Colour::c64Brown;
					}
#ifndef BUILD_PUBLIC_RELEASE
					if (testOpenWorldCell.is_set() && cell->at == testOpenWorldCell.get())
					{
						colour = Colour::magenta;
					}
#endif
				}
				if (cell->doneForMinPriority > cell->set.get_cell_priority())
				{
					colour = Colour::lerp(0.8f, colour, Colour::greyLight);
				}
				{
					dv->add_range2(r, colour);

					if (cell->set.get_as_any())
					{
						Range2 ar = r.expanded_by(Vector2::one * (-size * 0.2f));
						if (cell->dir == DirFourClockwise::Up) dv->add_arrow(ar.centre(), Vector2(ar.x.centre(), ar.y.max), colour);
						if (cell->dir == DirFourClockwise::Right) dv->add_arrow(ar.centre(), Vector2(ar.x.max, ar.y.centre()), colour);
						if (cell->dir == DirFourClockwise::Down) dv->add_arrow(ar.centre(), Vector2(ar.x.centre(), ar.y.min), colour);
						if (cell->dir == DirFourClockwise::Left) dv->add_arrow(ar.centre(), Vector2(ar.x.min, ar.y.centre()), colour);
					}
					if (cell->dependentOn.is_set())
					{
						dv->add_arrow(r.centre(), cell->dependentOn.get().to_vector2() * size, Colour::lerp(0.7f, Colour::purple, Colour::greyLight));
					}
				}

				Vector2 c;
				c.x = (float)x * size;
				c.y = (float)y * size;

				dv->add_text(c + Vector2(0.0f, +size * 0.25f), String::printf(TXT("%i x %i"), cell->at.x, cell->at.y), Colour::black, textScale * 3.0f);
				dv->add_text(c + Vector2(0.0f, -size * 0.2f), String::printf(TXT("%i (%i)"), cell->set.get_cell_priority(), cell->doneForMinPriority), Colour::black, textScale * 4.0f);
				dv->add_text(c + Vector2(0.0f, -size * 0.27f), String::printf(TXT("ord #%i->#%i"), cell->orderNoStart, cell->orderNoEnd), Colour::black, textScale * 2.0f);
				dv->add_text(c + Vector2(0.0f, -size * 0.35f), String::printf(TXT("iter %i"), cell->iteration), Colour::black, textScale * 2.0f);
				
				{
					VectorInt2 hlIndex = get_high_level_pattern_index(at, pilgrimage->open_world__get_definition()->get_pattern());
					dv->add_text(c + Vector2(0.0f, size * 0.2f), String::printf(TXT("hlr %ix%i"), hlIndex.x, hlIndex.y), Colour::blue, textScale);
				}

				if (auto* ls = cell->set.get_as_any())
				{
					if (cell->exits[DirFourClockwise::Up]) dv->add_range2(Range2(er.x, Range(r.y.max, wr.y.max)), colour);
					if (cell->exits[DirFourClockwise::Right]) dv->add_range2(Range2(Range(r.x.max, wr.x.max), er.y), colour);
					if (cell->exits[DirFourClockwise::Down]) dv->add_range2(Range2(er.x, Range(wr.y.min, r.y.min)), colour);
					if (cell->exits[DirFourClockwise::Left]) dv->add_range2(Range2(Range(wr.x.min, r.x.min), er.y), colour);

					dv->add_text(c, ls->get_name().to_string(), Colour::black, textScale);

					if (auto* pls = get_primal_set_at(at).get_as_any())
					{
						c.y -= size * 0.05f;
						dv->add_text(c, pls->get_name().to_string(), Colour::lerp(0.4f, Colour::black, Colour::white), textScale);
					}
				}

#ifdef DEBUG_TOWARDS_OBJECT_SHOW_ON_MAP
				if (cell->towardsObjectiveCostWorker.is_set())
				{ 
					dv->add_text(c + Vector2(0.0f, +size * 0.1f), String::printf(TXT("%.1f"), cell->towardsObjectiveCostWorker.get()), Colour::red, textScale * 10.0f);
				}
#endif
			}

			if (_highlightAt.is_set() &&
				_highlightAt.get() == at)
			{
				dv->add_range2(wr, Colour::white);
			}
		}
	}

#ifdef VISUALISE_CELL_GENERATION
	if (!getCellStackAt.is_empty())
	{
		VectorInt2 prev = getCellStackAt.get_first();
		for_every(a, getCellStackAt)
		{
			dv->add_arrow(prev.to_vector2() * size, a->to_vector2() * size, Colour::red);
			prev = *a;
		}
	}
#endif

	dv->end_gathering_data();
	dv->show_and_wait_for_key();
	dv->deactivate();
}

PilgrimageInstanceOpenWorld::ExistingCell* PilgrimageInstanceOpenWorld::get_existing_cell(VectorInt2 const& _at) const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get existing cell"));

	return get_existing_cell_already_locked(_at);
}

PilgrimageInstanceOpenWorld::ExistingCell* PilgrimageInstanceOpenWorld::get_existing_cell_already_locked(VectorInt2 const& _at) const
{
	for_every(cell, existingCells)
	{
		if (cell->at == _at)
		{
			return cast_to_nonconst(cell);
		}
	}
	return nullptr;
}

PilgrimageInstanceOpenWorld::ExistingCell* PilgrimageInstanceOpenWorld::get_existing_cell_for_already_locked(Framework::Room* _room) const
{
	if (_room)
	{
		Framework::Region* region = _room->get_in_region();
		while (region)
		{
			for_every(cell, existingCells)
			{
				if (cell->region == region)
				{
					return cast_to_nonconst(cell);
				}
			}
			region = region->get_in_region();
		}
	}
	return nullptr;
}

void PilgrimageInstanceOpenWorld::setup_vr_placement_for_door(Framework::Door* _door, VectorInt2 const& _at, VectorInt2 const& _dir)
{
	VectorInt2 absDir = _dir;
	absDir.x = abs(absDir.x);
	absDir.y = abs(absDir.y);

	Vector2 playAreaSize =  RoomGenerationInfo::get().get_play_area_rect_size();
	Vector2 playAreaOffset = RoomGenerationInfo::get().get_play_area_offset();
	float tileSize =  RoomGenerationInfo::get().get_tile_size();
	
	{
		bool alternate = false;
		bool dirPositive = _dir.x > 0 || _dir.y > 0;
		VectorInt2 atM = dirPositive ? _at : _at + _dir;
		VectorInt2 atP = dirPositive ? _at + _dir : _at;
		bool mOdd = mod(atM.x + atM.y, 2) == 1;

		Vector2 cellDoorRange = Vector2(6.0f, 6.0f); // we don't want to get too far
		if (auto* owDef = pilgrimage->open_world__get_definition())
		{
			alternate = pilgrimage->open_world__get_definition()->should_alternate_cell_door_vr_placement();
			if (pilgrimage->open_world__get_definition()->get_cell_door_tile_range().is_set())
			{
				cellDoorRange = pilgrimage->open_world__get_definition()->get_cell_door_tile_range().get().to_vector2() * tileSize;
			}
			if (pilgrimage->open_world__get_definition()->get_cell_door_range().is_set())
			{
				cellDoorRange = pilgrimage->open_world__get_definition()->get_cell_door_range().get();
			}
		}

		cellDoorRange.x = min(cellDoorRange.x, playAreaSize.x);
		cellDoorRange.y = min(cellDoorRange.y, playAreaSize.y);

		Vector3 aDir = absDir.x > absDir.y ? Vector3::xAxis : Vector3::yAxis;
		Vector3 centreAt = (playAreaSize * 0.5f + playAreaOffset).to_vector3();
		Vector3 aAt = centreAt;

		if (alternate)
		{
			aDir = -aDir;
			// offset doors from the centre
			// if we're going from left to right, keep dir this way but doors should be placed on max y and min y
			aAt = centreAt;
			if (abs(aDir.x) > abs(aDir.y))
			{
				aAt.y += (cellDoorRange.y * 0.5f - tileSize * 0.5f) * (mOdd? -1.0f : 1.0f);
			}
			else
			{
				aAt.x += (cellDoorRange.x * 0.5f - tileSize * 0.5f) * (mOdd ? -1.0f : 1.0f);
			}
		}

		if (RoomGenerationInfo::get().is_small() && abs(aDir.x) > abs(aDir.y))
		{
			aAt.x = centreAt.x;
			aAt.y = playAreaSize.y - tileSize * 0.5f + playAreaOffset.y;
		}

		Transform vrPlacement0 = look_matrix(aAt, aDir, Vector3::zAxis).to_transform();
		Transform vrPlacement1 = look_matrix(aAt, -aDir, Vector3::zAxis).to_transform();
		
		if (auto* owDef = pilgrimage->open_world__get_definition())
		{
			// check if there is a vr placement provided by any of the cells, through a pilgrimage definition
			// check both doors to have a single cell door vr placement working
			for_count(int, checkDir, 2)
			{
				VectorInt2 checkingFrom = checkDir == 0? atM : atP; // positive
				VectorInt2 checkingTo = checkDir == 0 ? atP : atM; // negative
				if (auto* cdVRPlacement = owDef->get_cell_door_vr_placement(checkingFrom, checkingTo))
				{
					WheresMyPoint::Value value;
					value.set<Transform>(look_matrix(Vector3::zero /* no offset as we want 0,0 to be centre, we offset it later on with centreAt */, (_dir).to_vector2().to_vector3(), Vector3::zAxis).to_transform());
					//
					WheresMyPoint::Context context(&simpleWMPOwnerForCellDoors);
					context.set_random_generator(Random::Generator(0, 0));
					//
					cdVRPlacement->wheresMyPoint.update(value, context);
					if (value.get_type() == type_id<Transform>())
					{
						if (checkDir == 0)
						{
							vrPlacement0 = value.get_as<Transform>();
							vrPlacement0.set_translation(vrPlacement0.get_translation() + centreAt);
							vrPlacement1 = vrPlacement0.to_world(rotation_xy_matrix(180.0f).to_transform());
						}
						else
						{
							vrPlacement1 = value.get_as<Transform>();
							vrPlacement1.set_translation(vrPlacement1.get_translation() + centreAt);
							vrPlacement0 = vrPlacement1.to_world(rotation_xy_matrix(180.0f).to_transform());
						}
						break;
					}
				}
			}
		}

		for_count(int, doorIdx, 2)
		{
			if (auto* dir = _door->get_linked_door(doorIdx))
			{
				if (!dir->is_vr_space_placement_set())
				{
					dir->set_vr_space_placement(doorIdx == 0? vrPlacement0 : vrPlacement1);
					dir->make_vr_placement_immovable();
				}
			}
		}
	}
	
	// can't be moved in vr
	_door->be_important_vr_door();

	// separates two cells
	_door->be_world_separator_door(WorldSeparatorReason::CellSeparator);

	// get vr elevator altitude to make some of the cells reach down, underground
	{
		auto const& cellAt = get_cell_at(_at);
		auto const& cellTo = get_cell_at(_at + _dir);

		Optional<float> cellAtVRElevatorAltitude;
		Optional<float> cellToVRElevatorAltitude;
		if (auto* rt = cellAt.set.get_as_any())
		{
			cellAtVRElevatorAltitude = rt->get_tags().get_tag(NAME(vrElevatorAltitude));
		}
		if (auto* rt = cellTo.set.get_as_any())
		{
			cellToVRElevatorAltitude = rt->get_tags().get_tag(NAME(vrElevatorAltitude));
		}
		float vrElevatorAltitude = 0.0f;
		if (cellAtVRElevatorAltitude.is_set() &&
			cellToVRElevatorAltitude.is_set())
		{
			// min of two
			vrElevatorAltitude = min(cellAtVRElevatorAltitude.get(), cellToVRElevatorAltitude.get());
		}
		else
		{
			// one of or default
			vrElevatorAltitude = cellAtVRElevatorAltitude.get(vrElevatorAltitude);
			vrElevatorAltitude = cellToVRElevatorAltitude.get(vrElevatorAltitude);
		}

		if (auto* owDef = pilgrimage->open_world__get_definition())
		{
			Vector2 preciseAt = _at.to_vector2();
			preciseAt += _dir.to_vector2() * 0.5f;
			vrElevatorAltitude += Vector2::dot(owDef->get_vr_elevator_altitude_gradient(), preciseAt); // g.x * at.x + g.y * at.y
		}

		_door->set_vr_elevator_altitude(vrElevatorAltitude); // base level
	}
}

void PilgrimageInstanceOpenWorld::set_vr_elevator_altitude_for(Framework::Door* _door, VectorInt2 const& _at) const
{
	if (auto* owDef = pilgrimage->open_world__get_definition())
	{
		Vector2 preciseAt = _at.to_vector2();
		float vrElevatorAltitude = Vector2::dot(owDef->get_vr_elevator_altitude_gradient(), preciseAt); // g.x * at.x + g.y * at.y
		_door->set_vr_elevator_altitude(vrElevatorAltitude); // base level
	}
}

Random::Generator PilgrimageInstanceOpenWorld::get_random_seed_at(VectorInt2 const& _at) const
{
	Random::Generator rs = randomSeed;
	rs.advance_seed(_at.x * 297 + (_at.x + 1) * 892 + _at.y * 967 + (_at.y + 3) * 972,
					_at.x * 124 + (_at.x + 5) * 974 + _at.y * 173 + (_at.y + 2) * 907);
	return rs;
}

Random::Generator PilgrimageInstanceOpenWorld::get_random_seed_at_tile(VectorInt2 const& _at) const
{
	Random::Generator rs = randomSeed;
	// can be the same as above
	rs.advance_seed(_at.x * 297 + (_at.x + 1) * 892 + _at.y * 967 + (_at.y + 3) * 972,
					_at.x * 124 + (_at.x + 5) * 974 + _at.y * 173 + (_at.y + 2) * 907);
	return rs;
}

PilgrimageOpenWorldDefinition::SpecialRules const& PilgrimageInstanceOpenWorld::get_special_rules() const
{
	return pilgrimage->open_world__get_definition()->get_special_rules();
}

bool PilgrimageInstanceOpenWorld::is_objective_ok(PilgrimageOpenWorldDefinition::Objective const* _objective) const
{
	if (!GameDefinition::check_chosen(_objective->forGameDefinitionTagged) ||
		!_objective->forSkipContentTags.check(skipContentTags))
	{
		return false;
	}
	return true;
}

bool PilgrimageInstanceOpenWorld::is_objective_ok_at(PilgrimageOpenWorldDefinition::Objective const* _objective, VectorInt2 const& _cellAt, Optional<bool> const& _allowPilgrimageDeviceGroupId) const
{
	if (!is_objective_ok(_objective))
	{
		return false;
	}
	if (_objective->does_apply_at(_cellAt, this))
	{
		return true;
	}
	if (_allowPilgrimageDeviceGroupId.get(false))
	{
		if (_objective->pilgrimageDeviceGroupId.is_valid())
		{
			Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("get_objective_for : pilgrimage devices"));

			for_every(cell, cells)
			{
				if (cell->at == _cellAt)
				{
					for_every(kpd, cell->knownPilgrimageDevices)
					{
						if (kpd->groupId == _objective->pilgrimageDeviceGroupId &&
							(! _objective->requiresToDepletePilgrimageDevice || ! kpd->depleted))
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

PilgrimageOpenWorldDefinition::Objective const* PilgrimageInstanceOpenWorld::get_objective_for(VectorInt2 const& _cellAt, Optional<int> const & _onlyObjectiveIdx) const
{
	auto* owDef = pilgrimage->open_world__get_definition();
	for_every(objective, owDef->get_objectives())
	{
		if (objective->notActualObjective)
		{
			continue;
		}
		if (_onlyObjectiveIdx.is_set() && _onlyObjectiveIdx.get() != for_everys_index(objective))
		{
			continue;
		}
		if (is_objective_ok_at(objective, _cellAt, true))
		{
			return objective;
		}
	}
	return nullptr;
}

PilgrimageOpenWorldDefinition::Objective const* PilgrimageInstanceOpenWorld::get_objective_to_create_cell(VectorInt2 const& _cellAt) const
{
	auto* owDef = pilgrimage->open_world__get_definition();
	for_every(objective, owDef->get_objectives())
	{
		if (objective->cellTypes.is_empty() &&
			objective->regionTypes.is_empty() &&
			objective->roomTypes.is_empty())
		{
			continue;
		}
		if (is_objective_ok_at(objective, _cellAt))
		{
			return objective;
		}
	}
	return nullptr;
}

PilgrimageInstanceOpenWorld::CellSet PilgrimageInstanceOpenWorld::get_primal_set_at(VectorInt2 const& _at, PrimalSet _primalSet, int _iteration, int _minCellPriority, bool _ignoreCellPriority) const
{
	PilgrimageInstanceOpenWorld::CellSet set;

	Optional<int> maxCellPriority; // every next iteration should have the same or lower priority

	auto* owDef = pilgrimage->open_world__get_definition();
	if (!is_cell_address_valid(_at))
	{
		// empty
		return set;
	}

	Random::Generator rg = get_random_seed_at(_at);
	rg.advance_seed(8680, 925);

	// to avoid checking for the same one over and over, let's keep a list and throw away those that don't work
	PilgrimageOpenWorldDefinition::CellSet const* useCellSet = nullptr;
	{
		if (_primalSet == PrimalSet_Objectives && !owDef->get_for_objectives_cell_set().is_empty())
		{
			useCellSet = &owDef->get_for_objectives_cell_set();
		}
		else if (_primalSet == PrimalSet_Rules && !owDef->get_for_rules_cell_set().is_empty())
		{
			useCellSet = &owDef->get_for_rules_cell_set();
		}
		else if (_at == get_start_at() && !owDef->get_starting_cell_set().is_empty())
		{
			useCellSet = &owDef->get_starting_cell_set();
		}
		else
		{
			useCellSet = &owDef->get_generic_cell_set();
		}
	}
	an_assert(useCellSet);
	auto const& cellSet = *useCellSet;
	ARRAY_PREALLOC_SIZE(PilgrimageOpenWorldDefinition::CellSet::Available, chooseFrom, cellSet.available.get_size());
	if (cellSet.maxAvailableCellPriority >= _minCellPriority)
	{
		for_every(a, cellSet.available)
		{
			if (GameDefinition::check_chosen(a->forGameDefinitionTagged))
			{
				chooseFrom.push_back(*a);
			}
		}
	}
	else
	{
		// empty
		return set;
	}

	int chooseFromFills = 0;

#ifdef OUTPUT_CELL_GENERATION_DETAILS
	output(TXT("get primal set for %i x %i %S: rg %S"), _at.x, _at.y, _primalSet == PrimalSet_Rules? TXT("(rules) ") : (_primalSet == PrimalSet_Objectives ? TXT("(objectives) ") : TXT("")), rg.get_seed_string().to_char());
#endif

	bool firstChoice = true;
	bool allowCrouch = RoomGenerationInfo::get().does_allow_crouch();
	bool allowAnyCrouch = RoomGenerationInfo::get().does_allow_any_crouch();
	while (_iteration >= 0) // iteration is to handle cases where we failed but also to choose from a lower cell priority, for objectives and rules we ignore cell priorities
	{
		if (chooseFrom.is_empty())
		{
			firstChoice = false;
			for_every(a, cellSet.available)
			{
				if (GameDefinition::check_chosen(a->forGameDefinitionTagged))
				{
					if (maxCellPriority.get(a->cellPriority) >= a->cellPriority || _ignoreCellPriority)
					{
						chooseFrom.push_back(*a);
					}
				}
			}
			if (chooseFrom.is_empty())
			{
				// no more stuff to choose from
				break;
			}
			++chooseFromFills;
		}
		rg.advance_seed(56, 2608);
		Framework::LibraryStored* chosen = nullptr;
		int chosenCellPriority = 0;
		{
			int idx = RandomUtils::ChooseFromContainer<Array<PilgrimageOpenWorldDefinition::CellSet::Available>, PilgrimageOpenWorldDefinition::CellSet::Available>::choose(rg,
				chooseFrom, [firstChoice, allowCrouch, allowAnyCrouch](PilgrimageOpenWorldDefinition::CellSet::Available const& a)
				{ return (firstChoice || ! a.notFirstChoice)
					  && (! a.crouch || allowCrouch)
					  && (! a.anyCrouch || allowAnyCrouch)
					? a.probCoef : 0.0f; });
#ifdef OUTPUT_CELL_GENERATION_DETAILS
			output(TXT("iteration left %i choose from:"), _iteration);
			for_every(c, chooseFrom)
			{
				output(TXT(" %C %S"), for_everys_index(c) == idx? '>' : ' ', c->libraryStored.get_name().to_string().to_char());
			}
#endif
			chosen = chooseFrom[idx].libraryStored.get();
			chosenCellPriority = chooseFrom[idx].cellPriority;
			chooseFrom.remove_fast_at(idx);
		}
		if (chosen)
		{
			if (! game->is_ok_to_use_for_region_generation(chosen))
			{
				continue;
			}
		}
		if (chosen && ! _ignoreCellPriority)
		{
			if (maxCellPriority.get(chosenCellPriority) < chosenCellPriority)
			{
				// look for a lower priority
				continue;
			}
			maxCellPriority = chosenCellPriority;
		}
		if (_iteration == 0)
		{
			if (chosen)
			{
				if (auto* rt = fast_cast<Framework::RoomType>(chosen))
				{
					set.roomType = rt;
					rt->load_on_demand_if_required();
				}
				if (auto* rt = fast_cast<Framework::RegionType>(chosen))
				{
					set.regionType = rt;
					rt->load_on_demand_if_required();
				}
			}
			break;
		}
		else
		{
			--_iteration;
		}
	}

#ifdef OUTPUT_CELL_GENERATION_DETAILS
	output(TXT("chosen %S"), set.get_as_any()? set.get_as_any()->get_name().to_string().to_char() : TXT("--"));
#endif
	return set;
}

VectorInt2 PilgrimageInstanceOpenWorld::cell_at_to_tile_at(VectorInt2 const& _cellAt) const
{
	auto* owDef = pilgrimage->open_world__get_definition();

	int tileSize = owDef->get_distanced_pilgrimage_devices_tile_size();

	int halfTileSize = tileSize / 2;

	VectorInt2 at = _cellAt + VectorInt2(halfTileSize, halfTileSize);
	VectorInt2 sub(mod(at.x, tileSize), mod(at.y, tileSize));
	VectorInt2 mainAt = at - sub;
	VectorInt2 tileAt(mainAt.x / tileSize, mainAt.y / tileSize);

	return tileAt;
}

VectorInt2 PilgrimageInstanceOpenWorld::get_tile_centre(VectorInt2 const& _tileAt) const
{
	auto* owDef = pilgrimage->open_world__get_definition();

	int tileSize = owDef->get_distanced_pilgrimage_devices_tile_size();

	VectorInt2 mainAt(_tileAt.x * tileSize, _tileAt.y * tileSize);
	VectorInt2 centreAt = mainAt;

	an_assert(cell_at_to_tile_at(centreAt) == _tileAt);
	return centreAt;
}

PilgrimageInstanceOpenWorld::PilgrimageDevicesTile const& PilgrimageInstanceOpenWorld::get_pilgrimage_devices_tile_at(int _dpdIdx, VectorInt2 const& _at, bool _atIsTileCoord) const
{
	VectorInt2 tileAt = _atIsTileCoord? _at : cell_at_to_tile_at(_at);

	auto* owDef = pilgrimage->open_world__get_definition();

	an_assert(owDef->get_distanced_pilgrimage_devices().is_index_valid(_dpdIdx));

	auto& dpds = owDef->get_distanced_pilgrimage_devices()[_dpdIdx];
	
	if (dpds.centreBased.use)
	{
		// for centre based, force to use the same tile
		tileAt = VectorInt2::zero;
	}

	// if already done, return it
	{
		Concurrency::ScopedSpinLock lock(pilgrimageDevicesTilesLock);
		for_every(tile, pilgrimageDevicesTiles)
		{
			if (!tile->centreBased && !dpds.centreBased.use)
			{
				if (tile->tile == tileAt &&
					tile->groupId == dpds.groupId)
				{
					return *tile;
				}
			}
			else if (tile->centreBased && dpds.centreBased.use)
			{
				if (tile->groupId == dpds.groupId)
				{
					return *tile;
				}
			}
		}
	}

	ArrayStatic<PilgrimageDevicesTile const *, 3> checkAgainstTiles; SET_EXTRA_DEBUG_INFO(checkAgainstTiles, TXT("PilgrimageInstanceOpenWorld::get_pilgrimage_devices_tile_at.checkAgainstTiles"));

	if (!dpds.centreBased.use)
	{
		// make sure tiles closer to 0,0 exist
		VectorInt2 towardsCentre = VectorInt2::zero;
		{
			towardsCentre.x = tileAt.x > 0 ? -1 : (tileAt.x < 0 ? 1 : 0);
			towardsCentre.y = tileAt.y > 0 ? -1 : (tileAt.y < 0 ? 1 : 0);
			{
				// stop propagation periodically
				// this may result in some devices may be closer than allowed but is require to have a robust solution for far away tiles
				// note that another reason is to avoid long recursion - although this could be avoided by changing the implementation to a loop-based
				int const tileDist = 5;
				if (mod_raw(tileAt.x, tileDist) == 0) towardsCentre.x = 0;
				if (mod_raw(tileAt.y, tileDist) == 0) towardsCentre.y = 0;
			}
			if (towardsCentre.x != 0)
			{
				checkAgainstTiles.push_back(&get_pilgrimage_devices_tile_at(_dpdIdx, VectorInt2(tileAt.x + towardsCentre.x, tileAt.y), true /* tile coord */));
			}
			if (towardsCentre.y != 0)
			{
				checkAgainstTiles.push_back(&get_pilgrimage_devices_tile_at(_dpdIdx, VectorInt2(tileAt.x, tileAt.y + towardsCentre.y), true /* tile coord */));
			}
			if (towardsCentre.x != 0 && towardsCentre.y != 0)
			{
				checkAgainstTiles.push_back(&get_pilgrimage_devices_tile_at(_dpdIdx, tileAt + towardsCentre, true /* tile coord */));
			}
		}
	}

	// make sure there's no one else doing it at the same time
	PilgrimageDevicesTile* tile;
	{
		Concurrency::ScopedSpinLock lock(pilgrimageDevicesTilesLock);
		for_every(tile, pilgrimageDevicesTiles)
		{
			if (!tile->centreBased && !dpds.centreBased.use)
			{
				if (tile->tile == tileAt &&
					tile->groupId == dpds.groupId)
				{
					return *tile;
				}
			}
			else if (tile->centreBased && dpds.centreBased.use)
			{
				if (tile->groupId == dpds.groupId)
				{
					return *tile;
				}
			}
		}

		PilgrimageDevicesTile newTile;
		newTile.centreBased = dpds.centreBased.use;
		newTile.tile = tileAt;
		newTile.groupId = dpds.groupId;
		pilgrimageDevicesTiles.push_back(newTile);
		tile = &pilgrimageDevicesTiles.get_last();
	}
	
	// common stuff for both centre based and tile based
	struct PDIt
	{
		Vector2 at;
		Vector2 moveBy;
		float distance = 1000.0f;	// to avoid checking if zero, will use minimum value from both
									// (otherwise it would end up being maximum, as the one with a greater value would push away the other one)
									// also used for centre based to sort (distance from centre)

		VectorInt2 atCell;

		static int compare(void const* _a, void const* _b)
		{
			PDIt const* a = plain_cast<PDIt>(_a);
			PDIt const* b = plain_cast<PDIt>(_b);
			if (a->at.y < b->at.y) return A_BEFORE_B;
			if (a->at.y > b->at.y) return B_BEFORE_A;
			if (a->at.x < b->at.x) return A_BEFORE_B;
			if (a->at.x > b->at.x) return B_BEFORE_A;
			return A_AS_B;
		}
		static int compare_distance(void const* _a, void const* _b)
		{
			PDIt const* a = plain_cast<PDIt>(_a);
			PDIt const* b = plain_cast<PDIt>(_b);
			if (a->distance < b->distance) return A_BEFORE_B;
			if (a->distance > b->distance) return B_BEFORE_A;
			return A_AS_B;
		}
	};
	Array<PDIt> pds;

	if (dpds.centreBased.use)
	{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
		output(TXT("[distanced pd] add pilgrimage devices distanced %i centre based"), _dpdIdx);
#endif
		VectorInt2 startAt = get_start_at();

		Random::Generator rgCB = get_random_seed_at_tile(VectorInt2::zero);
		rgCB.advance_seed(_dpdIdx, 0);

		// we offset by half to "be in cell"
		int triesLeft = 1000;
		while (triesLeft > 0)
		{
			--triesLeft;
			Vector2 centreAt = startAt.to_vector2() + Vector2::half;
			if (dpds.centreBased.distanceCentreToStartAt.is_set())
			{
				centreAt += Vector2(0.0f, rgCB.get(dpds.centreBased.distanceCentreToStartAt.get())).rotated_by_angle(rgCB.get_float(-180.0f, 180.0f));
			}
			float inRadius = max(0.5f, rgCB.get(dpds.centreBased.distanceFromCentre.get(Range(0.0f))));

			bool ok = true;

			for_every_ref(pd, dpds.devices)
			{
				if (auto* gd = GameDefinition::get_chosen())
				{
					if (!gd->get_pilgrimage_devices_tagged().check(pd->get_tags()))
					{
						// does not match, skip
						continue;
					}
				}

				Random::Generator rg = rgCB.spawn();

				Range useDistLimits;
				{
					if (dpds.distance.is_set())
					{
						useDistLimits = dpds.distance.get();
					}
					else
					{
						useDistLimits = pd->get_open_world_distance();
					}
					if (useDistLimits.is_empty())
					{
						warn(TXT("no dist limits provided for pilgrimage devices"));
						useDistLimits = Range(4.0f, 6.0f);
					}
				}

				int amount = (TypeConversions::Normal::f_i_closest(sqr(inRadius) * pi<float>() / sqr(useDistLimits.centre())) * 3) / 2;
				amount = max(amount, 1);

				pds.clear();
				pds.make_space_for(amount);

				while (pds.get_size() < amount)
				{
					PDIt pdIt;
					pdIt.at = centreAt;
					if (amount > 1)
					{
						pdIt.at += Vector2(0.0f, rg.get_float(0.0f, inRadius)).rotated_by_angle(rg.get_float(-180.0f, 180.0f));
					}
					pdIt.moveBy = Vector2::zero;
					pdIt.distance = rg.get(useDistLimits);
					pds.push_back(pdIt);
				}

				// move around and settle
				for_count(int, itIdx, 1000)
				{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
					dv->start_gathering_data();
					dv->clear();
#endif
					for_every(pd, pds)
					{
						pd->moveBy = Vector2::zero;
						if (dpds.forcedAt.is_index_valid(for_everys_index(pd)))
						{
							pd->at = dpds.forcedAt[for_everys_index(pd)].to_vector2();
						}
						else
						{
							float moveAwayFactor = 0.5f;
							float moveCloserStrength = 0.005f;

							auto& checkAgainstPDS = pds;

							for_every(opd, checkAgainstPDS)
							{
								if (pd == opd) continue;

								float minDistOfBoth = min(pd->distance, opd->distance);
								float maxDistOfBoth = max(pd->distance, opd->distance);
								float distMin = 0.6f * minDistOfBoth + 0.4f * maxDistOfBoth;
								float distMax = maxDistOfBoth * 2.0f;

								Vector2 to = opd->at - pd->at;
								Vector2 toDir = to.normal();
								float toDist = to.length();
								if (toDist < distMin)
								{
									pd->moveBy += -toDir * moveAwayFactor * (1.0f + 1.0f * ((distMin - toDist) / distMin));
								}
								if (toDist > distMax)
								{
									pd->moveBy += toDir * moveCloserStrength * (0.3f + 0.7f * (distMax / toDist));
								}
							}
						}
					}

					// move all a bit
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
					Colour colour = Colour::green;
					String pdIdx = String::printf(TXT("%i"), for_everys_index(pd));
#endif
					for_every(pd, pds)
					{
						pd->moveBy *= 0.2f;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
						dv->add_circle(pd->at, 0.5f, colour);
						dv->add_circle(pd->at, pd->distance, Colour::grey);
						dv->add_arrow(pd->at, pd->at + pd->moveBy, Colour::blue);
						dv->add_text(pd->at, pdIdx, Colour::black);
#endif
						pd->at += pd->moveBy;
					}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
					for_every(pd, otherTilesPDs)
					{
						dv->add_circle(pd->at, 0.5f, Colour::red);
					}
					dv->end_gathering_data();
					if ((itIdx % 200) == 0)
					{
						dv->show_and_wait_for_key();
					}
#endif
				}

				// remove all that are outside the radius or would end up in invalid cells
				for (int i = 0; i < pds.get_size(); ++i)
				{
					auto& pd = pds[i];
					bool isValid = true;
					pd.distance = (pd.at - centreAt).length();
					pd.atCell = TypeConversions::Normal::f_i_cells(pd.at);
					if (isValid)
					{
						isValid &= pd.distance <= inRadius;
					}
					if (isValid)
					{
						if (dpds.centreBased.xRange.is_set())
						{
							isValid &= dpds.centreBased.xRange.get().does_contain(pd.atCell.x);
						}
						if (dpds.centreBased.yRange.is_set())
						{
							isValid &= dpds.centreBased.yRange.get().does_contain(pd.atCell.y);
						}
					}
					if (isValid)
					{
						if (!is_cell_address_valid(pd.atCell))
						{
							isValid = false;
						}
						if (auto* owDef = pilgrimage->open_world__get_definition())
						{
							for_every(o, owDef->get_objectives())
							{
								if (!o->allowDistancedPilgrimageDevices &&
									is_objective_ok_at(o, pd.atCell))
								{
									isValid = false;
								}
							}
						}
					}
					if (! isValid)
					{
						pds.remove_fast_at(i);
						--i;
					}
				}

				// remove these occupying the same cell
				for (int i = 0; i < pds.get_size(); ++i)
				{
					for (int j = i+1; j < pds.get_size(); ++j)
					{
						if (pds[i].atCell == pds[j].atCell)
						{
							pds.remove_fast_at(j);
						}
					}
				}

				// closest come first
				sort(pds, PDIt::compare_distance);

				if (dpds.centreBased.maxCount.is_set())
				{
					int mc = dpds.centreBased.maxCount.get();
					if (pds.get_size() > mc)
					{
						pds.set_size(mc);
					}
				}

				if (pds.get_size() < dpds.centreBased.minCount)
				{
					ok = false;
				}

				if (ok)
				{
					for_every(ipd, pds)
					{
						KnownPilgrimageDeviceInTile kpd;
						kpd.at = TypeConversions::Normal::f_i_cells(ipd->at);
						kpd.device = pd;
						kpd.subPriority = rg.get_int(10000);
						an_assert(tile->knownPilgrimageDevices.has_place_left());
						if (tile->knownPilgrimageDevices.has_place_left())
						{
							tile->knownPilgrimageDevices.push_back(kpd);
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
							output(TXT("[distanced pd] add device \"%S\" at %ix%i (centre based)"), pd->get_name().to_string().to_char(), kpd.at.x, kpd.at.y);
#else
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_JUST_ADD_INFO
							output(TXT("[distanced pd] add device \"%S\" at %ix%i (centre based)"), pd->get_name().to_string().to_char(), kpd.at.x, kpd.at.y);
#endif
#endif
						}
					}
				}

				if (!ok)
				{
					break;
				}
			}

			if (ok)
			{
				tile->centreAt = TypeConversions::Normal::f_i_cells(centreAt);
				tile->radiusFromCentre = inRadius + 1.0f; // to compensate rounding above
				// we're done
				break;
			}
			else
			{
				tile->knownPilgrimageDevices.clear();
			}
		}

		if (tile->knownPilgrimageDevices.is_empty())
		{
			error(TXT("could not generate centre based pilgrimage devices"));
		}
	}
	else
	{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
		output(TXT("[distanced pd] add pilgrimage devices distanced %i at tile %ix%i"), _dpdIdx, tileAt.x, tileAt.y);
#endif

		int tileSize = owDef->get_distanced_pilgrimage_devices_tile_size();

		Random::Generator rgTile = get_random_seed_at_tile(tileAt);
		rgTile.advance_seed(_dpdIdx, 0);

#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
		output(TXT("[distanced pd] rg for tile %ix%i is %S"), tileAt.x, tileAt.y, rgTile.get_seed_string().to_char());
		output(TXT("[distanced pd] check against tiles:"));
		for_every_ptr(cat, checkAgainstTiles)
		{
			output(TXT("[distanced pd]  %ix%i"), cat->tile.x, cat->tile.y);
		}
#endif

		RangeInt2 tileRangeLocal = RangeInt2::empty;
		Range2 tileRangeLocalF = Range2::empty;
		{
			int halfTileSize = tileSize / 2;
			VectorInt2 lb = -VectorInt2(halfTileSize, halfTileSize);
			tileRangeLocal.include(lb);
			tileRangeLocal.include(lb + VectorInt2(tileSize - 1, tileSize - 1));
			//
			tileRangeLocalF.include(Vector2((float)tileRangeLocal.x.min, (float)tileRangeLocal.y.min));
			tileRangeLocalF.include(Vector2((float)tileRangeLocal.x.max + 1.0f, (float)tileRangeLocal.y.max + 1.0f));
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
			output(TXT("[distanced pd] tileRangeLocal %S"), tileRangeLocal.to_string().to_char());
			output(TXT("[distanced pd] tileRangeLocalF %S"), tileRangeLocalF.to_string().to_char());
#endif
		}

	
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
		DebugVisualiserPtr dv;
		{
			dv = (new DebugVisualiser(String::printf(TXT("tile %ix%i"), tileAt.x, tileAt.y)));
		}
		dv->activate();
#endif

		for_every_ref(pd, dpds.devices)
		{
			if (auto* gd = GameDefinition::get_chosen())
			{
				if (!gd->get_pilgrimage_devices_tagged().check(pd->get_tags()))
				{
					// does not match, skip
					continue;
				}
			}

			Random::Generator rg = rgTile.spawn();

#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
			output(TXT("[distanced pd] try device idx (%i:%S) rg %S"), for_everys_index(pd), pd->get_name().to_string().to_char(), rg.get_seed_string().to_char());
#endif

			Range useDistLimits;
			{
				if (dpds.distance.is_set())
				{
					useDistLimits = dpds.distance.get();
				}
				else
				{
					useDistLimits = pd->get_open_world_distance();
				}
				if (useDistLimits.is_empty())
				{
					warn(TXT("no dist limits provided for pilgrimage devices"));
					useDistLimits = Range(4.0f, 6.0f);
				}
			}

			Optional<int> atXLine = dpds.atXLine;
			Optional<int> atYLine = dpds.atYLine;
			int amount = 1;
			if (atXLine.is_set() && atYLine.is_set())
			{
				amount = 1;
			}
			else if (atXLine.is_set() || atYLine.is_set())
			{
				amount = (TypeConversions::Normal::f_i_closest((float)tileSize / useDistLimits.centre()) * 3) / 2;
			}
			else
			{
				amount = (TypeConversions::Normal::f_i_closest(sqr((float)tileSize / useDistLimits.centre())) * 3) / 2;
			}
			pds.clear();
			amount = max(amount, dpds.forcedAt.get_size()); // at least as many as forcedAt
			pds.make_space_for(amount);
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_DETAILED_ITERATION
			output(TXT("[distanced pd] tileRangeLocalF %S"), tileRangeLocalF.to_string().to_char());
#endif
			while (pds.get_size() < amount)
			{
				PDIt pdIt;
				// two separate lines to keep the execution order
				pdIt.at.x = rg.get_float(tileRangeLocalF.x);
				pdIt.at.y = rg.get_float(tileRangeLocalF.y);
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_DETAILED_ITERATION
				output(TXT("[distanced pd] pdIt.at %.5fx%.5f rg %S"), pdIt.at.x, pdIt.at.y, rg.get_seed_string().to_char());
#endif
				if (atXLine.is_set()) pdIt.at.x = (float)atXLine.get() + 0.5f;
				if (atYLine.is_set()) pdIt.at.y = (float)atYLine.get() + 0.5f;
				pdIt.moveBy = Vector2::zero;
				pdIt.distance = rg.get(useDistLimits);
				pds.push_back(pdIt);
			}

			Array<PDIt> otherTilesPDs;
			for_every_ptr(cat, checkAgainstTiles)
			{
				VectorInt2 offset = -(tileAt * tileSize);
				for_every(catPD, cat->knownPilgrimageDevices)
				{
					if (catPD->device.get() == pd)
					{
						PDIt pdIt;
						pdIt.at = (catPD->at + offset).to_vector2() + Vector2::half;
						pdIt.moveBy = Vector2::zero;
						// no distance
						otherTilesPDs.push_back(pdIt);
					}
				}
			}

			// we should use same order
			sort(otherTilesPDs);

#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
			output(TXT("[distanced pd] pds:"));
			for_every(pd, pds)
			{
				output(TXT("[distanced pd]    %03i %.3fx%.3f"), for_everys_index(pd), pd->at.x, pd->at.y);
			}
			output(TXT("[distanced pd] other tiles pds:"));
			for_every(pd, otherTilesPDs)
			{
				output(TXT("[distanced pd]    %03i %.3fx%.3f"), for_everys_index(pd), pd->at.x, pd->at.y);
			}

			if (atXLine.is_set())
			{
				output(TXT("[distanced pd]    atXLine %i"), atXLine.get());
			}
			else
			{
				output(TXT("[distanced pd]    atXLine not used"));
			}
			if (atYLine.is_set())
			{
				output(TXT("[distanced pd]    atYLine %i"), atYLine.get());
			}
			else
			{
				output(TXT("[distanced pd]    atYLine not used"));
			}
#endif

			// move around and settle
			for_count(int, itIdx, 1000)
			{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_DETAILED_ITERATION
				output(TXT("[distanced pd] iteration %i"), itIdx);
#endif
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
				dv->start_gathering_data();
				dv->clear();
#endif
				for_every(pd, pds)
				{
					pd->moveBy = Vector2::zero;
					if (dpds.forcedAt.is_index_valid(for_everys_index(pd)))
					{
						pd->at = dpds.forcedAt[for_everys_index(pd)].to_vector2();
					}
					else
					{
						for_count(int, checkAgainst, 2)
						{
							float moveAwayFactor = checkAgainst == 0 ? 0.5f : 2.0f;
							float moveCloserStrength = checkAgainst == 0 ? 0.005f : 0.0001f;

							auto& checkAgainstPDS = checkAgainst == 0 ? pds : otherTilesPDs;

							for_every(opd, checkAgainstPDS)
							{
								if (pd == opd) continue;

								float minDistOfBoth = min(pd->distance, opd->distance);
								float maxDistOfBoth = max(pd->distance, opd->distance);
								float distMin = checkAgainst == 0 ? 0.6f * minDistOfBoth + 0.4f * maxDistOfBoth : minDistOfBoth;
								float distMax = maxDistOfBoth * 2.0f;

								Vector2 to = opd->at - pd->at;
								Vector2 toDir = to.normal();
								float toDist = to.length();
								if (toDist < distMin)
								{
									pd->moveBy += -toDir * moveAwayFactor * (1.0f + 1.0f * ((distMin - toDist) / distMin));
								}
								if (toDist > distMax)
								{
									pd->moveBy += toDir * moveCloserStrength * (0.3f + 0.7f * (distMax / toDist));
								}
							}
						}
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_DETAILED_ITERATION
						output(TXT("[distanced pd]   %i move by %.3fx%.3f"), for_everys_index(pd), pd->moveBy.x, pd->moveBy.y);
#endif
					}
				}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
				dv->add_range2(tileRangeLocalF, Colour::black);
#endif

				// move all a bit
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
				Colour colour = Colour::green;
				String pdIdx = String::printf(TXT("%i"), for_everys_index(pd));
#endif
				for_every(pd, pds)
				{
					pd->moveBy *= 0.2f;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
					dv->add_circle(pd->at, 0.5f, colour);
					dv->add_circle(pd->at, pd->distance, Colour::grey);
					dv->add_arrow(pd->at, pd->at + pd->moveBy, Colour::blue);
					dv->add_text(pd->at, pdIdx, Colour::black);
#endif
					pd->at += pd->moveBy;
					// we add halves as we're dealing with cells
					if (atXLine.is_set()) pd->at.x = (float)atXLine.get() + 0.5f;
					if (atYLine.is_set()) pd->at.y = (float)atYLine.get() + 0.5f;
					if (dpds.forcedAt.is_index_valid(for_everys_index(pd)))
					{
						pd->at = dpds.forcedAt[for_everys_index(pd)].to_vector2() + Vector2::half;
					}
				}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
				for_every(pd, otherTilesPDs)
				{
					dv->add_circle(pd->at, 0.5f, Colour::red);
				}
				dv->end_gathering_data();
				if ((itIdx % 200) == 0)
				{
					dv->show_and_wait_for_key();
				}
#endif
			}

			VectorInt2 tileOffset = tileAt * tileSize;
			for_every(ipd, pds)
			{
				KnownPilgrimageDeviceInTile kpd;
				kpd.at = TypeConversions::Normal::f_i_cells(ipd->at);
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_DETAILED_ITERATION
				output(TXT("[distanced pd] pure kpd %i at %.3fx%.3f -> %ix%i"),
					for_everys_index(ipd),
					ipd->at.x, ipd->at.y,
					kpd.at.x, kpd.at.y);
#endif
				if (tileRangeLocal.does_contain(kpd.at))
				{
					kpd.at += tileOffset;
					kpd.device = pd;
					kpd.subPriority = rg.get_int(10000);
					an_assert(tile->knownPilgrimageDevices.has_place_left());
					if (tile->knownPilgrimageDevices.has_place_left())
					{
						tile->knownPilgrimageDevices.push_back(kpd);
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES
						output(TXT("[distanced pd] add device \"%S\" at %ix%i"), pd->get_name().to_string().to_char(), kpd.at.x, kpd.at.y);
#else
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES_TILES_JUST_ADD_INFO
						output(TXT("[distanced pd] add device \"%S\" at %ix%i"), pd->get_name().to_string().to_char(), kpd.at.x, kpd.at.y);
#endif
#endif
					}
				}
			}
		}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_PD_TILES
		dv->deactivate();
#endif
	}

	return *tile;
}

PilgrimageInstanceOpenWorld::Cell const& PilgrimageInstanceOpenWorld::get_cell_at(VectorInt2 const& _at, int _minPriority) const
{
	return cast_to_nonconst(this)->access_cell_at(_at, _minPriority);
}

PilgrimageInstanceOpenWorld::Cell & PilgrimageInstanceOpenWorld::access_cell_at(VectorInt2 const& _at, int _minPriority)
{
	scoped_call_stack_info_str_printf(TXT("get cell at %ix%i (minprio:%i)"), _at.x, _at.y, _minPriority);

	auto* owDef = pilgrimage->open_world__get_definition();

	VectorInt2 startAt = get_start_at();
	RangeInt2 size = owDef->get_size();

	Optional<int> orderNoStartPrev;

	// cached/existing
	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("get_cell_at : check cached/existing"));

		for_every(cell, cells)
		{
			if (cell->at == _at)
			{
				if (cell->doneForMinPriority != NONE &&
					cell->doneForMinPriority <= _minPriority)
				{
					return *cell;
				}
				orderNoStartPrev = cell->orderNoStart;
			}
		}
	}

	static int orderNoStart = 0;
	static int orderNoEnd = 0;
	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("get_cell_at : order no start/end"));
		if (cells.is_empty())
		{
			orderNoStart = 0;
			orderNoEnd = 0;
		}
	}

	if (!is_cell_address_valid(_at))
	{
		// add empty
		Cell c;
		c.init(_at, *this);
		c.doneForMinPriority = 0;
		Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("get_cell_at : add an invalid cell"));
		cells.push_back(c);
		return cells.get_last();
	}

#ifdef VISUALISE_CELL_GENERATION
	getCellStack.push_back(String::printf(TXT("get cell at %3i x %3i priority %2i"), _at.x, _at.y, _minPriority));
	getCellStackAt.push_back(_at);
#endif

#ifdef VISUALISE_CELL_GENERATION
#ifdef VISUALISE_CELL_GENERATION_PRE
	output(TXT("get cell"));
	for_every(cs, getCellStack)
	{
		output(TXT("  %02i %S"), for_everys_index(cs), cs->to_char());
	}
	debug_visualise_map(_at, false, _at);
#endif
#endif

	Cell cell;
	cell.init(_at, *this);

	orderNoStart++;
	cell.orderNoStart = orderNoStartPrev.get(orderNoStart);
	cell.iteration = 0;

	// generate a new one

	Random::Generator rg = cell.randomSeed;

#ifdef AN_DEVELOPMENT_OR_PROFILER
	showFailDetails = false;
#endif

	int dependencyFails = 0; // it's ok to fail dependency
	int tryIdx = 0;
	bool isOk = false;
	while (! isOk)
	{
		++tryIdx;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if ((tryIdx % 1000) > 990)
		{
			warn_dev_investigate(TXT("may fail, will show fail details now %ix%i"), _at.x, _at.y);
			showFailDetails = true;
		}
#endif
		if ((tryIdx % 1000) == 0)
		{
			warn(TXT("taking too long to get a cell %ix%i"), _at.x, _at.y);
#ifdef AN_DEVELOPMENT_OR_PROFILER
			AN_BREAK;
#else
			static bool sentOnce = false;
			if (!sentOnce)
			{
				String shortText = String::printf(TXT("taking too long to get a cell %ix%i"));
				String moreInfo;
				{
					moreInfo += TXT("taking too long to get a cell");
					List<String> list = build_dev_info();
					for_every(line, list)
					{
						moreInfo += *line + TXT("\n");
					}
				}
				ReportBug::send_quick_note_in_background(shortText, moreInfo);
				sentOnce = true;
			}
#endif
		}
		isOk = true;

		bool forcedCell = false;
#ifdef AN_ALLOW_BULLSHOTS
		if (Framework::BullshotSystem::is_active())
		{
			if (auto* rt = Framework::BullshotSystem::get_cell_region_at(_at))
			{
				cell.set = PilgrimageInstanceOpenWorld::CellSet();
				cell.set.regionType = rt;
				cell.dir = DirFourClockwise::Up;
				cell.usesRule = nullptr;
				cell.usesObjective = nullptr;
				cell.dependentOn.clear();
				forcedCell = true;
			}
		}
#endif
		if (owDef->has_no_cells())
		{
			cell.set = PilgrimageInstanceOpenWorld::CellSet();
			// no region, no room
			cell.dir = DirFourClockwise::Up;
			cell.usesRule = nullptr;
			cell.usesObjective = nullptr;
			cell.dependentOn.clear();
			forcedCell = true;
		}

		memory_zero(cell.exits, sizeof(bool)* DirFourClockwise::NUM);

		if (! forcedCell)
		{
			// so we have a working one
			cell.set = get_primal_set_at(_at, PrimalSet_Normal, cell.iteration, _minPriority);
			CellSet setForRules = get_primal_set_at(_at, PrimalSet_Rules, cell.iteration, _minPriority, true);
			CellSet setForObjectives = get_primal_set_at(_at, PrimalSet_Objectives, cell.iteration, _minPriority, true);

			cell.dir = (DirFourClockwise::Type)rg.get_int(DirFourClockwise::NUM);
			cell.set.fill_local_dependents(cell.localDependents, cell.dependentsRadius, rg);

			cell.usesRule = nullptr;
			cell.usesObjective = nullptr;
			cell.dependentOn.clear();
			for_count(int, d, 4)
			{
				cell.exits[d] = true;
			}

			// objectives, rules
			if (isOk &&
				(cell.at != startAt || owDef->get_starting_cell_set().is_empty()))  // if we have nothing for starting cell, go on with objectives etc
			{
				// setup objective
				if (!cell.usesRule && !cell.usesObjective)
				{
					bool anyObjectiveTried = false;
					bool cellPriorityBelowMin = true;
					if (auto* objective = get_objective_to_create_cell(cell.at))
					{
						bool hasObjective = false;
						if (! hasObjective &&
							! objective->regionTypes.is_empty())
						{
							int idx = objective->regionTypes.get_size() == 1 ? 0 : rg.get_int(objective->regionTypes.get_size());
							
							if (auto* rt = objective->regionTypes[idx].get())
							{
								anyObjectiveTried = true;
								cell.usesObjective = objective;

								cell.set.set(rt);
								hasObjective = true;
							}
						}
						if (! hasObjective &&
							! objective->roomTypes.is_empty())
						{
							int idx = objective->roomTypes.get_size() == 1 ? 0 : rg.get_int(objective->roomTypes.get_size());

							if (auto* rt = objective->roomTypes[idx].get())
							{
								anyObjectiveTried = true;
								cell.usesObjective = objective;

								cell.set.set(rt);
								hasObjective = true;
							}
						}
						if (! hasObjective &&
							! objective->cellTypes.is_empty())
						{
							anyObjectiveTried = true;
							cell.usesObjective = objective;
							hasObjective = true;

							Tags const* cellType = setForObjectives.get_cell_type();

							if (!objective->cellTypes.check(cellType ? *cellType : Tags::none))
							{
#ifdef AN_DEVELOPMENT_OR_PROFILER
								if (showFailDetails)
								{
									output(TXT("failed due to objective cellType"));
									output(TXT("objective->cellTypes: %S"), objective->cellTypes.to_string().to_char());
									output(TXT("cellType: %S"), cellType? cellType->to_string_all().to_char(): TXT("[no cell type]"));
								}
#endif
								isOk = false;
							}
							else
							{
								cell.set = setForObjectives;
							}
						}
						if (! hasObjective)
						{
#ifdef AN_DEVELOPMENT_OR_PROFILER
							if (showFailDetails)
							{
								output(TXT("nothing for objective"));
							}
#endif
							an_assert(false, TXT("nothing for objective!"));
							isOk = false;
						}

						int cellPriority = cell.set.get_cell_priority();
						if (cellPriority >= _minPriority)
						{
							cellPriorityBelowMin = false;
						}
					}
					if (anyObjectiveTried && cellPriorityBelowMin)
					{
						// no need to look any further for time being
						break;
					}
				}
				// setup rule
				if (!cell.usesRule && !cell.usesObjective)
				{
					bool anyRuleTried = false;
					bool cellPriorityBelowMin = true;
					for_every(rule, owDef->get_rules())
					{
						if (rule->type == PilgrimageOpenWorldDefinition::Rule::BorderNear ||
							rule->type == PilgrimageOpenWorldDefinition::Rule::BorderFar ||
							rule->type == PilgrimageOpenWorldDefinition::Rule::BorderLeft ||
							rule->type == PilgrimageOpenWorldDefinition::Rule::BorderRight)
						{
							Random::Generator rg = randomSeed;
							rg.advance_seed(9753 * (2 + for_everys_index(rule)), 9876 * (4 + 3 * for_everys_index(rule)));
							rg.advance_seed(cell.at.x * 6345,
								 rule->type == PilgrimageOpenWorldDefinition::Rule::BorderNear ? 7934697 :
								(rule->type == PilgrimageOpenWorldDefinition::Rule::BorderFar ? 85973 :
								(rule->type == PilgrimageOpenWorldDefinition::Rule::BorderLeft ? 52634 :
								(rule->type == PilgrimageOpenWorldDefinition::Rule::BorderRight ? 97342 : 0))));
							int depth = rg.get(rule->depth);
							bool applyRule = false;
							if (!size.y.is_empty())
							{
								if (rule->type == PilgrimageOpenWorldDefinition::Rule::BorderNear)
								{
									applyRule = cell.at.y >= size.y.min && cell.at.y < size.y.min + depth;
								}
								if (rule->type == PilgrimageOpenWorldDefinition::Rule::BorderFar)
								{
									applyRule = cell.at.y > size.y.max - depth && cell.at.y <= size.y.max;
								}
							}
							if (!size.x.is_empty())
							{
								if (rule->type == PilgrimageOpenWorldDefinition::Rule::BorderLeft)
								{
									applyRule = cell.at.x >= size.x.min && cell.at.x < size.x.min + depth;
								}
								if (rule->type == PilgrimageOpenWorldDefinition::Rule::BorderRight)
								{
									applyRule = cell.at.x > size.x.max - depth && cell.at.x <= size.x.max;
								}
							}
							if (applyRule)
							{
								anyRuleTried = true;

								cell.usesRule = rule;

								Tags const* cellType = setForRules.get_cell_type();

								int cellPriority = cell.set.get_cell_priority();
								if (cellPriority >= _minPriority)
								{
									cellPriorityBelowMin = false;
								}

								if (!rule->cellTypes.check(cellType ? *cellType : Tags::none))
								{
#ifdef AN_DEVELOPMENT_OR_PROFILER
									if (showFailDetails)
									{
										output(TXT("failed due to rule's cellType"));
									}
#endif
									isOk = false;
								}
								else
								{
									cell.set = setForRules;
								}
							}
						}
					}
					if (anyRuleTried && cellPriorityBelowMin)
					{
						// no need to look any further for time being
						break;
					}
				}
			}
		}

		if (auto* ls = cell.set.get_as_any())
		{
			cell.mainRoomType = cell.set.roomType.get();
			{
				Framework::LibraryName mrtName = ls->get_custom_parameters().get_value<Framework::LibraryName>(NAME(cellMainRoomType), Framework::LibraryName::invalid());
				if (mrtName.is_valid())
				{
					if (auto* mrt = Framework::Library::get_current()->get_room_types().find(mrtName))
					{
						cell.mainRoomType = mrt;
					}
				}
			}
			if (auto* rt = cell.mainRoomType.get()) { rt->load_on_demand_if_required(); }

			if (ls->get_custom_parameters().get_value<bool>(NAME(dirFixed), false))
			{
				cell.dir = DirFourClockwise::Up;
			}
			{
				int forcedDir = ls->get_custom_parameters().get_value<int>(NAME(forceDir), NONE);
				if (forcedDir != NONE)
				{
					cell.dir = (DirFourClockwise::Type)forcedDir;
				}
			}
			if (cell.usesObjective &&
				cell.usesObjective->atDir.is_set())
			{
				cell.dir = cell.usesObjective->atDir.get();
			}
			{
				cell.specialLocalDir = (DirFourClockwise::Type)rg.get_int(DirFourClockwise::NUM);
				int specialLocalDir = ls->get_custom_parameters().get_value<int>(NAME(specialLocalDir), NONE);
				if (specialLocalDir != NONE)
				{
					cell.specialLocalDir = (DirFourClockwise::Type)specialLocalDir;
				}
			}
			int cellPriority = cell.set.get_cell_priority();
			if (cellPriority < _minPriority)
			{
				// no need to look any further
				break;
			}

			struct LocalCentre
			{
				int temp;
				static int world_to_rel_centre(int _a)
				{
					_a += 16;
					_a = mod(_a, 32);
					_a -= 16;
					return _a;
				}
				static bool is_rel_centre(VectorInt2 _a)
				{
					_a.x = world_to_rel_centre(_a.x);
					_a.y = world_to_rel_centre(_a.y);
					return _a.is_zero();
				}
				static bool are_neighbours(VectorInt2 _a, VectorInt2 _b) // if they are not neighours, they are from different cells/centres
				{
					_a.x = world_to_rel_centre(_a.x);
					_a.y = world_to_rel_centre(_a.y);
					_b.x = world_to_rel_centre(_b.x);
					_b.y = world_to_rel_centre(_b.y);
					VectorInt2 d = _a - _b;
					return abs(d.x) <= 1 && abs(d.y) <= 1;
				}
			};

			// check dependency - higher priority required! if anything around could override us or change our direction
			if (!forcedCell &&
				isOk && ! cell.usesRule && ! cell.usesObjective &&
				!LocalCentre::is_rel_centre(_at)) // rel centre can be whatever it wants to be
			{
				// find candidates for being overriden first and sort them so the most important are first
				struct Candidate
				{
					VectorInt2 at;
					VectorInt2 relAt;
					int cellPriority;

					static inline int compare(void const* _a, void const* _b)
					{
						Candidate const & a = *plain_cast<Candidate>(_a);
						Candidate const & b = *plain_cast<Candidate>(_b);
						if (a.cellPriority > b.cellPriority) return A_BEFORE_B;
						if (a.cellPriority < b.cellPriority) return B_BEFORE_A;
						int distA = abs(a.relAt.x) + abs(a.relAt.y);
						int distB = abs(b.relAt.x) + abs(b.relAt.y);
						if (distA < distB) return A_BEFORE_B;
						if (distA > distB) return B_BEFORE_A;
						if (a.relAt.x < b.relAt.x) return A_BEFORE_B;
						if (a.relAt.x > b.relAt.x) return B_BEFORE_A;
						if (a.relAt.y < b.relAt.y) return A_BEFORE_B;
						if (a.relAt.y > b.relAt.y) return B_BEFORE_A;

						return A_AS_B;
					}
				};
				ArrayStatic<Candidate, (MAX_OVERRIDE_DIST * 2 + 1) * (MAX_OVERRIDE_DIST * 2 + 1)> candidates; SET_EXTRA_DEBUG_INFO(candidates, TXT("PilgrimageInstanceOpenWorld::access_cell_at.candidates"));
				{
					scoped_call_stack_info(TXT("check for overrides"));
					for_range(int, oy, -MAX_OVERRIDE_DIST, MAX_OVERRIDE_DIST)
					{
						for_range(int, ox, -MAX_OVERRIDE_DIST, MAX_OVERRIDE_DIST)
						{
							if (ox != 0 || oy != 0)
							{
								int dist = TypeConversions::Normal::f_i_cells(VectorInt2(ox, oy).to_vector2().length());
								if (dist <= MAX_OVERRIDE_DIST)
								{
									VectorInt2 at(_at.x + ox, _at.y + oy);
									Cell const& c = get_cell_at(at, cellPriority + 1);
									if (c.set.get_as_any())
									{
										int cCellPriority = c.set.get_cell_priority();
										if (cCellPriority > cellPriority)
										{
											Candidate cand;
											cand.at = at;
											cand.relAt = at - _at;
											cand.cellPriority = cCellPriority;
											candidates.push_back(cand);
										}
									}
								}
							}
						}
					}
				}
				sort(candidates);

				Tags const* cellType = cell.set.get_cell_type();
				TagCondition const* dependsOnCellType = cell.set.get_depends_on_cell_type();
				struct DependencyContext
				{
					Optional<VectorInt2> dependentOn;
					Optional<int> forCellPriority;
					bool dependsOnCellTypeOk = false;
					Optional<DirFourClockwise::Type> forcedDir;
				} dependencyContext;

				for_every(cand, candidates)
				{
					scoped_call_stack_info(TXT("check candidate"));

					if (dependencyContext.forCellPriority.is_set() &&
						dependencyContext.forCellPriority.get() > cand->cellPriority)
					{
						// lesser priority is not important to us
						break;
					}
					VectorInt2 at(cand->at);
					Cell const& c = get_cell_at(at, cellPriority + 1);
					bool isDependent = false;

					int worldDependents[4];
					c.get_dependents_world(worldDependents);
					int iterationStep = 100;
					if (cell.iteration >= iterationStep)
					{
						// there might be multiple sources of dependency
						// lower the distance to make the dependency influence weaker
						int makeSmaller = cell.iteration / iterationStep;
						while (makeSmaller)
						{
							for_count(int, i, 4) worldDependents[i] = max(0, worldDependents[i] - 1);
							--makeSmaller;
						}
					}
					if (_at.x == at.x && _at.y > at.y && _at.y <= at.y + worldDependents[DirFourClockwise::Up]) isDependent = true;
					if (_at.y == at.y && _at.x > at.x && _at.x <= at.x + worldDependents[DirFourClockwise::Right]) isDependent = true;
					if (_at.x == at.x && _at.y < at.y && _at.y >= at.y - worldDependents[DirFourClockwise::Down]) isDependent = true;
					if (_at.y == at.y && _at.x < at.x && _at.x >= at.x - worldDependents[DirFourClockwise::Left]) isDependent = true;

					if (c.dependentsRadius > 0)
					{
						int dist = TypeConversions::Normal::f_i_cells((_at - at).to_vector2().length());
						if (dist <= c.dependentsRadius)
						{
							isDependent = true;
						}
					}

					if (isDependent)
					{
						bool thisOk = true;

						if (thisOk && dependsOnCellType)
						{
							thisOk = dependsOnCellType->is_empty();
							Tags const* ct = c.set.get_cell_type();
							if (dependsOnCellType->check(ct? *ct : Tags::none))
							{
								thisOk = true;
							}
						}

						if (thisOk)
						{
							if (TagCondition const* tc = c.set.get_dependent_cell_types())
							{
								if (!tc->check(cellType? *cellType : Tags::none))
								{
									thisOk = false;
								}
								/*	this code seems to not be required and with priorities set to work this way may actually leave blank spots
								if (thisOk)
								{
									// check if there's something on the way that doesn't match - only higher priorities!
									for_count(int, d, 4)
									{
										VectorInt2 dir = DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)d);
										if (thisOk && worldDependents[d] > 1)
										{
											for_range(int, i, 1, worldDependents[d] - 1)
											{
												Cell const& ow = get_cell_at(at + dir * i, cellPriority + 1);
												if (ow.set.get_as_any() &&
													ow.set.get_cell_priority() > cellPriority)
												{
													bool blocks = true;
													auto* owType = ow.set.get_cell_type();
													if (tc->check(owType? *owType : Tags::none))
													{
														blocks = false;
													}
													if (blocks)
													{
														thisOk = false;
														break;
													}
												}
											}
										}
									}
								}
								*/
							}
						}

						Optional<DirFourClockwise::Type> newDir;
						if (thisOk)
						{
							bool dirAlign = c.set.should_dir_align_dependent();
							bool axisAlign = c.set.should_axis_align_dependent();
							bool alignTowards = c.set.should_align_dependent_towards();
							bool alignAway = c.set.should_align_dependent_away();
							if (dirAlign || axisAlign || alignTowards || alignAway)
							{
								if (dirAlign)
								{
									newDir = c.dir;
								}
								else if (axisAlign)
								{
									newDir = c.dir;
									if (rg.get_bool())
									{
										newDir = DirFourClockwise::opposite_dir(newDir.get());
									}
								}
								else
								{
									// assume away, switch later if not
									VectorInt2 relAt = _at - at;
									if (abs(relAt.x) >= abs(relAt.y))
									{
										newDir = relAt.x > 0 ? DirFourClockwise::Right : DirFourClockwise::Left;
									}
									else
									{
										newDir = relAt.y > 0 ? DirFourClockwise::Up : DirFourClockwise::Down;
									}
									if (alignAway)
									{
										newDir = DirFourClockwise::opposite_dir(newDir.get());
									}
								}
								an_assert(newDir.is_set());
								if (dependencyContext.forcedDir.is_set() &&
									!DirFourClockwise::same_axis_dir(dependencyContext.forcedDir.get(), newDir.get()))
								{
									thisOk = false;
								}
							}
						}

						if (thisOk)
						{
							dependencyContext.forcedDir = newDir;
							dependencyContext.dependentOn = at;
							if (dependsOnCellType)
							{
								// had to be checked earlier
								dependencyContext.dependsOnCellTypeOk = true;
							}
							dependencyContext.forCellPriority = cand->cellPriority;
						}
						else
						{
							++ dependencyFails;
							if (dependencyFails < 100)
							{
#ifdef AN_DEVELOPMENT_OR_PROFILER
								if (showFailDetails)
								{
									output(TXT("couldn't align"));
								}
#endif
								isOk = false;
							}
							else
							{
#ifdef AN_DEVELOPMENT_OR_PROFILER
								output(TXT("dependency failed but we allow it"));
#endif
							}
						}
					}
				}

				if (dependsOnCellType &&
					!dependencyContext.dependsOnCellTypeOk)
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (showFailDetails)
					{
						output(TXT("depends on something, couldn't meet"));
					}
#endif
					isOk = false;
				}

				if (isOk)
				{
					if (dependencyContext.dependentOn.is_set())
					{
						cell.dependentOn = dependencyContext.dependentOn.get();
					}
					if (dependencyContext.forcedDir.is_set())
					{
						cell.dir = dependencyContext.forcedDir.get();
					}
				}
			}

			// check if there are exit conflicts with neighbours (something closer to rel.centre (local 0,0) or a higher priority)
			if (!forcedCell &&
				isOk && !LocalCentre::is_rel_centre(_at)) // we do not care if we're rel.centre, we're the most important!
			{
				// check if exits are allowed
				for_count(int, d, 4)
				{
					scoped_call_stack_info(TXT("check if exits are allowed"));

					int priorityReq = cellPriority;
					VectorInt2 at = _at;
					{
						if (d == DirFourClockwise::Up)    { ++at.y; }
						if (d == DirFourClockwise::Right) { ++at.x; }
						if (d == DirFourClockwise::Down)  { --at.y; }
						if (d == DirFourClockwise::Left)  { --at.x; }
					}
					if (!LocalCentre::are_neighbours(at, _at))
					{
						if (d == DirFourClockwise::Up)    { if (at.y > 0) ++priorityReq; }
						if (d == DirFourClockwise::Right) { if (at.x > 0) ++priorityReq; }
						if (d == DirFourClockwise::Down)  { if (at.y < 0) ++priorityReq; }
						if (d == DirFourClockwise::Left)  { if (at.x < 0) ++priorityReq; }
					}
					else
					{
						if (d == DirFourClockwise::Up)    { if (LocalCentre::world_to_rel_centre(at.y) > 0) ++priorityReq; }
						if (d == DirFourClockwise::Right) { if (LocalCentre::world_to_rel_centre(at.x) > 0) ++priorityReq; }
						if (d == DirFourClockwise::Down)  { if (LocalCentre::world_to_rel_centre(at.y) < 0) ++priorityReq; }
						if (d == DirFourClockwise::Left)  { if (LocalCentre::world_to_rel_centre(at.x) < 0) ++priorityReq; }
					}
					if (LocalCentre::is_rel_centre(at))
					{
						priorityReq = 0;
					}
					if (LocalCentre::is_rel_centre(_at))
					{
						priorityReq = cellPriority + 1;
					}

					Cell next = get_cell_at(at, priorityReq);
					if (next.set.get_as_any() &&
						(next.set.get_cell_priority() >= priorityReq || LocalCentre::is_rel_centre(at)) &&
						!Cell::is_conflict_free(cell, next))
					{
#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (showFailDetails)
						{
							output(TXT("exit conflict"));
						}
#endif
						isOk = false;
					}
				}
			}

			// fill exits
			if (isOk && _minPriority <= cellPriority)
			{
				for_count(int, d, 4)
				{
					cell.exits[d] = is_cell_address_valid(_at + DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)d));
				}

				bool allowedExits[4];
				bool requiredExits[4];
				bool allPossibleExitsRequired;
				cell.fill_allowed_required_exits(allowedExits, requiredExits, allPossibleExitsRequired);
				cell.fill_preferred_side_exits();

				float cellRemove1ExitChance;
				float cellRemove2ExitsChance;
				float cellRemove3ExitsChance;
				float cellRemove4ExitsChance;
				float cellIgnoreRemovingExitChance;

				owDef->fill_cell_exit_chances(OUT_ cellRemove1ExitChance, OUT_ cellRemove2ExitsChance, OUT_ cellRemove3ExitsChance, OUT_ cellRemove4ExitsChance, OUT_ cellIgnoreRemovingExitChance);
				// now override for this particular cell
				cell.fill_cell_exit_chances(REF_ cellRemove1ExitChance, REF_ cellRemove2ExitsChance, REF_ cellRemove3ExitsChance, REF_ cellRemove4ExitsChance, REF_ cellIgnoreRemovingExitChance);

				bool dirImportantExits[4];
				bool dirImportant;
				cell.fill_dir_important_exits(dirImportantExits, dirImportant);

				// limit exits to allowed only
				for_count(int, i, 4)
				{
					cell.exits[i] &= allowedExits[i];
				}

				if (!allPossibleExitsRequired)
				{
					int removeExits = 0;
					{
						float removeExitsChance = rg.get_float(0.0f, 1.0f);
						if (cellRemove4ExitsChance != 0.0f && removeExitsChance <= cellRemove4ExitsChance)
						{
							removeExits = 4;
						}
						else if (cellRemove3ExitsChance != 0.0f && removeExitsChance <= cellRemove3ExitsChance)
						{
							removeExits = 3;
						}
						else if (cellRemove2ExitsChance != 0.0f && removeExitsChance <= cellRemove2ExitsChance)
						{
							removeExits = 2;
						}
						else if (cellRemove1ExitChance != 0.0f && removeExitsChance <= cellRemove1ExitChance)
						{
							removeExits = 1;
						}
					}
					int requiredExitsCount = (requiredExits[0] ? 1 : 0) + (requiredExits[1] ? 1 : 0) + (requiredExits[2] ? 1 : 0) + (requiredExits[3] ? 1 : 0);
					while (removeExits)
					{
						int exitsCount = (cell.exits[0] ? 1 : 0) + (cell.exits[1] ? 1 : 0) + (cell.exits[2] ? 1 : 0) + (cell.exits[3] ? 1 : 0);
						if (exitsCount == 0 || requiredExitsCount >= exitsCount)
						{
							break;
						}

						int exitIdx = rg.get_int(4);
						if (!requiredExits[exitIdx] &&
							cell.exits[exitIdx])
						{
							cell.exits[exitIdx] = false;
							--removeExits;
						}
						else if (rg.get_chance(cellIgnoreRemovingExitChance))
						{
							// treat it as "we might have removed it"
							--removeExits;
						}
					}
				}

				// propagate from neighbours (closer to 0,0 or higher priority)
				// check if exits are allowed
				if (!LocalCentre::is_rel_centre(_at)) // we do not care if we're rel.centre, we're the most important!)
				{
					for_count(int, d, 4)
					{
						scoped_call_stack_info(TXT("propagate from neighbours"));

						int priorityReq = cellPriority;
						VectorInt2 at = _at;
						{
							if (d == DirFourClockwise::Up)    { ++at.y; }
							if (d == DirFourClockwise::Right) { ++at.x; }
							if (d == DirFourClockwise::Down)  { --at.y; }
							if (d == DirFourClockwise::Left)  { --at.x; }
						}
						if (!LocalCentre::are_neighbours(at, _at))
						{
							if (d == DirFourClockwise::Up)    { if (at.y > 0) ++priorityReq; }
							if (d == DirFourClockwise::Right) { if (at.x > 0) ++priorityReq; }
							if (d == DirFourClockwise::Down)  { if (at.y < 0) ++priorityReq; }
							if (d == DirFourClockwise::Left)  { if (at.x < 0) ++priorityReq; }
						}
						else
						{
							if (d == DirFourClockwise::Up)    { if (LocalCentre::world_to_rel_centre(at.y) > 0) ++priorityReq; }
							if (d == DirFourClockwise::Right) { if (LocalCentre::world_to_rel_centre(at.x) > 0) ++priorityReq; }
							if (d == DirFourClockwise::Down)  { if (LocalCentre::world_to_rel_centre(at.y) < 0) ++priorityReq; }
							if (d == DirFourClockwise::Left)  { if (LocalCentre::world_to_rel_centre(at.x) < 0) ++priorityReq; }
						}
						if (LocalCentre::is_rel_centre(at))
						{
							priorityReq = 0;
						}
						if (LocalCentre::is_rel_centre(_at))
						{
							priorityReq = cellPriority + 1;
						}

						Cell next = get_cell_at(at, priorityReq);
						if (next.set.get_as_any() &&
							(next.set.get_cell_priority() >= priorityReq ||
							LocalCentre::is_rel_centre(at)))
						{
							cell.exits[d] = next.exits[DirFourClockwise::opposite_dir((DirFourClockwise::Type)d)];

							// if both are dir dependent and dir mismatches, invalidate this one
							if (dirImportant && cell.dir != next.dir)
							{
								bool nextDirImportantExits[4];
								bool nextDirImportant;
								next.fill_dir_important_exits(nextDirImportantExits, nextDirImportant);
								if (nextDirImportant)
								{
									if (dirImportantExits[d] && nextDirImportantExits[DirFourClockwise::opposite_dir((DirFourClockwise::Type)d)])
									{
#ifdef AN_DEVELOPMENT_OR_PROFILER
										if (showFailDetails)
										{
											output(TXT("both dir dependent and dir mismatches"));
										}
#endif
										isOk = false;
									}
								}
							}
						}
					}
				}

				if (isOk)
				{
					// after propagation check if possible/valid
					for_count(int, i, 4)
					{
						if (requiredExits[i])
						{
							if (allowedExits[i] ^ cell.exits[i])
							{
#ifdef AN_DEVELOPMENT_OR_PROFILER
								if (showFailDetails)
								{
									output(TXT("doesn't have a required exit"));
								}
#endif
								// can't have this exit
								isOk = false;
							}
						}
						else
						{
							if (!allowedExits[i] && cell.exits[i])
							{
#ifdef AN_DEVELOPMENT_OR_PROFILER
								if (showFailDetails)
								{
									output(TXT("can't have this exit, not allowed"));
								}
#endif
								// can't have this exit
								isOk = false;
							}
						}
					}
				}
			}

			if (isOk && !pilgrimage->open_world__get_definition()->has_no_cells())
			{
				// objectives need an exit, a normal exit (unless not an actual objective)
				if (!pilgrimage->open_world__get_definition()->are_authorised_cells_unreachable())
				{
					if ((cell.usesObjective && ! cell.usesObjective->notActualObjective) || ! pilgrimage->open_world__get_definition()->should_enforce_all_cells_reachable())
					{
						isOk = false;
						for_count(int, i, 4)
						{
							if (cell.exits[i])
							{
								isOk = true;
							}
						}
						if (!isOk)
						{
#ifdef AN_DEVELOPMENT_OR_PROFILER
							if (showFailDetails)
							{
								output(TXT("no exit"));
							}
#endif
						}
					}
				}
			}
		}

		if (isOk && cell.set.get_as_any()) // we may have nothing here as we're checking a higher priority
		{
			// check against centre based - if they prepared, they have to match!
			for_every(dpd, owDef->get_distanced_pilgrimage_devices())
			{
				if (!dpd->centreBased.use)
				{
					continue;
				}
				if (!GameDefinition::check_chosen(dpd->forGameDefinitionTagged))
				{
					continue;
				}

				auto& tile = get_pilgrimage_devices_tile_at(for_everys_index(dpd), _at);

				for_every(pd, tile.knownPilgrimageDevices)
				{
					if (pd->at == _at)
					{
						if (! pd->device->get_open_world_cell_tag_condition().check(cell.set.get_as_any()->get_tags()))
						{
							isOk = false;
						}
					}
				}
			}
		}

		// we're done or not?
		if (isOk)
		{
			// get additional:
			//	additional cell info
			//	line models
			//	colours for map
			cell.additionalCellInfo = cell.set.choose_additional_cell_info(cell.randomSeed);
			cell.mapColour = cell.set.get_map_colour_for_map();
			// will be filled for high level region if added later
			cell.lineModel = cell.set.get_line_model_for_map();
			cell.lineModelAdditional = cell.set.get_line_model_for_map_additional(cell.additionalCellInfo.get());
			cell.lineModelAlwaysOtherwise = cell.set.get_line_model_always_otherwise_for_map();
			cell.objectiveLineModels.clear();
			cell.objectiveLineModelsIfKnown.clear();
			for_every(o, pilgrimage->open_world__get_definition()->get_objectives())
			{
				if (is_objective_ok_at(o, cell.at))
				{
					if (o->lineModel.get())
					{
						cell.objectiveLineModels.push_back(o->lineModel);
					}
					if (o->lineModelIfKnown.get())
					{
						cell.objectiveLineModelsIfKnown.grow_size(1);
						auto& lm = cell.objectiveLineModelsIfKnown.get_last();
						lm.lineModel = o->lineModelIfKnown;
						lm.distance = o->lineModelIfKnownDistance;
					}
				}
			}
		}
		else
		{
			++cell.iteration;
		}
	}

	orderNoEnd++;
	cell.orderNoEnd = orderNoEnd;
	cell.doneForMinPriority = _minPriority;

	// new one?
	if (cell.doneForMinPriority == 0 && ! cell.knownPilgrimageDevicesAdded)
	{
		cell.rotateSlots = rg.get_int(4);
		cell.knownPilgrimageDevicesAdded = true;
		add_pilgrimage_devices(&cell);
	}

#ifdef OUTPUT_CELL_GENERATION_DETAILS
	output(TXT("get cell %i x %i : seed %S : %S"), _at.x, _at.y, cell.randomSeed.get_seed_string().to_char(), cell.set.get_as_any()? cell.set.get_as_any()->get_name().to_string().to_char() : TXT("--"));
#endif

	if (owDef->is_whole_map_known())
	{
		Concurrency::ScopedSpinLock lock(knownCellsLock);
		knownCells.push_back_unique(_at);
	}

	// update for a lower priority (if it would be a higher one, it would be already stored)
	{
		Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("get_cell_at : existing lower priority cell"));
		for_every(c, cells)
		{
			if (c->at == _at &&
				c->doneForMinPriority >= _minPriority)
			{
				cell.mapColour = c->mapColour; // preserve, it is set later usually
				*c = cell;
#ifdef VISUALISE_CELL_GENERATION
				debug_visualise_map(_at, false, _at);
				getCellStack.pop_back();
				getCellStackAt.pop_back();
#endif

				return *c;
			}
		}
	}

	// or store a new one
	{
		Concurrency::ScopedMRSWLockWrite lock(cellsLock, TXT("get_cell_at : add a cell"));
		cells.push_back(cell);
	}

#ifdef VISUALISE_CELL_GENERATION
	debug_visualise_map(_at, false, _at);
	getCellStack.pop_back();
	getCellStackAt.pop_back();
#endif

	// fill missing bits that may require high level region
	{
		auto& cell = cells.get_last();
		if (!cell.mapColour.is_set())
		{
			if (auto* regionType = async_get_high_level_region_type_for(cell.at))
			{
				if (auto* c = regionType->get_custom_parameters().get_existing<Colour>(NAME(forMapColour)))
				{
					cell.mapColour = *c;
				}
			}
		}
	}

	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("get_cell_at : return last cell"));
		return cells.get_last();
	}
}

DirFourClockwise::Type PilgrimageInstanceOpenWorld::dir_to_world(DirFourClockwise::Type _localDir, Framework::Region* _region) const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("dir to world"));
	for_every(ec, existingCells)
	{
		if (_region->is_in_region(ec->region.get()))
		{
			return DirFourClockwise::local_to_world(_localDir, ec->dir);
		}
	}

	an_assert(false, TXT("no region available"));
	return _localDir;
}

DirFourClockwise::Type PilgrimageInstanceOpenWorld::dir_to_world(DirFourClockwise::Type _localDir, Framework::Room* _room) const
{
	return dir_to_world(_localDir, _room->get_in_region());
}

DirFourClockwise::Type PilgrimageInstanceOpenWorld::dir_to_local(DirFourClockwise::Type _worldDir, Framework::Region* _region) const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("dir to local"));
	for_every(ec, existingCells)
	{
		if (_region->is_in_region(ec->region.get()))
		{
			return DirFourClockwise::world_to_local(_worldDir, ec->dir);
		}
	}

	an_assert(false, TXT("no region available"));
	return _worldDir;
}

DirFourClockwise::Type PilgrimageInstanceOpenWorld::dir_to_local(DirFourClockwise::Type _worldDir, Framework::Room* _room) const
{
	return dir_to_local(_worldDir, _room->get_in_region());
}

void place_connector_tags_at(Array<Name>& _distributedConnectorTags, Name const& _connectorTag, int _at, int _amount)
{
	int tried = 0;
	_at = mod(_at, _distributedConnectorTags.get_size());
	while (_amount && tried < _distributedConnectorTags.get_size())
	{
		if (!_distributedConnectorTags[_at].is_valid())
		{
			_distributedConnectorTags[_at] = _connectorTag;
			--_amount;
		}
		_at = mod(_at + 1, _distributedConnectorTags.get_size());
		++tried;
	}
}

Name const & PilgrimageInstanceOpenWorld::dir_to_connector_tag(DirFourClockwise::Type _dir)
{
	if (_dir == DirFourClockwise::Up) return NAME(openWorldUp);
	if (_dir == DirFourClockwise::Right) return NAME(openWorldRight);
	if (_dir == DirFourClockwise::Down) return NAME(openWorldDown);
	if (_dir == DirFourClockwise::Left) return NAME(openWorldLeft);
	return Name::invalid();
}

Optional<DirFourClockwise::Type> PilgrimageInstanceOpenWorld::connector_tag_to_dir(Name const & _connectorTag)
{
	if (_connectorTag == NAME(openWorldUp)) return DirFourClockwise::Up;
	if (_connectorTag == NAME(openWorldRight)) return DirFourClockwise::Right;
	if (_connectorTag == NAME(openWorldDown)) return DirFourClockwise::Down;
	if (_connectorTag == NAME(openWorldLeft)) return DirFourClockwise::Left;
	return NP;
}

Name const& PilgrimageInstanceOpenWorld::dir_to_layout_cue_tag(DirFourClockwise::Type _dir)
{
	if (_dir == DirFourClockwise::Up)		return NAME(layoutCueUp);
	if (_dir == DirFourClockwise::Right)	return NAME(layoutCueRight);
	if (_dir == DirFourClockwise::Down)		return NAME(layoutCueDown);
	if (_dir == DirFourClockwise::Left)		return NAME(layoutCueLeft);
	return Name::invalid();
}

Name const& PilgrimageInstanceOpenWorld::dir_to_layout_cue_cap_tag(DirFourClockwise::Type _dir)
{
	if (_dir == DirFourClockwise::Up)		return NAME(layoutCueCapUp);
	if (_dir == DirFourClockwise::Right)	return NAME(layoutCueCapRight);
	if (_dir == DirFourClockwise::Down)		return NAME(layoutCueCapDown);
	if (_dir == DirFourClockwise::Left)		return NAME(layoutCueCapLeft);
	return Name::invalid();
}

void PilgrimageInstanceOpenWorld::distribute_connector_tags_evenly(Array<Name>& _connectorTags, OUT_ Array<Name>& _distributedConnectorTags, OUT_ Array<DirFourClockwise::Type>& _distributedDirs, bool _looped,
	Optional<DirFourClockwise::Type> const & _worldBreakerDir, Optional<DirFourClockwise::Type> const & _preferredFirstDir,
	int _count, PilgrimageInstanceOpenWorldConnectorTagDistribution::Type _distribution, Random::Generator const& _seed)
{
	an_assert(!_worldBreakerDir.is_set() || !_preferredFirstDir.is_set(), TXT("don't mix _worldBreakerDir and _preferredFirstDir"));
	an_assert(!_preferredFirstDir.is_set() || _looped, TXT("use _preferredFirstDir only with loooped"));

	_distributedConnectorTags.set_size(_count);
	_distributedDirs.set_size(_count);

	int up = 0;
	int down = 0;
	int left = 0;
	int right = 0;
	int special = 0;

	for_every(c, _connectorTags)
	{
		if (*c == NAME(openWorldLeft)) ++left; else
		if (*c == NAME(openWorldRight)) ++right; else
		if (*c == NAME(openWorldDown)) ++down; else
		if (*c == NAME(openWorldUp)) ++up; else
		if (*c == NAME(openWorldSpecial)) ++special; else
		{
			error(TXT("not recognised connector tag \"%S\""), c->to_char());
		}
	}

	// we only support one door per direction
	an_assert(up <= 1);
	an_assert(down <= 1);
	an_assert(left <= 1);
	an_assert(right <= 1);
	an_assert(special <= 1);

	int total = up + down + left + right + special;
	int totalWithoutSpecial = up + down + left + right;
	bool useSpecial = totalWithoutSpecial == _count; // auto assume use of special
	if (!useSpecial)
	{
		total = totalWithoutSpecial;
	}
	an_assert(total <= _count);
	if (total > _count)
	{
		error(TXT("not enought places to use connector tags"));
	}

	int firstCount = up;

	Random::Generator rg = _seed;
	rg.advance_seed(783463, 19752);

	todo_note(TXT("no support for specialLocalDir yet, not used"));

	if (_distribution == PilgrimageInstanceOpenWorldConnectorTagDistribution::Clockwise ||
		_distribution == PilgrimageInstanceOpenWorldConnectorTagDistribution::CounterClockwise)
	{
		float count = (float)_count;
		int breakArrayAt = 0;

		int upAt = NONE;
		int rightAt = NONE;
		int downAt = NONE;
		int leftAt = NONE;

		if (_looped)
		{
			upAt = TypeConversions::Normal::f_i_closest(count * 0.0f / 4.0f);
			rightAt = TypeConversions::Normal::f_i_closest(count * 1.0f / 4.0f);
			downAt = TypeConversions::Normal::f_i_closest(count * 2.0f / 4.0f);
			leftAt = TypeConversions::Normal::f_i_closest(count * 3.0f / 4.0f);
			place_connector_tags_at(_distributedConnectorTags, NAME(openWorldUp), upAt, up);
			place_connector_tags_at(_distributedConnectorTags, NAME(openWorldRight), rightAt, right);
			place_connector_tags_at(_distributedConnectorTags, NAME(openWorldDown), downAt, down);
			place_connector_tags_at(_distributedConnectorTags, NAME(openWorldLeft), leftAt, left);
			if (_preferredFirstDir.is_set())
			{
				switch (_preferredFirstDir.get())
				{
				case DirFourClockwise::Up:		firstCount = up;	breakArrayAt = upAt;	break;
				case DirFourClockwise::Right:	firstCount = right;	breakArrayAt = rightAt;	break;
				case DirFourClockwise::Down:	firstCount = down;	breakArrayAt = downAt;	break;
				case DirFourClockwise::Left:	firstCount = left;	breakArrayAt = leftAt;	break;
				default: break;
				}
			}
		}
		else
		{
			bool mixAll = ! _worldBreakerDir.is_set();
			if (_worldBreakerDir.is_set())
			{
				if ((_worldBreakerDir.get() == DirFourClockwise::Up && up > 0) ||
					(_worldBreakerDir.get() == DirFourClockwise::Right && right > 0) || 
					(_worldBreakerDir.get() == DirFourClockwise::Down && down > 0) || 
					(_worldBreakerDir.get() == DirFourClockwise::Left && left > 0))
				{
					warn(TXT("should not use world breaker if there's exit in that dir"));
					mixAll = true;
				}
				else
				{
					float iExit = 0.0f;
					for_count(int, d, 4)
					{
						int at = TypeConversions::Normal::f_i_closest(count * iExit / 3.0f);
						int howMany = 0;
						if (d == 0) { upAt = at; howMany = up; }
						if (d == 1) { rightAt = at; howMany = right; }
						if (d == 2) { downAt = at; howMany = down; }
						if (d == 3) { leftAt = at; howMany = left; }
						if (d != _worldBreakerDir.get())
						{
							Name connectorTag = dir_to_connector_tag((DirFourClockwise::Type)d);
							place_connector_tags_at(_distributedConnectorTags, connectorTag, at, howMany);
							++iExit;
						}
						else
						{
							breakArrayAt = at;
						}
					}
					an_assert(iExit == 3.0f);
				}
			}

			if (mixAll)
			{
				upAt = TypeConversions::Normal::f_i_closest(count * 0.0f / 3.0f);
				rightAt = TypeConversions::Normal::f_i_closest(count * 1.0f / 3.0f);
				downAt = TypeConversions::Normal::f_i_closest(count * 2.0f / 3.0f) - down;
				leftAt = TypeConversions::Normal::f_i_closest(count * 3.0f / 3.0f) - left;
				place_connector_tags_at(_distributedConnectorTags, NAME(openWorldUp), upAt, up);
				place_connector_tags_at(_distributedConnectorTags, NAME(openWorldRight), rightAt, right);
				place_connector_tags_at(_distributedConnectorTags, NAME(openWorldDown), downAt, down);
				place_connector_tags_at(_distributedConnectorTags, NAME(openWorldLeft), leftAt, left);
			}

		}

		an_assert(upAt == 0);
		{
			// just fill circularly
			for_every(dd, _distributedDirs)
			{
				int idx = for_everys_index(dd);
				DirFourClockwise::Type d = DirFourClockwise::Up;
				if (idx < rightAt) d = DirFourClockwise::Up;
				else if (idx < downAt) d = DirFourClockwise::Right;
				else if (idx < leftAt) d = DirFourClockwise::Down;
				else d = DirFourClockwise::Left;
				*dd = d;
			}
		}

		if (useSpecial)
		{
			place_connector_tags_at(_distributedConnectorTags, NAME(openWorldSpecial), rg.get_int(_count + 1 - special), special);
		}

		// reorganise the array
		if (breakArrayAt > 0)
		{
			// split and swap
			ARRAY_PREALLOC_SIZE(Name, tempArray0, _distributedConnectorTags.get_size());
			ARRAY_PREALLOC_SIZE(Name, tempArray1, _distributedConnectorTags.get_size());
			an_assert(_distributedDirs.get_size() == _distributedConnectorTags.get_size());
			ARRAY_PREALLOC_SIZE(DirFourClockwise::Type, tempArrayD0, _distributedDirs.get_size());
			ARRAY_PREALLOC_SIZE(DirFourClockwise::Type, tempArrayD1, _distributedDirs.get_size());
			{
				Name const* a = _distributedConnectorTags.begin();
				DirFourClockwise::Type const* ad = _distributedDirs.begin();
				int i = 0;
				int const e = _distributedConnectorTags.get_size();
				while (i < breakArrayAt)
				{
					tempArray0.push_back(*a);
					tempArrayD0.push_back(*ad);
					++i;
					++a;
					++ad;
				}
				while (i < e)
				{
					tempArray1.push_back(*a);
					tempArrayD1.push_back(*ad);
					++i;
					++a;
					++ad;
				}
			}
			_distributedConnectorTags.clear();
			_distributedConnectorTags.add_from(tempArray1);
			_distributedConnectorTags.add_from(tempArray0);
			_distributedDirs.clear();
			_distributedDirs.add_from(tempArrayD1);
			_distributedDirs.add_from(tempArrayD0);

			if (!_looped)
			{
				// make sure the first (next to the breaker) and the last (prev to the breaker) are the first and the last
				{
					// check if the first one is next to worldBreakerDir, if so, move to be the first
					auto rightToWB = DirFourClockwise::next_to(_worldBreakerDir.get());
					Name connectorTag = dir_to_connector_tag((DirFourClockwise::Type)rightToWB);
					for_every(dct, _distributedConnectorTags)
					{
						if (dct->is_valid())
						{
							if (*dct == connectorTag)
							{
								swap(_distributedConnectorTags[0], _distributedConnectorTags[for_everys_index(dct)]);
								swap(_distributedDirs[0], _distributedDirs[for_everys_index(dct)]);
							}
							break;
						}
					}
				}
				{
					// check if the first one is next to worldBreakerDir, if so, move to be the first
					auto leftToWB = DirFourClockwise::prev_to(_worldBreakerDir.get());
					Name connectorTag = dir_to_connector_tag((DirFourClockwise::Type)leftToWB);
					for_every_reverse(dct, _distributedConnectorTags)
					{
						if (dct->is_valid())
						{
							if (*dct == connectorTag)
							{
								swap(_distributedConnectorTags[_distributedConnectorTags.get_size() - 1], _distributedConnectorTags[for_everys_index(dct)]);
								swap(_distributedDirs[_distributedDirs.get_size() - 1], _distributedDirs[for_everys_index(dct)]);
							}
							break;
						}
					}
				}
			}
		}

		if (_distribution == PilgrimageInstanceOpenWorldConnectorTagDistribution::CounterClockwise && _count > 1)
		{
			if (_looped)
			{
				// keep first being the first
				{
					int amount = (_count - firstCount) / 2;
					for_count(int, i, amount)
					{
						swap(_distributedConnectorTags[firstCount + i], _distributedConnectorTags[_count - 1 - i]);
						swap(_distributedDirs[firstCount + i], _distributedDirs[_count - 1 - i]);
					}
				}
				// swap firstCount around, so they're rearranged as well
				if (firstCount > 1)
				{
					int amount = (firstCount) / 2;
					for_count(int, i, amount)
					{
						swap(_distributedConnectorTags[i], _distributedConnectorTags[firstCount - 1 - i]);
						swap(_distributedDirs[i], _distributedDirs[firstCount - 1 - i]);
					}
				}
			}
			else
			{
				int amount = (_count) / 2;
				for_count(int, i, amount)
				{
					swap(_distributedConnectorTags[i], _distributedConnectorTags[_count - 1 - i]);
					swap(_distributedDirs[i], _distributedDirs[_count - 1 - i]);
				}
			}
		}

	}
	else
	{
		todo_implement;
	}
}

Framework::DoorInRoom* PilgrimageInstanceOpenWorld::get_linked_door_in(Framework::Door* _door, VectorInt2 const& _cellAt)
{
	if (auto* at = _door->get_variables().get_existing<VectorInt2>(NAME(aCell)))
	{
		if (*at == _cellAt)
		{
			return _door->get_linked_door_a();
		}
	}
	if (auto* at = _door->get_variables().get_existing<VectorInt2>(NAME(bCell)))
	{
		if (*at == _cellAt)
		{
			return _door->get_linked_door_b();
		}
	}
	return nullptr;
}

void PilgrimageInstanceOpenWorld::have_connector_tags_at_ends(Array<Name>& _distributedConnectorTags)
{
	if (!_distributedConnectorTags.get_first().is_valid())
	{
		for_every(dct, _distributedConnectorTags)
		{
			if (dct->is_valid())
			{
				swap(_distributedConnectorTags[0], _distributedConnectorTags[for_everys_index(dct)]);
				break;
			}
		}
	}
	if (!_distributedConnectorTags.get_last().is_valid())
	{
		for_every_reverse(dct, _distributedConnectorTags)
		{
			if (dct->is_valid())
			{
				swap(_distributedConnectorTags[_distributedConnectorTags.get_size() - 1], _distributedConnectorTags[for_everys_index(dct)]);
				break;
			}
		}
	}
}

bool PilgrimageInstanceOpenWorld::does_cell_have_special_device(VectorInt2 const& _at, Name const & _pdWithTag) const
{
	Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("does_cell_have_special_device"));

	for_every(cell, cells)
	{
		if (cell->at == _at)
		{
			for_every(pd, cell->knownPilgrimageDevices)
			{
				if (pd->device.get() && pd->device->is_special())
				{
					if (!_pdWithTag.is_valid() || pd->device->get_tags().get_tag(_pdWithTag))
					{
						return true;
					}
				}
			}
			return false;
		}
	}

	return false;
}

bool PilgrimageInstanceOpenWorld::does_cell_have_long_distance_detectable_device(VectorInt2 const& _at, OPTIONAL_ OUT_ bool* _detectableByDirection) const
{
	Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("does_cell_have_long_distance_detectable_device"));

	for_every(cell, cells)
	{
		if (cell->at == _at)
		{
			for_every(pd, cell->knownPilgrimageDevices)
			{
				if (pd->device.get() && pd->device->is_long_distance_detectable())
				{
					if (!pd->device->is_long_distance_detectable_only_if_not_depleted() || !pd->depleted)
					{
						assign_optional_out_param(_detectableByDirection, pd->device->is_long_distance_detectable_by_direction());
						return true;
					}
				}
			}
			return false;
		}
	}

	return false;
}

Framework::RegionType* PilgrimageInstanceOpenWorld::get_main_region_type() const
{
	auto* owDef = pilgrimage->open_world__get_definition();

	return owDef->get_main_region_type();
}

Framework::DoorInRoom* PilgrimageInstanceOpenWorld::find_best_door_in_dir(Framework::Room* _room, DirFourClockwise::Type _inDir)
{
	// direct layout cues
	{
		Name layoutCueTag;
		switch (_inDir)
		{
		case DirFourClockwise::Up:		layoutCueTag = NAME(layoutCueStartUp);		break;
		case DirFourClockwise::Right:	layoutCueTag = NAME(layoutCueStartRight);	break;
		case DirFourClockwise::Down:	layoutCueTag = NAME(layoutCueStartDown);	break;
		case DirFourClockwise::Left:	layoutCueTag = NAME(layoutCueStartLeft);	break;
		default: an_assert(false); break;
		}

		for_every_ptr(dir, _room->get_all_doors())
		{
			// invisible are fine here
			if (dir->get_tags().has_tag(layoutCueTag))
			{
				// won't be better
				return dir;
			}
		}
	}

	Framework::DoorInRoom* best = nullptr;
	int bestDist = NONE;

	for_count(int, iDist, 2)
	{
		Name distTag;
		switch (_inDir)
		{
		case DirFourClockwise::Up:		distTag = iDist == 0? NAME(openWorldCellDistFromUp)		: NAME(layoutCueDistFromUp);		break;
		case DirFourClockwise::Right:	distTag = iDist == 0? NAME(openWorldCellDistFromRight)	: NAME(layoutCueDistFromRight);		break;
		case DirFourClockwise::Down:	distTag = iDist == 0? NAME(openWorldCellDistFromDown)	: NAME(layoutCueDistFromDown);		break;
		case DirFourClockwise::Left:	distTag = iDist == 0? NAME(openWorldCellDistFromLeft)	: NAME(layoutCueDistFromLeft);		break;
		default: an_assert(false); break;
		}

		for_every_ptr(dir, _room->get_all_doors())
		{
			// invisible are fine here
			if (dir->get_tags().has_tag(distTag))
			{
				int dist = dir->get_tags().get_tag_as_int(distTag);
				if (dist > 0)
				{
					if (dist < bestDist || !best)
					{
						best = dir;
						bestDist = dist;
					}
				}
			}
		}
	}

	return best;
}

Optional<DirFourClockwise::Type> PilgrimageInstanceOpenWorld::get_dir_for_door(Framework::DoorInRoom* _dir)
{
	// direct layout cues
	{
		for_count(int, inDir, DirFourClockwise::NUM)
		{
			Name layoutCueTag;
			switch (inDir)
			{
			case DirFourClockwise::Up:		layoutCueTag = NAME(layoutCueStartUp);		break;
			case DirFourClockwise::Right:	layoutCueTag = NAME(layoutCueStartRight);	break;
			case DirFourClockwise::Down:	layoutCueTag = NAME(layoutCueStartDown);	break;
			case DirFourClockwise::Left:	layoutCueTag = NAME(layoutCueStartLeft);	break;
			default: an_assert(false); break;
			}

			if (_dir->get_tags().has_tag(layoutCueTag))
			{
				// won't be better
				return (DirFourClockwise::Type)inDir;
			}
		}
	}

	Optional<DirFourClockwise::Type> bestDFC;
	int bestDFCDist = 0;
	for_count(int, iDist, 2)
	{
		for_count(int, inDir, DirFourClockwise::NUM)
		{
			Name distTag;
			switch (inDir)
			{
			case DirFourClockwise::Up:		distTag = iDist == 0 ? NAME(openWorldCellDistFromUp)	: NAME(layoutCueDistFromUp);		break;
			case DirFourClockwise::Right:	distTag = iDist == 0 ? NAME(openWorldCellDistFromRight)	: NAME(layoutCueDistFromRight);		break;
			case DirFourClockwise::Down:	distTag = iDist == 0 ? NAME(openWorldCellDistFromDown)	: NAME(layoutCueDistFromDown);		break;
			case DirFourClockwise::Left:	distTag = iDist == 0 ? NAME(openWorldCellDistFromLeft)	: NAME(layoutCueDistFromLeft);		break;
			default: an_assert(false); break;
			}

			if (_dir->get_tags().has_tag(distTag))
			{
				bool inThisDir = false;

				int dist = _dir->get_tags().get_tag_as_int(distTag);
				if (_dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
				{
					inThisDir = true;

					for_every_ptr(dir, _dir->get_in_room()->get_all_doors())
					{
						// invisible are fine here
						if (dir != _dir)
						{
							int odist = dir->get_tags().get_tag_as_int(distTag);
							if (odist <= dist) // = if does not exist
							{
								inThisDir = false;
								break;
							}
						}
					}
				}
				else if (auto* odir = _dir->get_door_on_other_side())
				{
					int distOther = odir->get_tags().get_tag_as_int(distTag);
					inThisDir = distOther <= dist; // = if does not exist
				}

				if (inThisDir)
				{
					if (!bestDFC.is_set() ||
						dist < bestDFCDist)
					{
						bestDFC = (DirFourClockwise::Type)inDir;
						bestDFCDist = dist;
					}
				}
			}
		}
	}

	return bestDFC;
}

bool PilgrimageInstanceOpenWorld::is_door_in_dir(Framework::DoorInRoom* _dir, DirFourClockwise::Type _inDir)
{
	// direct layout cues
	{
		Name layoutCueTag;
		switch (_inDir)
		{
		case DirFourClockwise::Up:		layoutCueTag = NAME(layoutCueStartUp);		break;
		case DirFourClockwise::Right:	layoutCueTag = NAME(layoutCueStartRight);	break;
		case DirFourClockwise::Down:	layoutCueTag = NAME(layoutCueStartDown);	break;
		case DirFourClockwise::Left:	layoutCueTag = NAME(layoutCueStartLeft);	break;
		default: an_assert(false); break;
		}

		if (_dir->get_tags().has_tag(layoutCueTag))
		{
			// won't be better
			return true;
		}
	}

	Name distTag;
	for_count(int, iDist, 2)
	{
		switch (_inDir)
		{
		case DirFourClockwise::Up:		distTag = iDist == 0 ? NAME(openWorldCellDistFromUp)	: NAME(layoutCueDistFromUp);		break;
		case DirFourClockwise::Right:	distTag = iDist == 0 ? NAME(openWorldCellDistFromRight)	: NAME(layoutCueDistFromRight);		break;
		case DirFourClockwise::Down:	distTag = iDist == 0 ? NAME(openWorldCellDistFromDown)	: NAME(layoutCueDistFromDown);		break;
		case DirFourClockwise::Left:	distTag = iDist == 0 ? NAME(openWorldCellDistFromLeft)	: NAME(layoutCueDistFromLeft);		break;
		default: an_assert(false); break;
		}

		if (_dir->get_tags().has_tag(distTag))
		{
			int dist = _dir->get_tags().get_tag_as_int(distTag);
			if (_dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
			{
				for_every_ptr(dir, _dir->get_in_room()->get_all_doors())
				{
					// invisible are fine here
					if (dir != _dir)
					{
						int odist = dir->get_tags().get_tag_as_int(distTag);
						if (odist < dist)
						{
							return false;
						}
					}
				}
				return true;
			}
			else if (auto* odir = _dir->get_door_on_other_side())
			{
				int distOther = odir->get_tags().get_tag_as_int(distTag);
				if (dist > distOther)
				{
					return true;
				}
			}
			else
			{
				error(TXT("no door on the other side and not a world separator door"));
			}
		}
	}

	return false;
}

void PilgrimageInstanceOpenWorld::force_add_open_world_destination_at(Framework::Room* _room, PilgrimageDevice* _device)
{
	if (_room && _device)
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("tag open world directions for cell"));

		if (auto* ec = get_existing_cell_for_already_locked(_room))
		{
			for_every(fd, ec->forcedDestinations)
			{
				if (fd->room == _room &&
					fd->device.get() == _device)
				{
					return;
				}
			}
			ExistingCell::ForcedDestination fd;
			fd.room = _room;
			fd.device = _device;
			ec->forcedDestinations.push_back(fd);
		}
	}
}

void PilgrimageInstanceOpenWorld::async_hide_doors_that_wont_be_used(ExistingCell* _cell)
{
	ASSERT_ASYNC;

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	if (!_cell)
	{
		return;
	}

	VectorInt2 cellAt = _cell->at;
	//Optional<DirFourClockwise::Type> towardsObjective = _cell->towardsObjective.is_set() ? _cell->towardsObjective : _cell->towardsLocalObjectiveDir;
	Array<ExistingCell::Door> cellDoors = _cell->doors;

#ifdef AN_DEVELOPMENT_OR_PROFILER
	// cell should not be used beyond this point as we might be modifying existing cell array
	_cell = nullptr;
#endif

#ifdef AN_IMPORTANT_INFO
	output(TXT("async_hide_doors_that_wont_be_used %ix%i"), cellAt.x, cellAt.y);
	output(TXT("+ update towards objective first"));
	System::TimeStamp utoTS;
#endif
	async_update_towards_objective(cellAt);
#ifdef AN_IMPORTANT_INFO
	output(TXT("  done in %.3fs"), utoTS.get_time_since());
	System::TimeStamp hideDoorsTS;
#endif


	Framework::Room* playerInRoom = startInRoom.get();
	Optional<VectorInt2> playerAt;
	Vector3 playerCentreWS;
	todo_multiplayer_issue(TXT("we just get player here"));
	if (auto* g = Game::get_as<Game>())
	{
		if (auto* pa = g->access_player().get_actor())
		{
			playerAt = find_cell_at(pa);
			playerInRoom = pa->get_presence()->get_in_room();
			playerCentreWS = pa->get_presence()->get_centre_of_presence_WS();
#ifdef AN_IMPORTANT_INFO
			output(TXT(" player found at %ix%i"), playerAt.get().x, playerAt.get().y);
#endif
		}
	}
	Framework::Room* room = nullptr;
	Framework::DoorInRoom* cameThrough = nullptr;

	if (playerAt.is_set())
	{
		if (cellAt == playerAt.get())
		{
#ifdef AN_IMPORTANT_INFO
			output(TXT(" player in the same cell, find room and nearest door"));
#endif
			room = playerInRoom;
			// find closest door to pretend we moved through it
			cameThrough = nullptr;
			if (room != startingRoom.get() &&
				room->get_all_doors().get_size() > 1)
			{
				float bestDistSq = 0.0f;
				for_every_ptr(door, room->get_all_doors())
				{
					if (door->is_relevant_for_movement() &&
						door->is_relevant_for_vr() &&
						door->can_move_through() &&
						! door->may_ignore_vr_placement() &&
						!does_door_lead_towards(door, DoorLeadsTowardParams().to_objective(true)).get(false))
					{
						float distSq = (door->get_hole_centre_WS() - playerCentreWS).length_squared();
						if (distSq < bestDistSq || !cameThrough)
						{
							cameThrough = door;
							bestDistSq = distSq;
						}
					}
				}
			}
		}
		else
		{
#ifdef AN_IMPORTANT_INFO
			output(TXT(" player may come here from a different cell"));
#endif
			// there's just one path, from start
			if (cellAt == get_start_at())
			{
				room = startingRoom.get();
				cameThrough = nullptr;
			}
			else
			{
				if (!room)
				{
					// because there should be only one path, we only need to find the very previous one to know from where did we come

					Optional<VectorInt2> pAt; // previous, we will check all cells around to see which one points here
					for_count(int, dirIdx, DirFourClockwise::NUM)
					{
						VectorInt2 dirOff = DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)dirIdx);
						VectorInt2 pAtCandidate = cellAt + dirOff;
						ExistingCell* ec = nullptr;
						{
							Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("async_hide_doors_that_wont_be_used backtrack"));

							ec = get_existing_cell_already_locked(pAtCandidate);
						}
						if (ec && ec->towardsObjective.is_set())
						{
							if (pAtCandidate + DirFourClockwise::dir_to_vector_int2(ec->towardsObjective.get()) == cellAt)
							{
								pAt = pAtCandidate;
								break;
							}
						}
					}
					if (pAt.is_set())
					{
						// we're on path, get dir from where we will be coming
						for_every(door, cellDoors)
						{
							if (door->toCellAt.is_set() &&
								door->toCellAt == pAt.get())
							{
								if (auto* doorInRoom = get_linked_door_in(door->door.get(), cellAt))
								{
									room = doorInRoom->get_in_room();
									cameThrough = doorInRoom;
								}
							}
						}
					}
				}
				if (!room)
				{
					// failsafe?
					// let's find the door the player may come through and lead towards the end
					// if player is in the direction we want to go
					Optional<VectorInt2> lastValidPathAt;
					Optional<VectorInt2> prevToLastValidPathAt;
					{
						VectorInt2 pathAt = get_start_at();
						while (pathAt != playerAt.get())
						{
							VectorInt2 at = pathAt;
							async_update_towards_objective(at);
							ExistingCell* ec = nullptr;
							{
								Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("async_hide_doors_that_wont_be_used"));

								ec = get_existing_cell_already_locked(at);
							}
							if (ec &&
								ec->towardsObjective.is_set())
							{
								VectorInt2 newPathAt = at + DirFourClockwise::dir_to_vector_int2(ec->towardsObjective.get());
								if (pathAt != newPathAt)
								{
									prevToLastValidPathAt = lastValidPathAt;
									lastValidPathAt = pathAt;
									pathAt = newPathAt;
								}
								else
								{
									lastValidPathAt.clear();
									break;
								}
							}
							else
							{
								lastValidPathAt.clear();
								break;
							}
						}
					}

					if (lastValidPathAt.is_set() && lastValidPathAt == cellAt &&
						prevToLastValidPathAt.is_set())
					{
						// we're on path, get dir from where we came to last valid path at
						for_every(door, cellDoors)
						{
							if (door->toCellAt.is_set() &&
								door->toCellAt == prevToLastValidPathAt.get())
							{
								if (auto* doorInRoom = get_linked_door_in(door->door.get(), cellAt))
								{
									room = doorInRoom->get_in_room();
									cameThrough = doorInRoom;
								}
							}
						}
					}
					else
					{
						for_every(door, cellDoors)
						{
							if (door->toCellAt.is_set() &&
								door->toCellAt == playerAt.get())
							{
								if (auto* doorInRoom = get_linked_door_in(door->door.get(), cellAt))
								{
									room = doorInRoom->get_in_room();
									cameThrough = doorInRoom;
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if (playerInRoom == startingRoom.get())
		{
#ifdef AN_IMPORTANT_INFO
			output(TXT(" player in the starting room"));
#endif
			if (auto* d = startingRoomExitDoor.get())
			{
				auto* dirPlayer = d->get_linked_door_in(playerInRoom);
				cameThrough = dirPlayer->get_door_on_other_side();
				if (cameThrough)
				{
					room = cameThrough->get_in_room();
				}
			}
		}
	}

	Array<Framework::Room*> roomsOnPath;
	while (room)
	{
		roomsOnPath.push_back(room);
#ifdef AN_IMPORTANT_INFO
		output(TXT(" + room r%p <- r%p (dr'%p:dr'%p): \"%S\""), room, cameThrough? cameThrough->get_room_on_other_side() : nullptr, cameThrough, cameThrough? cameThrough->get_door_on_other_side() : nullptr, room->get_name().to_char());
#endif
		int bestDist = 0;
		Framework::DoorInRoom* nextBestDoor = nullptr;
		bool nextBestDoorWorldSeparator = false;
		for_every_ptr(door, room->get_all_doors())
		{
			if (auto* d = door->get_door())
			{
				Optional<bool> visible;
				if (door == cameThrough)
				{
					visible = true;
				}
				else if (!cameThrough || door->get_group_id() == cameThrough->get_group_id())
				{
					if (d->is_relevant_for_movement() &&
						door->can_move_through() &&
						!door->may_ignore_vr_placement())
					{
						d->set_visible(false); // we will unhide the best door

						bool worldSeparator = d->is_world_separator_door();
						int dist;
						Optional<bool> leads = does_door_lead_towards(door, DoorLeadsTowardParams().to_objective(true), OUT_ & dist);
						if (d == startingRoomExitDoor.get())
						{
							// special case for starting door
							worldSeparator = false;
							leads = true;
							dist = 0;
						}
						if (leads.get(false))
						{
							if (dist < bestDist || !nextBestDoor)
							{
								nextBestDoorWorldSeparator = worldSeparator;
								bestDist = dist;
								nextBestDoor = door;
							}
						}
					}
				}
				if (room == startingRoom.get())
				{
					// all visible in starting room, if going back, doesn't matter
					visible = true;
				}
				if (visible.is_set())
				{
					d->set_visible(visible.get());
				}

			}
		}
		Framework::Room* nextRoom = nullptr;
		Framework::DoorInRoom* nextCameThrough = nullptr;
		if (nextBestDoor)
		{
			if (auto* d = nextBestDoor->get_door())
			{
				d->set_visible(true);
			}
			if (!nextBestDoorWorldSeparator)
			{
				nextCameThrough = nextBestDoor->get_door_on_other_side();
				nextRoom = nextCameThrough ? nextCameThrough->get_in_room() : nullptr;
			}
		}
		room = nextRoom;
		cameThrough = nextCameThrough;
	}

	if (!roomsOnPath.is_empty())
	{
		ExistingCell* ec = nullptr;
		{
			Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("to make rooms inaccessible"));

			ec = get_existing_cell_already_locked(cellAt);
		}
		if (ec)
		{
			if (auto* r = ec->mainRegion.get())
			{
				Game::get()->perform_sync_world_job(TXT("make rooms inaccessible"), [&roomsOnPath, r]()
					{
						r->for_every_room([&roomsOnPath](Framework::Room* _room)
							{
								if (!roomsOnPath.does_contain(_room))
								{
									_room->access_tags().set_tag(NAME(inaccessible));
								}
							});
					});

			}
		}
	}

#ifdef AN_IMPORTANT_INFO
	output(TXT("  doors hidden in %.3fs"), hideDoorsTS.get_time_since());
#endif
}

void PilgrimageInstanceOpenWorld::tag_open_world_directions_for_cell(Framework::Room* _roomInsideCell, bool _pilgrimageDevicesOnly)
{
	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	scoped_call_stack_info(TXT("tag open world directions for cell"));

	ExistingCell* ec = nullptr;
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("tag open world directions for cell"));

		ec = get_existing_cell_for_already_locked(_roomInsideCell);
	}

	::System::TimeStamp tagTS;
#ifdef AN_IMPORTANT_INFO
	if (ec)
	{
		output(TXT("[start] tag open world directions for cell [%ix%i] r%p"), ec->at.x, ec->at.y, _roomInsideCell);
	}
	else
	{
		output(TXT("[start] tag open world directions for cell [no cell] r%p"), _roomInsideCell);
	}
#endif

	if (ec && (! ec->doors.is_empty() || ! ec->pilgrimageDevices.is_empty()))
	{
		ARRAY_PREALLOC_SIZE(Framework::Room*, allRooms, 128);

		scoped_call_stack_info_str_printf(TXT("cell %ix%i"), ec->at.x, ec->at.y);

		// clear all direction and distance tags first
		{
			if (allRooms.is_empty() && !ec->doors.is_empty())
			{
				for_every(ecd, ec->doors)
				{
					if (auto* doorInRoom = get_linked_door_in(ecd->door.get(), ec->at))
					{
						allRooms.push_back(doorInRoom->get_in_room());
						break;
					}
				}
			}
			if (allRooms.is_empty() && !ec->pilgrimageDevices.is_empty())
			{
				for_every(ecpd, ec->pilgrimageDevices)
				{
					if (auto* imo = ecpd->object.get())
					{
						allRooms.push_back(imo->get_presence()->get_in_room());
						break;
					}
				}
			}
			for (int i = 0; i < allRooms.get_size(); ++i)
			{
				auto* r = allRooms[i];
				an_assert(r);
				for_every_ptr(dir, r->get_all_doors())
				{
					if (!_pilgrimageDevicesOnly)
					{
						dir->access_tags().remove_tag(NAME(openWorldCellDistFromUp));
						dir->access_tags().remove_tag(NAME(openWorldCellDistFromRight));
						dir->access_tags().remove_tag(NAME(openWorldCellDistFromDown));
						dir->access_tags().remove_tag(NAME(openWorldCellDistFromLeft));
						dir->access_tags().remove_tag(NAME(openWorldCellDistFromSpecial));
						dir->access_tags().remove_tag(NAME(openWorldCellDistFromObjective));
						dir->access_tags().remove_tag(NAME(layoutCueStartUp));
						dir->access_tags().remove_tag(NAME(layoutCueStartRight));
						dir->access_tags().remove_tag(NAME(layoutCueStartDown));
						dir->access_tags().remove_tag(NAME(layoutCueStartLeft));
						dir->access_tags().remove_tag(NAME(layoutCueDistFromUp));
						dir->access_tags().remove_tag(NAME(layoutCueDistFromRight));
						dir->access_tags().remove_tag(NAME(layoutCueDistFromDown));
						dir->access_tags().remove_tag(NAME(layoutCueDistFromLeft));
					}
					for_every(pd, ec->pilgrimageDevices)
					{
						Name openWorldCellDistTag = pd->device->get_open_world_cell_dist_tag();
						if (openWorldCellDistTag.is_valid())
						{
							dir->access_tags().remove_tag(openWorldCellDistTag);
						}
					}
					if (dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
					{
						// skip world separators
						continue;
					}
					if (!dir->get_room_on_other_side())
					{
						continue;
					}
					allRooms.push_back_unique(dir->get_room_on_other_side());
				}
			}
		}

		// fill distances for each door-in-room (open world cells)
		if (!_pilgrimageDevicesOnly)
		{
			bool dirHandled[DirFourClockwise::NUM];
			for_count(int, i, DirFourClockwise::NUM)
			{
				dirHandled[i] = false;
			}
			Framework::Door* specialDoor = nullptr;
			for_every(d, ec->doors)
			{
				auto* door = d->door.get();
				if (!door)
				{
					continue;
				}
				if (door->get_variables().get_value<bool>(NAME(special), false) ||
					!d->toCellAt.is_set())
				{
					specialDoor = door;
					continue;
				}
				Name distTag;
				// get dist tag
				{
					auto od = DirFourClockwise::vector_int2_to_dir(d->toCellAt.get() - ec->at);
					if (!od.is_set())
					{
						error(TXT("no valid direction for this particular door"));
						continue;
					}
					switch (od.get())
					{
					case DirFourClockwise::Up: distTag = NAME(openWorldCellDistFromUp); break;
					case DirFourClockwise::Right: distTag = NAME(openWorldCellDistFromRight); break;
					case DirFourClockwise::Down: distTag = NAME(openWorldCellDistFromDown); break;
					case DirFourClockwise::Left: distTag = NAME(openWorldCellDistFromLeft); break;
					default: an_assert(false); break;
					}
					if (od.get() >= 0 && od.get() < DirFourClockwise::NUM)
					{
						dirHandled[od.get()] = true;
					}
				}

				// fill dist
				if (auto* dir = get_linked_door_in(door, ec->at))
				{
					clear_distances_for_doors(dir, distTag);
					fill_distances_for_doors(dir, distTag);
				}
				else
				{
					error(TXT("no valid door-in-room for dist tag \"%S\""), distTag.to_char());
				}
			}
			ec->towardsLocalObjectiveDir.clear();
			ec->towardsLocalObjectiveNoDir = false;
			if (specialDoor)
			{
				int dirHandledCount = 0;
				DirFourClockwise::Type oppositeDir = DirFourClockwise::Up;
				DirFourClockwise::Type freeDir = DirFourClockwise::Up;
				Optional<DirFourClockwise::Type> suggestedDir;
				bool allowDir = true;
				bool useObjective = false;
				if (specialDoor == startingRoomExitDoor.get())
				{
					if (get_special_rules().startingRoomWithoutDirection)
					{
						allowDir = false;
					}
					suggestedDir = get_special_rules().startingRoomDoorDirection;
				}
				if (specialDoor == ec->transitionEntranceDoor.get())
				{
					if (get_special_rules().transitionToNextRoomWithoutDirection)
					{
						allowDir = false;
					}
					suggestedDir = get_special_rules().transitionToNextRoomDoorDirection;
					if (get_special_rules().transitionToNextRoomWithObjectiveDirection)
					{
						allowDir = true;
						useObjective = true;
					}
				}
				{
					clear_distances_for_doors(specialDoor->get_linked_door_b(), NAME(openWorldCellDistFromSpecial));
					fill_distances_for_doors(specialDoor->get_linked_door_b(), NAME(openWorldCellDistFromSpecial));
				}
				if (allowDir)
				{
					if (useObjective)
					{
						ec->towardsLocalObjectiveNoDir = true;
						clear_distances_for_doors(specialDoor->get_linked_door_b(), NAME(openWorldCellDistFromObjective));
						fill_distances_for_doors(specialDoor->get_linked_door_b(), NAME(openWorldCellDistFromObjective));
					}
					else
					{
						for_count(int, i, DirFourClockwise::NUM)
						{
							if (dirHandled[i])
							{
								++dirHandledCount;
								oppositeDir = (DirFourClockwise::Type)((i + 2) % DirFourClockwise::NUM);
								if (suggestedDir.is_set() && suggestedDir.get() == i)
								{
									suggestedDir.clear();
								}
							}
							else
							{
								freeDir = (DirFourClockwise::Type)(i);
							}
						}
						if (dirHandledCount < 4)
						{
							DirFourClockwise::Type useDir = suggestedDir.get(dirHandledCount == 1 ? oppositeDir : freeDir);
							if (auto* dir = specialDoor->get_linked_door_b()) // a is linked to starting/transition room
							{
								Name distTag;
								// get dist tag
								{
									switch (useDir)
									{
									case DirFourClockwise::Up: distTag = NAME(openWorldCellDistFromUp); break;
									case DirFourClockwise::Right: distTag = NAME(openWorldCellDistFromRight); break;
									case DirFourClockwise::Down: distTag = NAME(openWorldCellDistFromDown); break;
									case DirFourClockwise::Left: distTag = NAME(openWorldCellDistFromLeft); break;
									default: an_assert(false); break;
									}
								}
								if (ec->usesObjective &&
									! ec->usesObjective->notActualObjective)
								{
									ec->towardsLocalObjectiveDir = useDir;
								}
								clear_distances_for_doors(dir, distTag);
								fill_distances_for_doors(dir, distTag);
							}
						}
					}
				}
			}
		}

		// fill distances for each door-in-room (pilgrimage devices)
		for_every(pd, ec->pilgrimageDevices)
		{
			Name distTag = pd->device->get_open_world_cell_dist_tag();

			if (distTag.is_valid())
			{
				auto* object = pd->object.get();
				an_assert(object);

				if (auto* mpd = object->get_custom<CustomModules::PilgrimageDevice>())
				{
					if (auto* guideTowards = mpd->get_guide_towards())
					{
						object = guideTowards;
					}
				}

				auto* room = object->get_presence()->get_in_room();

				for_every_ptr(dir, room->get_all_doors())
				{
					clear_distances_for_doors(dir, distTag, true);
				}

				for_every_ptr(dir, room->get_all_doors())
				{
					fill_distances_for_doors(dir, distTag, true);
				}
			}
		}

		// fill distances for forced destinations (forced destinations)
		if (!_pilgrimageDevicesOnly)
		{
			for_every(fd, ec->forcedDestinations)
			{
				Name distTag = fd->device->get_open_world_cell_dist_tag();

				if (distTag.is_valid())
				{
					if (auto* room = fd->room.get())
					{
						for_every_ptr(dir, room->get_all_doors())
						{
							clear_distances_for_doors(dir, distTag, true);
						}

						for_every_ptr(dir, room->get_all_doors())
						{
							fill_distances_for_doors(dir, distTag, true);
						}
					}
				}
			}
		}

		// fill distances for layout cues
		if (!_pilgrimageDevicesOnly)
		{
			// create starts
			for_every_ptr(room, allRooms)
			{
				for_every_ptr(dir, room->get_all_doors())
				{
					for_count(int, inDir, DirFourClockwise::NUM)
					{
						Name layoutCueTag, layoutCueCapTag, addLayoutCueTag;
						// get dist tag
						{
							switch (inDir)
							{
							case DirFourClockwise::Up:		layoutCueTag = NAME(layoutCueUp);		layoutCueCapTag = NAME(layoutCueCapUp);		addLayoutCueTag = NAME(layoutCueStartUp);		break;
							case DirFourClockwise::Right:	layoutCueTag = NAME(layoutCueRight);	layoutCueCapTag = NAME(layoutCueCapRight);	addLayoutCueTag = NAME(layoutCueStartRight);	break;
							case DirFourClockwise::Down:	layoutCueTag = NAME(layoutCueDown);		layoutCueCapTag = NAME(layoutCueCapDown);	addLayoutCueTag = NAME(layoutCueStartDown);		break;
							case DirFourClockwise::Left:	layoutCueTag = NAME(layoutCueLeft);		layoutCueCapTag = NAME(layoutCueCapLeft);	addLayoutCueTag = NAME(layoutCueStartLeft);		break;
							default: an_assert(false); break;
							}
						}

						if (dir->get_tags().has_tag(layoutCueTag))
						{
							dir->access_tags().set_tag(addLayoutCueTag);
						}
						if (dir->get_tags().has_tag(layoutCueCapTag))
						{
							if (!dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
							{
								if (auto* odir = dir->get_door_on_other_side())
								{
									odir->access_tags().set_tag(addLayoutCueTag);
								}
							}
						}
					}
				}
			}

			// fill distances now
			for_every_ptr(room, allRooms)
			{
				for_every_ptr(dir, room->get_all_doors())
				{
					for_count(int, inDir, DirFourClockwise::NUM)
					{
						Name layoutCueTag, distTag;
						// get dist tag
						{
							switch (inDir)
							{
							case DirFourClockwise::Up:		layoutCueTag = NAME(layoutCueStartUp);		distTag = NAME(layoutCueDistFromUp);	break;
							case DirFourClockwise::Right:	layoutCueTag = NAME(layoutCueStartRight);	distTag = NAME(layoutCueDistFromRight);	break;
							case DirFourClockwise::Down:	layoutCueTag = NAME(layoutCueStartDown);	distTag = NAME(layoutCueDistFromDown);	break;
							case DirFourClockwise::Left:	layoutCueTag = NAME(layoutCueStartLeft);	distTag = NAME(layoutCueDistFromLeft);	break;
							default: an_assert(false); break;
							}
						}

						if (dir->get_tags().has_tag(layoutCueTag))
						{
							fill_layout_cue_distances_for_doors(dir, distTag);
						}
					}
				}
			}
		}
	}

#ifdef AN_IMPORTANT_INFO
	output(TXT("[end] tag open world directions for cell r%p : time taken %.3f"), _roomInsideCell, tagTS.get_time_since());
#endif
}

void PilgrimageInstanceOpenWorld::clear_distances_for_doors(Framework::DoorInRoom* dir, Name const& distTag, bool _goThrough)
{
	scoped_call_stack_info_str_printf(TXT("clear distances \"%S\""), distTag.to_char());

#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
	output(TXT("clear distances \"%S\""), distTag.to_char());
#endif

	// clear tag within group and cell separators
	// if we won't do that but we added rooms&doors (doors growing into vr corridors), we might be reaching incorrect and incomplete data
	// leading to holes and messed up navigation
	if (_goThrough && dir)
	{
		dir->access_tags().remove_tag(distTag);
		dir = dir->get_door_on_other_side();
	}
	if (dir)
	{
		Array<Framework::DoorInRoom*> doorsToProcess;
		doorsToProcess.push_back(dir);
		auto* startingDir = dir;
		dir->access_tags().remove_tag(distTag);
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
		output(TXT("remove tag for dr%p"), dir);
#endif
		for(int i = 0; i < doorsToProcess.get_size(); ++ i)
		{
			auto* dir = doorsToProcess[i];
			doorsToProcess.push_back_unique(dir);
			auto* r = dir->get_in_room();
			int dirGroupId = dir->get_group_id();
			// go through each door that is not world separator, if dist is none or larger, push it to the queue
			for_every_ptr(odir, r->get_all_doors())
			{
				if (doorsToProcess.does_contain(odir))
				{
					// already processed
					continue;
				}
				if (odir->get_group_id() != dirGroupId)
				{
					// only the same group!
					continue;
				}
				if (odir == dir)
				{
					// skip the same
					continue;
				}
				if (odir != startingDir)
				{
					if (odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
					{
						// skip world separators but still set further dist
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
						output(TXT("remove tag for dr%p"), odir);
#endif
						odir->access_tags().remove_tag(distTag);
						continue;
					}
				}
				else
				{
					if (odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
					{
						continue;
					}
				}
				if (auto* osdir = odir->get_door_on_other_side())
				{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
					output(TXT("remove tag for dr%p"), odir);
					output(TXT("remove tag for dr%p"), osdir);
#endif
					odir->access_tags().remove_tag(distTag);
					osdir->access_tags().remove_tag(distTag);
					doorsToProcess.push_back_unique(odir);
					doorsToProcess.push_back_unique(osdir);
				}
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::fill_distances_for_doors(Framework::DoorInRoom* dir, Name const& distTag, bool _goThrough)
{
	scoped_call_stack_info_str_printf(TXT("fill distances \"%S\""), distTag.to_char());

#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
	output(TXT("fill distances \"%S\""), distTag.to_char());
#endif

	// fill distances
	int startValue = 1;
	if (_goThrough && dir)
	{
		dir->access_tags().set_tag(distTag, startValue);
		dir = dir->get_door_on_other_side();
		++startValue;
	}
	if (dir)
	{
		Array<Framework::DoorInRoom*> doorsToProcess;
		doorsToProcess.make_space_for(128);
		dir->access_tags().set_tag(distTag, startValue);
		doorsToProcess.push_back(dir);
		auto* startingDir = dir;
		while (!doorsToProcess.is_empty())
		{
			auto* dir = doorsToProcess.get_first();
			auto* r = dir->get_in_room();
			int dirGroupId = dir->get_group_id();
			doorsToProcess.pop_front();
			int curDist = dir->get_tags().get_tag_as_int(distTag);
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
			output(TXT("  process dr'%p in r%p, group %i, cur dist %i"), dir, r, dirGroupId, curDist);
#endif
			// go through each door that is not world separator, if dist is none or larger, push it to the queue
			for_every_ptr(odir, r->get_all_doors())
			{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
				output(TXT("   check dr'%p, group %i  %S"), odir, odir->get_group_id(), odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator)? TXT(" cell world separator") : TXT(""));
#endif
				if (odir->get_group_id() != dirGroupId)
				{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
					output(TXT("    different group id"));
#endif
					// only the same group!
					continue;
				}
				if (odir == dir)
				{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
					output(TXT("    same door"));
#endif
					// skip the same
					continue;
				}
				if (odir != startingDir)
				{
					if (odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
					{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
						output(TXT("    cell separator"));
#endif
						// skip world separators but still set further dist
						odir->access_tags().set_tag(distTag, curDist + 1);
						continue;
					}
				}
				else
				{
					if (odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
					{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
						output(TXT("    cell separator"));
#endif
						continue;
					}
				}
				if (auto* osdir = odir->get_door_on_other_side())
				{
					int osDist = osdir->get_tags().get_tag_as_int(distTag);
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
					output(TXT("    check (other side) dr'%p = %i"), osdir, osDist);
#endif
					if (osDist == 0 || osDist > curDist + 2)
					{
						if (odir == startingDir)
						{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
							output(TXT("     set dr'%p to %i"), odir, curDist);
							output(TXT("     set dr'%p to %i"), osdir, curDist - 1);
#endif
							odir->access_tags().set_tag(distTag, curDist);
							osdir->access_tags().set_tag(distTag, curDist - 1);
						}
						else
						{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
							output(TXT("     set dr'%p to %i"), odir, curDist + 1);
							output(TXT("     set dr'%p to %i"), osdir, curDist + 2);
#endif
							odir->access_tags().set_tag(distTag, curDist + 1);
							osdir->access_tags().set_tag(distTag, curDist + 2);
						}
						doorsToProcess.push_back(osdir);
					}
				}
			}
		}
	}
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
	output(TXT("filled distances \"%S\""), distTag.to_char());
#endif
}

void PilgrimageInstanceOpenWorld::fill_layout_cue_distances_for_doors(Framework::DoorInRoom* dir, Name const& distTag)
{
	scoped_call_stack_info_str_printf(TXT("fill layout cue distances \"%S\""), distTag.to_char());

#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
	output(TXT("fill layout cue distances \"%S\""), distTag.to_char());
#endif

	// fill distances
	{
		Array<Framework::DoorInRoom*> doorsToProcess;
		doorsToProcess.make_space_for(128);
		dir->access_tags().set_tag(distTag, 1);
		doorsToProcess.push_back(dir);
		auto* startingDir = dir;
		while (!doorsToProcess.is_empty())
		{
			auto* dir = doorsToProcess.get_first();
			auto* r = dir->get_in_room();
			int dirGroupId = dir->get_group_id();
			doorsToProcess.pop_front();
			int curDist = dir->get_tags().get_tag_as_int(distTag);
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
			output(TXT("  process dr'%p in r%p, group %i, cur dist %i"), dir, r, dirGroupId, curDist);
#endif
			// go through each door that is not world separator, if dist is none or larger, push it to the queue
			for_every_ptr(odir, r->get_all_doors())
			{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
				output(TXT("   check dr'%p, group %i  %S"), odir, odir->get_group_id(), odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator)? TXT(" cell world separator") : TXT(""));
#endif
				if (odir->get_group_id() != dirGroupId)
				{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
					output(TXT("    different group id"));
#endif
					// only the same group!
					continue;
				}
				if (odir == dir)
				{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
					output(TXT("    same door"));
#endif
					// skip the same
					continue;
				}
				if (odir != startingDir)
				{
					if (odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
					{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
						output(TXT("    cell separator"));
#endif
						// skip world separators but still set further dist
						odir->access_tags().set_tag(distTag, curDist + 1);
						continue;
					}
				}
				else
				{
					if (odir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
					{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
						output(TXT("    cell separator"));
#endif
						continue;
					}
				}
				{
					bool hasAnyLayoutCueTag = false;
					for_count(int, inDir, DirFourClockwise::NUM)
					{
						Name layoutCueTag;
						switch (inDir)
						{
						case DirFourClockwise::Up:		layoutCueTag = NAME(layoutCueStartUp);		break;
						case DirFourClockwise::Right:	layoutCueTag = NAME(layoutCueStartRight);	break;
						case DirFourClockwise::Down:	layoutCueTag = NAME(layoutCueStartDown);	break;
						case DirFourClockwise::Left:	layoutCueTag = NAME(layoutCueStartLeft);	break;
						default: an_assert(false); break;
						}

						if (odir->get_tags().has_tag(layoutCueTag) ||
							(odir->get_door_on_other_side() && odir->get_door_on_other_side()->get_tags().has_tag(layoutCueTag)))
						{
							hasAnyLayoutCueTag = true;
							break;
						}
					}
					// we should stop here, let's just set dist
					if (hasAnyLayoutCueTag)
					{
						odir->access_tags().set_tag(distTag, curDist + 1);
						continue;
					}
				}
				if (auto* osdir = odir->get_door_on_other_side())
				{
					int osDist = osdir->get_tags().get_tag_as_int(distTag);
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
					output(TXT("    check (other side) dr'%p = %i"), osdir, osDist);
#endif
					if (osDist == 0 || osDist > curDist + 2)
					{
						if (odir != startingDir)
						{
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
							output(TXT("     set dr'%p to %i"), odir, curDist + 1);
							output(TXT("     set dr'%p to %i"), osdir, curDist + 2);
#endif
							odir->access_tags().set_tag(distTag, curDist + 1);
							osdir->access_tags().set_tag(distTag, curDist + 2);
						}
						doorsToProcess.push_back(osdir);
					}
				}
			}
		}
	}
#ifdef INSPECT_FILL_DISTANCES_FOR_DOORS
	output(TXT("filled layout cue distances \"%S\""), distTag.to_char());
#endif
}

void PilgrimageInstanceOpenWorld::fill_dir_for_order(DoorForOrder& _dfo, Framework::DoorInRoom* dir, ExistingCell* ec) const
{
	_dfo.inDir0.clear();
	_dfo.inDir1.clear();
	if (dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
	{
		if (auto* d = dir->get_door())
		{
			auto* aECAt = d->get_variables().get_existing<VectorInt2>(NAME(aCell));
			auto* bECAt = d->get_variables().get_existing<VectorInt2>(NAME(bCell));
			if (aECAt && bECAt)
			{
				if (ec->at == *aECAt)
				{
					_dfo.inDir0 = DirFourClockwise::vector_int2_to_dir(*bECAt - ec->at);
				}
				if (ec->at == *bECAt)
				{
					_dfo.inDir0 = DirFourClockwise::vector_int2_to_dir(*aECAt - ec->at);
				}
			}
		}
		else
		{
			an_assert(false);
		}
	}
	else
	{
		an_assert(dir->get_in_room());
		int dominantDist0 = 0;
		int dominantDist1 = 0;
		Optional<DirFourClockwise::Type> dominantDir0;
		Optional<DirFourClockwise::Type> dominantDir1;
		if (auto* odir = dir->get_door_on_other_side())
		{
			for_count(int, iDist, 2)
			{
				for_count(int, iDir, DirFourClockwise::NUM)
				{
					DirFourClockwise::Type checkDir = DirFourClockwise::Type(iDir);
					Name distTag;
					switch (checkDir)
					{
					case DirFourClockwise::Up:		distTag = iDist == 0 ? NAME(openWorldCellDistFromUp)	: NAME(layoutCueDistFromUp);		break;
					case DirFourClockwise::Right:	distTag = iDist == 0 ? NAME(openWorldCellDistFromRight)	: NAME(layoutCueDistFromRight);		break;
					case DirFourClockwise::Down:	distTag = iDist == 0 ? NAME(openWorldCellDistFromDown)	: NAME(layoutCueDistFromDown);		break;
					case DirFourClockwise::Left:	distTag = iDist == 0 ? NAME(openWorldCellDistFromLeft)	: NAME(layoutCueDistFromLeft);		break;
					default: an_assert(false); break;
					}

					if (dir->get_tags().has_tag(distTag) &&
						odir->get_tags().has_tag(distTag))
					{
						int hereDist = dir->get_tags().get_tag_as_int(distTag);
						int thereDist = odir->get_tags().get_tag_as_int(distTag);
						if (thereDist < hereDist)
						{
							if (dominantDist0 > thereDist || !dominantDir0.is_set())
							{
								dominantDir1 = dominantDir0;
								dominantDist1 = dominantDist0;
								dominantDir0 = checkDir;
								dominantDist0 = thereDist;
							}
							else if (dominantDist1 > thereDist || !dominantDir1.is_set())
							{
								dominantDir1 = checkDir;
								dominantDist1 = thereDist;
							}
						}
					}
				}
			}
			// layout cue start
			{
				for_count(int, iDir, DirFourClockwise::NUM)
				{
					DirFourClockwise::Type checkDir = DirFourClockwise::Type(iDir);
					Name layoutCueTag;
					switch (checkDir)
					{
					case DirFourClockwise::Up:		layoutCueTag = NAME(layoutCueStartUp);		break;
					case DirFourClockwise::Right:	layoutCueTag = NAME(layoutCueStartRight);	break;
					case DirFourClockwise::Down:	layoutCueTag = NAME(layoutCueStartDown);	break;
					case DirFourClockwise::Left:	layoutCueTag = NAME(layoutCueStartLeft);	break;
					default: an_assert(false); break;
					}

					if (dir->get_tags().has_tag(layoutCueTag))
					{
						dominantDir1 = dominantDir0;
						dominantDist1 = dominantDist0;
						dominantDir0 = checkDir;
						dominantDist0 = 0;
					}
				}
			}
			if (!dominantDir0.is_set())
			{
				// check if this is special door
				// do this only if no dominant dir found
				// for transition rooms that have some enforced direction we should already have dir set
				Name distTag = NAME(openWorldCellDistFromSpecial);
				int hereDist = dir->get_tags().get_tag_as_int(distTag);
				int thereDist = odir->get_tags().get_tag_as_int(distTag);
				if (thereDist < hereDist)
				{
					dominantDir0 = DirFourClockwise::local_to_world(ec->specialLocalDir, ec->dir);
					dominantDist0 = thereDist;
				}
			}
		}

		_dfo.inDir0 = dominantDir0;
		_dfo.inDir1 = dominantDir1;
	}
}

void PilgrimageInstanceOpenWorld::get_clockwise_ordered_doors_in(Framework::Room* _room, REF_ Array<Framework::DoorInRoom*>& _doors)
{
	an_assert(!_doors.is_empty(), TXT("nothing to order?"));

	ExistingCell* ec = nullptr;
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("get clockwise ordered doors in"));

		ec = get_existing_cell_for_already_locked(_room);
	}

	an_assert(ec);
	if (!ec)
	{
		return;
	}

	ARRAY_PREALLOC_SIZE(DoorForOrder, doors, _doors.get_size());

	// fill, get only door we want to order
	for_every_ptr(dir, _doors)
	{
		DoorForOrder dfo;
		dfo.dir = dir;
		dfo.dirIdx = NONE;
		for_every_ptr(rdir, _room->get_all_doors())
		{
			if (dir == rdir)
			{
				dfo.dirIdx = for_everys_index(rdir);
			}
		}
		fill_dir_for_order(dfo, dir, ec);

		an_assert(doors.has_place_left());
		doors.push_back(dfo);
	}

	// sort clockwise
	sort(doors);

	// copy to ordered doors
	{
		_doors.clear();
		for_every(d, doors)
		{
			_doors.push_back(d->dir);
		}
	}
}

Random::Generator PilgrimageInstanceOpenWorld::get_random_seed_for_cell_context() const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		Random::Generator rg;
		rg.set_seed(0, 0);
		return rg;
	}

	auto& c = get_cell_at(cellContext.get());

	return c.randomSeed;
}

DirFourClockwise::Type PilgrimageInstanceOpenWorld::get_dir_to_world_for_cell_context(DirFourClockwise::Type _localDir) const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return _localDir;
	}

	auto& c = get_cell_at(cellContext.get());

	return DirFourClockwise::local_to_world(_localDir, c.dir);
}

VectorInt2 PilgrimageInstanceOpenWorld::get_cell_context() const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return VectorInt2::zero;
	}

	return cellContext.get();
}

int PilgrimageInstanceOpenWorld::get_dir_exits_count_for_cell_context() const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return 0;
	}

	auto& c = get_cell_at(cellContext.get());

	int exitsCount = 0;
	for_count(int, i, DirFourClockwise::NUM)
	{
		exitsCount += c.exits[i]? 1 : 0;
	}

	return exitsCount;
}

int PilgrimageInstanceOpenWorld::get_pilgrimage_devices_count_for_cell_context() const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return 0;
	}

	auto& c = get_cell_at(cellContext.get());

	return c.knownPilgrimageDevices.get_size();
}

DirFourClockwise::Type PilgrimageInstanceOpenWorld::get_dir_for_cell_context() const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return DirFourClockwise::Up;
	}

	auto& c = get_cell_at(cellContext.get());

	return c.dir;
}

bool PilgrimageInstanceOpenWorld::has_same_depends_on_in_local_dir_for_cell_context(DirFourClockwise::Type _localDir) const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return 0;
	}

	auto& c = get_cell_at(cellContext.get());
	auto& o = get_cell_at(cellContext.get() + DirFourClockwise::dir_to_vector_int2(DirFourClockwise::local_to_world(_localDir, c.dir)));
	if (c.dependentOn.is_set() || o.dependentOn.is_set())
	{
		// dependent or if not, itself
		VectorInt2 cdoAt = c.dependentOn.get(c.at);
		VectorInt2 odoAt = o.dependentOn.get(o.at);
		return cdoAt == odoAt;
	}

	return false;
}

PilgrimageInstanceOpenWorld::CellSet const* PilgrimageInstanceOpenWorld::get_cell_set_for_cell_context() const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return nullptr;
	}

	auto& c = get_cell_at(cellContext.get());

	return &c.set;
}

Framework::RoomType const* PilgrimageInstanceOpenWorld::get_main_room_type_for_cell_context() const
{
	an_assert(cellContext.is_set());

	if (!cellContext.is_set())
	{
		warn(TXT("cell context not set!"));
		return nullptr;
	}

	auto& c = get_cell_at(cellContext.get());

	return c.mainRoomType.get();
}

void PilgrimageInstanceOpenWorld::add_scenery_in_room(Framework::Room* _room, Optional<bool> const& _vistaScenery, Optional<Transform> const & _relToPlacement)
{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	::System::TimeStamp timeStamp;
#endif

	bool vistaScenery = _vistaScenery.get(false);
	Transform relToPlacement = _relToPlacement.get(Transform::identity);

	Optional<VectorInt2> atCell;
	float cellDistanceVisibilityCoef = 1.0f;
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("add scenery in room"));
		for_every(cell, existingCells)
		{
			if (_room->is_in_region(cell->region.get()))
			{
				atCell = cell->at;
				if (!vistaScenery)
				{
					cellDistanceVisibilityCoef = cell->set.get_cell_distance_visibility_coef();
				}
			}
		}
	}
	if (atCell.is_set())
	{
		if (auto* owDef = pilgrimage->open_world__get_definition())
		{
			Name anchorVariableName = owDef->get_background_scenery_anchor_variable_name();
			Transform anchorPlacement = Transform::identity;
			if (anchorVariableName.is_valid())
			{
				anchorPlacement = _room->get_value<Transform>(anchorVariableName, Transform::identity);
			}

			if (auto* backgroundSceneryType = owDef->get_background_scenery_type())
			{
				float cellSize = owDef->get_cell_size();

				// generate meshes and combine them together into a singular mesh (fuse parts)
				// do this for all except 0,0 - this one should remain alone (due to movement collision)
				struct CombinedCells
				{
					struct MeshInstance
					{
						Framework::Mesh* mesh = nullptr;
						Transform placement;

						MeshInstance() {}
						MeshInstance(Framework::Mesh* _mesh, Transform const& _placement) : mesh(_mesh), placement(_placement) {}
					};
					ArrayStatic<MeshInstance, 30> meshes;
					VectorInt2 at; // relative
					bool centre = false; // centre is just 0,0 to have movement collision

					static void add(ArrayStatic<CombinedCells, COMBINED_CELLS_COUNT>& combinedCells, VectorInt2 const& o, Framework::Mesh* useMesh, Transform const & placement, int superCellCentre, int superCellSize)
					{
						if (o.is_zero())
						{
							CombinedCells cc;
							cc.at = o;
							cc.centre = true;
							cc.meshes.push_back(CombinedCells::MeshInstance(useMesh, placement));
							combinedCells.push_back(cc);
						}
						else
						{
							VectorInt2 cAt = o;
							cAt += VectorInt2(superCellCentre); // to start at centre up to the next super cell (-1,0,1) becomes (0,1,2), (-4,-3,-2) becomes (-3,-2,-1)
							cAt.x = cAt.x - mod(cAt.x, superCellSize); // to get to the centre (0,1,2) becomes (0)
							cAt.y = cAt.y - mod(cAt.y, superCellSize); // to get to the centre (-3,-2,-1) becomes (-3)
							bool found = false;
							for_every(cc, combinedCells)
							{
								if (cc->at == cAt)
								{
									found = true;
									cc->meshes.push_back(CombinedCells::MeshInstance(useMesh, placement));
									break;
								}
							}
							if (!found)
							{
								CombinedCells cc;
								cc.at = cAt;
								cc.meshes.push_back(CombinedCells::MeshInstance(useMesh, placement));
								combinedCells.push_back(cc);
							}
						}
					}
				};
				ArrayStatic<CombinedCells, COMBINED_CELLS_COUNT> combinedCells;

				int const superCellSize = owDef->get_combine_scenery_cells_size();
				int const superCellCentre = (superCellSize - 1) / 2;

				//an_assert(sqr(veryFarRadius * 2 / superCellSize) <= combinedCells.get_max_size());
				an_assert(sqr(superCellSize) <= CombinedCells().meshes.get_max_size());

				// combine cell meshes that should end up in the background
				{
					int radius = owDef->get_background_scenery_cell_distance_visibility();
					int veryFarRadius = max(radius, owDef->get_background_scenery_cell_distance_visibility_very_far());

					int maxRadius = veryFarRadius;
					// check all objectives valid for this pilgrimage
					for_every(objective, owDef->get_objectives())
					{
						if (!is_objective_ok(objective))
						{
							continue;
						}
						maxRadius = max(maxRadius, objective->calculate_visibility_distance(veryFarRadius));
					}

#ifdef SHORT_VISIBILITY
					radius = 1;
					veryFarRadius = 1;
					maxRadius = 1;
#endif

#ifdef AN_ALLOW_BULLSHOTS
					if (Framework::BullshotSystem::is_active())
					{
						// force distant
						cellDistanceVisibilityCoef = 1.0f;
					}
#endif

					radius = TypeConversions::Normal::f_i_closest(cellDistanceVisibilityCoef * (float)radius);
					veryFarRadius = TypeConversions::Normal::f_i_closest(cellDistanceVisibilityCoef * (float)veryFarRadius);
					maxRadius = TypeConversions::Normal::f_i_closest(cellDistanceVisibilityCoef * (float)maxRadius);

					for_range(int, oy, -maxRadius, maxRadius)
					{
						for_range(int, ox, -maxRadius, maxRadius)
						{
							VectorInt2 o(ox, oy);
							int dist = TypeConversions::Normal::f_i_closest(o.to_vector2().length());
							bool withinRadius = dist <= veryFarRadius;
							if (!withinRadius)
							{
								if (auto* objective = get_objective_to_create_cell(atCell.get() + o))
								{
									int objectiveDistance = objective->calculate_visibility_distance(veryFarRadius);
									withinRadius = dist <= objectiveDistance;
								}
							}
							if (withinRadius)
							{
								auto& c = access_cell_at(atCell.get() + o);
								int distIdx = o.is_zero()? (vistaScenery? 1 : 0) : (dist > radius? 4 : (dist > radius / 2? 3 : 2));
#ifdef AN_ALLOW_BULLSHOTS
								if (::Framework::BullshotSystem::is_active() && distIdx >= 3)
								{
									distIdx = 2;
								}
#endif
								c.generate_scenery_meshes(this, distIdx);
								if (auto* useMesh = (distIdx == 1 ? c.vistaWindowsSceneryMesh.get()
												 : ((distIdx == 0 && c.closeSceneryMesh.get())? c.closeSceneryMesh.get()
												 :  (distIdx == 3? c.medFarSceneryMesh.get()
												 :  (distIdx >= 4? c.veryFarSceneryMesh.get()
												 : c.farSceneryMesh.get())))))
								{
									if (useMesh->get_mesh()->get_parts_num() == 0)
									{
										continue;
									}

									// add to combine list
									todo_note(TXT("this is flat placement"));
									Vector3 location = o.to_vector2().to_vector3() * cellSize;
									Transform placement(location, DirFourClockwise::dir_to_rotator3_yaw(c.dir).to_quat());

									placement = anchorPlacement.to_world(placement);
									placement = relToPlacement.to_world(placement);

									CombinedCells::add(combinedCells, o, useMesh, placement, superCellCentre, superCellSize);
								}
							}
						}
					}
				}

				// get backgrounds cell based
				{
					for_every(bcd, owDef->get_backgrounds_cell_based())
					{
						int radius = owDef->get_background_scenery_cell_distance_visibility();
						radius = max(radius, owDef->get_background_scenery_cell_distance_visibility_very_far());

						radius += bcd->extraVisibilityRangeCellRadius;

						if (bcd->visibilityRange != 0.0f)
						{
							radius = TypeConversions::Normal::f_i_ceil(bcd->visibilityRange / cellSize);
						}

						for_range(int, oy, -radius, radius)
						{
							for_range(int, ox, -radius, radius)
							{
								VectorInt2 o(ox, oy);
								int dist = TypeConversions::Normal::f_i_closest(o.to_vector2().length());
								if (dist <= radius)
								{
									VectorInt2 at = atCell.get() + o;

									VectorInt2 relToBCD = at - bcd->at;
									if (bcd->repeatOffset.x != 0)
									{
										relToBCD.x = mod(relToBCD.x, bcd->repeatOffset.x);
									}
									if (bcd->repeatOffset.y != 0)
									{
										relToBCD.y = mod(relToBCD.y, bcd->repeatOffset.y);
									}
									
									for_every(ronaa, bcd->repeatOffsetNotAxisAligned)
									{
										if (!ronaa->is_zero())
										{
											Vector2 ro = ronaa->to_vector2();
											Vector2 roNormal = ro.normal();
											Vector2 r2bcd = relToBCD.to_vector2();
											float alongRO = Vector2::dot(r2bcd, roNormal);
											int alongROCount = TypeConversions::Normal::f_i_closest(alongRO);
											relToBCD -= *ronaa * alongROCount;
										}
									}

									if (relToBCD.is_zero())
									{
										// generate or use existing and add to combined cells
										Framework::UsedLibraryStored<Framework::Mesh> mesh;
										Cell::get_or_generate_background_mesh(OUT_ mesh, at, *bcd, this);

										if (auto* useMesh = mesh.get())
										{
											Vector3 location = o.to_vector2().to_vector3() * cellSize;
											Transform placement(location, Quat::identity);

											placement = anchorPlacement.to_world(placement);
											placement = relToPlacement.to_world(placement);

											CombinedCells::add(combinedCells, o, useMesh, placement, superCellCentre, superCellSize);
										}
									}
								}
							}
						}
					}
				}

				// use combine list to spawn actual scenery objects
				for_every(cc, combinedCells)
				{
					VectorInt2 at = atCell.get() + cc->at;

					auto& c = access_cell_at(at);

					auto prevCellContext = cellContext;
					cellContext = at;

					backgroundSceneryType->load_on_demand_if_required();

					Framework::Scenery* scenery = nullptr;
					Game::get()->perform_sync_world_job(TXT("spawn surrounding background scenery"), [&scenery, backgroundSceneryType, _room, at]()
					{
						scenery = new Framework::Scenery(backgroundSceneryType, String::printf(TXT("scenery %ix%i"), at.x, at.y));
						scenery->init(_room->get_in_sub_world());
					});

					Random::Generator rg = c.randomSeed;
					scenery->access_individual_random_generator() = rg;
					scenery->access_variables().access<bool>(NAME(noMovementCollision)) = ! cc->centre;

					// place it at the centre of our combined cell
					todo_note(TXT("this is flat placement"));
					Transform placement(cc->at.to_vector2().to_vector3() * cellSize, DirFourClockwise::dir_to_rotator3_yaw(c.dir).to_quat());

					placement = anchorPlacement.to_world(placement);
					placement = relToPlacement.to_world(placement);

					// combine meshes at the new placement
					Framework::Mesh* useMesh = nullptr;
					if (cc->meshes.get_size() == 1 &&
						Transform::same_with_orientation(placement, cc->meshes[0].placement))
					{
						// reuse it as it is
						useMesh = cc->meshes[0].mesh;
						if (auto* pcm = useMesh->get_precise_collision_model())
						{
							if (!pcm->get_model() || pcm->get_model()->is_empty())
							{
								warn_dev_investigate(TXT("model does not have precise collision, may fail visibility checks"));
							}
						}
						else
						{
							warn_dev_investigate(TXT("model does not have precise collision, may fail visibility checks"));
						}
					}
					else
					{
						an_assert(!cc->centre, TXT("should not be used for centre, we depend on collisions being set as they are"));
						useMesh = new Framework::MeshStatic(Framework::Library::get_current(), Framework::LibraryName::invalid());
						useMesh->be_temporary();

						Framework::CollisionModel* precisionCollisionModel = new Framework::CollisionModel(Framework::Library::get_current(), Framework::LibraryName::invalid());
						precisionCollisionModel->be_temporary();

						useMesh->make_sure_mesh_exists(); // so we have a place to fuse
						useMesh->set_precise_collision_model(precisionCollisionModel); // we have to set it, so it has a place to fuse precision collision meshes
						Array<Framework::Mesh::FuseMesh> fuseMeshes;
						for_every(m, cc->meshes)
						{
							Transform relativePlacement = placement.to_local(m->placement);
							fuseMeshes.push_back(Framework::Mesh::FuseMesh(m->mesh, relativePlacement));
						}
						useMesh->fuse(fuseMeshes);
						useMesh->set_missing_materials(Framework::Library::get_current());

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
						useMesh->get_mesh()->set_debug_mesh_name(String::printf(TXT("combined (%i) at %ix%i"), cc->meshes.get_size(), at.x, at.y));
#endif
					}

					scenery->initialise_modules([this, c, cc, useMesh](Framework::Module* _module)
					{
						if (useMesh)
						{
							if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
							{
								moduleAppearance->use_mesh(useMesh);
								float cellOnTextureSize = 0.2f;
								moduleAppearance->set_shader_param(NAME(textureFloorScale), cellOnTextureSize / get_cell_size());
								{
									todo_hack(TXT("there should be something better than this, this will introduce discontinuity issues"));
									VectorInt2 cat = c.at;
									int discontinuityHackSize = 20;
									int discontinuityHackSizeHalf = discontinuityHackSize / 2;
									cat.x += discontinuityHackSizeHalf;
									cat.y += discontinuityHackSizeHalf;
									cat.x = mod(cat.x, discontinuityHackSize);
									cat.y = mod(cat.y, discontinuityHackSize);
									cat.x -= discontinuityHackSizeHalf;
									cat.y -= discontinuityHackSizeHalf;
									moduleAppearance->set_shader_param(NAME(textureFloorOffset), cat.to_vector2() * cellOnTextureSize);
								}
								Vector2 yAxis = DirFourClockwise::dir_to_vector_int2(c.dir).to_vector2();
								moduleAppearance->set_shader_param(NAME(textureFloorX), yAxis.rotated_right());
								moduleAppearance->set_shader_param(NAME(textureFloorY), yAxis);
								if (!cc->centre)
								{
									moduleAppearance->access_mesh_instance().set_rendering_order_priority(100); // higher priority, should be rendered later
								}
							}
						}
					});

					Game::get()->perform_sync_world_job(TXT("place background scenery"), [&scenery, _room, placement]()
					{
						scenery->get_presence()->place_in_room(_room, placement);
					});

					Game::get()->on_newly_created_object(scenery);

					cellContext = prevCellContext;
				}
			}
			if (auto* bdScenObj = owDef->get_background_scenery_objects(atCell.get()))
			{
				for_every(bs, *bdScenObj)
				{
					if (bs->forBullshots.is_set())
					{
						bool bullshotSystemActive = false;
#ifdef AN_ALLOW_BULLSHOTS
						bullshotSystemActive = Framework::BullshotSystem::is_active();
#endif
						if (bs->forBullshots.get() != bullshotSystemActive)
						{
							continue;
						}
					}
					bs->sceneryType->load_on_demand_if_required();

					Framework::Scenery* scenery = nullptr;
					Game::get()->perform_sync_world_job(TXT("spawn background scenery"), [&scenery, bs, _room]()
						{
							scenery = new Framework::Scenery(bs->sceneryType.get(), String::printf(TXT("scenery object")));
							scenery->init(_room->get_in_sub_world());
						});

					auto& c = get_cell_at(atCell.get());

					Random::Generator rg = c.randomSeed;
					rg.advance_seed(for_everys_index(bs), 2);
					scenery->access_individual_random_generator() = rg;

					scenery->initialise_modules();

					Transform placement = bs->placement;

					{
						Vector3 applyCellOffset = Vector3(bs->applyCellOffsetX ? 1.0f : 0.0f, bs->applyCellOffsetY ? 1.0f : 0.0f, 0.0f);
						if (!applyCellOffset.is_zero())
						{
							placement.set_translation(placement.get_translation() - (atCell.get().to_vector2().to_vector3() * applyCellOffset) * get_cell_size());
						}
					}

					placement = anchorPlacement.to_world(placement);
					placement = relToPlacement.to_world(placement);

					Game::get()->perform_sync_world_job(TXT("place background scenery"), [&scenery, _room, placement]()
					{
						scenery->get_presence()->place_in_room(_room, placement);
					});

					Game::get()->on_newly_created_object(scenery);

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output_colour(0, 1, 1, 1);
					output(TXT("added background scenery object %S"), bs->sceneryType.get_name().to_string().to_char());
					output_colour();
#endif
				}
			}
		}
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output_colour(0, 1, 1, 1);
		output(TXT("background sceneries for %i x %i added in %.2fs"), atCell.get().x, atCell.get().y, timeStamp.get_time_since());
		output_colour();
#endif
	}
}

void PilgrimageInstanceOpenWorld::keep_possible_door_leads(Framework::Room * _room, REF_ DoorLeadsTowardParams& _params)
{
	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		Optional<VectorInt2> at = piow->find_cell_at(_room);
		if (at.is_set())
		{
			if (auto* cell = piow->get_existing_cell(at.get()))
			{
				if (!cell->towardsObjectiveIdx.is_set())
				{
					_params.toObjective = false;
				}
				if (_params.toAmmo || _params.toHealth)
				{
					bool toAmmoFound = false;
					bool toHealthFound = false;
					for_every(pd, cell->pilgrimageDevices)
					{
						if (auto* pdDevice = pd->device.get())
						{
							if (pdDevice->does_provide(EnergyType::Ammo))
							{
								toAmmoFound = true;
							}
							if (pdDevice->does_provide(EnergyType::Health))
							{
								toHealthFound = true;
							}
						}
					}
					_params.toAmmo &= toAmmoFound;
					_params.toHealth &= toHealthFound;
				}
			}
		}
	}
}

Optional<bool> PilgrimageInstanceOpenWorld::does_door_lead_towards(Framework::DoorInRoom const* _dir, DoorLeadsTowardParams const & _params, OPTIONAL_ OUT_ int * _dist)
{
	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		Optional<VectorInt2> at = piow->find_cell_at(_dir->get_in_room());
		if (at.is_set())
		{
			if (auto* cell = piow->get_existing_cell(at.get()))
			{
				if (!cell->towardsObjectiveIdx.is_set())
				{
					return NP;
				}

				Optional<DirFourClockwise::Type> useTowardsObjectiveDir = cell->towardsObjective.is_set() ? cell->towardsObjective : cell->towardsLocalObjectiveDir;
				bool useTowardsObjectiveNoDir = cell->towardsLocalObjectiveNoDir;

				if (cell->towardsPilgrimageDevice.is_set())
				{
					useTowardsObjectiveDir.clear();
					useTowardsObjectiveNoDir = false;
				}

				if (_dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
				{
					if (useTowardsObjectiveDir.is_set())
					{
						if (auto* d = _dir->get_door())
						{
							auto* aECAt = d->get_variables().get_existing<VectorInt2>(NAME(aCell));
							auto* bECAt = d->get_variables().get_existing<VectorInt2>(NAME(bCell));
							if (aECAt && bECAt)
							{
								Optional<DirFourClockwise::Type> foundDir;
								if (d->get_linked_door_a() == _dir)
								{
									foundDir = DirFourClockwise::vector_int2_to_dir(*bECAt - *aECAt);
								}
								if (d->get_linked_door_b() == _dir)
								{
									foundDir = DirFourClockwise::vector_int2_to_dir(*aECAt - *bECAt);
								}
								if (foundDir.is_set() &&
									useTowardsObjectiveDir.get() == foundDir.get())
								{
									if (_params.toObjective)
									{
										assign_optional_out_param(_dist, 0); // can't be any closest
										return true;
									}
								}
							}
						}
					}
					return false;
				}
				else
				{
					for_count(int, checkParam, 2)
					{
						Name distTag;
						switch (checkParam)
						{
						case 0: if (_params.toObjective)
							{
								if (auto* pd = cell->towardsPilgrimageDevice.get())
								{
									distTag = pd->get_open_world_cell_dist_tag();
								}
								else if (useTowardsObjectiveNoDir)
								{
									distTag = NAME(openWorldCellDistFromObjective);
								}
								else if (useTowardsObjectiveDir.is_set())
								{
									switch (useTowardsObjectiveDir.get())
									{
									case DirFourClockwise::Up: distTag = NAME(openWorldCellDistFromUp); break;
									case DirFourClockwise::Right: distTag = NAME(openWorldCellDistFromRight); break;
									case DirFourClockwise::Down: distTag = NAME(openWorldCellDistFromDown); break;
									case DirFourClockwise::Left: distTag = NAME(openWorldCellDistFromLeft); break;
									default: an_assert(false); break;
									}
								}
							}
							break;
						case 1: if (_params.toAmmo || _params.toHealth)
							{
								for_every(pd, cell->pilgrimageDevices)
								{
									if (auto* pdDevice = pd->device.get())
									{
										if ((_params.toAmmo && pdDevice->does_provide(EnergyType::Ammo))
										 || (_params.toHealth && pdDevice->does_provide(EnergyType::Health)))
										{
											bool depleted = false;
											if (auto* kpd = cell->get_known_pilgrimage_device(pdDevice))
											{
												depleted = kpd->depleted;
											}
											if (!depleted)
											{
												distTag = pdDevice->get_open_world_cell_dist_tag();
											}
										}
									}
								}
							}
							break;
						default: break;
						}
						if (distTag.is_valid())
						{
							if (auto* odir = _dir->get_door_on_other_side())
							{
								int hereDist = _dir->get_tags().get_tag_as_int(distTag);
								int thereDist = odir->get_tags().get_tag_as_int(distTag);
								if (thereDist < hereDist)
								{
									assign_optional_out_param(_dist, hereDist);
									return true;
								}
							}
						}
					}
					return false;
				}
			}
		}
	}
	return NP;
}

bool PilgrimageInstanceOpenWorld::does_door_lead_in_dir(Framework::DoorInRoom const* _dir, DirFourClockwise::Type _inDir)
{
	if (_dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
	{
		if (auto* d = _dir->get_door())
		{
			auto* aECAt = d->get_variables().get_existing<VectorInt2>(NAME(aCell));
			auto* bECAt = d->get_variables().get_existing<VectorInt2>(NAME(bCell));
			if (aECAt && bECAt)
			{
				Optional<DirFourClockwise::Type> foundDir;
				if (d->get_linked_door_a() == _dir)
				{
					foundDir = DirFourClockwise::vector_int2_to_dir(*bECAt - *aECAt);
				}
				if (d->get_linked_door_b() == _dir)
				{
					foundDir = DirFourClockwise::vector_int2_to_dir(*aECAt - *bECAt);
				}
				if (foundDir.is_set())
				{
					return _inDir == foundDir.get();
				}
			}
		}
		return false;
	}
	else
	{
		Name distTag;
		switch (_inDir)
		{
		case DirFourClockwise::Up: distTag = NAME(openWorldCellDistFromUp); break;
		case DirFourClockwise::Right: distTag = NAME(openWorldCellDistFromRight); break;
		case DirFourClockwise::Down: distTag = NAME(openWorldCellDistFromDown); break;
		case DirFourClockwise::Left: distTag = NAME(openWorldCellDistFromLeft); break;
		default: an_assert(false); break;
		}

		if (auto* odir = _dir->get_door_on_other_side())
		{
			int hereDist = _dir->get_tags().get_tag_as_int(distTag);
			int thereDist = odir->get_tags().get_tag_as_int(distTag);
			if (thereDist < hereDist)
			{
				return true;
			}
		}
		return false;
	}
}

int PilgrimageInstanceOpenWorld::get_door_distance_in_dir(Framework::DoorInRoom const* _dir, DirFourClockwise::Type _inDir)
{
	if (_dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
	{
		if (auto* d = _dir->get_door())
		{
			auto* aECAt = d->get_variables().get_existing<VectorInt2>(NAME(aCell));
			auto* bECAt = d->get_variables().get_existing<VectorInt2>(NAME(bCell));
			if (aECAt && bECAt)
			{
				Optional<DirFourClockwise::Type> foundDir;
				if (d->get_linked_door_a() == _dir)
				{
					foundDir = DirFourClockwise::vector_int2_to_dir(*bECAt - *aECAt);
				}
				if (d->get_linked_door_b() == _dir)
				{
					foundDir = DirFourClockwise::vector_int2_to_dir(*aECAt - *bECAt);
				}
				if (foundDir.is_set() && foundDir.get() == _inDir)
				{
					return 0;
				}
			}
		}
	}
	{
		Name distTag;
		switch (_inDir)
		{
		case DirFourClockwise::Up: distTag = NAME(openWorldCellDistFromUp); break;
		case DirFourClockwise::Right: distTag = NAME(openWorldCellDistFromRight); break;
		case DirFourClockwise::Down: distTag = NAME(openWorldCellDistFromDown); break;
		case DirFourClockwise::Left: distTag = NAME(openWorldCellDistFromLeft); break;
		default: an_assert(false); break;
		}

		int hereDist = _dir->get_tags().get_tag_as_int(distTag);
		return hereDist;
	}
}

// returns true for special colour
bool PilgrimageInstanceOpenWorld::fill_direction_texture_parts_for(Framework::DoorInRoom const* _dir, OUT_ ArrayStack<Framework::TexturePartUse>& _dtp)
{
	if (GameDirector::get()->is_guidance_blocked())
	{
		return false;
	}

	struct DTPUtil
	{
		static void add(REF_ ArrayStack<Framework::TexturePartUse>& _dtp, Framework::TexturePartUse const & _add)
		{
			if (!_add.texturePart)
			{
				return;
			}
			for_every(dtp, _dtp)
			{
				if (_add == *dtp)
				{
					return;
				}
			}
			_dtp.push_back(_add);
		}
	};
	
	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		if (auto* r = _dir->get_in_room())
		{
			if (r->is_small_room())
			{
				// too small room
				return false;
			}
		}
		auto* owDef = piow->pilgrimage->open_world__get_definition();

		bool showDirectionsAsObjective = GameDirector::get()->is_guidance_simplified();
		bool showDirectionsOnly = GameDirector::get()->is_guidance_simplified(); // only path, no pilgrimage devices
		bool showDirectionsOnlyToObjective = GameSettings::get().difficulty.showDirectionsOnlyToObjective; // this still allows showing directions to pilgrimage devices
		bool showDirectionsOnlyToObjectiveNow = showDirectionsOnlyToObjective;
		bool showAllDoorDirectionsNow = ! showDirectionsOnlyToObjective;

		// if showing objective but with map, show all
		if (! showAllDoorDirectionsNow && PlayerPreferences::should_show_map())
		{
			todo_multiplayer_issue(TXT("we just get player here"));
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
					{
						auto& poi = mp->access_overlay_info();
						if (poi.is_actually_showing_map())
						{
							showAllDoorDirectionsNow = true;
						}
					}
				}
			}
		}

		ExistingCell* cell;
		if (piow->startingRoom == _dir->get_in_room())
		{
			Concurrency::ScopedMRSWLockRead lock(piow->existingCellsLock, TXT("fill_direction_texture_parts_for"));
			cell = piow->get_existing_cell_already_locked(piow->get_start_at());
		}
		else
		{
			Concurrency::ScopedMRSWLockRead lock(piow->existingCellsLock, TXT("fill_direction_texture_parts_for"));
			cell = piow->get_existing_cell_for_already_locked(_dir->get_in_room());
		}

		Optional<DirFourClockwise::Type> towardsObjective = cell ? (cell->towardsObjective.is_set()? cell->towardsObjective : cell->towardsLocalObjectiveDir) : NP;
		PilgrimageDevice* towardsPilgrimageDevice = nullptr;

		if (cell && cell->towardsPilgrimageDevice.is_set())
		{
			towardsObjective.clear();
			towardsPilgrimageDevice = cell->towardsPilgrimageDevice.get();
		}

		if (cell && piow->doorDirectionObjectiveOverride.is_set())
		{
			for_every(cpd, cell->pilgrimageDevices)
			{
				if (auto* pd = cpd->device.get())
				{
					if (pd->get_door_direction_objective_override() == piow->doorDirectionObjectiveOverride.get())
					{
						towardsObjective.clear();
						towardsPilgrimageDevice = pd;
					}
				}
			}
		}

		GetRightDoorDirectionTexturePartUse getRightDoorDirectionTexturePartUse(owDef, cell, showDirectionsAsObjective);

		if (_dir->get_door()->is_world_separator_door_due_to(WorldSeparatorReason::CellSeparator))
		{
			if (auto* d = _dir->get_door())
			{
				auto* aECAt = d->get_variables().get_existing<VectorInt2>(NAME(aCell));
				auto* bECAt = d->get_variables().get_existing<VectorInt2>(NAME(bCell));
				if (aECAt && bECAt)
				{
					Optional<DirFourClockwise::Type> foundDir;
					if (d->get_linked_door_a() == _dir)
					{
						foundDir = DirFourClockwise::vector_int2_to_dir(*bECAt - *aECAt);
					}
					if (d->get_linked_door_b() == _dir)
					{
						foundDir = DirFourClockwise::vector_int2_to_dir(*aECAt - *bECAt);
					}
					if (foundDir.is_set() && owDef)
					{
						if (showAllDoorDirectionsNow)
						{
							auto dtp = getRightDoorDirectionTexturePartUse.get_for(foundDir.get());
							if (dtp.texturePart)
							{
								DTPUtil::add(_dtp, dtp);
							}
						}
						if (showDirectionsOnlyToObjectiveNow && towardsObjective.is_set() && towardsObjective.get() == foundDir.get())
						{
							if (auto* tp = owDef->get_towards_objective_texture_part())
							{
								DTPUtil::add(_dtp, Framework::TexturePartUse::as_is(tp));
							}
						}
					}
				}
			}

			return ! showDirectionsOnlyToObjective; // treat is as normal one if showing only to objective
		}
		else
		{
			{
				if (auto* odir = _dir->get_door_on_other_side())
				{
					Name distTag = NAME(openWorldCellDistFromObjective);
					int hereDist = _dir->get_tags().get_tag_as_int(distTag);
					int thereDist = odir->get_tags().get_tag_as_int(distTag);
					if (thereDist < hereDist && owDef)
					{
						if (auto* tp = owDef->get_towards_objective_texture_part())
						{
							DTPUtil::add(_dtp, Framework::TexturePartUse::as_is(tp));
						}
					}
				}
			}
			for_count(int, iDir, DirFourClockwise::NUM)
			{
				DirFourClockwise::Type checkDir = DirFourClockwise::Type(iDir);
				bool showObjective = showDirectionsOnlyToObjectiveNow && towardsObjective.is_set() && towardsObjective.get() == checkDir;
				if (!showAllDoorDirectionsNow &&
					!showObjective)
				{
					// skip
					continue;
				}

				Name distTag;
				switch (checkDir)
				{
				case DirFourClockwise::Up: distTag = NAME(openWorldCellDistFromUp); break;
				case DirFourClockwise::Right: distTag = NAME(openWorldCellDistFromRight); break;
				case DirFourClockwise::Down: distTag = NAME(openWorldCellDistFromDown); break;
				case DirFourClockwise::Left: distTag = NAME(openWorldCellDistFromLeft); break;
				default: an_assert(false); break;
				}

				if (auto* odir = _dir->get_door_on_other_side())
				{
					int hereDist = _dir->get_tags().get_tag_as_int(distTag);
					int thereDist = odir->get_tags().get_tag_as_int(distTag);
					if (thereDist < hereDist && owDef)
					{
						if (showAllDoorDirectionsNow)
						{
							auto dtp = getRightDoorDirectionTexturePartUse.get_for(checkDir);
							if (dtp.texturePart)
							{
								DTPUtil::add(_dtp, dtp);
							}
						}
						if (showObjective)
						{
							if (auto* tp = owDef->get_towards_objective_texture_part())
							{
								DTPUtil::add(_dtp, Framework::TexturePartUse::as_is(tp));
							}
						}
					}
				}
			}
			if (cell && !showDirectionsOnly)
			{
				struct Util
				{
					ArrayStack<Framework::TexturePartUse>& _dtp;

					bool showDirectionsOnlyToNonDepleted = GameSettings::get().difficulty.showDirectionsOnlyToNonDepleted;
					bool pathOnly = GameSettings::get().difficulty.pathOnly;
					Framework::DoorInRoom const * dir = nullptr;
					ExistingCell* cell = nullptr;
					PilgrimageDevice* towardsPilgrimageDevice = nullptr;
					PilgrimageInstanceOpenWorld* piow = nullptr;
					PilgrimageOpenWorldDefinition const* owDef = nullptr;

					Util(ArrayStack<Framework::TexturePartUse>& __dtp, bool _showDirectionsOnly)
					: _dtp(__dtp)
					{
						pathOnly |= _showDirectionsOnly;
					}

					void add_dtp(PilgrimageDevice* pdDevice)
					{
						if (! pdDevice->should_show_door_directions()) // should_show_door_directions_only_if_depleted+should_show_door_directions_only_if_not_depleted+should_show_door_directions_only_if_visited is via texture part
						{
							return;
						}
						Name const& distTag = pdDevice->get_open_world_cell_dist_tag();
						if (distTag.is_valid())
						{
							if (auto* odir = dir->get_door_on_other_side())
							{
								int hereDist = dir->get_tags().get_tag_as_int(distTag);
								int thereDist = odir->get_tags().get_tag_as_int(distTag);
								if (thereDist < hereDist)
								{
									if (!pathOnly)
									{
										bool showDirection = true;
										auto* kpd = cell->get_known_pilgrimage_device(pdDevice);
										if (showDirectionsOnlyToNonDepleted && kpd)
										{
											if (kpd->depleted)
											{
												// skip
												showDirection = false;
											}
										}
										if (showDirection)
										{
											if (auto* tp = piow->get_pilgrimage_device_direction_texture_part(pdDevice, kpd))
											{
												DTPUtil::add(_dtp, Framework::TexturePartUse::as_is(tp));
											}
										}
									}
									if (towardsPilgrimageDevice == pdDevice)
									{
										if (auto* tp = owDef->get_towards_objective_texture_part())
										{
											DTPUtil::add(_dtp, Framework::TexturePartUse::as_is(tp));
										}
									}
								}
							}
						}
					}
				};
				Util util(_dtp, showDirectionsOnly);
				util.dir = _dir;
				util.cell = cell;
				util.towardsPilgrimageDevice = towardsPilgrimageDevice;
				util.piow = piow;
				util.owDef = owDef;
				for_every(pd, cell->pilgrimageDevices)
				{
					util.add_dtp(pd->device.get());
				}
				util.showDirectionsOnlyToNonDepleted = false; // forced should always be seen (we don't store them anyway)
				for_every(fd, cell->forcedDestinations)
				{
					util.add_dtp(fd->device.get());
				}
			}

			return false;
		}
	}

	return false;
}

Optional<DirFourClockwise::Type> PilgrimageInstanceOpenWorld::query_towards_objective_at(VectorInt2 const& _at) const
{
	Cell* c;
	if (get_cell(_at, c))
	{
		return c->towardsObjective;
	}

	return NP;
}

Vector2 PilgrimageInstanceOpenWorld::calculate_rel_centre_for(VectorInt2 const& _at) const
{
	// uses simplified query procedure

	CellQueryResult cqr;

	cqr.at = _at;

	if (is_cell_address_valid(_at))
	{
		cqr.randomSeed = get_random_seed_at(_at);

		for_every(cell, cells)
		{
			if (cell->at == _at)
			{
				cqr.dir = cell->dir;
				cqr.cellSet.any = cell->set.get_as_any();
			}
		}
	}

	return cqr.calculate_rel_centre();
}

Vector2 PilgrimageInstanceOpenWorld::calculate_random_small_offset_for(VectorInt2 const& _at, float radius, int rgOffset) const
{
	// uses simplified query procedure

	Random::Generator rg(0, 0);
	
	if (is_cell_address_valid(_at))
	{
		rg = get_random_seed_at(_at);
	}

	rg.advance_seed(23480 + rgOffset, 1495);

	Vector2 offset;
	offset.x = rg.get(Range(-radius, radius));
	offset.y = rg.get(Range(-radius, radius));

	return offset;
}

PilgrimageInstanceOpenWorld::CellQueryResult PilgrimageInstanceOpenWorld::query_cell_at(VectorInt2 const& _at) const
{
	CellQueryResult cqr;

	cqr.at = _at;

	if (is_cell_address_valid(_at))
	{
		cqr.randomSeed = get_random_seed_at(_at);

		{
			Concurrency::ScopedSpinLock lock(visitedCellsLock);
			cqr.visited = visitedCells.does_contain(_at);
		}

		if (cqr.visited)
		{
			cqr.known = true;
			cqr.knownExits = true;
			cqr.knownDevices = true;
			cqr.knownSpecialDevices = true;
		}
		else
		{
			{
				Concurrency::ScopedSpinLock lock(knownDevicesCellsLock);
				cqr.knownDevices = knownDevicesCells.does_contain(_at);
			}

			{
				Concurrency::ScopedSpinLock lock(knownSpecialDevicesCellsLock);
				cqr.knownSpecialDevices = knownSpecialDevicesCells.does_contain(_at);
			}

			{
				Concurrency::ScopedSpinLock lock(knownExitsCellsLock);
				cqr.knownExits = knownExitsCells.does_contain(_at);
			}

			{
				Concurrency::ScopedSpinLock lock(knownCellsLock);
				cqr.known = knownCells.does_contain(_at);
			}
		}
	}

	cqr.known |= cqr.visited;
	for_count(int, i, DirFourClockwise::NUM)
	{
		cqr.exits[i] = false;
		cqr.sideExits[i] = 0;
	}

	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("query_cell_at"));

		if (cqr.known || cqr.knownExits || cqr.knownDevices || cqr.knownSpecialDevices)
		{

			VectorInt2 atU = _at + VectorInt2( 0,  1);
			VectorInt2 atR = _at + VectorInt2( 1,  0);
			VectorInt2 atD = _at + VectorInt2( 0, -1);
			VectorInt2 atL = _at + VectorInt2(-1,  0);
			int dataGathered = 0;
			for_every(cell, cells)
			{
				if (cell->at == atU)
				{
					cqr.sideExits[DirFourClockwise::Up] = cell->sideExitsLeftRight;
					++dataGathered;
				}
				if (cell->at == atR)
				{
					cqr.sideExits[DirFourClockwise::Right] = cell->sideExitsUpDown;
					++dataGathered;
				}
				if (cell->at == atD)
				{
					cqr.sideExits[DirFourClockwise::Down] = cell->sideExitsLeftRight;
					++dataGathered;
				}
				if (cell->at == atL)
				{
					cqr.sideExits[DirFourClockwise::Left] = cell->sideExitsUpDown;
					++dataGathered;
				}
				if (cell->at == _at)
				{
					cqr.rotateSlots = cell->rotateSlots;
					//bool someUnknown = false;
					for_every(kpd, cell->knownPilgrimageDevices)
					{
						/*
						if (!someUnknown && !is_pilgrimage_device_direction_known(kpd->device.get()))
						{
							someUnknown = true;
						}
						*/
						if (kpd->device->is_special())
						{
							if (cqr.knownSpecialDevices)
							{
								cqr.knownPilgrimageDevices.push_back(*kpd);
							}
						}
						else
						{
							if (cqr.knownDevices)
							{
								cqr.knownPilgrimageDevices.push_back(*kpd);
							}
						}
					}
					/*
					if (! someUnknown)
					{
						cqr.rotateSlots = 0; // do not rotate them
					}
					*/
					if (cell->doneForMinPriority == 0)
					{
						if (cqr.known || cqr.knownExits)
						{
							cqr.dir = cell->dir;
							cqr.cellSet.any = cell->set.get_as_any();
							//if (auto* a = cqr.cellSet.any) { a->load_on_demand_if_required(); }
							cqr.lineModel = cell->lineModel.get();
							cqr.lineModelAdditional = cell->lineModelAdditional.get();
							cqr.mapColour = cell->mapColour;
							if (cqr.knownExits)
							{
								memory_copy(cqr.exits, cell->exits, DirFourClockwise::NUM * sizeof(bool));
							}
						}
						else
						{
							cqr.dir = cell->dir;

							cqr.lineModelAlwaysOtherwise = cell->lineModelAlwaysOtherwise.get();
						}
						for_every(lm, cell->objectiveLineModels)
						{
							if (cqr.objectiveLineModels.has_place_left())
							{
								cqr.objectiveLineModels.push_back(lm->get());
							}
						}
						if (cqr.known)
						{
							for_every(lm, cell->objectiveLineModelsIfKnown)
							{
								if (lm->distance == 0.0f)
								{
									if (cqr.objectiveLineModels.has_place_left())
									{
										cqr.objectiveLineModels.push_back(lm->lineModel.get());
									}
								}
							}
						}
					}
					++dataGathered;
				}
				if (dataGathered >= 5)
				{
					break;
				}
			}
		}
		else if (GameSettings::get().difficulty.showLineModelAlwaysOtherwiseOnMap)
		{
			for_every(cell, cells)
			{
				if (cell->at == _at)
				{
					cqr.rotateSlots = cell->rotateSlots;
					cqr.dir = cell->dir;
					for_every(lm, cell->objectiveLineModels)
					{
						if (!cqr.objectiveLineModels.has_place_left())
						{
							cqr.objectiveLineModels.push_back(lm->get());
						}
					}
					if (cell->lineModelAlwaysOtherwise.get())
					{
						if (cell->doneForMinPriority == 0)
						{
							cqr.lineModelAlwaysOtherwise = cell->lineModelAlwaysOtherwise.get();
						}
					}
				}
			}
		}

		{
			for_every(cell, cells)
			{
				if (cell->at == _at)
				{
					for_every(lm, cell->objectiveLineModelsIfKnown)
					{
						if (lm->distance > 0.0f)
						{
							bool addLineModel = false;
							float distSq = sqr(lm->distance);
							{
								Concurrency::ScopedSpinLock lock(knownCellsLock);
								for_every(at, knownCells)
								{
									float distToAt = (*at - _at).to_vector2().length_squared();
									if (distToAt <= distSq + 0.001f)
									{
										addLineModel = true;
										break;
									}
								}
							}

							if (addLineModel)
							{
								if (cqr.objectiveLineModels.has_place_left())
								{
									cqr.objectiveLineModels.push_back(lm->lineModel.get());
								}
							}
						}
					}
				}
			}
		}
	}

	// workaround to show upgrade machine in the starting room
	if (startingRoomUpgradeMachine.get() && startingRoomUpgradeMachinePilgrimageDevice.get())
	{
		if (_at == get_start_at())
		{
			KnownPilgrimageDevice kpd;
			kpd.depleted = upgradeInStartingRoomUnlocked;
			kpd.device = startingRoomUpgradeMachinePilgrimageDevice;
			cqr.knownPilgrimageDevices.push_back(kpd);
		}
	}

	return cqr;
}

void PilgrimageInstanceOpenWorld::rotate_slot(REF_ Vector2 & _slot, int _rotateSlots)
{
	_rotateSlots = mod(_rotateSlots, 4);
	for_count(int, i, _rotateSlots)
	{
		_slot.rotate_right();
	}
}

PilgrimageInstanceOpenWorld::CellQueryTexturePartsResult PilgrimageInstanceOpenWorld::query_cell_texture_parts_at(VectorInt2 const& _at) const
{
	CellQueryTexturePartsResult cqtpr;
	Cell* cell = nullptr;
	if (get_cell(_at, OUT_ cell))
	{
		auto* owDef = pilgrimage->open_world__get_definition();

		if (owDef)
		{
			GetRightDoorDirectionTexturePartUse getRightDoorDirectionTexturePartUse(owDef, cell);

			for_count(int, dirIdx, DirFourClockwise::NUM)
			{
				if (cell->exits[dirIdx])
				{
					CellQueryTexturePartsResult::Element e;
					e.offset = DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)dirIdx).to_vector2();
					e.texturePartUse = getRightDoorDirectionTexturePartUse.get_exit_to((DirFourClockwise::Type)dirIdx);
					e.texturePartUseEntrance = getRightDoorDirectionTexturePartUse.get_entered_from((DirFourClockwise::Type)dirIdx);
					e.cellDir = DirFourClockwise::dir_to_vector_int2((DirFourClockwise::Type)dirIdx);
					cqtpr.elements.push_back(e);
				}
			}
		}

		for_every(kpd, cell->knownPilgrimageDevices)
		{
			if (auto* tp = get_pilgrimage_device_direction_texture_part(kpd->device.get(), kpd))
			{
				CellQueryTexturePartsResult::Element e;
				e.offset = kpd->device->get_slot_offset();
				if (!e.offset.is_zero())
				{
					rotate_slot(e.offset, cell->rotateSlots);
					e.texturePartUse = Framework::TexturePartUse::as_is(tp);
					cqtpr.elements.push_back(e);
				}
			}
		}
	}

	return cqtpr;
}

void PilgrimageInstanceOpenWorld::mark_cell_visited(VectorInt2 const& _at)
{
	if (!is_cell_address_valid(_at))
	{
		return;
	}
	bool visitedNewOne = false;
	{
		Concurrency::ScopedSpinLock lock(visitedCellsLock);
		if (!visitedCells.does_contain(_at))
		{
			visitedNewOne = true;
			visitedCells.push_back(_at);
		}
	}
	if (visitedNewOne)
	{
		if (auto* ms = MissionState::get_current())
		{
			if (ms->has_started() && // we're not at the ship anymore
				_at != get_start_at()) // not at the starting one, we have to move somewhere further
			{
				ms->visited_cell();
			}
		}
	}
}

bool PilgrimageInstanceOpenWorld::is_cell_address_valid(VectorInt2 const& _at) const
{
	auto* owDef = pilgrimage->open_world__get_definition();

	VectorInt2 startAt = get_start_at();
	RangeInt2 size = owDef->get_size();

	if (_at == startAt)
	{
		return true;
	}

	if ((size.x.is_empty() || size.x.does_contain(_at.x)) &&
		(size.y.is_empty() || size.y.does_contain(_at.y)))
	{
		return true;
	}

	if (get_objective_to_create_cell(_at))
	{
		return true;
	}

	return false;
}

bool PilgrimageInstanceOpenWorld::mark_cell_known_exits(VectorInt2 const& _at, int _radius)
{
	bool result = false;
	Concurrency::ScopedSpinLock lock(knownExitsCellsLock);
	if (is_cell_address_valid(_at) &&
		!knownExitsCells.does_contain(_at))
	{
		knownExitsCells.push_back(_at);
		result = true;
	}
	if (_radius > 0)
	{
		for_range(int, oy, -_radius, _radius)
		{
			for_range(int, ox, -_radius, _radius)
			{
				VectorInt2 o(ox, oy);
				if (!o.is_zero())
				{
					VectorInt2 at = _at + o;
					int dist = TypeConversions::Normal::f_i_closest(o.to_vector2().length());
					if (dist <= _radius)
					{
						if (is_cell_address_valid(at) &&
							!knownExitsCells.does_contain(at))
						{
							knownExitsCells.push_back(at);
							result = true;
						}
					}
				}
			}
		}
	}
	if (result)
	{
		issue_make_sure_known_cells_are_ready();
	}
	return result;
}

bool PilgrimageInstanceOpenWorld::mark_cell_known_devices(VectorInt2 const& _at, int _radius)
{
	bool result = false;
	Concurrency::ScopedSpinLock lock(knownDevicesCellsLock);
	if (is_cell_address_valid(_at) &&
		!knownDevicesCells.does_contain(_at))
	{
		knownDevicesCells.push_back(_at);
		result = true;
	}
	if (_radius > 0)
	{
		for_range(int, oy, -_radius, _radius)
		{
			for_range(int, ox, -_radius, _radius)
			{
				VectorInt2 o(ox, oy);
				if (!o.is_zero())
				{
					VectorInt2 at = _at + o;
					int dist = TypeConversions::Normal::f_i_closest(o.to_vector2().length());
					if (dist <= _radius)
					{
						if (is_cell_address_valid(at) &&
							!knownDevicesCells.does_contain(at))
						{
							knownDevicesCells.push_back(at);
							result = true;
						}
					}
				}
			}
		}
	}
	if (result)
	{
		issue_make_sure_known_cells_are_ready();
	}
	return result;
}

bool PilgrimageInstanceOpenWorld::mark_cell_known_special_devices(VectorInt2 const& _at, int _radius, int _amount, Name const & _pdWithTag, OPTIONAL_ OUT_ Array<VectorInt2>* _markedAt)
{
	if (_amount <= 0)
	{
		return false;
	}

	bool result = false;
	struct SDCell
	{
		VectorInt2 at;
		float dist;
	};
	ARRAY_STACK(SDCell, sdCells, _amount + 1); // to contain the one we're in, just in any case
	int amountMax = _amount;
	if (_radius >= 0)
	{
		for_range(int, oy, -_radius, _radius)
		{
			for_range(int, ox, -_radius, _radius)
			{
				VectorInt2 o(ox, oy);
				{
					VectorInt2 at = _at + o;
					float dist = o.to_vector2().length();
					int distI = TypeConversions::Normal::f_i_closest(dist);
					if (distI <= _radius)
					{
						if (is_cell_address_valid(at))
						{
							bool known = false;
							{
								/*
								if (!known)
								{
									Concurrency::ScopedSpinLock lock(visitedCellsLock);
									known |= visitedCells.does_contain(at);
								}
								if (!known)
								{
									Concurrency::ScopedSpinLock lock(knownDevicesCellsLock);
									known |= knownDevicesCells.does_contain(at);
								}
								*/
								if (!known)
								{
									Concurrency::ScopedSpinLock lock(knownSpecialDevicesCellsLock, true);
									known |= knownSpecialDevicesCells.does_contain(at);
								}
							}
							if (!known)
							{
								if (does_cell_have_special_device(at, _pdWithTag))
								{
									if (_at == at)
									{
										amountMax = _amount + 1;
									}
									SDCell newCell;
									newCell.at = at;
									newCell.dist = dist;
									bool added = false;
									for_every(sdc, sdCells)
									{
										if (newCell.dist < sdc->dist)
										{
											int atIdx = for_everys_index(sdc);
											if (sdCells.get_size() >= amountMax)
											{
												sdCells.pop_back();
											}
											sdCells.insert_at(atIdx, newCell);
											added = true;
											break;
										}
									}
									if (!added && sdCells.get_size() < amountMax)
									{
										sdCells.push_back(newCell);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	for_every(sdc, sdCells)
	{
		{
			Concurrency::ScopedSpinLock lock(knownSpecialDevicesCellsLock, true);
			knownSpecialDevicesCells.push_back_unique(sdc->at);
		}
		if (sdc->at != _at)
		{
			if (_markedAt)
			{
				_markedAt->push_back(sdc->at);
			}
			result = true;
		}
	}
	if (result)
	{
		issue_make_sure_known_cells_are_ready();
	}
	return result;
}

void PilgrimageInstanceOpenWorld::add_long_distance_detection(VectorInt2 const& _at, int _radius, int _amount)
{
	if (_amount <= 0)
	{
		return;
	}

	struct SDCell
	{
		VectorInt2 at;
		float dist;
		Optional<Vector2> directionTowards;
	};
	ARRAY_STACK(SDCell, sdCells, _amount + 1); // to contain the one we're in, just in any case
	int amountMax = _amount;
	if (_radius >= 0)
	{
		Vector2 trueLoc = _at.to_vector2() + calculate_rel_centre_for(_at); // always in centre

		for_range(int, oy, -_radius, _radius)
		{
			for_range(int, ox, -_radius, _radius)
			{
				VectorInt2 o(ox, oy);
				if (! o.is_zero()) // ignore here
				{
					VectorInt2 at = _at + o;
					float dist = o.to_vector2().length();
					int distI = TypeConversions::Normal::f_i_closest(dist);
					if (distI <= _radius)
					{
						if (is_cell_address_valid(at))
						{
							bool detectableByDirection = false;
							if (does_cell_have_long_distance_detectable_device(at, OUT_ &detectableByDirection))
							{
								Vector2 trueOLoc = at.to_vector2() + calculate_rel_centre_for(at) + calculate_random_small_offset_for(at, 0.06f); // let it be a bit off but the same for all
								float trueDist = Vector2::distance(trueLoc, trueOLoc);
								SDCell newCell;
								newCell.at = at;
								newCell.dist = trueDist;
								if (detectableByDirection)
								{
									newCell.directionTowards = trueOLoc;
								}
								bool added = false;
								for_every(sdc, sdCells)
								{
									if (newCell.dist < sdc->dist)
									{
										int atIdx = for_everys_index(sdc);
										if (sdCells.get_size() >= amountMax)
										{
											sdCells.pop_back();
										}
										sdCells.insert_at(atIdx, newCell);
										added = true;
										break;
									}
								}
								if (!added && sdCells.get_size() < amountMax)
								{
									sdCells.push_back(newCell);
								}
							}
						}
					}
				}
			}
		}
	}

	{
		Concurrency::ScopedSpinLock lock(longDistanceDetection.lock, true);
		auto& ldde = longDistanceDetection.get_for(_at);
		ldde.clear();
		for_every(sd, sdCells)
		{
			ldde.add(sd->dist, sd->at, sd->directionTowards);
		}
	}
}

bool PilgrimageInstanceOpenWorld::mark_cell_known(VectorInt2 const& _at, int _radius)
{
	bool result = false;
	Concurrency::ScopedSpinLock lock(knownCellsLock);
	if (is_cell_address_valid(_at) &&
		!knownCells.does_contain(_at))
	{
		knownCells.push_back(_at);
		result = true;
	}
	if (_radius > 0)
	{
		for_range(int, oy, -_radius, _radius)
		{
			for_range(int, ox, -_radius, _radius)
			{
				VectorInt2 o(ox, oy);
				if (!o.is_zero())
				{
					VectorInt2 at = _at + o;
					int dist = TypeConversions::Normal::f_i_closest(o.to_vector2().length());
					if (dist <= _radius)
					{
						if (is_cell_address_valid(at) &&
							!knownCells.does_contain(at))
						{
							knownCells.push_back(at);
							result = true;
						}
					}
				}
			}
		}
	}
	if (result)
	{
		issue_make_sure_known_cells_are_ready();
	}
	return result;
}

void PilgrimageInstanceOpenWorld::mark_all_cells_known()
{
	auto* owDef = pilgrimage->open_world__get_definition();
	if (!owDef->has_no_cells())
	{
		RangeInt2 size = owDef->get_size();
		if (!size.x.is_empty() &&
			!size.y.is_empty())
		{
			Concurrency::ScopedSpinLock lock(knownCellsLock);
			for_range(int, y, size.y.min, size.y.max)
			{
				for_range(int, x, size.x.min, size.x.max)
				{
					VectorInt2 at(x, y);
					if (is_cell_address_valid(at) &&
						!knownCells.does_contain(at))
					{
						knownCells.push_back(at);
					}
				}
			}
		}
		else
		{
			error(TXT("mark_all_cells_known won't work for map with infinite dimension"));
		}
	}
	else
	{
		error(TXT("mark_all_cells_known won't work for map without cells"));
	}
}

void PilgrimageInstanceOpenWorld::make_sure_cells_are_ready(VectorInt2 const& _at, int _radius)
{
	if (auto* g = Game::get())
	{
		RefCountObjectPtr<PilgrimageInstance> keepThis(this);
		g->add_async_world_job(TXT("make_sure_cells_are_ready"), [this, keepThis, _at, _radius]() { async_make_sure_cells_are_ready(_at, _radius); });
	}
}

void PilgrimageInstanceOpenWorld::issue_make_sure_known_cells_are_ready()
{
	if (readiedCells.active.get())
	{
		return;
	}
	
	if (auto* g = Game::get())
	{
		if (! g->does_want_to_cancel_creation())
		{
			readiedCells.active.increase();
			RefCountObjectPtr<PilgrimageInstance> keepThis(this);
			g->add_async_world_job(TXT("async_make_sure_known_cells_are_ready"), [this, keepThis]() { async_make_sure_known_cells_are_ready(); });
		}
	}
}

void PilgrimageInstanceOpenWorld::async_make_sure_known_cells_are_ready()
{
	ASSERT_ASYNC;

	scoped_call_stack_info_str_printf(TXT("make sure known cells are ready "));

	if (auto* g = Game::get())
	{
		if (g->does_want_to_cancel_creation())
		{
			return;
		}
	}

	struct MakeSure
	{
		static bool perform(PilgrimageInstanceOpenWorld* piow, REF_ int& idx, REF_ Array<VectorInt2>& cells, REF_ Concurrency::SpinLock& cellsLock)
		{
			while (true)
			{
				VectorInt2 at;
				{
					Concurrency::ScopedSpinLock lock(cellsLock);
					if (cells.is_index_valid(idx))
					{
						at = cells[idx];
					}
					else
					{
						return false;
					}
				}
				piow->get_cell_at(at);
				++idx;
				return true;
			}
		}
	};

	if (MakeSure::perform(this, REF_ readiedCells.knownCells, REF_ knownCells, REF_ knownCellsLock) ||
		MakeSure::perform(this, REF_ readiedCells.knownExitsCells, REF_ knownExitsCells, REF_ knownExitsCellsLock) ||
		MakeSure::perform(this, REF_ readiedCells.knownDevicesCells, REF_ knownDevicesCells, REF_ knownDevicesCellsLock) ||
		MakeSure::perform(this, REF_ readiedCells.knownSpecialDevicesCells, REF_ knownSpecialDevicesCells, REF_ knownSpecialDevicesCellsLock))
	{
		if (auto* g = Game::get())
		{
			RefCountObjectPtr<PilgrimageInstance> keepThis(this);
			g->add_async_world_job(TXT("async_make_sure_known_cells_are_ready"), [this, keepThis]() { async_make_sure_known_cells_are_ready(); });
		}
	}

	// all done
	readiedCells.active = 0;
}

void PilgrimageInstanceOpenWorld::async_make_sure_cells_are_ready(VectorInt2 const& _at, int _radius)
{
	ASSERT_ASYNC;

	scoped_call_stack_info_str_printf(TXT("make sure cells are ready %ix%i in radius %i"), _at.x, _at.y, _radius);

	if (is_cell_address_valid(_at))
	{
		get_cell_at(_at);
	}
	if (_radius > 0)
	{
		for_range(int, oy, -_radius, _radius)
		{
			for_range(int, ox, -_radius, _radius)
			{
				VectorInt2 o(ox, oy);
				if (!o.is_zero())
				{
					scoped_call_stack_info_str_printf(TXT("offset %ix%i"), ox, oy);
					VectorInt2 at = _at + o;
					int dist = TypeConversions::Normal::f_i_closest(o.to_vector2().length());
					if (dist <= _radius)
					{
						if (is_cell_address_valid(at))
						{
							get_cell_at(at);
						}
					}
				}
				if (game->does_want_to_cancel_creation())
				{
					return;
				}
			}
		}
	}
}

//

void PilgrimageInstanceOpenWorld::CellSetFunctionality::fill_local_dependents(OUT_ int* _localDependents, OUT_ int & _dependentsRadius, Random::Generator const& _rg) const
{
	if (auto* ls = get_as_any())
	{
		Random::Generator rg = _rg;
		rg.advance_seed(2358, 9762);
		_localDependents[DirFourClockwise::Up] = ls->get_custom_parameters().get_value<int>(NAME(maxDependentUp), 0);
		_localDependents[DirFourClockwise::Right] = ls->get_custom_parameters().get_value<int>(NAME(maxDependentRight), 0);
		_localDependents[DirFourClockwise::Down] = ls->get_custom_parameters().get_value<int>(NAME(maxDependentDown), 0);
		_localDependents[DirFourClockwise::Left] = ls->get_custom_parameters().get_value<int>(NAME(maxDependentLeft), 0);
		for_count(int, d, DirFourClockwise::NUM)
		{
			_localDependents[d] = _localDependents[d] > 0 ? rg.get_int_from_range(1, _localDependents[d] + 1) : 0;
		}
		_dependentsRadius = ls->get_custom_parameters().get_value<int>(NAME(maxDependentRadius), 0);
	}
	else
	{
		_localDependents[DirFourClockwise::Up] = 0;
		_localDependents[DirFourClockwise::Right] = 0;
		_localDependents[DirFourClockwise::Down] = 0;
		_localDependents[DirFourClockwise::Left] = 0;
		_dependentsRadius = 0;
	}
}

int PilgrimageInstanceOpenWorld::CellSetFunctionality::get_cell_priority() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_value<int>(NAME(cellPriority), 0);
	}
	else
	{
		return NONE;
	}
}

float PilgrimageInstanceOpenWorld::CellSetFunctionality::get_cell_distance_visibility_coef() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_value<float>(NAME(cellDistanceVisibilityCoef), 1.0);
	}
	else
	{
		return NONE;
	}
}

bool PilgrimageInstanceOpenWorld::CellSetFunctionality::should_avoid_same_neighbours() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_existing<bool>(NAME(avoidSameNeighbours));
	}
	else
	{
		return false;
	}
}

Tags const* PilgrimageInstanceOpenWorld::CellSetFunctionality::get_cell_type() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_existing<Tags>(NAME(cellType));
	}
	else
	{
		return nullptr;
	}
}

bool PilgrimageInstanceOpenWorld::CellSetFunctionality::should_dir_align_dependent() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_value<bool>(NAME(dirAlignDependent), false);
	}
	else
	{
		return false;
	}
}

bool PilgrimageInstanceOpenWorld::CellSetFunctionality::should_axis_align_dependent() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_value<bool>(NAME(axisAlignDependent), false);
	}
	else
	{
		return false;
	}
}

bool PilgrimageInstanceOpenWorld::CellSetFunctionality::should_align_dependent_towards() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_value<bool>(NAME(alignDependentTowards), false);
	}
	else
	{
		return false;
	}
}

bool PilgrimageInstanceOpenWorld::CellSetFunctionality::should_align_dependent_away() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_value<bool>(NAME(alignDependentAway), false);
	}
	else
	{
		return false;
	}
}

Range2 PilgrimageInstanceOpenWorld::CellSetFunctionality::get_local_centre_for_map() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_value<Range2>(NAME(forMapCentreLocal), Range2::zero);
	}
	else
	{
		return Range2::zero;
	}
}

void PilgrimageInstanceOpenWorld::CellSetFunctionality::get_local_side_exits_info_for_map(bool* _sideSensitiveExits, Range & _sideExitsOffset) const
{
	_sideSensitiveExits[DirFourClockwise::Up] = false;
	_sideSensitiveExits[DirFourClockwise::Right] = false;
	_sideSensitiveExits[DirFourClockwise::Down] = false;
	_sideSensitiveExits[DirFourClockwise::Left] = false;
	_sideExitsOffset = Range(0.45f); // close
	if (auto* ls = get_as_any())
	{
		_sideSensitiveExits[DirFourClockwise::Up] = ls->get_custom_parameters().get_value<bool>(NAME(forMapSideSensitiveUpExit), _sideSensitiveExits[DirFourClockwise::Up]);
		_sideSensitiveExits[DirFourClockwise::Right] = ls->get_custom_parameters().get_value<bool>(NAME(forMapSideSensitiveRightExit), _sideSensitiveExits[DirFourClockwise::Right]);
		_sideSensitiveExits[DirFourClockwise::Down] = ls->get_custom_parameters().get_value<bool>(NAME(forMapSideSensitiveDownExit), _sideSensitiveExits[DirFourClockwise::Down]);
		_sideSensitiveExits[DirFourClockwise::Left] = ls->get_custom_parameters().get_value<bool>(NAME(forMapSideSensitiveLeftExit), _sideSensitiveExits[DirFourClockwise::Left]);
		_sideExitsOffset = ls->get_custom_parameters().get_value<Range>(NAME(forMapSideExitsOffset), _sideExitsOffset);
	}
}

Framework::LibraryBasedParameters const* PilgrimageInstanceOpenWorld::CellSetFunctionality::choose_additional_cell_info(Random::Generator const& _seed) const
{
	if (auto* ls = get_as_any())
	{
		if (auto* tc = ls->get_custom_parameters().get_existing<TagCondition>(NAME(additionalCellInfoTagged)))
		{
			struct Found
			{
				Framework::LibraryBasedParameters const* aci;
				float probCoef;
			};
			Array<Found> found;
			Framework::Library::get_current()->get_library_based_parameters().do_for_every([tc, &found](Framework::LibraryStored const* _ls)
				{
					if (tc->check(_ls->get_tags()))
					{
						auto* aci = fast_cast<Framework::LibraryBasedParameters>(_ls);
						an_assert(aci);
						found.grow_size(1);
						auto& f = found.get_last();
						f.aci = aci;
						f.probCoef = Framework::LibraryStored::get_prob_coef_from_tags<Framework::LibraryBasedParameters>(f.aci);
					}
				});
			if (!found.is_empty())
			{
				Random::Generator rg = _seed;
				rg.advance_seed(9534, 2075);
				int idx = RandomUtils::ChooseFromContainer<Array<Found>, Found>::choose(rg, found, [](Found const& a) { return a.probCoef; });
				return found[idx].aci;
			}
			error(TXT("no additional cell infos found, although requested"));
		}
	}
	return nullptr;
}

Optional<Colour> PilgrimageInstanceOpenWorld::CellSetFunctionality::get_map_colour_for_map() const
{
	if (auto* ls = get_as_any())
	{
		if (auto* c = ls->get_custom_parameters().get_existing<Colour>(NAME(forMapColour)))
		{
			return *c;
		}
	}
	return NP;
}

LineModel const * PilgrimageInstanceOpenWorld::CellSetFunctionality::get_line_model_for_map() const
{
	if (auto* ls = get_as_any())
	{
		Framework::LibraryName lineModelName = ls->get_custom_parameters().get_value<Framework::LibraryName>(NAME(forMapLineModel), Framework::LibraryName::invalid());
		if (lineModelName.is_valid())
		{
			return Library::get_current_as<Library>()->get_line_models().find(lineModelName);
		}
	}
	return nullptr;
}

LineModel const * PilgrimageInstanceOpenWorld::CellSetFunctionality::get_line_model_for_map_additional(Framework::LibraryStored const* _additionalInfo) const
{
	if (_additionalInfo)
	{
		Framework::LibraryName lineModelName = _additionalInfo->get_custom_parameters().get_value<Framework::LibraryName>(NAME(forMapLineModelAdditional), Framework::LibraryName::invalid());
		if (lineModelName.is_valid())
		{
			return Library::get_current_as<Library>()->get_line_models().find(lineModelName);
		}
	}
	return nullptr;
}

LineModel const * PilgrimageInstanceOpenWorld::CellSetFunctionality::get_line_model_always_otherwise_for_map() const
{
	if (auto* ls = get_as_any())
	{
		Framework::LibraryName lineModelName = ls->get_custom_parameters().get_value<Framework::LibraryName>(NAME(forMapLineModelAlwaysOtherwise), Framework::LibraryName::invalid());
		if (lineModelName.is_valid())
		{
			return Library::get_current_as<Library>()->get_line_models().find(lineModelName);
		}
	}
	return nullptr;
}

TagCondition const* PilgrimageInstanceOpenWorld::CellSetFunctionality::get_depends_on_cell_type() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_existing<TagCondition>(NAME(dependsOnCellType));
	}
	else
	{
		return nullptr;
	}
}

TagCondition const* PilgrimageInstanceOpenWorld::CellSetFunctionality::get_dependent_cell_types() const
{
	if (auto* ls = get_as_any())
	{
		return ls->get_custom_parameters().get_existing<TagCondition>(NAME(dependentCellTypes));
	}
	else
	{
		return nullptr;
	}
}

bool PilgrimageInstanceOpenWorld::is_pilgrimage_device_direction_known(PilgrimageDevice* _pd) const
{
	if (!GameSettings::get().difficulty.obfuscateSymbols)
	{
		return true;
	}
#ifdef DONT_OBFUSCATE_MAP_SYMBOLS
	return true;
#endif
	if (!obfuscatedPilgrimageDevicesAvailable)
	{
		return false;
	}
	for_every(pdlm, obfuscatedPilgrimageDevices)
	{
		if (pdlm->pilgrimageDevice.get() == _pd)
		{
			return pdlm->known;
		}
	}
	return true;
}

void PilgrimageInstanceOpenWorld::clear_pilgrimage_device_directions_known_on_hold()
{
	for_every(pdlm, obfuscatedPilgrimageDevices)
	{
		pdlm->knownOnHold = false;
	}
}

void PilgrimageInstanceOpenWorld::update_pilgrimage_device_directions_known_on_hold()
{
	struct ShowSymbol
	{
		Framework::UsedLibraryStored<Framework::TexturePart> from;
		Framework::UsedLibraryStored<Framework::TexturePart> to;

		ShowSymbol() {}
		ShowSymbol(Framework::TexturePart* _from, Framework::TexturePart* _to) : from(_from), to(_to) {}
	};
	ARRAY_STATIC(ShowSymbol, showSymbols, 16);
	for_every(pdlm, obfuscatedPilgrimageDevices)
	{
		if (pdlm->knownOnHold && ! pdlm->known)
		{
			pdlm->known = true;
			if (showSymbols.has_place_left())
			{
				showSymbols.push_back(ShowSymbol(pdlm->obfuscatedDirectionTexturePart.get(), pdlm->pilgrimageDevice->get_direction_texture_part()));
			}
		}
		pdlm->knownOnHold = false;
	}

	todo_multiplayer_issue(TXT("we just get player here"));
	if (auto* g = Game::get_as<Game>())
	{
		if (auto* pa = g->access_player().get_actor())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				auto& poi = mp->access_overlay_info();
				if (poi.is_trying_to_show_map())
				{
					poi.new_map_available(); // to update the symbol
				}
				for_every(ss, showSymbols)
				{
					if (GameSettings::get().difficulty.obfuscateSymbols)
					{
						poi.show_symbol_replacement(ss->from.get(), ss->to.get());
					}
				}
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::mark_all_pilgrimage_device_directions_known()
{
	if (auto* gd = GameDirector::get())
	{
		if (gd->is_pilgrimage_devices_unobfustication_on_hold())
		{
			for_every(pdlm, obfuscatedPilgrimageDevices)
			{
				pdlm->knownOnHold = true;
			}
			return;
		}
	}
	for_every(pdlm, obfuscatedPilgrimageDevices)
	{
		pdlm->known = true;
	}
}

void PilgrimageInstanceOpenWorld::mark_pilgrimage_device_direction_known(PilgrimageDevice const* _pd)
{
	if (auto* gd = GameDirector::get())
	{
		if (gd->is_pilgrimage_devices_unobfustication_on_hold())
		{
			for_every(pdlm, obfuscatedPilgrimageDevices)
			{
				if (pdlm->pilgrimageDevice.get() == _pd)
				{
					pdlm->knownOnHold = true;
					return;
				}
			}
			return;
		}
	}
	for_every(pdlm, obfuscatedPilgrimageDevices)
	{
		if (pdlm->pilgrimageDevice.get() == _pd)
		{
			if (!pdlm->known)
			{
				pdlm->known = true;
				todo_multiplayer_issue(TXT("we just get player here"));
				if (auto* g = Game::get_as<Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
						{
							auto& poi = mp->access_overlay_info();
							if (poi.is_trying_to_show_map())
							{
								poi.new_map_available(); // to update the symbol
							}
							if (GameSettings::get().difficulty.obfuscateSymbols)
							{
								poi.show_symbol_replacement(pdlm->obfuscatedDirectionTexturePart.get(), pdlm->pilgrimageDevice->get_direction_texture_part());
							}
						}
					}
				}
			}
			return;
		}
	}
}

void PilgrimageInstanceOpenWorld::mark_pilgrimage_device_direction_known(Framework::IModulesOwner* _object)
{
	if (startingRoomUpgradeMachine.get() && startingRoomUpgradeMachine == _object &&
		startingRoomUpgradeMachinePilgrimageDevice.get())
	{
		mark_pilgrimage_device_direction_known(startingRoomUpgradeMachinePilgrimageDevice.get());
		return;
	}

	auto at = find_cell_at(_object);
	if (!at.is_set())
	{
		return;
	}
	auto _at = at.get();
	ExistingCell* ec = nullptr;
	Cell* c = nullptr;
	if (get_cells(_at, c, ec))
	{
		for_every(pd, ec->pilgrimageDevices)
		{
			bool thisOne = pd->object == _object;
			if (!thisOne)
			{
				if (auto* mmpd = pd->object->get_gameplay_as<ModuleMultiplePilgrimageDevice>()) // this is a gameplay module!
				{
					if (mmpd->is_one_of_pilgrimage_devices(_object))
					{
						thisOne = true;
					}
				}
			}
			if (thisOne)
			{
#ifdef AN_OUTPUT_PILGRIMAGE_DEVICES
				output(TXT("[PD] mark pilgrimage device direction known %ix%i \"%S\" (object:\"%S\")"), _at.x, _at.y, pd->device.get_name().to_string().to_char(), _object->ai_get_name().to_char());
#endif
				mark_pilgrimage_device_direction_known(pd->device.get());
				return;
			}
		}
	}
}
Framework::TexturePart* PilgrimageInstanceOpenWorld::get_pilgrimage_device_direction_texture_part(PilgrimageDevice* _pd, KnownPilgrimageDevice const* kpd) const
{
	if (!_pd->should_show_door_directions())
	{
		return nullptr;
	}
	if (_pd->should_show_door_directions_only_if_depleted() && (! kpd || ! kpd->depleted))
	{
		return nullptr;
	}
	if (_pd->should_show_door_directions_only_if_not_depleted() && (! kpd || kpd->depleted))
	{
		return nullptr;
	}
	if (_pd->should_show_door_directions_only_if_visited() && (! kpd || ! kpd->visited))
	{
		return nullptr;
	}
	if (!GameSettings::get().difficulty.obfuscateSymbols)
	{
		return _pd->get_direction_texture_part();
	}
#ifdef DONT_OBFUSCATE_MAP_SYMBOLS
	return _pd->get_direction_texture_part();
#endif
	if (!obfuscatedPilgrimageDevicesAvailable)
	{
		return nullptr;
	}
	for_every(pdlm, obfuscatedPilgrimageDevices)
	{
		if (pdlm->pilgrimageDevice.get() == _pd)
		{
			return !pdlm->known ? pdlm->obfuscatedDirectionTexturePart.get() : _pd->get_direction_texture_part();
		}
	}
	return _pd->get_direction_texture_part();
}

LineModel* PilgrimageInstanceOpenWorld::get_pilgrimage_device_line_model_for_map(PilgrimageDevice* _pd, KnownPilgrimageDevice const* kpd) const
{
	if (!GameSettings::get().difficulty.obfuscateSymbols)
	{
		return _pd->get_for_map_line_model();
	}
#ifdef DONT_OBFUSCATE_MAP_SYMBOLS
	return _pd->get_for_map_line_model();
#endif
	if (!obfuscatedPilgrimageDevicesAvailable)
	{
		return nullptr;
	}
	for_every(pdlm, obfuscatedPilgrimageDevices)
	{
		if (pdlm->pilgrimageDevice.get() == _pd)
		{
			return !pdlm->known ? pdlm->obfuscatedLineModelForMap.get() : _pd->get_for_map_line_model();
		}
	}
	return _pd->get_for_map_line_model();
}

Colour obfuscated_pilgrimage_device_colour()
{
	return Colour::lerp(0.3f, Colour::orange, Colour::white);
}

void PilgrimageInstanceOpenWorld::create_obfuscated_pilgrimage_device_line_model_for_map(PilgrimageDevice* _pd)
{
	if (!_pd->should_show_door_directions() &&
		!_pd->may_appear_on_map())
	{
		return;
	}

	if (_pd->is_known_from_start() ||
		_pd->is_unobfuscated_from_start())
	{
		// no need to obfuscate it
		return;
	}

	an_assert(!obfuscatedPilgrimageDevicesAvailable);

	ObfuscatedPilgrimageDevice* pdlm = nullptr;
	for_every(p, obfuscatedPilgrimageDevices)
	{
		if (p->pilgrimageDevice.get() == _pd)
		{
			pdlm = p;
			break;
		}
	}
	if (!pdlm)
	{
		ObfuscatedPilgrimageDevice p;
		p.pilgrimageDevice = _pd;
		obfuscatedPilgrimageDevices.push_back(p);
		pdlm = &obfuscatedPilgrimageDevices.get_last();
	}

	Framework::UsedLibraryStored<LineModel> lm;
	lm = Library::get_current_as<Library>()->get_line_models().create_temporary();

	String pdName = _pd->get_name().to_string();
	Random::Generator rg = randomSeed;
	for_every(ch, pdName.get_data())
	{
		rg.advance_seed(for_everys_index(ch), *ch);
	}

	Colour colour = obfuscated_pilgrimage_device_colour();

	while (lm->get_lines().get_size() <= 4) // 2 is most likely a single line, we want something more than just a line
	{
		lm->clear();
		Vector3 p = Vector3::zero;
		for_count(int, i, rg.get_int_from_range(4, 20))
		{
			Vector3 np = p;
			{
				float angle = (float)(rg.get_int(8)) * 360.0f / 8.0f;
				Vector3 dir = Rotator3(0.0f, angle, 0.0f).get_forward();
				float l = (float)rg.get_int_from_range(1, 3) * 0.1f;
				if (abs(dir.x * dir.y) < 0.1f)
				{
					dir *= l;
				}
				else
				{
					dir *= sqrt(2.0f) * l;
				}

				if (rg.get_chance(0.7f) && dir.x * p.x > 0.0f && abs(p.x) > 0.25f)
				{
					dir.x = -dir.x;
				}
				if (rg.get_chance(0.7f) && dir.y * p.y > 0.0f && abs(p.y) > 0.25f)
				{
					dir.y = -dir.y;
				}

				np = p + dir;

				if (np.x < -0.5f) { np.y += -0.5f - np.x; np.x = -0.5f; }
				if (np.x >  0.5f) { np.y +=  0.5f - np.x; np.x =  0.5f; }
				if (np.y < -0.5f) { np.x += -0.5f - np.y; np.y = -0.5f; }
				if (np.y >  0.5f) { np.x +=  0.5f - np.y; np.y =  0.5f; }

				if (np != p)
				{
					lm->add_line(LineModel::Line(p, np, colour));
				}
			}

			p = np;
		}
		{
			int mirrorCount = rg.get_int_from_range(-1, 3);
			while (mirrorCount > 0)
			{
				--mirrorCount;
				float angle = (float)(rg.get_int(12)) * 360.0f / 12.0f;
				lm->mirror(Plane(Rotator3(0.0f, angle, 0.0f).get_forward(), Vector3::zero));
			}
			lm->mirror(Plane(Vector3::xAxis, Vector3::zero));
		}
	}

	lm->fit_xy(Range2(Range(-0.5f, 0.5f), Range(-0.5f, 0.5f)));

	pdlm->obfuscatedLineModelForMap = lm;
}

//

struct PilgrimageInstanceOpenWorld::InitObfuscatedDirectionsRT
: public Concurrency::AsynchronousJobData
{
	RefCountObjectPtr<PilgrimageInstance> forPilgrimage;

	InitObfuscatedDirectionsRT(PilgrimageInstanceOpenWorld* _piow)
	{
		forPilgrimage = _piow;
	}

	static void perform(Concurrency::JobExecutionContext const* _executionContext, void* _data)
	{
		assert_rendering_thread();
		InitObfuscatedDirectionsRT* data = (InitObfuscatedDirectionsRT*)_data;

		if (auto* piow = fast_cast<PilgrimageInstanceOpenWorld>(data->forPilgrimage.get()))
		{
			int sizeOneSide = 32;
			int buffer = 2;
			VectorInt2 sizeOne(sizeOneSide + buffer * 2, sizeOneSide + buffer * 2);
			Vector2 centreOffset = (sizeOne / 2).to_vector2() + Vector2::half;
			Vector2 scale = Vector2::one * ((float)TypeConversions::Normal::f_i_cells((float)sizeOneSide / 10.0f + 0.0001f) / 0.1f); // hardcoded, assumed we have steps 10 segments of 0.1 each

			VectorInt2 size = VectorInt2::one * 2;
			while (size.x < sizeOne.x * 4 && size.y < sizeOne.y * 4)
			{
				size *= 2;
			}
			Vector2 sizeVector = size.to_vector2();
			::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(size);
			DEFINE_STATIC_NAME(colour);
			// we use ordinary rgb 8bit textures
			rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
				::System::VideoFormat::rgba8,
				::System::BaseVideoFormat::rgba));
			RENDER_TARGET_CREATE_INFO(TXT("obfuscated directions rt")); 
			piow->obfuscatedDirectionsRT = new ::System::RenderTarget(rtSetup);

			::System::Video3D* v3d = ::System::Video3D::get();

			piow->obfuscatedDirectionsRT->bind();

			v3d->set_viewport(VectorInt2::zero, rtSetup.get_size());
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho(rtSetup.get_size().to_vector2());

			v3d->setup_for_2d_display();

			v3d->access_model_view_matrix_stack().clear();
			v3d->access_clip_plane_stack().clear();

			Colour useColour = obfuscated_pilgrimage_device_colour();
			v3d->clear_colour(Colour::black.with_alpha(0.0f));

			VectorInt2 at = VectorInt2::zero;
			for_every(p, piow->obfuscatedPilgrimageDevices)
			{
				if (auto* lm = p->obfuscatedLineModelForMap.get())
				{
					Vector2 offset = at.to_vector2() + centreOffset;
					// shillouette
					for_count(int, i, 4)
					{
						Vector2 off = Vector2(0.0f, 1.0f);
						for_count(int, j, i)
						{
							off.rotate_right();
						}
						for_every(line, lm->get_lines())
						{
							::System::Video3DPrimitives::line_2d(Colour::black, line->a.to_vector2() * scale + offset + off, line->b.to_vector2() * scale + offset + off);
						}
					}
					for_every(line, lm->get_lines())
					{
						::System::Video3DPrimitives::line_2d(line->colour.get(useColour), line->a.to_vector2() * scale + offset, line->b.to_vector2() * scale + offset);
					}
				}

				{
					Vector2 uv0 = at.to_vector2() + Vector2::half;
					Vector2 uv1 = (at + sizeOne - VectorInt2::one).to_vector2() + Vector2::half;
					Vector2 uvAnchor = at.to_vector2() + centreOffset;
					uv0 /= sizeVector;
					uv1 /= sizeVector;
					uvAnchor /= sizeVector;
					p->obfuscatedDirectionTexturePart = Library::get_current()->get_texture_parts().create_temporary();
					p->obfuscatedDirectionTexturePart->setup(piow->obfuscatedDirectionsRT.get(), 0, uv0, uv1, uvAnchor);
				}

				{
					at.x += sizeOne.x;
					if (at.x + sizeOne.x - 1 >= size.x)
					{
						at.x = 0;
						at.y += sizeOne.y;
					}
				}
			}

			piow->obfuscatedDirectionsRT->unbind();
			piow->obfuscatedDirectionsRT->resolve();

			piow->obfuscatedPilgrimageDevicesAvailable = true;
		}
	}
};

void PilgrimageInstanceOpenWorld::create_obfuscated_pilgrimage_device_directions()
{
	an_assert(!obfuscatedPilgrimageDevicesAvailable);

	Game::get()->async_system_job(Game::get(), PilgrimageInstanceOpenWorld::InitObfuscatedDirectionsRT::perform, new PilgrimageInstanceOpenWorld::InitObfuscatedDirectionsRT(this));
}

void PilgrimageInstanceOpenWorld::async_get_airship_obstacles(VectorInt2 const& _at, float _radius, OUT_ Array<AirshipObstacle>& _obstacles)
{
	ASSERT_ASYNC;

	float cellSize = get_cell_size();

	{
		Concurrency::ScopedMRSWLockRead lock(cellsLock, TXT("get airship obstacles"));

		for_every(c, cells)
		{
			float distance = ((c->at - _at).to_vector2() * cellSize).length();
			if (distance <= _radius)
			{
				if (auto* ls = c->set.get_as_any())
				{
					float aoh = ls->get_custom_parameters().get_value<float>(NAME(airshipObstacleHeight), 0.0f);
					if (aoh > 0.0f)
					{
						AirshipObstacle ao;
						ao.at = c->at - _at;
						ao.location = ao.at.to_vector2().to_vector3() * cellSize;
						ao.height = aoh;
						_obstacles.push_back(ao);
					}
				}
			}
		}
	}
}

void PilgrimageInstanceOpenWorld::async_update_no_door_markers(VectorInt2 const& _cellAt)
{
	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

	if (game->does_want_to_cancel_creation())
	{
		return;
	}

	Framework::SceneryType* noDoorMarkerType = pilgrimage->open_world__get_definition()->get_no_door_marker_scenery_type();
	if (auto* b = pilgrimage->open_world__get_definition()->get_block_away_from_objective())
	{
		if (auto* st = b->noDoorMarkerSceneryType.get())
		{
			noDoorMarkerType = st;
		}
	}

	if (!noDoorMarkerType)
	{
		return;
	}

	// modify no-door markers for cell _cellAt and its neighbours
	ArrayStatic<VectorInt2, 5> cellAt;
	cellAt.push_back(_cellAt);
	cellAt.push_back(_cellAt + VectorInt2( 0,  1));
	cellAt.push_back(_cellAt + VectorInt2( 1,  0));
	cellAt.push_back(_cellAt + VectorInt2( 0, -1));
	cellAt.push_back(_cellAt + VectorInt2(-1,  0));

	bool newNoDoorMarkersPending = false;

	for_every(at, cellAt)
	{
		if (auto* ec = get_existing_cell(*at))
		{
			ArrayStatic<DirFourClockwise::Type, 4> dirs;
			if (*at == _cellAt)
			{
				for_count(int, d, DirFourClockwise::NUM)
				{
					dirs.push_back((DirFourClockwise::Type)d);
				}
			}
			else
			{
				auto dir = DirFourClockwise::vector_int2_to_dir(_cellAt - *at);
				if (dir.is_set())
				{
					dirs.push_back(dir.get());
				}
			}
			for_every(d, dirs)
			{
				VectorInt2 dirToCellAt = *at + DirFourClockwise::dir_to_vector_int2(*d);
				for_every(door, ec->doors)
				{
					if (door->toCellAt == dirToCellAt)
					{
						if (!door->noDoorMarker.get())
						{
							if (auto* actualDoor = door->door.get())
							{
								if (auto* doorInRoom = get_linked_door_in(actualDoor, *at))
								{

									Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();

									noDoorMarkerType->load_on_demand_if_required();

									Framework::Scenery* noDoorMarker;
									game->perform_sync_world_job(TXT("create no-door marker"), [this, &noDoorMarker, noDoorMarkerType, dirToCellAt]()
										{
											an_assert(noDoorMarkerType);
											noDoorMarker = new Framework::Scenery(noDoorMarkerType, String::printf(TXT("no-door marker to %ix%i"), dirToCellAt.x, dirToCellAt.y));
											noDoorMarker->init(subWorld);
										});

									{
										Random::Generator rg = randomSeed;
										rg.advance_seed(239782, 23978234);
										noDoorMarker->access_individual_random_generator() = rg;
									}

									noDoorMarker->initialise_modules();

									game->perform_sync_world_job(TXT("place no-door marker"), [&noDoorMarker, doorInRoom]()
										{
											Transform placement = doorInRoom->get_placement();
											placement.set_translation(doorInRoom->get_hole_centre_WS());
											noDoorMarker->get_presence()->place_in_room(doorInRoom->get_in_room(), placement);
										});

									Game::get()->on_newly_created_object(noDoorMarker);

									game->request_ready_and_activate_all_inactive(activationGroup.get());
									game->pop_activation_group(activationGroup.get());

									door->noDoorMarker = noDoorMarker;
								}
							}
						}
						if (auto* noDoorMarker = door->noDoorMarker.get())
						{
							// update shader program params for no-door markers
							bool connected = false;
							if (auto* actualDoor = door->door.get())
							{
								connected = actualDoor->get_linked_door_a() && actualDoor->get_linked_door_b();
							}
							bool pending = false;
							if (!connected)
							{
								pending = !door->cellWontBeCreated;
							}

							door->noDoorMarkerPending = pending;

							newNoDoorMarkersPending |= pending;

							// add sync world job as we wait for the markers to be activated
							game->add_sync_world_job(TXT("place no-door marker"), [noDoorMarker, connected, pending]()
								{
									for_every_ptr(a, noDoorMarker->get_appearances())
									{
										a->be_visible(! connected);
										a->set_shader_param(NAME(connectionPending), pending? 1.0f : 0.0f);

									}
									if (auto* o = noDoorMarker->get_as_object())
									{
										if (pending)
										{
											o->access_tags().set_tag(NAME(noDoorMarkerPending));
										}
										else
										{
											o->access_tags().remove_tag(NAME(noDoorMarkerPending));
										}
									}
								});
						}
					}
				}
			}
		}
	}

	if (!newNoDoorMarkersPending)
	{
		Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("no door markers active update"));

		for_every(ec, existingCells)
		{
			for_every(door, ec->doors)
			{
				if (auto* noDoorMarker = door->noDoorMarker.get())
				{
					newNoDoorMarkersPending |= door->noDoorMarkerPending;
				}
				if (newNoDoorMarkersPending)
				{
					break;
				}
			}
			if (newNoDoorMarkersPending)
			{
				break;
			}
		}
	}

	noDoorMarkersPending = newNoDoorMarkersPending;
}

Framework::Room* PilgrimageInstanceOpenWorld::get_any_transition_room() const
{
	Concurrency::ScopedMRSWLockRead lock(existingCellsLock, TXT("store pilgrim device states for all existing cells"));
	for_every(ec, existingCells)
	{
		if (auto* r = ec->transitionRoom.get())
		{
			return r;
		}
	}

	return nullptr;
}

//--

void PilgrimageInstanceOpenWorld::CellSet::set(Framework::RoomType* rt)
{
	regionType.clear();
	roomType = rt;
	if (rt) { rt->load_on_demand_if_required(); }
}

void PilgrimageInstanceOpenWorld::CellSet::set(Framework::RegionType* rt)
{
	regionType = rt;
	roomType.clear();
	if (rt) { rt->load_on_demand_if_required(); }
}

Framework::LibraryStored* PilgrimageInstanceOpenWorld::CellSet::get_as_any() const
{
	if (roomType.is_set())
	{
		return roomType.get();
	}
	else
	{
		return regionType.get();
	}
}

//--

PilgrimageInstanceOpenWorld::Cell::Cell()
{
	SET_EXTRA_DEBUG_INFO(knownPilgrimageDevices, TXT("PilgrimageInstanceOpenWorld.Cell.knownPilgrimageDevices"));
}

int PilgrimageInstanceOpenWorld::Cell::get_as_world_address_id() const
{
	return to_world_address_id(at);
}

void PilgrimageInstanceOpenWorld::Cell::init(VectorInt2 const& _at, PilgrimageInstanceOpenWorld const& _openWorld)
{
	at = _at;
	randomSeed = _openWorld.get_random_seed_at(_at);
}

void PilgrimageInstanceOpenWorld::Cell::get_dependents_world(OUT_ int* _dependents) const
{
	_dependents[DirFourClockwise::Up] = localDependents[DirFourClockwise::Up];
	_dependents[DirFourClockwise::Right] = localDependents[DirFourClockwise::Right];
	_dependents[DirFourClockwise::Down] = localDependents[DirFourClockwise::Down];
	_dependents[DirFourClockwise::Left] = localDependents[DirFourClockwise::Left];

	int rotate = dir;
	while (rotate > 0)
	{
		--rotate;
		int leftDependents = _dependents[DirFourClockwise::Left];
		_dependents[DirFourClockwise::Left] = _dependents[DirFourClockwise::Down];
		_dependents[DirFourClockwise::Down] = _dependents[DirFourClockwise::Right];
		_dependents[DirFourClockwise::Right] = _dependents[DirFourClockwise::Up];
		_dependents[DirFourClockwise::Up] = leftDependents;
	}
}

void PilgrimageInstanceOpenWorld::Cell::fill_dir_important_exits(OUT_ bool* _dirImportantExits, OUT_ bool& _dirImportant) const
{
	if (auto* ls = set.get_as_any())
	{
		_dirImportantExits[DirFourClockwise::Up] = ls->get_custom_parameters().get_value<bool>(NAME(dirImportantUpExit), false);
		_dirImportantExits[DirFourClockwise::Right] = ls->get_custom_parameters().get_value<bool>(NAME(dirImportantRightExit), false);
		_dirImportantExits[DirFourClockwise::Down] = ls->get_custom_parameters().get_value<bool>(NAME(dirImportantDownExit), false);
		_dirImportantExits[DirFourClockwise::Left] = ls->get_custom_parameters().get_value<bool>(NAME(dirImportantLeftExit), false);
		_dirImportant = ls->get_custom_parameters().get_value<bool>(NAME(dirImportant), false);
	}
	else
	{
		_dirImportantExits[DirFourClockwise::Up] = false;
		_dirImportantExits[DirFourClockwise::Right] = false;
		_dirImportantExits[DirFourClockwise::Down] = false;
		_dirImportantExits[DirFourClockwise::Left] = false;
		_dirImportant = false;
	}
}

void PilgrimageInstanceOpenWorld::Cell::fill_preferred_side_exits()
{
	{
		Random::Generator rg = randomSeed;
		rg.advance_seed(9763, 97239);
		sideExitsUpDown = rg.get_bool()? -1 : 1;
		sideExitsLeftRight = rg.get_bool() ? -1 : 1;
	}
	if (auto* ls = set.get_as_any())
	{
		if (ls->get_custom_parameters().get_value<bool>(NAME(sideExitsAtDown), false))
		{
			sideExitsUpDown = -1;
		}
		if (ls->get_custom_parameters().get_value<bool>(NAME(sideExitsAtUp), false))
		{
			sideExitsUpDown = 1;
		}
		if (ls->get_custom_parameters().get_value<bool>(NAME(sideExitsAtLeft), false))
		{
			sideExitsLeftRight = -1;
		}
		if (ls->get_custom_parameters().get_value<bool>(NAME(sideExitsAtRight), false))
		{
			sideExitsLeftRight = 1;
		}
	}

	int rotate = dir;
	while (rotate > 0)
	{
		--rotate;

		int seup = sideExitsUpDown;
		int selr = sideExitsLeftRight;
		sideExitsUpDown = -selr;
		sideExitsLeftRight = seup;
	}
}

void PilgrimageInstanceOpenWorld::Cell::fill_cell_exit_chances(REF_ float& _cellRemove1ExitChance, REF_ float& _cellRemove2ExitsChance, REF_ float& _cellRemove3ExitsChance, REF_ float& _cellRemove4ExitsChance, REF_ float& _cellIgnoreRemovingExitChance)
{
	if (auto* ls = set.get_as_any())
	{
		_cellRemove1ExitChance = ls->get_custom_parameters().get_value<float>(NAME(cellRemove1ExitChance), _cellRemove1ExitChance);
		_cellRemove2ExitsChance = ls->get_custom_parameters().get_value<float>(NAME(cellRemove2ExitsChance), _cellRemove2ExitsChance);
		_cellRemove3ExitsChance = ls->get_custom_parameters().get_value<float>(NAME(cellRemove3ExitsChance), _cellRemove3ExitsChance);
		_cellRemove4ExitsChance = ls->get_custom_parameters().get_value<float>(NAME(cellRemove4ExitsChance), _cellRemove4ExitsChance);
		_cellIgnoreRemovingExitChance = ls->get_custom_parameters().get_value<float>(NAME(cellIgnoreRemovingExitChance), _cellIgnoreRemovingExitChance);
	}
}

void PilgrimageInstanceOpenWorld::Cell::fill_allowed_required_exits(OUT_ bool * _allowedExits, OUT_ bool* _requiredExits, OUT_ bool & _allPossibleExitsRequired) const
{
	if (auto* ls = set.get_as_any())
	{
		_allowedExits[DirFourClockwise::Up] = ! ls->get_custom_parameters().get_value<bool>(NAME(noUpExit), false);
		_allowedExits[DirFourClockwise::Right] = ! ls->get_custom_parameters().get_value<bool>(NAME(noRightExit), false);
		_allowedExits[DirFourClockwise::Down] = ! ls->get_custom_parameters().get_value<bool>(NAME(noDownExit), false);
		_allowedExits[DirFourClockwise::Left] = ! ls->get_custom_parameters().get_value<bool>(NAME(noLeftExit), false);
		_requiredExits[DirFourClockwise::Up] = false;
		_requiredExits[DirFourClockwise::Right] = false;
		_requiredExits[DirFourClockwise::Down] = false;
		_requiredExits[DirFourClockwise::Left] = false;
		bool allExitsRequired = ls->get_custom_parameters().get_value<bool>(NAME(allExitsRequired), false);
		if (allExitsRequired)
		{
			_requiredExits[DirFourClockwise::Up] = _allowedExits[DirFourClockwise::Up];
			_requiredExits[DirFourClockwise::Right] = _allowedExits[DirFourClockwise::Right];
			_requiredExits[DirFourClockwise::Down] = _allowedExits[DirFourClockwise::Down];
			_requiredExits[DirFourClockwise::Left] = _allowedExits[DirFourClockwise::Left];
		}
		_requiredExits[DirFourClockwise::Up] = ls->get_custom_parameters().get_value<bool>(NAME(upExitRequired), _requiredExits[DirFourClockwise::Up]);
		_requiredExits[DirFourClockwise::Right] = ls->get_custom_parameters().get_value<bool>(NAME(rightExitRequired), _requiredExits[DirFourClockwise::Right]);
		_requiredExits[DirFourClockwise::Down] = ls->get_custom_parameters().get_value<bool>(NAME(downExitRequired), _requiredExits[DirFourClockwise::Down]);
		_requiredExits[DirFourClockwise::Left] = ls->get_custom_parameters().get_value<bool>(NAME(leftExitRequired), _requiredExits[DirFourClockwise::Left]);
		_allPossibleExitsRequired = ls->get_custom_parameters().get_value<bool>(NAME(allPossibleExitsRequired), false);
	}
	else
	{
		_allowedExits[DirFourClockwise::Up] = false;
		_allowedExits[DirFourClockwise::Right] = false;
		_allowedExits[DirFourClockwise::Down] = false;
		_allowedExits[DirFourClockwise::Left] = false;
		_requiredExits[DirFourClockwise::Up] = false;
		_requiredExits[DirFourClockwise::Right] = false;
		_requiredExits[DirFourClockwise::Down] = false;
		_requiredExits[DirFourClockwise::Left] = false;
		_allPossibleExitsRequired = false;
	}

	int rotate = dir;
	while (rotate > 0)
	{
		--rotate;

		bool leftAllowed = _allowedExits[DirFourClockwise::Left];
		_allowedExits[DirFourClockwise::Left] = _allowedExits[DirFourClockwise::Down];
		_allowedExits[DirFourClockwise::Down] = _allowedExits[DirFourClockwise::Right];
		_allowedExits[DirFourClockwise::Right] = _allowedExits[DirFourClockwise::Up];
		_allowedExits[DirFourClockwise::Up] = leftAllowed;

		bool leftRequired = _requiredExits[DirFourClockwise::Left];
		_requiredExits[DirFourClockwise::Left] = _requiredExits[DirFourClockwise::Down];
		_requiredExits[DirFourClockwise::Down] = _requiredExits[DirFourClockwise::Right];
		_requiredExits[DirFourClockwise::Right] = _requiredExits[DirFourClockwise::Up];
		_requiredExits[DirFourClockwise::Up] = leftRequired;
	}
}

bool PilgrimageInstanceOpenWorld::Cell::is_conflict_free(Cell const& _cell, Cell const& _against)
{
	bool cellAllowedExits[4];
	bool againstAllowedExits[4];
	bool cellRequiredExits[4];
	bool cellAllPossibleExitsRequired;
	bool againstRequiredExits[4];
	bool againstAllPossibleExitsRequired;

	_cell.fill_allowed_required_exits(cellAllowedExits, cellRequiredExits, OUT_ cellAllPossibleExitsRequired);
	_against.fill_allowed_required_exits(againstAllowedExits, againstRequiredExits, OUT_ againstAllPossibleExitsRequired);

	{
		if (_cell.at.x + 1 == _against.at.x &&
			cellRequiredExits[DirFourClockwise::Right] &&
			cellAllowedExits[DirFourClockwise::Right] &&
			! againstAllowedExits[DirFourClockwise::Left])
		{
			return false;
		}
		if (_cell.at.x - 1 == _against.at.x &&
			cellRequiredExits[DirFourClockwise::Left] &&
			cellAllowedExits[DirFourClockwise::Left] &&
			! againstAllowedExits[DirFourClockwise::Right])
		{
			return false;
		}
		if (_cell.at.y + 1 == _against.at.y &&
			cellRequiredExits[DirFourClockwise::Up] &&
			cellAllowedExits[DirFourClockwise::Up] &&
			! againstAllowedExits[DirFourClockwise::Down])
		{
			return false;
		}
		if (_cell.at.y - 1 == _against.at.y &&
			cellRequiredExits[DirFourClockwise::Down] &&
			cellAllowedExits[DirFourClockwise::Down] &&
			! againstAllowedExits[DirFourClockwise::Up])
		{
			return false;
		}
	}
	{
		if (_cell.at.x + 1 == _against.at.x &&
			! cellAllowedExits[DirFourClockwise::Right] &&
			againstRequiredExits[DirFourClockwise::Left] &&
			againstAllowedExits[DirFourClockwise::Left])
		{
			return false;
		}
		if (_cell.at.x - 1 == _against.at.x &&
			! cellAllowedExits[DirFourClockwise::Left] &&
			againstRequiredExits[DirFourClockwise::Right] &&
			againstAllowedExits[DirFourClockwise::Right])
		{
			return false;
		}
		if (_cell.at.y + 1 == _against.at.y &&
			! cellAllowedExits[DirFourClockwise::Up] &&
			againstRequiredExits[DirFourClockwise::Down] &&
			againstAllowedExits[DirFourClockwise::Down])
		{
			return false;
		}
		if (_cell.at.y - 1 == _against.at.y &&
			! cellAllowedExits[DirFourClockwise::Down] &&
			againstRequiredExits[DirFourClockwise::Up] &&
			againstAllowedExits[DirFourClockwise::Up])
		{
			return false;
		}
	}
	if ((_cell.set.should_avoid_same_neighbours() || _against.set.should_avoid_same_neighbours()) &&
		_cell.set.get_as_any() == _against.set.get_as_any())
	{
		return false;
	}
	return true;
}

void PilgrimageInstanceOpenWorld::Cell::generate_scenery_mesh(int _distIdx, Name const& _meshGeneratorParam, Framework::UsedLibraryStored<Framework::Mesh>& _sceneryMesh, PilgrimageInstanceOpenWorld* _piow) const
{
	auto* library = Framework::Library::get_current();
	Framework::MeshGenerator* mg = nullptr;

	if (!mg)
	{
		if (auto* meshGeneratorName = set.get_as_any()->get_custom_parameters().get_existing<Framework::LibraryName>(_meshGeneratorParam))
		{
			mg = library->get_mesh_generators().find(*meshGeneratorName);
		}
	}

	if (!mg)
	{
		if (auto* meshGeneratorName = set.get_as_any()->get_custom_parameters().get_existing<Framework::LibraryName>(NAME(sceneryMeshGenerator)))
		{
			mg = library->get_mesh_generators().find(*meshGeneratorName);
		}
	}

	if (!mg)
	{
		if (auto* owDef = _piow->pilgrimage->open_world__get_definition())
		{
			mg = owDef->get_background_scenery_default_mesh_generator();
		}
	}

	if (mg)
	{
		Framework::PseudoRoomRegion* prr = nullptr;
		SimpleVariableStorage variables;
		PilgrimageInstanceOpenWorld::HighLevelRegion* hlr = _piow->async_get_or_create_high_level_region_for(at);
		if (Game::get_as<Game>()->does_want_to_cancel_creation())
		{
		}
		if (set.regionType.is_set())
		{
			prr = new Framework::PseudoRoomRegion(hlr ? hlr->containerRegion.get() : _piow->get_main_region(), set.regionType.get());
		}
		else if (set.roomType.is_set())
		{
			prr = new Framework::PseudoRoomRegion(hlr ? hlr->containerRegion.get() : _piow->get_main_region(), set.roomType.get());
		}
		if (prr)
		{
			prr->set_individual_random_generator(randomSeed);
			prr->run_wheres_my_point_processor_setup_parameters();
		}
		Framework::MeshGeneratorRequest request;
		request.using_random_regenerator(randomSeed);
		if (prr)
		{
			prr->collect_variables(REF_ variables);
		}
		{
			variables.access<bool>(NAME(backgroundScenery)) = true;
			variables.access<bool>(NAME(closeBackgroundScenery)) = _distIdx == 0;
			variables.access<bool>(NAME(farBackgroundScenery)) = _distIdx > 0;
			variables.access<bool>(NAME(medFarBackgroundScenery)) = _distIdx > 2;
			variables.access<bool>(NAME(veryFarBackgroundScenery)) = _distIdx > 3;
			variables.access<bool>(NAME(addVistaWindows)) = _distIdx == 1; // these windows are used when putting...? putting what?
			request.using_parameters(variables);
		}
		_sceneryMesh = mg->generate_temporary_library_mesh(library, request);
		delete prr;
	}
}

void PilgrimageInstanceOpenWorld::Cell::get_or_generate_background_mesh(Framework::UsedLibraryStored<Framework::Mesh>& _outputMesh, VectorInt2 const & _at, PilgrimageOpenWorldDefinition::BackgroundCellBased const & _bcd, PilgrimageInstanceOpenWorld* _piow)
{
	auto* library = Framework::Library::get_current();

	if (auto* mg = _bcd.meshGenerator.get())
	{
		Framework::MeshGeneratorRequest request;
		auto& c = _piow->access_cell_at(_at);
		request.using_random_regenerator(c.randomSeed);
		_outputMesh = mg->generate_temporary_library_mesh(library, request);
	}
	else if (auto* m = _bcd.mesh.get())
	{
		_outputMesh = m;
	}
}

PilgrimageInstanceOpenWorld::KnownPilgrimageDevice const* PilgrimageInstanceOpenWorld::Cell::get_known_pilgrimage_device(PilgrimageDevice* _pd) const
{
	for_every(kpd, knownPilgrimageDevices)
	{
		if (kpd->device.get() == _pd)
		{
			return kpd;
		}
	}
	return nullptr;
}

PilgrimageInstanceOpenWorld::KnownPilgrimageDevice* PilgrimageInstanceOpenWorld::Cell::access_known_pilgrimage_device(PilgrimageDevice* _pd)
{
	for_every(kpd, knownPilgrimageDevices)
	{
		if (kpd->device.get() == _pd)
		{
			return kpd;
		}
	}
	return nullptr;
}

void PilgrimageInstanceOpenWorld::Cell::generate_scenery_meshes(PilgrimageInstanceOpenWorld* _piow, int _distIdx)
{
	an_assert(doneForMinPriority == 0, TXT("should be created definitely"));

	if (!set.get_as_any())
	{
		return;
	}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
	if ((!closeSceneryMesh.is_set() && _distIdx == 0) ||
		(!vistaWindowsSceneryMesh.is_set() && _distIdx == 1) ||
		(!farSceneryMesh.is_set() && _distIdx == 2) ||
		(!medFarSceneryMesh.is_set() && _distIdx == 3) ||
		(!veryFarSceneryMesh.is_set() && _distIdx == 4))
#endif
	{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		::System::TimeStamp timeStamp;
		output_colour(0, 1, 1, 1);
		output(TXT("generating missing scenery mesh (%i) for %i x %i"), _distIdx, at.x, at.y);
		output_colour();
#endif
		auto prevCellContext = _piow->cellContext;
		_piow->cellContext = at;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		_piow->generatingSceneries = true;
#endif

		if (!closeSceneryMesh.is_set() && _distIdx == 0)
		{
			generate_scenery_mesh(0, NAME(closeSceneryMeshGenerator), closeSceneryMesh, _piow);
		}
		if (!vistaWindowsSceneryMesh.is_set() && _distIdx == 1)
		{
			generate_scenery_mesh(1, NAME(farSceneryMeshGenerator), vistaWindowsSceneryMesh, _piow);
		}
		if (!farSceneryMesh.is_set() && (_distIdx == 2 || !closeSceneryMesh.is_set())) // we need close or far for close if no close scenery mesh generator provided
		{
			generate_scenery_mesh(2, NAME(farSceneryMeshGenerator), farSceneryMesh, _piow);
		}
		if (!medFarSceneryMesh.is_set() && _distIdx == 3)
		{
			generate_scenery_mesh(3, NAME(farSceneryMeshGenerator), medFarSceneryMesh, _piow);
		}
		if (!veryFarSceneryMesh.is_set() && _distIdx == 4)
		{
			generate_scenery_mesh(4, NAME(farSceneryMeshGenerator), veryFarSceneryMesh, _piow);
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
		_piow->generatingSceneries = false;
#endif
		_piow->cellContext = prevCellContext;

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output_colour(0, 1, 1, 1);
		output(TXT("generated in %.2fs"), timeStamp.get_time_since());
		output_colour();
#endif
	}
}

//--

PilgrimageInstanceOpenWorld::ExistingCell::ExistingCell()
{
}

Framework::Door* PilgrimageInstanceOpenWorld::ExistingCell::get_door_to(VectorInt2 const& _toCellAt, int _doorIdx) const
{
	int workingDoorIdx = _doorIdx;
	for_every(door, doors)
	{
		if (door->toCellAt.is_set() && door->toCellAt.get() == _toCellAt)
		{
			if (workingDoorIdx == 0)
			{
				return door->door.get();
			}
			--workingDoorIdx;
		}
	}

	return nullptr;
}

void PilgrimageInstanceOpenWorld::ExistingCell::add_door(Optional<VectorInt2> const& _toCellAt, int _doorIdx, Framework::Door* _door)
{
	int workingDoorIdx = _doorIdx;
	for_every(door, doors)
	{
		if (door->toCellAt == _toCellAt)
		{
			if (workingDoorIdx == 0)
			{
				if (door->door.is_set() && door->door != _door)
				{
					if (_toCellAt.is_set())
					{
						error(TXT("door between cells (to %ix%i) already exists and is different"), _toCellAt.get().x, _toCellAt.get().y);
					}
					else
					{
						error(TXT("door (special) already exists and is different"));
					}
				}
				door->door = _door;
				return;
			}
			--workingDoorIdx;
		}
	}

	// add a new one, but first add all before (if do not exist)
	while (workingDoorIdx >= 0)
	{
		Door d;
		d.toCellAt = _toCellAt;
		if (workingDoorIdx == 0)
		{
			d.door = _door;
			doors.push_back(d);
			return;
		}
		else
		{
			doors.push_back(d);
		}

		--workingDoorIdx;
	}
}

//--

int PilgrimageInstanceOpenWorld::KnownPilgrimageDevice::compare(void const* _a, void const* _b)
{
	KnownPilgrimageDevice const& a = *plain_cast<KnownPilgrimageDevice>(_a);
	KnownPilgrimageDevice const& b = *plain_cast<KnownPilgrimageDevice>(_b);
	{
		int a_b = a.device->get_priority() - b.device->get_priority();
		if (a_b > 0) return A_BEFORE_B;
		if (a_b < 0) return B_BEFORE_A;
	}
	{
		int a_b = a.subPriority - b.subPriority;
		if (a_b > 0) return A_BEFORE_B;
		if (a_b < 0) return B_BEFORE_A;
	}
	return Framework::LibraryName::compare(&a.device->get_name(), &b.device->get_name());
}

//--

PilgrimageInstanceOpenWorld::CellQueryResult::CellQueryResult()
{
	SET_EXTRA_DEBUG_INFO(knownPilgrimageDevices, TXT("PilgrimageInstanceOpenWorld.CellQueryResult.knownPilgrimageDevices"));
}

Vector2 PilgrimageInstanceOpenWorld::CellQueryResult::calculate_rel_centre() const
{
	Random::Generator rg = randomSeed;
	Vector2 centreLocal;
	Range2 centreLocalRange = cellSet.get_local_centre_for_map();
	centreLocal.x = rg.get(centreLocalRange.x);
	centreLocal.y = rg.get(centreLocalRange.y);
	centreLocal.x = clamp(centreLocal.x, -0.4f, 0.4f);
	centreLocal.y = clamp(centreLocal.y, -0.4f, 0.4f);
	Vector2 centreRel = DirFourClockwise::vector_to_world(centreLocal, dir);

	return centreRel;
}

//

void PilgrimageInstanceOpenWorld::LongDistanceDetection::clear()
{
	Concurrency::ScopedSpinLock l(lock, true);
	detections.clear();
}

PilgrimageInstanceOpenWorld::LongDistanceDetection::Detection & PilgrimageInstanceOpenWorld::LongDistanceDetection::get_for(VectorInt2 const& _at)
{
	an_assert(lock.is_locked_on_this_thread());

	for_every(d, detections)
	{
		if (d->at == _at)
		{
			Detection nd = *d;
			detections.remove_at(for_everys_index(d));
			detections.insert_at(0, nd);
			return detections[0];
		}
	}

	if (!detections.has_place_left())
	{
		detections.pop_back();
	}
	detections.insert_at(0, Detection());
	detections[0].at = _at;
	return detections[0];
}

void PilgrimageInstanceOpenWorld::LongDistanceDetection::copy_from(LongDistanceDetection const& _source)
{
	Concurrency::ScopedSpinLock lockSrc(_source.lock, true);
	Concurrency::ScopedSpinLock lockDest(lock, true);

	detections = _source.detections;
}

//

void PilgrimageInstanceOpenWorld::LongDistanceDetection::Detection::clear()
{
	elements.clear();
}

void PilgrimageInstanceOpenWorld::LongDistanceDetection::Detection::add(float _radius, VectorInt2 const & _deviceAt, Optional<Vector2> const& _directionTowards)
{
	if (elements.has_place_left())
	{
		elements.push_back();
		elements.get_last().radius = _radius;
		elements.get_last().directionTowards = _directionTowards;
		elements.get_last().deviceAt = _deviceAt;
	}
}
