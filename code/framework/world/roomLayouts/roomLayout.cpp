#include "roomLayout.h"

#include "objectLayout.h"
#include "roomLayoutNames.h"
#include "wallLayout.h"

#include "..\..\debug\previewGame.h"
#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\library\usedLibraryStoredOrTagged.inl"

#include "..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// global references
DEFINE_STATIC_NAME_STR(grRoomLayoutRoomType, TXT("room layout room type"));
DEFINE_STATIC_NAME_STR(grRoomLayoutFloorTilesMeshGenerator, TXT("room layout floor tiles mesh generator"));
DEFINE_STATIC_NAME_STR(grRoomLayoutWallMeshGenerator, TXT("room layout wall mesh generator"));
DEFINE_STATIC_NAME_STR(grRoomLayoutDoorHoleInWallMeshGenerator, TXT("room layout door hole in wall mesh generator"));

//

REGISTER_FOR_FAST_CAST(RoomLayout);
LIBRARY_STORED_DEFINE_TYPE(RoomLayout, roomLayout);

RoomLayout::RoomLayout(Library * _library, LibraryName const & _name)
: base(_library, _name)
, roomGeneratorInterface(new RoomGeneratorInterface(this))
{
}

RoomLayout::~RoomLayout()
{
	if (auto* r = roomGeneratorInterface.get())
	{
		roomGeneratorInterface->clear_room_layout();
		roomGeneratorInterface.clear();
	}
}

bool RoomLayout::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear/reset first
	//
	roomTags.clear();
	doorCount = DoorCount();
	roomSize = Vector2(4.0f, 4.0f);
	roomHeight = 2.8f;
	roomContentTagsRequired.clear();
	environmentTypes.clear();
	floorTileMeshes.clear();
	objectLayout.clear();
	wallLayoutYP.clear();
	wallLayoutXP.clear();
	wallLayoutYM.clear();
	wallLayoutXM.clear();
	leftCornerLayoutYP.clear();
	leftCornerLayoutXP.clear();
	leftCornerLayoutYM.clear();
	leftCornerLayoutXM.clear();

	// load
	//
	for_every(node, _node->children_named(TXT("doorCount")))
	{
		doorCount.count.load_from_xml(node);
		doorCount.maxTotal = node->get_int_attribute(TXT("maxTotal"));
	}

	roomTags.load_from_xml_attribute_or_child_node(_node, TXT("roomTags"));
	roomSize.load_from_xml_child_node(_node, TXT("roomSize"));
	roomHeight = _node->get_float_attribute_or_from_child(TXT("roomHeight"), roomHeight);

	roomContentTagsRequired.load_from_xml_attribute_or_child_node(_node, TXT("roomContentTagsRequired"));
	for_every(node, _node->children_named(TXT("roomContent")))
	{
		roomContentTagsRequired.load_from_xml_attribute_or_child_node(node, TXT("tagsRequired"));
	}

	for_every(node, _node->children_named(TXT("environmentType")))
	{
		environmentTypes.push_back();
		if (environmentTypes.get_last().load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			environmentTypes.pop_back();
		}
	}

	for_every(node, _node->children_named(TXT("floorTileMesh")))
	{
		floorTileMeshes.push_back();
		if (floorTileMeshes.get_last().load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			floorTileMeshes.pop_back();
		}
	}

	for_every(node, _node->children_named(TXT("objectLayout")))
	{
		objectLayout.push_back();
		if (objectLayout.get_last().load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			objectLayout.pop_back();
		}
	}

	for_count(int, dirIdx, 4)
	{
		List<UsedLibraryStoredOrTagged<WallLayout>>* wallLayoutList = nullptr;
		tchar const* wallLayoutNameNode = nullptr;
		switch (dirIdx)
		{
		case 0: wallLayoutList = &wallLayoutYP; wallLayoutNameNode = TXT("wallLayoutYP"); break;
		case 1: wallLayoutList = &wallLayoutXP; wallLayoutNameNode = TXT("wallLayoutXP"); break;
		case 2: wallLayoutList = &wallLayoutYM; wallLayoutNameNode = TXT("wallLayoutYM"); break;
		case 3: wallLayoutList = &wallLayoutXM; wallLayoutNameNode = TXT("wallLayoutXM"); break;
		default: an_assert(false);
		}

		an_assert(wallLayoutList);

		for_every(node, _node->children_named(wallLayoutNameNode))
		{
			wallLayoutList->push_back();
			if (wallLayoutList->get_last().load_from_xml(node, _lc))
			{
				// ok
			}
			else
			{
				wallLayoutList->pop_back();
			}
		}
	}

	for_count(int, dirIdx, 4)
	{
		List<UsedLibraryStoredOrTagged<ObjectLayout>>* leftCornerLayoutList = nullptr;
		tchar const* leftCornerLayoutNameNode = nullptr;
		switch (dirIdx)
		{
		case 0: leftCornerLayoutList = &leftCornerLayoutYP; leftCornerLayoutNameNode = TXT("leftCornerLayoutYP"); break;
		case 1: leftCornerLayoutList = &leftCornerLayoutXP; leftCornerLayoutNameNode = TXT("leftCornerLayoutXP"); break;
		case 2: leftCornerLayoutList = &leftCornerLayoutYM; leftCornerLayoutNameNode = TXT("leftCornerLayoutYM"); break;
		case 3: leftCornerLayoutList = &leftCornerLayoutXM; leftCornerLayoutNameNode = TXT("leftCornerLayoutXM"); break;
		default: an_assert(false);
		}

		an_assert(leftCornerLayoutList);

		for_every(node, _node->children_named(leftCornerLayoutNameNode))
		{
			leftCornerLayoutList->push_back();
			if (leftCornerLayoutList->get_last().load_from_xml(node, _lc))
			{
				// ok
			}
			else
			{
				leftCornerLayoutList->pop_back();
			}
		}
	}

	return result;
}

