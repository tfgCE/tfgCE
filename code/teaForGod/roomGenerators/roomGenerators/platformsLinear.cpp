#include "platformsLinear.h"

#include "..\roomGenerationInfo.h"

#include "..\..\game\gameSettings.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomGeneratorInfo.inl"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
#ifndef BUILD_PUBLIC_RELEASE
#define OUTPUT_GENERATION_DETAILED
#endif
//#define OUTPUT_GENERATION_DETAILED_EXTENSIVELY
//#define DEBUG_GENERATION
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

DEFINE_STATIC_NAME(in);
DEFINE_STATIC_NAME(out);
DEFINE_STATIC_NAME(left);
DEFINE_STATIC_NAME(right);
DEFINE_STATIC_NAME(laneVector);
DEFINE_STATIC_NAME(platforms);
DEFINE_STATIC_NAME(platform);
DEFINE_STATIC_NAME(platformWidth);
DEFINE_STATIC_NAME(platformLength);
DEFINE_STATIC_NAME(cartWidth);
DEFINE_STATIC_NAME(cartLength);
DEFINE_STATIC_NAME(elevatorStopA);
DEFINE_STATIC_NAME(elevatorStopB);
DEFINE_STATIC_NAME(elevatorStopACartPoint); // cart point relative to elevator stop a
DEFINE_STATIC_NAME(elevatorStopBCartPoint); // cart point relative to elevator stop B
DEFINE_STATIC_NAME(noActiveButtonsForElevatorsWithoutPlayer);

DEFINE_STATIC_NAME(cartHeight);
DEFINE_STATIC_NAME(railingHeight);
DEFINE_STATIC_NAME(cartRailingThickness);
DEFINE_STATIC_NAME(platformRailingThickness);
DEFINE_STATIC_NAME(cartPointThickness); // space required for cart point
DEFINE_STATIC_NAME(cartDoorThickness);
DEFINE_STATIC_NAME(railingPlacement);
DEFINE_STATIC_NAME(railingDir);
DEFINE_STATIC_NAME(railingLeft);
DEFINE_STATIC_NAME(railingRight);
DEFINE_STATIC_NAME(railingLeftCorner); // if we're at the corner
DEFINE_STATIC_NAME(railingRightCorner);

DEFINE_STATIC_NAME(railingNoRailing);
DEFINE_STATIC_NAME(railingLowTransparent);
DEFINE_STATIC_NAME(railingLowSolid);
DEFINE_STATIC_NAME(railingMedSolid);
DEFINE_STATIC_NAME(railingHighSolid);

DEFINE_STATIC_NAME(shareCartMeshes);
DEFINE_STATIC_NAME(shareCartPointMeshes);

DEFINE_STATIC_NAME(useVerticals);
DEFINE_STATIC_NAME(ignoreMinimumCartsSeparation);
DEFINE_STATIC_NAME(platformSeparation);
DEFINE_STATIC_NAME(minDoorPlatformSeparation);
DEFINE_STATIC_NAME(overrideRequestedCellSize);
DEFINE_STATIC_NAME(overrideRequestedCellWidth);
DEFINE_STATIC_NAME(cartLineSeparation);
DEFINE_STATIC_NAME(platformHeight);
DEFINE_STATIC_NAME(maxSteepAngle);

DEFINE_STATIC_NAME(maxPlayAreaSize);

// pois
DEFINE_STATIC_NAME_STR(vrAnchor, TXT("vr anchor"));
DEFINE_STATIC_NAME(vrAnchorId);

// tags
DEFINE_STATIC_NAME(exitOut);
DEFINE_STATIC_NAME(exitIn);
DEFINE_STATIC_NAME(exitLeft);
DEFINE_STATIC_NAME(exitRight);

//

REGISTER_FOR_FAST_CAST(PlatformsLinearInfo);

PlatformsLinearInfo::PlatformsLinearInfo()
{
}

PlatformsLinearInfo::~PlatformsLinearInfo()
{
}

bool PlatformsLinearInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("environmentAnchor")))
	{
		if (node->get_name_attribute_or_from_child(TXT("variableName"), Name::invalid()).is_valid())
		{
			warn_loading_xml(node, TXT("variableName deprecated, use setVariableName"));
		}
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("variableName"), setAnchorVariable);
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("setVariableName"), setAnchorVariable);
		setSpaceOccupiedVariable = node->get_name_attribute_or_from_child(TXT("setSpaceOccupiedVariableName"), setSpaceOccupiedVariable);
		setCentralTopPlatformAtVariable = node->get_name_attribute_or_from_child(TXT("setCentralTopPlatformAtVariableName"), setCentralTopPlatformAtVariable);
		setCentralTopPlatformSizeVariable = node->get_name_attribute_or_from_child(TXT("setCentralTopPlatformSizeVariableName"), setCentralTopPlatformSizeVariable);
		increaseSpaceOccupied.load_from_xml(node, TXT("increaseSpaceOccupied"));
		anchorBelow.load_from_xml(node, TXT("below"));
		anchorAbove.load_from_xml(node, TXT("above"));
	}

	result &= platformSceneryType.load_from_xml(_node, TXT("platformSceneryType"));
	result &= atDoorSceneryType.load_from_xml(_node, TXT("atDoorSceneryType"));

	result &= cartPointSceneryType.load_from_xml(_node, TXT("cartPointSceneryType")); // common
	result &= cartPointVerticalSceneryType.load_from_xml(_node, TXT("cartPointVerticalSceneryType"));
	result &= cartPointHorizontalSceneryType.load_from_xml(_node, TXT("cartPointHorizontalSceneryType"));

	result &= cartSceneryType.load_from_xml(_node, TXT("cartSceneryType")); // common
	result &= cartVerticalSceneryType.load_from_xml(_node, TXT("cartVerticalSceneryType"));
	result &= cartHorizontalSceneryType.load_from_xml(_node, TXT("cartHorizontalSceneryType"));

	result &= cartLaneSceneryType.load_from_xml(_node, TXT("cartLaneSceneryType")); // common
	result &= cartLaneVerticalSceneryType.load_from_xml(_node, TXT("cartLaneVerticalSceneryType"));
	result &= cartLaneHorizontalSceneryType.load_from_xml(_node, TXT("cartLaneHorizontalSceneryType"));

	result &= allowRailingTransparent.load_from_xml(_node, TXT("allowRailingTransparent"));
	result &= railingNoRailing.load_from_xml(_node, TXT("railingNoRailing"));
	result &= railingLowTransparent.load_from_xml(_node, TXT("railingLowTransparent"));
	result &= railingLowSolid.load_from_xml(_node, TXT("railingLowSolid"));
	result &= railingMedSolid.load_from_xml(_node, TXT("railingMedSolid"));
	result &= railingHighSolid.load_from_xml(_node, TXT("railingHighSolid"));

	_lc.load_group_into(platformSceneryType);
	_lc.load_group_into(atDoorSceneryType);
	_lc.load_group_into(cartPointSceneryType);
	_lc.load_group_into(cartPointVerticalSceneryType);
	_lc.load_group_into(cartPointHorizontalSceneryType);
	_lc.load_group_into(cartSceneryType);
	_lc.load_group_into(cartVerticalSceneryType);
	_lc.load_group_into(cartHorizontalSceneryType);
	_lc.load_group_into(cartLaneSceneryType);
	_lc.load_group_into(cartLaneVerticalSceneryType);
	_lc.load_group_into(cartLaneHorizontalSceneryType);
	_lc.load_group_into(railingNoRailing);
	_lc.load_group_into(railingLowTransparent);
	_lc.load_group_into(railingLowSolid);
	_lc.load_group_into(railingMedSolid);
	_lc.load_group_into(railingHighSolid);

	result &= setup.load_from_xml(_node, _lc);
	result &= platformsSetup.load_from_xml(_node, _lc);
	result &= appearanceSetup.load_from_xml(_node, _lc);
	result &= verticalAdjustment.load_from_xml(_node, _lc);

	doorSetupThrough = PlatformsLinearDoorSetupThrough::DoorInRooms;

	for_every(node, _node->children_named(TXT("doorSetupThroughCreatePOIs")))
	{
		doorSetupThrough = PlatformsLinearDoorSetupThrough::CreatePOIs;
		doorSetupThroughCreatePOIs.load_from_xml(node, _lc);
	}

	return result;
}

bool PlatformsLinearInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool PlatformsLinearInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr PlatformsLinearInfo::create_copy() const
{
	PlatformsLinearInfo * copy = new PlatformsLinearInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool PlatformsLinearInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	PlatformsLinear pl(this);
	PlatformsLinearPlatformsSetup plSetup;
#ifdef OUTPUT_GENERATION
	output(TXT("random generator %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif
	result &= pl.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

bool PlatformsLinearSetup::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("setup")))
	{
		shareCartMeshes.load_from_xml(node, TXT("shareCartMeshes"));
		shareCartPointMeshes.load_from_xml(node, TXT("shareCartPointMeshes"));
	}

	return result;
}

void PlatformsLinearSetup::fill_with(SimpleVariableStorage const & _variables)
{
	shareCartMeshes.fill_value_with(_variables);
	shareCartPointMeshes.fill_value_with(_variables);
}

//--

bool PlatformsLinearExtraPlatform::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	if (auto* attr= _node->get_attribute(TXT("at")))
	{
		VectorInt2 a = VectorInt2::zero;
		a.load_from_string(attr->get_as_string());
		at = a;
	}
	else
	{
		at.load_from_xml(_node, TXT("at"));
	}

	matchCartPointLeft.load_from_xml(_node, TXT("matchCartPointLeft"));
	matchCartPointRight.load_from_xml(_node, TXT("matchCartPointRight"));
	matchCartPointForth.load_from_xml(_node, TXT("matchCartPointForth"));
	matchCartPointBack.load_from_xml(_node, TXT("matchCartPointBack"));

	railingsLimitLeft.load_from_xml(_node, TXT("railingsLimitLeft"));
	railingsLimitRight.load_from_xml(_node, TXT("railingsLimitRight"));
	railingsLimitForth.load_from_xml(_node, TXT("railingsLimitForth"));
	railingsLimitBack.load_from_xml(_node, TXT("railingsLimitBack"));

	blockCartSideLeft.load_from_xml(_node, TXT("blockCartSideLeft"));
	blockCartSideRight.load_from_xml(_node, TXT("blockCartSideRight"));
	blockCartSideForth.load_from_xml(_node, TXT("blockCartSideForth"));
	blockCartSideBack.load_from_xml(_node, TXT("blockCartSideBack"));

	for_every(n, _node->children_named(TXT("use")))
	{
		UseOrSpawn u;
		u.name.load_from_xml(n, TXT("meshGenerator"));
		u.atPt.load_from_xml(n, TXT("atPt"));
		u.forwardDir.load_from_xml(n, TXT("forwardDir"));
		u.forwardDir.load_from_xml(n, TXT("forward"));
		u.upDir.load_from_xml(n, TXT("upDir"));
		u.upDir.load_from_xml(n, TXT("up"));
		_lc.load_group_into(u.name);
		useMeshGenerators.push_back(u);
	}
	
	for_every(n, _node->children_named(TXT("spawnScenery")))
	{
		UseOrSpawn u;
		u.name.load_from_xml(n, TXT("type"));
		u.atPt.load_from_xml(n, TXT("atPt"));
		u.forwardDir.load_from_xml(n, TXT("forwardDir"));
		u.forwardDir.load_from_xml(n, TXT("forward"));
		u.upDir.load_from_xml(n, TXT("upDir"));
		u.upDir.load_from_xml(n, TXT("up"));
		u.tags.load_from_xml(n, TXT("tags"));
		_lc.load_group_into(u.name);
		spawnSceneryTypes.push_back(u);
	}
	return result;
}

void PlatformsLinearExtraPlatform::fill_with(SimpleVariableStorage const& _variables)
{
	at.fill_value_with(_variables);

	matchCartPointLeft.fill_value_with(_variables);
	matchCartPointRight.fill_value_with(_variables);
	matchCartPointForth.fill_value_with(_variables);
	matchCartPointBack.fill_value_with(_variables);

	railingsLimitLeft.fill_value_with(_variables);
	railingsLimitRight.fill_value_with(_variables);
	railingsLimitForth.fill_value_with(_variables);
	railingsLimitBack.fill_value_with(_variables);

	blockCartSideLeft.fill_value_with(_variables);
	blockCartSideRight.fill_value_with(_variables);
	blockCartSideForth.fill_value_with(_variables);
	blockCartSideBack.fill_value_with(_variables);

	for_every(u, useMeshGenerators)
	{
		u->name.fill_value_with(_variables);
		u->atPt.fill_value_with(_variables);
		u->forwardDir.fill_value_with(_variables);
		u->upDir.fill_value_with(_variables);
	}
	for_every(u, spawnSceneryTypes)
	{
		u->name.fill_value_with(_variables);
		u->atPt.fill_value_with(_variables);
		u->forwardDir.fill_value_with(_variables);
		u->upDir.fill_value_with(_variables);
	}
}

//--

bool PlatformsLinearPlatformsSetup::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	extraPlatforms.clear();

	for_every(node, _node->children_named(TXT("setup")))
	{
		lines.load_from_xml(node, TXT("lines"));
		width.load_from_xml(node, TXT("width"));
		exitPlatformsCentred.load_from_xml(node, TXT("exitPlatformsCentred"));
		centralPlatformInTheCentre.load_from_xml(node, TXT("centralPlatformInTheCentre"));
		stretchToMatchSizeX.load_from_xml(node, TXT("stretchToMatchSizeX"));
		stretchToMatchSizeY.load_from_xml(node, TXT("stretchToMatchSizeY"));
		allowSimplePath.load_from_xml(node, TXT("allowSimplePath"));
		fullOuterRing.load_from_xml(node, TXT("fullOuterRing"));
		platformChance.load_from_xml(node, TXT("platformChance"));
		connectChance.load_from_xml(node, TXT("connectChance"));
		useVerticals.load_from_xml(node, TXT("useVerticals"));
		chanceOfUsingTwoPlatformsForFourCarts.load_from_xml(node, TXT("chanceOfUsingTwoPlatformsForFourCarts"));
		ignoreMinimumCartsSeparation.load_from_xml(node, TXT("ignoreMinimumCartsSeparation"));
		platformSeparation.load_from_xml(node, TXT("platformSeparation"));
		minDoorPlatformSeparation.load_from_xml(node, TXT("minDoorPlatformSeparation"));
		overrideRequestedCellSize.load_from_xml(node, TXT("overrideRequestedCellSize"));
		overrideRequestedCellWidth.load_from_xml(node, TXT("overrideRequestedCellWidth"));
		cartLineSeparation.load_from_xml(node, TXT("cartLineSeparation"));
		platformHeight.load_from_xml(node, TXT("platformHeight"));
		maxSteepAngle.load_from_xml(node, TXT("maxSteepAngle"));
		maxPlayAreaSize.load_from_xml(node, TXT("maxPlayAreaSize"));
		maxPlayAreaTileSize.load_from_xml(node, TXT("maxPlayAreaTileSize"));
		maxTileSize.load_from_xml(node, TXT("maxTileSize"));

		result &= railingsLimitLeft.load_from_xml(node, TXT("railingsLimitLeft"));
		result &= railingsLimitRight.load_from_xml(node, TXT("railingsLimitRight"));
		result &= railingsLimitForth.load_from_xml(node, TXT("railingsLimitForth"));
		result &= railingsLimitBack.load_from_xml(node, TXT("railingsLimitBack"));
		
		result &= railingsLimitEntrances.load_from_xml(node, TXT("railingsLimitEntrances"));
		
		result &= outerGridRailings.load_from_xml(node, TXT("outerGridRailings"));

		result &= alongYSlopeChance.load_from_xml(node, TXT("alongYSlopeChance"));
		result &= alongXSlopeChance.load_from_xml(node, TXT("alongXSlopeChance"));
		result &= forceLowInTheMiddleChance.load_from_xml(node, TXT("forceLowInTheMiddleChance"));
		result &= forceHighOutsideTheMiddleChance.load_from_xml(node, TXT("forceHighOutsideTheMiddleChance"));
		result &= allHighChance.load_from_xml(node, TXT("allHighChance"));
		result &= noRailingsChance.load_from_xml(node, TXT("noRailingsChance"));

		for_every(epnode, node->children_named(TXT("extraPlatform")))
		{
			PlatformsLinearExtraPlatform ep;
			if (ep.load_from_xml(epnode, _lc))
			{
				extraPlatforms.push_back(ep);
			}
		}
	}

	return result;
}

void PlatformsLinearPlatformsSetup::fill_with(SimpleVariableStorage const & _variables)
{
	lines.fill_value_with(_variables);
	width.fill_value_with(_variables);
	exitPlatformsCentred.fill_value_with(_variables);
	centralPlatformInTheCentre.fill_value_with(_variables);
	stretchToMatchSizeX.fill_value_with(_variables);
	stretchToMatchSizeY.fill_value_with(_variables);
	allowSimplePath.fill_value_with(_variables);
	fullOuterRing.fill_value_with(_variables);
	platformChance.fill_value_with(_variables);
	connectChance.fill_value_with(_variables);
	useVerticals.fill_value_with(_variables);
	chanceOfUsingTwoPlatformsForFourCarts.fill_value_with(_variables);
	ignoreMinimumCartsSeparation.fill_value_with(_variables);
	platformSeparation.fill_value_with(_variables);
	minDoorPlatformSeparation.fill_value_with(_variables);
	overrideRequestedCellSize.fill_value_with(_variables);
	overrideRequestedCellWidth.fill_value_with(_variables);
	cartLineSeparation.fill_value_with(_variables);
	platformHeight.fill_value_with(_variables);
	maxSteepAngle.fill_value_with(_variables);
	maxPlayAreaSize.fill_value_with(_variables);
	maxPlayAreaTileSize.fill_value_with(_variables);
	maxTileSize.fill_value_with(_variables);

	railingsLimitLeft.fill_value_with(_variables);
	railingsLimitRight.fill_value_with(_variables);
	railingsLimitForth.fill_value_with(_variables);
	railingsLimitBack.fill_value_with(_variables);
	railingsLimitEntrances.fill_value_with(_variables);

	for_every(ep, extraPlatforms)
	{
		ep->fill_with(_variables);
	}
}

void PlatformsLinearPlatformsSetup::randomise_missing(Random::Generator const & _random)
{
	random = _random;
	if (!lines.is_value_set())
	{
		lines = RangeInt(random.get_int_from_range(1, 3));
	}
	if (!width.is_value_set())
	{
		width = RangeInt(random.get_int_from_range(2, 5));
	}
	if (!exitPlatformsCentred.is_value_set())
	{
		exitPlatformsCentred = false;
	}
	if (!centralPlatformInTheCentre.is_value_set())
	{
		centralPlatformInTheCentre = false;
	}
	if (!stretchToMatchSizeX.is_value_set())
	{
		stretchToMatchSizeX = false;
	}
	if (!stretchToMatchSizeY.is_value_set())
	{
		stretchToMatchSizeY = true;
	}
	if (!fullOuterRing.is_value_set())
	{
		fullOuterRing = false;
	}
	if (!allowSimplePath.is_value_set())
	{
		allowSimplePath = ! fullOuterRing.get();
	}
	if (!platformChance.is_value_set())
	{
		platformChance = 0.9f;
	}
	if (!connectChance.is_value_set())
	{
		connectChance = 0.8f;
	}
	if (!useVerticals.is_value_set())
	{
		useVerticals = random.get_float(0.1f, 0.2f);
	}
	if (!chanceOfUsingTwoPlatformsForFourCarts.is_value_set())
	{
		chanceOfUsingTwoPlatformsForFourCarts = 0.4f;
	}
	if (!ignoreMinimumCartsSeparation.is_value_set())
	{
		ignoreMinimumCartsSeparation = false;
	}
	if (!platformSeparation.is_value_set())
	{
		Vector3 ps;
		ps.x = random.get_float(0.5f, 3.0f);
		ps.y = ps.x;
		ps.z = random.get_float(0.25f, 2.0f);
		platformSeparation = ps;
	}
	if (!minDoorPlatformSeparation.is_value_set())
	{
		minDoorPlatformSeparation = random.get_float(0.5f, 3.0f);
	}
	if (!cartLineSeparation.is_value_set())
	{
		float v = random.get_float(0.25f, 2.0f);
		float len = random.get_float(0.25f, 3.0f);
		cartLineSeparation = Range::of_length(v, len);
	}
	if (!platformHeight.is_value_set())
	{
		platformHeight = random.get_float(2.3f, 3.0f);
	}
	if (!maxSteepAngle.is_value_set())
	{
		maxSteepAngle = random.get_float(20.0f, 60.0f);
	}
}

void PlatformsLinearPlatformsSetup::log(LogInfoContext & _context) const
{
	_context.log(TXT("platforms linear setup"));
	LOG_INDENT(_context);
	_context.log(TXT("random %S"), random.get_seed_string().to_char());
	_context.log(TXT("lines %S"), lines.get().to_string().to_char());
	_context.log(TXT("width %S"), width.get().to_string().to_char());
	_context.log(TXT("exitPlatformsCentred %S"), exitPlatformsCentred.get()? TXT("true") : TXT("false"));
	_context.log(TXT("centralPlatformInTheCentre %S"), centralPlatformInTheCentre.get()? TXT("true") : TXT("false"));
	_context.log(TXT("stretchToMatchSizeX %S"), stretchToMatchSizeX.get()? TXT("true") : TXT("false"));
	_context.log(TXT("stretchToMatchSizeY %S"), stretchToMatchSizeY.get()? TXT("true") : TXT("false"));
	_context.log(TXT("allowSimplePath %S"), allowSimplePath.get()? TXT("true") : TXT("false"));
	_context.log(TXT("fullOuterRing %S"), fullOuterRing.get()? TXT("true") : TXT("false"));
	_context.log(TXT("platformChance %.3f%%"), platformChance.get() * 100.0f);
	_context.log(TXT("connectChance %.3f%%"), connectChance.get() * 100.0f);
	_context.log(TXT("useVerticals %.3f"), useVerticals.get());
	_context.log(TXT("chanceOfUsingTwoPlatformsForFourCarts %.3f"), chanceOfUsingTwoPlatformsForFourCarts.get());
	_context.log(TXT("ignoreMinimumCartsSeparation %S"), ignoreMinimumCartsSeparation.get()? TXT("true") : TXT("false"));
	_context.log(TXT("platformSeparation %S"), platformSeparation.get().to_string().to_char());
	_context.log(TXT("minDoorPlatformSeparation %.3f"), minDoorPlatformSeparation.get());
	_context.log(TXT("overrideRequestedCellSize %.3f"), overrideRequestedCellSize.is_set()? overrideRequestedCellSize.get() : 0.0f);
	_context.log(TXT("overrideRequestedCellWidth %.3f"), overrideRequestedCellWidth.is_set()? overrideRequestedCellWidth.get() : 0.0f);
	_context.log(TXT("cartLineSeparation %S"), cartLineSeparation.get().to_string().to_char());
	_context.log(TXT("platformHeight %.3f"), platformHeight.get());
	_context.log(TXT("maxSteepAngle %.3f"), maxSteepAngle.get());
	_context.log(TXT("maxPlayAreaSize %S"), maxPlayAreaSize.get().to_string().to_char());
	_context.log(TXT("maxPlayAreaTileSize %S"), maxPlayAreaTileSize.get().to_string().to_char());
	_context.log(TXT("maxTileSize %.3f"), maxTileSize.get());
}

//--

bool PlatformsLinearAppearanceSetup::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("appearance")))
	{
		cartHeight.load_from_xml(node, TXT("cartHeight"));
		railingHeight.load_from_xml(node, TXT("railingHeight"));
		cartRailingThickness.load_from_xml(node, TXT("cartRailingThickness"));
		platformRailingThickness.load_from_xml(node, TXT("platformRailingThickness"));
		cartPointThickness.load_from_xml(node, TXT("cartPointThickness"));
		cartDoorThickness.load_from_xml(node, TXT("cartDoorThickness"));
	}

	return result;
}

void PlatformsLinearAppearanceSetup::fill_with(SimpleVariableStorage const & _variables)
{
	cartHeight.fill_value_with(_variables);
	railingHeight.fill_value_with(_variables);
	cartRailingThickness.fill_value_with(_variables);
	platformRailingThickness.fill_value_with(_variables);
	cartPointThickness.fill_value_with(_variables);
	cartDoorThickness.fill_value_with(_variables);
}

void PlatformsLinearAppearanceSetup::setup_parameters(SimpleVariableStorage & _parameters) const
{
	_parameters.access<float>(NAME(cartHeight)) = cartHeight.get() * Framework::Door::get_nominal_door_height_scale();
	_parameters.access<float>(NAME(railingHeight)) = railingHeight.get() * Framework::Door::get_nominal_door_height_scale();
	_parameters.access<float>(NAME(cartRailingThickness)) = cartRailingThickness.get();
	_parameters.access<float>(NAME(platformRailingThickness)) = platformRailingThickness.get();
	_parameters.access<float>(NAME(cartPointThickness)) = cartPointThickness.get();
	_parameters.access<float>(NAME(cartDoorThickness)) = cartDoorThickness.get();
}

void PlatformsLinearAppearanceSetup::setup_variables(Framework::Scenery* _scenery) const
{
	setup_parameters(_scenery->access_variables());
}

//--

bool PlatformsLinearVerticalAdjustment::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("verticalAdjustment")))
	{
		xA.load_from_xml(node, TXT("xA"));
		xB.load_from_xml(node, TXT("xB"));
		yA.load_from_xml(node, TXT("yA"));
		yB.load_from_xml(node, TXT("yB"));
		hLimit.load_from_xml(node, TXT("hLimit"));
		hPt.load_from_xml(node, TXT("hPt"));
	}

	return result;
}

void PlatformsLinearVerticalAdjustment::fill_with(SimpleVariableStorage const & _variables)
{
	xA.fill_value_with(_variables);
	xB.fill_value_with(_variables);
	yA.fill_value_with(_variables);
	yB.fill_value_with(_variables);
	hLimit.fill_value_with(_variables);
	hPt.fill_value_with(_variables);
}

void PlatformsLinearVerticalAdjustment::log(LogInfoContext & _context)
{
	_context.log(TXT("vertical adjustment params"));
	LOG_INDENT(_context);
	String result;
	if (xA.get().centre() != 0.0f)
	{
		result += String::printf(TXT(" %c%.3fx^"), sign(xA.get().centre())> 0.0f ? '+' : '-', abs(xA.get().centre()));
	}
	if (xB.get().centre() != 0.0f)
	{
		result += String::printf(TXT(" %c%.3fx"), sign(xB.get().centre()) > 0.0f ? '+' : '-', abs(xB.get().centre()));
	}
	if (yA.get().centre() != 0.0f)
	{
		result += String::printf(TXT(" %c%.3fy^"), sign(yA.get().centre())> 0.0f ? '+' : '-', abs(yA.get().centre()));
	}
	if (yB.get().centre() != 0.0f)
	{
		result += String::printf(TXT(" %c%.3fy"), sign(yB.get().centre())> 0.0f ? '+' : '-', abs(yB.get().centre()));
	}
	_context.log(TXT("adjust : %S"), result.to_char());
	if (hLimit.is_set())
	{
		_context.log(TXT("hLimit : %S"), hLimit.get().to_string().to_char());
	}
	if (hPt.is_set())
	{
		_context.log(TXT("hPt : %.3f"), hPt.get().centre());
	}
}

void PlatformsLinearVerticalAdjustment::choose(Random::Generator const & _random)
{
	Random::Generator random = _random;
	xA = Range(random.get(xA.get()));
	xB = Range(random.get(xB.get()));
	yA = Range(random.get(yA.get()));
	yB = Range(random.get(yB.get()));
	if (hPt.is_set())
	{
		hPt = Range(random.get(hPt.get()));
	}
}

//--

bool PlatformsLinearDoorSetupThroughCreatePOIs::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	inCount.load_from_xml(_node, TXT("inCount"));
	outCount.load_from_xml(_node, TXT("outCount"));
	leftCount.load_from_xml(_node, TXT("leftCount"));
	rightCount.load_from_xml(_node, TXT("rightCount"));
	inPOIName.load_from_xml(_node, TXT("inPOIName"));
	outPOIName.load_from_xml(_node, TXT("outPOIName"));
	leftPOIName.load_from_xml(_node, TXT("leftPOIName"));
	rightPOIName.load_from_xml(_node, TXT("rightPOIName"));

	return result;
}

void PlatformsLinearDoorSetupThroughCreatePOIs::fill_with(SimpleVariableStorage const& _variables)
{
	inCount.fill_value_with(_variables);
	outCount.fill_value_with(_variables);
	leftCount.fill_value_with(_variables);
	rightCount.fill_value_with(_variables);
	inPOIName.fill_value_with(_variables);
	outPOIName.fill_value_with(_variables);
	leftPOIName.fill_value_with(_variables);
	rightPOIName.fill_value_with(_variables);
}

//


//--

#ifdef AN_DEVELOPMENT_OR_PROFILER
tchar PlatformsLinear::CartPoint::get_dir_as_char() const
{
	return PlatformsLinear::get_dir_as_char(axis, dir);
}
#endif

void PlatformsLinear::CartPoint::set(PlatformDir _pd)
{
	if (_pd == PD_Left || _pd == PD_Right)
	{
		axis = CA_LeftRight;
		dir = _pd == PD_Left ? -1 : 1;
	}
	else if (_pd == PD_Back || _pd == PD_Forth)
	{
		axis = CA_BackForth;
		dir = _pd == PD_Back ? -1 : 1;
	}
	else if (_pd == PD_DownX || _pd == PD_UpX)
	{
		axis = CA_VerticalX;
		dir = _pd == PD_DownX ? -1 : 1;
	}
	else if (_pd == PD_DownY || _pd == PD_UpY)
	{
		axis = CA_VerticalY;
		dir = _pd == PD_DownY ? -1 : 1;
	}
	else
	{
		todo_important(TXT("implement_"));
	}
}

VectorInt2 PlatformsLinear::CartPoint::get_entry_tile_placement() const
{
	VectorInt2 entry = tilePlacement;
	if (axis == CA_BackForth || axis == CA_VerticalX)
	{
		an_assert(side != 0);
		entry.x += side < 0 ? 1 : -1;
	}
	else if (axis == CA_LeftRight || axis == CA_VerticalY)
	{
		an_assert(side != 0);
		entry.y += side < 0 ? 1 : -1;
	}
	else
	{
		todo_important(TXT("implement_"));
	}
	return entry;
}

RangeInt2 PlatformsLinear::CartPoint::get_entry_tiles(PlatformsLinear const * _generator) const
{
	RangeInt2 entry = RangeInt2::empty;
	RoomGenerationInfo const & rggi = _generator->get_room_generation_info();
	if (axis == CA_BackForth || axis == CA_VerticalX)
	{
		entry.y.min = 0;
		entry.y.max = rggi.get_tile_count().y - 1;
		an_assert(side != 0);
		if (side < 0)
		{
			entry.x.min = tilePlacement.x + 1;
			entry.x.max = rggi.get_tile_count().x - 1;
		}
		else
		{
			entry.x.min = 0;
			entry.x.max = tilePlacement.x - 1;
		}
	}
	else if (axis == CA_LeftRight || axis == CA_VerticalY)
	{
		entry.x.min = 0;
		entry.x.max = rggi.get_tile_count().x - 1;
		an_assert(side != 0);
		if (side < 0)
		{
			entry.y.min = tilePlacement.y + 1;
			entry.y.max = rggi.get_tile_count().y - 1;
		}
		else
		{
			entry.y.min = 0;
			entry.y.max = tilePlacement.y - 1;
		}
	}
	else
	{
		todo_important(TXT("implement_"));
	}
	return entry;
}

