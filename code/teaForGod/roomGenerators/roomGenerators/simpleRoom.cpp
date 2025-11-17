#include "simpleRoom.h"

#include "..\roomGenerationInfo.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\containers\arrayStack.h"
#include "..\..\..\core\debug\debugVisualiser.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\environment.h"
#include "..\..\..\framework\world\environmentInfo.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;
using namespace RoomGenerators;

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_DETAILED_FAILS
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
//#define OUTPUT_GENERATION_DEBUG_VISUALISER_SUCCESS_ONLY
//#define OUTPUT_GENERATION_DEBUG_VISUALISER_FAILS_ONLY
//#define OUTPUT_GENERATION_DEBUG_VISUALISER_FAIL_ELEVATORS
//#define OUTPUT_GENERATION_VARIABLES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

DEFINE_STATIC_NAME(compactPlacement);
DEFINE_STATIC_NAME(compactPlacementChance);
DEFINE_STATIC_NAME(compactPlacementSmall);
DEFINE_STATIC_NAME(compactPlacementSmallChance);
DEFINE_STATIC_NAME(minCompactPlacementTileDistance);
DEFINE_STATIC_NAME(maxCompactPlacementTileDistance);
DEFINE_STATIC_NAME(avoidElevators);
DEFINE_STATIC_NAME(maxAxisDealignmentAngle); // try to use it carefully with compact distance and without alignToTiles
DEFINE_STATIC_NAME(maxAxisDealignmentAngleBig);
DEFINE_STATIC_NAME(alignToTiles);
DEFINE_STATIC_NAME(allowBigRoomChance);
DEFINE_STATIC_NAME(maxSize);
DEFINE_STATIC_NAME(maxTileSize);
DEFINE_STATIC_NAME(maxSizePct);
DEFINE_STATIC_NAME(bigRoomMaxSize);
DEFINE_STATIC_NAME(bigRoomMaxTileSize);
DEFINE_STATIC_NAME(bigRoomMaxSizePct);

DEFINE_STATIC_NAME(maxPlayAreaSize);
DEFINE_STATIC_NAME(maxPlayAreaTileSize);

// this is actual path it follows
DEFINE_STATIC_NAME(roomCorner0);
DEFINE_STATIC_NAME(roomCorner1);
DEFINE_STATIC_NAME(roomCorner2);
DEFINE_STATIC_NAME(roomCorner3);

//

REGISTER_FOR_FAST_CAST(SimpleRoomInfo);

SimpleRoomInfo::SimpleRoomInfo()
{
	set_room_generator_priority(-100);
}

SimpleRoomInfo::~SimpleRoomInfo()
{
}

bool SimpleRoomInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= roomMeshGenerator.load_from_xml(_node, TXT("roomMeshGenerator"), _lc);

	forceMeshSizeToPlayAreaSize = _node->get_bool_attribute_or_from_child_presence(TXT("forceMeshSizeToPlayAreaSize"), forceMeshSizeToPlayAreaSize);

	return result;
}

bool SimpleRoomInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= roomMeshGenerator.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

bool SimpleRoomInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	_renamer.apply_to(roomMeshGenerator);

	return result;
}

Framework::RoomGeneratorInfoPtr SimpleRoomInfo::create_copy() const
{
	SimpleRoomInfo * copy = new SimpleRoomInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool SimpleRoomInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	SimpleRoom sr(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("simple room \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= sr.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

SimpleRoom::SimpleRoom(SimpleRoomInfo const * _info)
: info(_info)
{
}

SimpleRoom::~SimpleRoom()
{
}

struct DoorAtEdge
{
	Framework::DoorInRoom* dir;
	Transform placement; // both normal and vr placement
	float at;
};

struct DoorPlaced
: public DoorAtEdge
{
	DirFourClockwise::Type atEdge;
	DoorPlaced() {}
	DoorPlaced(DoorAtEdge const & dae) : DoorAtEdge(dae) {}

	static int compare(void const* _a, void const* _b)
	{
		DoorPlaced const & a = *plain_cast<DoorPlaced>(_a);
		DoorPlaced const & b = *plain_cast<DoorPlaced>(_b);
		if (a.atEdge < b.atEdge) return A_BEFORE_B;
		if (a.atEdge > b.atEdge) return B_BEFORE_A;
		if (a.at < b.at) return A_BEFORE_B;
		if (a.at > b.at) return B_BEFORE_A;
		return A_AS_B;
	}
};

static void fit_sides(int _fit, Range & _side, float _size, float _firstTileOffset, float _tileSize, float _playAreaSize, Random::Generator & _rg, bool _alignToTiles)
{
	float purePlayAreaSize = _playAreaSize;

	// for _alignToTiles do calculations in 0 -> tileCount * tileSize (rounded playAreaSize) and then adjust by first tile offset
	if (_alignToTiles)
	{
		_playAreaSize = min(_playAreaSize, round_to(_playAreaSize, _tileSize));
	}

	if (_fit == 1)
	{
		_side.min = _rg.get_chance(0.5f) ? 0.0f : _playAreaSize - _size;
	}
	else if (_fit == 2)
	{
		_side.min = _rg.get_float(_tileSize, _playAreaSize - _size - _tileSize);
	}
	else
	{
		_side.min = _rg.get_float(0.0f, _playAreaSize - _size);
	}

	if (_alignToTiles)
	{
		_side.min = round_to(_side.min, _tileSize);
	}
	_side.max = _side.min + _size;

	if (_alignToTiles)
	{
		_side.min += _firstTileOffset;
		_side.max += _firstTileOffset;
	}

	if (_alignToTiles && _side.max > purePlayAreaSize + _tileSize * 0.1f)
	{
		_side.min -= _tileSize;
		_side.max -= _tileSize;
	}
	// ignore alignToTiles, we can't be outside!
	if (_side.min < -_tileSize * 0.1f)
	{
		float moveBy = -_side.min;
		_side.min += moveBy;
		_side.max += moveBy;
		if (_side.max > purePlayAreaSize + _tileSize * 0.1f)
		{
			_side.max = purePlayAreaSize;
		}
	}
}

#define YP 0
#define XP 1
#define YM 2
#define XM 3

bool SimpleRoom::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;
	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();
	Random::Generator randomGenerator = _room->get_individual_random_generator();
	randomGenerator.advance_seed(978645, 689254);

#ifdef OUTPUT_GENERATION
	output(TXT("generating simple room %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif

	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);
	info->apply_generation_parameters_to(variables);

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("simple room"))));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::blue));
	dv->activate();
