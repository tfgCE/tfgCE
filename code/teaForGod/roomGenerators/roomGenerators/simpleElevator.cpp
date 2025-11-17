#include "simpleElevator.h"

#include "..\roomGenerationInfo.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\environmentInfo.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomGeneratorInfo.inl"
#include "..\..\..\framework\world\roomRegionVariables.inl"
#include "..\..\..\framework\world\world.h"

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

//

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
//#define OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
//#define OUTPUT_GENERATION_VARIABLES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

DEFINE_STATIC_NAME(vrCorridorUsing180);

DEFINE_STATIC_NAME(vrElevatorDoor); // door to the actual elevator

DEFINE_STATIC_NAME(vrCompact);
DEFINE_STATIC_NAME(alignToTiles);
DEFINE_STATIC_NAME(cartPointThickness);

DEFINE_STATIC_NAME(laneVector);
DEFINE_STATIC_NAME(cartHeight);
DEFINE_STATIC_NAME(cartWidth);
DEFINE_STATIC_NAME(cartLength);
DEFINE_STATIC_NAME(railingHeight);
DEFINE_STATIC_NAME(elevatorStopA);
DEFINE_STATIC_NAME(elevatorStopB);
DEFINE_STATIC_NAME(elevatorStopACartPoint);
DEFINE_STATIC_NAME(elevatorStopBCartPoint);

DEFINE_STATIC_NAME(elevatorWidth);
DEFINE_STATIC_NAME(elevatorEdgeTop);

//
DEFINE_STATIC_NAME_STR(navNodeScenery, TXT("nav node scenery"));

//

void SimpleElevatorAppearanceSetup::setup_parameters(Framework::Room const * _room, OUT_ SimpleVariableStorage & _parameters) const
{
	float useElevatorHeight = _room->get_value<float>(NAME(cartHeight), elevatorHeight);
	float useRailingHeight = _room->get_value<float>(NAME(railingHeight), railingHeight);

	_parameters.access<float>(NAME(cartHeight)) = useElevatorHeight * Framework::Door::get_nominal_door_height_scale();
	_parameters.access<float>(NAME(railingHeight)) = useRailingHeight * Framework::Door::get_nominal_door_height_scale();
}

void SimpleElevatorAppearanceSetup::setup_variables(Framework::Room const* _room, Framework::Scenery* _scenery) const
{
	setup_parameters(_room, _scenery->access_variables());
}

//

REGISTER_FOR_FAST_CAST(SimpleElevatorInfo);

SimpleElevatorInfo::SimpleElevatorInfo()
{
}

SimpleElevatorInfo::~SimpleElevatorInfo()
{
}

bool SimpleElevatorInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= floorDistance.load_from_xml(_node, TXT("floorDistance"));

	result &= elevatorDoorType.load_from_xml(_node, TXT("elevatorDoorType"));
	result &= floorRoomType.load_from_xml(_node, TXT("floorRoomType"));

	result &= cartPointSceneryType.load_from_xml(_node, TXT("cartPointSceneryType"));
	result &= cartSceneryType.load_from_xml(_node, TXT("cartSceneryType"));
	result &= cartLaneSceneryType.load_from_xml(_node, TXT("cartLaneSceneryType"));
	
	_lc.load_group_into(elevatorDoorType);
	_lc.load_group_into(floorRoomType);

	_lc.load_group_into(cartPointSceneryType);
	_lc.load_group_into(cartSceneryType);
	_lc.load_group_into(cartLaneSceneryType);

	result &= maxElevatorLength.load_from_xml(_node, TXT("maxElevatorLength"));
	result &= maxElevatorTileLength.load_from_xml(_node, TXT("maxElevatorTileLength"));
	result &= maxElevatorLengthAdd.load_from_xml(_node, TXT("maxElevatorLengthAdd"));

	{
		String readType = _node->get_string_attribute(TXT("elevatorType"));
		if (!readType.is_empty())
		{
			if (readType == TXT("vertical"))
			{
				type = ElevatorType::Vertical;
			}
			else if (readType == TXT("horizontal"))
			{
				type = ElevatorType::Horizontal;
			}
			else if (readType == TXT("rotateXY"))
			{
				type = ElevatorType::RotateXY;
			}
			else
			{
				error_loading_xml(_node, TXT("elevatorType \"%S\" not recognised"), readType.to_char());
				result = false;
			}
		}
	}

	error_loading_xml_on_assert(floorRoomType.is_set(), result, _node, TXT("floorRoomType required for simple elevator info"));

	return result;
}

bool SimpleElevatorInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool SimpleElevatorInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr SimpleElevatorInfo::create_copy() const
{
	SimpleElevatorInfo * copy = new SimpleElevatorInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool SimpleElevatorInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	SimpleElevator sr(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("simple elevator \"%S\" : \"%S\""), _room->get_room_type() ? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= sr.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

SimpleElevator::SimpleElevator(SimpleElevatorInfo const * _info)
: info(_info)
{
}

SimpleElevator::~SimpleElevator()
{
}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED
#define SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(info) \
	{ \
		VR::Zone elevatorZone; \
		elevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth)); \
		elevatorZone.place_at(elevatorVRLoc, elevatorVRDir); \
		VR::Zone safeElevatorZone; \
		safeElevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth)); \
		safeElevatorZone.place_at(safeElevatorVRLoc, elevatorVRDir); \
	 \
		dv->start_gathering_data(); \
		dv->clear(); \
	 \
		roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::c64LightBlue); \
	 \
		elevatorZone.debug_visualise(dv.get(), Colour::magenta); \
		safeElevatorZone.debug_visualise(dv.get(), Colour::green); \
		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d0, vrPlacement0.get_translation().to_vector2(), vrPlacement0.get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::red); \
		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d1, vrPlacement1.get_translation().to_vector2(), vrPlacement1.get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::blue); \
	 \
		dv->add_text(Vector2::zero, String(info), Colour::black, 0.3f); \
		dv->end_gathering_data(); \
		dv->show_and_wait_for_key(); \
	}
#else
#define SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(info)
#endif

bool SimpleElevator::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	scoped_call_stack_info(TXT("SimpleElevator::generate"));

	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();

	if (_room->get_all_doors().get_size() != 2)
	{
		error(TXT("corridor should have 2 doors"));
		return false;
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generating simple elevator %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif

	Random::Generator rg = _room->get_individual_random_generator();
	rg.advance_seed(232, 6477673);

	_room->collect_variables(roomVariables);
	info->apply_generation_parameters_to(roomVariables);

	float cartPointThickness = roomVariables.get_value<float>(NAME(cartPointThickness), 0.04f) + 0.01f; // a bit more space to avoid z-buffor fighting

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("simple elevator"))));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::magenta));
	dv->activate();