void PlatformsLinear::CartPoint::calculate_placement(PlatformsLinear const * _generator, PlatformsLinear::Platform const * _platform)
{
	RoomGenerationInfo const& rggi = _generator->get_room_generation_info();
	Vector3 at;
	at.x = rggi.get_first_tile_centre_offset().x + (float)tilePlacement.x * rggi.get_tile_size();
	at.y = rggi.get_first_tile_centre_offset().y + (float)tilePlacement.y * rggi.get_tile_size();
	at.z = 0.0f;
	at -= _platform->get_centre_offset();
	Vector3 dir = Vector3::zero;
	if (axis == CA_BackForth || axis == CA_VerticalX)
	{
		dir = side < 0 ? Vector3::xAxis : -Vector3::xAxis;
	}
	else if (axis == CA_LeftRight || axis == CA_VerticalY)
	{
		dir = side < 0 ? Vector3::yAxis : -Vector3::yAxis;
	}
	else
	{
		todo_important(TXT("implement_"));
	}
	placement = look_at_matrix(at, at + dir, Vector3::zAxis).to_transform();
}

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
Array<String> const PlatformsLinear::Platform::get_layout(PlatformsLinear const * _generator) const
{
	RoomGenerationInfo const & rggi = _generator->get_room_generation_info();
	Array<String> lines;
	String line;
	for_count(int, i, rggi.get_tile_count().x)
	{
		line += TXT(" .  ");
	}
	for_count(int, i, rggi.get_tile_count().y)
	{
		lines.push_back(line);
	}
	for_every(cp, cartPoints)
	{
		int y = rggi.get_tile_count().y - 1 - cp->tilePlacement.y;
		lines[y][cp->tilePlacement.x * 4 + 1] = cp->get_dir_as_char();
		lines[y][cp->tilePlacement.x * 4 + 2] = '0' + for_everys_index(cp);
		lines[y][cp->tilePlacement.x * 4 + 3] = cp->side > 0? '+' : '-';
		if (cp->cartIdx != NONE)
		{
			lines[y][cp->tilePlacement.x * 4] = '*';
		}
	}
	return lines;
}
#endif

Vector3 PlatformsLinear::Platform::calculate_half_size(float _platformHeight) const
{
	Vector3 const height(0.0f, 0.0f, _platformHeight);
	return (platform.length().to_vector3() + height) * 0.5f;
}

bool PlatformsLinear::Platform::can_have_another_vertical(int _dir) const
{
	for_every(cp, cartPoints)
	{
		if (is_vertical(cp->axis) &&
			cp->dir == _dir)
		{
			// we already have one going in that direction
			return false;
		}
	}
	return true;
}

bool PlatformsLinear::Platform::has_multiple_in_one_dir(CartPoint const * _cps, int _count) const
{
	int left = 0;
	int right = 0;
	int back = 0;
	int forth = 0;
	int down = 0;
	int up = 0;
	auto cp = _cps;
	for_count(int, i, _count)
	{
		if (is_vertical(cp->axis))
		{
			if (cp->dir < 0)
			{
				++down;
			}
			else
			{
				++up;
			}
		}
		if (cp->axis == CA_BackForth)
		{
			if (cp->dir < 0)
			{
				++back;
			}
			else
			{
				++forth;
			}
		}
		if (cp->axis == CA_LeftRight)
		{
			if (cp->dir < 0)
			{
				++left;
			}
			else
			{
				++right;
			}
		}
		++cp;
	}
	return down >= 2 || up >= 2 ||
		   left >= 2 || right >= 2 ||
		   back >= 2 || forth >= 2;
}

bool PlatformsLinear::Platform::has_multiple_in_one_dir() const
{
	return has_multiple_in_one_dir(cartPoints.begin(), cartPoints.get_size());
}

bool PlatformsLinear::Platform::can_add_to_have_unique_dirs(CartPoint const & _cp) const
{
	ARRAY_STACK(CartPoint, newCartPoints, cartPoints.get_size() + 1);
	newCartPoints = cartPoints;
	newCartPoints.push_back(_cp);
	return ! has_multiple_in_one_dir(newCartPoints.begin(), newCartPoints.get_size());
}

PlatformsLinear::CartPoint * PlatformsLinear::Platform::find_cart_point_unconnected(CartAxis _axis, int _dir)
{
	for_every(cp, cartPoints)
	{
		if (cp->cartIdx == NONE &&
			cp->axis == _axis &&
			cp->dir == _dir)
		{
			return cp;
		}
	}
	return nullptr;
}

void PlatformsLinear::Platform::calculate_platform(PlatformsLinear const * _generator, Random::Generator & _random)
{
	RoomGenerationInfo const & rggi = _generator->get_room_generation_info();
	RangeInt2 tiles = RangeInt2::empty;
	todo_note(TXT("add random sizes"));
	// random chance to use something bigger
	if (_random.get_chance(0.9f)) magic_number
	{
		tiles.x.min = 0;
		tiles.x.max = rggi.get_tile_count().x - 1;
	}
	else if (_random.get_chance(0.8f)) magic_number
	{
		tiles.x.min = _random.get_int_from_range(0, rggi.get_tile_count().x / 2);
		tiles.x.max = _random.get_int_from_range(rggi.get_tile_count().x / 2, rggi.get_tile_count().x - 1);
	}
	if (_random.get_chance(0.9f)) magic_number
	{
		tiles.y.min = 0;
		tiles.y.max = rggi.get_tile_count().y - 1;
	}
	else if (_random.get_chance(0.8f)) magic_number
	{
		tiles.y.min = _random.get_int_from_range(0, rggi.get_tile_count().y / 2);
		tiles.y.max = _random.get_int_from_range(rggi.get_tile_count().y / 2, rggi.get_tile_count().y - 1);
	}
	// find minimal size (we will cut sides/boundaries in next step)
	for_every(cartPoint, cartPoints)
	{
		tiles.x.include(cartPoint->tilePlacement.x);
		tiles.y.include(cartPoint->tilePlacement.y);
		// and add point at the platform too
		if (cartPoint->axis == CA_BackForth || cartPoint->axis == CA_VerticalX)
		{
			tiles.x.include(cartPoint->tilePlacement.x - sign(cartPoint->side));
		}
		else if (cartPoint->axis == CA_LeftRight || cartPoint->axis == CA_VerticalY)
		{
			tiles.y.include(cartPoint->tilePlacement.y - sign(cartPoint->side));
		}
	}
	// undefined size?
	if (tiles.x.is_empty())
	{
		tiles.x.min = 0;
		tiles.x.max = rggi.get_tile_count().x - 1;
	}
	if (tiles.y.is_empty())
	{
		tiles.y.min = 0;
		tiles.y.max = rggi.get_tile_count().y - 1;
	}
	// cut what is way to big
	for_every(cartPoint, cartPoints)
	{
		if (cartPoint->axis == CA_BackForth || cartPoint->axis == CA_VerticalX)
		{
			if (cartPoint->side < 0)
			{
				tiles.x.min = max(tiles.x.min, cartPoint->tilePlacement.x + 1);
			}
			if (cartPoint->side > 0)
			{
				tiles.x.max = min(tiles.x.max, cartPoint->tilePlacement.x - 1);
			}
		}
		if (cartPoint->axis == CA_LeftRight || cartPoint->axis == CA_VerticalY)
		{
			if (cartPoint->side < 0)
			{
				tiles.y.min = max(tiles.y.min, cartPoint->tilePlacement.y + 1);
			}
			if (cartPoint->side > 0)
			{
				tiles.y.max = min(tiles.y.max, cartPoint->tilePlacement.y - 1);
			}
		}
	}
	// in and out doors require at least one tile from the edge
	todo_note(TXT("in and out aligned to y axis"));
	if (type == PT_In)
	{
		tiles.y.min = max(1, tiles.y.min);
		tiles.y.max = max(1, tiles.y.max);
	}
	if (type == PT_Out)
	{
		tiles.y.min = min(rggi.get_tile_count().y - 2, tiles.y.min);
		tiles.y.max = min(rggi.get_tile_count().y - 2, tiles.y.max);
	}
	if (type == PT_Left)
	{
		tiles.x.min = max(1, tiles.x.min);
		tiles.x.max = max(1, tiles.x.max);
	}
	if (type == PT_Right)
	{
		tiles.x.min = min(rggi.get_tile_count().x - 2, tiles.x.min);
		tiles.x.max = min(rggi.get_tile_count().x - 2, tiles.x.max);
	}
	platform.x.min = rggi.get_tiles_zone_offset().x + (float)tiles.x.min * rggi.get_tile_size();
	platform.x.max = rggi.get_tiles_zone_offset().x + (float)(tiles.x.max + 1) * rggi.get_tile_size();
	platform.y.min = rggi.get_tiles_zone_offset().y + (float)tiles.y.min * rggi.get_tile_size();
	platform.y.max = rggi.get_tiles_zone_offset().y + (float)(tiles.y.max + 1) * rggi.get_tile_size();
}

//

bool PlatformsLinear::KeyPointInfo::is_empty()
{
	return left.platformIdx == NONE &&
		   right.platformIdx == NONE &&
		   back.platformIdx == NONE &&
		   forth.platformIdx == NONE;
}

void PlatformsLinear::KeyPointInfo::set(PlatformDir _dir, int _platformIdx, int _cartPointIdx)
{
	if (_dir == PD_Left) { left.platformIdx = _platformIdx; left.cartPointIdx = _cartPointIdx; } else
	if (_dir == PD_Right) { right.platformIdx = _platformIdx; right.cartPointIdx = _cartPointIdx; } else
	if (_dir == PD_Back) { back.platformIdx = _platformIdx; back.cartPointIdx = _cartPointIdx; } else
	if (_dir == PD_Forth) { forth.platformIdx = _platformIdx; forth.cartPointIdx = _cartPointIdx; } else
	{
		todo_important(TXT("implement_"));
	}
}

bool PlatformsLinear::KeyPointInfo::remove(Random::Generator & _random, OUT_ PlatformDir & _dir, OUT_ ElevatorCartPointRef & _ref)
{
	ArrayStatic<PlatformDir, 6> dirs; SET_EXTRA_DEBUG_INFO(dirs, TXT("PlatformsLinear.dirs (remove)"));
	ArrayStatic<ElevatorCartPointRef, 6> refs; SET_EXTRA_DEBUG_INFO(refs, TXT("PlatformsLinear.refs"));
	if (left.platformIdx != NONE) { refs.push_back(left); dirs.push_back(PD_Left); }
	if (right.platformIdx != NONE) { refs.push_back(right); dirs.push_back(PD_Right); }
	if (back.platformIdx != NONE) { refs.push_back(back); dirs.push_back(PD_Back); }
	if (forth.platformIdx != NONE) { refs.push_back(forth); dirs.push_back(PD_Forth); }
	if (dirs.is_empty())
	{
		an_assert(TXT("why?"));
		return false;
	}

	int idx = _random.get_int(dirs.get_size());
	_dir = dirs[idx];
	_ref = refs[idx];

	if (_dir == PD_Left) left = ElevatorCartPointRef(); else
	if (_dir == PD_Right) right = ElevatorCartPointRef(); else
	if (_dir == PD_Back) back = ElevatorCartPointRef(); else
	if (_dir == PD_Forth) forth = ElevatorCartPointRef(); else
	{
		todo_important(TXT("implement_"));
	}

	return true;
}

//

Vector3 PlatformsLinear::Cart::calculate_lane_vector(PlatformsLinear * _platformsLinear, Array<Framework::Scenery*> const & _platformSceneries, OUT_ bool & _reversePlatform) const
{
	_reversePlatform = false;
	Vector3 laneVector;
	auto const * plusPlatformScenery = _platformSceneries[plus.platformIdx];
	auto const * minusPlatformScenery = _platformSceneries[minus.platformIdx];
	auto plusCartPoint = _platformsLinear->find_cart_point(plus);
	auto minusCartPoint = _platformsLinear->find_cart_point(minus);
	laneVector = plusPlatformScenery->get_presence()->get_placement().location_to_world(plusCartPoint->placement.get_translation()) - minusPlatformScenery->get_presence()->get_placement().location_to_world(minusCartPoint->placement.get_translation());
	if (_platformsLinear->is_vertical(axis))
	{
		if (laneVector.z < 0.0f)
		{
			laneVector = -laneVector;
			_reversePlatform = !_reversePlatform;
		}
	}
	else
	{
		if (axis == CA_LeftRight)
		{
			laneVector = -laneVector;
			laneVector.z = -laneVector.z;
		}
		if (plusCartPoint->side < 0)
		{
			_reversePlatform = !_reversePlatform;
			laneVector.z = -laneVector.z;
		}
		if ((laneVector.x < 0.0f && axis == CA_LeftRight) ||
			(laneVector.y < 0.0f && axis == CA_BackForth))
		{
			laneVector = -laneVector;
			_reversePlatform = !_reversePlatform;
		}
		if (axis == CA_BackForth)
		{
			swap(laneVector.x, laneVector.y);
		}
		an_assert(laneVector.x != 0.0f);
	}

	return laneVector;
}

//

PlatformsLinear::PlatformsLinear(PlatformsLinearInfo const * _info)
: info(_info)
{
	random = Random::Generator(1234, 1245);
}

PlatformsLinear::~PlatformsLinear()
{
}

void PlatformsLinear::clear()
{
	platforms.clear();
	carts.clear();

	// virtual
	platformsIn.clear();
	platformsOut.clear();
	platformsLeft.clear();
	platformsRight.clear();
	platformsExtra.clear();

	inDoors.clear();
	outDoors.clear();
	leftDoors.clear();
	rightDoors.clear();
}

bool PlatformsLinear::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;

	_room->collect_variables(roomVariables);
	info->apply_generation_parameters_to(roomVariables);

	setup = info->setup;
	platformsSetup = info->platformsSetup;
	appearanceSetup = info->appearanceSetup;
	verticalAdjustment = info->verticalAdjustment;
	doorSetupThroughCreatePOIs = info->doorSetupThroughCreatePOIs;

	setup.fill_with(roomVariables);
	platformsSetup.fill_with(roomVariables);
	appearanceSetup.fill_with(roomVariables);
	verticalAdjustment.fill_with(roomVariables);
	doorSetupThroughCreatePOIs.fill_with(roomVariables);

	doorSetupThrough = info->doorSetupThrough;
		
	platformsSetup.randomise_missing(_room->get_individual_random_generator()); // first randomise

	// other missing
	platformsSetup.maxPlayAreaTileSize.set_if_not_set(Vector2::zero);
	platformsSetup.maxPlayAreaSize.set_if_not_set(Vector2::zero);

	roomGenerationInfo = RoomGenerationInfo::get();
	roomGenerationInfo.modify_randomly(platformsSetup.random);

	platformsSetup.maxTileSize.set_if_not_set(roomGenerationInfo.get_tile_size());
	platformsSetup.maxTileSize = min(platformsSetup.maxTileSize.get(), roomGenerationInfo.get_tile_size());

	if (!platformsSetup.maxPlayAreaTileSize.get().is_zero())
	{
		platformsSetup.maxPlayAreaSize.access().x = max(1.0f, platformsSetup.maxPlayAreaTileSize.get().x) * platformsSetup.maxTileSize.get();
		platformsSetup.maxPlayAreaSize.access().y = max(1.0f, platformsSetup.maxPlayAreaTileSize.get().y) * platformsSetup.maxTileSize.get();
		// will trigger next if by default
	}
	if (!platformsSetup.maxPlayAreaSize.get().is_zero())
	{
		platformsSetup.maxPlayAreaSize.access().x = max(platformsSetup.maxPlayAreaSize.get().x, 3.0f * platformsSetup.maxTileSize.get());
		platformsSetup.maxPlayAreaSize.access().y = max(platformsSetup.maxPlayAreaSize.get().y, 2.0f * platformsSetup.maxTileSize.get());
		roomGenerationInfo.limit_to(platformsSetup.maxPlayAreaSize.get(), platformsSetup.maxTileSize.get(), platformsSetup.random);
		an_assert(roomGenerationInfo.get_tile_count().x >= 3 && roomGenerationInfo.get_tile_count().y >= 2);
	}
	platformsSetup.platformSeparation.access().y = platformsSetup.platformSeparation.get().x;

	verticalAdjustment.choose(platformsSetup.random);

	random = platformsSetup.random;

	RoomGenerationInfo const & rggi = get_room_generation_info();

	if (rggi.is_small())
	{
		platformsSetup.useVerticals = 0.65f * platformsSetup.useVerticals.get(); // for small it becomes quite silly with too many verticals
	}

	int triesLeft = 20;

	while (triesLeft--)
	{
		clear();

		collect_doors(_room);

		place_doors_in_local_space_initially();

		create_door_platforms();

		create_platforms_and_connect();

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
		for_every(platform, platforms)
		{
			output(TXT("platform %i"), for_everys_index(platform));
			for_every(l, platform->get_layout(this))
			{
				output(TXT("  %S"), l->to_char());
			}
		}
#endif

		place_platforms();

		move_platforms(_room);

		if (adjust_platforms_vertically())
		{
			move_platforms(_room, true);
		}

		all_platforms_centred_xy();

		all_platforms_above_zero();

		// alright, this is not so nice but I could either try to find solution to not so obvious and simple cases
		// (sometimes learning too late that something is wrong, especially when mixing lots of verticals)
		// or do it like this: if there are huge messups, do it again
		if (check_if_valid())
		{
			break;
		}

#ifdef OUTPUT_GENERATION
		output(TXT("generate again!"));
#endif
	}

	// decide upon railings strategy and setup railings limits
	choose_railings_strategy_and_setup_limits();

	result &= put_everything_into_room(_game, _room, REF_ _roomGeneratingContext);

	if (info->setAnchorVariable.is_valid() ||
		info->setSpaceOccupiedVariable.is_valid() ||
		info->setCentralTopPlatformAtVariable.is_valid())
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			// tags are getting removed, we redo this
			piow->tag_open_world_directions_for_cell(_room);

			Range3 spaceOccupied = Range3::empty;
			Range doorSizeZ = calculate_door_size(_room);
			{
				Range3 platformsSpaceOccupied = spaceOccupied;
				for_every(platform, platforms)
				{
					Vector3 at = platform->get_room_placement();
					platformsSpaceOccupied.include(at);
					Vector3 halfSize = platform->calculate_half_size(0.0f);
					platformsSpaceOccupied.include(at + Vector3(-halfSize.x, -halfSize.y, doorSizeZ.min));
					platformsSpaceOccupied.include(at + Vector3( halfSize.x,  halfSize.y, doorSizeZ.max));
				}

				spaceOccupied = platformsSpaceOccupied;

				if (info->increaseSpaceOccupied.is_set())
				{
					Vector3 increaseSpaceOccupied = info->increaseSpaceOccupied.get(roomVariables, Vector3::zero);
					platformsSpaceOccupied.z.expand_by(increaseSpaceOccupied.z * 0.5f);
					if (platformsIn.is_empty())
					{
						platformsSpaceOccupied.y.min -= increaseSpaceOccupied.y * 0.5f;
					}
					if (platformsOut.is_empty())
					{
						platformsSpaceOccupied.y.max += increaseSpaceOccupied.y * 0.5f;
					}
					if (platformsLeft.is_empty())
					{
						platformsSpaceOccupied.x.min -= increaseSpaceOccupied.x * 0.5f;
					}
					if (platformsRight.is_empty())
					{
						platformsSpaceOccupied.x.max += increaseSpaceOccupied.x * 0.5f;
					}
				}

				if (info->setSpaceOccupiedVariable.is_valid())
				{
					_room->access_variables().access<Range3>(info->setSpaceOccupiedVariable) = platformsSpaceOccupied;
				}
			}
			if (info->setCentralTopPlatformAtVariable.is_valid() ||
				info->setCentralTopPlatformSizeVariable.is_valid())
			{
				Vector3 bestAt = Vector3::zero;
				Vector3 bestSize = Vector3::zero;
				find_central_top_platform(_room, OUT_ &bestAt, OUT_ &bestSize);
				if (info->setCentralTopPlatformAtVariable.is_valid())
				{
					_room->access_variables().access<Vector3>(info->setCentralTopPlatformAtVariable) = bestAt;
				}
				if (info->setCentralTopPlatformSizeVariable.is_valid())
				{
					_room->access_variables().access<Vector3>(info->setCentralTopPlatformSizeVariable) = bestSize;
				}
			}
			an_assert(!spaceOccupied.is_empty());
			Vector3 platformsCentre = spaceOccupied.centre();
			platformsCentre.z = spaceOccupied.z.centre();
			if (info->anchorAbove.is_set())
			{
				platformsCentre.z = spaceOccupied.z.max + random.get(info->anchorAbove.get(roomVariables, Range::zero));
			}
			if (info->anchorBelow.is_set())
			{
				platformsCentre.z = spaceOccupied.z.min - random.get(info->anchorBelow.get(roomVariables, Range::zero));
			}
				
			if (info->setAnchorVariable.is_valid())
			{
				if (doorSetupThrough == PlatformsLinearDoorSetupThrough::DoorInRooms)
				{
					// we should always have Up, other directions are just in any case
					DirFourClockwise::Type dirs[] = { DirFourClockwise::Up
													, DirFourClockwise::Down
													, DirFourClockwise::Right
													, DirFourClockwise::Left };
					for_count(int, i, DirFourClockwise::NUM)
					{
						DirFourClockwise::Type inDirLocal = dirs[i];
						DirFourClockwise::Type inDirWorld = piow->get_dir_to_world_for_cell_context(inDirLocal);
						if (auto* dir = piow->find_best_door_in_dir(_room, inDirWorld))
						{
							Vector3 actualInDir = dir->get_outbound_transform().get_axis(Axis::Forward);
							actualInDir.z = 0.0f;
							actualInDir.normalise();

							Transform inDirInWorld = look_matrix(Vector3::zero, DirFourClockwise::dir_to_vector_int2(inDirWorld).to_vector2().to_vector3(), Vector3::zAxis).to_transform();
							Transform inDirInRoom = look_matrix(platformsCentre, actualInDir, Vector3::zAxis).to_transform();

							output(TXT("inDirInWorld %.3f'"), inDirInWorld.get_orientation().to_rotator().yaw);
							output(TXT("inDirInRoom %.3f'"), inDirInRoom.get_orientation().to_rotator().yaw);
							output(TXT("inDirInRoom.to_world(inDirInWorld.inverted()) %.3f'"), inDirInRoom.to_world(inDirInWorld.inverted()).get_orientation().to_rotator().yaw);
							_room->access_variables().access<Transform>(info->setAnchorVariable) = inDirInRoom.to_world(inDirInWorld.inverted());
							break;
						}
					}
				}
				else
				{
					Transform anchor = look_matrix(platformsCentre, Vector3::yAxis, Vector3::zAxis).to_transform();

					_room->access_variables().access<Transform>(info->setAnchorVariable) = anchor;
				}
			}
		}
	}

#ifdef OUTPUT_GENERATION		
	output(TXT("generated platforms linear:"));
	{
		LogInfoContext logInfoContext;
		platformsSetup.log(logInfoContext);
		verticalAdjustment.log(logInfoContext);
		logInfoContext.output_to_output();
	}
#endif

	if (!_subGenerator)
	{
		_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	return result;
}

Range PlatformsLinear::calculate_door_size(Framework::Room const* _room) const
{
	Range doorSizeZ = Range::zero;
	if (doorSetupThrough == PlatformsLinearDoorSetupThrough::DoorInRooms)
	{
		for_every_ptr(door, _room->get_all_doors())
		{
			doorSizeZ.include(door->calculate_size().y.max);
		}
	}
	else
	{
		doorSizeZ.include(Framework::Door::get_nominal_door_height());
	}
	return doorSizeZ;
}

void PlatformsLinear::find_central_top_platform(Framework::Room const* _room, OUT_ Vector3 * _at, OUT_ Vector3 * _size) const
{
	int bestDist = NONE;
	Vector3 bestAt = Vector3::zero;
	Vector3 bestSize = Vector3::zero;
	Range doorSizeZ = calculate_door_size(_room);
	for_every(platform, platforms)
	{
		if (!platform->atGrid.is_set()) continue;

		int dist = abs(gridCentreAt.x - platform->atGrid.get().x) + abs(gridCentreAt.y - platform->atGrid.get().y);
		if (bestDist == NONE || dist <= bestDist)
		{
			Vector3 at = platform->get_room_placement();

			if (bestDist == NONE || dist < bestDist ||
				(dist == bestDist && at.z > bestAt.z))
			{
				bestDist = dist;
				bestAt = at;
				bestSize = platform->calculate_half_size(doorSizeZ.max);
			}
		}
	}
	assign_optional_out_param(_at, bestAt);
	assign_optional_out_param(_size, bestSize);
}

void PlatformsLinear::collect_doors(Framework::Room* _room)
{
	if (doorSetupThrough == PlatformsLinearDoorSetupThrough::DoorInRooms)
	{
		auto* piow = PilgrimageInstanceOpenWorld::get();
		DirFourClockwise::Type inDirWorld = DirFourClockwise::Down;
		DirFourClockwise::Type outDirWorld = DirFourClockwise::Up;
		DirFourClockwise::Type leftDirWorld = DirFourClockwise::Left;
		DirFourClockwise::Type rightDirWorld = DirFourClockwise::Right;
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			piow->tag_open_world_directions_for_cell(_room);
			inDirWorld = piow->get_dir_to_world_for_cell_context(inDirWorld);
			outDirWorld = piow->get_dir_to_world_for_cell_context(outDirWorld);
			leftDirWorld = piow->get_dir_to_world_for_cell_context(leftDirWorld);
			rightDirWorld = piow->get_dir_to_world_for_cell_context(rightDirWorld);
		}
		Array<DoorInfo> otherDoors;
		for_every_ptr(dir, _room->get_all_doors())
		{
			if ((! piow && dir->get_tags().get_tag(NAME(in))) ||
				(piow && piow->is_door_in_dir(dir, inDirWorld)))
			{
				inDoors.push_back(DoorInfo(dir));
			}
			else if ((! piow && dir->get_tags().get_tag(NAME(out))) ||
				(piow && piow->is_door_in_dir(dir, outDirWorld)))
			{
				outDoors.push_back(DoorInfo(dir));
			}
			else if ((! piow && dir->get_tags().get_tag(NAME(left))) ||
				(piow && piow->is_door_in_dir(dir, leftDirWorld)))
			{
				leftDoors.push_back(DoorInfo(dir));
			}
			else if ((! piow && dir->get_tags().get_tag(NAME(right))) ||
				(piow && piow->is_door_in_dir(dir, rightDirWorld)))
			{
				rightDoors.push_back(DoorInfo(dir));
			}
			else
			{
				otherDoors.push_back(DoorInfo(dir));
			}
		}
		while (! otherDoors.is_empty())
		{
			int dir = random.get_int(2);
			if (dir == 0)
			{
				inDoors.push_back(otherDoors[0]);
			}
			else
			{
				outDoors.push_back(otherDoors[0]);
			}
			otherDoors.remove_at(0);
		}
	}
	else if (doorSetupThrough == PlatformsLinearDoorSetupThrough::CreatePOIs)
	{
		inDoors.set_size(doorSetupThroughCreatePOIs.inCount.get());
		outDoors.set_size(doorSetupThroughCreatePOIs.outCount.get());
		leftDoors.set_size(doorSetupThroughCreatePOIs.leftCount.get());
		rightDoors.set_size(doorSetupThroughCreatePOIs.rightCount.get());
		for_every(d, inDoors)
		{
			d->createPOI = doorSetupThroughCreatePOIs.inPOIName.get();
		}
		for_every(d, outDoors)
		{
			d->createPOI = doorSetupThroughCreatePOIs.outPOIName.get();
		}
		for_every(d, leftDoors)
		{
			d->createPOI = doorSetupThroughCreatePOIs.leftPOIName.get();
		}
		for_every(d, rightDoors)
		{
			d->createPOI = doorSetupThroughCreatePOIs.rightPOIName.get();
		}
	}
	else
	{
		todo_implement_now;
	}
}

void PlatformsLinear::place_doors_in_local_space_initially()
{
	RoomGenerationInfo const & rggi = get_room_generation_info();

	// orient along y axis

	{
		float inDoorSideLocalNear = rggi.get_tiles_zone_offset().y + rggi.get_tile_size();
		for_every(door, inDoors)
		{
			Transform placement = look_at_matrix(Vector3(0.0f, inDoorSideLocalNear, 0.0f),
				Vector3(0.0f, inDoorSideLocalNear - rggi.get_tile_size(), 0.0f), Vector3::zAxis).to_transform();
			door->placement = placement;
		}
	}

	{
		float outDoorSideLocalFar = rggi.get_tiles_zone_offset().y + (float)(rggi.get_tile_count().y - 1) * rggi.get_tile_size();
		for_every(door, outDoors)
		{
			Transform placement = look_at_matrix(Vector3(0.0f, outDoorSideLocalFar, 0.0f),
				Vector3(0.0f, outDoorSideLocalFar + rggi.get_tile_size(), 0.0f), Vector3::zAxis).to_transform();
			door->placement = placement;
		}
	}

	{
		float leftDoorSideLocalNear = rggi.get_tiles_zone_offset().x + rggi.get_tile_size();
		for_every(door, leftDoors)
		{
			Transform placement = look_at_matrix(Vector3(leftDoorSideLocalNear, 0.0f, 0.0f),
				Vector3(leftDoorSideLocalNear - rggi.get_tile_size(), 0.0f, 0.0f), Vector3::zAxis).to_transform();
			door->placement = placement;
		}
	}

	{
		float rightDoorSideLocalFar = rggi.get_tiles_zone_offset().x + (float)(rggi.get_tile_count().x - 1) * rggi.get_tile_size();
		for_every(door, rightDoors)
		{
			Transform placement = look_at_matrix(Vector3(rightDoorSideLocalFar, 0.0f, 0.0f),
				Vector3(rightDoorSideLocalFar + rggi.get_tile_size(), 0.0f, 0.0f), Vector3::zAxis).to_transform();
			door->placement = placement;
		}
	}
}

void PlatformsLinear::create_door_platforms()
{
	RoomGenerationInfo const & rggi = get_room_generation_info();

	// create in platforms
	for_every(door, inDoors)
	{
		Platform platform;
		platform.type = PT_In;
#ifdef AN_PLATFORM_DEBUG_INFO
		platform.debugInfo = TXT("in platform");
#endif
		platform.tilesAvailable.x.min = 0;
		platform.tilesAvailable.x.max = rggi.get_tile_count().x - 1;
		platform.tilesAvailable.y.min = 1; // leave one for door
		platform.tilesAvailable.y.max = 1; // always at second
		platform.blockCartSide = PD_Back;

		platforms.push_back(platform);
		platformsIn.push_back(platforms.get_size() - 1);
	}

	// create out platforms
	for_every(door, outDoors)
	{
		Platform platform;
		platform.type = PT_Out;
#ifdef AN_PLATFORM_DEBUG_INFO
		platform.debugInfo = TXT("out platform");
#endif

		platform.tilesAvailable.x.min = 0;
		platform.tilesAvailable.x.max = rggi.get_tile_count().x - 1;
		platform.tilesAvailable.y.min = rggi.get_tile_count().y - 2; // always at second to end
		platform.tilesAvailable.y.max = rggi.get_tile_count().y - 2; // leave one for door
		platform.blockCartSide = PD_Forth;

		platforms.push_back(platform);
		platformsOut.push_back(platforms.get_size() - 1);
	}

	// create left platforms
	for_every(door, leftDoors)
	{
		Platform platform;
		platform.type = PT_Left;
#ifdef AN_PLATFORM_DEBUG_INFO
		platform.debugInfo = TXT("left platform");
#endif

		platform.tilesAvailable.x.min = 1; // leave one for door
		platform.tilesAvailable.x.max = 1; // always at second
		platform.tilesAvailable.y.min = 0;
		platform.tilesAvailable.y.max = rggi.get_tile_count().y - 1;
		platform.blockCartSide = PD_Left;

		platforms.push_back(platform);
		platformsLeft.push_back(platforms.get_size() - 1);
	}

	// create right platforms
	for_every(door, rightDoors)
	{
		Platform platform;
		platform.type = PT_Right;
#ifdef AN_PLATFORM_DEBUG_INFO
		platform.debugInfo = TXT("right platform");
#endif

		platform.tilesAvailable.x.min = rggi.get_tile_count().x - 2; // always at second to end
		platform.tilesAvailable.x.max = rggi.get_tile_count().x - 2; // leave one for door
		platform.tilesAvailable.y.min = 0;
		platform.tilesAvailable.y.max = rggi.get_tile_count().y - 1;
		platform.blockCartSide = PD_Right;

		platforms.push_back(platform);
		platformsRight.push_back(platforms.get_size() - 1);
	}

	// create extra platforms
	for_every(sep, platformsSetup.extraPlatforms)
	{
		if (sep->at.is_set())
		{
			Platform platform;
			platform.type = PT_Extra;
#ifdef AN_PLATFORM_DEBUG_INFO
			platform.debugInfo = TXT("extra platform");
#endif
			platform.matchCartPoint = 0;
			if (sep->matchCartPointBack.get(false)) platform.matchCartPoint |= PD_Back;
			if (sep->matchCartPointForth.get(false)) platform.matchCartPoint |= PD_Forth;
			if (sep->matchCartPointLeft.get(false)) platform.matchCartPoint |= PD_Left;
			if (sep->matchCartPointRight.get(false)) platform.matchCartPoint |= PD_Right;
			platform.blockCartSide = 0;
			if (sep->blockCartSideBack.get(false)) platform.blockCartSide |= PD_Back;
			if (sep->blockCartSideForth.get(false)) platform.blockCartSide |= PD_Forth;
			if (sep->blockCartSideLeft.get(false)) platform.blockCartSide |= PD_Left;
			if (sep->blockCartSideRight.get(false)) platform.blockCartSide |= PD_Right;
			platforms.push_back(platform);

			ExtraPlatformInfo epi;
			epi.extraPlatform = sep;
			epi.platformIdx = platforms.get_size() - 1;
			platformsExtra.push_back(epi);
		}
	}
}

