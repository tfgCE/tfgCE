#include "collectInTargetCone.h"

#include "..\modules\custom\health\mc_health.h"
#include "..\modules\gameplay\moduleEquipment.h"

#include "..\..\framework\module\moduleCollision.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\object.h"
#include "..\..\framework\world\doorInRoom.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_DRAW_INSPECT

//

using namespace TeaForGodEmperor;

//

// movement names
DEFINE_STATIC_NAME(attached);

//

void CollectInTargetCone::collect(Vector3 const& loc, Vector3 const& dir, float totalDist, float angle, float maxOffDist, Framework::Room* inRoom, float distanceLeft, ArrayStack<Entry>& collected, Framework::DoorInRoom* throughDoor, Transform const& roomTransform, Params const & _params)
{
	if (!inRoom)
	{
		return;
	}
	Vector3 dirNormalised = dir.normal();
	Segment segment(loc, loc + dirNormalised * totalDist);
	FOR_EVERY_OBJECT_IN_ROOM(obj, inRoom) // it may be called at any time, also when we're able to damage/remove objects
	{
		// consider only 
		Framework::IModulesOwner* healthOwner = nullptr;
		bool okToCollect = true;
		if (okToCollect && _params.notAttached.get(false))
		{
			if (obj->get_presence()->is_attached() || obj->get_movement_name() == NAME(attached))
			{
				okToCollect = false;
				if (_params.attachedWithTag.is_set())
				{
					if (obj->get_tags().get_tag(_params.attachedWithTag.get()))
					{
						okToCollect = true;
					}
				}
			}
		}
		if (okToCollect && _params.withTag.is_set())
		{
			if (! obj->get_tags().get_tag(_params.withTag.get()))
			{
				okToCollect = false;
			}
		}
		if (okToCollect && _params.collectHealthOwners.get(false))
		{
			Framework::IModulesOwner* goUp = obj;
			while (goUp)
			{
				an_assert(goUp->get_presence());
				auto* attachedTo = goUp->get_presence()->get_attached_to();
				if (attachedTo)
				{
					bool allowAttached = false;
					// only sub objects and actively used equipments are allowed to be considered here, objects that are just attached (items in pockets, on forearms) should be ignored
					if (!allowAttached)
					{
						if (auto* o = obj->get_as_object())
						{
							if (auto* ot = o->get_object_type())
							{
								allowAttached = ot->is_sub_object();
							}
						}
					}
					if (!allowAttached)
					{
						if (auto* eq = obj->get_gameplay_as<ModuleEquipment>())
						{
							if (eq->get_user_as_modules_owner())
							{
								allowAttached = true;
							}
						}
					}
					if (!allowAttached)
					{
						healthOwner = nullptr;
						break;
					}
				}
				if (goUp->get_custom<CustomModules::Health>())
				{
					// found one not attached or allowed to be attached
					healthOwner = goUp;
					break;
				}
				goUp = attachedTo;
			}
			okToCollect = healthOwner != nullptr;
		}
		if (okToCollect)
		{
			auto* op = obj->get_presence();
			an_assert(op);
			Vector3 oLoc = op->get_centre_of_presence_transform_WS().get_translation();

			if (throughDoor && throughDoor->get_plane().get_in_front(oLoc) < 0.0f)
			{
				// skip it, behind the door
				continue;
			}

			// use movement collision primitive to check
			if (auto* c = obj->get_collision())
			{
				Vector3 locOS = op->get_placement().location_to_local(loc);
				if (c->calc_closest_point_on_primitive_os(REF_ locOS))
				{
					Vector3 locWS = op->get_placement().location_to_world(locOS);
					float atT;
					float dist = segment.get_distance(locWS, atT);
					Vector3 segLocWS = segment.get_at_t(atT);

					locWS = segLocWS + (locWS - segLocWS).normal() * clamp(dist - c->get_collision_primitive_radius() + 0.2f, 0.0f, dist);

#ifdef DEBUG_DRAW_INSPECT
					debug_context(inRoom);
					debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::red, loc, locWS));
					debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::red, loc, loc + (locWS - loc).normal() * min(totalDist, (locWS - loc).length())));
					debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::purple, loc, segLocWS));
					debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::yellow, segLocWS, locWS));
					debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::orange, segLocWS, segLocWS + (locWS - segLocWS).normal() * min(maxOffDist, (locWS - segLocWS).length())));
					debug_no_context();
#endif

					// check if this point is within angle
					float dotResult = Vector3::dot((locWS - loc).normal(), dirNormalised);
					if ((dotResult >= cos_deg(angle) || (locWS - segLocWS).length() <= maxOffDist) &&
						(locWS - loc).length() <= totalDist)
					{
#ifdef DEBUG_DRAW_INSPECT
						debug_context(inRoom);
						debug_draw_time_based(1.0f, debug_draw_arrow(true, Colour::c64Cyan, loc, locWS));
						debug_no_context();
#endif
						Entry e;
						e.target = obj;
						e.healthOwner = healthOwner;
						e.score = -(locWS - loc).length() * (0.5f + 0.5f * dotResult);
						e.closestLocationWS = roomTransform.location_to_world(locWS);
						e.intoRoomTransform = roomTransform;
						collected.push_back_or_replace(e); // better to fail here but how is it possible that we exceed this amount? 
						if (!collected.has_place_left())
						{
							break;
						}
					}
				}
			}
		}
	}

	if (!_params.singleRoom.get(false))
	{
		for_every_ptr(door, inRoom->get_doors())
		{
			if (door != throughDoor &&
				door->is_visible() &&
				door->has_world_active_room_on_other_side())
			{
				Plane plane = door->get_plane();
				if (plane.get_in_front(loc) > 0.0f &&
					Vector3::dot(plane.get_normal(), dir) < 0.0f)
				{
					float dist = door->calculate_dist_of(loc);
					if (dist < distanceLeft)
					{
						collect(door->get_other_room_transform().location_to_local(loc), door->get_other_room_transform().vector_to_local(dir), totalDist, angle, maxOffDist,
							door->get_world_active_room_on_other_side(), distanceLeft - dist, collected, door->get_door_on_other_side(), roomTransform.to_world(door->get_other_room_transform()), _params);
					}
				}
			}
		}
	}
}