#endif

	Framework::DoorInRoom * d0 = _room->get_all_doors()[0];
	Framework::DoorInRoom * d1 = _room->get_all_doors()[1];
	Transform vrPlacement0 = d0->get_vr_space_placement();
	Transform vrPlacement1 = d1->get_vr_space_placement();

	if (_room->get_tags().get_tag(NAME(vrCorridorUsing180)))
	{
		vrPlacement1 = Transform::do180.to_world(vrPlacement1);
	}

	Vector2 vrLoc0 = vrPlacement0.get_translation().to_vector2();
	Vector2 vrLoc1 = vrPlacement1.get_translation().to_vector2();
	Vector2 vrDir0 = vrPlacement0.get_axis(Axis::Y).to_vector2();
	Vector2 vrDir1 = vrPlacement1.get_axis(Axis::Y).to_vector2();

	float doorWidth = d0->get_door()->calculate_vr_width();
	if (doorWidth == 0.0f)
	{
		doorWidth = roomGenerationInfo.get_tile_size();
	}
	float halfDoorWidth = doorWidth * 0.5f;

	float elevatorWidth = doorWidth;
	float elevatorLength = doorWidth;

	Optional<float> limitElevatorLength;
	if (info->maxElevatorLength.is_set())
	{
		float actual = info->maxElevatorLength.get(roomVariables, 1000.0f, AllowToFail);
		if (info->maxElevatorLength.get_value_param().is_set())
		{
			// in case we're dealing with Range, get min value
			actual = roomVariables.get_value<Range>(info->maxElevatorLength.get_value_param().get(), Range(actual)).min;
		}
		limitElevatorLength = min(actual, limitElevatorLength.get(actual));
	}
	if (info->maxElevatorTileLength.is_set())
	{
		float inTiles;
		inTiles = info->maxElevatorTileLength.get(roomVariables, 1000.0f, AllowToFail);
		if (info->maxElevatorTileLength.get_value_param().is_set())
		{
			// in case we're dealing with Range, get min value
			inTiles = roomVariables.get_value<Range>(info->maxElevatorTileLength.get_value_param().get(), Range(inTiles)).min;
		}
		float actual = inTiles * RoomGenerationInfo::get().get_tile_size();
		limitElevatorLength = min(actual, limitElevatorLength.get(actual));
	}
	if (limitElevatorLength.is_set())
	{
		if (info->maxElevatorLengthAdd.is_set())
		{
			limitElevatorLength = limitElevatorLength.get() + info->maxElevatorLengthAdd.get(roomVariables, 0.0f, AllowToFail);
		}
	}

	elevatorLength = min(elevatorLength, limitElevatorLength.get(elevatorLength));

	// door corners
	Vector2 vrDoorL0 = vrPlacement0.location_to_world(Vector3(-halfDoorWidth, 0.0f, 0.0f)).to_vector2();
	Vector2 vrDoorR0 = vrPlacement0.location_to_world(Vector3(halfDoorWidth, 0.0f, 0.0f)).to_vector2();
	Vector2 vrDoorL1 = vrPlacement1.location_to_world(Vector3(-halfDoorWidth, 0.0f, 0.0f)).to_vector2();
	Vector2 vrDoorR1 = vrPlacement1.location_to_world(Vector3(halfDoorWidth, 0.0f, 0.0f)).to_vector2();
	Vector2 vrDoorCorners[] = { vrDoorL0, vrDoorR0, vrDoorL1, vrDoorR1 };

	// outbound zones
	VR::Zone vrZoneD0;
	vrZoneD0.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth));
	vrZoneD0 = vrZoneD0.to_world_of(vrPlacement0);
	VR::Zone vrZoneD1;
	vrZoneD1.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth));
	vrZoneD1 = vrZoneD1.to_world_of(vrPlacement1);

	Vector2 elevatorVRDir = vrDir0;
	Vector2 elevatorVRLoc = vrLoc0;

	// mix, 0, 1, mix
	// if mix fits, great, if not we should try with just plain doors. one of them should be good enough
	for_count(int, approach, 4)
	{
		// elevatorVRDir
		elevatorVRDir = -(vrDir0 + vrDir1).normal();
		if (approach == 1) elevatorVRDir = -vrDir0;
		if (approach == 2) elevatorVRDir = -vrDir1;
		if (approach == 3) elevatorVRDir = elevatorVRDir.axis_aligned();

		// find furthest point
		Vector2 furthestPoint = vrDoorCorners[0];
		int furthestDoorIdx = 0;
		float furthestDist = Vector2::dot(elevatorVRDir, vrDoorCorners[0]);
		for_range(int, i, 1, 3)
		{
			float dist = Vector2::dot(elevatorVRDir, vrDoorCorners[i]);
			if (furthestDist < dist)
			{
				furthestDist = dist;
				furthestPoint = vrDoorCorners[i];
				furthestDoorIdx = i <= 1 ? 0 : 1;
			}
		}

		elevatorVRLoc = (vrLoc0 + vrLoc1) * 0.5f;
		if (approach == 1) elevatorVRLoc = vrLoc0;
		if (approach == 2) elevatorVRLoc = vrLoc1;

		// safe is further
		Vector2 safeElevatorVRLoc = elevatorVRLoc;

		SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("initial vr loc"));

		// move it so all door corners are behind us
		for_range(int, i, 0, 3)
		{
			float dist = Vector2::dot(elevatorVRDir, vrDoorCorners[i] - elevatorVRLoc);
			if (dist > 0.0f)
			{
				elevatorVRLoc += elevatorVRDir * dist; // push it further
				safeElevatorVRLoc = elevatorVRLoc + elevatorVRDir * 0.1f;
			}
		}

		SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("moved beyond corners"));

		// this is a must, otherwise we may end up with portals cutting through each other
		elevatorVRLoc += elevatorVRDir * (cartPointThickness + 0.005f);
		safeElevatorVRLoc += elevatorVRDir * (cartPointThickness + 0.05f);

		Vector2 originalElevatorVRLoc = elevatorVRLoc;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		Vector2 intersectionDebug = elevatorVRLoc;