void PlatformsLinear::create_platforms_and_connect()
{
	struct OutputGrid
	{
		static void perform(int pathWidth, int lines, PathGrid const * pathGrid)
		{
			output(TXT("platforms linear grid layout"));
			String borderUp;
			String borderDown;
			for_count(int, x, pathWidth)
			{
				if (x < pathWidth - 1)
				{
					borderUp += TXT("''");
					borderDown += TXT("..");
				}
				else
				{
					borderUp += TXT("'");
					borderDown += TXT(".");
				}
			}
			output(TXT("  /%S\\"), borderUp.to_char());
			for_count(int, y, lines)
			{
				String connectForth;
				String line;
				for_count(int, x, pathWidth)
				{
					int at = x + pathWidth * (lines - 1 - y);
					auto& h = pathGrid[at];
					tchar gch = 'o';
					if (!h.connected)
					{
						gch = 'x';
					}
					if (!h.active)
					{
						gch = ' ';
					}
					{
						connectForth += h.connectsForth ? '|' : ' ';
						if (x < pathWidth - 1)
						{
							connectForth += ' ';
						}
						line += gch;
						if (x < pathWidth - 1)
						{
							line += h.connectsRight ? '-' : ' ';
						}
					}
				}
				if (y > 0)
				{
					output(TXT("  :%S:"), connectForth.to_char());
				}
				output(TXT("  :%S:"), line.to_char());
			}
			output(TXT("  \\%S/"), borderDown.to_char());
		}
	};
	/*
	 *	paths and lines
	 *
	 *			  left    path     right
	 *			  path	  width	   path
	 *			 alley	 alleys	   alley
	 * 
	 *	out line		O		O
	 *					|		|
	 *					.		.
	 *					|		|
	 *	line 3			P   P-.-P		two paths are combined into one (1, 2)
	 *					|	|	|
	 *					.	.   .
	 *					|	|	|
	 *	line 2		L-.-P-.-P	P-.-R	two existing paths are connected (0, 1)
	 *					|	|	|
	 *					.	.	.
	 *					|	|	|
	 *	line 1			P-.-P-.-P-.-R	one more path is introduced (0) and two existing paths are connected (1, 2, they moved)
	 *					    |	|
	 *					    .	.
	 *					    |	|
	 *	line 0			    P-.-P		one more path is created (0, 1)
	 *						|
	 *						.
	 *						|
	 *	in line				I
	 *
	 *	I - in platforms
	 *	O - out platforms
	 *  L - left platforms
	 *  R - right platforms
	 *	P - platforms on lines, in some cases those might be double platforms
	 *		this is when we can't fit carts in all directions this happens when tileCount.y is 2
	 *		in such case we need three platforms
	 *
	 *					      |
	 *					    B.F
	 *					    |R---
	 *					    |
	 *					    F.
	 *					     .B
	 *					      |
	 *					   --L|
	 *						B.F
	 *						|
	 *
	 *		because of that, P should be considered to be set of platforms (that is coherent and filled), in this case, three platforms
	 *	. - any number of platforms that will fill space between two platforms and will make carts be properly connected
	 */
	// build paths & lines
	{
		int pathWidth = max(random.get(platformsSetup.width.get()), max(platformsOut.get_size(), platformsIn.get_size())) + 2;
		int lines = max(random.get(platformsSetup.lines.get()), max(platformsLeft.get_size(), platformsRight.get_size())) + 2;
#ifdef OUTPUT_GENERATION
		output(TXT("platforms linear grid, randomised: %i x %i"), pathWidth, lines);
#endif
		if (!platformsExtra.is_empty())
		{
			// we need something there
			lines = max(lines, 3); 
			pathWidth = max(pathWidth, 3);
		}
		else
		{
			if (platformsIn.get_size() > 1 ||
				platformsOut.get_size() > 1)
			{
				lines = max(lines, 3); // we need another layer between
			}
			else
			{
				if (lines <= 2 &&
					platformsLeft.is_empty() && platformsRight.is_empty() && platformsExtra.is_empty())
				{
					// we're good with just connecting in and out platforms
					pathWidth = 1;
					lines = 2;
				}
			}
			if (platformsLeft.get_size() > 1 ||
				platformsRight.get_size() > 1 ||
				platformsExtra.get_size() > 1)
			{
				pathWidth = max(pathWidth, 3); // we need another layer between
			}
		}
		// sanity check
		if (pathWidth < 3 && pathWidth != 1)
		{
			pathWidth = 3;
		}
#ifdef OUTPUT_GENERATION
		output(TXT("platforms linear grid, corrected: %i x %i"), pathWidth, lines);
#endif
		ARRAY_STACK(PathGrid, pathGrid, pathWidth * lines);
		ARRAY_STACK(VectorInt2, connectedAt, pathWidth* lines);
		pathGrid.set_size(pathWidth * lines);
		// add x line
		gridCentreAt.x = (pathWidth - 1) / 2;
		gridCentreAt.y = (lines - 1) / 2;
		grid = RangeInt2::empty;
		grid.x.min = 0;
		grid.x.max = pathWidth - 1;
		grid.y.min = 0;
		grid.y.max = lines - 1;
		if (lines > 2 || pathWidth > 2)
		{
			if (lines > 2)
			{
				// in and out
				for_count(int, i, platformsIn.get_size())
				{
					bool firstTry = true;
					while (true)
					{
						int atX = pathWidth > 2 ? random.get_int_from_range(1, pathWidth - 2) : random.get_int_from_range(0, pathWidth);
						if (platformsSetup.exitPlatformsCentred.get() && firstTry)
						{
							atX = (pathWidth - platformsIn.get_size()) / 2 + i;
						}
						int idx = atX;
						if (!pathGrid[idx].active)
						{
							pathGrid[idx].active = true;
							pathGrid[idx].platformIdx = platformsIn[i];
							break;
						}
						firstTry = false;
					}
				}
				for_count(int, i, platformsOut.get_size())
				{
					bool firstTry = true;
					while (true)
					{
						int atX = pathWidth > 2 ? random.get_int_from_range(1, pathWidth - 2) : random.get_int_from_range(0, pathWidth);
						if (platformsSetup.exitPlatformsCentred.get() && firstTry)
						{
							atX = (pathWidth - platformsOut.get_size()) / 2 + i;
						}
						int idx = atX + pathWidth * (lines - 1);
						if (!pathGrid[idx].active)
						{
							pathGrid[idx].active = true;
							pathGrid[idx].platformIdx = platformsOut[i];
							break;
						}
						firstTry = false;
					}
				}
			}
			if (pathWidth > 2)
			{
				// left and right
				for_count(int, i, platformsLeft.get_size())
				{
					bool firstTry = true;
					while (true)
					{
						int atY = lines > 2 ? random.get_int_from_range(1, lines - 2) : random.get_int_from_range(0, lines);
						if (platformsSetup.exitPlatformsCentred.get() && firstTry)
						{
							atY = (lines - platformsLeft.get_size()) / 2 + i;
						}
						int idx = pathWidth * atY;
						if (!pathGrid[idx].active)
						{
							pathGrid[idx].active = true;
							pathGrid[idx].platformIdx = platformsLeft[i];
							break;
						}
						firstTry = false;
					}
				}
				for_count(int, i, platformsRight.get_size())
				{
					bool firstTry = true;
					while (true)
					{
						int atY = lines > 2 ? random.get_int_from_range(1, lines - 2) : random.get_int_from_range(0, lines);
						if (platformsSetup.exitPlatformsCentred.get() && firstTry)
						{
							atY = (lines - platformsRight.get_size()) / 2 + i;
						}
						int idx = pathWidth * atY + (pathWidth - 1);
						if (!pathGrid[idx].active)
						{
							pathGrid[idx].active = true;
							pathGrid[idx].platformIdx = platformsRight[i];
							break;
						}
						firstTry = false;
					}
				}
			}
			for_every(ep, platformsExtra)
			{
				bool firstTry = true;
				while (true)
				{
					VectorInt2 at;
					if (firstTry)
					{
						at = ep->extraPlatform->at.get();
						if (at.x < 0) at.x = pathWidth + at.x;
						if (at.y < 0) at.y = pathWidth + at.y;
					}
					else
					{
						at.x = random.get_int(pathWidth - 1);
						at.y = random.get_int(lines - 1);
					}
					int idx = at.x + at.y * pathWidth;
					if (!pathGrid[idx].active)
					{
						pathGrid[idx].active = true;
						pathGrid[idx].platformIdx = ep->platformIdx;
						break;
					}
					else if (firstTry)
					{
						error(TXT("invalid extra platform \"at\", already occupied, may result in wrong setup or infinite loop"));
					}
					firstTry = false;
				}
			}
			if ((! platformsSetup.allowSimplePath.get() || ! GameSettings::get().difficulty.simplerMazes) && PilgrimageInstanceOpenWorld::get())
			{
				if (platformsSetup.fullOuterRing.get())
				{
#ifdef OUTPUT_GENERATION
					output(TXT("full outer ring"));
#endif
					for_range(int, x, 1, pathWidth - 2)
					{
						for_range(int, y, 1, lines - 2)
						{
							auto& h = pathGrid[x + y * pathWidth];
							if (y == 1 || y == lines - 2 ||
								x == 1 || x == pathWidth - 2)
							{
								h.active = true;
							}
						}
					}
				}

				// fill randomly all other
				for_range(int, x, 1, pathWidth - 2)
				{
					for_range(int, y, 1, lines - 2)
					{
						auto & h = pathGrid[x + y * pathWidth];
						h.active |= random.get_chance(platformsSetup.platformChance.get());
					}
				}

				// connect randomly
				for_range(int, x, 0, pathWidth - 1)
				{
					for_range(int, y, 0, lines - 1)
					{
						bool allowConnectLeftRight = lines <= 2 || (y > 0 && y < lines - 1);
						bool allowConnectForthBack = pathWidth <=2 || (x > 0 && x < pathWidth - 1);
						auto & h = pathGrid[x + y * pathWidth];
						if (h.active)
						{
							if (x < pathWidth - 1 && allowConnectLeftRight && pathGrid[x + 1 + y * pathWidth].active)
							{
								if (random.get_chance(platformsSetup.connectChance.get()))
								{
									h.connectsRight = true;
								}
								if (platformsSetup.fullOuterRing.get() && (y == 1 || y == lines - 2))
								{
									h.connectsRight = true;
								}
								if (x == 0 || x == pathWidth - 2)
								{
									h.connectsRight = true; // auto connect from border
								}
							}
							if (y < lines - 1 && allowConnectForthBack && pathGrid[x + (y + 1) * pathWidth].active)
							{
								if (random.get_chance(platformsSetup.connectChance.get()))
								{
									h.connectsForth = true;
								}
								if (platformsSetup.fullOuterRing.get() && (x == 1 || x == pathWidth - 2))
								{
									h.connectsForth = true;
								}
								if (y == 0 || y == lines - 2)
								{
									h.connectsForth = true; // auto connect from border
								}
							}
						}
					}
				}
			}
			// connect any input (check all sides)
			{
				bool anythingConnected = false;
				if (!anythingConnected)
				{
					for_count(int, x, pathWidth)
					{
						int at = x;
						if (pathGrid[at].active)
						{
							pathGrid[at].connected = true;
							anythingConnected = true;
							break;
						}
					}
				}
				if (!anythingConnected)
				{
					for_count(int, x, pathWidth)
					{
						int at = x + pathWidth * (lines - 1);
						if (pathGrid[at].active)
						{
							pathGrid[at].connected = true;
							anythingConnected = true;
							break;
						}
					}
				}
				if (!anythingConnected)
				{
					for_count(int, y, lines)
					{
						int at = y * pathWidth;
						if (pathGrid[at].active)
						{
							pathGrid[at].connected = true;
							anythingConnected = true;
							break;
						}
					}
				}
				if (!anythingConnected)
				{
					for_count(int, y, lines)
					{
						int at = y * pathWidth + (pathWidth - 1);
						if (pathGrid[at].active)
						{
							pathGrid[at].connected = true;
							anythingConnected = true;
							break;
						}
					}
				}
			}
#ifdef OUTPUT_GENERATION_DETAILED
			output(TXT("before connect all"));
			OutputGrid::perform(pathWidth, lines, pathGrid.get_data());
#endif
			// connect all
			while (true)
			{
				bool allConnected = true;
				while (true)
				{
					bool propagating = false;
					for_count(int, x, pathWidth)
					{
						for_count(int, y, lines)
						{
							auto & h = pathGrid[x + y * pathWidth];
							if (h.active &&
								h.connectsRight)
							{
								auto & r = pathGrid[(x + 1) + y * pathWidth];
								if (h.connected ^ r.connected)
								{
									propagating = true;
								}
								h.connected |= r.connected;
								r.connected |= h.connected;
							}
							if (h.active &&
								h.connectsForth)
							{
								auto & f = pathGrid[x + (y + 1) * pathWidth];
								if (h.connected ^ f.connected)
								{
									propagating = true;
								}
								h.connected |= f.connected;
								f.connected |= h.connected;
							}
						}
					}
					if (!propagating)
					{
						break;
					}
				}
				for_count(int, x, pathWidth)
				{
					for_count(int, y, lines)
					{
						auto & h = pathGrid[x + y * pathWidth];
						if (h.active &&
							!h.connected)
						{
							allConnected = false;
						}
					}
				}
#ifdef DEBUG_GENERATION
				output(TXT("all connected?"));
				OutputGrid::perform(pathWidth, lines, pathGrid.get_data());
#endif
				if (allConnected)
				{
					break;
				}
				else
				{
					bool addOnePiece = true;
					connectedAt.clear();
					for_count(int, x, pathWidth)
					{
						for_count(int, y, lines)
						{
							auto& h = pathGrid[x + y * pathWidth];
							if (h.active && h.connected)
							{
								connectedAt.push_back(VectorInt2(x, y));
							}
						}
					}

					// try to connect anything from the pull of already connected - ANYTHING
					while (! connectedAt.is_empty())
					{
						int idx = random.get_int(connectedAt.get_size());
						int x = connectedAt[idx].x;
						int y = connectedAt[idx].y;
						connectedAt.remove_fast_at(idx);

						int at = x + pathWidth * y;
						auto & h = pathGrid[at];
						if (h.active)
						{
							bool allowConnectLeftRight = lines <= 2 || (y > 0 && y < lines - 1);
							bool allowConnectForthBack = pathWidth <= 2 || (x > 0 && x < pathWidth - 1);
							int dirOffset = random.get_int(4);
							for_count(int, dirIdx, 4)
							{
								int dir = (dirIdx + dirOffset) % 4;
								if (allowConnectForthBack)
								{
									if (dir == 0 && y < lines - 1)
									{
										auto& n = pathGrid[at + pathWidth];
										if (n.active && !n.connected && !h.connectsForth)
										{
											addOnePiece = false;
											h.connectsForth = true;
											break;
										}
									}
									if (dir == 2 && y > 0)
									{
										auto& n = pathGrid[at - pathWidth];
										if (n.active && !n.connected && !n.connectsForth)
										{
											addOnePiece = false;
											n.connectsForth = true;
											break;
										}
									}
								}
								if (allowConnectLeftRight)
								{
									if (dir == 1 && x < pathWidth - 1)
									{
										auto& n = pathGrid[at + 1];
										if (n.active && !n.connected && !h.connectsRight)
										{
											addOnePiece = false;
											h.connectsRight = true;
											break;
										}
									}
									if (dir == 3 && x > 0)
									{
										auto& n = pathGrid[at - 1];
										if (n.active && !n.connected && !n.connectsRight)
										{
											addOnePiece = false;
											n.connectsRight = true;
											break;
										}
									}
								}
							}
							if (!addOnePiece)
							{
								break;
							}
						}
					}
					if (addOnePiece && random.get_chance(0.3f)) // this will add piece from an existing one
					{
						// add one piece but only from an existing one, doesn't actually have to be connected
						connectedAt.clear();
						for_count(int, x, pathWidth)
						{
							for_count(int, y, lines)
							{
								auto& h = pathGrid[x + y * pathWidth];
								if (h.active)
								{
									connectedAt.push_back(VectorInt2(x, y));
								}
							}
						}

						while (!connectedAt.is_empty())
						{
							int idx = random.get_int(connectedAt.get_size());
							int x = connectedAt[idx].x;
							int y = connectedAt[idx].y;
							connectedAt.remove_fast_at(idx);

							int at = x + pathWidth * y;
							auto& h = pathGrid[at];
							if (h.active)
							{
								bool allowConnectLeftRight = lines <= 2 || (y > 0 && y < lines - 1);
								bool allowConnectForthBack = pathWidth <= 2 || (x > 0 && x < pathWidth - 1);
								int dirOffset = random.get_int(4);
								for_count(int, dirIdx, 4)
								{
									int dir = (dirIdx + dirOffset) % 4;
									if (allowConnectForthBack)
									{
										if (dir == 0 && y < lines - 2)
										{
											auto& n = pathGrid[at + pathWidth];
											if (!n.active)
											{
												n.active = true;
												addOnePiece = false;
												break;
											}
										}
										if (dir == 2 && y > 1)
										{
											auto& n = pathGrid[at - pathWidth];
											if (! n.active)
											{
												n.active = true;
												addOnePiece = false;
												break;
											}
										}
									}
									if (allowConnectLeftRight)
									{
										if (dir == 1 && x < pathWidth - 2)
										{
											auto& n = pathGrid[at + 1];
											if (!n.active)
											{
												n.active = true;
												h.connectsRight = true;
												break;
											}
										}
										if (dir == 3 && x > 1)
										{
											auto& n = pathGrid[at - 1];
											if (!n.active)
											{
												n.active = true;
												n.connectsRight = true;
												break;
											}
										}
									}
								}
								if (!addOnePiece)
								{
									break;
								}
							}
						}
					}
					if (addOnePiece)
					{
						// add one piece, because maybe we do not have
						int x = pathWidth > 2? random.get_int_from_range(1, pathWidth - 2) : random.get_int_from_range(0, pathWidth);
						int y = lines > 2? random.get_int_from_range(1, lines - 2) : random.get_int_from_range(0, lines);
						int at = x + pathWidth * y;
						pathGrid[at].active = true;
#ifdef DEBUG_GENERATION
						output(TXT("added one piece"));
						OutputGrid::perform(pathWidth, lines, pathGrid.get_data());
#endif
					}
					else
					{
#ifdef DEBUG_GENERATION
						output(TXT("connected something"));
						OutputGrid::perform(pathWidth, lines, pathGrid.get_data());
#endif
					}
				}
			}

#ifdef OUTPUT_GENERATION_DETAILED
			output(TXT("post connect all"));
			OutputGrid::perform(pathWidth, lines, pathGrid.get_data());
#endif

			// remove to keep all connected
			if (platformsSetup.allowSimplePath.get() &&
				((GameSettings::get().difficulty.simplerMazes || !PilgrimageInstanceOpenWorld::get()) 
				 || random.get_chance(0.2f))) // be minimal, just a single path
			{
#ifdef OUTPUT_GENERATION
				output(TXT("simpler maze, remove to have a single path"));
#endif
				bool canStillRemove = true;
				int removeLeft = random.get_int(lines * pathWidth / 5);
				while (canStillRemove)
				{
					if ((! GameSettings::get().difficulty.simplerMazes && PilgrimageInstanceOpenWorld::get()) && removeLeft <= 0)
					{
						break;
					}
					--removeLeft;
					canStillRemove = false;
					VectorInt2 tryRemoving;
					tryRemoving.x = pathWidth > 2 ? random.get_int_from_range(1, pathWidth - 2) : random.get_int_from_range(0, pathWidth);
					tryRemoving.y = lines > 2 ? random.get_int_from_range(1, lines - 2) : random.get_int_from_range(0, lines);
					VectorInt2 startedAt = tryRemoving;
					while (true)
					{
						auto & h = pathGrid[tryRemoving.x + tryRemoving.y * pathWidth];
						if (h.active)
						{
							for_count(int, x, pathWidth)
							{
								for_count(int, y, lines)
								{
									auto & h = pathGrid[x + y * pathWidth];
									h.checkedConnection = false;
								}
							}
							for_count(int, x, pathWidth)
							{
								if (pathGrid[x].active)
								{
									pathGrid[x].checkedConnection = true;
									break;
								}
							}

							bool allConnected = true;
							while (true)
							{
								bool propagating = false;
								for_count(int, x, pathWidth)
								{
									for_count(int, y, lines)
									{
										if (x == tryRemoving.x && y == tryRemoving.y)
										{
											continue;
										}
										auto & h = pathGrid[x + y * pathWidth];
										if (h.active &&
											h.connectsRight &&
											(x + 1 != tryRemoving.x || y != tryRemoving.y))
										{
											auto & r = pathGrid[(x + 1) + y * pathWidth];
											if (r.active)
											{
												if (h.checkedConnection ^ r.checkedConnection)
												{
													propagating = true;
												}
												h.checkedConnection |= r.checkedConnection;
												r.checkedConnection |= h.checkedConnection;
											}
										}
										if (h.active &&
											h.connectsForth &&
											(x != tryRemoving.x || y + 1 != tryRemoving.y))
										{
											auto & f = pathGrid[x + (y + 1) * pathWidth];
											if (f.active)
											{
												if (h.checkedConnection ^ f.checkedConnection)
												{
													propagating = true;
												}
												h.checkedConnection |= f.checkedConnection;
												f.checkedConnection |= h.checkedConnection;
											}
										}
									}
								}
								if (!propagating)
								{
									break;
								}
							}
							for_count(int, x, pathWidth)
							{
								for_count(int, y, lines)
								{
									if (x == tryRemoving.x && y == tryRemoving.y)
									{
										continue;
									}
									auto & h = pathGrid[x + y * pathWidth];
									if (h.active &&
										!h.checkedConnection)
									{
										allConnected = false;
									}
								}
							}

							if (allConnected)
							{
								if (tryRemoving.x > 0)
								{
									pathGrid[(tryRemoving.x - 1) + tryRemoving.y * pathWidth].connectsRight = false;
								}
								if (tryRemoving.y > 0)
								{
									pathGrid[tryRemoving.x + (tryRemoving.y - 1) * pathWidth].connectsForth = false;
								}
								h.active = false;
								h.connectsRight = false;
								h.connectsForth = false;
								canStillRemove = true;
								break; // get next random
							}
						}

						VectorInt2 next = tryRemoving;
						next.x++;
						if (next.x >= pathWidth -1)
						{
							next.x = 1;
							next.y++;
							if (next.y >= lines - 1)
							{
								next.y = 1;
							}
						}
						if (next == startedAt)
						{
							break;
						}
						tryRemoving = next;
					}
				}
			}

#ifdef OUTPUT_GENERATION_DETAILED
			output(TXT("post remove to keep all connected"));
			OutputGrid::perform(pathWidth, lines, pathGrid.get_data());
#endif
		}
		else
		{
			// make both active (in and out)
			an_assert(pathGrid.get_size() == 2);
			an_assert(platformsIn.get_size() == 1);
			an_assert(platformsOut.get_size() == 1);
			an_assert(platformsLeft.get_size() == 0);
			an_assert(platformsRight.get_size() == 0);
			{
				int at = 0;
				pathGrid[at].active = true;
				pathGrid[at].connectsForth = true;
				pathGrid[at].platformIdx = platformsIn[0];
			}
			{
				int idx = 1;
				pathGrid[idx].active = true;
				pathGrid[idx - pathWidth].connectsForth = true;
				pathGrid[idx].platformIdx = platformsOut[0];
			}
		}

#ifdef OUTPUT_GENERATION
		OutputGrid::perform(pathWidth, lines, pathGrid.get_data());
#endif

		// build key points, also for existing platforms! we will reuse actual platforms but will create additional keypoints if required
		for_count(int, allowMatchCartPoint, 2)
		{
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
			output(TXT("create keypoints : %S"), allowMatchCartPoint? TXT("allow match cart point") : TXT("normal"));
#endif
			for_count(int, x, pathWidth)
			{
				for_count(int, y, lines)
				{
					int h = x + y * pathWidth;
					if (pathGrid[h].active)
					{
						bool performNow = ! pathGrid[h].kpiGenerated;
						if (performNow &&
							pathGrid[h].platformIdx.is_set())
						{
							if (!allowMatchCartPoint)
							{
								auto& platform = platforms[pathGrid[h].platformIdx.get()];
								if (platform.matchCartPoint)
								{
									performNow = false;
								}
							}
						}
						if (performNow)
						{
							int buildDirs = 0;
							if (pathGrid[h].connectsRight) buildDirs |= PD_Right;
							if (pathGrid[h].connectsForth) buildDirs |= PD_Forth;
							if (x >= 1)
							{
								if (pathGrid[h - 1].active && pathGrid[h-1].connectsRight) buildDirs |= PD_Left;
							}
							if (y >= 1)
							{
								if (pathGrid[h - pathWidth].active && pathGrid[h - pathWidth].connectsForth) buildDirs |= PD_Back;
							}
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
							output(TXT("create keypoint for %i x %i"), x, y);
#endif
							pathGrid[h].kpi = create_key_point(buildDirs, VectorInt2(x, y), pathGrid[h].platformIdx, allowMatchCartPoint);
							pathGrid[h].kpiGenerated = true;
						}
					}
				}
			}
		}

		// connect key points
		for_count(int, x, pathWidth)
		{
			for_count(int, y, lines)
			{
				auto & h = pathGrid[x + y * pathWidth];
				if (h.active && h.connectsRight && x < pathWidth - 1)
				{
					auto & r = pathGrid[(x + 1) + y * pathWidth];
					an_assert(r.active);
					connect_platforms(h.kpi.right.platformIdx, r.kpi.left.platformIdx, CA_LeftRight, 1, CA_LeftRight, -1);
				}
				if (h.active && h.connectsForth && y < lines - 1)
				{
					auto & f = pathGrid[x + (y + 1) * pathWidth];
					an_assert(f.active);
					connect_platforms(h.kpi.forth.platformIdx, f.kpi.back.platformIdx, CA_BackForth, 1, CA_BackForth, -1);
				}
			}
		}
	}
}

void PlatformsLinear::all_platforms_centred_xy()
{
	Range2 spaceOccupiedXY = Range2::empty;
	for_every(p, platforms)
	{
		Vector3 at = p->get_room_placement();

		spaceOccupiedXY.include(at.to_vector2());
	}

	Vector3 moveBy = -spaceOccupiedXY.centre().to_vector3();

	for_every(p, platforms)
	{
		p->at += moveBy;
	}
}

void PlatformsLinear::all_platforms_above_zero()
{
	Optional<float> lowestZ;
	for_every(p, platforms)
	{
		float pz = p->get_room_placement().z;

		lowestZ = min(pz, lowestZ.get(pz));
	}

	if (lowestZ.is_set() && lowestZ.get() < 0.0f)
	{
		float moveUp = -lowestZ.get();
		for_every(p, platforms)
		{
			p->at.z += moveUp;
		}
	}
}

void PlatformsLinear::align_door_platforms()
{
	// position "in" and "out" platforms each group in own line
	{
		Optional<float> inLine;
		for_every(in, platformsIn)
		{
			if (!inLine.is_set())
			{
				inLine = platforms[*in].at.y;
			}
			else
			{
				inLine = min(inLine.get(), platforms[*in].at.y);
			}
		}
		for_every(in, platformsIn)
		{
			platforms[*in].at.y = inLine.get();
		}
	}

	{
		Optional<float> inLine;
		for_every(out, platformsOut)
		{
			if (!inLine.is_set())
			{
				inLine = platforms[*out].at.y;
			}
			else
			{
				inLine = max(inLine.get(), platforms[*out].at.y);
			}
		}
		for_every(out, platformsOut)
		{
			platforms[*out].at.y = inLine.get();
		}
	}

	{
		Optional<float> inLine;
		for_every(in, platformsLeft)
		{
			if (!inLine.is_set())
			{
				inLine = platforms[*in].at.x;
			}
			else
			{
				inLine = min(inLine.get(), platforms[*in].at.x);
			}
		}
		for_every(in, platformsLeft)
		{
			platforms[*in].at.x = inLine.get();
		}
	}

	{
		Optional<float> inLine;
		for_every(out, platformsRight)
		{
			if (!inLine.is_set())
			{
				inLine = platforms[*out].at.x;
			}
			else
			{
				inLine = max(inLine.get(), platforms[*out].at.x);
			}
		}
		for_every(out, platformsRight)
		{
			platforms[*out].at.x = inLine.get();
		}
	}
}

bool PlatformsLinear::separate_door_platforms(int _againstPIdx)
{
	bool requiresMovement = false;
	float const minInOutSeparation = platformsSetup.minDoorPlatformSeparation.get();
	if (!platformsIn.does_contain(_againstPIdx) && !platformsIn.is_empty() &&
		platforms[_againstPIdx].at.y < platforms[platformsIn[0]].at.y + minInOutSeparation)
	{
		try_to_move(_againstPIdx, Vector3::yAxis * 0.2f);
		requiresMovement = true;
	}
	if (!platformsOut.does_contain(_againstPIdx) && !platformsOut.is_empty() &&
		platforms[_againstPIdx].at.y > platforms[platformsOut[0]].at.y - minInOutSeparation)
	{
		try_to_move(_againstPIdx, -Vector3::yAxis * 0.2f);
		requiresMovement = true;
	}
	if (!platformsLeft.does_contain(_againstPIdx) && !platformsLeft.is_empty() &&
		platforms[_againstPIdx].at.x < platforms[platformsLeft[0]].at.x + minInOutSeparation)
	{
		try_to_move(_againstPIdx, Vector3::xAxis * 0.2f);
		requiresMovement = true;
	}
	if (!platformsRight.does_contain(_againstPIdx) && !platformsRight.is_empty() &&
		platforms[_againstPIdx].at.x > platforms[platformsRight[0]].at.x - minInOutSeparation)
	{
		try_to_move(_againstPIdx, -Vector3::xAxis * 0.2f);
		requiresMovement = true;
	}
	return requiresMovement;
}

void PlatformsLinear::calculate_door_placement(DoorInfo* door, Platform const& platform, Vector3 const& _side, OUT_ Transform & _placement, OUT_ Transform & _vrPlacement)
{
	Vector3 relAt = _side * platform.platform.length().to_vector3();
	relAt *= 0.5f;
	_placement = Transform(platform.get_room_placement() + relAt, door->placement.get_orientation());
	_vrPlacement = Transform(platform.at, Quat::identity).to_local(_placement);
}

