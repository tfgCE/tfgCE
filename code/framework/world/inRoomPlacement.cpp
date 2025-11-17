#include "inRoomPlacement.h"

#include "room.h"

using namespace Framework;

//

InRoomPlacement InRoomPlacement::s_empty;

InRoomPlacement::InRoomPlacement()
{
}

InRoomPlacement::InRoomPlacement(Room* _room, Transform const & _placement)
: inRoom(_room)
, placement(_placement)
{
}

InRoomPlacement::InRoomPlacement(Room* _room, Vector3 const & _loc)
: inRoom(_room)
, placement(_loc, Quat::identity)
{
}

void InRoomPlacement::clear()
{
	inRoom.clear();
}

void InRoomPlacement::log(LogInfoContext & _context) const
{
	if (is_valid())
	{
		_context.log(is_valid()? TXT("VALID") : TXT("NOT VALID"));
		_context.log(TXT("in room : %S"), inRoom.get() ? inRoom->get_name().to_char() : TXT("--"));
		_context.log(TXT("placement :"));
		{
			LOG_INDENT(_context);
			placement.log(_context);
		}
	}
}
