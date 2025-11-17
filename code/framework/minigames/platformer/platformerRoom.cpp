#include "platformerRoom.h"

#include "platformerInfo.h"
#include "platformerTile.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\video\texturePart.h"

#ifdef AN_MINIGAME_PLATFORMER

using namespace Platformer;

REGISTER_FOR_FAST_CAST(Room);

LIBRARY_STORED_DEFINE_TYPE(Room, minigamePlatformerRoom);

Room::Room(::Framework::Library * _library, ::Framework::LibraryName const & _name)
: base(_library, _name)
{
}

Room::~Room()
{
}

Room::Door * Room::find_door_at(VectorInt2 const & _at)
{
	for_every(door, doors)
	{
		if (_at.x >= door->rect.x.min &&
			_at.x <= door->rect.x.max &&
			_at.y >= door->rect.y.min &&
			_at.y <= door->rect.y.max)
		{
			return door;
		}
	}
	return nullptr;
}

static int char_to_index(tchar c)
{
	int index = NONE;
	if (c >= '0' && c <= '9')
	{
		index = c - '0';
	}
	if (c >= 'A' && c <= 'Z')
	{
		index = c - 'A';
	}
	if (c >= 'a' && c <= 'z')
	{
		index = c - 'a';
	}
	return index;
}