void PlatformsLinear::place_doors(Framework::Game* _game, Framework::Room* _room)
{
	Framework::SceneryType* atDoorSceneryType = info->find<Framework::SceneryType>(roomVariables, info->atDoorSceneryType);

	if (atDoorSceneryType)
	{
		atDoorSceneryType->load_on_demand_if_required();
	}

	int vrAnchorIdIdx = 0;
	for_count(int, i, 4)
	{
		// 0 in
		// 1 out
		// 2 left
		// 3 right
		auto& doors = i <= 1 ? (i == 0 ? inDoors : outDoors) : (i == 2 ? leftDoors : rightDoors);
		auto& platformsDoors = i <= 1 ? (i == 0 ? platformsIn : platformsOut) : (i == 2 ? platformsLeft : platformsRight);
		for_every(pDoor, doors)
		{
			auto*& doorInRoom = pDoor->door;
			auto const& platform = platforms[platformsDoors[for_everys_index(pDoor)]];
			Transform placement = Transform::identity;
			Transform vrPlacement = Transform::identity;
			switch (i)
			{
			case 0:	calculate_door_placement(pDoor, platform, -Vector3::yAxis, OUT_ placement, OUT_ vrPlacement); break;
			case 1:	calculate_door_placement(pDoor, platform,  Vector3::yAxis, OUT_ placement, OUT_ vrPlacement); break;
			case 2:	calculate_door_placement(pDoor, platform, -Vector3::xAxis, OUT_ placement, OUT_ vrPlacement); break;
			case 3:	calculate_door_placement(pDoor, platform,  Vector3::xAxis, OUT_ placement, OUT_ vrPlacement); break;
			default: todo_implement_now; break;
			}
			if (doorSetupThrough == PlatformsLinearDoorSetupThrough::DoorInRooms)
			{
				if (!doorInRoom->is_vr_space_placement_set() ||
					!Framework::DoorInRoom::same_with_orientation_for_vr(doorInRoom->get_vr_space_placement(), vrPlacement))
				{
					if (doorInRoom->is_vr_placement_immovable())
					{
						doorInRoom = doorInRoom->grow_into_vr_corridor(NP, vrPlacement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
					}
					_game->perform_sync_world_job(TXT("place door"), [doorInRoom, placement, vrPlacement]()
					{
							doorInRoom->set_vr_space_placement(vrPlacement);
					});
				}
				_game->perform_sync_world_job(TXT("place door"), [doorInRoom, placement, vrPlacement]()
				{
						doorInRoom->set_placement(placement);
				});
			}
			else if (doorSetupThrough == PlatformsLinearDoorSetupThrough::CreatePOIs)
			{
				Transform vrAnchorPlacement = Transform::identity; // in vr space
				vrAnchorPlacement = vrPlacement.to_local(vrAnchorPlacement); // in door space
				vrAnchorPlacement = placement.to_world(vrAnchorPlacement); // in room/world space

				Name vrAnchorId = Name(String::printf(TXT("vra_%i"), vrAnchorIdIdx));

				// door poi
				{
					Framework::PointOfInterest* poi = new Framework::PointOfInterest();
					poi->name = pDoor->createPOI;
					poi->parameters.access<Name>(NAME(vrAnchorId)) = vrAnchorId;
					_room->add_poi(poi, placement);
				}

				// associated vr anchor poi
				{
					Framework::PointOfInterest* poi = new Framework::PointOfInterest();
					poi->name = NAME(vrAnchor);
					poi->parameters.access<Name>(NAME(vrAnchorId)) = vrAnchorId;
					_room->add_poi(poi, vrAnchorPlacement);
				}

			}
			else
			{
				todo_implement_now;
			}
			if (atDoorSceneryType)
			{
				Framework::Scenery* atDoorScenery = nullptr;
				int atDoorIdx = for_everys_index(pDoor);
				_game->perform_sync_world_job(TXT("spawn at door"), [&atDoorScenery, atDoorSceneryType, atDoorIdx, _room]()
					{
						atDoorScenery = new Framework::Scenery(atDoorSceneryType, String::printf(TXT("at door %i"), atDoorIdx));
						atDoorScenery->init(_room->get_in_sub_world());
					});
				switch (i)
				{
				case 0:	atDoorScenery->access_tags().set_tag(NAME(exitIn)); break;
				case 1:	atDoorScenery->access_tags().set_tag(NAME(exitOut)); break;
				case 2:	atDoorScenery->access_tags().set_tag(NAME(exitLeft)); break;
				case 3:	atDoorScenery->access_tags().set_tag(NAME(exitRight)); break;
				default: todo_implement_now; break;
				}
				atDoorScenery->access_individual_random_generator() = random.spawn();
				appearanceSetup.setup_variables(atDoorScenery);
				atDoorScenery->access_variables().set_from(roomVariables);
				info->apply_generation_parameters_to(atDoorScenery->access_variables());
				atDoorScenery->initialise_modules();
				_game->perform_sync_world_job(TXT("place at door"), [&atDoorScenery, placement, _room]()
					{
						atDoorScenery->get_presence()->place_in_room(_room, placement);
					});
				_game->on_newly_created_object(atDoorScenery);
			}

			pDoor->placement = placement;
		}
	}
}

void PlatformsLinear::connect_platforms(int _aIdx, int _bIdx, CartAxis _aUsingAxis, int _aDir, CartAxis _bUsingAxis, int _bDir, int _addExtraPlatforms)
{
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
	int addedPlatforms = 0;
	output(TXT("connecting platforms %i and %i --------------------"), _aIdx, _bIdx);
#endif
	an_assert(_aIdx != NONE);
	an_assert(_bIdx != NONE);
	CartAxis orgAAxis = _aUsingAxis;
	CartAxis orgBAxis = _bUsingAxis;
	int orgADir = _aDir;
	int orgBDir = _bDir;
	bool forceVerticals = platformsSetup.useVerticals.get() > 0.5f && random.get_chance(platformsSetup.useVerticals.get() * 0.4f);
	while (true)
	{
		if (_addExtraPlatforms <= 0)
		{
			// we're to add final one
			if (add_cart_between(_aIdx, _bIdx, _aUsingAxis, _aDir))
			{
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
				output(TXT("added %i when connecting two"), addedPlatforms);
#endif
				return;
			}

			// this works on case per case (I think I've got it all covered)
			// with fail safe of adding random platform
			// note: remember to add carts when everything is already set and fine
			// we can't connnect platforms directly
			// let's check what are our options

			bool doA = random.get_chance(0.5f);
			int & p_1 = doA ? _aIdx : _bIdx;
			CartAxis & axis_1 = doA ? _aUsingAxis : _bUsingAxis;
			int & dir_1 = doA ? _aDir : _bDir;

			int & p_2 = !doA ? _aIdx : _bIdx;
			CartAxis & axis_2 = !doA ? _aUsingAxis : _bUsingAxis;
			int & dir_2 = !doA ? _aDir : _bDir;

			{
				CartPoint *cp_1 = platforms[p_1].find_cart_point_unconnected(axis_1, dir_1);
				an_assert(cp_1, TXT("can't connect those platforms. what are we trying to do then?"));

				CartPoint *cp_2 = platforms[p_2].find_cart_point_unconnected(axis_2, dir_2);
				an_assert(cp_2, TXT("can't connect those platforms. what are we trying to do then?"));

				RangeInt2 ep_1 = cp_1->get_entry_tiles(this);
				RangeInt2 ep_2 = cp_2->get_entry_tiles(this);
				if (ep_1.overlaps(ep_2))
				{
					// they should see each other, therefore there should be need for just one platform in between
					// this of course might be not case if cart lines would cross
					// in such cases we will try going vertical for one or another
					// if both fails, we will try going vertical for both
					// if that fails too, we should do two platforms
					if (cp_1->tilePlacement != cp_2->tilePlacement)
					{
						// we should just need one extra platform between both of them, add it now
						Platform newPlatform;
#ifdef AN_PLATFORM_DEBUG_INFO
						newPlatform.debugInfo = TXT("one between two");
#endif

						CartPoint cpFrom1 = *cp_1;
						cpFrom1.dir = -cpFrom1.dir;
						newPlatform.cartPoints.push_back(cpFrom1);

						CartPoint cpFrom2 = *cp_2;
						cpFrom2.dir = -cpFrom2.dir;
						bool ok = false;
						if (!forceVerticals && find_placement_and_side_for(cpFrom2, newPlatform, cpFrom2.tilePlacement, cpFrom2.side))
						{
							newPlatform.cartPoints.push_back(cpFrom2);
							ok = true;
						}
						else if (platforms[p_2].can_have_another_vertical(cp_2->dir))
						{
							// we can't add it as it is, change one of axes to verticals (we don't want to change both of them!)
							an_assert(!is_vertical(cp_2->axis));
							cp_2->axis = change_verticality(cp_2->axis);
							cpFrom2.axis = cp_2->axis;
							if (find_placement_and_side_for(cpFrom2, newPlatform, cpFrom2.tilePlacement, cpFrom2.side))
							{
								newPlatform.cartPoints.push_back(cpFrom2);
								ok = true;
							}
							else
							{
								// restore, try the other one
								cp_2->axis = change_verticality(cp_2->axis);
								cpFrom2.axis = cp_2->axis;
								if (platforms[p_1].can_have_another_vertical(cp_1->dir))
								{
									//
									an_assert(!is_vertical(cp_1->axis));
									cp_1->axis = change_verticality(cp_1->axis);
									cpFrom1.axis = cp_1->axis;
									// change cartpoints, replace 1 with 2
									newPlatform.cartPoints[0] = cpFrom2;
									if (find_placement_and_side_for(cpFrom1, newPlatform, cpFrom1.tilePlacement, cpFrom1.side))
									{
										newPlatform.cartPoints.push_back(cpFrom1);
										ok = true;
									}
									else
									{
										bool doTwoVerticals = random.get_chance(platformsSetup.useVerticals.get() * 0.3f); // rather rare
										if (!doTwoVerticals)
										{
											// restore, try the other one
											cp_1->axis = change_verticality(cp_1->axis);
											cpFrom1.axis = cp_1->axis;
										}
										else
										{
											// let's try two verticals
											cp_2->axis = change_verticality(cp_2->axis);
											cpFrom2.axis = cp_2->axis;
											// update vertical
											newPlatform.cartPoints[0] = cpFrom2;
											// just make sure they are in opposite directions so we end up with three platforms on top of each other
											int orgCp1Dir = cp_1->dir;
											cp_1->dir = -cp_2->dir;
											cpFrom1.dir = -cp_1->dir;
											if (find_placement_and_side_for(cpFrom1, newPlatform, cpFrom1.tilePlacement, cpFrom1.side))
											{
												newPlatform.cartPoints.push_back(cpFrom1);
												ok = true;
											}
											else
											{
												// alright, we will need two platforms that share completely new cart
												// remember to restore axis and direction!
												cp_1->axis = change_verticality(cp_1->axis);
												cpFrom1.axis = cp_1->axis;
												cp_1->dir = orgCp1Dir;
											}
										}
									}
								}
							}
						}
						if (ok)
						{
							int np = platforms.get_size();
							platforms.push_back(newPlatform);

							if (!add_cart_between(p_1, np, cpFrom1.axis, -cpFrom1.dir) ||
								!add_cart_between(p_2, np, cpFrom2.axis, -cpFrom2.dir))
							{
								an_assert(false, TXT("couldn't work this out"));
								return;
							}

							// we've managed to do it with just one platform
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
							addedPlatforms += 1;
							output(TXT("added %i when connecting two (one platform scenario)"), addedPlatforms);
#endif
							return;
						}
					}

					// cp_1 and cp_2 may be invalid beyond this point, create copies

					CartPoint cp_1_c = *cp_1;
					CartPoint cp_2_c = *cp_2;

					// fallback if we have common entry space
					{
						// add two platforms and add one new connection between them
						int p1 = platforms.get_size();
						{
							Platform platform1;
#ifdef AN_PLATFORM_DEBUG_INFO
							platform1.debugInfo = TXT("common entry space [1]");
#endif
							CartPoint cpFrom1 = cp_1_c;
							cpFrom1.dir = -cpFrom1.dir;
							platform1.cartPoints.push_back(cpFrom1);

							platforms.push_back(platform1);
						}

						int p2 = platforms.get_size();
						{
							Platform platform2;
#ifdef AN_PLATFORM_DEBUG_INFO
							platform2.debugInfo = TXT("common entry space [2]");
#endif
							CartPoint cpFrom2 = cp_2_c;
							cpFrom2.dir = -cpFrom2.dir;
							platform2.cartPoints.push_back(cpFrom2);

							platforms.push_back(platform2);
						}

						// add extra connection between them - anything random that works fine will do
						int const triesAll = 100;
						int triesLeft = triesAll;
						while (triesLeft--)
						{
							// try randomly from both sides as we might get more lucky on the other side
							bool do_1 = random.get_chance(0.5f);
							int pA = do_1 ? p1 : p2;
							int pB = do_1 ? p2 : p1;
							auto & cp__c = do_1 ? cp_1_c : cp_2_c;
							CartPoint cpAtA;
							if ((triesLeft < triesAll - 20 && (random.get_chance(platformsSetup.useVerticals.get() * 0.4f))) || triesLeft < 10 || forceVerticals)
							{
								cpAtA.axis = random.get_chance(0.5f) ? CA_VerticalX : CA_VerticalY;
							}
							else
							{
								cpAtA.axis = random.get_chance(0.5f) ? CA_BackForth : CA_LeftRight;
							}
							cpAtA.side = random.get_chance(0.5f) ? -1 : 1;
							cpAtA.dir = random.get_chance(0.5f) ? -1 : 1;
							if (cpAtA.axis == cp__c.axis)
							{
								// to have both exits on platform in opposite directions to disallow overlapping
								// although it may still be the case because of cpAtB - we will check for multiple in one dir later on
								cpAtA.dir = cp__c.dir;
							}
							if (find_placement_and_side_for(cpAtA, platforms[pA]))//, NP, 0, epCommon))
							{
								CartPoint cpAtB = cpAtA;
								cpAtB.dir = -cpAtB.dir;
								if (find_placement_and_side_for(cpAtB, platforms[pB], cpAtA.tilePlacement, cpAtA.side)) // in the same place as on platform 1
								{
									platforms[pA].cartPoints.push_back(cpAtA);
									platforms[pB].cartPoints.push_back(cpAtB);
									// just make sure we don't add something that may break stuff
									if (!platforms[pA].has_multiple_in_one_dir() &&
										!platforms[pB].has_multiple_in_one_dir())
									{
										if (!add_cart_between(p_1, p1, cp_1_c.axis, cp_1_c.dir) ||
											!add_cart_between(p_2, p2, cp_2_c.axis, cp_2_c.dir) ||
											!add_cart_between(pA, pB, cpAtA.axis, cpAtA.dir))
										{
											an_assert(false, TXT("couldn't work this out"));
											return;
										}

										// all done!
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
										addedPlatforms += 2;
										output(TXT("added %i when connecting two (two platforms scenario)"), addedPlatforms);
#endif
										return;
									}
									else
									{
										platforms[pA].cartPoints.pop_back();
										platforms[pB].cartPoints.pop_back();
									}
								}
							}
						}

						// this may fail in some rare situations
						// consider those platforms
						//	p_1 platform 4
						//	   .  *<0+ >1+
						//	   .   .   .  
						//	p_2 platform 6
						//	   <1+*D0+ .  
						//	   .   .   ^2+
						//	we will have those platforms:
						//	p1 through 12
						//	   .   .   <0+
						//	   .   .   .  
						//	p2 through 13
						//	   >0+ .   .  
						//	   .   .   .  
						//	now top row is inaccessible as there is cart line
						//	bottom row doesn't have place that could be used by both
						//	that's why we need to add extra platform that will connect p1 and p2
						//	in above example, code generated:
						//	p1
						//	   .   .   <0+
						//	   u1- .   .  
						//	p2
						//	   >0+ .   .  
						//	   .   .   v1+
						//	middle platform
						//	   .   .   .  
						//	   d0- .   ^1+
						// remove both fillers we added
						platforms.set_size(platforms.get_size() - 2);
					}
				}
			}

			// cp_1 and cp_1 may be way out of date as we could modify platforms array
			{
				// we either don't have overlapping entry tiles or something went wrong

				CartPoint *cp_1 = platforms[p_1].find_cart_point_unconnected(axis_1, dir_1);
				an_assert(cp_1, TXT("can't connect those platforms. what are we trying to do then?"));

				CartPoint *cp_2 = platforms[p_2].find_cart_point_unconnected(axis_2, dir_2);
				an_assert(cp_2, TXT("can't connect those platforms. what are we trying to do then?"));

				{	// let's try if we could have three platforms
					// this should be only when cart axes are same
					// what we're doing here:
					//	we add two "entry" platforms that will use perpendicular direction to one that was meant to use to connect platforms
					//	add then third platform that will connect those two "entry" platforms
					if (!(is_entry_x(cp_1->axis) ^ is_entry_x(cp_2->axis)))
					{
						// copy
						CartPoint cp_1_c = *cp_1;
						CartPoint cp_2_c = *cp_2;

						// adjacent would have doubled/reversed cart points (as above)

						// on the way, add cart points that are on perpendicular axes and are placed among entry tiles
						RangeInt2 ep_1 = cp_1->get_entry_tiles(this);
						RangeInt2 ep_2 = cp_2->get_entry_tiles(this);

						int const triesAll = 100;
						int triesLeft = triesAll;
						while (triesLeft --)
						{
							int keepSize = platforms.get_size();

							int p1 = platforms.get_size();
							{
								Platform platform1;
#ifdef AN_PLATFORM_DEBUG_INFO
								platform1.debugInfo = String::printf(TXT("%i [1] to middle"), p_1);
#endif
								CartPoint cpFrom1 = cp_1_c;
								cpFrom1.dir = -cpFrom1.dir;
								platform1.cartPoints.push_back(cpFrom1);

								platforms.push_back(platform1);
							}

							int p2 = platforms.get_size();
							{
								Platform platform2;
#ifdef AN_PLATFORM_DEBUG_INFO
								platform2.debugInfo = String::printf(TXT("%i [2] to middle"), p_2);
#endif
								CartPoint cpFrom2 = cp_2_c;
								cpFrom2.dir = -cpFrom2.dir;
								platform2.cartPoints.push_back(cpFrom2);

								platforms.push_back(platform2);
							}

							// and now we should create cart points using different axes

							float alteredUseVerticals = 1.0f - (1.0f - platformsSetup.useVerticals.get()) * (forceVerticals ? 0.5f : 1.0f);
							CartPoint toMiddleFromP1;
							toMiddleFromP1.axis = change_perpendicular(cp_1_c.axis);
							if (random.get_chance(alteredUseVerticals))
							{
								toMiddleFromP1.axis = change_verticality(toMiddleFromP1.axis);
							}
							if (is_vertical(toMiddleFromP1.axis))
							{
								// maybe try changing perpendicularity? for verticals this could help as we can share same edge without problem
								//if (random.get_chance(0.5f))
								{
									toMiddleFromP1.axis = change_perpendicular(toMiddleFromP1.axis);
								}
							}
							toMiddleFromP1.dir = cp_1_c.dir; // prefer same dir
							bool foundPlacementAndSideFor = find_placement_and_side_for(toMiddleFromP1, platforms[p1], NP, 0, ep_1);
							if (!foundPlacementAndSideFor)
							{
								if (toMiddleFromP1.axis != cp_1_c.axis)
								{
									toMiddleFromP1.dir = -toMiddleFromP1.dir;
									foundPlacementAndSideFor = find_placement_and_side_for(toMiddleFromP1, platforms[p1], NP, 0, ep_1);
								}
							}
							if (foundPlacementAndSideFor)
							{
								CartPoint toMiddleFromP2;
								toMiddleFromP2.axis = change_perpendicular(cp_2_c.axis);
								if (random.get_chance(alteredUseVerticals))
								{
									toMiddleFromP2.axis = change_verticality(toMiddleFromP2.axis);
								}
								toMiddleFromP2.dir = random.get_chance(0.5f) ? -1 : 1;
								toMiddleFromP2.dir = cp_2_c.dir; // prefer going in same direction?
								if ((is_vertical(toMiddleFromP1.axis) &&
									 is_vertical(toMiddleFromP2.axis)) || // always do opposite direction for verticals (if verticals, they would have to share same placement, non verticals can go up and down and separate)
									 random.get_chance(0.2f))
								{
									toMiddleFromP2.dir = -toMiddleFromP1.dir; 
								}
								// if both have same axis, they have to have different direction!
								if (toMiddleFromP2.axis == toMiddleFromP1.axis)
								{
									toMiddleFromP2.dir = -toMiddleFromP1.dir;
								}
								bool foundPlacementAndSideFor = find_placement_and_side_for(toMiddleFromP2, platforms[p2], NP, 0, ep_2);
								if (!foundPlacementAndSideFor)
								{
									if (toMiddleFromP2.axis != cp_2_c.axis)
									{
										// try opposing direction (if not verticals and not same axis)
										if (!(is_vertical(toMiddleFromP1.axis) &&
											  is_vertical(toMiddleFromP2.axis)) &&
											toMiddleFromP1.axis != toMiddleFromP2.axis)
										{
											toMiddleFromP2.dir = -toMiddleFromP2.dir;
											foundPlacementAndSideFor = find_placement_and_side_for(toMiddleFromP2, platforms[p2], NP, 0, ep_2);
										}
									}
								}
								if (foundPlacementAndSideFor)
								{
									if (platforms[p1].can_add_to_have_unique_dirs(toMiddleFromP1) &&
										platforms[p2].can_add_to_have_unique_dirs(toMiddleFromP2))
									{
										// add middle platform
										Platform middlePlatform;
#ifdef AN_PLATFORM_DEBUG_INFO
										middlePlatform.debugInfo = TXT("middle platform");
#endif

										{
											CartPoint fromMiddleToP1 = toMiddleFromP1;
											fromMiddleToP1.dir = -fromMiddleToP1.dir;
											middlePlatform.cartPoints.push_back(fromMiddleToP1);
										}

										{
											CartPoint fromMiddleToP2 = toMiddleFromP2;
											fromMiddleToP2.dir = -fromMiddleToP2.dir;
											if (find_placement_and_side_for(fromMiddleToP2, middlePlatform, fromMiddleToP2.tilePlacement, fromMiddleToP2.side))
											{
												middlePlatform.cartPoints.push_back(fromMiddleToP2);

												if (!middlePlatform.has_multiple_in_one_dir()) // just in any case!
												{
													// and add those we didn't add earlier
													platforms[p1].cartPoints.push_back(toMiddleFromP1);
													platforms[p2].cartPoints.push_back(toMiddleFromP2);

													int mp = platforms.get_size();
													platforms.push_back(middlePlatform);

													// connect carts
													if (!add_cart_between(p_1, p1, cp_1_c.axis, cp_1_c.dir) ||
														!add_cart_between(p_2, p2, cp_2_c.axis, cp_2_c.dir) ||
														!add_cart_between(p1, mp, toMiddleFromP1.axis, toMiddleFromP1.dir) ||
														!add_cart_between(p2, mp, toMiddleFromP2.axis, toMiddleFromP2.dir))
													{
														an_assert(false, TXT("couldn't work this out"));
														return;
													}

													// all done!
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
													addedPlatforms += 3;
													output(TXT("added %i when connecting two (three platforms scenario)"), addedPlatforms);
#endif
													return;
												}
											}
										}
									}
								}
							}

							// restore for another try
							platforms.set_size(keepSize);
						}

#ifdef AN_DEVELOPMENT
						output(TXT("platform %i"), _aIdx);
						for_every(l, platforms[_aIdx].get_layout(this))
						{
							output(TXT("  %S"), l->to_char());
						}
						output(TXT("platform %i"), _bIdx);
						for_every(l, platforms[_bIdx].get_layout(this))
						{
							output(TXT("  %S"), l->to_char());
						}
						output(TXT("connecting platforms %i and %i --------------------"), _aIdx, _bIdx);
#endif
						todo_important(TXT("this was supposed to work!\n\nwe still have fail safe, but we should avoid using total randomness..."));
					}
				}
			}
		}

		// this is fail safe, just add random platform and hope that we will have it covered
		{
			// just add random platform on either side
			bool doA = random.get_chance(0.5f);
			int & p = doA ? _aIdx : _bIdx;
			CartAxis & axis = doA ? _aUsingAxis : _bUsingAxis;
			int & dir = doA ? _aDir : _bDir;

			int & o_p = ! doA ? _aIdx : _bIdx;
			CartAxis & o_axis = ! doA ? _aUsingAxis : _bUsingAxis;
			int & o_dir = ! doA ? _aDir : _bDir;

			CartPoint *cp = platforms[p].find_cart_point_unconnected(axis, dir);
			an_assert(cp, TXT("can't connect those platforms. what are we trying to do then?"));

			CartPoint *o_cp = platforms[o_p].find_cart_point_unconnected(o_axis, o_dir);
			an_assert(o_cp, TXT("can't connect those platforms. what are we trying to do then?"));

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
			output(TXT("connect A %i %c"), p, get_dir_as_char(axis, dir));
			for_every(l, platforms[p].get_layout(this))
			{
				output(TXT("  %S"), l->to_char());
			}
			output(TXT("connect B %i %c"), o_p, get_dir_as_char(o_axis, o_dir));
			for_every(l, platforms[o_p].get_layout(this))
			{
				output(TXT("  %S"), l->to_char());
			}
#endif

			// create new platform
			Platform newPlatform;
			newPlatform.type = PT_Filler;
#ifdef AN_PLATFORM_DEBUG_INFO
			newPlatform.debugInfo = TXT("random failsafe");
#endif

			// add cart point that matches our current dir
			CartPoint inCartPoint;
			inCartPoint.axis = axis;
			inCartPoint.dir = -dir;
			inCartPoint.side = cp->side;
			inCartPoint.tilePlacement = cp->tilePlacement;
			newPlatform.cartPoints.push_back(inCartPoint);

			CartPoint outCartPoint;
			bool addRandom = true;
			{
				outCartPoint.axis = o_axis;
				outCartPoint.dir = -o_dir;

				if (find_placement_and_side_for(outCartPoint, newPlatform, o_cp->tilePlacement, o_cp->side))
				{
					addRandom = false;
				}
			}
			if (addRandom)
			{
				bool ok = false;
				// first will try to match dir to end corridor
				// if that won't work, will try to make a turn
				// this is really important in 3x2 scenario that doesn't leave much room, see:
				//
				//		---X
				//		 ***
				//
				//		at this point we can only go down
				//						 |		 |
				//		---X			.|*		 |X--
				//		 .X*	->		.X*	->	.X*		-> we will get into same situation (which may sometimes happen and that's ok)
				//		  |
				//
				//		or
				//						|			|
				//		---X			|X---		|*X
				//		 X**	->		X*.		or	X*|		or something else
				//		 |							  |
				//
				for_count(int, tryToSide, 2)
				{
					int dirTriesLeft = 50;
					while (!ok && (dirTriesLeft--) > 0)
					{
						float choose = random.get_float(0.0f, 1.0f);
						if (!tryToSide && choose < 0.3f) magic_number // meeting along the way
						{
							outCartPoint.axis = o_axis;
							outCartPoint.dir = -o_dir; // no minus here might be used to build staircases
						}
						else if (!tryToSide && choose < 0.6f) magic_number // go along original axis
						{
							outCartPoint.axis = doA ? orgBAxis : orgAAxis;
							outCartPoint.dir = doA ? -orgBDir : -orgADir; // no minus here might be used to build staircases
						}
						else
						{
							if (axis == CA_VerticalX || axis == CA_VerticalY)
							{
								outCartPoint.axis = axis == CA_VerticalY? CA_BackForth : CA_LeftRight; // turn
							}
							else if (random.get_chance(0.5f))
							{
								outCartPoint.axis = axis == CA_LeftRight ? CA_VerticalX : CA_VerticalY; // turn
							}
							else
							{
								outCartPoint.axis = axis == CA_LeftRight ? CA_BackForth : CA_LeftRight; // turn
							}
							if (outCartPoint.axis != inCartPoint.axis)
							{
								outCartPoint.dir = random.get_chance(0.5f) ? -1 : 1;
							}
							else
							{
								// prefer going in same direction as was going
								outCartPoint.dir = (random.get_chance(0.8f) ? -1 : 1) * inCartPoint.dir; magic_number
							}
						}
						int placementAndSideTriesLeft = 50;
						while (!ok && (placementAndSideTriesLeft--) > 0)
						{
							ok = find_placement_and_side_for(outCartPoint, newPlatform);
						}
					}
					if (ok)
					{
						break;
					}
				}
				an_assert(ok);
			}
			newPlatform.cartPoints.push_back(outCartPoint);
			
			// add platform
			platforms.push_back(newPlatform);
			int np = platforms.get_size() - 1;
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
			output(TXT("added platform %i from dir %i towards %i"), platforms.get_size() - 1, p, o_p);
			for_every(l, platforms[np].get_layout(this))
			{
				output(TXT("  %S"), l->to_char());
			}
#endif

			--_addExtraPlatforms;
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
			++ addedPlatforms;
#endif

			// add cart between our current and newly created platforms
			if (!add_cart_between(p, np, axis, dir))
			{
				an_assert(false, TXT("couldn't work this out"));
				return;
			}

			// move 
			an_assert(np != NONE);
			p = np;
			axis = outCartPoint.axis;
			dir = outCartPoint.dir;

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
			output(TXT("will continue with %i %c (to %i %c)"), p, get_dir_as_char(axis, dir), o_p, get_dir_as_char(o_axis, o_dir));
#endif
		}
	}
}

#define TILE_CART_LANE 1

static inline int tile_idx(PlatformsLinear* _generator, int _x, int _y)
{
	RoomGenerationInfo const & rggi = _generator->get_room_generation_info();
	return _x + rggi.get_tile_count().x * _y;
}

RangeInt2 PlatformsLinear::calc_range_for(CartPoint const & _cp)
{
	RoomGenerationInfo const & rggi = get_room_generation_info();
	RangeInt2 laneRange = RangeInt2::empty;
	if (_cp.axis == CA_BackForth)
	{
		laneRange.x.min = laneRange.x.max = _cp.tilePlacement.x;
		if (_cp.dir < 0)
		{
			laneRange.y.min = 0;
			laneRange.y.max = _cp.tilePlacement.y;
		}
		if (_cp.dir > 0)
		{
			laneRange.y.min = _cp.tilePlacement.y;
			laneRange.y.max = rggi.get_tile_count().y - 1;
		}
	}
	else if (_cp.axis == CA_LeftRight)
	{
		laneRange.y.min = laneRange.y.max = _cp.tilePlacement.y;
		if (_cp.dir < 0)
		{
			laneRange.x.min = 0;
			laneRange.x.max = _cp.tilePlacement.x;
		}
		if (_cp.dir > 0)
		{
			laneRange.x.min = _cp.tilePlacement.x;
			laneRange.x.max = rggi.get_tile_count().x - 1;
		}
	}
	else if (_cp.axis == CA_VerticalX || _cp.axis == CA_VerticalY)
	{
		laneRange.x.min = laneRange.x.max = _cp.tilePlacement.x;
		laneRange.y.min = laneRange.y.max = _cp.tilePlacement.y;
	}
	else
	{
		todo_important(TXT("implement_"));
	}
	return laneRange;
}

bool PlatformsLinear::validate_placement_and_side_for(CartPoint& _cp, Platform const & _platform)
{
	return find_placement_and_side_for(_cp, _platform, _cp.tilePlacement, _cp.side);
}

bool PlatformsLinear::find_placement_and_side_for(CartPoint& _cp, Platform const & _platform, Optional<VectorInt2> const & _preferredTilePlacement, int _preferredSide, Optional<RangeInt2> const & _amongTilePlacements)
{
	RoomGenerationInfo const & rggi = get_room_generation_info();
	ARRAY_STACK(int, tiles, rggi.get_tile_count().x * rggi.get_tile_count().y);
	tiles.set_size(rggi.get_tile_count().x * rggi.get_tile_count().y);
	for_every(tile, tiles)
	{
		*tile = 0; // just clear them
	}
	// build platform side, min plaform size (already occupied)
	// and build existing lanes layout
	// these variables tell where are platform sides (if there are cartpoints, they are used)
	// any other limitations (tilesAvailable) should be used when randomising cart point placement
	int leftSide = NONE;
	int rightSide = NONE;
	int nearSide = NONE;
	int farSide = NONE;
	RangeInt2 minPlatformSize = RangeInt2::empty;
	for_every(cp, _platform.cartPoints)
	{
		if (is_vertical(_cp.axis) && is_vertical(cp->axis))
		{
			if (_cp.dir == cp->dir)
			{
				// disallow having two vertical carts heading in same direction (we will have overlapping platforms!)
				return false;
			}
		}
		if (cp->axis == CA_BackForth || cp->axis == CA_VerticalX)
		{
			if (cp->side < 0)
			{
				minPlatformSize.x.include(cp->tilePlacement.x + 1);
				minPlatformSize.y.include(cp->tilePlacement.y);
				leftSide = cp->tilePlacement.x + 1;
				an_assert(leftSide >= 0 && leftSide < rggi.get_tile_count().x);
			}
			if (cp->side > 0)
			{
				minPlatformSize.x.include(cp->tilePlacement.x - 1);
				minPlatformSize.y.include(cp->tilePlacement.y);
				rightSide = cp->tilePlacement.x - 1;
				an_assert(rightSide >= 0 && rightSide < rggi.get_tile_count().x);
			}
		}
		if (cp->axis == CA_LeftRight || cp->axis == CA_VerticalY)
		{
			if (cp->side < 0)
			{
				minPlatformSize.x.include(cp->tilePlacement.x);
				minPlatformSize.y.include(cp->tilePlacement.y + 1);
				nearSide = cp->tilePlacement.y + 1;
				an_assert(nearSide >= 0 && nearSide < rggi.get_tile_count().y);
			}
			if (cp->side > 0)
			{
				minPlatformSize.x.include(cp->tilePlacement.x);
				minPlatformSize.y.include(cp->tilePlacement.y - 1);
				farSide = cp->tilePlacement.y - 1;
				an_assert(farSide >= 0 && farSide < rggi.get_tile_count().y);
			}
		}
		RangeInt2 laneRange = calc_range_for(*cp);
		if (!laneRange.is_empty())
		{
			for_range(int, x, laneRange.x.min, laneRange.x.max)
			{
				for_range(int, y, laneRange.y.min, laneRange.y.max)
				{
					tiles[tile_idx(this, x, y)] |= TILE_CART_LANE;
				}
			}
		}
	}

	// find new placement and side
	VectorInt2 proposedPlacement;
	if (_preferredTilePlacement.is_set() || _amongTilePlacements.is_set())
	{
		// check if proposed placement isn't within existing platform and is on edge (if there is an edge)
		if (_preferredTilePlacement.is_set())
		{
			an_assert(_preferredSide != 0);
			proposedPlacement = _preferredTilePlacement.get();
		}
		else if (_amongTilePlacements.is_set())
		{
			int triesLeft = 10;
			int preferredSide = _preferredSide;
			while (triesLeft--)
			{
				if (_preferredSide == 0)
				{
					preferredSide = random.get_chance(0.5f) ? -1 : 1;
				}
				an_assert(!_amongTilePlacements.get().is_empty());
				RangeInt2 usePlacement = _amongTilePlacements.get();
				if (is_entry_x(_cp.axis))
				{
					proposedPlacement.x = preferredSide < 0 ? usePlacement.x.min : usePlacement.x.max;
					proposedPlacement.y = random.get_int_from_range(usePlacement.y.min, usePlacement.y.max);
				}
				else
				{
					proposedPlacement.x = random.get_int_from_range(usePlacement.x.min, usePlacement.x.max);
					proposedPlacement.y = preferredSide < 0 ? usePlacement.y.min : usePlacement.y.max; 
				}
				if (! minPlatformSize.does_contain(proposedPlacement))
				{
					_preferredSide = preferredSide;
					break;
				}
			}
		}
		if (minPlatformSize.does_contain(proposedPlacement))
		{
			return false;
		}
		int checkPlacement = proposedPlacement.x;
		int o_checkPlacement = proposedPlacement.y;
		int minSide = NONE;
		int maxSide = NONE;
		int o_minSide = NONE;
		int o_maxSide = NONE;
		if (_cp.axis == CA_BackForth || _cp.axis == CA_VerticalX)
		{
			checkPlacement = proposedPlacement.x;
			minSide = leftSide;
			maxSide = rightSide;
			o_checkPlacement = proposedPlacement.y;
			o_minSide = nearSide;
			o_maxSide = farSide;
		}
		else if (_cp.axis == CA_LeftRight || _cp.axis == CA_VerticalY)
		{
			checkPlacement = proposedPlacement.y;
			minSide = nearSide;
			maxSide = farSide;
			o_checkPlacement = proposedPlacement.x;
			o_minSide = leftSide;
			o_maxSide = rightSide;
		}
		else
		{
			todo_important(TXT("implement_"));
		}

		if (_preferredSide < 0)
		{
			if ((minSide == NONE && (maxSide == NONE || maxSide > checkPlacement)) || // is there enough space to fit platform
				(minSide != NONE && minSide == checkPlacement + 1)) // has to match
			{
				// all ok
			}
			else
			{
				return false;
			}
		}
		if (_preferredSide > 0)
		{
			if ((maxSide == NONE && (minSide == NONE || minSide < checkPlacement)) || // is there enough space to fit platform
				(maxSide != NONE && maxSide == checkPlacement - 1)) // has to match
			{
				// all ok
			}
			else
			{
				return false;
			}
		}
		// check if is within platform
		if (o_minSide != NONE && o_checkPlacement < o_minSide)
		{
			return false;
		}
		if (o_maxSide != NONE && o_checkPlacement > o_maxSide)
		{
			return false;
		}
	}
	else
	{
		// find placement along edge (if one exists)
		// and try to avoid lanes that go in the opposite direction
		an_assert(_preferredSide == 0);
		if (_cp.axis == CA_BackForth ||
			_cp.axis == CA_LeftRight ||
			true)
		{
			bool entryX = is_entry_x(_cp.axis);
			_preferredSide = random.get_chance(0.5f) ? -1 : 1;
			int o_start;
			int o_end;
			bool found = false;
			// try both sides, if we won't be able to work it out with one side, other one might be ok
			for_count(int, trySide, 2)
			{
				if (trySide == 1 && random.get_chance(0.05f))
				{
					// don't do that, just live with "it can't be done"
					break;
				}
				int const minSide = entryX ? leftSide : nearSide;
				int const maxSide = entryX ? rightSide : farSide;
				int o_minSide = entryX ? nearSide : leftSide;
				int o_maxSide = entryX ? farSide : rightSide;
				int const tileCount = entryX ? rggi.get_tile_count().x : rggi.get_tile_count().y;
				int const o_tileCount = entryX ? rggi.get_tile_count().y : rggi.get_tile_count().x;
				auto const & minPlatform = entryX ? minPlatformSize.x : minPlatformSize.y;
				int & placement = entryX ? proposedPlacement.x : proposedPlacement.y;
				RangeInt const & tilesAvailable = entryX ? _platform.tilesAvailable.x : _platform.tilesAvailable.y;
				RangeInt const & o_tilesAvailable = entryX ? _platform.tilesAvailable.y : _platform.tilesAvailable.x;

				if (minSide == 0)
				{
					_preferredSide = 1;
				}
				if (maxSide == tileCount - 1)
				{
					_preferredSide = -1;
				}
				if (_preferredSide < 0)
				{
					if (minPlatform.min <= 0)
					{
						_preferredSide = -_preferredSide; // try opposite side
						continue;
					}
					if (rggi.is_small())
					{
						placement = 0;	// always on sides!
					}
					else
					{
						if (minSide != NONE)
						{
							placement = minSide - 1;
						}
						else
						{
							int minAvailable = 0;
							int maxAvailable = (!minPlatform.is_empty() ? minPlatform.min - 1 : tileCount - 2 /* leave tile */) / 2;
							if (!tilesAvailable.is_empty())
							{
								minAvailable = clamp(minAvailable, tilesAvailable.min, tilesAvailable.max);
								maxAvailable = clamp(maxAvailable, tilesAvailable.min, tilesAvailable.max);
							}
							placement = random.get_int_from_range(minAvailable, maxAvailable);
						}
					}
				}
				if (_preferredSide > 0)
				{
					if (minPlatform.max >= tileCount - 1)
					{
						_preferredSide = -_preferredSide; // try opposite side
						continue;
					}
					if (rggi.is_small())
					{
						placement = tileCount - 1;	// always on sides!
					}
					else
					{
						if (maxSide != NONE)
						{
							placement = maxSide + 1;
						}
						else
						{
							int minAvailable = tileCount - 1 - (tileCount - 1 - (!minPlatform.is_empty() ? minPlatform.max + 1 : 1 /* leave tile */)) / 2;
							int maxAvailable = tileCount - 1;
							if (!tilesAvailable.is_empty())
							{
								minAvailable = clamp(minAvailable, tilesAvailable.min, tilesAvailable.max);
								maxAvailable = clamp(maxAvailable, tilesAvailable.min, tilesAvailable.max);
							}
							placement = random.get_int_from_range(minAvailable, maxAvailable);
						}
					}
				}
				if (o_minSide == NONE) o_minSide = 0;
				if (o_maxSide == NONE) o_maxSide = o_tileCount - 1;
				if (!o_tilesAvailable.is_empty())
				{
					o_minSide = clamp(o_minSide, o_tilesAvailable.min, o_tilesAvailable.max);
					o_maxSide = clamp(o_maxSide, o_tilesAvailable.min, o_tilesAvailable.max);
				}
				o_start = _cp.dir < 0 ? o_minSide : o_maxSide;
				o_end = o_start;
				int o_tooFar = _cp.dir < 0 ? o_maxSide + 1 : o_minSide - 1;
				an_assert(_cp.dir != 0);
				found = false;
				for (int o = o_start; o != o_tooFar; o -= _cp.dir)
				{
					if ((entryX ? tiles[tile_idx(this, proposedPlacement.x, o)] : tiles[tile_idx(this, o, proposedPlacement.y)]) & TILE_CART_LANE)
					{
						break;
					}
					o_end = o;
					found = true;
				}
				if (!found)
				{
					_preferredSide = -_preferredSide; // try opposite side
					continue;
				}
				break;
			}
			if (!found)
			{
				return false;
			}
			if (random.get_chance(0.05f) || rggi.is_small()) // for small prefer middle as we want to have as much place available as possible
			{
				if (o_end - o_start <= 1)
				{
					o_end = random.get_bool() ? o_start : o_end; // but if we have one or two options, choose at random
					o_start = o_end;
				}
				else
				{
					o_start = o_end = (o_start + o_end) / 2; // at centre
				}
			}
			else if (random.get_chance(0.8f))
			{
				if (o_end - o_start <= 1)
				{
					o_end = random.get_bool() ? o_start : o_end; // but if we have one or two options, choose at random
					o_start = o_end;
				}
				else
				{
					o_start = o_end + (o_start - o_end) / 2; // move closer to end (in direction of where this cart is going)
				}
			}
			int & o_placement = entryX ? proposedPlacement.y : proposedPlacement.x;
			if (o_start <= o_end)
			{
				o_placement = random.get_int_from_range(o_start, o_end);
			}
			else
			{
				o_placement = random.get_int_from_range(o_end, o_start);
			}
		}
		else
		{
			todo_important(TXT("implement_"));
		}
	}

	// check if we're still in valid range
	if (_platform.tilesAvailable.x.is_empty())
	{
		if (proposedPlacement.x < 0 || proposedPlacement.x > rggi.get_tile_count().x - 1)
		{
			return false;
		}
	}
	else
	{
		if (proposedPlacement.x < _platform.tilesAvailable.x.min || proposedPlacement.x > _platform.tilesAvailable.x.max)
		{
			return false;
		}
	}
	if (_platform.tilesAvailable.y.is_empty())
	{
		if (proposedPlacement.y < 0 || proposedPlacement.y > rggi.get_tile_count().y - 1)
		{
			return false;
		}
	}
	else
	{
		if (proposedPlacement.y < _platform.tilesAvailable.y.min || proposedPlacement.y > _platform.tilesAvailable.y.max)
		{
			return false;
		}
	}

	// check if we're on the allowed side
	if (_platform.blockCartSide)
	{
		if ((_platform.blockCartSide & PD_Back) &&
			(_cp.axis == CA_LeftRight || _cp.axis == CA_VerticalY) && _preferredSide < 0)
		{
			return false;
		}
		if ((_platform.blockCartSide & PD_Forth) &&
			(_cp.axis == CA_LeftRight || _cp.axis == CA_VerticalY) && _preferredSide > 0)
		{
			return false;
		}
		if ((_platform.blockCartSide & PD_Left) &&
			(_cp.axis == CA_BackForth || _cp.axis == CA_VerticalX) && _preferredSide < 0)
		{
			return false;
		}
		if ((_platform.blockCartSide & PD_Right) &&
			(_cp.axis == CA_BackForth || _cp.axis == CA_VerticalX) && _preferredSide > 0)
		{
			return false;
		}
	}

	// check if we do not cross any existing lane
	an_assert(_preferredSide != 0);
	_cp.tilePlacement = proposedPlacement;
	_cp.side = _preferredSide;
	if (_cp.axis == CA_BackForth || _cp.axis == CA_VerticalX)
	{
		if (_cp.side < 0 && _cp.tilePlacement.x >= rggi.get_tile_count().x - 1)
		{
			an_assert(false);
			return false;
		}
		if (_cp.side > 0 && _cp.tilePlacement.x < 1)
		{
			an_assert(false);
			return false;
		}
	}
	else if (_cp.axis == CA_LeftRight || _cp.axis == CA_VerticalY)
	{
		if (_cp.side < 0 && _cp.tilePlacement.y >= rggi.get_tile_count().y - 1)
		{
			an_assert(false);
			return false;
		}
		if (_cp.side > 0 && _cp.tilePlacement.y < 1)
		{
			an_assert(false);
			return false;
		}
	}
	RangeInt2 newLaneRange = calc_range_for(_cp);
	for_range(int, x, newLaneRange.x.min, newLaneRange.x.max)
	{
		for_range(int, y, newLaneRange.y.min, newLaneRange.y.max)
		{
			if (tiles[tile_idx(this, x, y)] & TILE_CART_LANE)
			{
				// lines would cross
				return false;
			}
		}
	}

	// check if all other cart points are in front of us
	VectorInt2 dir = VectorInt2::zero;
	if (_cp.axis == CA_BackForth || _cp.axis == CA_VerticalX) dir.x = -_cp.side;
	if (_cp.axis == CA_LeftRight || _cp.axis == CA_VerticalY) dir.y = -_cp.side;
	for_every(cp, _platform.cartPoints)
	{
		VectorInt2 diff = cp->tilePlacement - _cp.tilePlacement;
		if (VectorInt2::dot(diff, dir) < 0)
		{
			return false;
		}
		if (VectorInt2::dot(diff, dir) == 0)
		{
			if (((_cp.axis == CA_BackForth || _cp.axis == CA_VerticalX) && (cp->axis == CA_BackForth || cp->axis == CA_VerticalX)) ||
				((_cp.axis == CA_LeftRight || _cp.axis == CA_VerticalY) && (cp->axis == CA_LeftRight || cp->axis == CA_VerticalY)))
			{
				// ok
			}
			else
			{
				return false;
			}
		}
	}

#ifdef AN_DEVELOPMENT
	for_every(cp, _platform.cartPoints)
	{
		an_assert(cp->tilePlacement != _cp.tilePlacement);
	}
#endif
	// it seems we're fine
	return true;
}

bool PlatformsLinear::add_cart_between(int _aIdx, int _bIdx, Optional<CartAxis> const & _usingAxis, int _a2bDir)
{
	auto & pA = platforms[_aIdx];
	auto & pB = platforms[_bIdx];
	for_every(cpA, pA.cartPoints)
	{
		if (cpA->cartIdx != NONE)
		{
			continue;
		}
		if (_usingAxis.is_set() &&
			cpA->axis != _usingAxis.get())
		{
			continue;
		}
		if (_a2bDir != 0 &&
			_a2bDir != cpA->dir)
		{
			continue;
		}
		for_every(cpB, pB.cartPoints)
		{
			if (cpB->cartIdx != NONE)
			{
				continue;
			}
			if (cpA->axis == cpB->axis &&
				cpA->tilePlacement == cpB->tilePlacement &&
				cpA->side == cpB->side &&
				cpA->dir * cpB->dir < 0) // opposite
			{
				Cart cart;
				cart.axis = cpA->axis;
				cart.minus.platformIdx = _aIdx;
				cart.minus.cartPointIdx = for_everys_index(cpA);
				cart.plus.platformIdx = _bIdx;
				cart.plus.cartPointIdx = for_everys_index(cpB);
				carts.push_back(cart);
				// and store cart index in cart points
				cpA->cartIdx = cpB->cartIdx = carts.get_size() - 1;
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
				output(TXT("added cart between platforms %i.%i (%S) and %i.%i (%S)"), _aIdx, for_everys_index(cpA), cpA->tilePlacement.to_string().to_char(), _bIdx, for_everys_index(cpB), cpB->tilePlacement.to_string().to_char());
#endif
				return true;
			}
		}
	}
	return false;
}

void PlatformsLinear::place_initial_platforms(ArrayStack<int> & _platformsToPlace)
{
	RoomGenerationInfo const& rggi = get_room_generation_info();

	if (!platformsIn.is_empty())
	{
		// place "in" platforms in the world
		{
			float atWall = 0.0f;
			float atX = 0.0f;
			for_every(in, platformsIn)
			{
				atX += random.get_float(3.0f, 6.0f) * rggi.get_tile_size(); magic_number
				platforms[*in].at = Vector3(atX, atWall, 0.0f);
				platforms[*in].placed = true;
				_platformsToPlace.push_back(*in);
				return;
			}
		}

		for_every(in, platformsIn)
		{
			_platformsToPlace.push_back(*in);
		}

		return;
	}

	if (!platformsOut.is_empty())
	{
		// place "in" platforms in the world
		{
			float atWall = 0.0f;
			float atX = 0.0f;
			for_every(in, platformsOut)
			{
				atX += random.get_float(3.0f, 6.0f) * rggi.get_tile_size(); magic_number
				platforms[*in].at = Vector3(atX, atWall, 0.0f);
				platforms[*in].placed = true;
				_platformsToPlace.push_back(*in);
				return;
			}
		}

		for_every(in, platformsOut)
		{
			_platformsToPlace.push_back(*in);
		}

		return;
	}

	if (!platformsLeft.is_empty())
	{
		// place "in" platforms in the world
		{
			float atWall = 0.0f;
			float atY = 0.0f;
			for_every(in, platformsLeft)
			{
				atY += random.get_float(3.0f, 6.0f) * rggi.get_tile_size(); magic_number
				platforms[*in].at = Vector3(atWall, atY, 0.0f);
				platforms[*in].placed = true;
				_platformsToPlace.push_back(*in);
				return;
			}
		}

		for_every(in, platformsLeft)
		{
			_platformsToPlace.push_back(*in);
		}

		return;
	}

	if (!platformsRight.is_empty())
	{
		// place "in" platforms in the world
		{
			float atWall = 0.0f;
			float atY = 0.0f;
			for_every(in, platformsRight)
			{
				atY += random.get_float(3.0f, 6.0f) * rggi.get_tile_size(); magic_number
				platforms[*in].at = Vector3(atWall, atY, 0.0f);
				platforms[*in].placed = true;
				_platformsToPlace.push_back(*in);
				return;
			}
		}

		for_every(in, platformsRight)
		{
			_platformsToPlace.push_back(*in);
		}

		return;
	}

	_platformsToPlace.push_back(0);
}

void PlatformsLinear::place_platforms()
{
	// build platforms list starting with "in"s
	ARRAY_STACK(int, platformsToPlace, platforms.get_size());

	place_initial_platforms(platformsToPlace);

	{
		int placedCount = 0;
		for_every(platform, platforms)
		{
			if (platform->placed)
			{
				++placedCount;
			}
		}

		// have at least one platform placed
		if (placedCount == 0)
		{
			an_assert(platformsToPlace.is_empty());
			platforms[0].at = Vector3::zero;
			platforms[0].placed = true;
			platformsToPlace.push_back(0);
		}
	}

	// add more platforms to the "platforms to place" array
	for (int i = 0; i < platformsToPlace.get_size(); ++i)
	{
		int ptpIdx = platformsToPlace[i];
		auto const & platform = platforms[ptpIdx];
		for (int cp = 0; cp < platform.cartPoints.get_size(); ++cp)
		{
			int otherPlatformIdx = find_platform_on_other_end_of_cart_line(ptpIdx, cp);
			if (otherPlatformIdx != NONE)
			{
				platformsToPlace.push_back_unique(otherPlatformIdx);
			}
		}
	}

	ARRAY_STACK(bool, placedPlatforms, platforms.get_size());
	for_every(platform, platforms)
	{
		placedPlatforms.push_back(platform->placed);
	}

	// do this in waves placing and realigning movement axes
	// this is because realigning may move platforms dramatically and we want to keep as aligned as possible
	for_count(int, settle, 10)
	{
		{
			auto placedPlatformsIter = placedPlatforms.begin();
			for_every(platform, platforms)
			{
				platform->placed = *placedPlatformsIter;
				++placedPlatformsIter;
			}
		}

		// place platforms (and calculate local "platform" range)
		for_every(ptpIdx, platformsToPlace)
		{
			auto & platform = platforms[*ptpIdx];
			platform.calculate_platform(this, random);
			for_every(cartPoint, platform.cartPoints)
			{
				cartPoint->calculate_placement(this, &platform);
				int otherPlatformIdx = find_platform_on_other_end_of_cart_line(*ptpIdx, for_everys_index(cartPoint));
				if (otherPlatformIdx != NONE)
				{
					auto const & otherPlatform = platforms[otherPlatformIdx];
					if (otherPlatform.placed)
					{
						if (!platform.placed)
						{
							platform.at = otherPlatform.at;
							platform.at.z += random.get_float(-0.02f, 0.02f); magic_number
							if (cartPoint->axis == CA_BackForth)
							{
								platform.at.y = otherPlatform.at.y - (float)cartPoint->dir * random.get_float(2.0f, 4.0f); magic_number
							}
							if (cartPoint->axis == CA_LeftRight)
							{
								platform.at.x = otherPlatform.at.x - (float)cartPoint->dir * random.get_float(2.0f, 4.0f); magic_number
							}
							if (cartPoint->axis == CA_VerticalX ||
								cartPoint->axis == CA_VerticalY)
							{
								platform.at.z = otherPlatform.at.z - (float)cartPoint->dir * random.get_float(3.0f, 5.0f); magic_number
							}
							platform.placed = true;
						}
					}
				}
			}
		}

		align_door_platforms();
		
		// align platforms to match movement axes
		int triesLeft = 100;
		while (triesLeft--)
		{
			ARRAY_STACK(int, platformsToAlign, platforms.get_size());

			bool moved = false;

			// for each platform try to match other platforms around until we settle
			for_every(platform, platforms)
			{
				platformsToAlign.clear();
				platformsToAlign.push_back(for_everys_index(platform));
				gather_platforms_to_align(platformsToAlign, CA_LeftRight, CA_LeftRight);
				for (int i = 0; i < platformsToAlign.get_size(); ++i)
				{
					auto & platformToAlign = platforms[platformsToAlign[i]];
					if (platformToAlign.at.y != platform->at.y)
					{
						platformToAlign.at.y = lerp(0.3f, platformToAlign.at.y, platform->at.y);
						moved = true;
					}
				}

				platformsToAlign.clear();
				platformsToAlign.push_back(for_everys_index(platform));
				gather_platforms_to_align(platformsToAlign, CA_BackForth, CA_BackForth);
				for (int i = 0; i < platformsToAlign.get_size(); ++i)
				{
					auto & platformToAlign = platforms[platformsToAlign[i]];
					if (platformToAlign.at.x != platform->at.x)
					{
						platformToAlign.at.x = lerp(0.3f, platformToAlign.at.x, platform->at.x); 
						moved = true;
					}
				}

				platformsToAlign.clear();
				platformsToAlign.push_back(for_everys_index(platform));
				gather_platforms_to_align(platformsToAlign, CA_VerticalX, CA_VerticalY);
				for (int i = 0; i < platformsToAlign.get_size(); ++i)
				{
					auto & platformToAlign = platforms[platformsToAlign[i]];
					if (platformToAlign.at.x != platform->at.x ||
						platformToAlign.at.y != platform->at.y)
					{
						platformToAlign.at.x = lerp(0.3f, platformToAlign.at.x, platform->at.x);
						platformToAlign.at.y = lerp(0.3f, platformToAlign.at.y, platform->at.y);
						moved = true;
					}
				}
			}

			if (!moved)
			{
				break;
			}
		}
	}
}

void PlatformsLinear::move_platforms(Framework::Room const* _room, bool _final)
{
	RoomGenerationInfo const & rggi = get_room_generation_info();

	{	// prepare idFromin
		for_every(platform, platforms)
		{
			platform->idFromIn = NONE;
		}
		int idFromIn = 0;
		platforms[0].idFromIn = idFromIn++;
		{
			bool allDone = false;
			while (!allDone)
			{
				allDone = true;
				for_every(platform, platforms)
				{
					if (platform->idFromIn == NONE)
					{
						for_every(cp, platform->cartPoints)
						{
							int opIdx = find_platform_on_other_end_of_cart_line(for_everys_index(platform), for_everys_index(cp));
							if (platforms[opIdx].idFromIn != NONE)
							{
								platform->idFromIn = idFromIn++;
								break;
							}
						}
						if (platform->idFromIn == NONE)
						{
							allDone = false;
						}
					}
				}
			}
		}
	}

	float const alignCartLines_strength = 1.0f; // strongest
	float const separatePlatformsPairs_strength = 0.6f;
	float const separatePlatformsPairs_preferFlat = _final ? 0.0f : max(0.0f, 0.4f - platformsSetup.useVerticals.get() * 0.6f);
	float const moveAwayAlongCartLine_strength = 0.8f;
	float const moveAwayAlongCartLineTooSteep_strength = 1.2f; // strongest as it is also easiest to mess
	float const moveCloserAlongCartLine_strength = 0.1f;
	float const levelOutAlongCartLine_strength = 0.7f - platformsSetup.useVerticals.get() * 0.5f;
	float const move_strength = 0.4f;
	an_assert(moveAwayAlongCartLine_strength > separatePlatformsPairs_strength); // first move out of the way, then move to not cross platforms

	Optional<float> requestedCellSize;
	Optional<float> requestedCellWidth;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		requestedCellSize = piow->get_cell_size_inner() - hardcoded 1.0f; // better to be smaller
	}
	
	if (platformsSetup.overrideRequestedCellSize.is_set())
	{
		requestedCellSize = platformsSetup.overrideRequestedCellSize.get();
	}

	if (platformsSetup.overrideRequestedCellWidth.is_set())
	{
		requestedCellWidth = platformsSetup.overrideRequestedCellWidth.get();
	}

	if (requestedCellSize.is_set())
	{
		requestedCellWidth.set_if_not_set(requestedCellSize);
	}

	bool movementRequired = true;
	int tryNo = 0;
	while (movementRequired && tryNo < 2000)
	{
		// at some point we have to stop, otherwise we may have two platforms trying to pull towards each other
		// while on the same line there is somewhere else chain of platforms struggling to push away from each other
		// but because pulling increases with distance it becomes stronger than pushing away
		bool allowTooFar = tryNo < 1500;

		movementRequired = false;
		bool tooFar = false;

		for_every(platform, platforms)
		{
			platform->move = Vector3::zero;
		}

		// always check indices to not repeat stuff in reverse - ie. pA index < pB index

		// check each pair
		{
			for_every(pA, platforms)
			{
				for_every(pB, platforms)
				{
					if (for_everys_index(pA) < for_everys_index(pB)) // each pair just once
					{
						bool connectedByCart = false;
						for_every(cpA, pA->cartPoints)
						{
							for_every(cpB, pB->cartPoints)
							{
								if (cpA->cartIdx == cpB->cartIdx)
								{
									connectedByCart = true;
								}
							}
						}
						// for connected by cart work it out with going along lane
						if (!connectedByCart)
						{
							Vector3 diff = (pB->get_room_placement() - pA->get_room_placement());
							Vector3 pASize = pA->calculate_half_size(platformsSetup.platformHeight.get());
							Vector3 pBSize = pB->calculate_half_size(platformsSetup.platformHeight.get());
							Vector3 separation = platformsSetup.platformSeparation.get() * (_final ? 0.3f : 1.0f);
							// we have to leave enough place to have two carts fit
							if (!platformsSetup.ignoreMinimumCartsSeparation.get())
							{
								separation.x = max(separation.x, rggi.get_tile_size() * 2.6f);
								separation.y = max(separation.y, rggi.get_tile_size() * 2.6f);
							}
							Vector3 minSeparation = pASize + pBSize + separation;
							if (abs(diff.x) < minSeparation.x &&
								abs(diff.y) < minSeparation.y &&
								abs(diff.z) < minSeparation.z)
							{
								if (abs(diff.z) < minSeparation.z * 0.5f)
								{
									diff.z = pB->idFromIn > pA->idFromIn ? -1.0f : 1.0f; // to allow stair case building
								}
								if (diff.is_almost_zero())
								{
									// move randomly!
									diff.x = random.get_float(-1.0f, 1.0f);
									diff.y = random.get_float(-1.0f, 1.0f);
									diff.z = random.get_float(-1.0f, 1.0f);
								}
								Vector3 left = diff;
								left = left.normal() / left.length();
								Vector3 moveBy = left * separatePlatformsPairs_strength * 0.5f;
								{
									Vector3 m;
									m.x = random.get_float(-1.0f, 1.0f);
									m.y = random.get_float(-1.0f, 1.0f);
									m.z = random.get_float(-1.0f, 1.0f);
									moveBy += 0.02f * m; // little random bit
								}
								moveBy.z *= max(0.1f, (1.0f - separatePlatformsPairs_preferFlat));
								try_to_move(for_everys_index(pA), -moveBy);
								try_to_move(for_everys_index(pB), moveBy);
								movementRequired = true;
							}
						}
					}
				}

				movementRequired |= separate_door_platforms(for_everys_index(pA));
			}
		}

		// realign cart lines
		for_every(pA, platforms)
		{
			for_every(cp, pA->cartPoints)
			{
				int pBIdx = find_platform_on_other_end_of_cart_line(for_everys_index(pA), for_everys_index(cp));
				if (pBIdx != NONE &&
					for_everys_index(pA) < pBIdx)
				{
					auto * pB = &platforms[pBIdx];
					Vector3 diff = (pB->at - pA->at);

					Vector3 alignVector = Vector3::zero;
					if (cp->axis == CA_BackForth)
					{
						alignVector.x = 1.0f;
					}
					else if (cp->axis == CA_LeftRight)
					{
						alignVector.y = 1.0f;
					}
					else if (cp->axis == CA_VerticalX ||
						cp->axis == CA_VerticalY)
					{
						alignVector.x = 1.0f;
						alignVector.y = 1.0f;
					}
					else
					{
						todo_important(TXT("implement_"));
					}

					Vector3 moveBy = alignVector * diff;
					if (!moveBy.is_zero())
					{
						moveBy *= alignCartLines_strength * 0.5f;
						bool immediate = moveBy.length() < 0.1f;
						try_to_move(for_everys_index(pA), moveBy, immediate, pBIdx);
						try_to_move(pBIdx, -moveBy, immediate, for_everys_index(pA));
						movementRequired = true;
					}
				}
			}
		}

		// check against outgoing carts
		for_every(pA, platforms)
		{
			for_every(cp, pA->cartPoints)
			{
				int pBIdx = find_platform_on_other_end_of_cart_line(for_everys_index(pA), for_everys_index(cp));
				if (pBIdx != NONE &&
					for_everys_index(pA) < pBIdx)
				{
					auto * pB = &platforms[pBIdx];
					Vector3 diff = (pB->get_room_placement() - pA->get_room_placement());
					Vector3 atDiff = pB->at - pA->at;
					Vector3 pBSize = pB->calculate_half_size(platformsSetup.platformHeight.get());
					Vector3 pASize = pA->calculate_half_size(platformsSetup.platformHeight.get());

					Vector3 inDir = Vector3::zero;
					Vector3 thirdAxis = Vector3::zero;
					bool verticalCart = false;
					if (cp->axis == CA_BackForth)
					{
						inDir = float(cp->dir) * Vector3::yAxis;
						thirdAxis = Vector3::zAxis * (diff.z < 0.0f ? -1.0f : 1.0f);
					}
					else if (cp->axis == CA_LeftRight)
					{
						inDir = float(cp->dir) * Vector3::xAxis;
						thirdAxis = Vector3::zAxis * (diff.z < 0.0f ? -1.0f : 1.0f);
					}
					else if (cp->axis == CA_VerticalX ||
						cp->axis == CA_VerticalY)
					{
						verticalCart = true;
						inDir = float(cp->dir) * Vector3::zAxis;
					}
					else
					{
						todo_important(TXT("implement_"));
					}

					float diffAlongDir = Vector3::dot(diff, inDir);
					float atDiffAlongDir = Vector3::dot(atDiff, inDir);
					float separation = platformsSetup.cartLineSeparation.get().min * (_final ? 0.3f : 1.0f);
					// we have to leave enough place to have two carts fit
					separation = max(separation, rggi.get_tile_size() * 2.6f);
					float maxSeparation = platformsSetup.cartLineSeparation.get().max;
					maxSeparation = max(maxSeparation, rggi.get_tile_size() * 3.0f);
					float sizeAlongDir = abs(Vector3::dot(pASize, inDir)) + abs(Vector3::dot(pBSize, inDir));
					float distAlongDir = max(0.0f, diffAlongDir - sizeAlongDir);
					float steepCoef = tan_deg(platformsSetup.maxSteepAngle.get());
					if (distAlongDir < separation)
					{
						// too close
						Vector3 moveBy = inDir * (distAlongDir - separation - 0.1f);
						moveBy *= moveAwayAlongCartLine_strength * 0.5f;
						try_to_move(for_everys_index(pA), moveBy);
						try_to_move(pBIdx, -moveBy);
						movementRequired = true;
					}
					else if (!verticalCart && abs(atDiff.z) > atDiffAlongDir * steepCoef)
					{
						// too steep, will try to move all in dir to introduce huge gap
						Vector3 moveBy = inDir * (atDiffAlongDir - abs(atDiff.z) / steepCoef);
						moveBy *= moveAwayAlongCartLineTooSteep_strength * 0.5f;
						try_to_move(for_everys_index(pA), moveBy);
						try_to_move(pBIdx, -moveBy);
						movementRequired = true;
					}
					else if (distAlongDir > maxSeparation && allowTooFar &&
							 ! _final && // do not move closer on final
							 (verticalCart || abs(atDiff.z) < 0.8f * atDiffAlongDir * steepCoef)) // to not mess up with steepness
					{
						Vector3 moveBy = inDir * (distAlongDir - maxSeparation + 0.1f);
						moveBy *= moveCloserAlongCartLine_strength * 0.5f;
						try_to_move(for_everys_index(pA), moveBy);
						try_to_move(pBIdx, -moveBy);
						tooFar = true;
					}

					if (!verticalCart && abs(diff.z) > 0.1f && ! _final)
					{
						// try to level
						Vector3 moveBy = Vector3::zAxis * diff.z;
						if (!allowTooFar)
						{
							// limit as we don't want to move platforms that are too close
							float const maxDist = 0.5f;
							moveBy.z = clamp(moveBy.z, -maxDist, maxDist);
						}
						moveBy *= levelOutAlongCartLine_strength * 0.5f;
						try_to_move(for_everys_index(pA), moveBy);
						try_to_move(pBIdx, -moveBy);
						tooFar = true;
					}
				}
			}
		}

		// apply initial move
		for_every(p, platforms)
		{
			p->at += p->move * move_strength;
		}

		// stretch to fit cell - force it
		if (requestedCellSize.is_set() ||
			requestedCellWidth.is_set())
		{
			if (tryNo < 50) // initial movement to have them separated
			{
				movementRequired = true;
			}
			else
			{
				Range2 actualSize = Range2::empty;
				for_every(p, platforms)
				{
					Vector2 at = p->get_room_placement().to_vector2();
					actualSize.include(at - p->platform.length() * 0.5f);
					actualSize.include(at + p->platform.length() * 0.5f);
				}

				float useWidth = requestedCellWidth.get(0.0f) - 0.2f;
				float useLength = requestedCellSize.get(0.0f) - 0.01f;
				bool shouldResizeX = (requestedCellWidth.is_set() &&
											(actualSize.x.length() > useWidth ||
											 (platformsSetup.stretchToMatchSizeX.get() && ((actualSize.x.length() < requestedCellWidth.get() - 0.5f) || actualSize.x.length() > requestedCellWidth.get() - 0.01f))));
				bool shouldResizeY = (requestedCellSize.is_set() &&
											(actualSize.y.length() > useLength ||
											 (platformsSetup.stretchToMatchSizeY.get() && ((actualSize.y.length() < requestedCellSize.get() - 0.5f) || actualSize.y.length() > requestedCellSize.get() - 0.01f))));
				if (shouldResizeX || shouldResizeY)
				{
					float resizeX = requestedCellWidth.is_set() ? (platformsSetup.stretchToMatchSizeX.get() ? useWidth / actualSize.x.length() : min(1.0f, useWidth / actualSize.x.length())) : 1.0f;
					float resizeY = requestedCellSize.is_set() ? (platformsSetup.stretchToMatchSizeY.get() ? useLength / actualSize.y.length() : min(1.0f, useLength/ actualSize.y.length())) : 1.0f;
					Vector2 resize(resizeX, resizeY);
					Vector2 centre = actualSize.centre();
					if (platformsSetup.centralPlatformInTheCentre.get())
					{
						Vector3 centralAt = Vector3::zero;
						Vector3 centralSize = Vector3::zero;
						find_central_top_platform(_room, OUT_& centralAt, OUT_& centralSize);
						centre = centralAt.to_vector2();
					}
					for_every(p, platforms)
					{
						Vector2 pCentreOffset = p->get_centre_offset().to_vector2();
						Vector2 at = p->at.to_vector2() + pCentreOffset;
						at = centre + (at - centre) * resize;
						at = at - pCentreOffset;
						p->at.x = at.x;
						p->at.y = at.y;
					}
				}
			}
		}

		++tryNo;
		if (tooFar)
		{
			movementRequired = true;
		}
	}
}