#endif
	
	_room->mark_check_if_doors_are_too_close(true);

	bool useExistingDoors = false;
	if (_room->get_all_doors().get_size() == 2) // only 2 doors case
	{
#ifdef OUTPUT_GENERATION
		output(TXT("check 2 doors case"));
#endif
		int doorWithVRSet = 0;
		int doorWithVRImmovable = 0;
		for_every_ptr(dir, _room->get_all_doors())
		{
#ifdef OUTPUT_GENERATION
			if (dir->get_door()->is_important_vr_door())
			{
				output(TXT(" door %i is important vr door"), for_everys_index(dir));
			}
#endif
			if (dir->is_vr_space_placement_set())
			{
#ifdef OUTPUT_GENERATION
				output(TXT(" door %i has vr space placement set"), for_everys_index(dir));
#endif
				doorWithVRSet++;
			}
			if (dir->is_vr_placement_immovable())
			{
#ifdef OUTPUT_GENERATION
				output(TXT(" door %i is immovable"), for_everys_index(dir));
#endif
				doorWithVRImmovable++;
			}
		}
		if (doorWithVRImmovable > 0 &&
			doorWithVRSet == _room->get_all_doors().get_size())
		{
			useExistingDoors = true;
		}

		if (useExistingDoors)
		{
			auto const& doors = _room->get_all_doors();
			Transform const& firstDoorPlacement = doors.get_first()->get_vr_space_placement();
			Transform const& firstDoorVRSpacePlacement = doors.get_first()->get_vr_space_placement();
			ArrayStatic<Transform, 2> doorPlacements; SET_EXTRA_DEBUG_INFO(doorPlacements, TXT("SimpleRoom.doorPlacements"));
			doorPlacements.set_size(doors.get_size());
			for_count(int, i, doors.get_size())
			{
				Transform doorVRPlacement = doors[i]->get_vr_space_placement();
				doorPlacements[i] = firstDoorPlacement.to_world(firstDoorVRSpacePlacement.to_local(doorVRPlacement));
			}
			Vector2 d0fwd = doorPlacements[0].get_axis(Axis::Y).to_vector2();
			Vector2 d1fwd = doorPlacements[1].get_axis(Axis::Y).to_vector2();
			if (Vector2::dot(d0fwd, d1fwd) >= -0.7f)
			{
				// not facing each other enough
				useExistingDoors = false;
			}
			if (Vector2::dot(d0fwd, (doorPlacements[1].get_translation() - doorPlacements[0].get_translation()).to_vector2().normal()) > -0.1f)
			{
				// behind or almost behind
				useExistingDoors = false;
			}
			if (Vector2::dot(d0fwd, d1fwd) < -0.7f)
			{
				if (auto* d = doors[0]->get_door())
				{
					float doorWidth = doors[0]->get_door()->calculate_vr_width();
					Vector2 d0rel1 = doorPlacements[0].location_to_local(doorPlacements[1].get_translation()).to_vector2();
					if (abs(d0rel1).x > doorWidth * 0.25f && -d0rel1.y < doorWidth * 0.9f)
					{
						// facing each other but moved to sides too much
						useExistingDoors = false;
					}
				}
			}
		}
	}

	if (useExistingDoors)
	{
#ifdef OUTPUT_GENERATION
		output(TXT("use existing doors for simple room %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif
		// doors are already placed
		an_assert(_room->get_all_doors().get_size() == 2, TXT("for time being we assume we only have two doors for this case!"));

		auto const & doors = _room->get_all_doors();

		// we assume first door placement to be at vr space placement and place other doors accordingly
		Transform const & firstDoorPlacement = doors.get_first()->get_vr_space_placement();
		Transform const & firstDoorVRSpacePlacement = doors.get_first()->get_vr_space_placement();

		for_every_ptr(door, doors)
		{
			Transform doorVRPlacement = door->get_vr_space_placement();
			door->set_placement(firstDoorPlacement.to_world(firstDoorVRSpacePlacement.to_local(doorVRPlacement)));
		}

		Vector2 roomCorners[4]; // all room corners

		float offset = -info->get_wall_offset(); // small offset with doors in mind

		an_assert(Vector2::dot(doors[0]->get_placement().get_axis(Axis::Y).to_vector2(),
							   doors[1]->get_placement().get_axis(Axis::Y).to_vector2()) < -0.7f, TXT("both doors should face each other more or less (diff %.3f')"),
							   abs(Rotator3::get_yaw(doors[0]->get_placement().get_axis(Axis::Y)) - 
							       Rotator3::get_yaw(doors[1]->get_placement().get_axis(Axis::Y))));

		// because there are two doors, their placement is outbound, putting corners on left then on right should give us nice quad
		for_every_ptr(door, doors)
		{
			float doorWidth = door->get_door()->calculate_vr_width() * 1.2f;
			roomCorners[for_everys_index(door) * 2 + 0] = door->get_placement().location_to_world(Vector3(-doorWidth * 0.5f, offset, 0.0f)).to_vector2();
			roomCorners[for_everys_index(door) * 2 + 1] = door->get_placement().location_to_world(Vector3( doorWidth * 0.5f, offset, 0.0f)).to_vector2();
		}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		dv->start_gathering_data();
		dv->clear();

		roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::black.with_alpha(0.25f));
#endif

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		{	// show dealigned
			Colour cColour = Colour::lerp(0.5f,Colour::green, Colour::grey);
			dv->add_line(roomCorners[0], roomCorners[1], cColour);
			dv->add_line(roomCorners[1], roomCorners[2], cColour);
			dv->add_line(roomCorners[2], roomCorners[3], cColour);
			dv->add_line(roomCorners[3], roomCorners[0], cColour);
		}
#endif


		// side walls, for each door's side wall check how big it should be to contain both doors
		{
			Vector2 const playAreaSize = roomGenerationInfo.get_play_area_rect_size();
			Vector2 const halfPlayAreaSize = playAreaSize * 0.5f + Vector2::one * (magic_number 0.02f);
			for (int doorIdx = 0; doorIdx < 2; ++doorIdx)
			{
				Transform const& doorPlacement = doors[doorIdx]->get_placement();
				Vector2 doorAxis = doorPlacement.get_axis(Axis::Y).to_vector2();
				Vector2 sideAxis = doorAxis.rotated_right();
				Vector2 anchor = roomCorners[0];
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				{
					float doorWidth = doors[doorIdx]->get_door()->calculate_vr_width() * 1.2f;
					Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, doors[doorIdx], doorPlacement.get_translation().to_vector2(), doorPlacement.get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::red);
				}
#endif
				Range sideCoverage = Range::empty;
				float sidePos[4];
				for (int i = 0; i < 4; ++i)
				{
					Vector2 rc = roomCorners[i];
					sidePos[i] = Vector2::dot(rc - anchor, sideAxis);
					sideCoverage.include(sidePos[i]);
				}
				if (doorIdx == 0)
				{
					roomCorners[0] += sideAxis * (sideCoverage.min - sidePos[0]);
					roomCorners[1] += sideAxis * (sideCoverage.max - sidePos[1]);
				}
				if (doorIdx == 1)
				{
					roomCorners[2] += sideAxis * (sideCoverage.min - sidePos[2]);
					roomCorners[3] += sideAxis * (sideCoverage.max - sidePos[3]);
				}
				// can be outside play area, force to be within
				for (int rci = doorIdx * 2; rci < doorIdx * 2 + 2; ++rci)
				{
					Vector2& rc = roomCorners[rci];
					Vector2 toDoor = sideAxis * sign(Vector2::dot(sideAxis, doorPlacement.get_translation().to_vector2() - rc));
					while (true)
					{
						if (rc.x > -halfPlayAreaSize.x &&
							rc.x <  halfPlayAreaSize.x &&
							rc.y > -halfPlayAreaSize.y &&
							rc.y <  halfPlayAreaSize.y)
						{
							// is within
							break;
						}
						rc += toDoor * 0.01f; // move 1cm
					}
				}
			}
		}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		{	// show dealigned
			Colour cColour = Colour::green;
			dv->add_line(roomCorners[0], roomCorners[1], cColour);
			dv->add_line(roomCorners[1], roomCorners[2], cColour);
			dv->add_line(roomCorners[2], roomCorners[3], cColour);
			dv->add_line(roomCorners[3], roomCorners[0], cColour);
		}
#endif

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
		dv->end_gathering_data();
		dv->show_and_wait_for_key();
#endif

		variables.access<Vector3>(NAME(roomCorner0)) = roomCorners[0].to_vector3();
		variables.access<Vector3>(NAME(roomCorner1)) = roomCorners[1].to_vector3();
		variables.access<Vector3>(NAME(roomCorner2)) = roomCorners[2].to_vector3();
		variables.access<Vector3>(NAME(roomCorner3)) = roomCorners[3].to_vector3();
	}
	else
	{
#ifdef OUTPUT_GENERATION
		output(TXT("place new doors"));
#endif
		// place doors on rectangle - check for corridor corners to be able toS fit 
		// check if it is small and if we have 4 doors - this is hardcoded solution
		bool compactPlacement = variables.get_value(NAME(compactPlacement), false);
		float compactPlacementChance = variables.get_value(NAME(compactPlacementChance), 0.0f);
		float minCompactPlacementTileDistance = variables.get_value(NAME(minCompactPlacementTileDistance), -1.0f);
		float maxCompactPlacementTileDistance = variables.get_value(NAME(maxCompactPlacementTileDistance), 0.0f);
		bool alignToTiles = variables.get_value(NAME(alignToTiles), roomGenerationInfo.is_small()); // for small room always align to tiles (unless provided as "false", it gives more freedom
		float maxAxisDealignmentAngle = variables.get_value(NAME(maxAxisDealignmentAngle), 0.0f);
		float allowBigRoomChance = variables.get_value(NAME(allowBigRoomChance), 0.1f);
		Vector2 maxSize = variables.get_value(NAME(maxSize), Vector2::zero);
		Vector2 maxTileSize = variables.get_value(NAME(maxTileSize), Vector2::zero);
		Vector2 maxSizePct = variables.get_value(NAME(maxSizePct), Vector2::zero);
		Vector2 bigRoomMaxSize = variables.get_value(NAME(bigRoomMaxSize), Vector2::zero);
		Vector2 bigRoomMaxTileSize = variables.get_value(NAME(bigRoomMaxTileSize), Vector2::zero);
		Vector2 bigRoomMaxSizePct = variables.get_value(NAME(bigRoomMaxSizePct), Vector2::zero);
		Vector2 maxPlayAreaSize = variables.get_value(NAME(maxPlayAreaSize), Vector2::zero);
		Vector2 maxPlayAreaTileSize = variables.get_value(NAME(maxPlayAreaTileSize), Vector2::zero);
		bool avoidElevators = variables.get_value(NAME(avoidElevators), false);

		if (roomGenerationInfo.is_small())
		{
			compactPlacement = variables.get_value(NAME(compactPlacementSmall), compactPlacement);
			compactPlacementChance = variables.get_value(NAME(compactPlacementSmallChance), compactPlacementChance);
		}
		if (roomGenerationInfo.is_big())
		{
			maxAxisDealignmentAngle = variables.get_value(NAME(maxAxisDealignmentAngleBig), maxAxisDealignmentAngle);
		}
		maxAxisDealignmentAngle = 0.0f; // for time being this causes problems !@#

		float const tileSize = roomGenerationInfo.get_tile_size();
		float const halfTileSize = tileSize * 0.5f;

		// auto params
		float minCompactPlacementDistance = minCompactPlacementTileDistance * tileSize;
		float maxCompactPlacementDistance = maxCompactPlacementTileDistance * tileSize;

		{
			Vector2 tempMaxSize = Vector2(99999.0f, 99999.0f);
			if (!maxPlayAreaTileSize.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, max(1.0f, maxPlayAreaTileSize.x) * tileSize);
				tempMaxSize.y = min(tempMaxSize.y, max(1.0f, maxPlayAreaTileSize.y) * tileSize);
			}
			if (!maxPlayAreaSize.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, maxPlayAreaSize.x);
				tempMaxSize.y = min(tempMaxSize.y, maxPlayAreaSize.y);
			}
			maxPlayAreaSize = tempMaxSize;
		}

		if (!maxPlayAreaSize.is_zero())
		{
			roomGenerationInfo.limit_to(maxPlayAreaSize, NP, randomGenerator);
		}

		{
			Vector2 tempMaxSize = Vector2(99999.0f, 99999.0f);
			if (!maxTileSize.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, max(1.0f, maxTileSize.x) * tileSize);
				tempMaxSize.y = min(tempMaxSize.y, max(1.0f, maxTileSize.y) * tileSize);
			}
			if (!maxSizePct.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, maxSizePct.x * roomGenerationInfo.get_play_area_rect_size().x);
				tempMaxSize.y = min(tempMaxSize.y, maxSizePct.y * roomGenerationInfo.get_play_area_rect_size().y);
			}
			if (!maxSize.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, maxSize.x);
				tempMaxSize.y = min(tempMaxSize.y, maxSize.y);
			}
			maxSize = tempMaxSize;
		}

		{
			Vector2 tempMaxSize = Vector2(99999.0f, 99999.0f);
			if (!bigRoomMaxTileSize.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, max(1.0f, bigRoomMaxTileSize.x) * tileSize);
				tempMaxSize.y = min(tempMaxSize.y, max(1.0f, bigRoomMaxTileSize.y) * tileSize);
			}
			if (!bigRoomMaxSizePct.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, bigRoomMaxSizePct.x * roomGenerationInfo.get_play_area_rect_size().x);
				tempMaxSize.y = min(tempMaxSize.y, bigRoomMaxSizePct.y * roomGenerationInfo.get_play_area_rect_size().y);
			}
			if (!bigRoomMaxSize.is_zero())
			{
				tempMaxSize.x = min(bigRoomMaxSize.x, maxSize.x);
				tempMaxSize.y = min(bigRoomMaxSize.y, maxSize.y);
			}
			bigRoomMaxSize = tempMaxSize;
		}

		if (!variables.get_existing<bool>(NAME(compactPlacement)) &&
			!variables.get_existing<float>(NAME(compactPlacementChance)))
		{
			// by default on in most cases, will try to make it as compact as possible without lengthy corridors
			compactPlacementChance = 0.9f;
		}

		if (roomGenerationInfo.is_small())
		{
			compactPlacementChance = 1.2f - (1.0f - compactPlacementChance) * 0.5f;
		}

		compactPlacement |= randomGenerator.get_chance(compactPlacementChance);

		for_every_ptr(door, _room->get_all_doors())
		{
			if (door->get_door()->is_important_vr_door())
			{
				// for vr important always prefer compact placement
				compactPlacement = true;
			}
		}

		bool ok = false;
		bool allowBigRoom = randomGenerator.get_chance(allowBigRoomChance);

		// allocate here as allocating during each iteration will mess up the stack
		ARRAY_PREALLOC_SIZE(int, doorIndices, _room->get_all_doors().get_size());
		ARRAY_PREALLOC_SIZE(DoorPlaced, allDoors, _room->get_all_doors().get_size());
		ARRAY_PREALLOC_SIZE(Framework::DoorInRoom*, requestedDoorOrder, _room->get_all_doors().get_size());

		ARRAY_PREALLOC_SIZE(DoorAtEdge, yp, _room->get_all_doors().get_size());
		ARRAY_PREALLOC_SIZE(DoorAtEdge, xp, _room->get_all_doors().get_size());
		ARRAY_PREALLOC_SIZE(DoorAtEdge, ym, _room->get_all_doors().get_size());
		ARRAY_PREALLOC_SIZE(DoorAtEdge, xm, _room->get_all_doors().get_size());

		for (int checkBigRoom = allowBigRoom ? 1 : 0; checkBigRoom >= 0 && !ok; --checkBigRoom)
		{
			int triesLeft = 1000;
			while (!ok)
			{
				if (triesLeft <= 0)
				{
					break;
				}
				triesLeft--;

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				dv->start_gathering_data();
				dv->clear();
				dv->add_log(NP, String::printf(TXT("tries left %i"), triesLeft));
#endif

				bool stillOptionsLeft = triesLeft < 800;
				bool lastResort = triesLeft < 200;
				bool alignToTilesNow = alignToTiles || triesLeft < 700;
				bool allowCompactPlacement = triesLeft > 500;
				bool noMinCompactDistance = triesLeft < 300; // maybe compact distance breaks stuff? kick in sooner than prefer right in front
				bool noMaxCompactDistance = triesLeft < 600; // maybe compact distance breaks stuff? kick in sooner than prefer right in front
				bool preferRightInFront = triesLeft > (_room->get_all_doors().get_size() <= 2 ? 900 : 990);
				bool failOnElevators = avoidElevators && triesLeft > 400;
				bool smallRoomFourDoors = (roomGenerationInfo.is_small() && _room->get_all_doors().get_size() >= 4) || triesLeft < 10;

				float threshold = 0.001f;
				Vector2 const playAreaSize = roomGenerationInfo.get_play_area_rect_size();
				Vector2 const firstTileOffset = roomGenerationInfo.get_tiles_zone_offset() - roomGenerationInfo.get_play_area_offset();
				Vector2 rectSize;
				if (smallRoomFourDoors)
				{
					rectSize.x = tileSize;
					rectSize.y = tileSize * 2.0f;
				}
				else
				{
					if (checkBigRoom)
					{
						// create biggest possible room
						rectSize.x = playAreaSize.x;
						rectSize.y = playAreaSize.y;
						if (!bigRoomMaxSize.is_zero())
						{
							rectSize.x = min(rectSize.x, bigRoomMaxSize.x);
							rectSize.y = min(rectSize.y, bigRoomMaxSize.y);
						}
						else
						{
							if (randomGenerator.get_chance(0.1f))
							{
								rectSize.x -= tileSize;
							}
							else
							{
								rectSize.y -= tileSize;
							}
						}

						if (maxAxisDealignmentAngle == 0.0f)
						{
							alignToTilesNow |= randomGenerator.get_chance(0.3f);
						}
					}
					else
					{
						Vector2 const smallerPlayAreaSize(playAreaSize.x - tileSize * (randomGenerator.get_chance(0.5f) ? 1.0f : 0.0f),
							playAreaSize.y - tileSize * (randomGenerator.get_chance(0.5f) ? 1.0f : 0.0f));
						Vector2 const smallestPlayAreaSize(playAreaSize.x - tileSize * 2.0f,
							playAreaSize.y - tileSize * 2.0f);
						Vector2 const & usePlayAreaSize = lastResort ? smallestPlayAreaSize : (stillOptionsLeft ? smallerPlayAreaSize : playAreaSize);
						rectSize.x = randomGenerator.get_float(tileSize, usePlayAreaSize.x);
						rectSize.y = randomGenerator.get_float(tileSize, usePlayAreaSize.y);
					}

					if (!checkBigRoom &&
						!maxSize.is_zero())
					{
						rectSize.x = min(rectSize.x, max(tileSize, maxSize.x));
						rectSize.y = min(rectSize.y, max(tileSize, maxSize.y));
					}

					if (alignToTilesNow)
					{
						rectSize.x = round_to(rectSize.x, tileSize);
						rectSize.y = round_to(rectSize.y, tileSize);
					}
				}

				// sane
				rectSize.x = clamp(rectSize.x, tileSize, playAreaSize.x);
				rectSize.y = clamp(rectSize.y, tileSize, playAreaSize.y);

				int doorsX = (int)floor(rectSize.x / tileSize + 0.001f);
				int doorsY = (int)floor(rectSize.y / tileSize + 0.001f);
				int availableDoors = 0;

				// check how many doors do we have available and if we should fit rectangle to one or the other side
				// 0 randomly, won't matter
				// 1 one side has to fit (ie. on one side there should be place for room behind)
				// 2 two sides have to fit (ie. on both sides there should be place for room behind)
				int fitX = 0;
				int fitY = 0;
				if (smallRoomFourDoors)
				{
					availableDoors += doorsY * 2;
					fitX = 2;
					fitY = 0;
				}
				else
				{
					if (rectSize.x <= playAreaSize.x - tileSize + 0.05f)
					{
						fitX = 1;
						availableDoors += doorsY;
					}
					if (rectSize.x <= playAreaSize.x - tileSize * 2.0f + 0.05f)
					{
						fitX = 2;
						availableDoors += doorsY;
					}
					if (rectSize.y <= playAreaSize.y - tileSize + 0.05f)
					{
						fitY = 1;
						availableDoors += doorsX;
					}
					if (rectSize.y <= playAreaSize.y - tileSize * 2.0f + 0.05f)
					{
						fitY = 2;
						availableDoors += doorsX;
					}
				}

				if (availableDoors < _room->get_all_doors().get_size())
				{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
					dv->end_gathering_data();
#endif
#ifdef OUTPUT_GENERATION_DETAILED_FAILS
					output(TXT("less available doors than doors in the room"));
#endif
					continue;
				}

				// calc rect, we're working in 0->playAreaSize, remember afterwards to move to -playAreaSize*0.5 -> playAreaSize*0.5
				Range2 rect;
				fit_sides(fitX, rect.x, rectSize.x, firstTileOffset.x, tileSize, playAreaSize.x, randomGenerator, alignToTilesNow);
				fit_sides(fitY, rect.y, rectSize.y, firstTileOffset.y, tileSize, playAreaSize.y, randomGenerator, alignToTilesNow);

				int startWithDoorIdx = NONE;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				{
					if (compactPlacement && allowCompactPlacement)
					{
						dv->add_log(TXT("compact placement"));
					}
					else
					{
						dv->add_log(TXT("non compact placement"));
					}
				}
#endif
				float const useEpsilon = EPSILON;
				float const useMargin = MARGIN;

				if (compactPlacement && allowCompactPlacement)
				{
					// do randomisation as it may help when we're stuck trying to avoid elevators or something else and would keep on using same vr placement
					{
						doorIndices.clear(); // reusing
						// prioritise important vr doors
						// first add non important
						for_every_ptr(door, _room->get_all_doors())
						{
							if (!door->get_door()->is_important_vr_door())
							{
								doorIndices.insert_at(randomGenerator.get_int(doorIndices.get_size()), for_everys_index(door));
							}
						}
						// then important at the beginning
						for_every_ptr(door, _room->get_all_doors())
						{
							if (door->get_door()->is_important_vr_door())
							{
								doorIndices.insert_at(0, for_everys_index(door));
							}
						}
					}
					for_every(doorIdx, doorIndices)
					{
						auto const * door = _room->get_all_doors()[*doorIdx];
						if (! door->get_door()->is_important_vr_door())
						{
							if (auto * otherDoor = door->get_door_on_other_side())
							{
								if (otherDoor->is_vr_space_placement_set())
								{
									Transform odVRPlacement = otherDoor->get_vr_space_placement();
									odVRPlacement.set_translation(odVRPlacement.get_translation() + playAreaSize.to_vector3() * 0.5f); // move from (-pas/2 -> pas/2) to (0 -> pas)
									Vector3 odLoc = odVRPlacement.get_translation();
									Vector3 odFwd = odVRPlacement.get_axis(Axis::Y);
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
									{
										Colour dColour = Colour::purple;
										Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, otherDoor, odLoc.to_vector2() - playAreaSize * 0.5f, odVRPlacement.vector_to_world(Vector3::yAxis).to_vector2(), tileSize, dColour.with_alpha(0.3f));
									}
#endif

									Range2 newRect = rect;
									// no spacing sometimes, just because
									float minSpacing = (!noMinCompactDistance && minCompactPlacementDistance >= 0.0)? minCompactPlacementDistance : 0.0f;
									float spacing = noMaxCompactDistance ? 0.0f : (randomGenerator.get_chance(magic_number 0.1f) ? 0.0f : randomGenerator.get_float(Range(minSpacing, min(max(playAreaSize.x, playAreaSize.y), maxCompactPlacementDistance))));
									if (!noMinCompactDistance && minSpacing >= 0.0f)
									{
										spacing = max(spacing, minSpacing);
									}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
									dv->add_log(NP, String::printf(TXT("spacing %.3f"), spacing));
#endif
									bool wallSharingRequired = randomGenerator.get_chance(0.2f);
									// place it in front of the other room
									if (odFwd.y < -0.70f)
									{
										newRect.y.max = clamp(odLoc.y - spacing, rect.y.length(), odLoc.y);
										newRect.y.min += newRect.y.max - rect.y.max;
										wallSharingRequired |= abs(newRect.y.max - odLoc.y) < halfTileSize;
									}
									else if (odFwd.x < -0.70f)
									{
										newRect.x.max = clamp(odLoc.x - spacing, rect.x.length(), odLoc.x);
										newRect.x.min += newRect.x.max - rect.x.max;
										wallSharingRequired |= abs(newRect.x.max - odLoc.x) < halfTileSize;
									}
									else if (odFwd.y > 0.70f)
									{
										newRect.y.min = clamp(odLoc.y + spacing, odLoc.y, playAreaSize.y - rect.y.length());
										newRect.y.max += newRect.y.min - rect.y.min;
										wallSharingRequired |= abs(newRect.y.min - odLoc.y) < halfTileSize;
									}
									else if (odFwd.x > 0.70f)
									{
										newRect.x.min = clamp(odLoc.x + spacing, odLoc.x, playAreaSize.x - rect.x.length());
										newRect.x.max += newRect.x.min - rect.x.min;
										wallSharingRequired |= abs(newRect.x.min - odLoc.x) < halfTileSize;
									}
									else
									{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
										dv->add_log(NP, String::printf(TXT("couldn't place %i in front of other vr"), *doorIdx));
#endif
										continue;
									}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
									dv->add_log(NP, String::printf(TXT("wallSharingRequired %c"), wallSharingRequired ? '+' : '-'));
									dv->add_log(NP, String::printf(TXT("preferRightInFront %c"), preferRightInFront ? '+' : '-'));
#endif
									if (wallSharingRequired
										&& (preferRightInFront || spacing == 0.0f) // after couple of tries loosen it a bit
										//&& randomGenerator.get_chance(0.9f + 0.1f * compactPlacementChance * (roomGenerationInfo.is_small() ? 1.25f : 1.0f))
										) // maybe we don't want to be in front (if chance failed), for small rooms prefer more to be in front
									{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
										dv->add_log(NP, String::printf(TXT("wall sharing %i"), *doorIdx));
#endif
										spacing = 0.0f;
										Vector2 moveBy = Vector2::zero;
										// make sure that they share wall
										if (abs(odFwd.y) > 0.70f)
										{
											if (newRect.x.min > odLoc.x - halfTileSize) moveBy.x += (odLoc.x - halfTileSize) - newRect.x.min;
											if (newRect.x.max < odLoc.x + halfTileSize) moveBy.x += (odLoc.x + halfTileSize) - newRect.x.max;
										}
										if (abs(odFwd.x) > 0.70f)
										{
											if (newRect.y.min > odLoc.y - halfTileSize) moveBy.y += (odLoc.y - halfTileSize) - newRect.y.min;
											if (newRect.y.max < odLoc.y + halfTileSize) moveBy.y += (odLoc.y + halfTileSize) - newRect.y.max;
										}
										// if they're close enough, move them to each other
										if (odFwd.y < -0.70f && newRect.y.max > odLoc.y - halfTileSize) moveBy.y += odLoc.y - newRect.y.max;
										if (odFwd.x < -0.70f && newRect.x.max > odLoc.x - halfTileSize) moveBy.x += odLoc.x - newRect.x.max;
										if (odFwd.y >  0.70f && newRect.y.min < odLoc.y + halfTileSize) moveBy.y += odLoc.y - newRect.y.min;
										if (odFwd.x >  0.70f && newRect.x.min < odLoc.x + halfTileSize) moveBy.x += odLoc.x - newRect.x.min;
										// move new rect
										newRect.x.min += moveBy.x;
										newRect.x.max += moveBy.x;
										newRect.y.min += moveBy.y;
										newRect.y.max += moveBy.y;
									}

									if (alignToTilesNow)
									{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
										dv->add_log(NP, String::printf(TXT("align to tiles %i"), *doorIdx));
#endif
										newRect.x.min -= firstTileOffset.x;
										newRect.y.min -= firstTileOffset.y;
										newRect.x.min = round_to(newRect.x.min, tileSize);
										newRect.y.min = round_to(newRect.y.min, tileSize);
										newRect.x.min += firstTileOffset.x;
										newRect.y.min += firstTileOffset.y;
										newRect.x.max = newRect.x.min + rect.x.length();
										newRect.y.max = newRect.y.min + rect.y.length();
									}

									if (newRect.x.min >= -0.05f && newRect.x.max <= playAreaSize.x + 0.05f &&
										newRect.y.min >= -0.05f && newRect.y.max <= playAreaSize.y + 0.05f)
									{
										if ((odFwd.y < -0.70f && newRect.y.max <= odLoc.y + useEpsilon) ||
											(odFwd.x < -0.70f && newRect.x.max <= odLoc.x + useEpsilon) ||
											(odFwd.y > 0.70f && newRect.y.min >= odLoc.y - useEpsilon) ||
											(odFwd.x > 0.70f && newRect.x.min >= odLoc.x - useEpsilon))
										{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
											Colour dColour = Colour::purple;
											Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, otherDoor, odLoc.to_vector2() - playAreaSize * 0.5f, odVRPlacement.vector_to_world(Vector3::yAxis).to_vector2(), tileSize, dColour);
											dv->add_log(NP, String::printf(TXT("compact placement! %i"), *doorIdx));
#endif

											rect = newRect;
											startWithDoorIdx = *doorIdx;
											break;
										}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
										else
										{
											dv->add_log(NP, String::printf(TXT("couldn't place %i close and in front of other vr"), *doorIdx));
										}
#endif
									}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
									else
									{
										dv->add_log(NP, String::printf(TXT("couldn't fit %i into play area"), *doorIdx));
									}
#endif
								}
							}
						}
					}
				}

				int availableDoorsLeft = availableDoors;
				yp.clear(); int ypLeft = rect.y.max <= playAreaSize.y - tileSize + threshold ? doorsX : 0;
				xp.clear(); int xpLeft = rect.x.max <= playAreaSize.x - tileSize + threshold ? doorsY : 0;
				ym.clear(); int ymLeft = rect.y.min >= tileSize - threshold ? doorsX : 0;
				xm.clear(); int xmLeft = rect.x.min >= tileSize - threshold ? doorsY : 0;
				Array<DoorAtEdge>* doorsAtEdges[] = { &yp, &xp, &ym, &xm };
				int* left[] = { &ypLeft, &xpLeft, &ymLeft, &xmLeft };
				float edges[] = { rectSize.x, rectSize.y, rectSize.x, rectSize.y };
				float edgesStart[] = { rect.x.min, rect.y.min, rect.x.min, rect.y.min };
				ok = true;
				for_count(int, tryAligning, 2)
				{
					ok = true;
					doorIndices.clear(); // reusing
					for_count(int, id, _room->get_all_doors().get_size())
					{
						doorIndices.insert_at(randomGenerator.get_int(doorIndices.get_size()), id);
					}
					if (startWithDoorIdx != NONE)
					{
						doorIndices.remove(startWithDoorIdx);
						doorIndices.insert_at(0, startWithDoorIdx);
					}
					for_every(doorIndex, doorIndices)
					{
						auto * door = _room->get_all_doors()[*doorIndex];
						todo_note(TXT("this assumes doors are not bigger than tile and are symmetrical"));
						int doorLeft = _room->get_all_doors().get_size() - for_everys_index(doorIndex);
						bool alignToTileSize = tryAligning || alignToTilesNow || doorLeft <= availableDoors + 1;
						{
							bool foundPlace = false;
							int findPlaceForDoorTriesLeft = 500;
							int edgeIdx = randomGenerator.get_int(4);
							Optional<float> tryToPlaceWhereOtherDoorIs;
							if (compactPlacement && findPlaceForDoorTriesLeft > 400) // try to face other room or face in same direction
							{
								if (! door->get_door()->is_important_vr_door())
								{
									if (auto * otherDoor = door->get_door_on_other_side())
									{
										if (otherDoor->is_vr_space_placement_set())
										{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
											dv->add_log(NP, String::printf(TXT("try compact for %i"), *doorIndex));
#endif
											Transform odVRPlacement = otherDoor->get_vr_space_placement();
											odVRPlacement.set_translation(odVRPlacement.get_translation() + playAreaSize.to_vector3() * 0.5f); // move from (-pas/2 -> pas/2) to (0 -> pas)
											Vector3 odFwd = odVRPlacement.get_axis(Axis::Y);
											Vector3 odLoc = odVRPlacement.get_translation();
											Vector3 relLoc = odLoc - rect.centre().to_vector3();
											int newEdgeIdx = NONE;
											if (odFwd.y < -0.70f && rect.y.max <= odLoc.y + useEpsilon)
											{
												if (abs(rect.y.max - odLoc.y) < halfTileSize && // close!
													(odLoc.x <= rect.x.min + halfTileSize - useMargin || odLoc.x >= rect.x.max - halfTileSize + useMargin)) // but to the side...
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("won't fit YP, try X"));
#endif
													newEdgeIdx = relLoc.x < 0.0f ? XM : XP;
												}
												else
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("match YP"));
#endif
													newEdgeIdx = YP; tryToPlaceWhereOtherDoorIs = odLoc.y - edgesStart[newEdgeIdx] - halfTileSize;
												}
											}
											else if (odFwd.x < -0.70f && rect.x.max <= odLoc.x + useEpsilon)
											{
												if (abs(rect.x.max - odLoc.x) < halfTileSize && // close!
													(odLoc.y <= rect.y.min + halfTileSize - useMargin || odLoc.y >= rect.y.max - halfTileSize + useMargin)) // but to the side...
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("won't fit XP, try Y"));
#endif
													newEdgeIdx = relLoc.y < 0.0f ? YM : YP;
												}
												else
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("match XP"));
#endif
													newEdgeIdx = XP; tryToPlaceWhereOtherDoorIs = odLoc.x - edgesStart[newEdgeIdx] - halfTileSize;
												}
											}
											else if (odFwd.y > 0.70f && rect.y.min >= odLoc.y - useEpsilon)
											{
												if (abs(rect.y.min - odLoc.y) < halfTileSize && // close!
													(odLoc.x <= rect.x.min + halfTileSize - useMargin || odLoc.x >= rect.x.max - halfTileSize + useMargin)) // but to the side...
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("won't fit YM, try X"));
#endif
													newEdgeIdx = relLoc.x < 0.0f ? XM : XP;
												}
												else
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("match YM"));
#endif
													newEdgeIdx = YM; tryToPlaceWhereOtherDoorIs = odLoc.y - edgesStart[newEdgeIdx] - halfTileSize;
												}
											}
											else if (odFwd.x > 0.70f && rect.x.min >= odLoc.x - useEpsilon)
											{
												if (abs(rect.x.min - odLoc.x) < halfTileSize && // close!
													(odLoc.y <= rect.y.min + halfTileSize - useMargin || odLoc.y >= rect.y.max - halfTileSize + useMargin)) // but to the side...
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("won't fit XM, try Y"));
#endif
													newEdgeIdx = relLoc.y < 0.0f ? YM : YP;
												}
												else
												{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
													dv->add_log(TXT("match YM"));
#endif
													newEdgeIdx = XM; tryToPlaceWhereOtherDoorIs = odLoc.x - edgesStart[newEdgeIdx] - halfTileSize;
												}
											}
											if (newEdgeIdx == NONE &&
												randomGenerator.get_chance(0.9f))
											{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
												dv->add_log(TXT("try to face same dir then (if not axis aligned)"));
#endif
												// try to face same dir then (if not axis aligned)
												if (odFwd.y < -0.70f && abs(relLoc.x) > halfTileSize) { newEdgeIdx = YM; }
												else
												if (odFwd.x < -0.70f && abs(relLoc.y) > halfTileSize) { newEdgeIdx = XM; }
												else
												if (odFwd.y > 0.70f && abs(relLoc.x) > halfTileSize) { newEdgeIdx = YP; }
												else
												if (odFwd.x > 0.70f && abs(relLoc.y) > halfTileSize) { newEdgeIdx = XP; }
											}
											if (newEdgeIdx == NONE &&
												randomGenerator.get_chance(0.6f))
											{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
												dv->add_log(TXT("try to face other door then (if not axis aligned)"));
#endif
												// try to face other door then (if not axis aligned)
												if (relLoc.y >= abs(relLoc.x) && abs(relLoc.x) > halfTileSize) { newEdgeIdx = YP; }
												else
												if (relLoc.x >= abs(relLoc.y) && abs(relLoc.y) > halfTileSize) { newEdgeIdx = XP; }
												else
												if (relLoc.y <= -abs(relLoc.x) && abs(relLoc.x) > halfTileSize) { newEdgeIdx = YM; }
												else
												if (relLoc.x <= -abs(relLoc.y) && abs(relLoc.y) > halfTileSize) { newEdgeIdx = XM; }
											}
											if (newEdgeIdx != NONE)
											{
												edgeIdx = newEdgeIdx;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
												dv->add_log(NP, String::printf(TXT("at edge %i"), edgeIdx));
#endif
											}
										}
									}
								}
							}
							while (findPlaceForDoorTriesLeft--)
							{
								if (*(left[edgeIdx]) <= 0)
								{
									if (randomGenerator.get_chance(0.3f))
									{
										edgeIdx = randomGenerator.get_int(4);
									}
									else
									{
										edgeIdx = (edgeIdx + 1) % 4;
									}
									continue;
								}
								float at = randomGenerator.get_float(0.0f, edges[edgeIdx]);
								if (alignToTileSize)
								{
									at = at - mod(at + SMALL_MARGIN, tileSize);
								}
								at = clamp(at, 0.0f, max(0.0f, edges[edgeIdx] - tileSize));
								bool isOk = true;
								for_every(ed, *(doorsAtEdges[edgeIdx]))
								{
									float const threshold = 0.01f;
									if (at + tileSize - threshold >= ed->at &&
										at <= ed->at + tileSize - threshold)
									{
										isOk = false;
										break;
									}
								}
								if (isOk)
								{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
									dv->add_log(NP, String::printf(TXT("add %i to edge %i"), *doorIndex, edgeIdx));
#endif
									DoorAtEdge dae;
									dae.at = at;
									dae.dir = door;
									doorsAtEdges[edgeIdx]->push_back(dae);
									foundPlace = true;
									--(*(left[edgeIdx]));
									--availableDoorsLeft;
									edgeIdx = randomGenerator.get_int(4); // choose completely new edge
									break;
								}
							}
							if (!foundPlace)
							{
#ifdef OUTPUT_GENERATION_DETAILED_FAILS
								output(TXT("could not find place"));
#endif
								ok = false;
								break;
							}
						}
					}
					if (ok)
					{
						break;
					}
				}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::black.with_alpha(0.25f));
				Vector2 c[4];
				c[0] = Vector2(rect.x.min, rect.y.min);
				c[1] = Vector2(rect.x.min, rect.y.max);
				c[2] = Vector2(rect.x.max, rect.y.max);
				c[3] = Vector2(rect.x.max, rect.y.min);
				for_count(int, ci, 4)
				{
					c[ci] -= playAreaSize * 0.5f;
				}
				Colour cColour = Colour::blue;
				dv->add_line(c[0], c[1], cColour);
				dv->add_line(c[1], c[2], cColour);
				dv->add_line(c[2], c[3], cColour);
				dv->add_line(c[3], c[0], cColour);