bool RoomLayout::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(libStoredOrTagged, environmentTypes)
	{
		result &= libStoredOrTagged->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	for_every(libStoredOrTagged, floorTileMeshes)
	{
		result &= libStoredOrTagged->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	for_every(ol, objectLayout)
	{
		result &= ol->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	for_every(wl, wallLayoutYP) { result &= wl->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }
	for_every(wl, wallLayoutXP) { result &= wl->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }
	for_every(wl, wallLayoutYM) { result &= wl->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }
	for_every(wl, wallLayoutXM) { result &= wl->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }

	for_every(ol, leftCornerLayoutYP) { result &= ol->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }
	for_every(ol, leftCornerLayoutXP) { result &= ol->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }
	for_every(ol, leftCornerLayoutYM) { result &= ol->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }
	for_every(ol, leftCornerLayoutXM) { result &= ol->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve); }
	
	return result;
}

bool RoomLayout::setup_custom_preview_room(SubWorld* _testSubWorld, REF_ Room*& _testRoomForCustomPreview, Random::Generator const& _rg)
{
	base::setup_custom_preview_room(_testSubWorld, _testRoomForCustomPreview, _rg);

	auto* game = Game::get();

	ActivationGroupPtr activationGroup = game->push_new_activation_group();

	_testRoomForCustomPreview = async_create_room(CreateRoomParams()
		.use_random_generator(_rg)
		.in_sub_world(_testSubWorld));

	if (_testRoomForCustomPreview)
	{
		_testRoomForCustomPreview->ready_for_game();
	}

	game->request_ready_and_activate_all_inactive(activationGroup.get(), _testSubWorld->get_in_world());

	game->pop_activation_group(activationGroup.get());

	return true;
}

Room* RoomLayout::async_create_room(CreateRoomParams const& _params)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	an_assert(_params.region || _params.subWorld);
	an_assert(Game::get()->get_current_activation_group());

	Random::Generator rg;
	if (_params.randomGenerator.is_set())
	{
		rg = _params.randomGenerator.get();
	}

	Room* room = nullptr;

	if (RoomType* roomType = ::Library::get_current()->get_global_references().get<RoomType>(NAME(grRoomLayoutRoomType)))
	{
		Game::get()->perform_sync_world_job(TXT("create room"), [&room, roomType, &rg, &_params]()
			{
				room = new Room(_params.subWorld? _params.subWorld : _params.region->get_in_sub_world(), _params.region, roomType, rg);
			});

		room->access_tags().set_tags_from(roomTags);
		room->set_room_generator_info(roomGeneratorInterface.get());
	}

	return room;
}