bool PlatformsLinear::check_if_valid() const
{
	RoomGenerationInfo const & rggi = get_room_generation_info();

	bool isValid = true;

	for_every(platform, platforms)
	{
		if (platform->has_multiple_in_one_dir())
		{
#ifndef AN_DEVELOPMENT
			return false;
#endif
			isValid = false;
		}
		an_assert(!platform->has_multiple_in_one_dir(), TXT("platform %i has multiple carts in one direction"), for_everys_index(platform));
	}

	for_every(pA, platforms)
	{
		for_every(cp, pA->cartPoints)
		{
			int pBIdx = find_platform_on_other_end_of_cart_line(for_everys_index(pA), for_everys_index(cp));
			if (pBIdx != NONE &&
				for_everys_index(pA) < pBIdx)
			{
				auto * pB = &platforms[pBIdx];
				Vector3 inDir = Vector3::zero;
				bool verticalCart = false;
				if (cp->axis == CA_BackForth)
				{
					inDir = float(cp->dir) * Vector3::yAxis;
				}
				else if (cp->axis == CA_LeftRight)
				{
					inDir = float(cp->dir) * Vector3::xAxis;
				}
				else if (cp->axis == CA_VerticalX ||
						 cp->axis == CA_VerticalY)
				{
					verticalCart = true;
					inDir = float(cp->dir) * Vector3::zAxis;
				}
				else
				{
					todo_important(TXT("implement_"));
				}

				Vector3 atDiff = pB->at - pA->at;
				float atDiffAlongDir = Vector3::dot(atDiff, inDir);
				if (atDiffAlongDir < 0.0f)
				{
					output_colour(1, 0, 1, 1);
					output(TXT("[problem] platforms %i and %i are not on proper side of each other"), for_everys_index(pA), pBIdx);
					output_colour();
#ifndef AN_DEVELOPMENT
					return false;
#endif
					isValid = false;
				}

				float steepCoef = tan_deg(platformsSetup.maxSteepAngle.get());

				if (!verticalCart && abs(atDiff.z) > atDiffAlongDir * steepCoef * 1.02f)
				{
					output_colour(1, 0, 1, 1);
					output(TXT("[problem] cart between %i and %i too steep! %.3f"), for_everys_index(pA), pBIdx, atDiffAlongDir != 0.0f? atan_deg(abs(atDiff.z) / atDiffAlongDir) : 90.0f);
					output_colour();
#ifndef AN_DEVELOPMENT
					return false;
#endif
					isValid = false;
				}
			}
		}
	}

	for_every(pA, platforms)
	{
		// check if is in front of in and out platforms
		if (!platformsIn.does_contain(for_everys_index(pA)))
		{
			float minY = pA->get_room_placement().y - pA->calculate_half_size(platformsSetup.platformHeight.get()).y;
			for_every(pIn, platformsIn)
			{
				auto const & p = platforms[*pIn];
				float pInMaxY = p.get_room_placement().y + p.calculate_half_size(platformsSetup.platformHeight.get()).y;
				if (minY < pInMaxY)
				{
					output_colour(1, 0, 1, 1);
					output(TXT("[problem] platform %i is way to close to in %i"), for_everys_index(pA), *pIn);
					output_colour();
#ifndef AN_DEVELOPMENT
					return false;
#endif
					isValid = false;
				}
			}
		}
		if (!platformsOut.does_contain(for_everys_index(pA)))
		{
			float maxY = pA->get_room_placement().y + pA->calculate_half_size(platformsSetup.platformHeight.get()).y;
			for_every(pOut, platformsOut)
			{
				auto const & p = platforms[*pOut];
				float pOutMinY = p.get_room_placement().y - p.calculate_half_size(platformsSetup.platformHeight.get()).y;
				if (maxY > pOutMinY)
				{
					output_colour(1, 0, 1, 1);
					output(TXT("[problem] platform %i is way to close to out %i"), for_everys_index(pA), *pOut);
					output_colour();
#ifndef AN_DEVELOPMENT
					return false;
#endif
					isValid = false;
				}
			}
		}
		if (!platformsLeft.does_contain(for_everys_index(pA)))
		{
			float minX = pA->get_room_placement().x - pA->calculate_half_size(platformsSetup.platformHeight.get()).x;
			for_every(pLeft, platformsLeft)
			{
				auto const& p = platforms[*pLeft];
				float pLeftMaxX = p.get_room_placement().x + p.calculate_half_size(platformsSetup.platformHeight.get()).x;
				if (minX < pLeftMaxX)
				{
					output_colour(1, 0, 1, 1);
					output(TXT("[problem] platform %i is way to close to left %i"), for_everys_index(pA), *pLeft);
					output_colour();
#ifndef AN_DEVELOPMENT
					return false;
#endif
					isValid = false;
				}
			}
		}
		if (!platformsRight.does_contain(for_everys_index(pA)))
		{
			float maxX = pA->get_room_placement().x + pA->calculate_half_size(platformsSetup.platformHeight.get()).x;
			for_every(pRight, platformsRight)
			{
				auto const& p = platforms[*pRight];
				float pRightMinX = p.get_room_placement().x - p.calculate_half_size(platformsSetup.platformHeight.get()).x;
				if (maxX > pRightMinX)
				{
					output_colour(1, 0, 1, 1);
					output(TXT("[problem] platform %i is way to close to right %i"), for_everys_index(pA), *pRight);
					output_colour();
#ifndef AN_DEVELOPMENT
					return false;
#endif
					isValid = false;
				}
			}
		}

		for_every(pB, platforms)
		{
			if (for_everys_index(pA) < for_everys_index(pB)) // each pair just once
			{
				Vector3 diff = (pB->get_room_placement() - pA->get_room_placement());
				Vector3 pASize = pA->calculate_half_size(platformsSetup.platformHeight.get());
				Vector3 pBSize = pB->calculate_half_size(platformsSetup.platformHeight.get());
				// really small - we were trying to separate them but it's ok if we're within separation zone
				Vector3 separation = platformsSetup.platformSeparation.get() * 0.1f;
				// we have to leave enough place to have two carts fit (smaller than during separation process)
				separation.x = max(separation.x, rggi.get_tile_size() * 2.2f);
				separation.y = max(separation.y, rggi.get_tile_size() * 2.2f);
				Vector3 minSeparation = pASize + pBSize + separation;
				if (abs(diff.x) < minSeparation.x &&
					abs(diff.y) < minSeparation.y &&
					abs(diff.z) < minSeparation.z)
				{
					output_colour(1, 0, 1, 1);
					output(TXT("[problem] platforms %i and %i occupy same space"), for_everys_index(pA), for_everys_index(pB));
					output_colour();
#ifndef AN_DEVELOPMENT
					return false;
#endif
					isValid = false;
				}
			}
		}
	}

	return isValid;
}