bool Room::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= roomName.load_from_xml_child_required(_node, TXT("roomName"), _lc, TXT("room name"));

	borderColour.load_from_xml_child_node(_node, TXT("borderColour"));
	backgroundColour.load_from_xml_child_node(_node, TXT("backgroundColour"));
	foregroundColour.load_from_xml_child_node(_node, TXT("foregroundColour"));
	brightnessIndicator = _node->first_child_named(TXT("brightColour")) ? Colour::white : brightnessIndicator;
	brightnessIndicator = _node->first_child_named(TXT("noBrightColour")) ? Colour::black : brightnessIndicator;
	brightnessIndicator = _node->first_child_named(TXT("notBrightColour")) ? Colour::black : brightnessIndicator;
	brightnessIndicator = _node->first_child_named(TXT("dimmColour")) ? Colour::black : brightnessIndicator;

	for_every(containerNode, _node->children_named(TXT("tileTypes")))
	{
		for_every(node, containerNode->children_named(TXT("tile")))
		{
			TileRef tt;
			String ttCh = node->get_string_attribute(TXT("char"), String(TXT("#")));
			tt.ch = ttCh.get_data().get_first();
			if (tt.ch == 0 ||
				tt.ch == '.' ||
				tt.ch == ' ' ||
				tt.ch == '@' ||
				tt.ch == '%')
			{
				error_loading_xml(node, TXT("invalid char for a tile"));
				result = false;
			}
			else
			{
				result &= tt.tile.load_from_xml(node, TXT("use"), _lc);
			}
			tileRefs.push_back(tt);
		}
	}

	size.load_from_xml_child_node(_node, TXT("size"));

	tiles.set_size(size.x * size.y);
	for_every(tile, tiles)
	{
		*tile = NONE;
	}

	int rowIdx = size.y - 1;
	for_every(containerNode, _node->children_named(TXT("room")))
	{
		for_every(node, containerNode->children_named(TXT("row")))
		{
			if (rowIdx < 0)
			{
				error_loading_xml(node, TXT("bigger than room"));
				result = false;
			}
			String row = node->get_text();
			int colIdx = 0;
			int doorStartedAtCol = NONE;
			int guardianStartedAtCol = NONE;
			VectorInt2 guardianStartingDir = VectorInt2::zero;
			for_every(ch, row.get_data())
			{
				if (*ch == 0)
				{
					// we've reached end
					break;
				}
				if (colIdx >= size.x)
				{
					error_loading_xml(node, TXT("bigger than room"));
					result = false;
				}
				tchar c = *ch;
				if (doorStartedAtCol != NONE)
				{
					if (c != '@')
					{
						int doorIndex = char_to_index(c);
						if (doorIndex >= 0)
						{
							while (doors.get_size() <= doorIndex)
							{
								doors.push_back(Door());
							}
							Door & door = doors[doorIndex];
							door.rect.include(RangeInt2(RangeInt(doorStartedAtCol, colIdx), RangeInt(rowIdx, rowIdx)));
						}
						else
						{
							error_loading_xml(node, TXT("door invalid"));
							result = false;
						}
						doorStartedAtCol = NONE;
					}
					c = '.';
				}
				else if (c == '@')
				{
					doorStartedAtCol = colIdx;
					c = '.';
				}
				if (guardianStartedAtCol != NONE)
				{
					if (c != '%')
					{
						int guardianIndex = char_to_index(c);
						if (c == '*')
						{
							guardianStartingDir = VectorInt2(0, 0);
						}
						else if (c == '}')
						{
							guardianStartingDir = VectorInt2(1, 0);
						}
						else if (c == '{')
						{
							guardianStartingDir = VectorInt2(-1, 0);
						}
						else if (c == '^')
						{
							guardianStartingDir = VectorInt2(0, 1);
						}
						else if (c == 'v')
						{
							guardianStartingDir = VectorInt2(0, -1);
						}
						else if (guardianIndex >= 0)
						{
							while (guardians.get_size() <= guardianIndex)
							{
								guardians.push_back(Guardian());
							}
							Guardian & guardian = guardians[guardianIndex];
							guardian.rect.include(RangeInt2(RangeInt(guardianStartedAtCol, colIdx), RangeInt(rowIdx, rowIdx)));
							guardian.startingDir = guardianStartingDir;
							if (guardianStartingDir.x > 0)
							{
								guardian.startingLoc.x = guardianStartedAtCol;
							}
							else if (guardianStartingDir.x < 0)
							{
								guardian.startingLoc.x = colIdx + 1;
							}
							guardian.startingLoc.y = rowIdx;
							guardianStartedAtCol = NONE;
							guardianStartingDir = VectorInt2::zero;
						}
						else
						{
							error_loading_xml(node, TXT("guardian invalid"));
							result = false;
							guardianStartedAtCol = NONE;
							guardianStartingDir = VectorInt2::zero;
						}
					}
					c = '.';
				}
				else if (c == '%')
				{
					guardianStartedAtCol = colIdx;
					c = '.';
				}
				int tileType = find_tile_by_char(c);
				tiles[rowIdx * size.x + colIdx] = tileType;
				++colIdx;
			}
			--rowIdx;
		}
	}
	for_every(containerNode, _node->children_named(TXT("doors")))
	{
		for_every(node, containerNode->children_named(TXT("door")))
		{
			bool autoDoor = false;
			int doorIdx = node->get_int_attribute(TXT("index"), NONE);
			if (!node->has_attribute(TXT("index")))
			{
				autoDoor = true;
				doorIdx = doors.get_size();
				doors.push_back(Door());
			}
			if (doors.is_index_valid(doorIdx))
			{
				result &= doors[doorIdx].connectsTo.load_from_xml(node, TXT("toRoom"), _lc);
				doors[doorIdx].tile.load_from_xml(node, TXT("tile"), _lc);
				doors[doorIdx].connectsToDoor = node->get_int_attribute(TXT("toDoorIndex"), NONE);
				if (doors[doorIdx].connectsToDoor == NONE && ! autoDoor)
				{
					error_loading_xml(node, TXT("missing to door index"));
					result = false;
				}
				if (IO::XML::Attribute const * attr = node->get_attribute(TXT("dir")))
				{
					if (attr->get_as_string() == TXT("}") ||
						attr->get_as_string() == TXT("right"))
					{
						doors[doorIdx].dir = DoorDir::Right;
					}
					else if (attr->get_as_string() == TXT("{") ||
						attr->get_as_string() == TXT("left"))
					{
						doors[doorIdx].dir = DoorDir::Left;
					}
					else if (attr->get_as_string() == TXT("v") ||
						attr->get_as_string() == TXT("down"))
					{
						doors[doorIdx].dir = DoorDir::Down;
					}
					else if (attr->get_as_string() == TXT("^") ||
						attr->get_as_string() == TXT("up"))
					{
						doors[doorIdx].dir = DoorDir::Up;
					}
					else
					{
						error_loading_xml(node, TXT("invalid door dir for door index %i"), doorIdx);
						result = false;
					}
				}
			}
			else
			{
				error_loading_xml(node, TXT("invalid door index %i"), doorIdx);
				result = false;
			}
			if (autoDoor)
			{
				Door & door = doors[doorIdx];
				door.autoDoor = true;
				if (door.dir == DoorDir::Left)
				{
					door.rect.x.min = 0;
					door.rect.x.max = 1;
					door.rect.y.min = 0;
					door.rect.y.max = size.y - 1;
				}
				if (door.dir == DoorDir::Right)
				{
					door.rect.x.min = size.x - 2;
					door.rect.x.max = size.x - 1;
					door.rect.y.min = 0;
					door.rect.y.max = size.y - 1;
				}
				if (door.dir == DoorDir::Down)
				{
					door.rect.x.min = 0;
					door.rect.x.max = size.x - 1;
					door.rect.y.min = 0;
					door.rect.y.max = 1;
				}
				if (door.dir == DoorDir::Up)
				{
					door.rect.x.min = 0;
					door.rect.x.max = size.x - 1;
					door.rect.y.min = size.y - 2;
					door.rect.y.max = size.y - 1;
				}
			}
		}
	}

	for_every(containerNode, _node->children_named(TXT("guardians")))
	{
		for_every(node, containerNode->children_named(TXT("guardian")))
		{
			int guardianIdx = node->get_int_attribute(TXT("index"), NONE);
			if (!node->has_attribute(TXT("index")))
			{
				guardianIdx = guardians.get_size();
				guardians.push_back(Guardian());
			}
			if (guardians.is_index_valid(guardianIdx))
			{
				Guardian & guardian = guardians[guardianIdx];
				result &= guardian.character.load_from_xml(node, TXT("character"), _lc);
				if (IO::XML::Attribute const * attr = node->get_attribute(TXT("startingDir")))
				{
					if (attr->get_as_string() == TXT("*"))
					{
						guardian.startingDir = VectorInt2(0, 0);
					}
					else if (attr->get_as_string() == TXT("}") ||
						attr->get_as_string() == TXT("right"))
					{
						guardian.startingDir = VectorInt2(1, 0);
					}
					else if (attr->get_as_string() == TXT("{") ||
							 attr->get_as_string() == TXT("left"))
					{
						guardian.startingDir = VectorInt2(-1, 0);
					}
					else if (attr->get_as_string() == TXT("v") ||
							 attr->get_as_string() == TXT("down"))
					{
						guardian.startingDir = VectorInt2(0, 1);
					}
					else if (attr->get_as_string() == TXT("^") ||
							 attr->get_as_string() == TXT("up"))
					{
						guardian.startingDir = VectorInt2(0, -1);
					}
					else
					{
						error_loading_xml(node, TXT("invalid guardian dir for guardian index %i"), guardianIdx);
						result = false;
					}
				}
				if (IO::XML::Attribute const * attr = node->get_attribute(TXT("x")))
				{
					RangeInt xOv = RangeInt::empty;
					xOv.load_from_string(attr->get_as_string());
					guardian.rect.x = xOv;
				}
				if (IO::XML::Attribute const * attr = node->get_attribute(TXT("y")))
				{
					RangeInt yOv = RangeInt::empty;
					yOv.load_from_string(attr->get_as_string());
					guardian.rect.y = yOv;
				}
				guardian.startingLoc.load_from_xml(node, TXT("atX"), TXT("atY"));
				guardian.startingLoc.x = clamp(guardian.startingLoc.x, guardian.rect.x.min, guardian.rect.x.max);
				guardian.startingLoc.y = clamp(guardian.startingLoc.y, guardian.rect.y.min, guardian.rect.y.max);
			}
			else
			{
				error_loading_xml(node, TXT("invalid guardian index %i"), guardianIdx);
				result = false;
			}
		}
	}

	return result;
}

