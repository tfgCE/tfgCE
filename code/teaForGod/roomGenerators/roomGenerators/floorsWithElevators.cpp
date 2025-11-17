#include "floorsWithElevators.h"

#include "..\roomGenerationInfo.h"
#include "..\roomGeneratorUtils.h"

#include "..\..\artDir.h"

#include "..\..\game\game.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\debug\debugVisualiserUtils.h"
#include "..\..\..\framework\game\game.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\roomRegionVariables.inl"
#include "..\..\..\framework\world\world.h"

#include "..\..\..\core\debug\debugRenderer.h"

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
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
//#define OUTPUT_GENERATION_VARIABLES
//#define OUTPUT_OPEN_WORLD_BALCONIES
//#define DEBUG_DRAW_BLOCK_SLIDING_LOCOMOTION
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

// variables for mesh generator
DEFINE_STATIC_NAME(floorCount);
DEFINE_STATIC_NAME(floorOuterSize);
DEFINE_STATIC_NAME(floorInnerSize);
DEFINE_STATIC_NAME(doorWidth);
DEFINE_STATIC_NAME(boxSize);
DEFINE_STATIC_NAME(boxAtPt);

// mesh node names
DEFINE_STATIC_NAME(floor);

// mesh node variables
DEFINE_STATIC_NAME(index);

// carts
DEFINE_STATIC_NAME(withSlidingLocomotionCollision);
DEFINE_STATIC_NAME(cartWidth);
DEFINE_STATIC_NAME(cartLength);
DEFINE_STATIC_NAME(elevatorStopA);
DEFINE_STATIC_NAME(elevatorStopB);
DEFINE_STATIC_NAME(elevatorStopACartPoint);
DEFINE_STATIC_NAME(elevatorStopBCartPoint);
DEFINE_STATIC_NAME(laneVector);

//

REGISTER_FOR_FAST_CAST(FloorsWithElevatorsInfo);

FloorsWithElevatorsInfo::FloorsWithElevatorsInfo()
{
}

FloorsWithElevatorsInfo::~FloorsWithElevatorsInfo()
{
}

bool FloorsWithElevatorsInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= mainMeshGenerator.load_from_xml(_node, TXT("mainMeshGenerator"));
	result &= floorSceneryType.load_from_xml(_node, TXT("floorSceneryType"));
	result &= doorMeshGenerator.load_from_xml(_node, TXT("doorMeshGenerator"));

	_lc.load_group_into(mainMeshGenerator);
	_lc.load_group_into(floorSceneryType);
	_lc.load_group_into(doorMeshGenerator);

	result &= cartPointSceneryType.load_from_xml(_node, TXT("cartPointSceneryType"));
	result &= cartSceneryType.load_from_xml(_node, TXT("cartSceneryType"));
	result &= cartLaneSceneryType.load_from_xml(_node, TXT("cartLaneSceneryType"));

	_lc.load_group_into(cartPointSceneryType);
	_lc.load_group_into(cartSceneryType);
	_lc.load_group_into(cartLaneSceneryType);

	result &= blockSlidingLocomotionBoxSceneryType.load_from_xml(_node, TXT("blockSlidingLocomotionBoxSceneryType"));
	
	_lc.load_group_into(blockSlidingLocomotionBoxSceneryType);
	
	result &= maxOuterRadiusAsTiles.load_from_xml(_node, TXT("maxOuterRadiusAsTiles"));

	extraFloors.load_from_xml(_node, TXT("extraFloors"));
	totalFloors.load_from_xml(_node, TXT("totalFloors"));

	for_every(node, _node->children_named(TXT("environmentAnchor")))
	{
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("variableName"), setAnchorVariable);
		setAnchorVariable = node->get_name_attribute_or_from_child(TXT("setVariableName"), setAnchorVariable);
		getAnchorPOI = node->get_name_attribute_or_from_child(TXT("getFromPOI"), getAnchorPOI);
		roomCentrePOI = node->get_name_attribute_or_from_child(TXT("roomCentrePOI"), roomCentrePOI);
	}

	eachDoorAtUniqueFloor = _node->get_bool_attribute_or_from_child_presence(TXT("eachDoorAtUniqueFloor"), eachDoorAtUniqueFloor);

	return result;
}

bool FloorsWithElevatorsInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool FloorsWithElevatorsInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	return result;
}

Framework::RoomGeneratorInfoPtr FloorsWithElevatorsInfo::create_copy() const
{
	FloorsWithElevatorsInfo * copy = new FloorsWithElevatorsInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool FloorsWithElevatorsInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	FloorsWithElevators bs(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("floors with elevators \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= bs.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

FloorsWithElevators::FloorsWithElevators(FloorsWithElevatorsInfo const * _info)
: info(_info)
{
}

FloorsWithElevators::~FloorsWithElevators()
{
}

bool FloorsWithElevators::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	scoped_call_stack_info(TXT("FloorsWithElevators::generate"));

	bool result = true;
	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();
	Random::Generator randomGenerator = _room->get_individual_random_generator();
	randomGenerator.advance_seed(859726, 2975);

#ifdef OUTPUT_GENERATION
	output(TXT("generating floors with elevators %S"), randomGenerator.get_seed_string().to_char());
#endif

	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);

	scoped_call_stack_info(TXT("FloorsWithElevators::generate [1]"));

	elevatorAppearanceSetup.setup_parameters(_room, variables);

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("floors with elevators"))));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::purple));
	dv->activate();