void PlatformsLinear::try_to_move(int _pIdx, Vector3 const & _by, bool _immediateMovement, int _skipPIdx)
{
	an_assert(_pIdx != _skipPIdx);
	// move all connected to not break lines

	ARRAY_STACK(int, platformsToMove, platforms.get_size());
	
	if (abs(_by.x) > 0.0001f)
	{
		platformsToMove.clear();
		platformsToMove.push_back(_pIdx);
		add_platforms_to_move(platformsToMove, CA_LeftRight, _pIdx, _skipPIdx);
		gather_platforms_to_move(platformsToMove, CA_LeftRight, CA_LeftRight, _skipPIdx);
		for (int i = 0; i < platformsToMove.get_size(); ++i)
		{
			if (_immediateMovement)
			{
				platforms[platformsToMove[i]].at.x += _by.x;
			}
			else
			{
				platforms[platformsToMove[i]].move.x += _by.x;
			}
		}
	}

	if (abs(_by.y) > 0.0001f)
	{
		platformsToMove.clear();
		platformsToMove.push_back(_pIdx);
		add_platforms_to_move(platformsToMove, CA_BackForth, _pIdx, _skipPIdx);
		gather_platforms_to_move(platformsToMove, CA_BackForth, CA_BackForth, _skipPIdx);
		for (int i = 0; i < platformsToMove.get_size(); ++i)
		{
			if (_immediateMovement)
			{
				platforms[platformsToMove[i]].at.y += _by.y;
			}
			else
			{
				platforms[platformsToMove[i]].move.y += _by.y;
			}
		}
	}

	if (abs(_by.z) > 0.0001f)
	{
		// vertically move only this one
		if (_immediateMovement)
		{
			platforms[_pIdx].at.z += _by.z;
		}
		else
		{
			platforms[_pIdx].move.z += _by.z;
		}
	}
}


void PlatformsLinear::try_to_move_all_in_dir(int _pIdx, Vector3 const & _by)
{
	ARRAY_STACK(int, platformsToMove, platforms.get_size());

	if (abs(_by.x) > 0.0001f)
	{
		platformsToMove.clear();
		platformsToMove.push_back(_pIdx);
		add_platforms_to_move(platformsToMove, CA_LeftRight, _pIdx, NONE);
		gather_platforms_to_move(platformsToMove, CA_LeftRight, CA_LeftRight, NONE);
		gather_platforms_to_move_all_in_dir(platformsToMove, _pIdx, Vector3(_by.x, 0.0f, 0.0f));
		for (int i = 0; i < platformsToMove.get_size(); ++i)
		{
			platforms[platformsToMove[i]].move.x += _by.x;
		}
	}

	if (abs(_by.y) > 0.0001f)
	{
		platformsToMove.clear();
		platformsToMove.push_back(_pIdx);
		add_platforms_to_move(platformsToMove, CA_BackForth, _pIdx, NONE);
		gather_platforms_to_move(platformsToMove, CA_BackForth, CA_BackForth, NONE);
		gather_platforms_to_move_all_in_dir(platformsToMove, _pIdx, Vector3(0.0f, _by.y, 0.0f));
		for (int i = 0; i < platformsToMove.get_size(); ++i)
		{
			platforms[platformsToMove[i]].move.y += _by.y;
		}
	}

	if (abs(_by.z) > 0.0001f)
	{
		// vertically move only this one
		platforms[_pIdx].move.z += _by.z;
	}
}

void PlatformsLinear::add_platforms_to_move(ArrayStack<int> & _platformsToMove, CartAxis _axis, int _pIdx, int _skipPIdx)
{
	if (_axis == CA_BackForth)
	{
		// move in and out in line too
		if (platformsIn.does_contain(_pIdx))
		{
			for_every(in, platformsIn)
			{
				if (*in != _skipPIdx)
				{
					_platformsToMove.push_back_unique(*in);
				}
			}
		}
		if (platformsOut.does_contain(_pIdx))
		{
			for_every(out, platformsOut)
			{
				if (*out != _skipPIdx)
				{
					_platformsToMove.push_back_unique(*out);
				}
			}
		}
	}

	if (_axis == CA_LeftRight)
	{
		// move in and out in line too
		if (platformsLeft.does_contain(_pIdx))
		{
			for_every(in, platformsLeft)
			{
				if (*in != _skipPIdx)
				{
					_platformsToMove.push_back_unique(*in);
				}
			}
		}
		if (platformsRight.does_contain(_pIdx))
		{
			for_every(out, platformsRight)
			{
				if (*out != _skipPIdx)
				{
					_platformsToMove.push_back_unique(*out);
				}
			}
		}
	}
}

void PlatformsLinear::gather_platforms_to_align(ArrayStack<int> & _platformsToMove, CartAxis _alongAxis, CartAxis _alongAxis2, int _skipPIdx) const
{
	for (int i = 0; i < _platformsToMove.get_size(); ++i)
	{
		for (int j = 0; j < platforms[_platformsToMove[i]].cartPoints.get_size(); ++j)
		{
			auto const & cp = platforms[_platformsToMove[i]].cartPoints[j];
			if (cp.axis == _alongAxis ||
				cp.axis == _alongAxis2)
			{
				int op = find_platform_on_other_end_of_cart_line(_platformsToMove[i], j);
				an_assert(op != NONE);
				if (op != _skipPIdx)
				{
					_platformsToMove.push_back_unique(op);
				}
			}
		}
	}
}

void PlatformsLinear::gather_platforms_to_move(ArrayStack<int> & _platformsToMove, CartAxis _alongAxis, CartAxis _alongAxis2, int _skipPIdx) const
{
	for (int i = 0; i < _platformsToMove.get_size(); ++i)
	{
		for (int j = 0; j < platforms[_platformsToMove[i]].cartPoints.get_size(); ++j)
		{
			auto const & cp = platforms[_platformsToMove[i]].cartPoints[j];
			if (cp.axis != _alongAxis &&
				cp.axis != _alongAxis2)
			{
				int op = find_platform_on_other_end_of_cart_line(_platformsToMove[i], j);
				an_assert(op != NONE);
				if (op != _skipPIdx)
				{
					_platformsToMove.push_back_unique(op);
				}
			}
		}
	}
}

void PlatformsLinear::gather_platforms_to_move_all_in_dir(ArrayStack<int> & _platformsToMove, int _pIdx, Vector3 const & _by) const
{
	Vector3 refPoint = platforms[_pIdx].at;
	for (int i = 0; i < platforms.get_size(); ++i)
	{
		if (Vector3::dot((platforms[i].at - refPoint), _by) >= 0)
		{
			_platformsToMove.push_back_unique(i);
		}
	}
}

void PlatformsLinear::generate_railings_to(Framework::Room* _room, Framework::Scenery* _platformScenery, Platform* _platform)
{
	todo_note(TXT("in and out aligned to y axis"));
	if (_platform->type != PlatformType::PT_In)
	{
		generate_one_side_railings_to(_room, _platformScenery, _platform, true, -1);
	}
	if (_platform->type != PlatformType::PT_Out)
	{
		generate_one_side_railings_to(_room, _platformScenery, _platform, true, 1);
	}
	if (_platform->type != PlatformType::PT_Left)
	{
		generate_one_side_railings_to(_room, _platformScenery, _platform, false, -1);
	}
	if (_platform->type != PlatformType::PT_Right)
	{
		generate_one_side_railings_to(_room, _platformScenery, _platform, false, 1);
	}
}

int compare_cart_point_at(void const * _a, void const * _b)
{
	float const * a = (float const *)(_a);
	float const * b = (float const *)(_b);
	float diff = *a - *b;
	if (diff < 0.0f) return -1;
	if (diff >= 0.0f) return 1;
	return 0;
}

void PlatformsLinear::generate_one_side_railings_to(Framework::Room* _room, Framework::Scenery* _platformScenery, Platform* _platform, bool _x, int _side)
{
	RoomGenerationInfo const & rggi = get_room_generation_info();

	Array<float> cartPointsAt;
	for_every(cp, _platform->cartPoints)
	{
		if (cp->side == _side)
		{
			if (( _x && (cp->axis == CA_LeftRight || cp->axis == CA_VerticalY)) ||
				(!_x && (cp->axis == CA_BackForth || cp->axis == CA_VerticalX)))
			{
				float at = _x ? cp->placement.get_translation().x : cp->placement.get_translation().y;
				cartPointsAt.push_back(at);
			}
		}
	}

	sort(cartPointsAt, compare_cart_point_at);

	Range sideRange = _x ? _platform->platform.x : _platform->platform.y;
	sideRange.min += _x ? - _platform->get_centre_offset().x : - _platform->get_centre_offset().y;
	sideRange.max += _x ? - _platform->get_centre_offset().x : - _platform->get_centre_offset().y;

	RangeInt railingsLimits;
	if (_x)
	{
		railingsLimits = _side > 0 ? _platform->railingsForth : _platform->railingsBack;
	}
	else
	{
		railingsLimits = _side > 0 ? _platform->railingsRight : _platform->railingsLeft;
	}
	if (railingsLimits.is_empty())
	{
		railingsLimits = RangeInt(RS_Low, RS_High);
	}

	Array<Range> railings;
	{
		float currentStart = sideRange.min;
		for_every(cpAt, cartPointsAt)
		{
			float s = *cpAt - rggi.get_tile_size() * 0.5f;
			float e = *cpAt + rggi.get_tile_size() * 0.5f;

			if (currentStart < s - 0.01f)
			{
				railings.push_back(Range(currentStart, s));
			}

			currentStart = e;
		}
		if (currentStart < sideRange.max - 0.01f)
		{
			railings.push_back(Range(currentStart, sideRange.max));
		}
	}

	if (railings.is_empty())
	{
		return;
	}

	Framework::MeshGenerator * railingNoRailingMeshGenerator = info->find<Framework::MeshGenerator>(roomVariables, info->railingNoRailing);
	Framework::MeshGenerator * railingLowTransparentMeshGenerator = info->find<Framework::MeshGenerator>(roomVariables, info->railingLowTransparent);
	Framework::MeshGenerator * railingLowSolidMeshGenerator = info->find<Framework::MeshGenerator>(roomVariables, info->railingLowSolid);
	Framework::MeshGenerator * railingMedSolidMeshGenerator = info->find<Framework::MeshGenerator>(roomVariables, info->railingMedSolid);
	Framework::MeshGenerator * railingHighSolidMeshGenerator = info->find<Framework::MeshGenerator>(roomVariables, info->railingHighSolid);

	Array<Framework::MeshGenerator*> railingMeshGenerators;
	if (railingsLimits.does_contain(RS_None) && railingNoRailingMeshGenerator)
	{
		for_count(int, i, 3) railingMeshGenerators.push_back(railingNoRailingMeshGenerator);
	}
	if (railingsLimits.does_contain(RS_Low) && railingLowTransparentMeshGenerator && info->allowRailingTransparent.get(roomVariables, true))
	{
		for_count(int, i, 4) railingMeshGenerators.push_back(railingLowTransparentMeshGenerator);
	}
	if (railingsLimits.does_contain(RS_Low) && railingLowSolidMeshGenerator)
	{
		for_count(int, i, 3) railingMeshGenerators.push_back(railingLowSolidMeshGenerator);
	}
	if (railingsLimits.does_contain(RS_Med) && railingMedSolidMeshGenerator)
	{
		for_count(int, i, 2) railingMeshGenerators.push_back(railingMedSolidMeshGenerator);
	}
	if (railingsLimits.does_contain(RS_High) && railingHighSolidMeshGenerator)
	{
		for_count(int, i, 2) railingMeshGenerators.push_back(railingHighSolidMeshGenerator);
	}

	if (!railingMeshGenerators.is_empty())
	{
		bool oneRailingMeshGeneratorType = true;
		for_every(railingMeshGenerator, railingMeshGenerators)
		{
			if (*railingMeshGenerator != railingMeshGenerators[0])
			{
				oneRailingMeshGeneratorType = false;
				break;
			}
		}

		if (!oneRailingMeshGeneratorType)
		{
			for (int i = 0; i < railings.get_size(); ++i)
			{
				auto & railing = railings[i];
				float length = railing.length();
				float minBreakingLength = rggi.get_tile_size();
				if (length > minBreakingLength * 2.0f && random.get_chance(length > 2.5f? 0.85f : (length > 1.5f ? 0.6f : 0.2f)))
				{
					float breakAt = railing.min + random.get_float(minBreakingLength, length - minBreakingLength);
					Range toAdd = railing;
					railing.max = breakAt;
					toAdd.min = breakAt;
					railings.insert_at(i + 1, toAdd);
					--i;
				}
			}
		}

		// so we will have them properly placed, but as they are reversed, remember that fact
		float railingMultiplier = 1.0f;
		if ((_x && _side < 0) || (!_x && _side > 0))
		{
			railingMultiplier = -1.0f;
			for_every(railing, railings)
			{
				swap(railing->min, railing->max);
			}
		}

		Framework::MeshGenerator* lastRailingMeshGenerator = nullptr;
			
		Array<Framework::UsedLibraryStored<Framework::Mesh>> tempSideMeshes;

		// point outwards
		Quat dir;
		{
			Vector3 inDir = (_x ? Vector3::yAxis : Vector3::xAxis) * (float)_side;
			Matrix44 dirMatrix = look_at_matrix(Vector3::zero, inDir, Vector3::zAxis);
			dir = dirMatrix.to_quat();
		}
		
		for_every(railing, railings)
		{
			Framework::MeshGenerator* meshGenerator = nullptr;
			while (meshGenerator == lastRailingMeshGenerator || meshGenerator == nullptr)
			{
				int idx = random.get_int(railingMeshGenerators.get_size());
				meshGenerator = railingMeshGenerators[idx];
				if (random.get_chance(0.05f) || oneRailingMeshGeneratorType)
				{
					break;
				}
			}
			lastRailingMeshGenerator = meshGenerator;
			Framework::MeshGeneratorRequest meshGeneratorRequest;

			SimpleVariableStorage parameters;
			float railingCentre = (railing->min + railing->max) * 0.5f;
			{
				Vector3 location = Vector3::zero;
				if (_x)
				{
					location.x = railingCentre;
					location.y = (_side > 0 ? _platform->platform.y.max : _platform->platform.y.min) - _platform->get_centre_offset().y;
				}
				else
				{
					location.x = (_side > 0 ? _platform->platform.x.max : _platform->platform.x.min) - _platform->get_centre_offset().x;
					location.y = railingCentre;
				}
				Transform railingPlacement(location, dir);
				parameters.access<Transform>(NAME(railingPlacement)) = railingPlacement;
				parameters.access<Vector3>(NAME(railingDir)) = railingPlacement.get_axis(Axis::Y);
			}
			parameters.access<float>(NAME(railingLeft)) = (railing->min - railingCentre) * railingMultiplier;
			parameters.access<float>(NAME(railingRight)) = (railing->max - railingCentre) * railingMultiplier;
			parameters.access<bool>(NAME(railingLeftCorner)) = abs(railing->min - sideRange.min) < 0.01f || abs(railing->min - sideRange.max) < 0.01f;
			parameters.access<bool>(NAME(railingRightCorner)) = abs(railing->max - sideRange.max) < 0.01f || abs(railing->max - sideRange.max) < 0.01f;
			appearanceSetup.setup_parameters(parameters);
			_room->collect_variables(parameters);

			info->apply_generation_parameters_to(parameters);

			Random::Generator rg = random;
			meshGeneratorRequest.using_random_regenerator(rg);
			meshGeneratorRequest.using_parameters(parameters);
			Framework::UsedLibraryStored<Framework::Mesh> mesh = meshGenerator->generate_temporary_library_mesh(Framework::Library::get_current(), REF_ meshGeneratorRequest);
			if (mesh.is_set())
			{
				tempSideMeshes.push_back(mesh);
			}
		}

		Array<Framework::Mesh::FuseMesh> tempSideMeshesToFuse;
		for_every(tsm, tempSideMeshes)
		{
			tempSideMeshesToFuse.push_back(Framework::Mesh::FuseMesh(tsm->get()));
		}

		_platformScenery->get_appearance()->access_mesh()->fuse(tempSideMeshesToFuse);
		_platformScenery->get_appearance()->use_mesh(_platformScenery->get_appearance()->access_mesh()); // to reinitialise
	}
}