int Room::find_tile_by_char(tchar _ch) const
{
	if (_ch == '.' || _ch == ' ')
	{
		return NONE;
	}
	for_every(tile, tileRefs)
	{
		if (tile->ch == _ch)
		{
			return for_everys_index(tile);
		}
	}
	error(TXT("tile %c not recognised"), _ch);
	return NONE;
}

bool Room::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(door, doors)
	{
		if (door->dir == 0)
		{
			error(TXT("door dir (index %i) not specified!"), for_everys_index(door));
			result = false;
		}
		if (door->connectsTo.is_name_valid())
		{
			result &= door->connectsTo.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

			if (door->autoDoor &&
				door->connectsTo.get())
			{
				DoorDir::Type oppDir = DoorDir::get_opposite(door->dir);
				for_every(otherDoor, door->connectsTo->doors)
				{
					if (otherDoor->dir == oppDir)
					{
						door->connectsToDoor = for_everys_index(otherDoor);
					}
				}
			}
		}
		if (door->tile.is_name_valid())
		{
			result &= door->tile.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
		}
	}

	for_every(tileRef, tileRefs)
	{
		result &= tileRef->tile.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	}

	for_every(guardian, guardians)
	{
		result &= guardian->character.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, ::Framework::LibraryPrepareLevel::Resolve)
	{
		itemsAt.clear();
		for (int y = 0; y <= size.y; ++y)
		{
			for (int x = 0; x <= size.x; ++x)
			{
				if (Tile const * t = get_tile_at(VectorInt2(x, y)))
				{
					if (t->isItem)
					{
						itemsAt.push_back(VectorInt2(x, y));
					}
				}
			}
		}
	}

	return result;
}