#endif

	// get count basing on doors
	int floorCount = info->eachDoorAtUniqueFloor? _room->get_all_doors().get_size() : _room->get_all_doors().get_size() / 3;

	floorCount += info->extraFloors.get(randomGenerator);
	floorCount = max(floorCount, info->totalFloors.get(randomGenerator));

	{
		int minFloorCount = max(1, TypeConversions::Normal::f_i_cells(((float)_room->get_all_doors().get_size() - 0.5f) / 4.0f));
		floorCount = max(minFloorCount, floorCount);
	}

	Range maxOuterRadiusAsTilesRange = info->maxOuterRadiusAsTiles.get_with_default(_room, Range::empty);
	float maxOuterRadiusAsTiles = (float)min(roomGenerationInfo.get_tile_count().x, roomGenerationInfo.get_tile_count().y);
	if (!maxOuterRadiusAsTilesRange.is_empty())
	{
		maxOuterRadiusAsTiles = min(maxOuterRadiusAsTiles, randomGenerator.get(maxOuterRadiusAsTilesRange));
	}

	float const tileSize = roomGenerationInfo.get_tile_size();

	float floorOuterSize = maxOuterRadiusAsTiles * tileSize;
	float floorInnerSize = floorOuterSize - tileSize * 2.0f;

	scoped_call_stack_info(TXT("FloorsWithElevators::generate [2]"));

	Framework::MeshGenerator* mainMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->mainMeshGenerator.get(variables));
	auto* floorSceneryType = Framework::Library::get_current()->get_scenery_types().find(info->floorSceneryType.get(variables));
	Framework::MeshGenerator* doorMeshGenerator = nullptr;
	if (info->doorMeshGenerator.is_set())
	{
		doorMeshGenerator = Framework::Library::get_current()->get_mesh_generators().find(info->doorMeshGenerator.get(variables));
	}
	if (!mainMeshGenerator)
	{
		error(TXT("no main mesh generator provided"));
		an_assert(false);
		return false;
	}
	if (!floorSceneryType)
	{
		error(TXT("no floor scenery type provided"));
		an_assert(false);
		return false;
	}

	scoped_call_stack_info(TXT("FloorsWithElevators::generate [3]"));

	bool forSlidingLocomotion = Game::is_using_sliding_locomotion();

	{
		Transform mainPlacement = Transform::identity;

		Framework::UsedLibraryStored<Framework::Mesh> mainMesh;
		// generate main mesh with provided balcony count
		if (mainMeshGenerator)
		{
			scoped_call_stack_info(TXT("use mainMeshGenerator"));

			SimpleVariableStorage mainVariables(variables);

			Random::Generator rg(randomGenerator);
			rg.advance_seed(97974, 979265);

			mainPlacement = Transform::identity;

			mainVariables.access<int>(NAME(floorCount)) = floorCount;
			mainVariables.access<float>(NAME(floorOuterSize)) = floorOuterSize;
			mainVariables.access<float>(NAME(floorInnerSize)) = floorInnerSize;

			mainMesh = mainMeshGenerator->generate_temporary_library_mesh(::Framework::Library::get_current(),
				::Framework::MeshGeneratorRequest()
					.for_wmp_owner(_room)
					.no_lods()
					.using_parameters(mainVariables)
					.using_random_regenerator(rg)
					.using_mesh_nodes(_roomGeneratingContext.meshNodes));
			if (mainMesh.get())
			{
				_room->add_mesh(mainMesh.get(), mainPlacement);
				Framework::MeshNode::copy_not_dropped_to(mainMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes, mainPlacement);
			}
			else
			{
				error(TXT("could not generate main mesh"));
				AN_BREAK;
				return false;
			}
		}

		Array<Optional<Transform>> floorPlacement;
		floorPlacement.set_size(floorCount);

		for_every_ref(meshNode, mainMesh->get_mesh_nodes()) // we will be referring to same mesh nodes and when we drop, the nodes in roomGeneratingContext will also be dropped
		{
			if (meshNode->name == NAME(floor))
			{
				if (int const* pIndex = meshNode->variables.get_existing<int>(NAME(index)))
				{
					if (floorPlacement.is_index_valid(*pIndex))
					{
						floorPlacement[*pIndex] = meshNode->placement;
					}
					else
					{
						error(TXT("invalid floor index"));
						result = false;
						AN_BREAK;
					}
				}
				else
				{
					error(TXT("no floor index"));
					result = false;
					AN_BREAK;
				}
			}
		}

		for_every(f, floorPlacement)
		{
			if (!f->is_set())
			{
				error(TXT("not all floors have placement assigned"));
				return false;
			}
		}

		Array<Framework::Scenery*> floorSceneries;

		// generator floors
		{
			scoped_call_stack_info(TXT("generate floors"));
			for_every(f, floorPlacement)
			{
				if (floorSceneryType)
				{
					scoped_call_stack_info_str_printf(TXT("floor %i"), for_everys_index(f));

					floorSceneryType->load_on_demand_if_required();

					Framework::Scenery* floorScenery = nullptr;
					_game->perform_sync_world_job(TXT("spawn floor scenery"), [&floorScenery, floorSceneryType, _room]()
					{
						floorScenery = new Framework::Scenery(floorSceneryType, String::empty());
						floorScenery->init(_room->get_in_sub_world());
					});
					floorScenery->access_individual_random_generator() = randomGenerator.spawn();
					floorScenery->access_variables().access<float>(NAME(floorInnerSize)) = floorInnerSize;
					floorScenery->access_variables().access<float>(NAME(floorOuterSize)) = floorOuterSize;
					_room->collect_variables(floorScenery->access_variables());
					info->apply_generation_parameters_to(floorScenery->access_variables());
					floorScenery->initialise_modules();
					_game->perform_sync_world_job(TXT("place cart point"), [&floorScenery, _room, f]()
					{
						floorScenery->get_presence()->place_in_room(_room, f->get());
					});
					_game->on_newly_created_object(floorScenery);
					floorSceneries.push_back(floorScenery);
				}
			}
		}
		Array<Range3> blocked;

		Array<Framework::DoorInRoom*> doors;
		Array<Transform> doorPlacement;
		Array<Transform> doorVRPlacement;

		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			piow->tag_open_world_directions_for_cell(_room);
		}

		// get placement of doors
		{
			scoped_call_stack_info(TXT("placement of doors"));

			ARRAY_PREALLOC_SIZE(int, floorsLeft, floorCount);
			ARRAY_PREALLOC_SIZE(int, floorsToReadd, floorCount);
			for_count(int, i, floorCount)
			{
				floorsLeft.push_back(i);
			}
			{
				int allDoorsCount = _room->get_all_doors().get_size();
				doors.set_size(allDoorsCount);
				doorPlacement.set_size(allDoorsCount);
				doorVRPlacement.set_size(allDoorsCount);
			}
			
			// placeDoorsIdx:
			//	0 - with dir suggested
			//	1 - others, they may go anywhere
			for_count(int, placeDoorsIdx, 2)
			{
				for_every_ptr(dir, _room->get_all_doors())
				{
					int dirIdx = for_everys_index(dir);
					Optional<DirFourClockwise::Type> dirSuggested;
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						dirSuggested = piow->get_dir_for_door(dir);
						if ((placeDoorsIdx == 0 && !dirSuggested.is_set()) ||
							(placeDoorsIdx == 1 && dirSuggested.is_set()))
						{
							continue;
						}
					}
					bool placed = false;
					int tryNo = 0;
					while (!placed)
					{
						++tryNo;
						int idx = randomGenerator.get_int(floorsLeft.get_size());
						int floorIdx = floorsLeft[idx];

						DirFourClockwise::Type dirWorld = (DirFourClockwise::Type)randomGenerator.get_int(DirFourClockwise::NUM);

						if (dirSuggested.is_set())
						{
							dirWorld = dirSuggested.get();
						}

						DirFourClockwise::Type dirLocal = dirWorld;

						if (auto* piow = PilgrimageInstanceOpenWorld::get())
						{
							dirLocal = piow->dir_to_local(dirWorld, _room);
						}

						// outbound, look at the centre
						Transform floorLocalPlacement = look_at_matrix((Vector3::yAxis * (floorInnerSize * 0.5f + 0.01f /* a bit to avoid cart doors from intersecting */)).rotated_by_yaw(90.0f * (float)dirLocal), Vector3::zero, Vector3::zAxis).to_transform();
						Transform dpvr = rotation_xy_matrix(180.0f).to_transform().to_world(floorLocalPlacement); // this way we will face the door to other cell

						Transform dp = floorPlacement[floorIdx].get().to_world(floorLocalPlacement);

						Range3 block = Range3::empty;
						block.include(dp.location_to_world(Vector3(0.0f, 0.0f, 0.0f)));
						block.include(dp.location_to_world(Vector3(-tileSize * 0.5f, tileSize, 0.0f)));
						block.include(dp.location_to_world(Vector3(tileSize * 0.5f, tileSize, 0.0f)));
						block.include(dp.location_to_world(Vector3(0.0f, 0.0f, 2.0f)));
						block.expand_by(-Vector3::one * 0.1f); // to allow for elevators to be close to the door

						{
							placed = true;
							for_every(b, blocked)
							{
								if (b->overlaps(block))
								{
									placed = false;
									break;
								}
							}
						}

						bool shiftFloors = tryNo > 20;
						if (placed)
						{
							doors[dirIdx] = dir;
							doorPlacement[dirIdx] = dp;
							doorVRPlacement[dirIdx] = dpvr;

							blocked.push_back(block);

							if (info->eachDoorAtUniqueFloor)
							{
								floorsLeft.remove_at(idx);
							}
							else
							{
								shiftFloors = true;
							}
						}

						if (shiftFloors)
						{
							tryNo = 0;
							if (!floorsToReadd.is_empty() && (randomGenerator.get_chance(0.3f) || floorsLeft.get_size() == 1))
							{
								floorsLeft.push_back(floorsToReadd.get_first());
								floorsToReadd.pop_front();
							}
							if (floorsLeft.get_size() > 1)
							{
								floorsLeft.remove_at(idx);
								floorsToReadd.push_back(floorIdx);
							}
						}
					}
				}
			}
		}

		struct ElevatorInfo
		{
			int floor0;
			int floor1;
			Transform placement0;
			Transform placement1;

			static bool is_everything_connected(Array<ElevatorInfo> const& _elevators, int _floorCount, REF_ Array<bool> & connected)
			{
				connected.clear();
				while (connected.get_size() < _floorCount)
				{
					connected.push_back(false);
				}
				connected[max(0, _floorCount / 2 - 1)] = true;
				while (true)
				{
					bool modifiedSomething = false;
					for_every(c, connected)
					{
						if (*c)
						{
							int fIdx = for_everys_index(c);
							for_every(e, _elevators)
							{
								if (e->floor0 == fIdx &&
									!connected[e->floor1])
								{
									connected[e->floor1] = true;
									modifiedSomething = true;
								}
								if (e->floor1 == fIdx &&
									!connected[e->floor0])
								{
									connected[e->floor0] = true;
									modifiedSomething = true;
								}
							}
						}
					}
					if (!modifiedSomething)
					{
						break;
					}
				}

				for_every(c, connected)
				{
					if (!*c)
					{
						return false;
					}
				}
				return true;
			}
		};

		Array<ElevatorInfo> elevators;

		// add elevators
		{
			scoped_call_stack_info(TXT("add elevators"));

			int blockedCheckpoint = blocked.get_size();
			ARRAY_PREALLOC_SIZE(bool, connected, floorCount);
			ARRAY_PREALLOC_SIZE(int, connectedFloors, floorCount);
			ARRAY_PREALLOC_SIZE(int, notConnectedFloors, floorCount);
			bool elevatorsOk = false;
			int const bigTriesCount = 10;
			int bigTriesLeft = bigTriesCount;
			while (bigTriesLeft > 0 && ! elevatorsOk)
			{
				elevators.clear();
				blocked.set_size(blockedCheckpoint);
				--bigTriesLeft;
				bool keepAdding = true;
				int extraToAdd = randomGenerator.get_int_from_range(floorCount * 2 / 3, floorCount * 2);
				int triesLeft = 1000;
				int extraTriesLeft = 100;
				elevatorsOk = false;
				ElevatorInfo::is_everything_connected(elevators, floorCount, REF_ connected); // initial update of connected
				while (keepAdding && triesLeft > 0)
				{
					--triesLeft;
					connectedFloors.clear();
					notConnectedFloors.clear();
					for_every(c, connected)
					{
						if (*c)
						{
							connectedFloors.push_back(for_everys_index(c));
						}
						else
						{
							notConnectedFloors.push_back(for_everys_index(c));
						}
					}

					int tryFloor = connectedFloors[randomGenerator.get_int(connectedFloors.get_size())];

					DirFourClockwise::Type atSide = (DirFourClockwise::Type)randomGenerator.get_int(DirFourClockwise::NUM);
					float offsetLoc = randomGenerator.get_float(-floorInnerSize * 0.5f + tileSize * 0.5f, floorInnerSize * 0.5f - tileSize * 0.5f);
					offsetLoc = round_to(offsetLoc, tileSize);

					bool addingNormal = ! notConnectedFloors.is_empty();
					int tryToFloor = ! notConnectedFloors.is_empty()? notConnectedFloors[randomGenerator.get_int(notConnectedFloors.get_size())]
																	: connectedFloors[randomGenerator.get_int(connectedFloors.get_size())];

					bool isOk = false;

					/*
					if (!addingNormal)
					{
						// avoid elevators going between same floors
						for_every(e, elevators)
						{
							if ((e->floor0 == tryFloor && e->floor1 == tryToFloor) ||
								(e->floor1 == tryFloor && e->floor0 == tryToFloor))
							{
								tryToFloor = tryFloor;
								break;
							}
						}
					}
					*/
					if (tryFloor != tryToFloor)
					{
						Transform floorLocalPlacement = look_at_matrix((Vector3::yAxis * floorInnerSize * 0.5f).rotated_by_yaw(90.0f * (float)atSide), Vector3::zero, Vector3::zAxis).to_transform();
						floorLocalPlacement = floorLocalPlacement.to_world(rotation_xy_matrix(180.0f).to_transform());
						floorLocalPlacement.set_translation(floorLocalPlacement.location_to_world(Vector3::xAxis * offsetLoc - Vector3::yAxis * tileSize * 0.5f));

						Range3 wouldBlock = Range3::empty;
						for_count(int, f, 2)
						{
							int floorIdx = f == 0 ? tryFloor : tryToFloor;
							wouldBlock.include(floorPlacement[floorIdx].get().to_world(floorLocalPlacement).location_to_world(Vector3(0.0f, 0.0f, 0.1f)));
							wouldBlock.include(floorPlacement[floorIdx].get().to_world(floorLocalPlacement).location_to_world(Vector3(-tileSize * 0.5f,  tileSize * 0.5f, 0.0f)));
							wouldBlock.include(floorPlacement[floorIdx].get().to_world(floorLocalPlacement).location_to_world(Vector3( tileSize * 0.5f,  tileSize * 0.5f, 0.0f)));
							wouldBlock.include(floorPlacement[floorIdx].get().to_world(floorLocalPlacement).location_to_world(Vector3(-tileSize * 0.5f, -tileSize * 0.5f, 0.0f)));
							wouldBlock.include(floorPlacement[floorIdx].get().to_world(floorLocalPlacement).location_to_world(Vector3( tileSize * 0.5f, -tileSize * 0.5f, 0.0f)));
							wouldBlock.include(floorPlacement[floorIdx].get().to_world(floorLocalPlacement).location_to_world(Vector3(0.0f, 0.0f, 2.0f)));
						}

						Range3 checkIfClear = wouldBlock;
						if (bigTriesLeft > bigTriesCount / 2)
						{
							// try bigger so they won't touch
							checkIfClear.expand_by(Vector3::one * 0.05f);
						}
						else
						{
							checkIfClear.expand_by(-Vector3::one * 0.05f);
						}

						isOk = true;
						for_every(b, blocked)
						{
							if (b->overlaps(checkIfClear))
							{
								isOk = false;
								break;
							}
						}

						if (isOk)
						{
							ElevatorInfo ei;
							ei.floor0 = tryFloor < tryToFloor ? tryFloor : tryToFloor;
							ei.floor1 = tryFloor < tryToFloor ? tryToFloor : tryFloor;
							ei.placement0 = floorPlacement[ei.floor0].get().to_world(floorLocalPlacement);
							ei.placement1 = floorPlacement[ei.floor1].get().to_world(floorLocalPlacement);
							elevators.push_back(ei);
							blocked.push_back(wouldBlock);
						}
					}

					bool allOk = ElevatorInfo::is_everything_connected(elevators, floorCount, REF_ connected);

					keepAdding = false;
					if (!allOk)
					{
						keepAdding = true;
					}
					else
					{
						elevatorsOk = true;
						if (!addingNormal)
						{
							--extraTriesLeft;
							if (isOk && extraToAdd > 0)
							{
								--extraToAdd;
							}
						}
						keepAdding = extraTriesLeft > 0 && extraToAdd > 0;
					}
				}
			}
			if (!elevatorsOk)
			{
				error(TXT("could not add enough elevators"));
				return false;
			}
		}

		// place doors (and generate door meshes if a generator was provided)
		{
			scoped_call_stack_info(TXT("place doors"));

			for_every_ptr(pdir, doors)
			{
				Transform dp = doorPlacement[for_everys_index(pdir)];
				Transform dpvr = doorVRPlacement[for_everys_index(pdir)];
				auto* dir = pdir;
				if ((!dir->is_vr_space_placement_set() ||
					!Framework::DoorInRoom::same_with_orientation_for_vr(dir->get_vr_space_placement(), dpvr)))
				{
					if (dir->is_vr_placement_immovable())
					{
						dir = dir->grow_into_vr_corridor(NP, dpvr, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
					}
					dir->set_vr_space_placement(dpvr);
				}
				dir->set_placement(dp);
				if (doorMeshGenerator)
				{
					scoped_call_stack_info(TXT("generate mesh"));

					SimpleVariableStorage doorVariables(variables);

					Random::Generator rg(randomGenerator);
					rg.advance_seed(957957 + for_everys_index(pdir) * 972, 7863);

					doorVariables.access<float>(NAME(doorWidth)) = tileSize;

					Framework::UsedLibraryStored<Framework::Mesh> doorMesh;
					doorMesh = doorMeshGenerator->generate_temporary_library_mesh(::Framework::Library::get_current(),
							::Framework::MeshGeneratorRequest()
							.for_wmp_owner(_room)
							.no_lods()
							.using_parameters(doorVariables)
							.using_random_regenerator(rg));
					if (doorMesh.get())
					{
						Meshes::Mesh3DInstance* m3di = nullptr;
						_room->add_mesh(doorMesh.get(), dp, &m3di);
						if (m3di)
						{
							m3di->access_can_be_combined() = false;
							m3di->set_prepare_to_render([dir](Meshes::Mesh3DInstance& _meshInstance) { ArtDir::door_in_room_mesh__setup_open_world_directions(nullptr, &_meshInstance, dir); });
						}
						Framework::MeshNode::copy_not_dropped_to(doorMesh->get_mesh_nodes(), _roomGeneratingContext.meshNodes, dp);
					}
					else
					{
						error(TXT("could not generate door mesh"));
						AN_BREAK;
						return false;
					}
				}
			}
		}

		// place elevators
		{
			scoped_call_stack_info(TXT("place elevators"));
			for_every(e, elevators)
			{
				Framework::Scenery* cartPoints[] = { nullptr, nullptr };
				// add cart points
				if (auto* cartPointSceneryType = info->find<Framework::SceneryType>(variables, info->cartPointSceneryType))
				{
					cartPointSceneryType->load_on_demand_if_required();

					for_count(int, floorIdx, 2)
					{
						scoped_call_stack_info(TXT("spawn cart point"));

						Framework::Scenery* cartPointScenery = nullptr;
						_game->perform_sync_world_job(TXT("spawn cart point"), [&cartPointScenery, cartPointSceneryType, _room]()
							{
								cartPointScenery = new Framework::Scenery(cartPointSceneryType, String::empty());
								cartPointScenery->init(_room->get_in_sub_world());
							});
						elevatorAppearanceSetup.setup_variables(_room, cartPointScenery);
						cartPointScenery->access_individual_random_generator() = randomGenerator.spawn();
						cartPointScenery->access_variables().access<float>(NAME(cartWidth)) = tileSize;
						cartPointScenery->access_variables().access<float>(NAME(cartLength)) = tileSize;
						_room->collect_variables(cartPointScenery->access_variables());
						info->apply_generation_parameters_to(cartPointScenery->access_variables());
						cartPointScenery->initialise_modules();
						auto useCartPointPlacement = floorIdx == 0 ? e->placement0 : e->placement1;
						auto floorScenery = floorSceneries[floorIdx == 0 ? e->floor0 : e->floor1];
						_game->perform_sync_world_job(TXT("place cart point"), [&cartPointScenery, _room, useCartPointPlacement, floorScenery]()
							{
								cartPointScenery->get_presence()->place_in_room(_room, useCartPointPlacement);
								cartPointScenery->get_presence()->force_base_on(floorScenery);
							});
						_game->on_newly_created_object(cartPointScenery);
						cartPoints[floorIdx] = cartPointScenery;
					}
				}

				// add cart
				if (auto* cartSceneryType = info->find<Framework::SceneryType>(variables, info->cartSceneryType))
				{
					scoped_call_stack_info(TXT("add cart"));

					cartSceneryType->load_on_demand_if_required();

					Framework::Scenery* cartScenery = nullptr;
					_game->perform_sync_world_job(TXT("spawn cart"), [&cartScenery, cartSceneryType, _room]()
						{
							cartScenery = new Framework::Scenery(cartSceneryType, String::empty());
							cartScenery->init(_room->get_in_sub_world());
						});
					Vector3 laneVector = e->placement0.vector_to_world(e->placement1.get_translation() - e->placement0.get_translation());
					elevatorAppearanceSetup.setup_variables(_room, cartScenery);
					cartScenery->access_individual_random_generator() = randomGenerator.spawn();
					cartScenery->access_variables().access<bool>(NAME(withSlidingLocomotionCollision)) = false; // we take care of that here by explicitly blocking stuff
					cartScenery->access_variables().access<float>(NAME(cartWidth)) = tileSize;
					cartScenery->access_variables().access<float>(NAME(cartLength)) = tileSize;
					cartScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopA)) = floorSceneries[e->floor0];
					cartScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopB)) = floorSceneries[e->floor1];
					cartScenery->access_variables().access<Transform>(NAME(elevatorStopACartPoint)) = floorSceneries[e->floor0]->get_presence()->get_placement().to_local(e->placement0);
					cartScenery->access_variables().access<Transform>(NAME(elevatorStopBCartPoint)) = floorSceneries[e->floor1]->get_presence()->get_placement().to_local(e->placement1);
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
				if (auto* cartLaneSceneryType = info->find<Framework::SceneryType>(variables, info->cartLaneSceneryType))
				{
					scoped_call_stack_info(TXT("add cart lane"));

					cartLaneSceneryType->load_on_demand_if_required();

					Framework::Scenery* cartLaneScenery = nullptr;
					Transform placementA = e->placement0;
					Transform placementB = e->placement1;
					int floorIdx = 0;
					Vector3 laneVector = e->placement0.vector_to_world(e->placement1.get_translation() - e->placement0.get_translation());
					if (laneVector.z < 0.0f)
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
					an_assert(laneVector.z > 0.0f);
					elevatorAppearanceSetup.setup_variables(_room, cartLaneScenery);
					cartLaneScenery->access_individual_random_generator() = randomGenerator.spawn();
					cartLaneScenery->access_variables().access<float>(NAME(cartWidth)) = tileSize;
					cartLaneScenery->access_variables().access<float>(NAME(cartLength)) = tileSize;
					cartLaneScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopA)) = floorSceneries[floorIdx == 0 ? e->floor0 : e->floor1];
					cartLaneScenery->access_variables().access<SafePtr<Framework::IModulesOwner>>(NAME(elevatorStopB)) = floorSceneries[floorIdx == 0 ? e->floor1 : e->floor0];
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

			}
		}

		// add block-sliding-locomotion boxes
		if (forSlidingLocomotion)
		{
			scoped_call_stack_info(TXT("do work for sliding locomotion"));

			float const offEdge = 0.05f;
			float const offSides = 0.01f;
			/**
			 *	Go through each floor, through each side and find holes between boxes (just try each next boxes max + offset to check if it is valid, then the lowest min)
			 */
#ifdef DEBUG_DRAW_BLOCK_SLIDING_LOCOMOTION
			for_every(b, blocked)
			{
				Box bb;
				bb.setup(*b);
				bb.set_half_size(bb.get_half_size() - Vector3(0.1f, 0.1f, 0.0f));
				debug_context(_room);
				debug_draw_time_based(1000000.0f, debug_draw_box(true, false, Colour::orange, 0.3f, bb));
				debug_no_context();
			}
#endif
			if (auto* blockSlidingLocomotionBoxSceneryType = info->find<Framework::SceneryType>(variables, info->blockSlidingLocomotionBoxSceneryType))
			{
				scoped_call_stack_info(TXT("got block sliding locomotion box scenery type"));
				scoped_call_stack_info_str_printf(TXT("it is: %S"), blockSlidingLocomotionBoxSceneryType->get_name().to_string().to_char());

				for_every(f, floorPlacement)
				{
					for_count(int, dirLocal, DirFourClockwise::NUM)
					{
						// towards the centre, dirLocal is just to get cardinal directions right
						Transform edgePlacementLS = look_at_matrix((Vector3::yAxis * (floorInnerSize * 0.5f - offEdge)).rotated_by_yaw(90.0f * (float)dirLocal), Vector3::zero, Vector3::zAxis).to_transform();

						Transform edgePlacementWS = f->get().to_world(edgePlacementLS);
					
						Array<Range3> blockedLS;
						for_every(b, blocked)
						{
							Range3 localB;
							localB.construct_from_placement_and_range3(edgePlacementWS.to_local(Transform::identity), *b);
							blockedLS.push_back(localB);
						}
#ifdef DEBUG_DRAW_BLOCK_SLIDING_LOCOMOTION
						for_every(b, blockedLS)
						{
							Box bb;
							bb.setup(*b);
							debug_context(_room);
							debug_push_transform(edgePlacementWS);
							debug_draw_time_based(1000000.0f, debug_draw_box(true, false, Colour::yellow, 0.05f, bb));
							debug_pop_transform();
							debug_no_context();
						}
#endif

						Vector3 dirLS = Vector3::xAxis;
						Optional<Vector3> lastValidLS;
						float endAtX = floorInnerSize * 0.5f - offEdge;
						Vector3 atLS = Vector3(-endAtX, tileSize * 0.5f, 0.5f);
						while (atLS.x < endAtX)
						{
							if (! lastValidLS.is_set())
							{
								// check if this is a good spot
								bool goodSpot = true;
								for_every(b, blockedLS)
								{
									if (b->does_contain(atLS))
									{
										goodSpot = false;
										break;
									}
								}
								if (goodSpot)
								{
									lastValidLS = atLS;
									continue;
								}
								else
								{
									float nextX = endAtX;
									for_every(b, blockedLS)
									{
										if (b->does_contain(atLS))
										{
											float xCandidate = b->x.max;
											if (xCandidate > atLS.x &&
												nextX > xCandidate)
											{
												nextX = xCandidate + offSides;
											}
										}
									}
#ifdef DEBUG_DRAW_BLOCK_SLIDING_LOCOMOTION
									debug_context(_room);
									debug_draw_time_based(1000000.0f, debug_draw_arrow(true, Colour::green, edgePlacementWS.location_to_world(atLS), edgePlacementWS.location_to_world(Vector3(nextX, atLS.y, atLS.z))));
									debug_no_context();
#endif
									atLS.x = nextX;
									continue; // we will check in the next go
								}
							}
							else
							{
								float nextX = endAtX;
								for_every(b, blockedLS)
								{
									float xCandidate = b->x.min;
									if (xCandidate > atLS.x &&
										nextX > xCandidate)
									{
										nextX = xCandidate - offSides;
									}
								}
								atLS.x = nextX;

#ifdef DEBUG_DRAW_BLOCK_SLIDING_LOCOMOTION
								debug_context(_room);
								debug_draw_time_based(1000000.0f, debug_draw_arrow(true, Colour::red, edgePlacementWS.location_to_world(lastValidLS.get()), edgePlacementWS.location_to_world(atLS)));
								debug_no_context();
#endif

								{
									Transform placement(Vector3((atLS.x + lastValidLS.get().x) * 0.5f, 0.0f, 0.0f), Quat::identity);
									Vector2 boxSize(atLS.x - lastValidLS.get().x - offEdge, tileSize - offEdge * 2.0f);
									add_block_sliding_locomotion_box(_game, _room, blockSlidingLocomotionBoxSceneryType, edgePlacementWS.to_world(placement), boxSize, Vector2(0.5f, 0.0f), randomGenerator);
								}

								lastValidLS.clear();
								atLS.x += offSides * 2.0f;
							}
						}
					}

					{
						float size = floorInnerSize - tileSize * 2.0f;
						Vector2 boxSize = Vector2::one * size;
						add_block_sliding_locomotion_box(_game, _room, blockSlidingLocomotionBoxSceneryType, f->get(), boxSize, Vector2(0.5f, 0.5f), randomGenerator);
					}
				}
			}
		}

		if (info->setAnchorVariable.is_valid())
		{
			scoped_call_stack_info_str_printf(TXT("set anchor variable"));

			Transform anchorProvided = Transform::identity;
			if (info->getAnchorPOI.is_valid())
			{
				_room->for_every_point_of_interest(info->getAnchorPOI, [&anchorProvided](Framework::PointOfInterestInstance* _poi)
					{
						anchorProvided = _poi->calculate_placement();
					});
			}

			Transform roomCentrePOI = anchorProvided;
			if (info->roomCentrePOI.is_valid())
			{
				_room->for_every_point_of_interest(info->roomCentrePOI, [&roomCentrePOI](Framework::PointOfInterestInstance* _poi)
					{
						roomCentrePOI = _poi->calculate_placement();
					});
			}

			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				piow->tag_open_world_directions_for_cell(_room);
				DirFourClockwise::Type dirs[] = { DirFourClockwise::Up
												, DirFourClockwise::Down
												, DirFourClockwise::Right
												, DirFourClockwise::Left };
				for_count(int, i, DirFourClockwise::NUM)
				{
					DirFourClockwise::Type inDir = dirs[i];
					if (auto* dir = piow->find_best_door_in_dir(_room, inDir))
					{
						Vector3 placementInDir = dir->get_hole_centre_WS();
						Vector3 actualInDir = placementInDir - roomCentrePOI.get_translation();
						actualInDir.z = 0.0f;
						actualInDir.normalise();

						// take inDir into account
						actualInDir = actualInDir.rotated_by_yaw(-(float)inDir * 90.0f);

						// where room centre should be
						Transform roomCentrePOIAsShouldBe = look_matrix(roomCentrePOI.get_translation(), actualInDir, Vector3::zAxis).to_transform();

						Transform inDirInRoom = roomCentrePOIAsShouldBe.to_world(roomCentrePOI.to_local(anchorProvided));

						_room->access_variables().access<Transform>(info->setAnchorVariable) = inDirInRoom;
						break;
					}
				}
			}
		}
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated floors with elevators"));
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