void PlatformsLinear::choose_railings_strategy_and_setup_limits()
{
	float alongYSlopeChance = 0.0f;
	float alongXSlopeChance = 0.0f;
	float forceLowInTheMiddleChance = 0.0f;
	float forceHighOutsideTheMiddleChance = 0.0f;

	if (abs(verticalAdjustment.yA.get().centre()) > 0.2f ||
		abs(verticalAdjustment.xA.get().centre()) > 0.2f)
	{
		forceLowInTheMiddleChance = 0.45f;
		forceHighOutsideTheMiddleChance = 0.1f;
	}
		
	if (verticalAdjustment.yB.get().centre() != 0.0f)
	{
		alongYSlopeChance = abs(verticalAdjustment.yB.get().centre()) > 0.6f? 0.95f : abs(verticalAdjustment.yA.get().centre()) < 0.2f ? 0.6f : 0.7f;
	}
	else
	{
		alongYSlopeChance = abs(verticalAdjustment.yA.get().centre()) < 0.2f ? 0.2f : 0.7f;
	}

	if (verticalAdjustment.xB.get().centre() != 0.0f)
	{
		alongXSlopeChance = abs(verticalAdjustment.xB.get().centre()) > 0.6f ? 0.8f : abs(verticalAdjustment.xA.get().centre()) < 0.2f ? 0.5f : 0.7f;
	}
	else
	{
		alongXSlopeChance = abs(verticalAdjustment.xA.get().centre()) < 0.2f ? 0.2f : 0.7f;
	}

	float allHighChance = random.get_chance(0.2f) ? random.get_float(0.0f, 0.4f) : 0.0f;
	float noRailingsChance = 0.1f;

	alongYSlopeChance = info->platformsSetup.alongYSlopeChance.get(roomVariables, alongYSlopeChance);
	alongXSlopeChance = info->platformsSetup.alongXSlopeChance.get(roomVariables, alongXSlopeChance);
	forceLowInTheMiddleChance = info->platformsSetup.forceLowInTheMiddleChance.get(roomVariables, forceLowInTheMiddleChance);
	forceHighOutsideTheMiddleChance = info->platformsSetup.forceHighOutsideTheMiddleChance.get(roomVariables, forceHighOutsideTheMiddleChance);
	allHighChance = info->platformsSetup.allHighChance.get(roomVariables, allHighChance);
	noRailingsChance = info->platformsSetup.noRailingsChance.get(roomVariables, noRailingsChance);

	bool alongYSlope = random.get_chance(alongYSlopeChance);
	bool alongXSlope = random.get_chance(alongXSlopeChance);
	bool forceLowInTheMiddle = random.get_chance(forceLowInTheMiddleChance);
	bool forceHighOutsideTheMiddle = random.get_chance(forceHighOutsideTheMiddleChance);

	Range2 space = Range2::empty;
	for_every(platform, platforms)
	{
		space.include(platform->at.to_vector2());
	}

	float middleCoef = random.get_float(0.3f, 0.45f);
	Range2 middle = Range2(Range(space.x.get_at(middleCoef), space.x.get_at(1.0f - middleCoef)),
						   Range(space.y.get_at(middleCoef), space.y.get_at(1.0f - middleCoef)));

	todo_note(TXT("add general limits - all low, all high, only med to high etc"));
#ifdef OUTPUT_GENERATION
	{
		LogInfoContext logInfoContext;
		logInfoContext.log(TXT("railings strategy:"));
		{
			LOG_INDENT(logInfoContext);
			if (alongXSlope)
			{
				logInfoContext.log(TXT("- along x slope"));
			}
			if (alongYSlope)
			{
				logInfoContext.log(TXT("- along y slope"));
			}
			if (forceLowInTheMiddle)
			{
				logInfoContext.log(TXT("- force low in the middle"));
			}
			if (forceHighOutsideTheMiddle)
			{
				logInfoContext.log(TXT("- force high outside the middle"));
			}
			if (allHighChance)
			{
				logInfoContext.log(TXT("- chance of all high: %.1f%%"), allHighChance * 100.0f);
			}
			if (noRailingsChance)
			{
				logInfoContext.log(TXT("- chance of no railings: %.1f%%"), noRailingsChance * 100.0f);
			}
		}
		logInfoContext.output_to_output();
	}
#endif

	RangeInt upSlope = RangeInt(RS_Med, RS_High);
	RangeInt downSlope = RangeInt(RS_Low, RS_Low);
	RangeInt noSlope = RangeInt(RS_Low, RS_Low);
	float slopeThreshold = 0.01f;
	if (random.get_chance(0.3f))
	{
		downSlope = RangeInt(RS_Low, RS_Med);
	}
	if (random.get_chance(0.4f))
	{
		noSlope = RangeInt(RS_Low, RS_Med);
		if (random.get_chance(0.4f))
		{
			noSlope = RangeInt(RS_Low, RS_High);
		}
	}

	RangeInt railingsLimitLeft = info->platformsSetup.railingsLimitLeft.get(roomVariables, RangeInt::empty);
	RangeInt railingsLimitRight = info->platformsSetup.railingsLimitRight.get(roomVariables, RangeInt::empty);
	RangeInt railingsLimitForth = info->platformsSetup.railingsLimitForth.get(roomVariables, RangeInt::empty);
	RangeInt railingsLimitBack = info->platformsSetup.railingsLimitBack.get(roomVariables, RangeInt::empty);
	RangeInt railingsLimitEntrances = info->platformsSetup.railingsLimitEntrances.get(roomVariables, RangeInt::empty);
	
	bool outerGridRailings = info->platformsSetup.outerGridRailings.get(roomVariables, true);

	for_every(platform, platforms)
	{
		int platformId = for_everys_index(platform);
		bool inTheMiddle = middle.does_contain(platform->at.to_vector2());
		if (random.get_chance(noRailingsChance))
		{
			platform->railingsLeft = RangeInt(RS_None);
			platform->railingsRight = RangeInt(RS_None);
			platform->railingsBack = RangeInt(RS_None);
			platform->railingsForth = RangeInt(RS_None);
		}
		else if (random.get_chance(allHighChance))
		{
			platform->railingsLeft = RangeInt(RS_High);
			platform->railingsRight = RangeInt(RS_High);
			platform->railingsBack = RangeInt(RS_High);
			platform->railingsForth = RangeInt(RS_High);
		}
		else if (forceLowInTheMiddle && inTheMiddle)
		{
			platform->railingsLeft = RangeInt(RS_Low);
			platform->railingsRight = RangeInt(RS_Low);
			platform->railingsBack = RangeInt(RS_Low);
			platform->railingsForth = RangeInt(RS_Low);
		}
		else if (forceHighOutsideTheMiddle && ! inTheMiddle)
		{
			platform->railingsLeft = RangeInt(RS_High);
			platform->railingsRight = RangeInt(RS_High);
			platform->railingsBack = RangeInt(RS_High);
			platform->railingsForth = RangeInt(RS_High);
		}
		else
		{
			// by default keep them no slope
			platform->railingsLeft = noSlope;
			platform->railingsRight = noSlope;
			platform->railingsBack = noSlope;
			platform->railingsForth = noSlope;
			if (alongXSlope)
			{
				int xMinus = find_platform_in_dir(platformId, -Vector3::xAxis);
				if (xMinus != NONE)
				{
					float slope = (platforms[xMinus].at.z - platform->at.z) / (platforms[xMinus].at.x - platform->at.x);
					if (slope > slopeThreshold)
					{
						platform->railingsLeft = upSlope;
					}
					if (slope < slopeThreshold)
					{
						platform->railingsLeft = downSlope;
					}
				}
				int xPlus = find_platform_in_dir(platformId, Vector3::xAxis);
				if (xPlus != NONE)
				{
					float slope = (platforms[xPlus].at.z - platform->at.z) / (platforms[xPlus].at.x - platform->at.x);
					if (slope > slopeThreshold)
					{
						platform->railingsRight = upSlope;
					}
					if (slope < slopeThreshold)
					{
						platform->railingsRight = downSlope;
					}
				}
			}
			if (alongYSlope)
			{
				int yMinus = find_platform_in_dir(platformId, -Vector3::yAxis);
				if (yMinus != NONE)
				{
					float slope = (platforms[yMinus].at.z - platform->at.z) / (platforms[yMinus].at.x - platform->at.x);
					if (slope > slopeThreshold)
					{
						platform->railingsBack = upSlope;
					}
					if (slope < slopeThreshold)
					{
						platform->railingsBack = downSlope;
					}
				}
				int yPlus = find_platform_in_dir(platformId, Vector3::yAxis);
				if (yPlus != NONE)
				{
					float slope = (platforms[yPlus].at.z - platform->at.z) / (platforms[yPlus].at.x - platform->at.x);
					if (slope > slopeThreshold)
					{
						platform->railingsForth = upSlope;
					}
					if (slope < slopeThreshold)
					{
						platform->railingsForth = downSlope;
					}
				}
			}
		}
		platform->railingsLeft = platform->railingsLeft.get_limited_to(railingsLimitLeft);
		platform->railingsRight = platform->railingsRight.get_limited_to(railingsLimitRight);
		platform->railingsForth = platform->railingsForth.get_limited_to(railingsLimitForth);
		platform->railingsBack = platform->railingsBack.get_limited_to(railingsLimitBack);

		if (!railingsLimitEntrances.is_empty())
		{
			if (platformsIn.does_contain(platformId) ||
				platformsOut.does_contain(platformId) ||
				platformsLeft.does_contain(platformId) ||
				platformsRight.does_contain(platformId))
			{
				platform->railingsLeft = platform->railingsLeft.get_limited_to(railingsLimitEntrances);
				platform->railingsRight = platform->railingsRight.get_limited_to(railingsLimitEntrances);
				platform->railingsForth = platform->railingsForth.get_limited_to(railingsLimitEntrances);
				platform->railingsBack = platform->railingsBack.get_limited_to(railingsLimitEntrances);
			}
		}

		if (! outerGridRailings && platform->atGrid.is_set() && ! grid.is_empty())
		{
			if (platform->atGrid.get().x <= 1)
			{
				platform->railingsLeft = RangeInt(RS_None, RS_None);
			}
			if (platform->atGrid.get().x >= grid.x.max - 1)
			{
				platform->railingsRight = RangeInt(RS_None, RS_None);
			}
			if (platform->atGrid.get().y <= 1)
			{
				platform->railingsBack = RangeInt(RS_None, RS_None);
			}
			if (platform->atGrid.get().y >= grid.y.max - 1)
			{
				platform->railingsForth = RangeInt(RS_None, RS_None);
			}
		}
	}

	// apply platforms extra limits
	for_every(ep, platformsExtra)
	{
		if (ep->extraPlatform)
		{
			auto& platform = platforms[ep->platformIdx];
			platform.railingsLeft = platform.railingsLeft.get_limited_to(ep->extraPlatform->railingsLimitLeft.get(roomVariables, RangeInt::empty));
			platform.railingsRight = platform.railingsRight.get_limited_to(ep->extraPlatform->railingsLimitRight.get(roomVariables, RangeInt::empty));
			platform.railingsForth = platform.railingsForth.get_limited_to(ep->extraPlatform->railingsLimitForth.get(roomVariables, RangeInt::empty));
			platform.railingsBack = platform.railingsBack.get_limited_to(ep->extraPlatform->railingsLimitBack.get(roomVariables, RangeInt::empty));
		}
	}
}

bool PlatformsLinear::put_everything_into_room(Framework::Game* _game, Framework::Room* _room, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	RoomGenerationInfo const & rggi = get_room_generation_info();

#ifdef OUTPUT_GENERATION
	output(TXT("placing doors..."));
#endif

	place_doors(_game, _room);

	// create sceneries for platforms
#ifdef OUTPUT_GENERATION
	output(TXT("creating sceneries for platforms..."));
#endif
	Framework::SceneryType * platformSceneryType = info->find<Framework::SceneryType>(roomVariables, info->platformSceneryType);
	Array<Framework::Scenery*> platformSceneries;
	SimpleVariableStorage roomVariables;
	_room->collect_variables(roomVariables);
	if (platformSceneryType)
	{
		platformSceneryType->load_on_demand_if_required();

		for_every(platform, platforms)
		{
			Framework::Scenery* platformScenery = nullptr;
			int platformIdx = for_everys_index(platform);
			_game->perform_sync_world_job(TXT("spawn platform"), [&platformScenery, platformSceneryType, platformIdx, _room]()
			{
				platformScenery = new Framework::Scenery(platformSceneryType, String::printf(TXT("platform %i"), platformIdx));
				platformScenery->init(_room->get_in_sub_world());
			});
			platformScenery->access_individual_random_generator() = random.spawn();
			platformScenery->access_variables().access<float>(NAME(platformWidth)) = platform->platform.x.length();
			platformScenery->access_variables().access<float>(NAME(platformLength)) = platform->platform.y.length();
			appearanceSetup.setup_variables(platformScenery);
			platformScenery->access_variables().set_from(roomVariables);
			info->apply_generation_parameters_to(platformScenery->access_variables());
			platformScenery->initialise_modules();
			generate_railings_to(_room, platformScenery, platform);
			_game->perform_sync_world_job(TXT("place platform"), [&platformScenery, platform, _room]()
			{
				platformScenery->get_presence()->place_in_room(_room, Transform(platform->get_room_placement(), Quat::identity));
			});
			_game->on_newly_created_object(platformScenery);
			platformSceneries.push_back(platformScenery);
		}
	}
	else
	{
		error(TXT("no platform sceneries"));
		return false;
	}

	if (!platformsExtra.is_empty())
	{
#ifdef OUTPUT_GENERATION
		output(TXT("creating extra sceneries for platforms..."));
#endif
		for_every(ep, platformsExtra)
		{
			auto const& platform = platforms[ep->platformIdx];
			for_every(u, ep->extraPlatform->useMeshGenerators)
			{
				Framework::LibraryName useMeshGeneratorName = u->name.get(Framework::LibraryName::invalid());
				if (useMeshGeneratorName.is_valid())
				{
					if (Framework::MeshGenerator* useMeshGenerator = info->find<Framework::MeshGenerator>(roomVariables, useMeshGeneratorName))
					{
						if (auto* mg = useMeshGenerator)
						{
							SimpleVariableStorage epVariables;
							epVariables = roomVariables;
							epVariables.access<float>(NAME(platformWidth)) = platform.platform.x.length();
							epVariables.access<float>(NAME(platformLength)) = platform.platform.y.length();

							auto generatedMesh = mg->generate_temporary_library_mesh(::Framework::Library::get_current(),
								::Framework::MeshGeneratorRequest()
								.for_wmp_owner(_room)
								.no_lods()
								.using_parameters(roomVariables)
								.using_random_regenerator(_room->get_individual_random_generator())
								.using_mesh_nodes(_roomGeneratingContext.meshNodes));
							if (generatedMesh.get())
							{
								Vector3 loc = platform.get_room_placement();
								Vector3 ptCentre = Vector3(0.5f, 0.5f, 0.0f);
								loc += platform.calculate_half_size(0.0f) * ((u->atPt.get(ptCentre) - ptCentre) * Vector3(2.0f, 2.0f, 1.0f));

								Transform placeAt = look_matrix(loc, u->forwardDir.get(Vector3::yAxis), u->upDir.get(Vector3::zAxis)).to_transform();

								_room->add_mesh(generatedMesh.get(), placeAt);
								_room->place_pending_doors_on_pois();
							}
						}
					}
				}
			}
			for_every(u, ep->extraPlatform->spawnSceneryTypes)
			{
				Framework::LibraryName spawnSceneryTypeName = u->name.get(Framework::LibraryName::invalid());
				if (spawnSceneryTypeName.is_valid())
				{
					if (Framework::SceneryType* spawnSceneryType = info->find<Framework::SceneryType>(roomVariables, spawnSceneryTypeName))
					{
						Vector3 loc = platform.get_room_placement();
						Vector3 ptCentre = Vector3(0.5f, 0.5f, 0.0f);
						loc += platform.calculate_half_size(0.0f) * ((u->atPt.get(ptCentre) - ptCentre) * Vector3(2.0f, 2.0f, 1.0f));

						Transform placeAt = look_matrix(loc, u->forwardDir.get(Vector3::yAxis), u->upDir.get(Vector3::zAxis)).to_transform();

						{
							Framework::Scenery* extraScenery = nullptr;
							int epIdx = for_everys_index(ep);
							_game->perform_sync_world_job(TXT("spawn platform"), [&extraScenery, spawnSceneryType, epIdx, _room]()
								{
									extraScenery = new Framework::Scenery(spawnSceneryType, String::printf(TXT("extra platform scenery %i"), epIdx));
									extraScenery->init(_room->get_in_sub_world());
								});
							extraScenery->access_tags().set_tags_from(u->tags);
							extraScenery->access_individual_random_generator() = random.spawn();
							extraScenery->access_variables().access<float>(NAME(platformWidth)) = platform.platform.x.length();
							extraScenery->access_variables().access<float>(NAME(platformLength)) = platform.platform.y.length();
							appearanceSetup.setup_variables(extraScenery);
							extraScenery->access_variables().set_from(roomVariables);
							info->apply_generation_parameters_to(extraScenery->access_variables());
							extraScenery->initialise_modules();
							_game->perform_sync_world_job(TXT("place platform"), [&extraScenery, placeAt, _room]()
								{
									extraScenery->get_presence()->place_in_room(_room, placeAt);
								});
							_game->on_newly_created_object(extraScenery);
							platformSceneries.push_back(extraScenery);
						}
					}
				}
			}
		}
	}

	// create sceneries for cart points
#ifdef OUTPUT_GENERATION
	output(TXT("creating sceneries for cart points..."));
#endif
	Framework::SceneryType * cartPointVerticalSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartPointVerticalSceneryType, info->cartPointSceneryType);
	Framework::SceneryType * cartPointHorizontalSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartPointHorizontalSceneryType, info->cartPointSceneryType);

	{
		if (cartPointVerticalSceneryType) cartPointVerticalSceneryType->load_on_demand_if_required();
		if (cartPointHorizontalSceneryType) cartPointHorizontalSceneryType->load_on_demand_if_required();

		Framework::Mesh const * sharedCartPointVerticalMesh = nullptr;
		Framework::Mesh const * sharedCartPointHorizontalMesh = nullptr;
		for_every(platform, platforms)
		{
			for_every(cartPoint, platform->cartPoints)
			{
				if (is_vertical(cartPoint->axis) ? cartPointVerticalSceneryType : cartPointHorizontalSceneryType)
				{
					Framework::Scenery* cartPointScenery = nullptr;
					_game->perform_sync_world_job(TXT("spawn cart point"), [cartPoint, &cartPointScenery, cartPointVerticalSceneryType, cartPointHorizontalSceneryType, _room]()
					{
						cartPointScenery = new Framework::Scenery(is_vertical(cartPoint->axis) ? cartPointVerticalSceneryType : cartPointHorizontalSceneryType, String::empty());
						cartPointScenery->init(_room->get_in_sub_world());
					});
					cartPointScenery->access_individual_random_generator() = random.spawn();
					cartPointScenery->access_variables().access<float>(NAME(cartWidth)) = rggi.get_tile_size();
					cartPointScenery->access_variables().access<float>(NAME(cartLength)) = rggi.get_tile_size();
					appearanceSetup.setup_variables(cartPointScenery);
					_room->collect_variables(cartPointScenery->access_variables());
					info->apply_generation_parameters_to(cartPointScenery->access_variables());
					cartPointScenery->initialise_modules([this, cartPoint, sharedCartPointVerticalMesh, sharedCartPointHorizontalMesh](Framework::Module* _module)
					{
						if (setup.shareCartPointMeshes.get())
						{
							if (auto * moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
							{
								if (is_vertical(cartPoint->axis))
								{
									if (sharedCartPointVerticalMesh)
									{
										moduleAppearance->use_mesh(sharedCartPointVerticalMesh);
									}
								}
								else
								{
									if (sharedCartPointHorizontalMesh)
									{
										moduleAppearance->use_mesh(sharedCartPointHorizontalMesh);
									}
								}
							}
						}
					});
					auto platformScenery = platformSceneries[for_everys_index(platform)];
					_game->perform_sync_world_job(TXT("place cart point"), [platformScenery, &cartPointScenery, cartPoint, _room]()
					{
						cartPointScenery->get_presence()->place_in_room(_room, platformScenery->get_presence()->get_placement().to_world(cartPoint->placement));
						cartPointScenery->get_presence()->force_base_on(platformScenery);
					});
					_game->on_newly_created_object(cartPointScenery);
					if (setup.shareCartPointMeshes.get())
					{
						if (is_vertical(cartPoint->axis))
						{
							if (!sharedCartPointVerticalMesh)
							{
								cartPointScenery->get_appearance()->access_mesh()->be_shared();
								sharedCartPointVerticalMesh = cartPointScenery->get_appearance()->get_mesh();
							}
						}
						else
						{
							if (!sharedCartPointHorizontalMesh)
							{
								cartPointScenery->get_appearance()->access_mesh()->be_shared();
								sharedCartPointHorizontalMesh = cartPointScenery->get_appearance()->get_mesh();
							}
						}
					}
				}
			}
		}
	}

	// create sceneries for carts
	// after platforms and cart points as carts will be looking for them
#ifdef OUTPUT_GENERATION
	output(TXT("creating sceneries for carts..."));
#endif
	Framework::SceneryType * cartVerticalSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartVerticalSceneryType, info->cartSceneryType);
	Framework::SceneryType * cartHorizontalSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartHorizontalSceneryType, info->cartSceneryType);

	Framework::Mesh const * sharedCartVerticalMesh = nullptr;
	Framework::Mesh const * sharedCartHorizontalMesh = nullptr;
	if (cartVerticalSceneryType && cartHorizontalSceneryType)
	{
		cartVerticalSceneryType->load_on_demand_if_required();
		cartHorizontalSceneryType->load_on_demand_if_required();

		for_every(cart, carts)
		{
			if (is_vertical(cart->axis) ? cartVerticalSceneryType : cartHorizontalSceneryType)
			{
				Framework::Scenery* cartScenery = nullptr;
				_game->perform_sync_world_job(TXT("spawn cart"), [cart, &cartScenery, cartVerticalSceneryType, cartHorizontalSceneryType, _room]()
				{
					cartScenery = new Framework::Scenery(is_vertical(cart->axis) ? cartVerticalSceneryType : cartHorizontalSceneryType, String::empty());
					cartScenery->init(_room->get_in_sub_world());
				});
				bool reversePlatform = false; // for horizontal only - left is plus, right is minus
				Vector3 laneVector = cart->calculate_lane_vector(this, platformSceneries, OUT_ reversePlatform);
				cartScenery->access_individual_random_generator() = random.spawn();
				cartScenery->access_variables().access<float>(NAME(cartWidth)) = rggi.get_tile_size();
				cartScenery->access_variables().access<float>(NAME(cartLength)) = rggi.get_tile_size();
				appearanceSetup.setup_variables(cartScenery);
				cartScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopA)) = platformSceneries[cart->minus.platformIdx];
				cartScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopB)) = platformSceneries[cart->plus.platformIdx];
				cartScenery->access_variables().access<Transform>(NAME(elevatorStopACartPoint)) = find_cart_point(cart->minus)->placement;
				cartScenery->access_variables().access<Transform>(NAME(elevatorStopBCartPoint)) = find_cart_point(cart->plus)->placement;
				cartScenery->access_variables().access<Vector3>(NAME(laneVector)) = laneVector;
				/* allow active buttons always */ //cartScenery->access_variables().access<bool>(NAME(noActiveButtonsForElevatorsWithoutPlayer)) = true;
				_room->collect_variables(cartScenery->access_variables());
				info->apply_generation_parameters_to(cartScenery->access_variables());
				Framework::Mesh const * & sharedCartMesh = is_vertical(cart->axis) ? sharedCartVerticalMesh : sharedCartHorizontalMesh;
				cartScenery->initialise_modules([this, sharedCartMesh](Framework::Module* _module)
				{
					if (setup.shareCartMeshes.get())
					{
						if (auto * moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
						{
							if (sharedCartMesh)
							{
								moduleAppearance->use_mesh(sharedCartMesh);
							}
						}
					}
				});
				_game->perform_sync_world_job(TXT("place cart"), [&cartScenery, _room]()
				{
					cartScenery->get_presence()->place_in_room(_room, Transform::identity); // dirty solution but still, will be replaced with movement
				});
				_game->on_newly_created_object(cartScenery);
				if (auto * moduleAppearance = cartScenery->get_appearance())
				{
					if (setup.shareCartMeshes.get())
					{
						if (!sharedCartMesh)
						{
							moduleAppearance->access_mesh()->be_shared();
							sharedCartMesh = moduleAppearance->get_mesh();
						}
					}
				}
			}
		}
	}
	else
	{
		error(TXT("no cart sceneries"));
		return false;
	}

	// create sceneries for lanes
#ifdef OUTPUT_GENERATION
	output(TXT("creating sceneries for lanes..."));
#endif
	Framework::SceneryType * cartLaneVerticalSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartLaneVerticalSceneryType, info->cartLaneSceneryType);
	Framework::SceneryType * cartLaneHorizontalSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartLaneHorizontalSceneryType, info->cartLaneSceneryType);

	if (cartLaneHorizontalSceneryType || cartLaneVerticalSceneryType)
	{
		for_every(cart, carts)
		{
			bool reversePlatform = false; // for horizontal only - left is plus, right is minus
			Vector3 laneVector = cart->calculate_lane_vector(this, platformSceneries, OUT_ reversePlatform);

			Framework::SceneryType* sceneryType = is_vertical(cart->axis)? cartLaneVerticalSceneryType : cartLaneHorizontalSceneryType;
			if (!sceneryType)
			{
				continue;
			}
				
			Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
			doc->inRoom = _room;
			doc->name = TXT("lane");
			doc->objectType = sceneryType;
			doc->placement = Transform::identity;
			doc->randomGenerator = random.spawn();

			// store A as left/bottom and B as right/top
			ElevatorCartPointRef minus = reversePlatform ? cart->plus : cart->minus;
			ElevatorCartPointRef plus = reversePlatform ? cart->minus : cart->plus;

			doc->variables.access<float>(NAME(cartWidth)) = rggi.get_tile_size();
			doc->variables.access<float>(NAME(cartLength)) = rggi.get_tile_size();
			appearanceSetup.setup_parameters(doc->variables);
			doc->variables.access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopA)) = platformSceneries[minus.platformIdx];
			doc->variables.access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopB)) = platformSceneries[plus.platformIdx];
			doc->variables.access<Transform>(NAME(elevatorStopACartPoint)) = find_cart_point(minus)->placement;
			doc->variables.access<Transform>(NAME(elevatorStopBCartPoint)) = find_cart_point(plus)->placement;
			doc->variables.access<Vector3>(NAME(laneVector)) = laneVector;
			_room->collect_variables(doc->variables);
			info->apply_generation_parameters_to(doc->variables);

			doc->placement = platformSceneries[minus.platformIdx]->get_presence()->get_placement().to_world(find_cart_point(minus)->placement);

			_game->queue_delayed_object_creation(doc, true);
		}
	}

	// volumetric limit POIs
	Range3 allPlatformsBBox = Range3::empty;
	for_every(platform, platforms)
	{
		allPlatformsBBox.include(platform->at);
	}

	for_every(door, inDoors)
	{
		allPlatformsBBox.include(door->placement.get_translation());
	}
	for_every(door, outDoors)
	{
		allPlatformsBBox.include(door->placement.get_translation());
	}
	for_every(door, leftDoors)
	{
		allPlatformsBBox.include(door->placement.get_translation());
	}
	for_every(door, rightDoors)
	{
		allPlatformsBBox.include(door->placement.get_translation());
	}

	allPlatformsBBox.expand_by(Vector3(2.0f, 2.0f, 2.0f));

	_room->add_volumetric_limit_pois(allPlatformsBBox);

#ifdef OUTPUT_GENERATION
	output(TXT("done!"));
#endif

	return true;
}

PlatformsLinear::Cart const * PlatformsLinear::find_cart(int _platformIdx, int _cartPointIdx) const
{
	for_every(cart, carts)
	{
		if (cart->minus.platformIdx == _platformIdx &&
			cart->minus.cartPointIdx == _cartPointIdx)
		{
			return cart;
		}
		if (cart->plus.platformIdx == _platformIdx &&
			cart->plus.cartPointIdx == _cartPointIdx)
		{
			return cart;
		}
	}
	return nullptr;
}

PlatformsLinear::CartPoint * PlatformsLinear::find_cart_point(ElevatorCartPointRef const & _ref)
{
	return &platforms[_ref.platformIdx].cartPoints[_ref.cartPointIdx];
}

int PlatformsLinear::find_platform_on_other_end_of_cart_line(int _platformIdx, int _cartPointIdx) const
{
	for_every(cart, carts)
	{
		if (cart->minus.platformIdx == _platformIdx &&
			cart->minus.cartPointIdx == _cartPointIdx)
		{
			an_assert(cart->plus.platformIdx != NONE);
			return cart->plus.platformIdx;
		}
		if (cart->plus.platformIdx == _platformIdx &&
			cart->plus.cartPointIdx == _cartPointIdx)
		{
			an_assert(cart->minus.platformIdx != NONE);
			return cart->minus.platformIdx;
		}
	}
	an_assert(false);
	return NONE;
}

int PlatformsLinear::find_platform_in_dir(int _fromPlatform, Vector3 const & _dir) const
{
	for_every(cp, platforms[_fromPlatform].cartPoints)
	{
		if (_dir.x != 0.0f &&
			cp->axis == CA_LeftRight &&
			((_dir.x < 0.0f && cp->dir < 0) ||
			 (_dir.x > 0.0f && cp->dir > 0)))
		{
			return find_platform_on_other_end_of_cart_line(_fromPlatform, for_everys_index(cp));
		}
		if (_dir.y != 0.0f &&
			cp->axis == CA_BackForth &&
			((_dir.y < 0.0f && cp->dir < 0) ||
			 (_dir.y > 0.0f && cp->dir > 0)))
		{
			return find_platform_on_other_end_of_cart_line(_fromPlatform, for_everys_index(cp));
		}
		if (_dir.z != 0.0f &&
			is_vertical(cp->axis) &&
			((_dir.z < 0.0f && cp->dir < 0) ||
			 (_dir.z > 0.0f && cp->dir > 0)))
		{
			return find_platform_on_other_end_of_cart_line(_fromPlatform, for_everys_index(cp));
		}
	}
	return NONE;
}

