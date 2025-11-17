#include "aipr_FindClosest.h"

#include "..\..\..\framework\ai\aiPerception.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

#include "..\..\..\core\random\randomUtils.h"

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace PerceptionRequests;

//

REGISTER_FOR_FAST_CAST(FindClosest);

FindClosest::FindClosest(Framework::IModulesOwner* _owner, Optional<float> const& _maxDistance, Optional<int> const & _maxDepth, CheckObject _check_object)
: check_object(_check_object)
, maxDistance(_maxDistance.is_set()? _maxDistance.get() : -1.0f)
, maxDepth(_maxDepth.is_set()? _maxDepth.get() : -1)
, owner(_owner)
{
	SET_EXTRA_DEBUG_INFO(doorPenalties, TXT("FindClosest.doorPenalties"));
	if (maxDistance <= 0.0f && maxDepth < 0)
	{
		warn(TXT("try to use maxDistance or maxDepth, otherwise we might be looking too far and too long"));
	}
}

bool FindClosest::process()
{
	scoped_call_stack_info(TXT("perception request - find closest"));

	if (owner.get())
	{
		if (phase == P_Setup)
		{
			scoped_call_stack_info(TXT("setup"));
#ifdef AN_DEBUG_FIND_CLOSEST
			output(TXT("fc:%p reset on setup"), this);
#endif
			result = nullptr;
			resultDistance = maxDistance;
			phase = P_SearchRooms;

			if (auto* room = owner->get_presence()->get_in_room())
			{
				// first one
				searchRooms.push_back(SearchRoom());
				auto& sr = searchRooms.get_last();
				sr.toRoom.reset();
				sr.toRoom.set_owner(owner.get());
				sr.toRoom.set_placement_in_final_room(owner->get_presence()->get_placement());
				sr.depth = 0;
				sr.distanceSoFar = 0.0f;
			}
		}

		if (phase == P_SearchRooms)
		{
			scoped_call_stack_info(TXT("search rooms"));
			int processLeft = 5; // small number of rooms at a time
			while (!searchRooms.is_empty() && processLeft > 0)
			{
				process_search_room();
				--processLeft;
			}
			if (searchRooms.is_empty())
			{
				phase = P_Finalise;
			}
		}

		if (phase == P_Finalise)
		{
			scoped_call_stack_info(TXT("finalise"));
#ifdef AN_DEBUG_FIND_CLOSEST
			output(TXT("fc:%p reset on finalise"), this);
#endif

			if (owner.get() && result.get())
			{
				if (!pathToResult.find_path(owner.get(), result.get()))
				{
					result = nullptr;
				}
			}

			return base::process();
		}

		return false; // keep processing
	}
	else
	{
		return base::process();
	}
}