#endif
		{
			Vector2 intersection;
			if (roomGenerationInfo.get_play_area_zone().calc_intersection(elevatorVRLoc, elevatorVRDir, intersection, 0.0f, halfDoorWidth * 0.8f))
			{
				intersection -= elevatorVRDir * doorWidth;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				intersectionDebug = intersection;
#endif
				float normalToIntersection = Vector2::dot(elevatorVRDir, intersection - elevatorVRLoc);
				float normalToSafe = Vector2::dot(elevatorVRDir, safeElevatorVRLoc - elevatorVRLoc);
				elevatorVRLoc = elevatorVRLoc + elevatorVRDir * max(0.01f, min(normalToIntersection, normalToSafe)); // closest

				SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("after intersection with room"));
			}
		}

		// push it right towards the edge, only if dirs are aligned and doors are further away
		if (Vector2::dot(vrDir0, vrDir1) > 0.99f && Vector2::distance(vrLoc0, vrLoc1) > halfDoorWidth)
		{
			Vector2 intersection;
			if (roomGenerationInfo.get_play_area_zone().calc_intersection(elevatorVRLoc, elevatorVRDir, intersection, -0.05f))
			{
				// has to move at least cartPointThickness to allow cart point (plus extra 0.01 to have doors separated)
				float minMove = cartPointThickness + 0.01f;
				float maxMove = max(minMove, (intersection - elevatorVRDir * doorWidth - elevatorVRLoc).length());
				Vector2 newElevatorVRLoc = elevatorVRLoc + elevatorVRDir * rg.get_float(minMove, maxMove);
				// if we get a bigger floor, allow for decentralised doors
				if ((elevatorVRLoc - newElevatorVRLoc).length() >= doorWidth)
				{
					int triesLeft = 50;
					float sideMoveCoef = 0.5f; // start centralised
					while (triesLeft--)
					{
						float sideMove = 2.0f * sideMoveCoef;
						// move perpendiculary
						newElevatorVRLoc += elevatorVRDir.rotated_right() * (rg.get_float(-1.0f, 1.0f) * sideMove);
						// check if fits
						VR::Zone elevatorZone;
						elevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth));
						elevatorZone.place_at(newElevatorVRLoc, elevatorVRDir);
						if (roomGenerationInfo.get_play_area_zone().does_contain(elevatorZone, 0.0f)) // be very strict here!
						{
							elevatorVRLoc = newElevatorVRLoc;
							SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("decentralised doors"));
							break;
						}
						sideMoveCoef = min(1.0f, sideMoveCoef + 0.05f);
					}
				}
				else
				{
					elevatorVRLoc = newElevatorVRLoc;
					SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("new vr loc"));
				}
			}
		}

		{	// make elevator wider
			float distToCloser = Vector2::dot(elevatorVRLoc - (furthestDoorIdx == 0 ? vrLoc0 : vrLoc1), elevatorVRDir);
			float distToFurther = Vector2::dot(elevatorVRLoc - (furthestDoorIdx == 0 ? vrLoc1 : vrLoc0), elevatorVRDir);
			bool centreElevator = false;
			if (distToFurther < doorWidth * rg.get_float(0.7f, 1.2f))
			{
				// make it as wide to hold both doors
				Vector2 elevatorRight = elevatorVRDir.rotated_right();
				elevatorWidth = abs(Vector2::dot(elevatorRight, vrLoc1 - vrLoc0)) + halfDoorWidth * 2.0f;
				elevatorVRLoc += elevatorRight * Vector2::dot(elevatorRight, (vrLoc1 + vrLoc0) * 0.5f - elevatorVRLoc);
				centreElevator = true;
				SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("hold both doors"));
			}
			else if (distToCloser > doorWidth * 0.8f)
			{
				centreElevator = rg.get_chance(0.5f);
			}
			else
			{
				// centre at closer
				Vector2 elevatorRight = elevatorVRDir.rotated_right();
				elevatorVRLoc += elevatorRight * Vector2::dot(elevatorRight, (furthestDoorIdx == 0 ? vrLoc0 : vrLoc1) - elevatorVRLoc);
				SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("centre at closer"));
			}
			if (centreElevator)
			{
				Vector2 elevatorRight = elevatorVRDir.rotated_right();
				Vector2 intersectRight, intersectLeft;
				if (roomGenerationInfo.get_play_area_zone().calc_intersection(elevatorVRLoc, elevatorRight, intersectRight) &&
					roomGenerationInfo.get_play_area_zone().calc_intersection(elevatorVRLoc, -elevatorRight, intersectLeft))
				{
					Range range(Vector2::dot(intersectRight - elevatorVRLoc, elevatorRight) - elevatorWidth * 0.5f,
						Vector2::dot(intersectLeft - elevatorVRLoc, elevatorRight) + elevatorWidth * 0.5f);
					elevatorVRLoc += elevatorRight * range.clamp(0.0f);
					SHOW_OUTPUT_GENERATION_DEBUG_VISUALISER_DETAILED(TXT("centred"));
				}
			}
		}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		{
			VR::Zone elevatorZone;
			elevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth));
			elevatorZone.place_at(elevatorVRLoc, elevatorVRDir);
			VR::Zone prevElevatorZone;
			prevElevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth));
			prevElevatorZone.place_at(originalElevatorVRLoc, elevatorVRDir);
			VR::Zone safeElevatorZone;
			safeElevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth));
			safeElevatorZone.place_at(safeElevatorVRLoc, elevatorVRDir);

			dv->start_gathering_data();
			dv->clear();

			roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::c64LightBlue);

			prevElevatorZone.debug_visualise(dv.get(), Colour::grey);
			elevatorZone.debug_visualise(dv.get(), Colour::magenta);
			safeElevatorZone.debug_visualise(dv.get(), Colour::green);
			Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d0, vrPlacement0.get_translation().to_vector2(), vrPlacement0.get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::red);
			Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d1, vrPlacement1.get_translation().to_vector2(), vrPlacement1.get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::blue);
			dv->add_circle(intersectionDebug, 0.05f, Colour::red);

			dv->end_gathering_data();
			dv->show_and_wait_for_key();
		}
