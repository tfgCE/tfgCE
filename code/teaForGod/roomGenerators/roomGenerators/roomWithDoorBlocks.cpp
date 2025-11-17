#include "roomWithDoorBlocks.h"

#include "..\roomGenerationInfo.h"

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
//#define OUTPUT_GENERATION_DEBUG_VISUALISER_FAILS_ONLY
//#define OUTPUT_GENERATION_DEBUG_VISUALISER_FAIL_ELEVATORS
//#define OUTPUT_GENERATION_VARIABLES
#else
#ifdef LOG_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif
#endif

//

DEFINE_STATIC_NAME(alignToTiles);
DEFINE_STATIC_NAME(maxSize);
DEFINE_STATIC_NAME(maxTileSize);
DEFINE_STATIC_NAME(maxSizePct);
DEFINE_STATIC_NAME(minSize);
DEFINE_STATIC_NAME(minTileSize);
DEFINE_STATIC_NAME(minSizePct);

DEFINE_STATIC_NAME(maxPlayAreaSize);
DEFINE_STATIC_NAME(maxPlayAreaTileSize);
DEFINE_STATIC_NAME(maxDoorPerBlock);
DEFINE_STATIC_NAME(maxBlockSize);
DEFINE_STATIC_NAME(maxBlockTileSize);

// mesh node
DEFINE_STATIC_NAME(doorBlock);
DEFINE_STATIC_NAME(doorBlockSize);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(roomSize);

//

REGISTER_FOR_FAST_CAST(RoomWithDoorBlocksInfo);

RoomWithDoorBlocksInfo::RoomWithDoorBlocksInfo()
{
}

RoomWithDoorBlocksInfo::~RoomWithDoorBlocksInfo()
{
}

bool RoomWithDoorBlocksInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= roomMeshGenerator.load_from_xml(_node, TXT("roomMeshGenerator"), _lc);

	forceMeshSizeToPlayAreaSize = _node->get_bool_attribute_or_from_child_presence(TXT("forceMeshSizeToPlayAreaSize"), forceMeshSizeToPlayAreaSize);

	return result;
}

bool RoomWithDoorBlocksInfo::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= roomMeshGenerator.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

bool RoomWithDoorBlocksInfo::apply_renamer(Framework::LibraryStoredRenamer const & _renamer, Framework::Library* _library)
{
	bool result = base::apply_renamer(_renamer, _library);

	_renamer.apply_to(roomMeshGenerator);

	return result;
}

Framework::RoomGeneratorInfoPtr RoomWithDoorBlocksInfo::create_copy() const
{
	RoomWithDoorBlocksInfo * copy = new RoomWithDoorBlocksInfo();
	*copy = *this;
	return Framework::RoomGeneratorInfoPtr(copy);
}

bool RoomWithDoorBlocksInfo::internal_generate(Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext) const
{
	bool result = true;

	RoomWithDoorBlocks sr(this);
	if (_room->get_name().is_empty())
	{
		_room->set_name(String::printf(TXT("room with door blocks \"%S\" : \"%S\""), _room->get_room_type()? _room->get_room_type()->get_name().to_string().to_char() : TXT("??"), _room->get_individual_random_generator().get_seed_string().to_char()));
	}
	result &= sr.generate(Framework::Game::get(), _room, _subGenerator, REF_ _roomGeneratingContext);

	result &= base::internal_generate(_room, _subGenerator, REF_ _roomGeneratingContext);
	return result;
}

//

RoomWithDoorBlocks::RoomWithDoorBlocks(RoomWithDoorBlocksInfo const * _info)
: info(_info)
{
}

RoomWithDoorBlocks::~RoomWithDoorBlocks()
{
}

#define YP 0
#define XP 1
#define YM 2
#define XM 3

struct DoorInDoorBlock
{
	Transform placement; // within vr / room
	VR::Zone zone;
	float doorWidth;
	int side; // above
	float at; // on the side
	Framework::DoorInRoom* dir;
};

struct DoorBlock
{
	int preferredDoorCount = 1;
	Range2 block; // within rect
	VR::Zone zone;
	ArrayStatic<DoorInDoorBlock, 16> doors;
	int doorsAvailable[4]; // per side

	DoorBlock()
	{
		SET_EXTRA_DEBUG_INFO(doors, TXT("RoomWithDoorBlocks.doors"));
	}

	float calculate_at(int side, Vector2 const & _loc)
	{
		if (side == YP)
		{
			return _loc.x - block.x.min;
		}
		else if (side == YM)
		{
			return block.x.max - _loc.x;
		}
		else if (side == XP)
		{
			return _loc.y - block.y.min;
		}
		else //if (side == XM)
		{
			return block.y.max - _loc.y;
		}
	}

