#pragma once

#include "..\..\core\collision\checkSegmentResult.h"

namespace Framework
{
	class Room;
	class PresenceLink;

	struct CheckSegmentResult
	: public Collision::CheckSegmentResult
	{
		Room const * inRoom = nullptr;
		Transform intoInRoomTransform = Transform::identity;
		PresenceLink const * presenceLink = nullptr;
		bool changedRoom = false; // if we want the result to be in our room, inRoom may stay the same but this will be set

		CheckSegmentResult();
	};
};