#endif

		bool isOk = true;
		{	// check if elevator would fit
			VR::Zone elevatorZone;
			elevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth));
			elevatorZone.place_at(elevatorVRLoc, elevatorVRDir);
			if (!roomGenerationInfo.get_play_area_zone().does_contain(elevatorZone, -0.05f))
			{
				isOk = false;
				if (approach != 3)
				{
					// won't move it to furthest door
					// although it may make no sense this way
					elevatorVRLoc = originalElevatorVRLoc;
					elevatorWidth = doorWidth;
				}
			}
		}

		if (isOk)
		{
			break;
		}
	}

	{
		// update elevatorLength
		Vector2 intersection;
		if (roomGenerationInfo.get_play_area_zone().calc_intersection(elevatorVRLoc, elevatorVRDir, intersection, 0.0f, (elevatorWidth * 0.5f) * 0.85f))
		{
			float maxElevatorLength = Vector2::dot(elevatorVRDir, intersection - elevatorVRLoc);
			if (maxElevatorLength < elevatorLength)
			{
				// shorter elevator
				elevatorLength = min(elevatorLength, max(0.55f, max(elevatorLength * 0.8f, maxElevatorLength)));
			}
			else
			{
				// longer elevator
				elevatorLength = rg.get_float(elevatorLength, min(elevatorLength * 1.5f, maxElevatorLength));
			}
		}

		// keep it within defined limits
		elevatorLength = min(elevatorLength, limitElevatorLength.get(elevatorLength));
	}

	// elevator dir is in opposite dir to door

	// elevator stop is not at the centre of the elevator but at the door - where it should be
	Transform elevatorStopVRPlacement = look_matrix(elevatorVRLoc.to_vector3(), elevatorVRDir.to_vector3(), Vector3::zAxis).to_transform();

	// we want to freely move between these
	Transform elevatorStop0 = elevatorStopVRPlacement;
	Transform elevatorStop1 = elevatorStopVRPlacement;

	Transform door0VRPlacement = vrPlacement0;
	Transform door1VRPlacement = vrPlacement1;

	Range useFloorDistance = Range(3.0f, 6.0f);
	if (info->floorDistance.is_set())
	{
		useFloorDistance = info->floorDistance.get(roomVariables, useFloorDistance, AllowToFail);
	}
	// we may require larger distance
	useFloorDistance *= Framework::Door::get_nominal_door_height_scale();

	// this is local to first stop/placement
	Transform elevatorTravel(Vector3::zAxis * (rg.get_chance(0.5f) ? -1.0f : 1.0f) * rg.get_float(useFloorDistance), Quat::identity);
	{
		if (info->type == SimpleElevatorInfo::ElevatorType::RotateXY)
		{
			// we move by 90 degree, hardcoded for now
			float dist = elevatorTravel.get_translation().length();
			// whole = 2 pi r
			// 1/4 whole = 1/2 pi r
			// dist = 1/2 pi r
			// r = dist * 2 / pi
			float radius = dist * 2.0f / pi<float>();
			Vector3 inDir = rg.get_bool() ? Vector3::xAxis : -Vector3::xAxis;

			elevatorTravel = look_matrix((inDir + Vector3::yAxis) * radius, -inDir, Vector3::zAxis).to_transform();
		}
		else
		{
			if (info->type == SimpleElevatorInfo::ElevatorType::Horizontal)
			{
				Vector3 fo = elevatorTravel.get_translation();
				swap(fo.x, fo.z);
				elevatorTravel.set_translation(fo);
			}
		}

		if (info->type == SimpleElevatorInfo::ElevatorType::Vertical)
		{
			// correct traveling dir and set values for doors if required
			float altDir = elevatorTravel.get_translation().z;

			auto* r0 = ! d0->get_door()->is_world_separator_door()? d0->get_room_on_other_side() : nullptr;
			auto* r1 = ! d1->get_door()->is_world_separator_door()? d1->get_room_on_other_side() : nullptr;
			if ((r0 || d0->get_door()->get_vr_elevator_altitude().is_set()) &&
				(r1 || d1->get_door()->get_vr_elevator_altitude().is_set()))
			{
				int vrElevatorAltitudeDepth0 = 0;
				int vrElevatorAltitudeDepth1 = 0;
				Optional<float> vrElevatorAltitude0 = r0? r0->find_vr_elevator_altitude(_room, d0->get_door(), &vrElevatorAltitudeDepth0) : d0->get_door()->get_vr_elevator_altitude().get();
				Optional<float> vrElevatorAltitude1 = r1? r1->find_vr_elevator_altitude(_room, d1->get_door(), &vrElevatorAltitudeDepth1) : d1->get_door()->get_vr_elevator_altitude().get();

				if (vrElevatorAltitude0.is_set() &&
					vrElevatorAltitude1.is_set())
				{
					if (vrElevatorAltitude0.get() != vrElevatorAltitude1.get())
					{
						altDir = abs(altDir) * (vrElevatorAltitude1.get() > vrElevatorAltitude0.get() ? 1.0f : -1.0f);
					}
					else
					{
						// move by byDistance, just to decide direction
						float const byDistance = abs(altDir * 0.25f);
						float centre = vrElevatorAltitude0.get();
						vrElevatorAltitude0 = centre - sign(altDir) * byDistance;
						vrElevatorAltitude1 = centre + sign(altDir) * byDistance;
					}

					// alter actual altitudes (although this might be quite uncommon scenario due to vr corridors left and right)
					if (r0 && r1 && r0->is_vr_arranged() && !r1->is_vr_arranged())
					{
						vrElevatorAltitude1 = vrElevatorAltitude0.get() + altDir;
					}
					else if (r0 && r1 && r1->is_vr_arranged() && !r0->is_vr_arranged())
					{
						vrElevatorAltitude0 = vrElevatorAltitude1.get() - altDir;
					}
					else
					{
						// alter actual altitudes, depending where we're closer
						if (vrElevatorAltitudeDepth0 <= vrElevatorAltitudeDepth1)
						{
							vrElevatorAltitude1 = vrElevatorAltitude0.get() + altDir;
						}
						else
						{
							vrElevatorAltitude0 = vrElevatorAltitude1.get() - altDir;
						}
					}
				}
				else
				{
					// we're at the same level, make random dir count
					if (vrElevatorAltitude0.is_set())
					{
						vrElevatorAltitude1 = vrElevatorAltitude0.get() + altDir;
					}
					else if (vrElevatorAltitude1.is_set())
					{
						vrElevatorAltitude0 = vrElevatorAltitude1.get() - altDir;
					}
				}

				if (vrElevatorAltitude0.is_set() &&
					vrElevatorAltitude1.is_set())
				{
					// force doors at the right altitude
					d0->get_door()->set_vr_elevator_altitude(vrElevatorAltitude0.get());
					d1->get_door()->set_vr_elevator_altitude(vrElevatorAltitude1.get());
					_room->set_vr_elevator_altitude((vrElevatorAltitude0.get() + vrElevatorAltitude1.get()) * 0.5f);
				}
			}

			// apply alt dir
			{
				Vector3 elevatorTravelOffset = elevatorTravel.get_translation();
				elevatorTravelOffset.z = abs(elevatorTravelOffset.z) * sign(altDir);
				elevatorTravel.set_translation(elevatorTravelOffset);
			}
		}

		// offset stop
		elevatorStop1 = elevatorStop0.to_world(elevatorTravel);
	}

	// get elevator edge as is common in this case
	if (info->type == SimpleElevatorInfo::ElevatorType::Vertical ||
		info->type == SimpleElevatorInfo::ElevatorType::Horizontal)
	{
		Transform edgeOffset = Transform(-Vector3::yAxis * cartPointThickness, Quat::identity);
		Vector3 elevatorEdge0 = elevatorStop0.to_world(edgeOffset).get_translation();
		Vector3 elevatorEdge1 = elevatorStop1.to_world(edgeOffset).get_translation();
		Vector3 elevatorEdgeTop = (elevatorEdge0 + elevatorEdge1) * 0.5f;
		elevatorEdgeTop.z = max(elevatorEdge0.z, elevatorEdge1.z);
		elevatorEdgeTop.z += rg.get_float(5.0f, 20.0f); magic_number
		_room->access_variables().access<Vector3>(NAME(elevatorEdgeTop)) = elevatorEdgeTop;
		_room->access_variables().access<float>(NAME(elevatorWidth)) = elevatorWidth;
	}

	// get variables
	{
		SimpleVariableStorage variables;
		_room->collect_variables(variables);
		appearanceSetup.setup_parameters(_room, variables);
	}

	Transform floorSceneryPlacement[2];
	Transform cartPointPlacement[2];
	float cartPointWidth[2];
	Transform elevatorDoorVRPlacement[2];
	Transform elevatorDoorPlacement[2];

	// elevator edge vr
	Vector2 elevatorEdgeVRDir = elevatorVRDir.rotated_right();

	{
		for (int doorIdx = 0; doorIdx < 2; ++doorIdx)
		{
			Vector2 doorVRLoc = (doorIdx == 0 ? door0VRPlacement : door1VRPlacement).get_translation().to_vector2();
			Vector2 doorVRDir = (doorIdx == 0 ? door0VRPlacement : door1VRPlacement).get_axis(Axis::Forward).to_vector2();
			Framework::DoorInRoom *& d = doorIdx == 0 ? d0 : d1;
			float dWidth = d->get_door()->calculate_vr_width();

			cartPointWidth[doorIdx] = dWidth;

			// just hole inside
			float maxOffDist = max(0.0f, (elevatorWidth - dWidth) * 0.5f);
			Vector2 doorEdgeVRStart = elevatorVRLoc - elevatorEdgeVRDir * maxOffDist - elevatorVRDir * cartPointThickness;
			Vector2 doorEdgeVREnd = elevatorVRLoc + elevatorEdgeVRDir * maxOffDist - elevatorVRDir * cartPointThickness;
			Segment segDoorPossible((doorEdgeVRStart).to_vector3(), (doorEdgeVREnd).to_vector3()); // where door can be placed

			Vector2 droppedElevatorDoorVRLoc;
			{
				// drop door onto possible segment
				float alongT = segDoorPossible.get_t(doorVRLoc.to_vector3());
				droppedElevatorDoorVRLoc = segDoorPossible.get_at_t(alongT).to_vector2();
			}

			// check intersection (better to have as straight corridor as possible
			Vector2 elevatorDoorVRLoc = Vector2::zero;
			if (!Vector2::calc_intersection(doorVRLoc, doorVRDir, segDoorPossible.get_start().to_vector2(), segDoorPossible.get_start_to_end_dir().to_vector2(), elevatorDoorVRLoc))
			{
				// drop door onto possible segment
				elevatorDoorVRLoc = droppedElevatorDoorVRLoc;
			}
			else
			{
				// prioritize intersection, but allow dropped to have an influence
				elevatorDoorVRLoc = elevatorDoorVRLoc * 0.7f + 0.3f * droppedElevatorDoorVRLoc;
			}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
			Vector2 tempLoc = elevatorDoorVRLoc;
#endif

			{	// within safe bounds
				float alongT = segDoorPossible.get_t(elevatorDoorVRLoc.to_vector3());
				elevatorDoorVRLoc = segDoorPossible.get_at_t(clamp(alongT, 0.0f, 1.0f)).to_vector2();
			}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
			{
				VR::Zone elevatorZone;
				elevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth));
				elevatorZone.place_at(elevatorVRLoc, elevatorVRDir);

				dv->start_gathering_data();
				dv->clear();

				roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::c64LightBlue);

				elevatorZone.debug_visualise(dv.get(), Colour::magenta);
				Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d0, doorVRLoc, doorVRDir, elevatorLength, Colour::red);

				dv->add_arrow(doorVRLoc, tempLoc, Colour::red);
				dv->add_arrow(tempLoc, elevatorDoorVRLoc, Colour::green);

				dv->end_gathering_data();
				dv->show_and_wait_for_key();
			}