void FindClosest::process_search_room()
{
	auto& sr = searchRooms.get_first();

#ifdef AN_DEBUG_FIND_CLOSEST
	{
		output(TXT("fc:%p:%i"), this, psrNo);
		++psrNo;
		output(TXT("result distance: %.1f"), resultDistance);
		output(TXT("search rooms queue"));
		for_every(sr, searchRooms)
		{
			output(TXT("   %i%c d%i r%p[%S]"), for_everys_index(sr), sr->toRoom.is_active()? '+' : '-', sr->depth, sr->toRoom.get_in_final_room(), sr->toRoom.get_in_final_room()? sr->toRoom.get_in_final_room()->get_name().to_char() : TXT("--"));
		}
	}
#endif
	if (sr.toRoom.is_active())
	{
		if (auto* room = sr.toRoom.get_in_final_room())
		{
			float penalty = 0.0f;
			float nextPenalty = 0.0f;

			{
				int throughDoors = sr.toRoom.get_through_doors().get_size();
				for_every(dp, doorPenalties)
				{
					if (throughDoors > dp->atDoor)
					{
						penalty += dp->addDistance;
					}
					if (throughDoors == dp->atDoor)
					{
						nextPenalty += dp->addDistance;
					}
				}
			}

			if (resultDistance < 0.0f || sr.distanceSoFar + penalty < resultDistance)
			{
				// find objects in room
				{
#ifdef AN_DEBUG_FIND_CLOSEST
					output(TXT("scan objects in the room"));
#endif
					//auto* intoRoomThroughDoor = sr.toRoom.get_through_door_into_final_room();
					//Vector3 loc = intoRoomThroughDoor ? intoRoomThroughDoor->get_hole_centre_WS() : (sr.toRoom.get_owner_presence() ? sr.toRoom.get_owner_presence()->get_centre_of_presence_WS() : Vector3::zero);
					for_every_ptr(object, room->get_objects())
					{
						if (object == owner.get() ||
							!Framework::AI::Perception::is_valid_for_perception(object))
						{
							continue;
						}
						Optional<Transform> offsetOS = Transform::identity;
						if (!check_object || check_object(object, REF_ offsetOS))
						{
							Transform placement = object->get_presence()->get_placement();
							if (offsetOS.is_set())
							{
								placement = placement.to_world(offsetOS.get());
							}
							else
							{
								placement.set_translation(object->get_presence()->get_centre_of_presence_WS());
							}
							sr.toRoom.set_placement_in_final_room(placement);
							float dist = sr.toRoom.calculate_string_pulled_distance();
							dist += penalty;
							//float dist = sr.distanceSoFar + (placement.get_translation() - loc).length();
							dist = max(0.0f, dist);
							if (dist < resultDistance || resultDistance < 0.0f)
							{
#ifdef AN_DEBUG_FIND_CLOSEST
								output(TXT("found new closest %S at %.1f"), object->ai_get_name().to_char(), dist);
#endif
								resultDistance = dist;
								result = object;
							}
						}
					}
				}

				// get doors to more rooms (except the one we entered)
				if (maxDepth < 0 || sr.depth <= maxDepth)
				{
#ifdef AN_DEBUG_FIND_CLOSEST
					output(TXT("go through doors and add to the list (at the end)"));
#endif
					// get through all doors (not the one we came through and add to the queue)
					auto* throughDoorIntoFinal = sr.toRoom.get_through_door_into_final_room();
					int throughDoorIntoFinalNavGroupId = throughDoorIntoFinal ? throughDoorIntoFinal->get_nav_group_id() : NONE;
					if ((!throughDoorIntoFinal || throughDoorIntoFinalNavGroupId == NONE) &&
						room == owner->get_presence()->get_in_room())
					{
						auto foundNavNode = room->find_nav_location(owner->get_presence()->get_placement());
						if (foundNavNode.is_valid())
						{
							throughDoorIntoFinalNavGroupId = foundNavNode.node->get_group();
						}
					}
					for_every_ptr(door, room->get_doors())
					{
						if (!door->is_visible())
						{
#ifdef AN_DEBUG_FIND_CLOSEST
							output(TXT("  not visible (inactive)"));
#endif
							continue;
						}
						if (!door->has_room_on_other_side())
						{
#ifdef AN_DEBUG_FIND_CLOSEST
							output(TXT("  no room connected"));
#endif
							continue;
						}
#ifdef AN_DEBUG_FIND_CLOSEST
						output(TXT(" %i d%p"), for_everys_index(door), door);
#endif
						if (door == throughDoorIntoFinal)
						{
							// already went there
#ifdef AN_DEBUG_FIND_CLOSEST
							output(TXT("  we came through this door"));
#endif
							continue;
						}
						if (onlyNavigable)
						{
							if (auto* d = door->get_door())
							{
								if (auto* dt = d->get_door_type())
								{
									if (!dt->is_nav_connector())
									{
#ifdef AN_DEBUG_FIND_CLOSEST
										output(TXT("  not a nav connector"));
#endif
										continue;
									}
								}
							}
							if (door->get_nav_group_id() != throughDoorIntoFinalNavGroupId ||
								throughDoorIntoFinalNavGroupId == NONE)
							{
#ifdef AN_DEBUG_FIND_CLOSEST
								output(TXT("  different nav group id (door:%i we came through:%i)"), door->get_nav_group_id(), throughDoorIntoFinalNavGroupId);
#endif
								continue;
							}
						}
						if (sr.toRoom.get_through_doors().has_place_left())
						{
							sr.toRoom.set_placement_in_final_room(door->get_placement());
							float doorDist = sr.toRoom.calculate_string_pulled_distance();
							float totalDist = doorDist + sr.distanceSoFar;
							float totalDistWithPenalty = doorDist + sr.distanceSoFar + penalty + nextPenalty;
							if (resultDistance < 0.0f || totalDistWithPenalty < resultDistance)
							{
								SearchRoom nsr;
								nsr.depth = sr.depth + 1;
								nsr.distanceSoFar = totalDist;
								nsr.toRoom = sr.toRoom;
								if (nsr.toRoom.get_through_doors().has_place_left())
								{
									nsr.toRoom.push_through_door(door);
									if (auto* od = door->get_door_on_other_side())
									{
										if (door->is_placed() && od->is_placed())
										{
											nsr.toRoom.set_placement_in_final_room(od->get_placement());
											searchRooms.push_back(nsr);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	searchRooms.pop_front();
}

FindClosest* FindClosest::add_door_penalty(int _atDoor, float _addDistance)
{
	DoorPenalty dp;
	dp.atDoor = _atDoor;
	dp.addDistance = _addDistance;
	doorPenalties.push_back(dp);
	return this;
}
