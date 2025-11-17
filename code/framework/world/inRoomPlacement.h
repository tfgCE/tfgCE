#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"

namespace Framework
{
	class Room;

	struct InRoomPlacement
	{
		static InRoomPlacement const & empty() { return s_empty; }

		SafePtr<Room> inRoom;
		Transform placement;

		InRoomPlacement();
		InRoomPlacement(Room* _room, Transform const & _placement);
		InRoomPlacement(Room* _room, Vector3 const & _loc);

		bool is_valid() const { return inRoom.is_set(); }
		void clear();

		void log(LogInfoContext & _context) const;

	private:
		static InRoomPlacement s_empty;
	};
};

DECLARE_REGISTERED_TYPE(Framework::InRoomPlacement);
DECLARE_REGISTERED_TYPE(Framework::InRoomPlacement*);