#endif

			Transform elevatorStop = doorIdx == 0 ? elevatorStop0 : elevatorStop1;
			{
				Vector3 floorSceneryVRLoc = (elevatorDoorVRLoc + elevatorVRDir * cartPointThickness).to_vector3();
				floorSceneryPlacement[doorIdx] = elevatorStop.to_world(elevatorStopVRPlacement.to_local(look_at_matrix(floorSceneryVRLoc, floorSceneryVRLoc - elevatorVRDir.to_vector3(), Vector3::zAxis).to_transform()));
			}
			{
				Vector3 cartPointVRLoc = (elevatorDoorVRLoc + elevatorVRDir * cartPointThickness + elevatorVRDir * elevatorLength * 0.5f).to_vector3();
				cartPointPlacement[doorIdx] = elevatorStop.to_world(elevatorStopVRPlacement.to_local(look_at_matrix(cartPointVRLoc, cartPointVRLoc - elevatorVRDir.to_vector3(), Vector3::zAxis).to_transform()));
			}
			{
				elevatorDoorVRPlacement[doorIdx] = look_at_matrix(elevatorDoorVRLoc.to_vector3(), (elevatorDoorVRLoc - elevatorVRDir).to_vector3(), Vector3::zAxis).to_transform();
				elevatorDoorPlacement[doorIdx] = elevatorStop.to_world(elevatorStopVRPlacement.to_local(elevatorDoorVRPlacement[doorIdx]));
			}
#ifdef AN_DEVELOPMENT
			{
				auto _beInZone = roomGenerationInfo.get_play_area_zone();
				if (!_beInZone.does_contain(elevatorDoorVRPlacement[doorIdx].get_translation().to_vector2(), doorWidth * 0.4f))
				{
					DebugVisualiserPtr dv(new DebugVisualiser(_room->get_name()));
					dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
					//if (debugThis)
					{
						dv->activate();
					}
					dv->add_text(Vector2::zero, String(TXT("door outside!")), Colour::red, 0.3f);
					_beInZone.debug_visualise(dv.get(), Colour::black.with_alpha(0.4f));
					Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv,
						d, elevatorDoorVRPlacement[doorIdx].get_translation().to_vector2(), elevatorDoorVRPlacement[doorIdx].get_axis(Axis::Forward).to_vector2(),
						doorWidth, Colour::blue);
					Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv,
						d0, d0->get_vr_space_placement().get_translation().to_vector2(), vrPlacement0.get_axis(Axis::Forward).to_vector2(),
						doorWidth, Colour::red);
					Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv,
						d1, d1->get_vr_space_placement().get_translation().to_vector2(), vrPlacement1.get_axis(Axis::Forward).to_vector2(),
						doorWidth, Colour::red);
					dv->end_gathering_data();
					dv->show_and_wait_for_key();

					an_assert(_beInZone.does_contain(elevatorDoorVRPlacement[doorIdx].get_translation().to_vector2(), doorWidth * 0.4f));
				}
			}
