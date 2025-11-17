#include "aipr_FindInVisibleRooms.h"

#include "..\aiRayCasts.h"
#include "..\logics\aiLogic_npcBase.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiPerception.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#include "..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace PerceptionRequests;

//

REGISTER_FOR_FAST_CAST(FindInVisibleRooms);

FindInVisibleRooms::FindInVisibleRooms(Framework::IModulesOwner* _owner, CheckObject _check_object, RateObject _rate_object, int _notVisibleRoomDepth)
: check_object(_check_object)
, rate_object(_rate_object)
, notVisibleRoomDepth(_notVisibleRoomDepth)
, owner(_owner)
{
	SET_EXTRA_DEBUG_INFO(found, TXT("FindInVisibleRooms.found"));
	SET_EXTRA_DEBUG_INFO(lookInto, TXT("FindInVisibleRooms.lookInto"));
	checkFoundIdx = NONE;
	if (owner.get())
	{
		if (auto* ai = owner->get_ai())
		{
			if (auto* mind = ai->get_mind())
			{
				npcBase = fast_cast<AI::Logics::NPCBase>(mind->get_logic());
			}
		}
	}
}

void FindInVisibleRooms::look_into(Framework::Room const * _room, Transform const & _viewpointPlacement, Framework::DoorInRoom const * _throughDoor, Optional<int> const & _notVisibleRoomIdx)
{
	if (! owner.get() || !_room)
	{
		return;
	}
	if (lookInto.get_space_left() <= 0)
	{
		// if first one, we need two, but as it is the first one, we won't be exceeding the limit
		return;
	}
	if (_viewpointPlacement.get_scale() == 0.0f)
	{
		// no valid view point
		return;
	}
	bool firstOne = lookInto.is_empty();
	if (firstOne)
	{
		LookInto li;
		li.throughDoor = _throughDoor;
		li.viewpointPlacement = _viewpointPlacement;
		li.viewFrustum.clear();
		lookInto.push_back(li);
	}
	// add all valid objects
	for_every_ptr(object, _room->get_objects())
	{
		if (object == owner.get() ||
			!Framework::AI::Perception::is_valid_for_perception(object) ||
			!check_object(object))
		{
			continue;
		}
		int atIdx;
		if (found.get_max_size() > found.get_size())
		{
			atIdx = found.get_size();
			found.set_size(found.get_size() + 1);
		}
		else
		{
			atIdx = Random::get_int(found.get_size());
		}
		Found& f = found[atIdx];
		f.object = object;
		f.path.reset();
		f.path.set_owner(owner.get());
		f.path.set_target(object);
		f.isInVisibleRoom = ! _notVisibleRoomIdx.is_set();
		for_every(li, lookInto)
		{
			if (li->throughDoor)
			{
				f.path.push_through_door(li->throughDoor->get_door_on_other_side());
			}
		}
	}
	an_assert(!_throughDoor || !lookInto.is_empty());
	for_every_ptr(door, _room->get_doors())
	{
		if (door != _throughDoor &&
			door->is_visible() &&
			door->has_world_active_room_on_other_side())
		{
			if (door->get_plane().get_in_front(_viewpointPlacement.get_translation()) > 0.0f)
			{
				Optional<int> notVisibleRoomIdx = _notVisibleRoomIdx;
				System::ViewFrustum doorVF;
				if (! notVisibleRoomIdx.is_set())
				{
					// only if still visible
					System::ViewFrustum wholeDoorVF;
					door->get_hole()->setup_frustum_placement(door->get_side(), door->get_hole_scale(), wholeDoorVF, _viewpointPlacement, door->get_outbound_transform());

					if (lookInto.get_last().viewFrustum.is_empty())
					{
						an_assert(lookInto.get_size() == 1);
						doorVF = wholeDoorVF;
					}
					else
					{
						if (!doorVF.clip(&wholeDoorVF, &lookInto.get_last().viewFrustum))
						{
							notVisibleRoomIdx = 0;
						}
					}
				}
				else
				{
					notVisibleRoomIdx = notVisibleRoomIdx.get() + 1;
				}

				// allow getting from not visible rooms;
				if (!notVisibleRoomIdx.is_set() || notVisibleRoomIdx.get() <= notVisibleRoomDepth)
				{
					LookInto li;
					li.throughDoor = door->get_door_on_other_side();
					li.viewFrustum = doorVF;
					li.viewpointPlacement = door->get_other_room_transform().to_local(_viewpointPlacement);

					lookInto.push_back(li);
					look_into(door->get_world_active_room_on_other_side(), li.viewpointPlacement, li.throughDoor, notVisibleRoomIdx);
					lookInto.pop_back();
				}
			}
		}
	}
	if (firstOne)
	{
		lookInto.pop_back();
	}
}