#endif

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				bool problem = false;
#endif
				// place all doors
				allDoors.clear();
				if (ok)
				{
					for_count(int, i, 4)
					{
						Vector3 inDir = i == 0 ?  Vector3::yAxis :
									   (i == 1 ?  Vector3::xAxis :
									   (i == 2 ? -Vector3::yAxis :
												 -Vector3::xAxis));
						Transform doorPlacement = look_at_matrix(Vector3::zero, inDir, Vector3::zAxis).to_transform();
						for_every(ed, *(doorsAtEdges[i]))
						{
							Vector3 placement = Vector3::zero;
							float at = ed->at + halfTileSize;
							if (i == 0) placement = Vector3(at + rect.x.min, rect.y.max, 0.0f); else
							if (i == 1) placement = Vector3(rect.x.max, rect.y.max - at, 0.0f); else
							if (i == 2) placement = Vector3(rect.x.max - at, rect.y.min, 0.0f); else
							if (i == 3) placement = Vector3(rect.x.min, rect.y.min + at, 0.0f);
							// offset to proper space
							placement -= playAreaSize.to_vector3() * 0.5f;
							doorPlacement.set_translation(placement);
							ed->placement = doorPlacement;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
							Colour dColour = Colour::red;
							Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, ed->dir, placement.to_vector2(), doorPlacement.vector_to_world(Vector3::yAxis).to_vector2(), tileSize, dColour);
							if (auto* otherDoor = ed->dir->get_door_on_other_side())
							{
								if (otherDoor->is_vr_space_placement_set())
								{
									Colour colour = Colour::purple.mul_rgb(0.5f).with_alpha(0.5f);
									Transform odVRPlacement = otherDoor->get_vr_space_placement();
									odVRPlacement.set_translation(odVRPlacement.get_translation() + playAreaSize.to_vector3() * 0.5f); // move from (-pas/2 -> pas/2) to (0 -> pas)
									Vector3 odLoc = odVRPlacement.get_translation();
									Vector3 odFwd = odVRPlacement.get_axis(Axis::Y);
									Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, otherDoor, odLoc.to_vector2() - playAreaSize * 0.5f, odVRPlacement.vector_to_world(Vector3::yAxis).to_vector2(), tileSize, colour);
									dv->add_line(placement.to_vector2(), otherDoor->get_vr_space_placement().get_translation().to_vector2(), colour);
								}
							}

							if (!roomGenerationInfo.get_play_area_zone().does_contain(placement.to_vector2(), halfTileSize))
							{
								problem = true;
							}
#endif
							an_assert(roomGenerationInfo.get_play_area_zone().does_contain(placement.to_vector2(), halfTileSize));
							DoorPlaced dp(*ed);
							dp.atEdge = (DirFourClockwise::Type)i;
							allDoors.push_back(dp);
						}
					}
				}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				if (problem)
				{
					dv->add_text(Vector2::zero, String(TXT("outside!")), Colour::red, 0.1f);
					dv->end_gathering_data();
#ifndef OUTPUT_GENERATION_DEBUG_VISUALISER_SUCCESS_ONLY
					dv->show_and_wait_for_key();
#endif
					dv->start_gathering_data();
				}