#endif
		}
	}

	// place them where they should be, we expect them to be
	d0->set_placement(elevatorStop0.to_world(elevatorStopVRPlacement.to_local(door0VRPlacement)));
	d1->set_placement(elevatorStop1.to_world(elevatorStopVRPlacement.to_local(door1VRPlacement)));

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	{
		VR::Zone elevatorZone;
		elevatorZone.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, halfDoorWidth));
		elevatorZone.place_at(elevatorVRLoc, elevatorVRDir);

		dv->start_gathering_data();
		dv->clear();
		
		roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::c64LightBlue);

		elevatorZone.debug_visualise(dv.get(), Colour::magenta);
		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d0, door0VRPlacement.get_translation().to_vector2(), door0VRPlacement.get_axis(Axis::Forward).to_vector2(), elevatorLength, Colour::red);
		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d1, door1VRPlacement.get_translation().to_vector2(), door1VRPlacement.get_axis(Axis::Forward).to_vector2(), elevatorLength, Colour::blue);

		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d0, elevatorDoorVRPlacement[0].get_translation().to_vector2(), elevatorDoorVRPlacement[0].get_axis(Axis::Forward).to_vector2(), elevatorLength, Colour::red.with_alpha(0.5f));
		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d1, elevatorDoorVRPlacement[1].get_translation().to_vector2(), elevatorDoorVRPlacement[1].get_axis(Axis::Forward).to_vector2(), elevatorLength, Colour::blue.with_alpha(0.5f));

		dv->add_text(Vector2::zero, String(TXT("vr setup")), Colour::green.with_alpha(0.2f), 0.1f);
		dv->end_gathering_data();
		dv->show_and_wait_for_key();
	}
#endif

	// add rooms when doors are not placed on each other
	for (int doorIdx = 0; doorIdx < 2; ++doorIdx)
	{
		Transform & elevatorDoorVR = elevatorDoorVRPlacement[doorIdx];
		Transform & elevatorDoor = elevatorDoorPlacement[doorIdx];
		Transform & doorVR = doorIdx == 0 ? door0VRPlacement : door1VRPlacement;

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		output(TXT("check doors"));
#endif

		// if they do not match, add room
		if (!Framework::DoorInRoom::same_with_orientation_for_vr(elevatorDoorVR, doorVR))
		{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
			output(TXT("different orientation"));
#endif
			String newRoomName = String::printf(TXT("%S (entrance %i)"), _room->get_name().to_char(), doorIdx);
			Framework::DoorInRoom *& d = doorIdx == 0 ? d0 : d1;
			auto* elevatorDoorType = info->find<Framework::DoorType>(roomVariables, info->elevatorDoorType);
			auto* floorRoomType = info->find<Framework::RoomType>(roomVariables, info->floorRoomType);
			if (!floorRoomType)
			{
				error(TXT("lack of floor room type!"));
				return false;
			}
			if (d->get_door()->is_important_vr_door())
			{
				d = d->grow_into_room(elevatorDoor, elevatorDoorVR, floorRoomType, elevatorDoorType, newRoomName.to_char());
			}
			else if (d->has_room_on_other_side())
			{
				if (Framework::DoorInRoom * newD = d->get_door()->change_into_room(floorRoomType, _room, elevatorDoorType, newRoomName.to_char()))
				{
					an_assert(d == newD);

					/*
					 *			|										|
					 *			|		  _room = elevator room			|
					 *			|										|
					 *			|			    d & newD				|
					 *			+-----------------DOOR------------------+
					 *			|	   d->get_door_on_other_side()		|
					 *			|										|
					 *			|		new room that was created		|
					 *			|		 with "change_into_room"		|
					 *			|										|
					 *			|			    otherDoor				|		<- will have same placement and vr placement as "d" had
					 *			+-----------------DOOR------------------+
					 *			|  otherDoor->get_door_on_other_side()  |
					 *			|										|
					 *			|	    actual floor behind door		|
					 *			|										|
					 *
					 */
					Framework::DoorInRoom* otherDoor = nullptr;
					// get other door to have d's placement
					// we will update d's placement and on-the-other-side-door's
					for_every_ptr(door, d->get_room_on_other_side()->get_all_doors())
					{
						if (door != d->get_door_on_other_side())
						{
							door->set_vr_space_placement(d->get_vr_space_placement());
							door->set_placement(d->get_placement());
							if (d->is_vr_placement_immovable())
							{
								// pass immovability onto that door
								door->make_vr_placement_immovable();
								d->make_vr_placement_movable();
							}
#ifdef AN_DEVELOPMENT
							{
								auto _beInZone = roomGenerationInfo.get_play_area_zone();
								if (!_beInZone.does_contain(door->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f) ||
									!_beInZone.does_contain(d->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f))
								{
									DebugVisualiserPtr dv(new DebugVisualiser(_room->get_name()));
									dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
									//if (debugThis)
									{
										dv->activate();
									}
									dv->add_text(Vector2::zero, String(TXT("door outside!")), Colour::red, 0.3f);
									Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, _beInZone,
										door, door->get_vr_space_placement().get_translation().to_vector2(), door->get_vr_space_placement().get_axis(Axis::Forward).to_vector2(),
										d, d->get_vr_space_placement().get_translation().to_vector2(), d->get_vr_space_placement().get_axis(Axis::Forward).to_vector2(),
										doorWidth);
									dv->end_gathering_data();
									dv->show_and_wait_for_key();

									an_assert(_beInZone.does_contain(door->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f));
									an_assert(_beInZone.does_contain(d->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f));
								}
							}
#endif
							otherDoor = door;
						}
					}
					an_assert(otherDoor);

					/*
					{
						// is it really required?
						Vector2 doorRelativeSize = Vector2::one;
						doorRelativeSize.x = elevatorWidth / d->get_door()->calculate_vr_width();
						d->get_door()->set_relative_size(doorRelativeSize);
					}
					*/

					// set new vr space placement
					an_assert(!d->is_vr_placement_immovable());
					d->set_vr_space_placement(elevatorDoorVRPlacement[doorIdx]); // d is not immovable! can't be as we made sure about that above
					d->set_placement(elevatorDoorPlacement[doorIdx]);
					d->get_door_on_other_side()->update_vr_placement_from_door_on_other_side();
					d->get_door_on_other_side()->make_vr_placement_immovable();
#ifdef AN_DEVELOPMENT
					{
						auto _beInZone = roomGenerationInfo.get_play_area_zone();
						if (!_beInZone.does_contain(d->get_door_on_other_side()->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f) ||
							!_beInZone.does_contain(d->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f))
						{
							DebugVisualiserPtr dv(new DebugVisualiser(_room->get_name()));
							dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
							//if (debugThis)
							{
								dv->activate();
							}
							dv->add_text(Vector2::zero, String(TXT("door outside!")), Colour::red, 0.3f);
							Framework::DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, _beInZone,
								d->get_door_on_other_side(), d->get_door_on_other_side()->get_vr_space_placement().get_translation().to_vector2(), d->get_door_on_other_side()->get_vr_space_placement().get_axis(Axis::Forward).to_vector2(),
								d, d->get_vr_space_placement().get_translation().to_vector2(), d->get_vr_space_placement().get_axis(Axis::Forward).to_vector2(),
								doorWidth);
							dv->end_gathering_data();
							dv->show_and_wait_for_key();

							an_assert(_beInZone.does_contain(d->get_door_on_other_side()->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f));
							an_assert(_beInZone.does_contain(d->get_vr_space_placement().get_translation().to_vector2(), doorWidth * 0.4f));
						}
					}
#endif
					// update placement of door on other side of "d"
					Transform otherDoor_dOtherSide = otherDoor->get_vr_space_placement().to_local(d->get_door_on_other_side()->get_vr_space_placement());
					d->get_door_on_other_side()->set_placement(otherDoor->get_placement().to_world(otherDoor_dOtherSide));
								
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
					{
						output(TXT("between room (vr)"));
						dv->start_gathering_data();
						dv->clear();

						roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::c64LightBlue);

						VR::Zone door1Zone;
						for_every_ptr(door, d->get_room_on_other_side()->get_all_doors())
						{
							Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, door, door->get_vr_space_placement().get_translation().to_vector2(), door->get_vr_space_placement().get_axis(Axis::Forward).to_vector2(), elevatorLength, door == d->get_door_on_other_side()? Colour::red : Colour::blue);
						}

						dv->add_text(Vector2::zero, String(TXT("between room (vr)")), Colour::green.with_alpha(0.2f), 0.1f);
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
					}
					{
						output(TXT("between room"));
						dv->start_gathering_data();
						dv->clear();

						roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::c64LightBlue);

						VR::Zone door1Zone;
						for_every_ptr(door, d->get_room_on_other_side()->get_all_doors())
						{
							Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, door, door->get_placement().get_translation().to_vector2(), door->get_placement().get_axis(Axis::Forward).to_vector2(), elevatorLength, door == d->get_door_on_other_side()? Colour::red : Colour::blue);
						}

						dv->add_text(Vector2::zero, String(TXT("between room")), Colour::blue.with_alpha(0.2f), 0.1f);
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
					}