	Transform calculate_door_placement(int side, float at)
	{
		// outbound!
		if (side == YP)
		{
			return look_matrix(Vector3(block.x.min + at, block.y.max, 0.0f), -Vector3::yAxis, Vector3::zAxis).to_transform();
		}
		else if (side == YM)
		{
			return look_matrix(Vector3(block.x.max - at, block.y.min, 0.0f), Vector3::yAxis, Vector3::zAxis).to_transform();
		}
		else if (side == XP)
		{
			return look_matrix(Vector3(block.x.max, block.y.max - at, 0.0f), -Vector3::xAxis, Vector3::zAxis).to_transform();
		}
		else //if (side == XM)
		{
			return look_matrix(Vector3(block.x.min, block.y.min + at, 0.0f), Vector3::xAxis, Vector3::zAxis).to_transform();
		}
	}
};

static void fit_sides(Range & _side, float _size, float _firstTileOffset, float _tileSize, float _playAreaSize, Random::Generator & _rg, bool _alignToTiles)
{
	float purePlayAreaSize = _playAreaSize;

	// for _alignToTiles do calculations in 0 -> tileCount * tileSize (rounded playAreaSize) and then adjust by first tile offset
	if (_alignToTiles)
	{
		_playAreaSize = min(_playAreaSize, round_to(_playAreaSize, _tileSize));
	}

	_side.min = _rg.get_float(0.0f, _playAreaSize - _size);

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

	// to proper space
	_side.min -= purePlayAreaSize * 0.5f;
	_side.max -= purePlayAreaSize * 0.5f;
}

bool RoomWithDoorBlocks::generate(Framework::Game* _game, Framework::Room* _room, bool _subGenerator, REF_ Framework::RoomGeneratingContext& _roomGeneratingContext)
{
	bool result = true;
	RoomGenerationInfo roomGenerationInfo = RoomGenerationInfo::get();
	Random::Generator randomGenerator = _room->get_individual_random_generator();
	randomGenerator.advance_seed(978645, 689254);

#ifdef OUTPUT_GENERATION
	output(TXT("generating room with door blocks %S"), _room->get_individual_random_generator().get_seed_string().to_char());
#endif

	SimpleVariableStorage variables;
	_room->collect_variables(REF_ variables);
	info->apply_generation_parameters_to(variables);

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("room with door blocks"))));
	dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::blue));
	dv->activate();
#endif
	
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	dv->start_gathering_data();
	dv->clear();
	roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::black);
#endif

#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
	dv->end_gathering_data();
	dv->show_and_wait_for_key();
