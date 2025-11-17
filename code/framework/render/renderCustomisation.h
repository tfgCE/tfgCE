#pragma once

#include <functional>

namespace Framework
{
	class DoorInRoom;
	class Room;

	namespace Render
	{
		class DoorInRoomProxy;
		class EnvironmentProxy;
		class SceneBuildContext;

		struct Customisation
		{
			std::function<void(SceneBuildContext const & _context, DoorInRoomProxy*, DoorInRoom const*)> setup_door_in_room_proxy = nullptr;
			std::function<void(SceneBuildContext const & _context, EnvironmentProxy*, EnvironmentProxy* previous, Room const * forRoom)> setup_environment_proxy = nullptr;
		};

	};
};