void Room::prepare_to_unload()
{
	for_every(door, doors)
	{
		door->connectsTo.set(nullptr);
	}
}

void Room::get_items_at(Info* _info, RangeInt2 const & rect, OUT_ ArrayStack<int> & _itemsIdx) const
{
	_itemsIdx.clear();
	VectorInt2 tileMin = VectorInt2(rect.x.min / _info->tileSize.x, rect.y.min / _info->tileSize.y);
	VectorInt2 tileMax = VectorInt2(rect.x.max / _info->tileSize.x, rect.y.max / _info->tileSize.y);
	for (int x = tileMin.x; x <= tileMax.x; ++x)
	{
		for (int y = tileMin.y; y <= tileMax.y; ++y)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->isItem)
				{
					for_every(itemAt, itemsAt)
					{
						if (itemAt->x == x && itemAt->y == y)
						{
							_itemsIdx.push_back(for_everys_index(itemAt));
							break;
						}
					}
				}
			}
		}
	}
}

bool Room::does_kill(Info* _info, RangeInt2 const & rect) const
{
	VectorInt2 tileMin = VectorInt2(rect.x.min / _info->tileSize.x, rect.y.min / _info->tileSize.y);
	VectorInt2 tileMax = VectorInt2(rect.x.max / _info->tileSize.x, rect.y.max / _info->tileSize.y);

	for (int x = tileMin.x; x <= tileMax.x; ++x)
	{
		for (int y = tileMin.y; y <= tileMax.y; ++y)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->kills)
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Room::is_location_valid(Info* _info, RangeInt2 const & rect, RangeInt2 const & _ignore) const
{
	VectorInt2 tileMin = VectorInt2(rect.x.min / _info->tileSize.x, rect.y.min / _info->tileSize.y);
	VectorInt2 tileMax = VectorInt2(rect.x.max / _info->tileSize.x, rect.y.max / _info->tileSize.y);

	VectorInt2 ignoreTileMin = VectorInt2(_ignore.x.min / _info->tileSize.x, _ignore.y.min / _info->tileSize.y);
	VectorInt2 ignoreTileMax = VectorInt2(_ignore.x.max / _info->tileSize.x, _ignore.y.max / _info->tileSize.y);

	for (int x = tileMin.x; x <= tileMax.x; ++x)
	{
		for (int y = tileMin.y; y <= tileMax.y; ++y)
		{
			if (!_ignore.is_empty())
			{
				if (x >= ignoreTileMin.x && x <= ignoreTileMax.x &&
					y >= ignoreTileMin.y && y <= ignoreTileMax.y)
				{
					continue;
				}
			}
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->blocks)
				{
					return false;
				}
			}
		}
	}
	return true;
}