#endif

				if (ok && PilgrimageInstanceOpenWorld::get() && allDoors.get_size() > 2)
				{
					// reorganise doors, so we have them clockwise in right directions
					// this is to make it so that if you're going from "down/south" and want to go "right/east", you have to choose door on your right

					// first just get the tags sorted
					PilgrimageInstanceOpenWorld::get()->tag_open_world_directions_for_cell(_room);

					// make them clockwise (within the room) with vr placed door being first
					{
						sort(allDoors);
						for_count(int, i, allDoors.get_size())
						{
							if (allDoors.get_first().dir->is_vr_space_placement_set())
							{
								break;
							}
							auto pd = allDoors.get_first();
							allDoors.pop_front();
							allDoors.push_back(pd);
						}
					}

					requestedDoorOrder.clear(); // reusing
					for_every(d, allDoors)
					{
						requestedDoorOrder.push_back(d->dir);
					}
					PilgrimageInstanceOpenWorld::get()->get_clockwise_ordered_doors_in(_room, REF_ requestedDoorOrder);

					an_assert(requestedDoorOrder.get_size() == allDoors.get_size());

					// copy order
					if (requestedDoorOrder.get_size() == allDoors.get_size())
					{
						for_every(ad, allDoors)
						{
							ad->dir = requestedDoorOrder[for_everys_index(ad)];
						}
					}
					else
					{
						an_assert(false);
					}
				}

				if (ok && failOnElevators)
				{
					for_every(ed, allDoors)
					{
						if (!ed->dir->get_door()->is_important_vr_door())
						{
							if (auto * otherDoor = ed->dir->get_door_on_other_side())
							{
								if (otherDoor->is_vr_space_placement_set() &&
									Vector3::dot(ed->placement.get_axis(Axis::Forward), otherDoor->get_vr_space_placement().get_axis(Axis::Forward)) > 0.7f) // only if facing same dir
								{
									float doorWidth = ed->dir->get_door()->calculate_vr_width();
									if (doorWidth == 0.0f)
									{
										doorWidth = tileSize;
									}
									float const halfDoorWidth = doorWidth * 0.5f;
									float const makeItNarrower = 0.99f;
									Vector3 halfDoor(halfDoorWidth * makeItNarrower, 0.0f, 0.0f);
									Vector3 inwardsABit(0.0f, halfDoorWidth * 0.02f, 0.0f);
									VR::Zone vrZoneED;
									vrZoneED.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth)); // outbound
									vrZoneED = vrZoneED.to_world_of(ed->placement);
									Segment2 vrSegED(ed->placement.location_to_world(-halfDoor + inwardsABit).to_vector2(),
										ed->placement.location_to_world(halfDoor + inwardsABit).to_vector2());
									VR::Zone vrZoneOD;
									vrZoneOD.be_rect(Vector2(doorWidth, doorWidth), Vector2(0.0f, halfDoorWidth)); // outbound
									vrZoneOD = vrZoneOD.to_world_of(otherDoor->get_vr_space_placement());
									Segment2 vrSegOD(otherDoor->get_vr_space_placement().location_to_world(-halfDoor + inwardsABit).to_vector2(),
										otherDoor->get_vr_space_placement().location_to_world(halfDoor + inwardsABit).to_vector2());
									if (vrZoneED.does_intersect_with(vrSegOD) || vrZoneOD.does_intersect_with(vrSegED)) // one door (as segment) is within zone of other door
									{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER_FAIL_ELEVATORS
										vrZoneED.debug_visualise(dv.get(), Colour::yellow);
										vrZoneOD.debug_visualise(dv.get(), Colour::cyan);
										dv->add_line(vrSegED.get_start(), vrSegED.get_end(), Colour::yellow);
										dv->add_line(vrSegOD.get_start(), vrSegOD.get_end(), Colour::cyan);
										dv->add_line(ed->placement.get_translation().to_vector2(), ed->placement.location_to_world(Vector3::yAxis * halfDoorWidth).to_vector2(), Colour::yellow);
										dv->add_line(otherDoor->get_vr_space_placement().get_translation().to_vector2(), otherDoor->get_vr_space_placement().location_to_world(Vector3::yAxis * halfDoorWidth).to_vector2(), Colour::cyan);
										dv->add_text(Vector2::zero, String(TXT("avoid elevators")), Colour::red, 0.1f);
										dv->end_gathering_data();
										dv->show_and_wait_for_key();
										dv->start_gathering_data();
#endif
#endif
#ifdef OUTPUT_GENERATION_DETAILED_FAILS
										output(TXT("doors intersect"));
#endif
										ok = false;
										break;
									}
								}
							}
						}
					}
				}

				if (!ok)
				{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
					dv->end_gathering_data();
#endif
					continue;
				}

				if (info->forceMeshSizeToPlayAreaSize)
				{
					rect= Range2(Range(0.0f, roomGenerationInfo.get_play_area_rect_size().x),
								 Range(0.0f, roomGenerationInfo.get_play_area_rect_size().y));
				}

				// get points, axis aligned
				ArrayStatic<Vector2, 4> roomCorners; SET_EXTRA_DEBUG_INFO(roomCorners, TXT("SimpleRoom.roomCorners"));
				float offset = -info->get_wall_offset(); // small offset with doors in mind
				roomCorners.push_back(Vector2(rect.x.min - offset, rect.y.min - offset));
				roomCorners.push_back(Vector2(rect.x.min - offset, rect.y.max + offset));
				roomCorners.push_back(Vector2(rect.x.max + offset, rect.y.max + offset));
				roomCorners.push_back(Vector2(rect.x.max + offset, rect.y.min - offset));

				// into room space and calculate centre
				Vector2 centre = Vector2::zero;
				for_every(roomCorner, roomCorners)
				{
					*roomCorner -= playAreaSize * 0.5f;
					centre += *roomCorner;
				}
				centre /= 4.0f;

				if (maxAxisDealignmentAngle != 0.0f)
				{
					float minDistanceToEdge = max(playAreaSize.x, playAreaSize.y);
					minDistanceToEdge = min(rect.x.min, minDistanceToEdge);
					minDistanceToEdge = min(rect.y.min, minDistanceToEdge);
					minDistanceToEdge = min(playAreaSize.x - rect.x.max, minDistanceToEdge);
					minDistanceToEdge = min(playAreaSize.y - rect.y.max, minDistanceToEdge);
					float axisDealignment = randomGenerator.get_float(-maxAxisDealignmentAngle, maxAxisDealignmentAngle);

					Transform dealignTransform = translation_matrix(centre.to_vector3()).to_world(rotation_xy_matrix(axisDealignment).to_world(translation_matrix(-centre.to_vector3()))).to_transform();

					// check if will fit if realigned
					for_every(roomCorner, roomCorners)
					{
						*roomCorner = dealignTransform.location_to_world(roomCorner->to_vector3()).to_vector2();
						if (!roomGenerationInfo.get_play_area_zone().does_contain(*roomCorner, -0.02f))
						{
#ifdef OUTPUT_GENERATION_DETAILED_FAILS
							output(TXT("won't fit if realigned [corners]"));
#endif
							ok = false;
						}
					}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
					{	// show dealigned
						Colour cColour = Colour::green;
						dv->add_line(roomCorners[0], roomCorners[1], cColour);
						dv->add_line(roomCorners[1], roomCorners[2], cColour);
						dv->add_line(roomCorners[2], roomCorners[3], cColour);
						dv->add_line(roomCorners[3], roomCorners[0], cColour);
					}
#endif
					for_every(ed, allDoors)
					{
						ed->placement = dealignTransform.to_world(ed->placement);
						VR::Zone outboundZone;
						outboundZone.be_rect(Vector2(tileSize, tileSize), Vector2(0.0f, halfTileSize));
						outboundZone = outboundZone.to_world_of(ed->placement);
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
						Colour cColour = Colour::green;
#endif
						if (!roomGenerationInfo.get_play_area_zone().does_contain(outboundZone, -0.02f))
						{
#ifdef OUTPUT_GENERATION_DETAILED_FAILS
							output(TXT("won't fit if realigined [doors]"));
#endif
							ok = false;
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
							cColour = Colour::red;
#endif
						}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
						//!@#outboundZone.debug_visualise(dv.get(), cColour);
#endif
					}
				}

				if (!ok)
				{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
					dv->end_gathering_data();
#ifndef OUTPUT_GENERATION_DEBUG_VISUALISER_SUCCESS_ONLY
					dv->show_and_wait_for_key();
#endif
#endif
					continue;
				}

				// ready everything to generate

				// setup mesh generator
				// to proper space
				variables.access<Vector3>(NAME(roomCorner0)) = roomCorners[0].to_vector3();
				variables.access<Vector3>(NAME(roomCorner1)) = roomCorners[1].to_vector3();
				variables.access<Vector3>(NAME(roomCorner2)) = roomCorners[2].to_vector3();
				variables.access<Vector3>(NAME(roomCorner3)) = roomCorners[3].to_vector3();