bool RoomLayout::generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);

	RoomLayoutContext context;
	{
		Tags useRoomContentTagsRequired;
		useRoomContentTagsRequired.set_tags_from(roomContentTagsRequired);
		context.roomContentTagged.setup_with_tags(useRoomContentTagsRequired);
	}

	auto rg = _room->get_individual_random_generator();
	rg.advance_seed(805, 208584);

	auto* game = Game::get();

	// prepare variables for generating stuff
	variables.access<Vector2>(RoomLayoutNames::MeshVariables::room_size()) = roomSize;
	variables.access<float>(RoomLayoutNames::MeshVariables::room_height()) = roomHeight;

	// store also for room
	_room->access_variables().access<Vector2>(RoomLayoutNames::MeshVariables::room_size()) = roomSize;

	if (!environmentTypes.is_empty())
	{
		EnvironmentType* useEnvironmentType = UsedLibraryStoredOrTagged<EnvironmentType>::choose_random(environmentTypes, REF_ rg, context.roomContentTagged, true);

		if (useEnvironmentType)
		{
			_room->access_use_environment().clear();
			_room->access_use_environment().useEnvironmentType = useEnvironmentType;
		}
	}

	if (! floorTileMeshes.is_empty())
	{
		Mesh* floorTileMesh = UsedLibraryStoredOrTagged<Mesh>::choose_random(floorTileMeshes, REF_ rg, context.roomContentTagged, true);

		if (floorTileMesh)
		{
			variables.access<LibraryName>(RoomLayoutNames::MeshVariables::floor_tile_mesh()) = floorTileMesh->get_name();
			if (auto* floorTilesMeshGenerator = ::Library::get_current()->get_global_references().get<MeshGenerator>(NAME(grRoomLayoutFloorTilesMeshGenerator)))
			{
				game->async_add_generic_object(Game::AddGenericObjectParams()
					.to_room(_room)
					.use_mesh_generator(floorTilesMeshGenerator)
					.use_variables(&variables));
			}
		}
	}

	Random::Generator layoutRG = rg.spawn();

	struct PlacedDoor
	{
		int wallIdx = NONE;
		float atWall = 0.0f;
		Range occupies = Range::empty;
		float doorDepth = 0.0f;
		float doorHolePieceLength = 0.0f;
		DoorInRoom* doorInRoom = nullptr;
		DoorType const * doorType = nullptr;
	};
	struct AllowedDoorPlacement
	{
		int wallIdx = NONE;
		Optional<float> atFixed;
		Optional<int> atFixedIdx;
		Optional<Range> atRange;
		float probCoef = 1.0f;
	};

	ObjectLayoutPrepared objectLayoutPrepared;
	WallLayoutPrepared wallLayoutPrepared[4];
	ObjectLayoutPrepared leftCornerLayoutPrepared[4];
	Array<PlacedDoor> placedDoors;
	Array<AllowedDoorPlacement> allowedDoorPlacements;

	auto* doorHoleInWallMeshGenerator = ::Library::get_current()->get_global_references().get<MeshGenerator>(NAME(grRoomLayoutDoorHoleInWallMeshGenerator));

	int tryIdx = 0;
	while (true)
	{
		++tryIdx;
		if (tryIdx > 1000)
		{
			error(TXT("can't generate"));
#ifndef AN_DEVELOPMENT
			break;
#endif
		}

		bool isOk = true;

		// clear setup
		{
			objectLayoutPrepared.clear();
			for_count(int, wlIdx, 4)
			{
				wallLayoutPrepared[wlIdx].clear();
				leftCornerLayoutPrepared[wlIdx].clear();
			}
			placedDoors.clear();
		}

		// prepare wall layouts to know where the door could be placed
		{
			for_count(int, wlIdx, 4)
			{
				List<UsedLibraryStoredOrTagged<WallLayout>> const * wallLayoutList = nullptr;
				Transform wallPlacement = Transform::identity;
				float wallLength = 0.0f;
				switch (wlIdx)
				{
				case 0: wallLayoutList = &wallLayoutYP; wallPlacement = Transform(roomSize.to_vector3() * Vector3( 0.0f,  0.5f, 0.0f), Rotator3(0.0f,   0.0f, 0.0f).to_quat()); wallLength = roomSize.x; break;
				case 1: wallLayoutList = &wallLayoutXP; wallPlacement = Transform(roomSize.to_vector3() * Vector3( 0.5f,  0.0f, 0.0f), Rotator3(0.0f,  90.0f, 0.0f).to_quat()); wallLength = roomSize.y; break;
				case 2: wallLayoutList = &wallLayoutYM; wallPlacement = Transform(roomSize.to_vector3() * Vector3( 0.0f, -0.5f, 0.0f), Rotator3(0.0f, 180.0f, 0.0f).to_quat()); wallLength = roomSize.x; break;
				case 3: wallLayoutList = &wallLayoutXM; wallPlacement = Transform(roomSize.to_vector3() * Vector3(-0.5f,  0.0f, 0.0f), Rotator3(0.0f, 270.0f, 0.0f).to_quat()); wallLength = roomSize.y; break;
				default: an_assert(false);
				}

				if (WallLayout const* wallLayout = UsedLibraryStoredOrTagged<WallLayout>::choose_random(*wallLayoutList, REF_ layoutRG, context.roomContentTagged, true))
				{
					Random::Generator useRG = layoutRG.spawn();

					wallLayout->async_prepare_layout(context, _room, wallPlacement, wallLength, REF_ useRG, OUT_ wallLayoutPrepared[wlIdx]);
				}
				else
				{
					wallLayoutPrepared[wlIdx].prepare_empty(wallPlacement, wallLength);
				}
			}
		}

		// prepare left corner layouts
		{
			for_count(int, lclIdx, 4)
			{
				List<UsedLibraryStoredOrTagged<ObjectLayout>> const * leftCornerLayoutList = nullptr;
				Transform leftCornerPlacement = Transform::identity;
				switch (lclIdx)
				{
				case 0: leftCornerLayoutList = &leftCornerLayoutYP; leftCornerPlacement = Transform(roomSize.to_vector3() * Vector3(-0.5f,  0.5f, 0.0f), Rotator3(0.0f,   0.0f, 0.0f).to_quat()); break;
				case 1: leftCornerLayoutList = &leftCornerLayoutXP; leftCornerPlacement = Transform(roomSize.to_vector3() * Vector3( 0.5f,  0.5f, 0.0f), Rotator3(0.0f,  90.0f, 0.0f).to_quat()); break;
				case 2: leftCornerLayoutList = &leftCornerLayoutYM; leftCornerPlacement = Transform(roomSize.to_vector3() * Vector3( 0.5f, -0.5f, 0.0f), Rotator3(0.0f, 180.0f, 0.0f).to_quat()); break;
				case 3: leftCornerLayoutList = &leftCornerLayoutXM; leftCornerPlacement = Transform(roomSize.to_vector3() * Vector3(-0.5f, -0.5f, 0.0f), Rotator3(0.0f, 270.0f, 0.0f).to_quat()); break;
				default: an_assert(false);
				}

				if (ObjectLayout const* useLeftCornerLayout = UsedLibraryStoredOrTagged<ObjectLayout>::choose_random(*leftCornerLayoutList, REF_ layoutRG, context.roomContentTagged, true))
				{
					Random::Generator useRG = layoutRG.spawn();

					useLeftCornerLayout->async_prepare_layout(context, _room, leftCornerPlacement, REF_ useRG, OUT_ leftCornerLayoutPrepared[lclIdx]);
				}
				else
				{
					leftCornerLayoutPrepared[lclIdx].clear();
				}
			}
		}

		// prepare objects
		{
			if (ObjectLayout const* useObjectLayout = UsedLibraryStoredOrTagged<ObjectLayout>::choose_random(objectLayout, REF_ layoutRG, context.roomContentTagged, true))
			{
				Random::Generator useRG = layoutRG.spawn();

				useObjectLayout->async_prepare_layout(context, _room, Transform::identity, REF_ useRG, OUT_ objectLayoutPrepared);
			}
			else
			{
				objectLayoutPrepared.clear();
			}
		}

		// check if doors can be placed to fit with layouts
		{
			Random::Generator doorsRG = layoutRG.spawn();
			for_every_ptr(dir, _room->get_all_doors())
			{
				placedDoors.push_back();
				if (dir->is_placed())
				{
					continue;
				}
				auto& placedDoor = placedDoors.get_last();
				placedDoor.doorInRoom = dir;
				// door params
				float & doorDepth = placedDoor.doorDepth;
				float & doorPieceLength = placedDoor.doorHolePieceLength;
				// get door params (from door type, door piece length etc)
				{
					doorDepth = 0.0f;
					doorPieceLength = 0.0f;
					if (auto* door = dir->get_door())
					{
						if (auto* dt = door->get_door_type())
						{
							placedDoor.doorType = dt;
							doorDepth = dt->get_custom_parameters().get_value<float>(RoomLayoutNames::DoorTypeCustomParameters::door_depth(), doorDepth);
							{
								float v = dt->get_custom_parameters().get_value<float>(RoomLayoutNames::DoorTypeCustomParameters::door_hole_piece_length(), 0.0f);
								if (v > 0.0f)
								{
									doorPieceLength = v;
								}
								else if (auto* dh = dt->get_hole())
								{
									Range2 dhSize = dh->calculate_size(DoorSide::A, Vector2::one);
									doorPieceLength = max(abs(dhSize.x.min), abs(dhSize.x.max)) * 2.0f;
								}
							}
						}
					}
				}
				// get list of allowed placements
				{
					allowedDoorPlacements.clear();
					for_count(int, wallIdx, 4)
					{
						auto const& wallLayout = wallLayoutPrepared[wallIdx];
						if (wallLayout.usesFixedDoorPlacements)
						{
							for_every(fdp, wallLayout.fixedDoorPlacements)
							{
								float at = fdp->at;
								bool isValid = true;

								for_every(ddr, wallLayout.disallowedDoorPlacements)
								{
									if (at + doorPieceLength * 0.5f >= ddr->range.min &&
										at - doorPieceLength * 0.5f < ddr->range.max)
									{
										isValid = false;
										break;
									}
								}

								if (isValid)
								{
									allowedDoorPlacements.push_back();
									auto& adp = allowedDoorPlacements.get_last();
									adp.wallIdx = wallIdx;
									adp.atFixed = at;
									adp.atFixedIdx = for_everys_index(fdp);
									adp.probCoef = 1.0f;
								}
							}
						}
						else
						{
							float start = wallLayout.wallRange.min;
							while (start < wallLayout.wallRange.max)
							{
								// first find valid start
								{
									bool isStartValid = false;
									while (!isStartValid && start < wallLayout.wallRange.max)
									{
										isStartValid = true;
										for_every(ddr, wallLayout.disallowedDoorPlacements)
										{
											if (start >= ddr->range.min &&
												start < ddr->range.max)
											{
												start = ddr->range.max;
												isStartValid = false;
											}
										}
									}
								}

								float end = wallLayout.wallRange.max;
								// find valid end
								{
									for_every(ddr, wallLayout.disallowedDoorPlacements)
									{
										if (ddr->range.max > start)
										{
											end = min(end, ddr->range.min);
										}
									}
								}

								if (start < end)
								{
									if (end - start >= placedDoor.doorHolePieceLength)
									{
										allowedDoorPlacements.push_back();
										auto& adp = allowedDoorPlacements.get_last();
										adp.wallIdx = wallIdx;
										adp.atRange = Range(start + placedDoor.doorHolePieceLength * 0.5f, end - placedDoor.doorHolePieceLength * 0.5f);
										adp.probCoef = 1.0f;
									}
									start = end;
								}
								else if (start < wallLayout.wallRange.max)
								{
									an_assert(false, TXT("shouldn't happen?"));
									start = end + 0.001f;
								}
							}
						}
					}
				}
				// choose one allowed placement and store
				{
					if (allowedDoorPlacements.is_empty())
					{
						isOk = false;
						break;
					}
					else
					{
						int chosenIdx = RandomUtils::ChooseFromContainer<Array<AllowedDoorPlacement>, AllowedDoorPlacement>::choose(
							doorsRG, allowedDoorPlacements, [](AllowedDoorPlacement const& _e) { return _e.probCoef; });
						auto& adp = allowedDoorPlacements[chosenIdx];

						if (adp.atFixed.is_set())
						{
							// use it and invalidate it
							an_assert(adp.atFixedIdx.is_set());
							placedDoor.wallIdx = adp.wallIdx;
							placedDoor.atWall = adp.atFixed.get();
							wallLayoutPrepared[adp.wallIdx].fixedDoorPlacements.remove_fast_at(adp.atFixedIdx.get());
						}
						else if (adp.atRange.is_set())
						{
							placedDoor.wallIdx = adp.wallIdx; 
							placedDoor.atWall = doorsRG.get_float(adp.atRange.get());
						}
						else
						{
							an_assert(false, TXT("what is this allowed placement?"));
							isOk = false;
						}
						placedDoor.occupies = Range(placedDoor.atWall);
						placedDoor.occupies.expand_by(doorPieceLength * 0.5f);
						// and block everything around
						{
							wallLayoutPrepared[adp.wallIdx].disallowedDoorPlacements.push_back();
							auto& ddp = wallLayoutPrepared[adp.wallIdx].disallowedDoorPlacements.get_last();
							ddp.range = placedDoor.occupies;
							wallLayoutPrepared[adp.wallIdx].unify_disallowed_door_placements();
						}
					}
				}
			}
			if (!isOk)
			{
				continue;
			}
		}

		// populate room with wall layout and doors
		if (isOk)
		{
			// wall layouts
			{
				for_count(int, dirIdx, 4)
				{
					auto const& wallLayout = wallLayoutPrepared[dirIdx];

					Tags tagElements;

					switch (dirIdx)
					{
					case 0: tagElements.set_tag(RoomLayoutNames::ObjectTags::wall_element_yp()); break;
					case 1: tagElements.set_tag(RoomLayoutNames::ObjectTags::wall_element_xp()); break;
					case 2: tagElements.set_tag(RoomLayoutNames::ObjectTags::wall_element_ym()); break;
					case 3: tagElements.set_tag(RoomLayoutNames::ObjectTags::wall_element_xm()); break;
					default: an_assert(false);
					}
					// add plain walls
					if (auto* wallMeshGenerator = ::Library::get_current()->get_global_references().get<MeshGenerator>(NAME(grRoomLayoutWallMeshGenerator)))
					{
						float wallStart = wallLayout.wallRange.min;
						while (wallStart < wallLayout.wallRange.max)
						{
							// find valid start
							{
								bool isStartValid = false;
								while (! isStartValid && wallStart < wallLayout.wallRange.max)
								{
									isStartValid = true;
									for_every(e, wallLayout.meshes)
									{
										if (is_flag_set(e->flags, WallLayoutElementFlags::ActualWall))
										{
											Range occupies = e->spaceOccupied.x;
											if (wallStart >= occupies.min &&
												wallStart < occupies.max)
											{
												wallStart = occupies.max;
												isStartValid = false;
											}
										}
									}
									for_every(pd, placedDoors)
									{
										if (pd->wallIdx == dirIdx)
										{
											Range occupies = pd->occupies;
											if (wallStart >= occupies.min &&
												wallStart < occupies.max)
											{
												wallStart = occupies.max;
												isStartValid = false;
											}
										}
									}
								}
							}

							// find valid end
							float wallEnd = wallLayout.wallRange.max;
							{
								for_every(e, wallLayout.meshes)
								{
									if (is_flag_set(e->flags, WallLayoutElementFlags::ActualWall))
									{
										Range occupies = e->spaceOccupied.x;
										if (occupies.min > wallStart)
										{
											wallEnd = min(wallEnd, occupies.min);
										}
									}
								}
								for_every(pd, placedDoors)
								{
									if (pd->wallIdx == dirIdx)
									{
										Range occupies = pd->occupies;
										if (occupies.min > wallStart)
										{
											wallEnd = min(wallEnd, occupies.min);
										}
									}
								}
							}

							if (wallStart < wallEnd)
							{
								variables.access<float>(RoomLayoutNames::MeshVariables::wall_length()) = wallEnd - wallStart;
								Transform placement = wallLayout.wallPlacement.to_world(Transform(Vector3::xAxis * ((wallStart + wallEnd) * 0.5f), Quat::identity));
								game->async_add_generic_object(Game::AddGenericObjectParams()
									.to_room(_room)
									.at(placement)
									.use_mesh_generator(wallMeshGenerator)
									.use_variables(&variables)
									.tagged(&tagElements));

								wallStart = wallEnd; // will skip the wall on the next validation
							}
							else if (wallStart < wallLayout.wallRange.max)
							{
								an_assert(false, TXT("shouldn't happen?"));
								wallStart = wallEnd + 0.001f;
							}
						}
					}

					// add door holes
					for_every(pd, placedDoors)
					{
						if (pd->wallIdx == dirIdx && pd->doorType)
						{
							Transform atWallPlacement = Transform(Vector3::xAxis * pd->atWall, Quat::identity);

							Transform placement = wallLayout.wallPlacement.to_world(atWallPlacement);

							auto* useDoorHoleInWallMeshGenerator = doorHoleInWallMeshGenerator;
							{
								auto doorMGName = pd->doorType->get_custom_parameters().get_value<LibraryName>(RoomLayoutNames::DoorTypeCustomParameters::door_hole_in_wall_mesh_generator(), LibraryName::invalid());
								if (doorMGName.is_valid())
								{
									if (auto* doorMG = ::Library::get_current()->get_mesh_generators().find(doorMGName))
									{
										useDoorHoleInWallMeshGenerator = doorMG;
									}
								}
							}
							variables.access<Vector2>(RoomLayoutNames::MeshVariables::door_hole_piece_size()) = Vector2(pd->doorHolePieceLength, roomHeight);
							variables.access<LibraryName>(RoomLayoutNames::MeshVariables::door_type()) = pd->doorType->get_name();
							game->async_add_generic_object(Game::AddGenericObjectParams()
								.to_room(_room)
								.at(placement)
								.use_mesh_generator(useDoorHoleInWallMeshGenerator)
								.use_variables(&variables)
								.tagged(&tagElements));
						}
					}

					// add elements
					for_every(e, wallLayout.meshes)
					{
						Transform placement = wallLayout.wallPlacement.to_world(e->placement);
						game->async_add_generic_object(Game::AddGenericObjectParams()
							.to_room(_room)
							.at(placement)
							.use_mesh(e->mesh)
							.use_variables(&variables)
							.tagged(&tagElements));
					}
				}
			}

			// corner elements
			{
				for_count(int, dirIdx, 4)
				{
					auto const& leftCornerLayout = leftCornerLayoutPrepared[dirIdx];

					Tags tagElements;

					switch (dirIdx)
					{
					case 0: tagElements.set_tag(RoomLayoutNames::ObjectTags::left_corner_element_yp()); break;
					case 1: tagElements.set_tag(RoomLayoutNames::ObjectTags::left_corner_element_xp()); break;
					case 2: tagElements.set_tag(RoomLayoutNames::ObjectTags::left_corner_element_ym()); break;
					case 3: tagElements.set_tag(RoomLayoutNames::ObjectTags::left_corner_element_xm()); break;
					default: an_assert(false);
					}
					// add elements
					for_every(e, leftCornerLayout.meshes)
					{
						Transform placement = e->placement;
						game->async_add_generic_object(Game::AddGenericObjectParams()
							.to_room(_room)
							.at(placement)
							.use_mesh(e->mesh)
							.use_variables(&variables)
							.tagged(&tagElements));
					}
				}
			}

			// object layouts
			{
				Tags tagElements;

				// add elements
				for_every(e, objectLayoutPrepared.meshes)
				{
					Transform placement = e->placement;
					game->async_add_generic_object(Game::AddGenericObjectParams()
						.to_room(_room)
						.at(placement)
						.use_mesh(e->mesh)
						.use_variables(&variables)
						.tagged(&tagElements));
				}
			}

			// placed doors
			for_every(pd, placedDoors)
			{
				if (!pd->doorInRoom)
				{
					continue;
				}
				auto& atWall = wallLayoutPrepared[pd->wallIdx];

				Transform relativePlacement = Transform(Vector3::yAxis * pd->doorDepth, Quat::identity);
				Transform atWallPlacement = Transform(Vector3::xAxis * pd->atWall, Quat::identity).to_world(relativePlacement);

				Transform placement = atWall.wallPlacement.to_world(atWallPlacement);

				pd->doorInRoom->set_placement(placement);
			}

			// everything is fine, we can break 
			break;
		}
	}

	// todo - add more here

	if (!_subGenerator)
	{
		//_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	return true;
}

