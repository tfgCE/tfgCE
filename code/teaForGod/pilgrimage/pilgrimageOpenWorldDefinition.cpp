#include "pilgrimageOpenWorldDefinition.h"

#include "pilgrimageDevice.h"
#include "pilgrimageEnvironment.h"
#include "pilgrimageInstance.h"
#include "pilgrimageInstanceOpenWorld.h"
#include "..\library\library.h"

#include "..\..\core\system\core.h"
#include "..\..\core\utils\utils_loadingForSystemShader.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// cell custom parameters
DEFINE_STATIC_NAME(cellAnyCrouch);
DEFINE_STATIC_NAME(cellCrouch);
DEFINE_STATIC_NAME(cellProbCoef);
DEFINE_STATIC_NAME(cellNotFirstChoice);
DEFINE_STATIC_NAME(cellPriority);
DEFINE_STATIC_NAME(maxDependentRadius);
DEFINE_STATIC_NAME(maxDependentLeft);
DEFINE_STATIC_NAME(maxDependentRight);
DEFINE_STATIC_NAME(maxDependentUp);
DEFINE_STATIC_NAME(maxDependentDown);

// region custom parameters
DEFINE_STATIC_NAME(highLevelRegionProbCoef);
DEFINE_STATIC_NAME(highLevelSubRegionProbCoef);
DEFINE_STATIC_NAME(highLevelSubRegionTagged);
DEFINE_STATIC_NAME(cellLevelRegionProbCoef);
DEFINE_STATIC_NAME(cellLevelSubRegionProbCoef);
DEFINE_STATIC_NAME(cellLevelSubRegionTagged);

//

bool is_within_size(VectorInt2 const& _at, RangeInt2 const & size)
{
	return (size.x.is_empty() || size.x.does_contain(_at.x)) &&
		   (size.y.is_empty() || size.y.does_contain(_at.y));
}

//

PilgrimageOpenWorldDefinition::PilgrimageOpenWorldDefinition()
{
	SET_EXTRA_DEBUG_INFO(directionTextureParts, TXT("PilgrimageOpenWorldDefinition.directionTextureParts"));
}

bool PilgrimageOpenWorldDefinition::has_door_directions(bool _objectives) const
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("has door directions?"));
	output(TXT("noDoorDirections = %S"), noDoorDirections ? TXT("true") : TXT("false"));
	output(TXT("noDoorDirectionsUnlessObjectives = %S"), noDoorDirectionsUnlessObjectives ? TXT("true") : TXT("false"));
	output(TXT("_objectives = %S"), _objectives ? TXT("true") : TXT("false"));
#endif
	if (noDoorDirections)
	{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT(" +-> no door directions -> return false"));
#endif
		return false;
	}
	if (noDoorDirectionsUnlessObjectives && !_objectives)
	{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT(" +-> no door directions unless objectives and no objectives -> return false"));
#endif
		return false;
	}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT(" +-> has door directions -> return true"));
#endif
	return true;
}

