#include "wmp_roomGeneratorInfo.h"

#include "..\roomGenerators\roomGenerationInfo.h"

//

using namespace TeaForGodEmperor;

//

bool TileSize::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<float>(RoomGenerationInfo::get().get_tile_size());

	return result;
}

//

bool TileCount::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<VectorInt2>(RoomGenerationInfo::get().get_tile_count());

	return result;
}

//

bool TilesZone::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<Range2>(RoomGenerationInfo::get().get_tiles_zone().to_range2());

	return result;
}

//

bool PlayAreaSize::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<Vector2>(RoomGenerationInfo::get().get_play_area_rect_size());

	return result;
}

//

bool PlayAreaZone::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<Range2>(RoomGenerationInfo::get().get_play_area_zone().to_range2());

	return result;
}

//

bool DoesAllowCrouch::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<bool>(RoomGenerationInfo::get().does_allow_crouch());

	return result;
}

//

bool DoesAllowAnyCrouch::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<bool>(RoomGenerationInfo::get().does_allow_any_crouch());

	return result;
}

//