bool RoomLayout::post_generate(Room* _room, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	return true;
}

//--

REGISTER_FOR_FAST_CAST(RoomLayout::RoomGeneratorInterface);

RoomLayout::RoomGeneratorInterface::RoomGeneratorInterface(RoomLayout* _roomLayout)
: roomLayout(_roomLayout)
{
}

RoomLayout::RoomGeneratorInterface::~RoomGeneratorInterface()
{
}

void RoomLayout::RoomGeneratorInterface::clear_room_layout()
{
	roomLayout = nullptr;
}

bool RoomLayout::RoomGeneratorInterface::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	an_assert(false, TXT("do not use this in such a way!"));
	return false;
}

bool RoomLayout::RoomGeneratorInterface::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	an_assert(false, TXT("do not use this in such a way!"));
	return false;
}

bool RoomLayout::RoomGeneratorInterface::apply_renamer(LibraryStoredRenamer const& _renamer, Library* _library)
{
	an_assert(false, TXT("do not use this in such a way!"));
	return false;
}

RoomGeneratorInfoPtr RoomLayout::RoomGeneratorInterface::create_copy() const
{
	an_assert(false, TXT("do not use this in such a way!"));
	return RoomGeneratorInfoPtr();
}

RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> RoomLayout::RoomGeneratorInterface::generate_piece_for_region_generator(REF_ PieceCombiner::Generator<RegionGenerator>& _generator, Region* _region, REF_ Random::Generator& _randomGenerator) const
{
	an_assert(false, TXT("do not use this in such a way!"));

	return RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>>();
}

bool RoomLayout::RoomGeneratorInterface::generate(Room* _room, bool _subGenerator, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	if (roomLayout)
	{
		return roomLayout->generate(_room, _subGenerator, _roomGeneratingContext);
	}
	return false;
}

bool RoomLayout::RoomGeneratorInterface::post_generate(Room* _room, REF_ RoomGeneratingContext& _roomGeneratingContext) const
{
	if (roomLayout)
	{
		return roomLayout->post_generate(_room, _roomGeneratingContext);
	}
	return false;
}