#endif
				}
			}
			else
			{
				error(TXT("can't create a floor"));
			}
		}
	}

	// elevator placements
	Transform elevatorPlacement0 = elevatorStop0.to_world(elevatorStopVRPlacement.to_local(look_at_matrix((elevatorVRLoc + elevatorVRDir * elevatorLength * 0.5f).to_vector3(), elevatorVRLoc.to_vector3(), Vector3::zAxis).to_transform()));
	Transform elevatorPlacement1 = elevatorStop1.to_world(elevatorStopVRPlacement.to_local(look_at_matrix((elevatorVRLoc + elevatorVRDir * elevatorLength * 0.5f).to_vector3(), elevatorVRLoc.to_vector3(), Vector3::zAxis).to_transform()));

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	{
		VR::Zone elevatorStop0;
		elevatorStop0.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, 0.0f));
		elevatorStop0.place_at(elevatorPlacement0.get_translation().to_vector2(), elevatorPlacement0.get_axis(Axis::Forward).to_vector2());

		VR::Zone elevatorStop1;
		elevatorStop1.be_rect(Vector2(elevatorWidth, elevatorLength), Vector2(0.0f, 0.0f));
		elevatorStop1.place_at(elevatorPlacement1.get_translation().to_vector2(), elevatorPlacement1.get_axis(Axis::Forward).to_vector2());

		dv->start_gathering_data();
		dv->clear();
		roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::c64LightBlue);
		elevatorStop0.debug_visualise(dv.get(), Colour::magenta);
		elevatorStop1.debug_visualise(dv.get(), Colour::magenta);
		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d0, d0->get_placement().get_translation().to_vector2(), d0->get_placement().get_axis(Axis::Forward).to_vector2(), elevatorLength, Colour::red);
		Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, d1, d1->get_placement().get_translation().to_vector2(), d1->get_placement().get_axis(Axis::Forward).to_vector2(), elevatorLength, Colour::blue);
		dv->add_text(Vector2::zero, String(TXT("actual placement")), Colour::yellow.with_alpha(0.2f), 0.1f);
		dv->end_gathering_data();
		dv->show_and_wait_for_key();
	}