bool Room::has_something_under_feet(Info* _info, RangeInt2 const & rect, VectorInt2 const & _at, VectorInt2 const & _offsetForStairs, RangeInt const & _feetForStairs, int _step) const
{
	VectorInt2 at = _at + _offsetForStairs;
	// rect should include point we step onto
	if (mod(_at.y, _info->tileSize.y) == 0)
	{
		VectorInt2 tileMin = VectorInt2(rect.x.min / _info->tileSize.x, rect.y.min / _info->tileSize.y);
		VectorInt2 tileMax = VectorInt2(rect.x.max / _info->tileSize.x, rect.y.max / _info->tileSize.y);
		for (int y = tileMax.y; y >= tileMin.y; --y)
		{
			for (int x = tileMin.x; x <= tileMax.x; ++x)
			{
				if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
				{
					if (!tt->stairsUp)
					{
						if (tt->canStepOnto)
						{
							return true;
						}
					}
				}
			}
		}
	}
	VectorInt2 tileStepsMin = VectorInt2((at.x + _feetForStairs.min) / _info->tileSize.x, rect.y.min / _info->tileSize.y);
	VectorInt2 tileStepsMax = VectorInt2((at.x + _feetForStairs.max) / _info->tileSize.x, rect.y.max / _info->tileSize.y);
	for (int y = tileStepsMax.y; y >= tileStepsMin.y; --y)
	{
		for (int x = tileStepsMin.x; x <= tileStepsMax.x; ++x)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->stairsUp)
				{
					RangeInt checkRange(at.x + _feetForStairs.min, at.x + _feetForStairs.max);
					checkRange.min = max(checkRange.min, x * _info->tileSize.x);
					checkRange.max = min(checkRange.max, (x + 1) * _info->tileSize.x - 1);
					for (int subx = checkRange.min; subx <= checkRange.max; ++subx)
					{
						int stairsAt = tt->calc_stairs_at(subx - x * _info->tileSize.x, _step, _info->tileSize.x);
						if (at.y - y * _info->tileSize.y <= stairsAt)
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

int Room::get_auto_movement_dir(Info* _info, RangeInt2 const & rect) const
{
	// rect should include point we step onto
	VectorInt2 tileMin = VectorInt2(rect.x.min / _info->tileSize.x, rect.y.min / _info->tileSize.y);
	VectorInt2 tileMax = VectorInt2(rect.x.max / _info->tileSize.x, rect.y.max / _info->tileSize.y);
	for (int x = tileMin.x; x <= tileMax.x; ++x)
	{
		for (int y = tileMin.y; y <= tileMax.y; ++y)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->canStepOnto && tt->autoMovement)
				{
					return tt->autoMovement;
				}
			}
		}
	}
	return 0;
}

