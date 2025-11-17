#pragma once

#include "..\..\core\math\math.h"

namespace Framework
{
	class DoorInRoom;
	class Room;
	class Environment;

	/*
	 *	Usage is build per thread when preparing scene for rendering.
	 *	Values (activation level and placement) are stored per thread with last frame placement stored (shader by) same for all threads.
	 *
	 *	This means that on rare occasions when both eyes see same environment for first time from two different rooms, they will have two different points of view.
	 *	But after one frame they will share last frame placement (as it will be stored in same order for each room using thread id) and it should stabilise.
	 *	Case described above should never happen in normal usage (only situation would be putting player right at the door rotated along door plane).
	 */
	struct EnvironmentUsage
	{
	public:
		EnvironmentUsage(Room* _owner);

		void ready_for_rendering(float _deltaTime);

		Transform activate(int _atActivationLevel); // this is part of environment proxy building how environment is placed

		int get_activation_level() const;

	private:
		struct ThreadUsage
		{
			int activationLevel; // -1 if not active, 0 if first with this environment visible
			Transform placement;
		};
		const int NOT_ACTIVE = -1;
		Room* owner;
		Environment* environment;
		Array<ThreadUsage> threadUsages;
		bool wasActiveLastFrame;
		bool wasActiveAnytime;
		float timeInactive;
		Transform requestedPlacement;
		Transform lastActivePlacement;

		friend class Room;

		Transform activate_worker(int _atActivationLevel, DoorInRoom const * _throughDoorFromOuter);

		Transform const & get_placement() const;
	};
};