#ifdef OUTPUT_GENERATION
				output(TXT("place all doors for simple room %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif

				// place all doors
				for_every(ed, allDoors)
				{
#ifdef OUTPUT_GENERATION
					output(TXT("check door %i of simple room %S"), for_everys_index(ed), _room->get_individual_random_generator().get_seed_string().to_char());
					output(TXT("  %S is vr placement set"), ed->dir->is_vr_space_placement_set() ? TXT("yes") : TXT("NO "));
					output(TXT("  %S same placement"), ed->dir->is_vr_space_placement_set() && Framework::DoorInRoom::same_with_orientation_for_vr(ed->dir->get_vr_space_placement(), ed->placement) ? TXT("yes") : TXT("NO "));
#endif
					if (!ed->dir->is_vr_space_placement_set() ||
						!Framework::DoorInRoom::same_with_orientation_for_vr(ed->dir->get_vr_space_placement(), ed->placement))
					{
#ifdef AN_DEVELOPMENT
						bool vrSpacePlacementSet = ed->dir->is_vr_space_placement_set();
						bool samePlacement = vrSpacePlacementSet && Framework::DoorInRoom::same_with_orientation_for_vr(ed->dir->get_vr_space_placement(), ed->placement);
#endif
#ifdef OUTPUT_GENERATION
						output(TXT("  %S immovable"), ed->dir->is_vr_placement_immovable()? TXT("YES") : TXT("no "));
						output(TXT("  %S world separator door"), ed->dir->get_door()->is_important_vr_door()? TXT("YES") : TXT("no "));
#endif
						if (ed->dir->is_vr_placement_immovable() || ed->dir->get_door()->is_important_vr_door())
						{
#ifdef OUTPUT_GENERATION
							output(TXT("grow corridor during generation of simple room %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif
							ed->dir = ed->dir->grow_into_vr_corridor(NP, ed->placement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
#ifdef OUTPUT_GENERATION
							output(TXT("grown corridor during generation of simple room %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif
						}
						DOOR_IN_ROOM_HISTORY(ed->dir, TXT("set vr placement [simple room]"));
						ed->dir->set_vr_space_placement(ed->placement);
					}
					ed->dir->set_placement(ed->placement);
				}

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				dv->end_gathering_data();
#ifndef OUTPUT_GENERATION_DEBUG_VISUALISER_FAILS_ONLY
				output(TXT("working doors"));
				for_every(ed, allDoors)
				{
					output(TXT("%i"), for_everys_index(ed));
					output(TXT("   this   %p"), ed->dir);
					output(TXT("   other  %p"), ed->dir->get_door_on_other_side());
					output(TXT("   door   %p"), ed->dir->get_door());
					if (ed->dir->get_door()->get_linked_door_a() != ed->dir &&
						ed->dir->get_door()->get_linked_door_a() != ed->dir->get_door_on_other_side())
					{
						output_colour(1, 0, 0, 1);
					}
					output(TXT("    dir A %p"), ed->dir->get_door()->get_linked_door_a());
					output_colour();
					if (ed->dir->get_door()->get_linked_door_b() != ed->dir &&
						ed->dir->get_door()->get_linked_door_b() != ed->dir->get_door_on_other_side())
					{
						output_colour(1, 0, 0, 1);
					}
					output(TXT("    dir B %p"), ed->dir->get_door()->get_linked_door_b());
					output_colour();
				}
				dv->show_and_wait_for_key();
#endif
#endif
			}
		}

		if (!ok)
		{
			error(TXT("could not generate simple room"));
			an_assert(false);
			return false;
		}
	}

	{
		if (auto * mg = info->roomMeshGenerator.get())
		{
#ifdef OUTPUT_GENERATION_VARIABLES
			LogInfoContext log;
			log.log(TXT("generating mesh using:"));
			{
				LOG_INDENT(log);
				variables.log(log);
			}
			log.output_to_output();
#endif
			result = true;
			auto generatedMesh = mg->generate_temporary_library_mesh(::Framework::Library::get_current(),
				::Framework::MeshGeneratorRequest()
					.for_wmp_owner(_room)
					.no_lods()
					.using_parameters(variables)
					.using_random_regenerator(_room->get_individual_random_generator())
					.using_mesh_nodes(_roomGeneratingContext.meshNodes));
			if (generatedMesh.get())
			{
				_room->add_mesh(generatedMesh.get());
				Framework::MeshNode::copy_not_dropped_to(generatedMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes);
			}
			else
			{
				error(TXT("could not generate mesh"));
				result = false;
			}
		}
		else
		{
			error(TXT("could not generate simple room mesh"));
			return false;
		}

		/* this is ok, but for future:
		 * do in loops - if fails, drop one vr placement for door, if all dropped, fail - although we should never get to that
		 * all doors should face each other (should be in front of each other). of course being in one line is also acceptable
		 * calculate widths of doors first
		 * check if doors with vr placement set are fine against each other
		 */ 
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated simple room %S \"%S\""), _room->get_individual_random_generator().get_seed_string().to_char(), _room->get_name().to_char());
#endif

	if (!_subGenerator)
	{
		_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	// otherwise first try to use existing vr door placement - if it is impossible, try dropping vr placement on more and more pieces
	return result;
}

#undef YP
#undef XP
#undef YM
#undef XM