int Room::get_offset_on_stairs(Info* _info, VectorInt2 const & _at, VectorInt2 const & _offsetForStairs, RangeInt const & _feetForStairs, int _dir, int _step) const
{
	RangeInt feetForStairs(_feetForStairs.min - _step, _feetForStairs.max + _step);
	VectorInt2 at = _at + _offsetForStairs;
	VectorInt2 tileUpMin = VectorInt2((at.x + feetForStairs.min) / _info->tileSize.x, at.y / _info->tileSize.y);
	VectorInt2 tileUpMax = VectorInt2((at.x + feetForStairs.max) / _info->tileSize.x, at.y / _info->tileSize.y);
	VectorInt2 tileDownMin = VectorInt2((at.x + feetForStairs.min) / _info->tileSize.x, (at.y - 1) / _info->tileSize.y);
	VectorInt2 tileDownMax = VectorInt2((at.x + feetForStairs.max) / _info->tileSize.x, (at.y - 1) / _info->tileSize.y);
	VectorInt2 checkStairsAtWhenGoingUp = _dir > 0 ? VectorInt2(at.x + feetForStairs.max, at.y) : VectorInt2(at.x + feetForStairs.min, at.y);
	VectorInt2 checkStairsAtWhenGoingDown = _dir > 0 ? VectorInt2(at.x + feetForStairs.min, at.y) : VectorInt2(at.x + feetForStairs.max, at.y);
	for (int y = tileUpMax.y; y >= tileUpMin.y; --y)
	{
		for (int x = tileUpMin.x; x <= tileUpMax.x; ++x)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->stairsUp)
				{
					if (tt->stairsUp == _dir)
					{
						// going up in same direction
						if (checkStairsAtWhenGoingUp.x >= x * _info->tileSize.x &&
							checkStairsAtWhenGoingUp.x < (x + 1) * _info->tileSize.x)
						{
							int stairsAt = tt->calc_stairs_at(checkStairsAtWhenGoingUp.x - x * _info->tileSize.x, _step, _info->tileSize.x);
							if (stairsAt - _step == at.y - y * _info->tileSize.y)
							{
								return _step; // going up
							}
						}
					}
				}
			}
		}
	}

	for (int y = tileDownMax.y; y >= tileDownMin.y; --y)
	{
		for (int x = tileDownMin.x; x <= tileDownMax.x; ++x)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->stairsUp)
				{
					if (tt->stairsUp != _dir)
					{
						// going up, opposite, deduce step
						if (checkStairsAtWhenGoingDown.x + _step >= x * _info->tileSize.x &&
							checkStairsAtWhenGoingDown.x - _step < (x + 1) * _info->tileSize.x)
						{
							int stairsAt = tt->calc_stairs_at(checkStairsAtWhenGoingDown.x - x * _info->tileSize.x, _step, _info->tileSize.x);
							if (stairsAt - _step == at.y - y * _info->tileSize.y)
							{
								return -_step; // going down
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

bool Room::can_step_onto_except_stairs(Info* _info, RangeInt2 const & rect) const
{
	// rect should include point we step onto
	VectorInt2 tileMin = VectorInt2(rect.x.min / _info->tileSize.x, rect.y.min / _info->tileSize.y);
	VectorInt2 tileMax = VectorInt2(rect.x.max / _info->tileSize.x, rect.y.max / _info->tileSize.y);
	for (int y = tileMax.y; y >= tileMin.y; --y)
	{
		for (int x = tileMin.x; x <= tileMax.x; ++x)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->canStepOnto)
				{
					if (! tt->stairsUp &&
						y < tileMax.y)
					{
						// tileMax has y - 1 this will make checking whether we can step onto only if in this movement we will have rect.y.min in block below
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool Room::can_fall_onto(Info* _info, RangeInt2 const & rect, int _dir, OUT_ int & _atY, VectorInt2 const & _at, VectorInt2 const & _offsetForStairs, RangeInt const & _feetForStairs, int _step, OUT_ bool & _ontoStairs) const
{
	_ontoStairs = false;
	VectorInt2 at = _at + _offsetForStairs;
	// rect should include point we step onto
	VectorInt2 tileMin = VectorInt2(rect.x.min / _info->tileSize.x, rect.y.min / _info->tileSize.y);
	VectorInt2 tileMax = VectorInt2(rect.x.max / _info->tileSize.x, rect.y.max / _info->tileSize.y);
	int landsOnStep = NONE;
	for (int y = tileMax.y; y >= tileMin.y; --y)
	{
		for (int x = tileMin.x; x <= tileMax.x; ++x)
		{
			if (Tile const * tt = get_tile_at(VectorInt2(x, y)))
			{
				if (tt->canStepOnto)
				{
					if (tt->stairsUp)
					{
						if (_dir == tt->stairsUp) // stairs have to be in same dir
						{
							// check if we are able to catch that piece that we should land on (-step)
							// although I noticed that in original game it behaves like it is at least (-step * 3)
							if (tt->stairsUp == 1 && rect.x.max < (x + 1) * _info->tileSize.x - _step * 3)
							{
								continue;
							}
							if (tt->stairsUp == -1 && rect.x.max > x * _info->tileSize.x + _step * 3)
							{
								continue;
							}
							if (y < tileMax.y)
							{
								_atY = (y + 1) * _info->tileSize.y;
								return true;
							}
						}
						else //if (_dir == 0)
						{
							RangeInt checkRange(at.x + _feetForStairs.min, at.x + _feetForStairs.max);
							checkRange.min = max(checkRange.min, x * _info->tileSize.x);
							checkRange.max = min(checkRange.max, (x + 1) * _info->tileSize.x - 1);
							for (int subx = checkRange.min; subx <= checkRange.max; ++subx)
							{
								int stairsAt = tt->calc_stairs_at(subx - x * _info->tileSize.x, _step, _info->tileSize.x);
								stairsAt += y * _info->tileSize.y;
								if (//rect.y.max > stairsAt &&
									stairsAt >= rect.y.min)
								{
									landsOnStep = max(landsOnStep, stairsAt);
								}
							}
						}
					}
					else if (y < tileMax.y /*&&
							 mod(_at.y, _info->tileSize.y) == _info->tileSize.y - 1*/)
					{
						// tileMax has y - 1 this will make checking whether we can step onto only if in this movement we will have rect.y.min in block below
						_atY = (y + 1) * _info->tileSize.y;
						return true;
					}
				}
			}
		}
		if (landsOnStep != NONE &&
			landsOnStep <= rect.y.max)
		{
			_ontoStairs = true;
			_atY = landsOnStep;
			return true;
		}
	}
	return false;
}

Tile const * Room::get_tile_at(VectorInt2 const & _at) const
{
	if (_at.x >= 0 && _at.x < size.x &&
		_at.y >= 0 && _at.y < size.y)
	{
		int tile = tiles[_at.y * size.x + _at.x];
		if (tileRefs.is_index_valid(tile))
		{
			return tileRefs[tile].tile.get();
		}
	}
	return nullptr;
}

#endif