bool FindInVisibleRooms::process()
{
	scoped_call_stack_info(TXT("perception request - find in visible rooms"));

	if (owner.get())
	{
		if (checkFoundIdx == NONE)
		{
			Transform perceptionWS;
			if (npcBase)
			{
				Transform perceptionSocketOS = npcBase->get_perception_socket_index() != NONE ? owner->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index()) : owner->get_presence()->get_centre_of_presence_os();
				Transform perceptionSocketWS = owner->get_presence()->get_placement().to_world(perceptionSocketOS);
				perceptionWS = perceptionSocketWS;
			}
			else
			{
				perceptionWS = owner->get_presence()->get_centre_of_presence_transform_WS();
			}

			if (Framework::Room* room = owner->get_presence()->get_in_room())
			{
				room->move_through_doors(owner->get_presence()->get_centre_of_presence_transform_WS(), perceptionWS, room);

				// go through rooms and find all objects that might be a possible target
				look_into(room, perceptionWS, nullptr, NP);
			}

			checkFoundIdx = 0; // continue with first one
		}

		if (found.get_size() == 0)
		{
			// nothing
			bestIdx = NONE;
			return base::process();
		}
		else if (found.get_size() == 1)
		{
			// just one target, get it
			bestIdx = 0;
			return base::process();
		}
		else
		{
			for_count(int, rateObjectIdx, 3)
			{
				if (checkFoundIdx < found.get_size())
				{
					Found& f = found[checkFoundIdx];
					f.score = rate_object ? rate_object(f.object.get(), f.path) : own_rate_object(f.object.get(), f.path, f.isInVisibleRoom);
					if (bestIdx == NONE || bestScore < f.score)
					{
						bestIdx = checkFoundIdx;
						bestScore = f.score;
					}

					++checkFoundIdx;
				}
			}
			if (checkFoundIdx >= found.get_size())
			{
				// finished
				return base::process();
			}
			return false; // keep going on
		}
	}

	return base::process();
}

float FindInVisibleRooms::own_rate_object(Framework::IModulesOwner const * _check, Framework::PresencePath& _path, bool _isInVisibleRoom) const
{
	if (!owner.get())
	{
		return 0.0f;
	}
	// if we see object
	Transform perceptionWS;
	Range perceptionFOV = Range(-180.0f, 180.0f);
	Range perceptionVerticalFOV = Range(-90.0f, 90.0f);
	if (npcBase)
	{
		Transform perceptionSocketOS = npcBase->get_perception_socket_index() != NONE ? owner->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index()) : owner->get_presence()->get_centre_of_presence_os();
		if (npcBase->get_scanning_perception_socket_index() != NONE)
		{
			Transform proposedPerceptionSocketOS = owner->get_appearance()->calculate_socket_os(npcBase->get_scanning_perception_socket_index());
			if (proposedPerceptionSocketOS.get_scale() > 0.0f)
			{
				perceptionSocketOS = proposedPerceptionSocketOS;
			}
		}
		Transform perceptionSocketWS = owner->get_presence()->get_placement().to_world(perceptionSocketOS);
		perceptionWS = perceptionSocketWS;
		if (!npcBase->is_omniscient()) // if omniscient keep all-around fov
		{
			perceptionFOV = npcBase->get_perception_fov();
			perceptionVerticalFOV = npcBase->get_perception_vertical_fov();
		}
	}
	else
	{
		perceptionWS = owner->get_presence()->get_centre_of_presence_transform_WS();
	}

	float result = 0.0f;
	if (_path.is_active())
	{
		Transform enemyPlacementInOwnerRoom = _path.from_target_to_owner(_path.get_target_presence()->get_placement());
		if (!npcBase ||
			!npcBase->is_omniscient()) // omniscient will choose the closest
		{
			if (_isInVisibleRoom &&
				AI::check_clear_perception_ray_cast(AI::CastInfo(), perceptionWS, perceptionFOV, perceptionVerticalFOV, owner.get(),
					_check,
					_path.get_target_presence()->get_in_room(), enemyPlacementInOwnerRoom))
			{
				result += magic_number 1000.0f;
			}
		}
		result -= (enemyPlacementInOwnerRoom.get_translation() - perceptionWS.get_translation()).length();
	}
	else
	{
		// maybe it was just destroyed, a frame ago or even in the same frame?
		result -= magic_number 20000.0f;
	}

	return result;
}