#endif

	Array<Framework::MeshNodePtr> meshNodes;

	{
		// place doors on rectangle - check for corridor corners to be able to fit 
		// check if it is small and if we have 4 doors - this is hardcoded solution
		bool alignToTiles = variables.get_value(NAME(alignToTiles), roomGenerationInfo.is_small()); // for small room always align to tiles, it gives more freedom
		Vector2 maxSize = variables.get_value(NAME(maxSize), Vector2::zero);
		Vector2 maxTileSize = variables.get_value(NAME(maxTileSize), Vector2::zero);
		Vector2 maxSizePct = variables.get_value(NAME(maxSizePct), Vector2::zero);
		Vector2 minSize = variables.get_value(NAME(minSize), Vector2::zero);
		Vector2 minTileSize = variables.get_value(NAME(minTileSize), Vector2::zero);
		Vector2 minSizePct = variables.get_value(NAME(minSizePct), Vector2::zero);
		Vector2 maxPlayAreaSize = variables.get_value(NAME(maxPlayAreaSize), Vector2::zero);
		Vector2 maxPlayAreaTileSize = variables.get_value(NAME(maxPlayAreaTileSize), Vector2::zero);
		int maxDoorPerBlock = variables.get_value(NAME(maxDoorPerBlock), (int)0);
		Vector2 maxBlockSize = variables.get_value(NAME(maxBlockSize), Vector2::zero);
		Vector2 maxBlockTileSize = variables.get_value(NAME(maxBlockTileSize), Vector2::zero);

		float const tileSize = roomGenerationInfo.get_tile_size();

		// auto params
		{
			Vector2 tempMaxSize = Vector2(99999.0f, 99999.0f);
			if (!maxPlayAreaTileSize.is_zero())
			{
				tempMaxSize.x = min(tempMaxSize.x, max(1.0f, maxPlayAreaTileSize.x) * tileSize);
				tempMaxSize.y = min(tempMaxSize.y, max(1.0f, maxPlayAreaTileSize.y) * tileSize);
			}
			if (!maxPlayAreaSize.is_zero())
			{
				tempMaxSize.x = min(maxPlayAreaSize.x, maxSize.x);
				tempMaxSize.y = min(maxPlayAreaSize.y, maxSize.y);
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
			Vector2 tempMinSize = Vector2(0.0f, 0.0f);
			if (!minTileSize.is_zero())
			{
				tempMinSize.x = max(tempMinSize.x, max(1.0f, minTileSize.x) * tileSize);
				tempMinSize.y = max(tempMinSize.y, max(1.0f, minTileSize.y) * tileSize);
			}
			if (!minSizePct.is_zero())
			{
				tempMinSize.x = max(tempMinSize.x, minSizePct.x * roomGenerationInfo.get_play_area_rect_size().x);
				tempMinSize.y = max(tempMinSize.y, minSizePct.y * roomGenerationInfo.get_play_area_rect_size().y);
			}
			if (!minSize.is_zero())
			{
				tempMinSize.x = max(tempMinSize.x, minSize.x);
				tempMinSize.y = max(tempMinSize.y, minSize.y);
			}
			minSize = tempMinSize;
		}

		if (maxBlockTileSize.is_zero() && maxBlockSize.is_zero())
		{
			maxBlockTileSize = Vector2::one;
		}

		if (!maxBlockTileSize.is_zero())
		{
			maxBlockSize.x = max(maxBlockTileSize.x * tileSize, maxBlockSize.x);
			maxBlockSize.y = max(maxBlockTileSize.y * tileSize, maxBlockSize.y);
		}
		maxBlockSize.x = max(maxBlockSize.x, tileSize);
		maxBlockSize.y = max(maxBlockSize.y, tileSize);

		bool ok = false;

		{
			// allocate here as allocating during each iteration will mess up the stack
			ARRAY_PREALLOC_SIZE(DoorBlock, blocks, _room->get_all_doors().get_size());
			ARRAY_PREALLOC_SIZE(Framework::DoorInRoom*, doorsLeft, _room->get_all_doors().get_size());
			ARRAY_PREALLOC_SIZE(int, blocksAvailable, _room->get_all_doors().get_size()); // no more than blocks

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
#endif

				int ignoreVRSpacePlacementAfterIdx = (triesLeft - 500) / 200;
				bool alignToTilesNow = alignToTiles;
				bool smallRoomFourDoors = roomGenerationInfo.is_small() && _room->get_all_doors().get_size() >= 4;
				
				if (triesLeft < 900)
				{
					alignToTilesNow = false;
				}

				Vector2 const playAreaSize = roomGenerationInfo.get_play_area_rect_size();
				Vector2 const firstTileOffset = roomGenerationInfo.get_tiles_zone_offset() - roomGenerationInfo.get_play_area_offset();
				Vector2 rectSize;
				if (smallRoomFourDoors)
				{
					rectSize.x = tileSize * 3.0f;
					rectSize.y = tileSize * 2.0f;
				}
				else
				{
					/*
					Vector2 const smallerPlayAreaSize(playAreaSize.x - tileSize * (randomGenerator.get_chance(0.5f) ? 1.0f : 0.0f),
						playAreaSize.y - tileSize * (randomGenerator.get_chance(0.5f) ? 1.0f : 0.0f));
					Vector2 const smallestPlayAreaSize(playAreaSize.x - tileSize * 2.0f,
						playAreaSize.y - tileSize * 2.0f);
					Vector2 const & usePlayAreaSize = lastResort ? playAreaSize : (stillOptionsLeft ? smallerPlayAreaSize : smallestPlayAreaSize);
					*/
					Vector2 const & usePlayAreaSize = playAreaSize;
					if (!minSize.is_zero())
					{
						rectSize.x = randomGenerator.get_float(minSize.x, usePlayAreaSize.x);
						rectSize.y = randomGenerator.get_float(minSize.y, usePlayAreaSize.y);
					}
					else
					{
						rectSize.x = randomGenerator.get_float(tileSize, usePlayAreaSize.x);
						rectSize.y = randomGenerator.get_float(tileSize, usePlayAreaSize.y);
					}
				}

				if (!maxSize.is_zero())
				{
					rectSize.x = min(rectSize.x, max(tileSize, maxSize.x));
					rectSize.y = min(rectSize.y, max(tileSize, maxSize.y));
				}

				if (alignToTilesNow)
				{
					rectSize.x = round_to(rectSize.x, tileSize);
					rectSize.y = round_to(rectSize.y, tileSize);
				}

				// sane
				rectSize.x = clamp(rectSize.x, tileSize, playAreaSize.x);
				rectSize.y = clamp(rectSize.y, tileSize, playAreaSize.y);

				// we work in <-psa/5, psa/5> space and fit sides takes care of that
				Range2 rect;
				fit_sides(rect.x, rectSize.x, firstTileOffset.x, tileSize, playAreaSize.x, randomGenerator, alignToTilesNow);
				fit_sides(rect.y, rectSize.y, firstTileOffset.y, tileSize, playAreaSize.y, randomGenerator, alignToTilesNow);

				#define GENEROUS_MARGIN 0.1f
				an_assert(rect.x.min >= -playAreaSize.x * 0.5f - GENEROUS_MARGIN);
				an_assert(rect.x.max <= playAreaSize.x * 0.5f + GENEROUS_MARGIN);
				an_assert(rect.y.min >= -playAreaSize.y * 0.5f - GENEROUS_MARGIN);
				an_assert(rect.y.max <= playAreaSize.y * 0.5f + GENEROUS_MARGIN);
				#undef GENEROUS_MARGIN

				// let's assume everything will go ok
				ok = true;

				blocks.clear(); // reuse
				doorsLeft.clear(); // reuse

				// this is the core
				// we add blocks first and try to put door onto them
				{
					// start with doors that have vr space placement set
					for_every_ptr(door, _room->get_all_doors())
					{
						if (! door->get_door()->is_important_vr_door())
						{
							if (auto* otherDoor = door->get_door_on_other_side())
							{
								if (otherDoor->is_vr_space_placement_set())
								{
									doorsLeft.push_back(door);
								}
							}
						}
					}
					for_every_ptr(door, _room->get_all_doors())
					{
						doorsLeft.push_back_unique(door);
					}
					for_every_ptr(door, doorsLeft)
					{
						float doorWidth = door->get_door()->calculate_vr_width();
						if (doorWidth > tileSize)
						{
							warn(TXT("door \"%S\" is wider %.3f than tile size %.3f"), door->get_door()->get_door_type()->get_name().to_string().to_char(), doorWidth, tileSize);
							doorWidth = tileSize;
						}
						if (! ok)
						{
							break;
						}
						// check if we have to add a block or not
						bool addBlock = false;
						if (blocks.is_empty())
						{
							addBlock = true;
						}
						else
						{
							bool doorsCanBePlacedInExistingBlock = false;
							for_every(block, blocks)
							{
								if (block->doors.get_size() < block->preferredDoorCount)
								{
									doorsCanBePlacedInExistingBlock = true;
								}
							}
							if (!doorsCanBePlacedInExistingBlock)
							{
								addBlock = true;
							}
						}

						bool useVRDoorPlacement = false;
						int useVRDoorPlacementBlockIdx = NONE;
						Optional<Transform> useVRDoorPlacementDoorPlacement;
						if (for_everys_index(door) <= ignoreVRSpacePlacementAfterIdx && randomGenerator.get_chance(0.7f))
						{
							if (!door->get_door()->is_important_vr_door())
							{
								if (auto* otherDoor = door->get_door_on_other_side())
								{
									if (otherDoor->is_vr_space_placement_set())
									{
										useVRDoorPlacement = true;
									}
								}
							}
						}

						addBlock |= useVRDoorPlacement;

						if (addBlock)
						{
							Vector2 blockSize;
							blockSize.x = randomGenerator.get_float(tileSize, maxBlockSize.x);
							blockSize.y = randomGenerator.get_float(tileSize, maxBlockSize.y);
							if (alignToTilesNow)
							{
								blockSize.x = round_to(blockSize.x, tileSize);
								blockSize.y = round_to(blockSize.y, tileSize);
							}

							Vector2 blockLeftBottomCorner;
							blockLeftBottomCorner.x = randomGenerator.get_float(rect.x.min, rect.x.max - blockSize.x);
							blockLeftBottomCorner.y = randomGenerator.get_float(rect.y.min, rect.y.max - blockSize.y);

							if (alignToTilesNow)
							{
								blockLeftBottomCorner.x = round_to(blockLeftBottomCorner.x, tileSize);
								blockLeftBottomCorner.y = round_to(blockLeftBottomCorner.y, tileSize);
							}

							Range2 blockAt;
							blockAt.x.min = blockLeftBottomCorner.x;
							blockAt.x.max = blockLeftBottomCorner.x + blockSize.x;
							blockAt.y.min = blockLeftBottomCorner.y;
							blockAt.y.max = blockLeftBottomCorner.y + blockSize.y;

							if (smallRoomFourDoors)
							{
								blockSize = Vector2(tileSize, tileSize * 2.0f);
								blockLeftBottomCorner.y = 0.0f;
								if (blocks.get_size() == 0)
								{
									blockLeftBottomCorner.x = 0.0f;
								}
								else
								{
									blockLeftBottomCorner.x = tileSize * 2.0f;
								}
							}
							else if (useVRDoorPlacement)
							{
								if (!door->get_door()->is_important_vr_door())
								{
									if (auto* otherDoor = door->get_door_on_other_side())
									{
										if (otherDoor->is_vr_space_placement_set())
										{
											Transform odVRPlacement = otherDoor->get_vr_space_placement();
											Vector2 odLoc = odVRPlacement.get_translation().to_vector2();
											Vector2 odFwd = odVRPlacement.get_axis(Axis::Y).to_vector2();

											// odFwd it outbound for other door, this is inbound for our door
											// can be used to align block
											if (abs(odFwd.x) < MARGIN || abs(odFwd.y) < MARGIN)
											{
												int side = XP;
												if (odFwd.x > MARGIN) side = XP;
												if (odFwd.x < -MARGIN) side = XM;
												if (odFwd.y > MARGIN) side = YP;
												if (odFwd.y < -MARGIN) side = YM;
												useVRDoorPlacementBlockIdx = side;
												useVRDoorPlacementDoorPlacement = look_matrix(odLoc.to_vector3(), -odFwd.to_vector3(), Vector3::zAxis).to_transform();

												// get sides dependent on doors
												switch (side)
												{
												case XP:
													blockAt.x.min = odLoc.x - blockAt.x.length();
													blockAt.x.max = odLoc.x;
													break;
												case XM:
													blockAt.x.max = odLoc.x + blockAt.x.length();
													blockAt.x.min = odLoc.x;
													break;
												case YP:
													blockAt.y.min = odLoc.y - blockAt.y.length();
													blockAt.y.max = odLoc.y;
													break;
												case YM:
													blockAt.y.max = odLoc.y + blockAt.y.length();
													blockAt.y.min = odLoc.y;
													break;
												}

												// get other axis to contain door
												switch (side)
												{
												case XP:
												case XM:
													{
														float at = randomGenerator.get_float(doorWidth * 0.5f, blockAt.y.length() - doorWidth * 0.5f);
														blockAt.y.max = odLoc.y + blockAt.y.length() - at;
														blockAt.y.min = odLoc.y - at;
														if (!blocks.is_empty())
														{
															if (blockAt.y.min < rect.y.min)
															{
																blockAt.y.max += rect.y.min - blockAt.y.min;
																blockAt.y.min += rect.y.min - blockAt.y.min;
															}
															if (blockAt.y.max > rect.y.max)
															{
																blockAt.y.min += rect.y.max - blockAt.y.max;
																blockAt.y.max += rect.y.max - blockAt.y.max;
															}
														}
													}
													break;
												case YP:
												case YM:
													{
														float at = randomGenerator.get_float(doorWidth * 0.5f, blockAt.x.length() - doorWidth * 0.5f);
														blockAt.x.max = odLoc.x + blockAt.x.length() - at;
														blockAt.x.min = odLoc.x - at;
														if (!blocks.is_empty())
														{
															if (blockAt.x.min < rect.x.min)
															{
																blockAt.x.max += rect.x.min - blockAt.x.min;
																blockAt.x.min += rect.x.min - blockAt.x.min;
															}
															if (blockAt.x.max > rect.x.max)
															{
																blockAt.x.min += rect.x.max - blockAt.x.max;
																blockAt.x.max += rect.x.max - blockAt.x.max;
															}
														}
													}
													break;
												}

												Range2 rectExpanded = rect;
												rectExpanded.expand_by(Vector2(MARGIN, MARGIN));
												if (blocks.is_empty())
												{
													if (!rectExpanded.does_contain(blockAt))
													{
														// move to contain blockAt
														if (rect.y.max < blockAt.y.max)
														{
															rect.y.min += blockAt.y.max - rect.y.max;
															rect.y.max += blockAt.y.max - rect.y.max;
														}
														if (rect.y.min > blockAt.y.min)
														{
															rect.y.max += blockAt.y.min - rect.y.min;
															rect.y.min += blockAt.y.min - rect.y.min;
														}
														if (rect.x.max < blockAt.x.max)
														{
															rect.x.min += blockAt.x.max - rect.x.max;
															rect.x.max += blockAt.x.max - rect.x.max;
														}
														if (rect.x.min > blockAt.x.min)
														{
															rect.x.max += blockAt.x.min - rect.x.min;
															rect.x.min += blockAt.x.min - rect.x.min;
														}
													}
												}
												else
												{
													if (!rectExpanded.does_contain(blockAt))
													{
														// redo and hope for a block that fits in rect
														ok = false;
													}
												}
											}
										}
									}
								}
							}
						
							for_every(block, blocks)
							{
								if (block->block.intersects_extended(blockAt, tileSize * 0.5f))
								{
									ok = false;
									break;
								}
							}

							if (!ok)
							{
								// couldn't fit block to not overlap
								continue;
							}

							DoorBlock block;
							block.block = blockAt;
							if (smallRoomFourDoors)
							{
								block.preferredDoorCount = 2;
							}
							else if (maxDoorPerBlock > 0)
							{
								block.preferredDoorCount = randomGenerator.get_int_from_range(1, maxDoorPerBlock);
							}
							else
							{
								block.preferredDoorCount = 99999;
							}

							block.doorsAvailable[XM] = 0;
							block.doorsAvailable[XP] = 0;
							block.doorsAvailable[YM] = 0;
							block.doorsAvailable[YP] = 0;
							float const treshold = 0.1f; // we can be short of 10cm and it still should be fine
							if (blockAt.x.min >= rect.x.min + tileSize - treshold)
							{
								block.doorsAvailable[XM] = (int)(floor(blockAt.y.length() / tileSize + MARGIN));
							}
							if (blockAt.x.max <= rect.x.max - tileSize + treshold)
							{
								block.doorsAvailable[XP] = (int)(floor(blockAt.y.length() / tileSize + MARGIN));
							}
							if (blockAt.y.min >= rect.y.min + tileSize - treshold)
							{
								block.doorsAvailable[YM] = (int)(floor(blockAt.x.length() / tileSize + MARGIN));
							}
							if (blockAt.y.max <= rect.y.max - tileSize + treshold)
							{
								block.doorsAvailable[YP] = (int)(floor(blockAt.x.length() / tileSize + MARGIN));
							}
							block.preferredDoorCount = min(block.preferredDoorCount, block.doorsAvailable[XP] + block.doorsAvailable[XM] + block.doorsAvailable[YP] + block.doorsAvailable[YM]);

							block.zone.be_rect(block.block.length(), block.block.centre());

							blocks.push_back(block);
						}

						blocksAvailable.clear(); // reuse
						for_every(block, blocks)
						{
							if (block->doorsAvailable[XP] + block->doorsAvailable[XM] + block->doorsAvailable[YP] + block->doorsAvailable[YM] > 0)
							{
								blocksAvailable.push_back(for_everys_index(block));
							}
						}

						if (blocksAvailable.is_empty())
						{
							ok = false;
							continue;
						}

	#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
						dv->clear();
						roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::black);
						dv->add_range2(rect, Colour::blue);
						for_every(block, blocks)
						{
							block->zone.debug_visualise(dv.get(), Colour::c64Brown);
						}
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
						dv->start_gathering_data();
						dv->clear();
	#endif

						bool placed = false;
						int placeDoorTriesLeft = 50;
						while ((placeDoorTriesLeft--) > 0)
						{
							int blockIdx = randomGenerator.get_int(blocksAvailable.get_size());

							auto& block = blocks[blockIdx];

							ArrayStatic<int, 4> sidesAvailable; SET_EXTRA_DEBUG_INFO(sidesAvailable, TXT("RoomWithDoorBlocks.sidesAvailable"));
							for_count(int, sideIdx, 4)
							{
								if (block.doorsAvailable[sideIdx])
								{
									sidesAvailable.push_back(sideIdx);
								}
							}

							int sideIdx = useVRDoorPlacementBlockIdx != NONE? useVRDoorPlacementBlockIdx :
										 (! sidesAvailable.is_empty()? sidesAvailable[randomGenerator.get_int(sidesAvailable.get_size())] : NONE);

							if (sideIdx < 0 || sideIdx >= 4)
							{
								// try again and get a valid side index
								continue;
							}

							if (!block.doorsAvailable[sideIdx])
							{
								if (useVRDoorPlacementBlockIdx != NONE)
								{
									// we won't make it this way!
									// try again using random side
									useVRDoorPlacementBlockIdx = NONE;
								}
								// we should fail if we try to use not available side
								continue;
							}

							float sidesSize[] = { block.block.x.length(), block.block.y.length(), block.block.x.length(), block.block.y.length() };

							float at;
							Transform doorPlacement;
							if (useVRDoorPlacementBlockIdx != NONE && useVRDoorPlacementDoorPlacement.is_set())
							{
								doorPlacement = useVRDoorPlacementDoorPlacement.get();
								at = block.calculate_at(sideIdx, doorPlacement.get_translation().to_vector2());

								{
									bool isOkNow = true;
									for_count(int, againstDoorsTries, 10)
									{
										isOkNow = true;

										for_every(dinb, block.doors)
										{
											if (dinb->side == sideIdx)
											{
												if (at + doorWidth * 0.5f > dinb->at - dinb->doorWidth * 0.5f &&
													at + doorWidth * 0.5f < dinb->at + MARGIN)
												{
													isOkNow = false;
													at = dinb->at - (dinb->doorWidth * 0.5f + doorWidth * 0.5f);
												}
												if (at - doorWidth * 0.5f < dinb->at + dinb->doorWidth * 0.5f &&
													at - doorWidth * 0.5f > dinb->at - MARGIN)
												{
													isOkNow = false;
													at = dinb->at + (dinb->doorWidth * 0.5f + doorWidth * 0.5f);
												}
											}
										}

										if (isOkNow)
										{
											break;
										}
									}

									if (!isOkNow)
									{
										// overlaps
										continue;
									}
								}

								if (at < doorWidth * 0.5f - MARGIN || at > sidesSize[sideIdx] - doorWidth * 0.5f + MARGIN)
								{
									// not placed on a side, outside!
									continue;
								}
							}

							if (useVRDoorPlacementBlockIdx == NONE)
							{
								at = randomGenerator.get_float(doorWidth * 0.5f, sidesSize[sideIdx] - doorWidth * 0.5f);

								if (alignToTilesNow)
								{
									at = round_to(at - tileSize * 0.5f, tileSize) + tileSize * 0.5f;
								}

								{	// check if overlaps against other doors
									bool isOkNow = false;
									for_count(int, againstDoorsTries, 10)
									{
										isOkNow = true;

										if (alignToTilesNow)
										{
											at = round_to(at - tileSize * 0.5f, tileSize) + tileSize * 0.5f;
										}

										for_every(dinb, block.doors)
										{
											if (dinb->side == sideIdx)
											{
												if (at + doorWidth * 0.5f > dinb->at - dinb->doorWidth * 0.5f &&
													at + doorWidth * 0.5f < dinb->at + MARGIN)
												{
													isOkNow = false;
													at = dinb->at - (dinb->doorWidth * 0.5f + doorWidth * 0.5f);
												}
												if (at - doorWidth * 0.5f < dinb->at + dinb->doorWidth * 0.5f &&
													at - doorWidth * 0.5f > dinb->at - MARGIN)
												{
													isOkNow = false;
													at = dinb->at + (dinb->doorWidth * 0.5f + doorWidth * 0.5f);
												}
											}
										}

										if (isOkNow)
										{
											break;
										}
									}

									if (!isOkNow)
									{
										// overlaps
										continue;
									}
								}


								if (at < doorWidth * 0.5f -MARGIN || at > sidesSize[sideIdx] - doorWidth * 0.5f + MARGIN)
								{
									// not placed on a side, outside!
									continue;
								}

								doorPlacement = block.calculate_door_placement(sideIdx, at);
							}

							VR::Zone doorZone;
							doorZone.be_rect(Vector2(doorWidth * 0.95f, doorWidth * 0.95f), Vector2(0.0f, -doorWidth * 0.5f)); // inbound
							doorZone = doorZone.to_world_of(doorPlacement);
						
							{	// check if door overlaps with a block
								bool isOkNow = true;
								for_every(oBlock, blocks)
								{
									if (doorZone.does_intersect_with(oBlock->zone))
									{
										isOkNow = false;
										break;
									}
								}

								if (!isOkNow)
								{
									// will overlap with an other block
									continue;
								}
							}

							an_assert(roomGenerationInfo.get_play_area_zone().does_contain(doorPlacement.get_translation().to_vector2(), doorWidth * 0.4f));

							DoorInDoorBlock dinb;
							dinb.dir = door;
							dinb.doorWidth = doorWidth;
							dinb.at = at;
							dinb.side = sideIdx;
							dinb.placement = doorPlacement;
							dinb.zone = doorZone;

							block.doors.push_back(dinb);
							--block.doorsAvailable[sideIdx];
							an_assert(block.doorsAvailable[sideIdx] >= 0);

	#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
							dv->clear();
							roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::black);
							dv->add_range2(rect, Colour::blue);
							for_every(block, blocks)
							{
								block->zone.debug_visualise(dv.get(), Colour::c64Brown);
							}
							Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, door, doorPlacement.get_translation().to_vector2(), doorPlacement.get_axis(Axis::Forward).to_vector2(), doorWidth, Colour::green);
							dv->end_gathering_data();
							dv->show_and_wait_for_key();
							dv->start_gathering_data();
							dv->clear();
	#endif

							// success!
							placed = true;
							break;
						}

						if (!placed)
						{
							ok = false;
						}
					}
				}

				// if something doesn't feel right, start all over again
				if (!ok)
				{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
					dv->end_gathering_data();
#endif
					// start again
					continue;
				}

				//OK//
				
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				dv->clear();
				roomGenerationInfo.get_play_area_zone().debug_visualise(dv.get(), Colour::black);
				dv->add_range2(rect, Colour::blue);