void FloorsWithElevators::add_block_sliding_locomotion_box(Framework::Game* _game, Framework::Room* _room, Framework::SceneryType* blockSlidingLocomotionBoxSceneryType, Transform const& _placement, Vector2 const& _boxSize, Vector2 const & _boxAtPt, Random::Generator& randomGenerator)
{
	scoped_call_stack_info(TXT("add_block_sliding_locomotion_box"));
	blockSlidingLocomotionBoxSceneryType->load_on_demand_if_required();

	Framework::Scenery* blockSlidingLocomotionBoxScenery = nullptr;
	_game->perform_sync_world_job(TXT("spawn block sliding locomotion box"), [&blockSlidingLocomotionBoxScenery, blockSlidingLocomotionBoxSceneryType, _room]()
		{
			scoped_call_stack_info(TXT("spawn block sliding locomotion box"));
			blockSlidingLocomotionBoxScenery = new Framework::Scenery(blockSlidingLocomotionBoxSceneryType, String::empty());
			scoped_call_stack_info(TXT("init in world"));
			blockSlidingLocomotionBoxScenery->init(_room->get_in_sub_world());
		});
	blockSlidingLocomotionBoxScenery->access_individual_random_generator() = randomGenerator.spawn();
	blockSlidingLocomotionBoxScenery->access_variables().access<Vector3>(NAME(boxSize)) = Vector3(_boxSize.x, _boxSize.y, 2.0f);
	blockSlidingLocomotionBoxScenery->access_variables().access<Vector3>(NAME(boxAtPt)) = Vector3(_boxAtPt.x, _boxAtPt.y, 0.0f);
	_room->collect_variables(blockSlidingLocomotionBoxScenery->access_variables());
	info->apply_generation_parameters_to(blockSlidingLocomotionBoxScenery->access_variables());
	blockSlidingLocomotionBoxScenery->initialise_modules();
	_game->perform_sync_world_job(TXT("place block-sliding-locomotion box"), [_placement, &blockSlidingLocomotionBoxScenery, _room]()
		{
			blockSlidingLocomotionBoxScenery->get_presence()->place_in_room(_room, _placement);
		});
	_game->on_newly_created_object(blockSlidingLocomotionBoxScenery);
}