bool PilgrimageOpenWorldDefinition::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	sizes.clear();
	for_every(node, _node->children_named(TXT("size")))
	{
		Size s;
		s.size.load_from_xml(node);
		s.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
		sizes.push_back(s);
	}

	createAllCells = _node->get_bool_attribute_or_from_child_presence(TXT("createAllCells"), createAllCells);
	enforceAllCellsReachable = _node->get_bool_attribute_or_from_child_presence(TXT("enforceAllCellsReachable"), enforceAllCellsReachable);
	authorisedCellsUnreachable = _node->get_bool_attribute_or_from_child_presence(TXT("authorisedCellsUnreachable"), authorisedCellsUnreachable);
	for_every(node, _node->children_named(TXT("attemptToCellsHave")))
	{
		attemptToCellsHave.exitsCount = node->get_int_attribute_or_from_child(TXT("exitsCount"), attemptToCellsHave.exitsCount);
		attemptToCellsHave.exitsCountAttemptChance = node->get_float_attribute_or_from_child(TXT("exitsCountAttemptChance"), attemptToCellsHave.exitsCountAttemptChance);
	}

	for_every(node, _node->children_named(TXT("cellExits")))
	{
		cellRemove1ExitChance = node->get_float_attribute_or_from_child(TXT("remove1ExitChance"), cellRemove1ExitChance);
		cellRemove2ExitsChance = node->get_float_attribute_or_from_child(TXT("remove2ExitsChance"), cellRemove2ExitsChance);
		cellRemove3ExitsChance = node->get_float_attribute_or_from_child(TXT("remove3ExitsChance"), cellRemove3ExitsChance);
		cellRemove4ExitsChance = node->get_float_attribute_or_from_child(TXT("remove4ExitsChance"), cellRemove4ExitsChance);
		cellIgnoreRemovingExitChance = node->get_float_attribute_or_from_child(TXT("ignoreRemovingExitChance"), cellIgnoreRemovingExitChance);
	}

	startAt.load_from_xml_child_node(_node, TXT("startAt"));
	autoGenerateAtStart.clear();
	for_every(node, _node->children_named(TXT("autoGenerateAtStart")))
	{
		for_every(cnode, node->children_named(TXT("cellAt")))
		{
			VectorInt2 cellAt = startAt.centre();
			cellAt.load_from_xml(cnode);
			autoGenerateAtStart.push_back_unique(cellAt);
		}
	}
	alwaysGenerate.clear();
	for_every(node, _node->children_named(TXT("alwaysGenerate")))
	{
		for_every(cnode, node->children_named(TXT("cellAt")))
		{
			VectorInt2 cellAt = startAt.centre();
			cellAt.load_from_xml(cnode);
			alwaysGenerate.push_back_unique(cellAt);
		}
	}

	vrElevatorAltitudeGradient.load_from_xml_child_node(_node, TXT("vrElevatorAltitudeGradient"));

	patterns.clear();
	for_every(node, _node->children_named(TXT("highLevelRegionPattern")))
	{
		PatternElement p;
		{
			String patternString = node->get_text();
			if (!patternString.is_empty())
			{
				if (patternString == TXT("crosses")) p.pattern = Pattern::Crosses; else
				if (patternString == TXT("cross squre")) p.pattern = Pattern::CrossSqure; else
				if (patternString == TXT("single lines")) p.pattern = Pattern::SingleLines; else
				if (patternString == TXT("double lines")) p.pattern = Pattern::DoubleLines; else
				if (patternString == TXT("double lines offset")) p.pattern = Pattern::DoubleLinesOffset; else
				if (patternString == TXT("squares aligned")) p.pattern = Pattern::SquaresAligned; else
				if (patternString == TXT("squares moved x")) p.pattern = Pattern::SquaresMovedX; else
				{
					error_loading_xml(node, TXT("couldn't parse pattern \"%S\""), patternString.to_char());
					result = false;
				}
			}
		}
		p.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
		patterns.push_back(p);
	}

	for_every(node, _node->children_named(TXT("main")))
	{
		result &= mainRegionType.load_from_xml(node, TXT("regionType"), _lc);
	}

	for_every(node, _node->children_named(TXT("cellDoors")))
	{
		for_every(cdnode, node->children_named(TXT("cellDoor")))
		{
			cellDoorVRPlacements.push_back(CellDoorVRPlacement());
			CellDoorVRPlacement & cd = cellDoorVRPlacements.get_last();
			cd.from.load_from_xml_child_or_attr(cdnode, TXT("from"));
			cd.to.load_from_xml_child_or_attr(cdnode, TXT("to"));
			result &= cd.wheresMyPoint.load_from_xml(cdnode->first_child_named(TXT("wheresMyPoint")));
		}
	}

	alternateCellDoorVRPlacement = _node->get_bool_attribute_or_from_child_presence(TXT("alternateCellDoorVRPlacement"), alternateCellDoorVRPlacement);
	alternateCellDoorVRPlacement = ! _node->get_bool_attribute_or_from_child_presence(TXT("doNotAlternateCellDoorVRPlacement"), ! alternateCellDoorVRPlacement);

	cellDoorTileRange.load_from_xml(_node, TXT("cellDoorTileRange"));

	backgroundDefinitions.clear();

	for_every(node, _node->children_named(TXT("background")))
	{
		RangeInt2 forCell = RangeInt2::empty;

		forCell.load_from_xml(node, TXT("forCellX"), TXT("forCellY"));
		forCell.load_from_xml_child_node(node, TXT("forCell"));

		BackgroundDefinition* bd = nullptr;
		for_every(bds, backgroundDefinitions)
		{
			if (bds->forCell == forCell)
			{
				bd = bds;
			}
		}
		if (!bd)
		{
			backgroundDefinitions.grow_size(1);
			bd = &backgroundDefinitions.get_last();
			bd->forCell = forCell;
		}

		backgroundSceneryAnchorVariableName = node->get_name_attribute(TXT("anchorVariableName"), backgroundSceneryAnchorVariableName);
		result &= backgroundSceneryType.load_from_xml(node, TXT("sceneryType"), _lc);
		result &= defaultBackgroundSceneryMeshGenerator.load_from_xml(node, TXT("defaultMeshGenerator"), _lc);
		combineSceneryCellsSize = node->get_int_attribute_or_from_child(TXT("combineSceneryCellsSize"), combineSceneryCellsSize);
		backgroundSceneryCellDistanceVisibility = node->get_int_attribute_or_from_child(TXT("cellDistanceVisibility"), backgroundSceneryCellDistanceVisibility);
		backgroundSceneryCellDistanceVisibilityVeryFar = node->get_int_attribute_or_from_child(TXT("cellDistanceVisibilityVeryFar"), backgroundSceneryCellDistanceVisibilityVeryFar);

		for_every(addNode, node->children_named(TXT("add")))
		{
			if (addNode->has_attribute(TXT("sceneryType")))
			{
				BackgroundSceneryObject bs;
				bs.forBullshots.load_from_xml(addNode, TXT("forBullshots"));
				bs.sceneryType.load_from_xml(addNode, TXT("sceneryType"), _lc);
				bs.placement.load_from_xml_child_node(addNode, TXT("placement"));
				bs.applyCellOffsetX = addNode->get_bool_attribute_or_from_child_presence(TXT("applyCellOffsetXY"), bs.applyCellOffsetX);
				bs.applyCellOffsetY = addNode->get_bool_attribute_or_from_child_presence(TXT("applyCellOffsetXY"), bs.applyCellOffsetY);
				bs.applyCellOffsetX = addNode->get_bool_attribute_or_from_child_presence(TXT("applyCellOffsetX"), bs.applyCellOffsetX);
				bs.applyCellOffsetY = addNode->get_bool_attribute_or_from_child_presence(TXT("applyCellOffsetY"), bs.applyCellOffsetY);
				bd->backgroundSceneryObjects.push_back(bs);
			}
			else
			{
				error_loading_xml(node, TXT("not recognised"));
				result = false;
			}
		}

		for_every(forNode, node->children_named(TXT("for")))
		{
			if (CoreUtils::Loading::should_load_for_system_or_shader_option(forNode))
			{
				combineSceneryCellsSize = forNode->get_int_attribute_or_from_child(TXT("combineSceneryCellsSize"), combineSceneryCellsSize); 
				backgroundSceneryCellDistanceVisibility = forNode->get_int_attribute_or_from_child(TXT("cellDistanceVisibility"), backgroundSceneryCellDistanceVisibility);
				backgroundSceneryCellDistanceVisibilityVeryFar = forNode->get_int_attribute_or_from_child(TXT("cellDistanceVisibilityVeryFar"), backgroundSceneryCellDistanceVisibilityVeryFar);
			}
		}
	}

	backgroundsCellBased.clear();

	for_every(cnode, _node->children_named(TXT("backgroundsCellBased")))
	{
		for_every(node, cnode->children_named(TXT("background")))
		{
			backgroundsCellBased.push_back();
			auto& bcd = backgroundsCellBased.get_last();
			result &= bcd.mesh.load_from_xml(node, TXT("mesh"), _lc);
			result &= bcd.meshGenerator.load_from_xml(node, TXT("meshGenerator"), _lc);
			bcd.at.load_from_xml(node, TXT("atX"), TXT("atY"));
			bcd.at.load_from_xml_child_node(node, TXT("at"));
			bcd.repeatOffset.load_from_xml_child_node(node, TXT("repeatOffset"));
			for_every(n, node->children_named(TXT("repeatOffsetNotAxisAligned")))
			{
				if (bcd.repeatOffsetNotAxisAligned.has_place_left())
				{
					bcd.repeatOffsetNotAxisAligned.push_back();
					auto& ro = bcd.repeatOffsetNotAxisAligned.get_last();
					ro.load_from_xml(n);
				}
				else
				{
					error_loading_xml(n, TXT("too many not axis aligned repeat offsets"));
					result = false;
				}
			}
			ArrayStatic<VectorInt2, 4> repeatOffsetNotAxisAligned; // not axis-aligned, may be a few of them

			bcd.placement.load_from_xml_child_node(node, TXT("placement"));
			bcd.visibilityRange = node->get_float_attribute(TXT("visibilityRange"), bcd.visibilityRange);
			bcd.extraVisibilityRangeCellRadius = node->get_int_attribute(TXT("extraVisibilityRangeCellRadius"), bcd.extraVisibilityRangeCellRadius);
		}
	}
	
	for_every(node, _node->children_named(TXT("vistaScenery")))
	{
		result &= vistaSceneryRoomType.load_from_xml(node, TXT("roomType"), _lc);
	}

	allowDelayedObjectCreationOnStart = _node->get_bool_attribute_or_from_child_presence(TXT("allowDelayedObjectCreationOnStart"), allowDelayedObjectCreationOnStart);

	for_every(node, _node->children_named(TXT("starting")))
	{
		result &= startingRoomType.load_from_xml(node, TXT("roomType"), _lc);
		result &= startingEntranceDoorType.load_from_xml(node, TXT("doorType"), _lc);
		result &= startingExitDoorType.load_from_xml(node, TXT("doorType"), _lc);
		result &= startingEntranceDoorType.load_from_xml(node, TXT("entranceDoorType"), _lc);
		result &= startingExitDoorType.load_from_xml(node, TXT("exitDoorType"), _lc);
		startingDoorConnectWithPreviousPilgrimage = node->get_bool_attribute_or_from_child_presence(TXT("connectWithPreviousPilgrimage"), startingDoorConnectWithPreviousPilgrimage);
	}
	// noCellsRooms should be after starting
	for_every(node, _node->children_named(TXT("noCellsRooms")))
	{
		limitNoCellRoomsPregeneration.load_from_xml(node, TXT("limitPregeneration"));

		for_every(ncrNode, node->children_named(TXT("add")))
		{
			NoCellsRoom ncr;
			ncr.tags.load_from_xml_attribute_or_child_node(ncrNode, TXT("tags"));
			if (ncr.roomType.load_from_xml(ncrNode, TXT("roomType"), _lc) &&
				ncr.toNextDoorType.load_from_xml(ncrNode, TXT("toNextDoorType"), _lc))
			{
				noCellsRooms.push_back(ncr);
			}
			else
			{
				result = false;
			}
		}
	}
	//
	if (!startingRoomType.is_name_valid() && !noCellsRooms.is_empty())
	{
		// make starting room from the first nocells
		startingRoomType = noCellsRooms.get_first().roomType;
		if (!startingExitDoorType.is_name_valid())
		{
			startingExitDoorType = noCellsRooms.get_first().toNextDoorType;
		}
	}
	if (startingRoomType.is_name_valid() &&
		!noCellsRooms.is_empty() && startingRoomType != noCellsRooms.get_first().roomType)
	{
		error_loading_xml(_node, TXT("you have to define starting room as part of no cell rooms"));
		result = false;
	}
	if (startingRoomType.is_name_valid() && noCellsRooms.is_empty())
	{
		NoCellsRoom ncr;
		ncr.roomType = startingRoomType;
		ncr.toNextDoorType = startingExitDoorType;
		noCellsRooms.push_front(ncr);
	}

	for_every(node, _node->children_named(TXT("cell")))
	{
		result &= cellRegionType.load_from_xml(node, TXT("regionType"), _lc);
	}

	allowHighLevelRegionAtStart = _node->get_bool_attribute_or_from_child_presence(TXT("allowHighLevelRegionAtStart"), allowHighLevelRegionAtStart);
	for_every(node, _node->children_named(TXT("highLevelRegionSet")))
	{
		result &= highLevelRegionSet.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("cellLevelRegionSet")))
	{
		result &= cellLevelRegionSet.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("genericCellSet")))
	{
		result &= genericCellSet.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("forRulesCellSet")))
	{
		result &= forRulesCellSet.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("forObjectivesCellSet")))
	{
		result &= forObjectivesCellSet.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("startingCellSet")))
	{
		result &= startingCellSet.load_from_xml(node, _lc);
	}

	noCells = _node->get_bool_attribute_or_from_child_presence(TXT("noCells"), noCells);
	noStartingRoomDoorOutConnection = _node->get_bool_attribute_or_from_child_presence(TXT("noStartingRoomDoorOutConnection"), noStartingRoomDoorOutConnection);
	noTransitionDoorOutRequired = _node->get_bool_attribute_or_from_child_presence(TXT("noTransitionDoorOutRequired"), noTransitionDoorOutRequired);
	requiresDirectionsTowardsObjective = _node->get_bool_attribute_or_from_child_presence(TXT("requiresDirectionsTowardsObjective"), requiresDirectionsTowardsObjective);
	noDoorDirections = _node->get_bool_attribute_or_from_child_presence(TXT("noDoorDirections"), noDoorDirections);
	noDoorDirectionsUnlessObjectives = _node->get_bool_attribute_or_from_child_presence(TXT("noDoorDirectionsUnlessObjectives"), noDoorDirectionsUnlessObjectives);
	noTileChangeNotification = _node->get_bool_attribute_or_from_child_presence(TXT("noTileChangeNotification"), noTileChangeNotification);
	followGuidanceBlocked = _node->get_bool_attribute_or_from_child_presence(TXT("followGuidanceBlocked"), followGuidanceBlocked);
	noMap = _node->get_bool_attribute_or_from_child_presence(TXT("noMap"), noMap);
	wholeMapKnown = _node->get_bool_attribute_or_from_child_presence(TXT("wholeMapKnown"), wholeMapKnown);

	if (noCells)
	{
		sizes.clear();
	}

	error_loading_xml_on_assert(noCells || genericCellSet.is_defined(), result, _node, TXT("no genericCellSet provided and not marked as noCells"));
	error_loading_xml_on_assert(noCells || startingCellSet.is_defined() || (! noCells && is_within_size(startAt.centre(), get_size())), result, _node, TXT("no startingCellSet provided (at start is outside) and not marked as noCells"));
	error_loading_xml_on_assert(startingRoomType.is_name_valid(), result, _node, TXT("no starting room type"));
	error_loading_xml_on_assert(noCells || startingEntranceDoorType.is_name_valid() || startingExitDoorType.is_name_valid(), result, _node, TXT("no starting door type and not marked as noCells"));
	error_loading_xml_on_assert(cellRegionType.is_name_valid(), result, _node, TXT("no cell region type"));
	for_every(ncr, noCellsRooms)
	{
		error_loading_xml_on_assert(ncr->roomType.is_name_valid(), result, _node, TXT("no noCellsRoom \"roomType\""));
		if (for_everys_index(ncr) < noCellsRooms.get_size() - 1)
		{
			error_loading_xml_on_assert(ncr->toNextDoorType.is_name_valid(), result, _node, TXT("no noCellsRoom \"toNextDoorType\""));
		}
	}
	if (!_node->get_bool_attribute_or_from_child_presence(TXT("noBackground")))
	{
		if (!backgroundSceneryType.is_name_valid())
		{
			warn_loading_xml(_node, TXT("background sceneryType strongly suggested!"));
		}
		if (!defaultBackgroundSceneryMeshGenerator.is_name_valid())
		{
			warn_loading_xml(_node, TXT("background defaultMeshGenerator strongly suggested!"));
		}
	}

	for_every(node, _node->children_named(TXT("rules")))
	{
		for_every(rnode, node->children_named(TXT("rule")))
		{
			Rule rule;
			if (rule.load_from_xml(rnode, _lc))
			{
				rules.push_back(rule);
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("noDoorMarker")))
	{
		noDoorMarkerSceneryType.load_from_xml(node, TXT("sceneryType"), _lc);
	}

	for_every(node, _node->children_named(TXT("blockAwayFromObjective")))
	{
		BlockAwayFromObjective b;
		b.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
		b.timePerCell.load_from_xml(node, TXT("timePerCell"));
		b.blockBehind.load_from_xml(node, TXT("blockBehind"));
		b.startBlockedAwayCells.load_from_xml(node, TXT("startBlockedAwayCells"));
		b.shouldBlockHostileSpawnsAtBlockedAwayCells.load_from_xml(node, TXT("shouldBlockHostileSpawnsAtBlockedAwayCells"));
		result &= b.noDoorMarkerSceneryType.load_from_xml(node, TXT("noDoorMarkerSceneryType"), _lc);
		blockAwayFromObjectives.push_back(b);
	}

	for_every(node, _node->children_named(TXT("objectives")))
	{
		objectivesGeneralDir.load_from_xml(node, TXT("generalDir"));
		sequentialObjectives = node->get_bool_attribute_or_from_child_presence(TXT("sequentialObjectives"), sequentialObjectives);
		for_every(rnode, node->children_named(TXT("objective")))
		{
			Objective objective;
			if (objective.load_from_xml(rnode, _lc))
			{
				objectives.push_back(objective);
			}
			else
			{
				result = false;
			}
		}
	}

	result &= skipContentSet.load_from_xml(_node);
	for_every(node, _node->children_named(TXT("skipContentInfo")))
	{
		result &= skipContentSet.load_from_xml(node);
	}

	cellSize = _node->get_float_attribute_or_from_child(TXT("cellSize"), cellSize);
	cellSizeInner = cellSize * 0.9f;
	cellSizeInner = _node->get_float_attribute_or_from_child(TXT("cellSizeInner"), cellSizeInner);

	randomPilgrimageDevices.clear();
	distancedPilgrimageDevices.clear();
	for_every(node, _node->children_named(TXT("pilgrimageDevices")))
	{
		distancedPilgrimageDevicesTileSize = node->get_int_attribute(TXT("distancedTileSize"), distancedPilgrimageDevicesTileSize);
		for_every(n, node->children_named(TXT("random")))
		{
			RandomPilgrimageDevices rpd;
			rpd.tagged.load_from_xml_attribute(n, TXT("tagged"));
			rpd.forGameDefinitionTagged.load_from_xml_attribute(n, TXT("forGameDefinitionTagged"));
			randomPilgrimageDevices.push_back(rpd);
		}
		for_every(n, node->children_named(TXT("distanced")))
		{
			DistancedPilgrimageDevices dpd;
			dpd.groupId = n->get_name_attribute(TXT("groupId"), dpd.groupId);
			dpd.useLimit = n->get_int_attribute(TXT("useLimit"), dpd.useLimit);
			dpd.tagged.load_from_xml_attribute(n, TXT("tagged"));
			dpd.forGameDefinitionTagged.load_from_xml_attribute(n, TXT("forGameDefinitionTagged"));
			dpd.distance.load_from_xml(n, TXT("distance"));
			dpd.atXLine.load_from_xml(n, TXT("atXLine"));
			dpd.atYLine.load_from_xml(n, TXT("atYLine"));
			for_every(nf, n->children_named(TXT("forcedAt")))
			{
				VectorInt2 at = VectorInt2::zero;
				at.load_from_xml(nf);
				dpd.forcedAt.push_back(at);
			}
			for_every(nf, n->children_named(TXT("centreBased")))
			{
				dpd.centreBased.use = true;
				dpd.centreBased.distanceCentreToStartAt.load_from_xml(nf, TXT("distanceCentreToStartAt"));
				dpd.centreBased.distanceFromCentre.load_from_xml(nf, TXT("distanceFromCentre"));
				dpd.centreBased.minCount = nf->get_int_attribute(TXT("minCount"), dpd.centreBased.minCount);
				dpd.centreBased.maxCount.load_from_xml(nf, TXT("maxCount"));
				dpd.centreBased.yRange.load_from_xml(nf, TXT("yRange"));
				dpd.centreBased.xRange.load_from_xml(nf, TXT("xRange"));
			}
			distancedPilgrimageDevices.push_back(dpd);
		}
	}

	if (distancedPilgrimageDevices.get_size() > 1)
	{
		bool allUnique = true;
		for_every(dpd, distancedPilgrimageDevices)
		{
			for_every_continue_after(cdpd, dpd)
			{
				if (cdpd->groupId == dpd->groupId)
				{
					allUnique = false;
					break;
				}
			}
		}
		error_loading_xml_on_assert(allUnique, result, _node, TXT("multiple distanced pilgrimage devices have same groupId"));
	}

	directionTextureParts.grow_size(directionTextureParts.get_space_left());

	for_every(node, _node->children_named(TXT("directions")))
	{
		result &= directionTextureParts[DoorDirectionCombination::Up].load_from_xml_child_node(node, TXT("up"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::UpD].load_from_xml_child_node(node, TXT("upD"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::UpR].load_from_xml_child_node(node, TXT("upR"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::UpRD].load_from_xml_child_node(node, TXT("upRD"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::UpRDL].load_from_xml_child_node(node, TXT("upRDL"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::UpRL].load_from_xml_child_node(node, TXT("upRL"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::NoUp].load_from_xml_child_node(node, TXT("noUp"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::NoUpD].load_from_xml_child_node(node, TXT("noUpD"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::NoUpR].load_from_xml_child_node(node, TXT("noUpR"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::NoUpRD].load_from_xml_child_node(node, TXT("noUpRD"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::NoUpRDL].load_from_xml_child_node(node, TXT("noUpRDL"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::NoUpRL].load_from_xml_child_node(node, TXT("noUpRL"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::UpExitTo].load_from_xml_child_node(node, TXT("upExitTo"), TXT("texturePart"), _lc);
		result &= directionTextureParts[DoorDirectionCombination::UpEnteredFrom].load_from_xml_child_node(node, TXT("upEnteredFrom"), TXT("texturePart"), _lc);
		result &= towardsObjectiveTextuerPart.load_from_xml_child_node(node, TXT("towardsObjective"), TXT("texturePart"), _lc);
	}

	{
		for_every(node, _node->children_named(TXT("lineModelsAlwaysOnMap")))
		{
			for_every(n, node->children_named(TXT("alwaysOnMap")))
			{
				LineModelOnMap lmom;
				lmom.lineModel.load_from_xml(n, TXT("lineModel"), _lc);
				lmom.storyFactsRequired.load_from_xml_attribute_or_child_node(n, TXT("storyFactsRequired"));
				lmom.at.load_from_xml(n);
				lmom.at.load_from_xml_child_node(n, TXT("at"));
				lineModelsAlwaysOnMap.push_back(lmom);
			}
		}
		for_every(n, _node->children_named(TXT("alwaysOnMap")))
		{
			LineModelOnMap lmom;
			lmom.lineModel.load_from_xml(n, TXT("lineModel"), _lc);
			lmom.storyFactsRequired.load_from_xml_attribute_or_child_node(n, TXT("storyFactsRequired"));
			lmom.at.load_from_xml(n);
			lmom.at.load_from_xml_child_node(n, TXT("at"));
			lineModelsAlwaysOnMap.push_back(lmom);
		}

		alwaysShowWholeMap = _node->get_bool_attribute_or_from_child_presence(TXT("alwaysShowWholeMap"), alwaysShowWholeMap);
	}

	{
		restartInRoomTagged.clear();
		for_every(node, _node->children_named(TXT("restart")))
		{
			RestartInRoomTagged rirt;
			rirt.cellAt.load_from_xml(node, TXT("cellAt"));
			rirt.roomTagged.load_from_xml_attribute_or_child_node(node, TXT("inRoomTagged"));
			rirt.onStoryFacts.load_from_xml_attribute_or_child_node(node, TXT("onStoryFacts"));
			restartInRoomTagged.push_back(rirt);
		}
	}

	if (createAllCells || enforceAllCellsReachable || attemptToCellsHave.exitsCount)
	{
		bool finiteSize = ! sizes.is_empty();
		for_every(s, sizes)
		{
			if (s->size.x.is_empty())
			{
				finiteSize = false;
			}
			if (s->size.y.is_empty())
			{
				finiteSize = false;
			}
		}
		if (!finiteSize)
		{
			if (createAllCells) { result = false; error_loading_xml(_node, TXT("createAllCells requires finite size defined")); }
			if (enforceAllCellsReachable) { result = false; error_loading_xml(_node, TXT("enforceAllCellsReachable requires finite size defined")); }
			if (attemptToCellsHave.exitsCount) { result = false; error_loading_xml(_node, TXT("attemptToCellsHave.exitsCount requires finite size defined")); }
		}
	}

	for_every(node, _node->children_named(TXT("specialRules")))
	{
		specialRules.upgradeMachinesProvideMap = node->get_bool_attribute_or_from_child_presence(TXT("upgradeMachinesProvideMap"), specialRules.upgradeMachinesProvideMap);
		specialRules.upgradeMachinesProvideMap = ! node->get_bool_attribute_or_from_child_presence(TXT("upgradeMachinesDontProvideMap"), !specialRules.upgradeMachinesProvideMap);
		specialRules.pilgrimOverlayInformsAboutNewMap = node->get_bool_attribute_or_from_child_presence(TXT("pilgrimOverlayInformsAboutNewMap"), specialRules.pilgrimOverlayInformsAboutNewMap);
		specialRules.pilgrimOverlayInformsAboutNewMap = ! node->get_bool_attribute_or_from_child_presence(TXT("pilgrimOverlayDoesNotInformAboutNewMap"), !specialRules.pilgrimOverlayInformsAboutNewMap);
		specialRules.silentAutomaps = node->get_bool_attribute_or_from_child_presence(TXT("silentAutomaps"), specialRules.silentAutomaps);
		specialRules.startingRoomWithoutDirection = node->get_bool_attribute_or_from_child_presence(TXT("startingRoomWithoutDirection"), specialRules.startingRoomWithoutDirection);
		{
			String dir = node->get_string_attribute_or_from_child(TXT("startingRoomDoorDirection"));
			if (!dir.is_empty())
			{
				specialRules.startingRoomDoorDirection = DirFourClockwise::parse(dir);
			}
		}
		specialRules.transitionToNextRoomWithoutDirection = node->get_bool_attribute_or_from_child_presence(TXT("transitionToNextRoomWithoutDirection"), specialRules.transitionToNextRoomWithoutDirection);
		{
			String dir = node->get_string_attribute_or_from_child(TXT("transitionToNextRoomDoorDirection"));
			if (!dir.is_empty())
			{
				specialRules.transitionToNextRoomDoorDirection = DirFourClockwise::parse(dir);
			}
		}
		specialRules.transitionToNextRoomWithObjectiveDirection = node->get_bool_attribute_or_from_child_presence(TXT("transitionToNextRoomWithObjectiveDirection"), specialRules.transitionToNextRoomWithObjectiveDirection);

		specialRules.detectLongDistanceDetectableDevicesAmount.load_from_xml(node, TXT("detectLongDistanceDetectableDevicesAmount"));
		specialRules.detectLongDistanceDetectableDevicesRadius.load_from_xml(node, TXT("detectLongDistanceDetectableDevicesRadius"));
	}

	return result;
}

bool PilgrimageOpenWorldDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= mainRegionType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= backgroundSceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= defaultBackgroundSceneryMeshGenerator.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	for_every(bd, backgroundDefinitions)
	{
		for_every(bs, bd->backgroundSceneryObjects)
		{
			result &= bs->sceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
		}
	}

	for_every(bcd, backgroundsCellBased)
	{
		result &= bcd->mesh.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
		result &= bcd->meshGenerator.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	result &= vistaSceneryRoomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= startingRoomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= startingEntranceDoorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= startingExitDoorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	
	result &= noDoorMarkerSceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	for_every(b, blockAwayFromObjectives)
	{
		result &= b->noDoorMarkerSceneryType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	
	for_every(ncr, noCellsRooms)
	{
		result &= ncr->roomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
		result &= ncr->toNextDoorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	result &= cellRegionType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	
	result &= highLevelRegionSet.prepare_for_game(_library, _pfgContext,
		NAME(highLevelRegionProbCoef), NAME(highLevelSubRegionProbCoef), NAME(highLevelSubRegionTagged));
	result &= cellLevelRegionSet.prepare_for_game(_library, _pfgContext,
		NAME(cellLevelRegionProbCoef), NAME(cellLevelSubRegionProbCoef), NAME(cellLevelSubRegionTagged));

	result &= genericCellSet.prepare_for_game(_library, _pfgContext);
	result &= forRulesCellSet.prepare_for_game(_library, _pfgContext);
	result &= forObjectivesCellSet.prepare_for_game(_library, _pfgContext);
	result &= startingCellSet.prepare_for_game(_library, _pfgContext);

	for_every(dtp, directionTextureParts)
	{
		result &= dtp->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	result &= towardsObjectiveTextuerPart.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	
	for_every(objective, objectives)
	{
		result &= objective->prepare_for_game(_library, _pfgContext);
	}

	for_every(lmom, lineModelsAlwaysOnMap)
	{
		result &= lmom->lineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	//

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		for_every(rpd, randomPilgrimageDevices)
		{
			rpd->devices.clear();
			if (auto* lib = up_cast<Library>(_library))
			{
				for_every_ptr(pd, lib->get_pilgrimage_devices().get_tagged(rpd->tagged))
				{
					rpd->devices.push_back(Framework::UsedLibraryStored<PilgrimageDevice>(pd));
				}
			}
			sort(rpd->devices);
		}

		for_every(dpd, distancedPilgrimageDevices)
		{
			dpd->devices.clear();
			if (auto* lib = up_cast<Library>(_library))
			{
				for_every_ptr(pd, lib->get_pilgrimage_devices().get_tagged(dpd->tagged))
				{
					dpd->devices.push_back(Framework::UsedLibraryStored<PilgrimageDevice>(pd));
				}
			}
			sort(dpd->devices);
		}
	}

	return result;
}

PilgrimageOpenWorldDefinition::BlockAwayFromObjective const* PilgrimageOpenWorldDefinition::get_block_away_from_objective() const
{
	for_every(b, blockAwayFromObjectives)
	{
		if (!b->forGameDefinitionTagged.is_empty() &&
			GameDefinition::check_chosen(b->forGameDefinitionTagged))
		{
			return b;
		}
	}
	for_every(b, blockAwayFromObjectives)
	{
		if (b->forGameDefinitionTagged.is_empty())
		{
			return b;
		}
	}
	return nullptr;
}

RangeInt2 const& PilgrimageOpenWorldDefinition::get_size() const
{
	if (noCells)
	{
		return RangeInt2::zero;
	}
	if (sizes.is_empty())
	{
		return RangeInt2::empty;
	}
	if (sizes.get_size() == 1)
	{
		return sizes.get_first().size;
	}
	for_every(size, sizes)
	{
		if (!size->forGameDefinitionTagged.is_empty() &&
			GameDefinition::check_chosen(size->forGameDefinitionTagged))
		{
			return size->size;
		}
	}
	for_every(size, sizes)
	{
		if (size->forGameDefinitionTagged.is_empty())
		{
			return size->size;
		}
	}
	return RangeInt2::empty;
}

PilgrimageOpenWorldDefinition::Pattern PilgrimageOpenWorldDefinition::get_pattern() const
{
	if (patterns.is_empty())
	{
		return Pattern::Crosses;
	}
	if (patterns.get_size() == 1)
	{
		return patterns.get_first().pattern;
	}
	for_every(pattern, patterns)
	{
		if (!pattern->forGameDefinitionTagged.is_empty() &&
			GameDefinition::check_chosen(pattern->forGameDefinitionTagged))
		{
			return pattern->pattern;
		}
	}
	for_every(pattern, patterns)
	{
		if (pattern->forGameDefinitionTagged.is_empty())
		{
			return pattern->pattern;
		}
	}
	return Pattern::Crosses;
}

PilgrimageOpenWorldDefinition::CellDoorVRPlacement const* PilgrimageOpenWorldDefinition::get_cell_door_vr_placement(VectorInt2 const& _from, VectorInt2 const& _to) const
{
	for_every(cd, cellDoorVRPlacements)
	{
		if (cd->from == _from &&
			cd->to == _to)
		{
			return cd;
		}
	}
	return nullptr;
}

PilgrimageOpenWorldDefinition::BackgroundDefinition const * PilgrimageOpenWorldDefinition::find_background_definition(VectorInt2 const& _at) const
{
	if (!backgroundDefinitionIdxForCellAt.is_set() ||
		backgroundDefinitionIdxForCellAt.get() != _at)
	{
		backgroundDefinitionIdxForCellAt = _at;
		backgroundDefinitionIdx = NONE;
		for_every(bd, backgroundDefinitions)
		{
			if ((bd->forCell.x.is_empty() ||
				 bd->forCell.x.does_contain(_at.x)) &&
				(bd->forCell.y.is_empty() ||
				 bd->forCell.y.does_contain(_at.y)))
			{
				backgroundDefinitionIdx = for_everys_index(bd);
			}
		}
	}

	return backgroundDefinitions.is_index_valid(backgroundDefinitionIdx) ? &backgroundDefinitions[backgroundDefinitionIdx] : nullptr;
}

void PilgrimageOpenWorldDefinition::fill_cell_exit_chances(OUT_ float& _cellRemove1ExitChance, OUT_ float& _cellRemove2ExitsChance, OUT_ float& _cellRemove3ExitsChance, OUT_ float& _cellRemove4ExitsChance, OUT_ float& _cellIgnoreRemovingExitChance) const
{
	_cellRemove1ExitChance = cellRemove1ExitChance;
	_cellRemove2ExitsChance = cellRemove2ExitsChance;
	_cellRemove3ExitsChance = cellRemove3ExitsChance;
	_cellRemove4ExitsChance = cellRemove4ExitsChance;
	_cellIgnoreRemovingExitChance = cellIgnoreRemovingExitChance;
}

//

bool PilgrimageOpenWorldDefinition::CellSet::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("include")))
	{
		if (auto* attr = node->get_attribute(TXT("roomType")))
		{
			IncludeExplicit<Framework::RoomType> rt;
			if (rt.element.load_from_xml(attr, _lc))
			{
				rt.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
				warn_loading_xml(_node, TXT("avoid using room types for open world cells, use regions with embedded room types"));
				includeRoomTypes.push_back(rt);
			}
		}
		if (auto* attr = node->get_attribute(TXT("regionType")))
		{
			IncludeExplicit<Framework::RegionType> rt;
			if (rt.element.load_from_xml(attr, _lc))
			{
				rt.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
				includeRegionTypes.push_back(rt);
			}
		}
		if (auto* attr = node->get_attribute(TXT("roomTypesTagged")))
		{
			IncludeTagged it;
			if (it.tagged.load_from_string(attr->get_as_string()))
			{
				it.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
				includeRoomTypesTagged.push_back(it);
			}
		}
		if (auto* attr = node->get_attribute(TXT("regionTypesTagged")))
		{
			IncludeTagged it;
			if (it.tagged.load_from_string(attr->get_as_string()))
			{
				it.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
				includeRegionTypesTagged.push_back(it);
			}
		}
	}

	return result;
}

bool PilgrimageOpenWorldDefinition::CellSet::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(rt, includeRoomTypes)
	{
		result &= rt->element.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	for_every(rt, includeRegionTypes)
	{
		result &= rt->element.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	bool anyRoomTypes = false;

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		available.clear();
		for_every(rt, includeRoomTypes)
		{
			anyRoomTypes = true;
			available.push_back(Available(rt->element.get(), rt->forGameDefinitionTagged));
		}
		for_every(rt, includeRegionTypes)
		{
			available.push_back(Available(rt->element.get(), rt->forGameDefinitionTagged));
		}
		for_every(it, includeRoomTypesTagged)
		{
			for_every_ptr(rt, _library->get_room_types().get_tagged(it->tagged))
			{
				anyRoomTypes = true;
				available.push_back(Available(rt, it->forGameDefinitionTagged));
			}
		}
		for_every(it, includeRegionTypesTagged)
		{
			for_every_ptr(rt, _library->get_region_types().get_tagged(it->tagged))
			{
				available.push_back(Available(rt, it->forGameDefinitionTagged));
			}
		}
		maxAvailableCellPriority = NONE;
		for_every(a, available)
		{
			auto* ls = a->libraryStored.get();
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (Framework::Library::may_ignore_missing_references() && !ls)
			{
				continue;
			}
#endif
			an_assert(ls);
			maxAvailableCellPriority = max(maxAvailableCellPriority, ls->get_custom_parameters().get_value<int>(NAME(cellPriority), 0));
		}
		sort(available);

		if (anyRoomTypes)
		{
			warn(TXT("avoid using room types for open world cells, use regions with embedded room types"));
			AN_BREAK;
		}
	}

	return result;
}

bool PilgrimageOpenWorldDefinition::CellSet::is_defined() const
{
	return ! includeRoomTypes.is_empty() ||
		   ! includeRegionTypes.is_empty() ||
		   ! includeRoomTypesTagged.is_empty() ||
		   ! includeRegionTypesTagged.is_empty();
}

bool PilgrimageOpenWorldDefinition::CellSet::is_empty() const
{
	for_every(a, available)
	{
		if (GameDefinition::check_chosen(a->forGameDefinitionTagged))
		{
			return false;
		}
	}
	return true;
}

//

#define CHECK_MAX_DIST(what) \
	if (ls->get_custom_parameters().get_value<int>(NAME(what), 0) > PilgrimageOpenWorldDefinition::maxDependentDist) \
	{ \
		warn(TXT("\"%S\"'s %S exceeds max dist of %i"), ls->get_name().to_string().to_char(), TXT(#what), PilgrimageOpenWorldDefinition::maxDependentDist); \
	}

PilgrimageOpenWorldDefinition::CellSet::Available::Available(Framework::LibraryStored* ls, TagCondition const& _forGameDefinitionTagged)
: libraryStored(ls)
, forGameDefinitionTagged(_forGameDefinitionTagged)
{
	if (ls)
	{
		probCoef = ls->get_custom_parameters().get_value<float>(NAME(cellProbCoef), probCoef);
		crouch = ls->get_custom_parameters().get_value<bool>(NAME(cellCrouch), crouch);
		anyCrouch = ls->get_custom_parameters().get_value<bool>(NAME(cellAnyCrouch), anyCrouch);
		notFirstChoice = ls->get_custom_parameters().get_value<bool>(NAME(cellNotFirstChoice), notFirstChoice);
		cellPriority = ls->get_custom_parameters().get_value<int>(NAME(cellPriority), cellPriority);

		CHECK_MAX_DIST(maxDependentRadius);
		CHECK_MAX_DIST(maxDependentLeft);
		CHECK_MAX_DIST(maxDependentRight);
		CHECK_MAX_DIST(maxDependentUp);
		CHECK_MAX_DIST(maxDependentDown);
	}
}

//

bool PilgrimageOpenWorldDefinition::HighLevelRegionSet::is_empty() const
{
	for_every(a, available)
	{
		if (GameDefinition::check_chosen(a->forGameDefinitionTagged))
		{
			return false;
		}
	}
	return true;
}

bool PilgrimageOpenWorldDefinition::HighLevelRegionSet::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("include")))
	{
		if (auto* attr = node->get_attribute(TXT("regionType")))
		{
			Framework::UsedLibraryStored<Framework::RegionType> rt;
			if (rt.load_from_xml(attr, _lc))
			{
				Element e;
				e.includeRegionType = rt;
				e.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
				include.push_back(e);
			}
		}
		if (auto* attr = node->get_attribute(TXT("regionTypesTagged")))
		{
			TagCondition tc;
			if (tc.load_from_string(attr->get_as_string()))
			{
				Element e;
				e.includeRegionTypesTagged = tc;
				e.forGameDefinitionTagged.load_from_xml_attribute(node, TXT("forGameDefinitionTagged"));
				include.push_back(e);
			}
		}
	}

	return result;
}

bool PilgrimageOpenWorldDefinition::HighLevelRegionSet::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext,
	Name const& _regionProbCoefVarName, Name const& _subRegionProbCoefVarName, Name const& _subRegionTaggedVarName)
{
	bool result = true;

	for_every(e, include)
	{
		result &= e->includeRegionType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		available.clear();
		for_every(e, include)
		{
			if (e->includeRegionType.is_set())
			{
				available.push_back(Available(e->includeRegionType.get(), e->forGameDefinitionTagged, _library, 0, _regionProbCoefVarName, _subRegionProbCoefVarName, _subRegionTaggedVarName));
			}
			if (!e->includeRegionTypesTagged.is_empty())
			{
				for_every_ptr(rt, _library->get_region_types().get_tagged(e->includeRegionTypesTagged))
				{
					available.push_back(Available(rt, e->forGameDefinitionTagged, _library, 0, _regionProbCoefVarName, _subRegionProbCoefVarName, _subRegionTaggedVarName));
				}
			}
		}
		sort(available);
	}

	return result;
}

//

PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available::~Available()
{
}

PilgrimageOpenWorldDefinition::HighLevelRegionSet::Available::Available(Framework::LibraryStored* ls, TagCondition const & _forGameDefinitionTagged, Framework::Library* _library, int _depth,
	Name const & _regionProbCoefVarName, Name const & _subRegionProbCoefVarName, Name const & _subRegionTaggedVarName
	)
: libraryStored(ls)
, forGameDefinitionTagged(_forGameDefinitionTagged)
{
	if (ls)
	{
		probCoef = ls->get_custom_parameters().get_value<float>(_regionProbCoefVarName, probCoef);
		probCoef = ls->get_custom_parameters().get_value<float>(_subRegionProbCoefVarName, probCoef);
		if (auto* tc = libraryStored->get_custom_parameters().get_existing<TagCondition>(_subRegionTaggedVarName))
		{
			if (_depth < 5)
			{
				highLevelSubRegionSet = new HighLevelRegionSet();
				for_every_ptr(rt, _library->get_region_types().get_tagged(*tc))
				{
					highLevelSubRegionSet->available.push_back(Available(rt, _forGameDefinitionTagged, _library, _depth + 1, _regionProbCoefVarName, _subRegionProbCoefVarName, _subRegionTaggedVarName));
				}
				sort(highLevelSubRegionSet->available);
			}
			else
			{
				error(TXT("too many nested high-level sub regions"));
			}
		}
	}
}

//

bool PilgrimageOpenWorldDefinition::Rule::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	Name typeRead = _node->get_name_attribute(TXT("type"), Name::invalid());
	if (typeRead == TXT("borderDown") ||
		typeRead == TXT("borderNear"))
	{
		type = BorderNear;
	}
	else if (typeRead == TXT("borderUp") ||
			 typeRead == TXT("borderFar"))
	{
		type = BorderFar;
	}
	else if (typeRead == TXT("borderLeft"))
	{
		type = BorderLeft;
	}
	else if (typeRead == TXT("borderRight"))
	{
		type = BorderRight;
	}
	else
	{
		error_loading_xml(_node, TXT("missing valid \"type\" attribute"));
		result = false;
		type = None;
	}

	if (type == BorderNear ||
		type == BorderFar ||
		type == BorderLeft ||
		type == BorderRight)
	{
		depth.load_from_xml(_node, TXT("depth"));
	}

	cellTypes.load_from_xml_attribute_or_child_node(_node, TXT("cellTypes"));

	allowHighLevelRegion = _node->get_bool_attribute_or_from_child_presence(TXT("allowHighLevelRegion"), allowHighLevelRegion);

	return result;
}

bool PilgrimageOpenWorldDefinition::Rule::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

//

bool PilgrimageOpenWorldDefinition::Objective::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);

	notActualObjective = _node->get_bool_attribute_or_from_child_presence(TXT("notActualObjective"), notActualObjective);
	forGameDefinitionTagged.load_from_xml_attribute_or_child_node(_node, TXT("forGameDefinitionTagged"));
	forSkipContentTags.load_from_xml_attribute_or_child_node(_node, TXT("forSkipContentTags"));

	for_every(node, _node->children_named(TXT("forceTowards")))
	{
		forceTowardsDir.load_from_xml(node, TXT("dir"));
	}

	addTransitionRoom = _node->get_bool_attribute_or_from_child_presence(TXT("addTransitionRoom"), addTransitionRoom);

	allowHighLevelRegion = _node->get_bool_attribute_or_from_child_presence(TXT("allowHighLevelRegion"), allowHighLevelRegion);

	pilgrimageDeviceGroupId = _node->get_name_attribute_or_from_child(TXT("pilgrimageDeviceGroupId"), pilgrimageDeviceGroupId);
	requiresToDepletePilgrimageDevice = _node->get_bool_attribute_or_from_child_presence(TXT("requiresToDepletePilgrimageDevice"), requiresToDepletePilgrimageDevice);
	forceRandomPilgrimageDeviceTagged.load_from_xml_attribute_or_child_node(_node, TXT("forceRandomPilgrimageDeviceTagged"));
	
	allowRandomPilgrimageDevices = _node->get_bool_attribute_or_from_child_presence(TXT("allowRandomPilgrimageDevices"), allowRandomPilgrimageDevices);;
	allowRandomPilgrimageDevices = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowRandomPilgrimageDevices"), ! allowRandomPilgrimageDevices);;
	allowDistancedPilgrimageDevices = _node->get_bool_attribute_or_from_child_presence(TXT("allowDistancedPilgrimageDevices"), allowDistancedPilgrimageDevices);;
	allowDistancedPilgrimageDevices = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowDistancedPilgrimageDevices"), !allowDistancedPilgrimageDevices);;

	result &= forceSpecialRoomType.load_from_xml(_node, TXT("forceSpecialRoomType"), _lc);
	for_every(node, _node->children_named(TXT("forceSpecial")))
	{
		result &= forceSpecialRoomType.load_from_xml(node, TXT("roomType"), _lc);
	}	
	regionGenerationTags.load_from_xml_attribute_or_child_node(_node, TXT("regionGenerationTags"));

	result &= lineModel.load_from_xml(_node, TXT("lineModel"), _lc);
	result &= lineModelIfKnown.load_from_xml(_node, TXT("lineModelIfKnown"), _lc);
	lineModelIfKnownDistance = _node->get_float_attribute(TXT("lineModelIfKnownDistance"), lineModelIfKnownDistance);

	atStartAt = _node->get_bool_attribute_or_from_child_presence(TXT("atStartAt"), atStartAt);
	at.load_from_xml(_node, TXT("at"));
	for_every(node, _node->children_named(TXT("at")))
	{
		atDir.load_from_xml(node, TXT("dir"));
	}
	
	repeatOffset.load_from_xml_child_node(_node, TXT("repeatOffset"));
	randomOffset.load_from_xml_child_node(_node, TXT("randomOffset"));
	notInRange.clear();
	for_every(n, _node->children_named(TXT("notInRange")))
	{
		notInRange.push_back();
		notInRange.get_last().load_from_xml(n);
	}
	ignoreIfBeyondRange = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreIfBeyondRange"), ignoreIfBeyondRange);

	beingInCellRequiresNoViolence = _node->get_bool_attribute_or_from_child_presence(TXT("beingInCellRequiresNoViolence"), beingInCellRequiresNoViolence);

	for_every(node, _node->children_named(TXT("skipCompleteOn")))
	{
		skipCompleteOn.reachedX.load_from_xml(node, TXT("reachedX"));
		skipCompleteOn.reachedY.load_from_xml(node, TXT("reachedY"));
	}

	visibilityDistance.load_from_xml(_node, TXT("visibilityDistance"));
	visibilityDistanceCoef.load_from_xml(_node, TXT("visibilityDistanceCoef"));
	for_every(node, _node->children_named(TXT("visibility")))
	{
		visibilityDistance.load_from_xml(node, TXT("distance"));
		visibilityDistanceCoef.load_from_xml(node, TXT("distanceCoef"));
	}

	cellTypes.load_from_xml_attribute_or_child_node(_node, TXT("cellTypes"));
	{
		if (_node->has_attribute(TXT("regionType")))
		{
			regionTypes.grow_size(1);
			result &= regionTypes.get_last().load_from_xml(_node, TXT("regionType"), _lc);
		}
		for_every(n, _node->children_named(TXT("choose")))
		{
			if (n->has_attribute(TXT("regionType")))
			{
				regionTypes.grow_size(1);
				result &= regionTypes.get_last().load_from_xml(n, TXT("regionType"), _lc);
			}
		}
	}
	{
		if (_node->has_attribute(TXT("roomType")))
		{
			roomTypes.grow_size(1);
			result &= roomTypes.get_last().load_from_xml(_node, TXT("roomType"), _lc);
		}
		for_every(n, _node->children_named(TXT("choose")))
		{
			if (n->has_attribute(TXT("roomType")))
			{
				roomTypes.grow_size(1);
				result &= roomTypes.get_last().load_from_xml(n, TXT("roomType"), _lc);
			}
		}
	}
	{
		if (_node->has_attribute(TXT("regionTypesTagged")))
		{
			regionTypesTagged.grow_size(1);
			result &= regionTypesTagged.get_last().load_from_xml_attribute(_node, TXT("regionTypesTagged"));
		}
		for_every(n, _node->children_named(TXT("choose")))
		{
			if (n->has_attribute(TXT("regionTypesTagged")))
			{
				regionTypesTagged.grow_size(1);
				result &= regionTypesTagged.get_last().load_from_xml_attribute(n, TXT("regionTypesTagged"));
			}
		}
	}
	{
		if (_node->has_attribute(TXT("roomTypesTagged")))
		{
			roomTypesTagged.grow_size(1);
			result &= roomTypesTagged.get_last().load_from_xml_attribute(_node, TXT("roomTypesTagged"));
		}
		for_every(n, _node->children_named(TXT("choose")))
		{
			if (n->has_attribute(TXT("roomTypesTagged")))
			{
				roomTypesTagged.grow_size(1);
				result &= roomTypesTagged.get_last().load_from_xml_attribute(n, TXT("roomTypesTagged"));
			}
		}
	}

	error_loading_xml_on_assert(forceRandomPilgrimageDeviceTagged.is_empty() || at.is_set(), result, _node, TXT("\"forceRandomPilgrimageDeviceTagged\" requires \"at\" to be defined"));
	error_loading_xml_on_assert(! lineModel.is_set() || at.is_set(), result, _node, TXT("\"lineModel\" requires \"at\" to be defined"));
	error_loading_xml_on_assert(! lineModelIfKnown.is_set() || at.is_set(), result, _node, TXT("\"lineModelIfKnown\" requires \"at\" to be defined"));

	return result;
}

bool PilgrimageOpenWorldDefinition::Objective::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(regionType, regionTypes)
	{
		result &= regionType->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}
	for_every(roomType, roomTypes)
	{
		result &= roomType->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	for_every(rtt, regionTypesTagged)
	{
		for_every_ptr(rt, _library->get_region_types().get_tagged(*rtt))
		{
			regionTypes.grow_size(1);
			regionTypes.get_last() = rt;
		}
	}
	for_every(rtt, roomTypesTagged)
	{
		for_every_ptr(rt, _library->get_room_types().get_tagged(*rtt))
		{
			roomTypes.grow_size(1);
			roomTypes.get_last() = rt;
		}
	}

	result &= forceSpecialRoomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= lineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= lineModelIfKnown.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

int PilgrimageOpenWorldDefinition::Objective::calculate_visibility_distance(int _veryFarDistance) const
{
	Optional<int> distance;
	if (visibilityDistance.is_set())
	{
		int distanceProp = visibilityDistance.get();
		distance = min(distanceProp, distance.get(distanceProp));
	}
	if (visibilityDistanceCoef.is_set())
	{
		int distanceProp = TypeConversions::Normal::f_i_closest(visibilityDistanceCoef.get() * (float)_veryFarDistance);
		distance = min(distanceProp, distance.get(distanceProp));
	}

	return max(_veryFarDistance, distance.get(_veryFarDistance));
}

bool PilgrimageOpenWorldDefinition::Objective::does_apply_at(VectorInt2 const& _cellAt, PilgrimageInstanceOpenWorld const * _forPilgrimage) const
{
	if (at.is_set() || atStartAt)
	{
		VectorInt2 objAt;
		if (atStartAt)
		{
			objAt = _forPilgrimage->get_start_at();
		}
		else
		{
			an_assert(at.is_set());
			objAt = at.get();
		}
		{
			// we want to move objectiveAt closer to cellAt
			for_count(int, coordIdx, 2)
			{
				int cellAt = coordIdx == 0 ? _cellAt.x : _cellAt.y;
				int& objectiveAt = coordIdx == 0 ? objAt.x : objAt.y;
				int objectiveRepeatOffset = coordIdx == 0 ? repeatOffset.x : repeatOffset.y;
				int halfObjectiveRepeatOffset = objectiveRepeatOffset / 2;
				if (objectiveRepeatOffset != 0)
				{
					// calculate global offset
					int objectiveOffset = objectiveAt - cellAt;
					// now move offset to the closest objective from cell at
					// use half objective repeat offset to get the centre
					int localObjectiveOffset = mod(objectiveOffset + halfObjectiveRepeatOffset, objectiveRepeatOffset) - halfObjectiveRepeatOffset;
					// knowing the offset (cell->local objective) set objective placement there
					objectiveAt = cellAt + localObjectiveOffset;
				}
			}
		}
		if (!randomOffset.is_empty())
		{
			Random::Generator rg = _forPilgrimage->get_seed();
			rg.advance_seed(230957, 9174);
			rg.advance_seed(objAt.x, 24);
			rg.advance_seed(objAt.y, 2354);
			objAt.x += rg.get(randomOffset.x);
			objAt.y += rg.get(randomOffset.y);
		}
		for_every(nir, notInRange)
		{
			if ((nir->x.is_empty() || nir->x.does_contain(objAt.x)) &&
				(nir->y.is_empty() || nir->y.does_contain(objAt.y)))
			{
				return false;
			}
		}
		if (ignoreIfBeyondRange)
		{
			if (auto* owDef = _forPilgrimage->get_pilgrimage()->open_world__get_definition())
			{
				RangeInt2 size = owDef->get_size();

				if ((! size.x.is_empty() && ! size.x.does_contain(objAt.x)) ||
					(! size.y.is_empty() && ! size.y.does_contain(objAt.y)))
				{
					return false;
				}
			}
			else
			{
				error(TXT("open world but not open world"));
				return false;
			}
		}
		if (_cellAt == objAt)
		{
			return true;
		}
	}
	return false;
}

//