#endif

				// place actual doors now
				// place doors
				for_every(block, blocks)
				{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
					block->zone.debug_visualise(dv.get(), Colour::c64Brown);
#endif
					for_every(door, block->doors)
					{
						if (!door->dir->is_vr_space_placement_set() ||
							!Framework::DoorInRoom::same_with_orientation_for_vr(door->dir->get_vr_space_placement(), door->placement))
						{
							if (door->dir->is_vr_placement_immovable())
							{
								door->dir = door->dir->grow_into_vr_corridor(NP, door->placement, roomGenerationInfo.get_play_area_zone(), roomGenerationInfo.get_tile_size());
							}
							an_assert(roomGenerationInfo.get_play_area_zone().does_contain(door->placement.get_translation().to_vector2(), door->doorWidth * 0.4f));
							door->dir->set_vr_space_placement(door->placement);
							door->dir->set_placement(door->placement);
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
							Framework::DebugVisualiserUtils::add_door_to_debug_visualiser(dv, door->dir, door->placement.get_translation().to_vector2(), door->placement.get_axis(Axis::Forward).to_vector2(), door->doorWidth, Colour::green);
#endif
						}
						else
						{
							// make sure it is placed in the room as well
							door->dir->set_placement(door->placement);
						}
					}

					Framework::MeshNode* meshNode = new Framework::MeshNode();
					meshNode->name = NAME(doorBlock);
					int turn90 = randomGenerator.get_int(4);
					meshNode->placement = Transform(block->block.centre().to_vector3(), Rotator3(0.0f, (float)turn90 * 90.0f, 0.0f).to_quat());
					Vector2 doorBlockSize = block->block.length();
					if (turn90 % 2)
					{
						swap(doorBlockSize.x, doorBlockSize.y);
					}
					meshNode->variables.access<Vector2>(NAME(doorBlockSize)) = doorBlockSize;

					meshNodes.push_back(Framework::MeshNodePtr(meshNode));
				}

				{
					Framework::MeshNode* meshNode = new Framework::MeshNode();
					meshNode->name = NAME(room);
					int turn90 = randomGenerator.get_int(4);
					meshNode->placement = Transform(rect.centre().to_vector3(), Rotator3(0.0f, (float)turn90 * 90.0f, 0.0f).to_quat());
					Vector2 roomSize = rect.length();
					if (turn90 % 2)
					{
						swap(roomSize.x, roomSize.y);
					}
					meshNode->variables.access<Vector2>(NAME(roomSize)) = roomSize;

					meshNodes.push_back(Framework::MeshNodePtr(meshNode));
				}
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
				dv->end_gathering_data();
#ifndef OUTPUT_GENERATION_DEBUG_VISUALISER_FAILS_ONLY
				dv->show_and_wait_for_key();
#endif
#endif
			}
		}

		if (!ok)
		{
			error(TXT("could not generate room with door blocks"));
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
					.using_mesh_nodes(meshNodes)
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
			error(TXT("could not generate room with door blocks mesh"));
			return false;
		}
	}

#ifdef OUTPUT_GENERATION
	output(TXT("generated room with door blocks"));
#endif

	if (!_subGenerator)
	{
		_room->place_pending_doors_on_pois();
		_room->mark_vr_arranged();
		_room->mark_mesh_generated();
	}

	return result;
}

#undef YP
#undef XP
#undef YM
#undef XM