PlatformsLinear::KeyPointInfo PlatformsLinear::create_key_point(int _platformDirs, VectorInt2 const& _atGrid, Optional<int> const& _usePlatformIdx, bool _allowMatchCartPoint)
{
	RoomGenerationInfo const & rggi = get_room_generation_info();

	an_assert(!(_platformDirs & PD_DownX));
	an_assert(!(_platformDirs & PD_DownY));
	an_assert(!(_platformDirs & PD_UpX));
	an_assert(!(_platformDirs & PD_UpY));
	ArrayStatic<PlatformDir, 4> dirs; SET_EXTRA_DEBUG_INFO(dirs, TXT("PlatformsLinear.dirs"));
	if (_platformDirs & PD_Left) dirs.push_back(PD_Left);
	if (_platformDirs & PD_Right) dirs.push_back(PD_Right);
	if (_platformDirs & PD_Back) dirs.push_back(PD_Back);
	if (_platformDirs & PD_Forth) dirs.push_back(PD_Forth);

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
	output(TXT("creating key point %i ======================"), dirs.get_size());
#endif
	PlatformsLinear::KeyPointInfo keyPoint;

	bool forceVerticals = platformsSetup.useVerticals.get() > 0.5f && random.get_chance(platformsSetup.useVerticals.get());
	if (forceVerticals && dirs.get_size() <= 1)
	{
		forceVerticals = false;
	}
	if (forceVerticals && dirs.get_size() <= 2 && random.get_chance(0.4f))
	{
		// more unlikely for two
		forceVerticals = false;
	}
	if (forceVerticals && _usePlatformIdx.is_set())
	{
		// we don't want to force vertical on existing platforms that have tiles available set
		// we don't want to mess that as verticals require more space
		auto& existingPlatform = platforms[_usePlatformIdx.get()];
		if (!existingPlatform.tilesAvailable.x.is_empty() ||
			!existingPlatform.tilesAvailable.y.is_empty())
		{
			forceVerticals = false;
		}
	}
	bool done = false;
	if (!done &&
		!forceVerticals &&
		(dirs.get_size() <= 2 ||
		 (dirs.get_size() == 3 && random.get_chance(0.7f))))
	{
		int redoAllDirsTriesLeft = 50;
		while (redoAllDirsTriesLeft--)
		{
			ArrayStatic<PlatformDir, 4> dirsAdded; SET_EXTRA_DEBUG_INFO(dirsAdded, TXT("PlatformsLinear.dirsAdded"));

			Platform platform;
			platform.type = PT_KeyPoint;
			platform.atGrid = _atGrid;
			an_assert(!_usePlatformIdx.is_set() || _atGrid == platforms[_usePlatformIdx.get()].atGrid.get(_atGrid));
			if (_usePlatformIdx.is_set())
			{
				auto& existingPlatform = platforms[_usePlatformIdx.get()];
				platform.tilesAvailable = existingPlatform.tilesAvailable;
				platform.blockCartSide = existingPlatform.blockCartSide;
				platform.matchCartPoint = existingPlatform.matchCartPoint; // used only here
			}
#ifdef AN_PLATFORM_DEBUG_INFO
			platform.debugInfo = TXT("key point");
#endif

			int pIdx = _usePlatformIdx.get(platforms.get_size()); // will be when we add it, unless we use existing one
			while (!dirs.is_empty())
			{
				// we should be able to add all three
				// try to add this dir to current platform
				CartPoint cartPoint;

				// choose next dir
				int dirIdx = NONE;

				bool added = false;
				{
					int triesLeft = 50;
					while (!added && (triesLeft--) > 0)
					{
						dirIdx = random.get_int(dirs.get_size());
						PlatformDir dir = dirs[dirIdx];

						cartPoint.set(dir);

						Optional<VectorInt2> preferredTilePlacement;
						Optional<int> preferredSide;
						if (_allowMatchCartPoint && triesLeft > 10)
						{
							if (platform.matchCartPoint &&
								platform.atGrid.is_set())
							{
								Optional<VectorInt2> matchOffset;
								if ((platform.matchCartPoint & PD_Back) &&
									(cartPoint.axis == CA_BackForth && cartPoint.dir < 0))
								{
									matchOffset = VectorInt2(0, -1);
								}
								if ((platform.matchCartPoint & PD_Forth) &&
									(cartPoint.axis == CA_BackForth && cartPoint.dir > 0))
								{
									matchOffset = VectorInt2(0, 1);
								}
								if ((platform.matchCartPoint & PD_Left) &&
									(cartPoint.axis == CA_LeftRight && cartPoint.dir < 0))
								{
									matchOffset = VectorInt2(-1, 0);
								}
								if ((platform.matchCartPoint & PD_Right) &&
									(cartPoint.axis == CA_LeftRight && cartPoint.dir > 0))
								{
									matchOffset = VectorInt2(1, 0);
								}

								if (matchOffset.is_set())
								{
									VectorInt2 matchAt = platform.atGrid.get() + matchOffset.get();
									for_every(op, platforms)
									{
										if (op->atGrid.is_set() &&
											op->atGrid == matchAt)
										{
											for_every(cp, op->cartPoints)
											{
												if (cp->axis == cartPoint.axis &&
													cp->dir * cartPoint.dir < 0)
												{
													preferredTilePlacement = cp->tilePlacement;
													preferredSide = cp->side;
													break;
												}
											}
											if (preferredTilePlacement.is_set())
											{
												break;
											}
										}
									}
								}
							}
						}

						int toPlatformIdx = pIdx; 
						if (find_placement_and_side_for(cartPoint, platform, preferredTilePlacement, preferredSide.get(0)))
						{
							platform.cartPoints.push_back(cartPoint);
							added = true;
							keyPoint.set(dir, toPlatformIdx, platform.cartPoints.get_size() - 1);
						}
					}
				}

				if (added)
				{
					dirsAdded.push_back(dirs[dirIdx]);
					dirs.remove_fast_at(dirIdx);
				}
				else
				{
					for_every(dir, dirsAdded)
					{
						dirs.push_back(*dir);
					}
					dirsAdded.clear();
					keyPoint = PlatformsLinear::KeyPointInfo(); // clear
					break;
				}
			}

			if (dirs.is_empty())
			{
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
				output(TXT("generated 1/2/3 way key point"));
#endif
				if (_usePlatformIdx.is_set())
				{
					auto& existingPlatform = platforms[_usePlatformIdx.get()];
					platform.type = existingPlatform.type;
#ifdef AN_PLATFORM_DEBUG_INFO
					platform.debugInfo = existingPlatform.debugInfo;
#endif
					existingPlatform = platform; // replace kpi and cartpoints
				}
				else
				{
					platforms.push_back(platform);
				}
				break;
			}
		}
	}
	done = dirs.is_empty();
	if (!done) // 3 or 4
	{
		if (rggi.is_small() || random.get_chance(platformsSetup.chanceOfUsingTwoPlatformsForFourCarts.get(0.4f)) || forceVerticals)
		{
			/*	small is very limited when it comes to 4-way key points
			 *	it's either this (or mirrored)			or this two-level (or mirrored)
			 *		      |
			 *		    B.F										 dR---
			 *		    |R---									B..
			 *		    |										|
			 *		    F.										  |
			 *		     .B									----Lu|
			 *		      |										..F
			 *		   --L|
			 *			B.F
			 *			|
			 */
			bool doTwoLevel = forceVerticals || random.get_chance(platformsSetup.useVerticals.get() * 0.55f);

			if (doTwoLevel)
			{
				/*
				 *			 dR---
				 * 			B..
				 *			|
				 *			  |
				 *		----Lu|
				 *			..F
				 */
				Platform upperPlatform;
				Platform lowerPlatform;

				upperPlatform.type = PT_KeyPoint;
				lowerPlatform.type = PT_KeyPoint;
				upperPlatform.atGrid = _atGrid;
				lowerPlatform.atGrid = _atGrid;
				an_assert(!_usePlatformIdx.is_set() || _atGrid == platforms[_usePlatformIdx.get()].atGrid.get(_atGrid));
				if (_usePlatformIdx.is_set())
				{
					auto& existingPlatform = platforms[_usePlatformIdx.get()];
					upperPlatform.tilesAvailable = existingPlatform.tilesAvailable;
					upperPlatform.blockCartSide = existingPlatform.blockCartSide;
					lowerPlatform.tilesAvailable = existingPlatform.tilesAvailable;
					lowerPlatform.blockCartSide = existingPlatform.blockCartSide;
				}
#ifdef AN_PLATFORM_DEBUG_INFO
				upperPlatform.debugInfo = TXT("upper");
				lowerPlatform.debugInfo = TXT("lower");
#endif

				int elevatorX = random.get_int_from_range(1, rggi.get_tile_count().x - 2);
				int elevatorY = random.get_chance(0.5f) ? 0 : rggi.get_tile_count().y - 1;

				// elevator cart points
				CartPoint upperElevator;
				upperElevator.axis = CA_VerticalY;
				upperElevator.side = elevatorY == 0 ? -1 : 1;
				upperElevator.dir = -1;
				upperElevator.tilePlacement = VectorInt2(elevatorX, elevatorY);
				upperPlatform.cartPoints.push_back(upperElevator);

				CartPoint lowerElevator = upperElevator;
				lowerElevator.dir = 1;
				lowerPlatform.cartPoints.push_back(lowerElevator);
				//

				int upperX = random.get_chance(0.5f) ? -1 : 1;
				int upperY = random.get_chance(0.5f) ? -1 : 1;

				int leftCartIdx = NONE;
				int rightCartIdx = NONE;
				int backCartIdx = NONE;
				int forthCartIdx = NONE;

				for_count(int, p, 2)
				{
					int thisX = p == 0 ? upperX : -upperX;
					int thisY = p == 0 ? upperY : -upperY;
					PlatformDir thisXDir = thisX < 0 ? PD_Left : PD_Right;
					PlatformDir thisYDir = thisY < 0 ? PD_Back : PD_Forth;
					int& thisXCartIdx = thisX < 0 ? leftCartIdx : rightCartIdx;
					int& thisYCartIdx = thisY < 0 ? backCartIdx : forthCartIdx;
					auto& platform = p == 0 ? upperPlatform : lowerPlatform;
					if (_platformDirs & thisXDir)
					{
						CartPoint cpX;
						cpX.axis = CA_LeftRight;
						cpX.side = upperElevator.side; // same for both
						cpX.dir = thisX;
						cpX.tilePlacement = thisX < 0 ? VectorInt2(random.get_int_from_range(0, elevatorX - 1), elevatorY)
							: VectorInt2(random.get_int_from_range(elevatorX + 1, rggi.get_tile_count().x - 1), elevatorY);
						an_assert(validate_placement_and_side_for(cpX, platform));
						thisXCartIdx = platform.cartPoints.get_size();
						platform.cartPoints.push_back(cpX);
					}
					if (_platformDirs & thisYDir)
					{
						CartPoint cpY;
						cpY.axis = CA_BackForth;
						cpY.side = -thisX;
						cpY.dir = thisY;
						int atY = elevatorY == 0 ? random.get_int_from_range(1, rggi.get_tile_count().y - 1)
							: random.get_int_from_range(0, elevatorY - 2);
						cpY.tilePlacement = thisX > 0 ? VectorInt2(random.get_int_from_range(0, elevatorX - 1), atY)
							: VectorInt2(random.get_int_from_range(elevatorX + 1, rggi.get_tile_count().x - 1), atY);
						an_assert(validate_placement_and_side_for(cpY, platform));
						thisYCartIdx = platform.cartPoints.get_size();
						platform.cartPoints.push_back(cpY);
					}
				}

				int keepSize = platforms.get_size();

				int upIdx = platforms.get_size();
				if (_usePlatformIdx.is_set())
				{
					auto& existingPlatform = platforms[_usePlatformIdx.get()];
					upperPlatform.type = existingPlatform.type;
#ifdef AN_PLATFORM_DEBUG_INFO
					upperPlatform.debugInfo = existingPlatform.debugInfo;
#endif
					existingPlatform = upperPlatform; // replace kpi and cartpoints
					upIdx = _usePlatformIdx.get();
				}
				else
				{
					platforms.push_back(upperPlatform);
				}

				int loIdx = platforms.get_size();
				platforms.push_back(lowerPlatform);

				if (!add_cart_between(upIdx, loIdx, CA_VerticalY, -1))
				{
					an_assert(false, TXT("couldn't work this out"));
					platforms.set_size(keepSize);
				}
				else
				{
					keyPoint.left.cartPointIdx = leftCartIdx;
					keyPoint.right.cartPointIdx = rightCartIdx;
					keyPoint.back.cartPointIdx = backCartIdx;
					keyPoint.forth.cartPointIdx = forthCartIdx;

					keyPoint.left.platformIdx = leftCartIdx != NONE ? (upperX < 0 ? upIdx : loIdx) : NONE;
					keyPoint.right.platformIdx = rightCartIdx != NONE ? (upperX < 0 ? loIdx : upIdx) : NONE;
					keyPoint.back.platformIdx = backCartIdx != NONE ? (upperY < 0 ? upIdx : loIdx) : NONE;
					keyPoint.forth.platformIdx = forthCartIdx != NONE ? (upperY < 0 ? loIdx : upIdx) : NONE;

					dirs.clear();
				}

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
				output(TXT("generated 4-way two-level key point"));
#endif
			}
			else
			{
				/*
				 *		      |
				 *		    B.F
				 *		    |R---
				 *		    |
				 *		    F.
				 *		     .B
				 *		      |
				 *		   --L|
				 *			B.F
				 *			|
				 */
				Platform farPlatform;
				Platform nearPlatform;

				farPlatform.type = PT_KeyPoint;
				nearPlatform.type = PT_KeyPoint;
				farPlatform.atGrid = _atGrid;
				nearPlatform.atGrid = _atGrid;
				an_assert(!_usePlatformIdx.is_set() || _atGrid == platforms[_usePlatformIdx.get()].atGrid.get(_atGrid));
				if (_usePlatformIdx.is_set())
				{
					auto& existingPlatform = platforms[_usePlatformIdx.get()];
					farPlatform.tilesAvailable = existingPlatform.tilesAvailable;
					farPlatform.blockCartSide = existingPlatform.blockCartSide;
					nearPlatform.tilesAvailable = existingPlatform.tilesAvailable;
					nearPlatform.blockCartSide = existingPlatform.blockCartSide;
				}
#ifdef AN_PLATFORM_DEBUG_INFO
				farPlatform.debugInfo = TXT("far");
				nearPlatform.debugInfo = TXT("near");
#endif

				// this determines on which side is back and which forth
				int farX = random.get_chance(0.5f) ? -1 : 1;

				int middleX = random.get_int_from_range(1, rggi.get_tile_count().x - 2);

				int leftCartIdx = NONE;
				int rightCartIdx = NONE;
				int backCartIdx = NONE;
				int forthCartIdx = NONE;

				for_count(int, p, 2)
				{
					int thisX = p == 0 ? farX : -farX;
					PlatformDir thisXDir = thisX < 0 ? PD_Left : PD_Right;
					PlatformDir thisYDir = p == 0 ? PD_Forth : PD_Back;
					int& thisXCartIdx = thisX < 0 ? leftCartIdx : rightCartIdx;
					int& thisYCartIdx = p == 0 ? forthCartIdx : backCartIdx;
					auto& platform = p == 0 ? farPlatform : nearPlatform;
					{
						CartPoint cpTO; // towards other
						cpTO.axis = CA_BackForth;
						cpTO.side = -thisX;
						cpTO.dir = p == 0 ? -1 : 1;
						int atY = p == 0 ? random.get_int_from_range(1, rggi.get_tile_count().y - 1)
							: random.get_int_from_range(0, rggi.get_tile_count().y - 2);
						cpTO.tilePlacement = thisX > 0 ? VectorInt2(random.get_int_from_range(0, middleX - 1), atY)
							: VectorInt2(random.get_int_from_range(middleX + 1, rggi.get_tile_count().x - 1), atY);
						an_assert(validate_placement_and_side_for(cpTO, platform));
						platform.cartPoints.push_back(cpTO);
					}
					if (_platformDirs & thisXDir)
					{
						CartPoint cpSide;
						cpSide.axis = CA_LeftRight;
						cpSide.side = p == 0 ? -1 : 1;
						cpSide.dir = thisX;
						cpSide.tilePlacement = VectorInt2(middleX, p == 0 ? 0 : rggi.get_tile_count().y - 1);
						an_assert(validate_placement_and_side_for(cpSide, platform));
						thisXCartIdx = platform.cartPoints.get_size();
						platform.cartPoints.push_back(cpSide);
					}
					if (_platformDirs & thisYDir)
					{
						CartPoint cpA; // away
						cpA.axis = CA_BackForth;
						cpA.side = thisX;
						cpA.dir = p == 0 ? 1 : -1;
						int atY = p == 0 ? random.get_int_from_range(1, rggi.get_tile_count().y - 1)
							: random.get_int_from_range(0, rggi.get_tile_count().y - 2);
						cpA.tilePlacement = thisX > 0 ? VectorInt2(random.get_int_from_range(middleX + 1, rggi.get_tile_count().x - 1), atY)
							: VectorInt2(random.get_int_from_range(0, middleX - 1), atY);
						an_assert(validate_placement_and_side_for(cpA, platform));
						thisYCartIdx = platform.cartPoints.get_size();
						platform.cartPoints.push_back(cpA);
					}
				}

				int farIdx = platforms.get_size();
				if (_usePlatformIdx.is_set())
				{
					auto& existingPlatform = platforms[_usePlatformIdx.get()];
					farPlatform.type = existingPlatform.type;
#ifdef AN_PLATFORM_DEBUG_INFO
					farPlatform.debugInfo = existingPlatform.debugInfo;
#endif
					existingPlatform = farPlatform; // replace kpi and cartpoints
					farIdx = _usePlatformIdx.get();
				}
				else
				{
					platforms.push_back(farPlatform);
				}

				int nearIdx = platforms.get_size();
				platforms.push_back(nearPlatform);

				connect_platforms(farIdx, nearIdx, CA_BackForth, -1, CA_BackForth, 1);

				keyPoint.left.cartPointIdx = leftCartIdx;
				keyPoint.right.cartPointIdx = rightCartIdx;
				keyPoint.back.cartPointIdx = backCartIdx;
				keyPoint.forth.cartPointIdx = forthCartIdx;

				keyPoint.left.platformIdx = leftCartIdx != NONE ? (farX < 0 ? farIdx : nearIdx) : NONE;
				keyPoint.right.platformIdx = rightCartIdx != NONE ? (farX < 0 ? nearIdx : farIdx) : NONE;
				keyPoint.back.platformIdx = backCartIdx != NONE ? nearIdx : NONE;
				keyPoint.forth.platformIdx = forthCartIdx != NONE ? farIdx : NONE;

				dirs.clear();

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
				output(TXT("generated 4-way two-level key point"));
#endif
			}
		}
	}
	done = dirs.is_empty();
	if (!done)
	{
		{
			bool clockwise = random.get_chance(0.5f);
			// clockwise			anticlockwise
			//		^					  ^
			//		|R->			   <-L|
			//		F B					B F
			//	   <-L|				    |R->
			//		  v					v

			int pIdx = _usePlatformIdx.get(platforms.get_size());

			Platform platform;
			platform.type = PT_KeyPoint;
			platform.atGrid = _atGrid;
			an_assert(!_usePlatformIdx.is_set() || _atGrid == platforms[_usePlatformIdx.get()].atGrid.get(_atGrid));
			if (_usePlatformIdx.is_set())
			{
				auto& existingPlatform = platforms[_usePlatformIdx.get()];
				platform.tilesAvailable = existingPlatform.tilesAvailable;
				platform.blockCartSide = existingPlatform.blockCartSide;
			}
#ifdef AN_PLATFORM_DEBUG_INFO
			platform.debugInfo = TXT("clock");
#endif

			int xSide[] = { random.get_int_from_range(0, rggi.get_tile_count().x - 3),
							random.get_int_from_range(2, rggi.get_tile_count().x - 1) };
			int ySide[] = { random.get_int_from_range(0, rggi.get_tile_count().y - 3),
							random.get_int_from_range(2, rggi.get_tile_count().y - 1) };

			// move them apart if too close
			while (xSide[1] - xSide[0] < 2)
			{
				xSide[0] = max(xSide[0] - 1, 0);
				xSide[1] = min(xSide[1] + 1, rggi.get_tile_count().x - 1);
			}
			while (ySide[1] - ySide[0] < 2)
			{
				ySide[0] = max(ySide[0] - 1, 0);
				ySide[1] = min(ySide[1] + 1, rggi.get_tile_count().y - 1);
			}

			// cl
			if (_platformDirs & PD_Right)
			{
				CartPoint cp;
				cp.axis = CA_LeftRight;
				cp.side = clockwise ? 1 : -1;
				cp.dir = 1;
				cp.tilePlacement = VectorInt2(random.get_int_from_range(xSide[0] + 1, xSide[1] - 1), ySide[max(0, cp.side)]);
				keyPoint.right.platformIdx = pIdx;
				keyPoint.right.cartPointIdx = platform.cartPoints.get_size();
				an_assert(validate_placement_and_side_for(cp, platform));
				platform.cartPoints.push_back(cp);
			}
			if (_platformDirs & PD_Left)
			{
				CartPoint cp;
				cp.axis = CA_LeftRight;
				cp.side = clockwise ? -1 : 1;
				cp.dir = -1;
				cp.tilePlacement = VectorInt2(random.get_int_from_range(xSide[0] + 1, xSide[1] - 1), ySide[max(0, cp.side)]);
				keyPoint.left.platformIdx = pIdx;
				keyPoint.left.cartPointIdx = platform.cartPoints.get_size();
				an_assert(validate_placement_and_side_for(cp, platform));
				platform.cartPoints.push_back(cp);
			}
			if (_platformDirs & PD_Forth)
			{
				CartPoint cp;
				cp.axis = CA_BackForth;
				cp.side = clockwise ? -1 : 1;
				cp.dir = 1;
				cp.tilePlacement = VectorInt2(xSide[max(0, cp.side)], random.get_int_from_range(ySide[0] + 1, ySide[0] - 1));
				keyPoint.forth.platformIdx = pIdx;
				keyPoint.forth.cartPointIdx = platform.cartPoints.get_size();
				an_assert(validate_placement_and_side_for(cp, platform));
				platform.cartPoints.push_back(cp);
			}
			if (_platformDirs & PD_Back)
			{
				CartPoint cp;
				cp.axis = CA_BackForth;
				cp.side = clockwise ? 1 : -1;
				cp.dir = -1;
				cp.tilePlacement = VectorInt2(xSide[max(0, cp.side)], random.get_int_from_range(ySide[0] + 1, ySide[0] - 1));
				keyPoint.back.platformIdx = pIdx;
				keyPoint.back.cartPointIdx = platform.cartPoints.get_size();
				an_assert(validate_placement_and_side_for(cp, platform));
				platform.cartPoints.push_back(cp);
			}

			if (_usePlatformIdx.is_set())
			{
				auto& existingPlatform = platforms[_usePlatformIdx.get()];
				platform.type = existingPlatform.type;
#ifdef AN_PLATFORM_DEBUG_INFO
				platform.debugInfo = existingPlatform.debugInfo;
#endif
				existingPlatform = platform; // replace kpi and cartpoints
			}
			else
			{
				platforms.push_back(platform);
			}

			dirs.clear();

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
			output(TXT("generated 4-way full-circle key point"));
#endif
		}
	}

	an_assert(dirs.is_empty(), TXT("this was supposed to work properly and we should not rely on a fail safe"));

	if (!dirs.is_empty())
	{
		Array<int> keyPointPlatforms;

		{	// have one platform
			Platform platform;
			platform.type = PT_KeyPoint;
			platform.atGrid = _atGrid;
			an_assert(!_usePlatformIdx.is_set() || _atGrid == platforms[_usePlatformIdx.get()].atGrid.get(_atGrid));
			if (_usePlatformIdx.is_set())
			{
				auto& existingPlatform = platforms[_usePlatformIdx.get()];
				platform.tilesAvailable = existingPlatform.tilesAvailable;
				platform.blockCartSide = existingPlatform.blockCartSide;
			}
#ifdef AN_PLATFORM_DEBUG_INFO
			platform.debugInfo = TXT("one");
#endif

			int kppIdx;
			if (_usePlatformIdx.is_set())
			{
				auto& existingPlatform = platforms[_usePlatformIdx.get()];
				platform.type = existingPlatform.type;
#ifdef AN_PLATFORM_DEBUG_INFO
				platform.debugInfo = existingPlatform.debugInfo;
#endif
				existingPlatform = platform; // replace kpi and cartpoints
				kppIdx = _usePlatformIdx.get();
			}
			else
			{
				platforms.push_back(platform);
				kppIdx = platforms.get_size() - 1;
			}

			keyPointPlatforms.push_back(kppIdx);
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
			output(TXT("added initial key point platform %i"), platforms.get_size() - 1);
	#endif
		}

		while (!dirs.is_empty())
		{
			an_assert(keyPointPlatforms.get_size() < 10);

			// try to add this dir to current platform
			CartPoint cartPoint;

			// choose next dir
			int dirIdx = NONE;

			bool added = false;
			{
				int triesLeft = 50;
				while (!added && (triesLeft--) > 0)
				{
					dirIdx = random.get_int(dirs.get_size());
					PlatformDir dir = dirs[dirIdx];

					cartPoint.set(dir);

					int toPlatformIdx = keyPointPlatforms[random.get_int(keyPointPlatforms.get_size())];
					if (find_placement_and_side_for(cartPoint, platforms[toPlatformIdx]))
					{
						platforms[toPlatformIdx].cartPoints.push_back(cartPoint);
						added = true;
						keyPoint.set(dir, toPlatformIdx, platforms[toPlatformIdx].cartPoints.get_size() - 1);
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
						output(TXT("added cart point to existing key point platform %i"), toPlatformIdx);
						for_every(l, platforms[toPlatformIdx].get_layout(this))
						{
							output(TXT("  %S"), l->to_char());
						}
#endif
					}
				}
			}

			if (added)
			{
				dirs.remove_fast_at(dirIdx);
			}
			else
			{
				// we can't add it anywhere to existing platforms
				// we will have to disconnect one cart and create platform there to add cart
				PlatformDir rDir;
				ElevatorCartPointRef pcpRef;
				if (keyPoint.remove(random, rDir, pcpRef))
				{
					// we will have to redo this dir
					dirs.push_back(rDir);

					// add platform that is on the other side of pcpRef
					Platform newPlatform;
					newPlatform.type = PT_KeyPoint;
					newPlatform.atGrid = _atGrid;
#ifdef AN_PLATFORM_DEBUG_INFO
					newPlatform.debugInfo = TXT("??");
#endif

					auto * pcp = find_cart_point(pcpRef);
					if (random.get_chance(platformsSetup.useVerticals.get()) && random.get_chance(0.5f))
					{
						// make sure there are no more than two vertical carts and they should be going in opposing directions
						int verticalCount = 0;
						int verticalDir = random.get_chance(0.5f) ? -1 : 1;
						for_every(cp, platforms[pcpRef.platformIdx].cartPoints)
						{
							if (is_vertical(cp->axis))
							{
								++verticalCount;
								verticalDir = -cp->dir;
							}
						}
						if (verticalCount < 2)
						{
							// change dir to vertical
							if (pcp->axis == CA_BackForth) { pcp->axis = CA_VerticalX; pcp->dir = verticalDir; rDir = pcp->dir > 0 ? PD_UpX : PD_DownX; } else
							if (pcp->axis == CA_LeftRight) { pcp->axis = CA_VerticalY; pcp->dir = verticalDir; rDir = pcp->dir > 0 ? PD_UpY : PD_DownY; }
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
							output(TXT("modified platform %i"), pcpRef.platformIdx);
							for_every(l, platforms[pcpRef.platformIdx].get_layout(this))
							{
								output(TXT("  %S"), l->to_char());
							}
#endif
						}
					}

					// opposite dir
					CartPoint cartPoint;
					PlatformDir oDir = opposite(rDir);
					cartPoint.set(oDir);

					bool added = false;
					if (pcp->axis == CA_VerticalX ||
						pcp->axis == CA_VerticalY)
					{
						if (find_placement_and_side_for(cartPoint, newPlatform, pcp->tilePlacement, pcp->side))
						{
							added = true;
							newPlatform.cartPoints.push_back(cartPoint);
						}
					}
					else
					{
						if (find_placement_and_side_for(cartPoint, newPlatform, pcp->tilePlacement, pcp->side))
						{
							added = true;
							newPlatform.cartPoints.push_back(cartPoint);
						}
					}
					if (!added)
					{
						int triesLeft = 50;
						while (!added && (triesLeft--) > 0)
						{
							if (find_placement_and_side_for(cartPoint, newPlatform))
							{
								added = true;
								newPlatform.cartPoints.push_back(cartPoint);
							}
						}
					}

					an_assert(added);
					platforms.push_back(newPlatform);
					int newPlatformIdx = platforms.get_size() - 1;
					keyPointPlatforms.push_back(newPlatformIdx);
#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
					output(TXT("added key point platform %i"), platforms.get_size() - 1);
					for_every(l, platforms[newPlatformIdx].get_layout(this))
					{
						output(TXT("  %S"), l->to_char());
					}
#endif

					connect_platforms(pcpRef.platformIdx, newPlatformIdx, axis_of(rDir), dir_of(rDir), axis_of(oDir), dir_of(oDir));
				}
			}
		}
	}

#ifdef OUTPUT_GENERATION_DETAILED_EXTENSIVELY
	output(TXT("result key point:"));
	if (keyPoint.left.platformIdx != NONE)
	{
		output(TXT(" left %i"), keyPoint.left.platformIdx);
		for_every(l, platforms[keyPoint.left.platformIdx].get_layout(this))
		{
			output(TXT("  %S"), l->to_char());
		}
	}
	if (keyPoint.right.platformIdx != NONE)
	{
		output(TXT(" right %i"), keyPoint.right.platformIdx);
		for_every(l, platforms[keyPoint.right.platformIdx].get_layout(this))
		{
			output(TXT("  %S"), l->to_char());
		}
	}
	if (keyPoint.back.platformIdx != NONE)
	{
		output(TXT(" back %i"), keyPoint.back.platformIdx);
		for_every(l, platforms[keyPoint.back.platformIdx].get_layout(this))
		{
			output(TXT("  %S"), l->to_char());
		}
	}
	if (keyPoint.forth.platformIdx != NONE)
	{
		output(TXT(" forth %i"), keyPoint.forth.platformIdx);
		for_every(l, platforms[keyPoint.forth.platformIdx].get_layout(this))
		{
			output(TXT("  %S"), l->to_char());
		}
	}
#endif

	return keyPoint;
}

PlatformsLinear::PlatformDir PlatformsLinear::opposite(PlatformDir _dir)
{
	if (_dir == PD_Left) return PD_Right;
	if (_dir == PD_Right) return PD_Left;
	if (_dir == PD_Back) return PD_Forth;
	if (_dir == PD_Forth) return PD_Back;
	if (_dir == PD_DownX) return PD_UpX;
	if (_dir == PD_UpX) return PD_DownX;
	if (_dir == PD_DownY) return PD_UpY;
	if (_dir == PD_UpY) return PD_DownY;
	todo_important(TXT("implement_"));
	return PD_Right;
}

PlatformsLinear::CartAxis PlatformsLinear::axis_of(PlatformDir _dir)
{
	if (_dir == PD_Left) return CA_LeftRight;
	if (_dir == PD_Right) return CA_LeftRight;
	if (_dir == PD_Back) return CA_BackForth;
	if (_dir == PD_Forth) return CA_BackForth;
	if (_dir == PD_DownX) return CA_VerticalX;
	if (_dir == PD_UpX) return CA_VerticalX;
	if (_dir == PD_DownY) return CA_VerticalY;
	if (_dir == PD_UpY) return CA_VerticalY;
	todo_important(TXT("implement_"));
	return CA_LeftRight;
}

int PlatformsLinear::dir_of(PlatformDir _dir)
{
	if (_dir == PD_Left) return -1;
	if (_dir == PD_Right) return 1;
	if (_dir == PD_Back) return -1;
	if (_dir == PD_Forth) return 1;
	if (_dir == PD_DownX) return -1;
	if (_dir == PD_UpX) return 1;
	if (_dir == PD_DownY) return -1;
	if (_dir == PD_UpY) return 1;
	todo_important(TXT("implement_"));
	return 0;
}

bool PlatformsLinear::is_entry_x(CartAxis _ca)
{
	if (_ca == CA_BackForth || _ca == CA_VerticalX) return true;
	if (_ca == CA_LeftRight || _ca == CA_VerticalY) return false;
	todo_important(TXT("implement_"));
	return false;
}

PlatformsLinear::CartAxis PlatformsLinear::change_verticality(CartAxis _ca)
{
	if (_ca == CA_BackForth) return CA_VerticalX;
	if (_ca == CA_LeftRight) return CA_VerticalY;
	if (_ca == CA_VerticalX) return CA_BackForth;
	if (_ca == CA_VerticalY) return CA_LeftRight;
	todo_important(TXT("implement_"));
	return CA_VerticalX;
}

PlatformsLinear::CartAxis PlatformsLinear::change_perpendicular(CartAxis _ca)
{
	if (_ca == CA_BackForth) return CA_LeftRight;
	if (_ca == CA_LeftRight) return CA_BackForth;
	if (_ca == CA_VerticalX) return CA_VerticalY;
	if (_ca == CA_VerticalY) return CA_VerticalX;
	todo_important(TXT("implement_"));
	return CA_VerticalX;
}

bool PlatformsLinear::is_vertical(CartAxis _ca)
{
	if (_ca == CA_VerticalX || _ca == CA_VerticalY) return true;
	if (_ca == CA_BackForth || _ca == CA_LeftRight) return false;
	todo_important(TXT("implement_"));
	return false;
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
tchar PlatformsLinear::get_dir_as_char(CartAxis _axis, int _dir)
{
	if (_axis == CA_LeftRight)
	{
		return _dir < 0 ? '<' : '>';
	}
	else if (_axis == CA_BackForth)
	{
		return _dir < 0 ? 'v' : '^';
	}
	else if (_axis == CA_VerticalX)
	{
		return _dir < 0 ? 'd' : 'u';
	}
	else if (_axis == CA_VerticalY)
	{
		return _dir < 0 ? 'D' : 'U';
	}
	todo_important(TXT("implement_"));
	return ' ';
}

tchar PlatformsLinear::get_dir_as_char(PlatformDir _dir)
{
	if (_dir == PD_Left) return '<';
	if (_dir == PD_Right) return '>';
	if (_dir == PD_Back) return 'v';
	if (_dir == PD_Forth) return '^';
	if (_dir == PD_DownX) return 'd';
	if (_dir == PD_UpX) return 'u';
	if (_dir == PD_DownY) return 'D';
	if (_dir == PD_UpY) return 'U';
	todo_important(TXT("implement_"));
	return ' ';
}
#endif

bool PlatformsLinear::adjust_platforms_vertically()
{
	Range3 allPlatformsBBox = Range3::empty;
	for_every(platform, platforms)
	{
		allPlatformsBBox.include(platform->at);
	}

	float xA = verticalAdjustment.xA.get().centre();
	float yA = verticalAdjustment.yA.get().centre();
	float xB = verticalAdjustment.xB.get().centre();
	float yB = verticalAdjustment.yB.get().centre();

	if (xA == 0.0f && xB == 0.0f &&
		yA == 0.0f && yB == 0.0f)
	{
		return false;
	}

	float h = 0.0f;
	if (verticalAdjustment.hPt.is_set())
	{
		h = verticalAdjustment.hPt.get().centre() * max(allPlatformsBBox.x.length(), allPlatformsBBox.y.length());
		if (verticalAdjustment.hLimit.is_set())
		{
			h = clamp(h, verticalAdjustment.hLimit.get().min, verticalAdjustment.hLimit.get().max);
		}
	}
	else
	{
		Random::Generator random = platformsSetup.random;
		if (verticalAdjustment.hLimit.is_set())
		{
			h = random .get(verticalAdjustment.hLimit.get());
		}
	}

	for_every(platform, platforms)
	{
		Vector2 normalisedAt = Vector2(allPlatformsBBox.x.get_pt_from_value(platform->at.x),
									   allPlatformsBBox.y.get_pt_from_value(platform->at.y));

		normalisedAt = (normalisedAt - Vector2::half) * 2.0f;

		platform->at.z += h * (xA * sqr(normalisedAt.x) + xB * normalisedAt.x);
		platform->at.z += h * (yA * sqr(normalisedAt.y) + yB * normalisedAt.y);
	}

	return true;
}