#endif

	// add actual floors - just empty actors with nav node to connect door to platform
	Array<Framework::Scenery*> floorSceneries;
	Framework::SceneryType * floorSceneryType = nullptr;
	{
		Framework::LibraryName navNodeSceneryName = Framework::Library::get_current()->get_global_references().get_name(NAME(navNodeScenery));
		floorSceneryType = Framework::Library::get_current()->get_scenery_types().find(navNodeSceneryName);
	}
	if (floorSceneryType)
	{
		floorSceneryType->load_on_demand_if_required();

		for_count(int, floorIdx, 2)
		{
			Framework::Scenery* floorScenery = nullptr;
			_game->perform_sync_world_job(TXT("spawn platform"), [&floorScenery, floorSceneryType, _room, floorIdx]()
			{
				floorScenery = new Framework::Scenery(floorSceneryType, String::printf(TXT("floor %i"), floorIdx));
				floorScenery->init(_room->get_in_sub_world());
			});

			floorScenery->access_individual_random_generator() = rg.spawn();
			_room->collect_variables(floorScenery->access_variables());
			info->apply_generation_parameters_to(floorScenery->access_variables());
#ifdef OUTPUT_GENERATION_VARIABLES
			LogInfoContext log;
			log.log(TXT("create scenery using:"));
			{
				LOG_INDENT(log);
				floorScenery->access_variables().log(log);
			}
			log.output_to_output();
#endif
			floorScenery->initialise_modules();
			Transform floorPlacement = floorSceneryPlacement[floorIdx];
			_game->perform_sync_world_job(TXT("place floor"), [&floorScenery, floorPlacement, _room]()
			{
				floorScenery->get_presence()->place_in_room(_room, floorPlacement);
			});
			_game->on_newly_created_object(floorScenery);

			floorSceneries.push_back(floorScenery);
		}
	}
	else
	{
		error(TXT("no floor scenery type"));
		return false;
	}

	// add cart points
	if (auto * cartPointSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartPointSceneryType))
	{
		cartPointSceneryType->load_on_demand_if_required();

		for_count(int, floorIdx, 2)
		{
			Framework::Scenery* cartPointScenery = nullptr;
			_game->perform_sync_world_job(TXT("spawn cart point"), [&cartPointScenery, cartPointSceneryType, _room]()
			{
				cartPointScenery = new Framework::Scenery(cartPointSceneryType, String::empty());
				cartPointScenery->init(_room->get_in_sub_world());
			});
			appearanceSetup.setup_variables(_room, cartPointScenery);
			cartPointScenery->access_individual_random_generator() = rg.spawn();
			cartPointScenery->access_variables().access<float>(NAME(cartWidth)) = cartPointWidth[floorIdx];
			cartPointScenery->access_variables().access<float>(NAME(cartLength)) = elevatorLength;
			_room->collect_variables(cartPointScenery->access_variables());
			info->apply_generation_parameters_to(cartPointScenery->access_variables());
			cartPointScenery->initialise_modules();
			auto floorScenery = floorSceneries[floorIdx];
			auto useCartPointPlacement = cartPointPlacement[floorIdx];
			_game->perform_sync_world_job(TXT("place cart point"), [floorScenery, &cartPointScenery, _room, useCartPointPlacement]()
			{
				cartPointScenery->get_presence()->place_in_room(_room, useCartPointPlacement);
				cartPointScenery->get_presence()->force_base_on(floorScenery);
			});
			_game->on_newly_created_object(cartPointScenery);
		}
	}

	// add cart
	if (auto * cartSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartSceneryType))
	{
		cartSceneryType->load_on_demand_if_required();

		Framework::Scenery* cartScenery = nullptr;
		_game->perform_sync_world_job(TXT("spawn cart"), [&cartScenery, cartSceneryType, _room]()
		{
			cartScenery = new Framework::Scenery(cartSceneryType, String::empty());
			cartScenery->init(_room->get_in_sub_world());
		});
		Vector3 laneVector = elevatorTravel.get_translation();
		appearanceSetup.setup_variables(_room, cartScenery);
		cartScenery->access_individual_random_generator() = rg.spawn();
		cartScenery->access_variables().access<float>(NAME(cartWidth)) = elevatorWidth;
		cartScenery->access_variables().access<float>(NAME(cartLength)) = elevatorLength;
		cartScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopA)) = floorSceneries[0];
		cartScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopB)) = floorSceneries[1];
		cartScenery->access_variables().access<Transform>(NAME(elevatorStopACartPoint)) = floorSceneries[0]->get_presence()->get_placement().to_local(elevatorPlacement0);
		cartScenery->access_variables().access<Transform>(NAME(elevatorStopBCartPoint)) = floorSceneries[1]->get_presence()->get_placement().to_local(elevatorPlacement1);
		cartScenery->access_variables().access<Vector3>(NAME(laneVector)) = laneVector;
		_room->collect_variables(cartScenery->access_variables());
		info->apply_generation_parameters_to(cartScenery->access_variables());
		cartScenery->initialise_modules();
		_game->perform_sync_world_job(TXT("place cart"), [&cartScenery, _room]()
		{
			cartScenery->get_presence()->place_in_room(_room, Transform::identity); // dirty solution but still, will be replaced with movement
		});
		_game->on_newly_created_object(cartScenery);
	}

	// add cart lane
	if (auto * cartLaneSceneryType = info->find<Framework::SceneryType>(roomVariables, info->cartLaneSceneryType))
	{
		cartLaneSceneryType->load_on_demand_if_required();

		Framework::Scenery* cartLaneScenery = nullptr;
		Vector3 laneVector = elevatorTravel.get_translation();
		Transform placementA = elevatorPlacement0;
		Transform placementB = elevatorPlacement1;
		int floorIdx = 0;
		// lane goes up and to the right but it is placed where elevator is placed, which is opposite to elevatorTravel which is placed where stops are placed
		// hence reversing laneVector.x
		laneVector.x = -laneVector.x;
		if (laneVector.z < 0.0f ||
			laneVector.x < 0.0f)
		{
			laneVector = -laneVector;
			swap(placementA, placementB);
			floorIdx = 1;
		}
		_game->perform_sync_world_job(TXT("spawn cart lane"), [&cartLaneScenery, cartLaneSceneryType, _room]()
		{
			cartLaneScenery = new Framework::Scenery(cartLaneSceneryType, String::empty());
			cartLaneScenery->init(_room->get_in_sub_world());
		});
		an_assert((info->type == SimpleElevatorInfo::ElevatorType::Vertical && laneVector.z > 0.0f) ||
				  (info->type == SimpleElevatorInfo::ElevatorType::Horizontal && laneVector.x > 0.0f) ||
				  (info->type != SimpleElevatorInfo::ElevatorType::Vertical && info->type != SimpleElevatorInfo::ElevatorType::Horizontal));
		appearanceSetup.setup_variables(_room, cartLaneScenery);
		cartLaneScenery->access_individual_random_generator() = rg.spawn();
		cartLaneScenery->access_variables().access<float>(NAME(cartWidth)) = elevatorWidth;
		cartLaneScenery->access_variables().access<float>(NAME(cartLength)) = elevatorLength;
		cartLaneScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopA)) = floorSceneries[0];
		cartLaneScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopB)) = floorSceneries[1];
		cartLaneScenery->access_variables().access<Transform>(NAME(elevatorStopACartPoint)) = placementA;
		cartLaneScenery->access_variables().access<Transform>(NAME(elevatorStopBCartPoint)) = placementB;
		cartLaneScenery->access_variables().access<Vector3>(NAME(laneVector)) = laneVector;
		_room->collect_variables(cartLaneScenery->access_variables());
		info->apply_generation_parameters_to(cartLaneScenery->access_variables());
		cartLaneScenery->initialise_modules();
		_game->perform_sync_world_job(TXT("place cart lane"), [placementA, &cartLaneScenery, _room]()
		{
			cartLaneScenery->get_presence()->place_in_room(_room, placementA);
		});
		_game->on_newly_created_object(cartLaneScenery);
	}

	for_count(int, i, 2)
	{
		if (Framework::DoorInRoom* d = _room->get_all_doors()[i])
		{
			d->access_tags().set_tag(NAME(vrElevatorDoor));
		}
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated simple elevator"));
#endif

	if (!_subGenerator)
	{
		_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	// otherwise first try to use existing vr door placement - if it is impossible, try dropping vr placement on more and more pieces
	return true; // always succeeds
}

