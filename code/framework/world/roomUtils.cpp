#include "roomUtils.h"

#include "door.h"
#include "doorInRoom.h"
#include "room.h"

#include "..\game\game.h"
#include "..\presence\relativeToPresencePlacement.h"
#include "..\world\inRoomPlacement.h"

#include "..\..\core\random\randomUtils.h"
#include "..\..\core\tags\tag.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// tags
DEFINE_STATIC_NAME(canBeBlocked);

//

DoorInRoom* RoomUtils::find_closest_door(Room* _room, Vector3 const & _loc)
{
	DoorInRoom* result = nullptr;
	float best = 0.0f;
	for_every_ptr(door, _room->get_doors())
	{
		if (!door->is_visible()) continue;
		float dist = (door->get_hole_centre_WS() - _loc).length_2d();
		if (!result || best > dist)
		{
			result = door;
			best = dist;
		}
	}
	return result;
}

DoorInRoom* RoomUtils::find_furthest_door(Room* _room, Vector3 const & _loc)
{
	DoorInRoom* result = nullptr;
	float best = 0.0f;
	for_every_ptr(door, _room->get_doors())
	{
		if (!door->is_visible()) continue;
		float dist = (door->get_hole_centre_WS() - _loc).length_2d();
		if (!result || best < dist)
		{
			result = door;
			best = dist;
		}
	}
	return result;
}

DoorInRoom* RoomUtils::get_random_door(Room* _room, DoorInRoom* _butNotThisOne, bool _canBeTheSame)
{
	if (_room->get_doors().get_size() == 1)
	{
		auto* onlyDoor = _room->get_doors().get_first();
		if (_canBeTheSame || onlyDoor != _butNotThisOne)
		{
			return onlyDoor;
		}
		else
		{
			return nullptr;
		}
	}
	else if (_room->get_doors().get_size() > 1)
	{
		Random::Generator rg;

		auto & doors = _room->get_doors();

		int validCount = 0;
		for_every_ptr(door, doors)
		{
			if (door != _butNotThisOne &&
				door->can_move_through() &&
				door->is_relevant_for_movement() &&
				door->is_relevant_for_vr() &&
				door->get_door() &&
				door->get_door()->get_operation() != DoorOperation::StayClosed)
			{
				++validCount;
			}
		}

		if (validCount == 0 && !_canBeTheSame)
		{
			return nullptr;
		}
		else
		{
			// if no valid doors, allow to accept the same we've entered

			int idx = RandomUtils::ChooseFromContainer<Array<DoorInRoom*>, DoorInRoom*>::choose(rg, doors,
				[_butNotThisOne, validCount](DoorInRoom* const& _dir) { return
						((validCount == 0 || _dir != _butNotThisOne) &&
						 _dir->can_move_through() &&
						 _dir->is_relevant_for_movement() &&
						 _dir->is_relevant_for_vr() &&
						 _dir->get_door() && 
						 _dir->get_door()->get_operation() != DoorOperation::StayClosed) ? 1.0f : 0.0f; });
			return doors[idx];
		}
	}
	else
	{
		return nullptr;
	}
}

DoorInRoom* RoomUtils::get_door_towards(Room* _room, IModulesOwner* _forOwner, InRoomPlacement const& _inRoomPlacement)
{
	if (_room->get_doors().get_size() == 1)
	{
		return _room->get_doors().get_first();
	}
	else if (_room->get_doors().get_size() > 1)
	{
		RelativeToPresencePlacement rpp;
		rpp.be_temporary_snapshot();
		if (rpp.find_path(_forOwner, _inRoomPlacement.inRoom.get(), _inRoomPlacement.placement))
		{
			auto& doors = _room->get_doors();
			for_every_ptr(door, doors)
			{
				if (door == rpp.get_through_door())
				{
					return door;
				}
			}
		}
		return nullptr;
	}
	else
	{
		return nullptr;
	}
}
