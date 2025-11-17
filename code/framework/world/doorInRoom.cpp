#include "doorInRoom.h"

#include "door.h"
#include "doorType.h"
#include "room.h"
#include "spaceBlocker.h"
#include "world.h"

#include "..\appearance\mesh.h"
#include "..\appearance\skeleton.h"
#include "..\collision\collisionModel.h"
#include "..\game\game.h"
#include "..\library\usedLibraryStored.inl"
#include "..\nav\navMesh.h"
#include "..\nav\navSystem.h"
#include "..\nav\nodes\navNode_Point.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\collision\queryPrimitivePoint.h"
#include "..\..\core\collision\queryPrimitiveSegment.h"
#include "..\..\core\vr\iVR.h"

// !@# why? without this it does not compile
#include "..\video\texture.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define AN_INSPECT_SET_VR_PLACEMENT
#endif

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifndef BUILD_PUBLIC_RELEASE
		#ifndef DEBUG_WORLD_LIFETIME
			#define DEBUG_WORLD_LIFETIME
		#endif
	#endif
#endif

//

using namespace Framework;

//

#ifdef AN_DEVELOPMENT
DEFINE_STATIC_NAME(door);
DEFINE_STATIC_NAME(doorEntrace);
#endif

// collision flags
DEFINE_STATIC_NAME(worldSolidFlags);

// room tags
DEFINE_STATIC_NAME(inaccessible);
DEFINE_STATIC_NAME(vrInaccessible);

//

REGISTER_FOR_FAST_CAST(DoorInRoom);

DoorInRoom::DoorInRoom(Room* _inRoom)
: SafeObject<DoorInRoom>(nullptr) // we will make ourselves available soon
, inRoom(_inRoom)
, door(nullptr)
, individual(Random::get_float(0.0f, 1.0f))
, isPlaced(false)
, placement(Transform::identity)
, applySidePlacement(Transform::identity)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("DoorInRoom [new] dr'%p"), this);
#endif

	DOOR_IN_ROOM_HISTORY(this, TXT("created dr'%p [%i]"), this, get_activation_group() ? get_activation_group()->get_id() : -1);

	an_assert(Collision::DefinedFlags::has(NAME(worldSolidFlags)));
	set_collision_flags(Collision::DefinedFlags::get_existing_or_basic(NAME(worldSolidFlags)));

	make_safe_object_available(this);
	an_assert(_inRoom);
	inRoom->add_door(this);

	cache_ai_object_pointers();
}

DoorInRoom::~DoorInRoom()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("DoorInRoom [delete] dr'%p"), this);
#endif

	make_safe_object_unavailable();
	outdate_nav();
	if (door)
	{
		door->unlink(this);
	}
	update_on_link();
	if (inRoom)
	{
		inRoom->remove_door(this);
		inRoom->mark_nav_mesh_creation_required();
	}
}

void DoorInRoom::make_safe_object_available(DoorInRoom* _object)
{
	Collision::ICollidableObject::make_safe_object_available(_object);
	SafeObject<DoorInRoom>::make_safe_object_available(_object);
	SafeObject<IAIObject>::make_safe_object_available(_object);
}

void DoorInRoom::make_safe_object_unavailable()
{
	Collision::ICollidableObject::make_safe_object_unavailable();
	SafeObject<DoorInRoom>::make_safe_object_unavailable();
	SafeObject<IAIObject>::make_safe_object_unavailable();
}

void DoorInRoom::link(Door* _door)
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	an_assert(door == nullptr, TXT("door in room should not be linked when being linked to door"));
	DOOR_IN_ROOM_HISTORY(this, TXT("linked to %p"), _door);
	door = _door;
	if (! door->is_linked(this))
	{
		door->link(this);
	}
	update_on_link();
}

void DoorInRoom::update_on_link()
{
	update_side();
	on_set_placement(); // even if placement is identity, we will still get applySidePlacement calculated properly
}

void DoorInRoom::update_side()
{
	side = door ? (door->is_linked_as_a(this) ? DoorSide::A : (door->is_linked_as_b(this) ? DoorSide::B : DoorSide::Any)) : DoorSide::Any;
}

void DoorInRoom::link_as_a(Door* _door)
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	an_assert(door == nullptr, TXT("door in room should not be linked when being linked to door"));
	DOOR_IN_ROOM_HISTORY(this, TXT("linked to %p as A"), _door);
	door = _door;
	if (!door->is_linked(this))
	{
		door->link_a(this);
	}
	update_on_link();
}

void DoorInRoom::link_as_b(Door* _door)
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	an_assert(door == nullptr, TXT("door in room should not be linked when being linked to door"));
	DOOR_IN_ROOM_HISTORY(this, TXT("linked to %p as B"), _door);
	door = _door;
	if (!door->is_linked(this))
	{
		door->link_b(this);
	}
	update_on_link();
}

void DoorInRoom::unlink()
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC_OR_ASYNC(! get_in_world() || get_in_world()->is_being_destroyed());
	}
#endif
	an_assert(door != nullptr, TXT("couldn't unlink door in room that are not linked"));
	DOOR_IN_ROOM_HISTORY(this, TXT("unlinked from %p"), door);
	Door* wasLinkedTo = door;
	door = nullptr;
	if (wasLinkedTo->is_linked(this))
	{
		wasLinkedTo->unlink(this);
	}
}

void DoorInRoom::on_connection_through_door_created()
{
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif
	an_assert(door->get_on_other_side(this), TXT("door in room on other side should exist"));
	roomOnOtherSide = door->get_on_other_side(this)->inRoom;
	update_other_room_transform();
	updatedForDoorOpenFactor = -9999.0f;
	update_wings();
	update_movement_info();
	if (inRoom)
	{
		inRoom->mark_nav_mesh_creation_required(); // we need a new one, if we already ordered one, won't matter
	}
}

void DoorInRoom::update_other_room_transform()
{
	DoorInRoom* doorOnOtherSide = door->get_on_other_side(this);
	if (!is_placed() || !doorOnOtherSide->is_placed())
	{
		// we might need to wait until other this door is placed or other one is placed
		// when one or other is placed we get updated, so don't worry about that
		return;
	}
	otherRoomMatrix = outboundMatrix.to_world(doorOnOtherSide->inboundMatrix.inverted());
	otherRoomTransform = otherRoomMatrix.to_transform();
	an_assert(otherRoomTransform.is_ok(), TXT("other room transform is not ok"));
	an_assert(otherRoomTransform.get_orientation().is_normalised(), TXT("other room transform is not ok"));
	// TODO -- yy what?
}

void DoorInRoom::on_connection_through_door_broken()
{
	roomOnOtherSide = nullptr;
	// TODO -- yy what?
}

void DoorInRoom::update_placement_to_requested()
{
	if (requestedPlacement.is_set())
	{
		set_placement(requestedPlacement.get());
		requestedPlacement.clear();
	}
}

void DoorInRoom::set_placement(Transform const & _placement)
{
	isPlaced = true;
	placement = _placement;
	on_set_placement();
	if (DoorInRoom* otherDoor = get_door_on_other_side())
	{
		if (otherDoor->is_placed())
		{
			otherDoor->on_set_placement();
			update_other_room_transform();
			otherDoor->update_other_room_transform();
		}
	}
}

void DoorInRoom::make_vr_placement_immovable()
{
	an_assert(vrSpacePlacement.is_set());
	vrSpacePlacementImmovable = true;
}

void DoorInRoom::make_vr_placement_movable()
{
	vrSpacePlacementImmovable = false;
}

void DoorInRoom::set_vr_space_placement(Transform const & _vrSpacePlacement, bool _forceOver180)
{
	if (! GameStaticInfo::get()->is_sensitive_to_vr_anchor())
	{
		vrSpacePlacement = _vrSpacePlacement;
		on_set_placement();
		return;
	}
#ifdef AN_INSPECT_SET_VR_PLACEMENT
	output(TXT("set vr placement %S / %S"), _vrSpacePlacement.get_translation().to_string().to_char(), _vrSpacePlacement.get_orientation().to_rotator().to_string().to_char());
#endif

	an_assert(!vrSpacePlacementImmovable, TXT("can't set, use grow_into_vr_corridor"));
	if (vrSpacePlacement.is_set() &&
		DoorInRoom::same_with_orientation_for_vr(vrSpacePlacement.get(), _vrSpacePlacement))
	{
		// if we force over 180, we should check actual transform, not turned
		if (!_forceOver180 || Transform::same_with_orientation(vrSpacePlacement.get(), _vrSpacePlacement))
		{
			return;
		}
	}
	// this maybe should be allowed if we want to move door
	// if we can't move, do anything
	an_assert(!can_move_through() || !is_relevant_for_movement() ||!is_relevant_for_vr() || (inRoom && !inRoom->is_vr_arranged()) || ! vrSpacePlacement.is_set(), TXT("has been already vr arranged, do not change vr placement, use grow_into_vr_corridor"));
	vrSpacePlacement = _vrSpacePlacement;
	if (!inRoom ||
		(!inRoom->get_tags().get_tag(NAME(inaccessible)) &&
			!inRoom->get_tags().get_tag(NAME(vrInaccessible))))
	{
		if (!
			(may_ignore_vr_placement() || !can_move_through() || !is_relevant_for_movement() || !is_relevant_for_vr() || vrSpacePlacement.get().get_translation().z == 0.0f || (get_door() && get_door()->get_door_type() && get_door()->get_door_type()->can_be_off_floor())))
		{
			warn_dev_investigate(TXT("door is off the floor, may indicate invalid vr anchor setup, \"%S\" in \"%S\""),
				get_door() && get_door()->get_door_type() ? get_door()->get_door_type()->get_name().to_string().to_char() : TXT("[no door type]"),
				get_in_room() ? get_in_room()->get_name().to_char() : TXT("[non in a room]"));
		}
	}
	on_set_placement();
}

void DoorInRoom::update_vr_placement_from_door_on_other_side()
{
	scoped_call_stack_info(TXT("update_vr_placement_from_door_on_other_side"));
	an_assert(get_door_on_other_side());
	Transform vrPlacementFromOtherDoor = Door::reverse_door_placement(get_door_on_other_side()->get_vr_space_placement());
	set_vr_space_placement(vrPlacementFromOtherDoor);
	on_set_placement();
}

Transform const & DoorInRoom::get_vr_space_placement() const
{
	an_assert(vrSpacePlacement.is_set());
	return vrSpacePlacement.get();
}

DoorInRoom* DoorInRoom::grow_into_room(Optional<Transform> const& _requestedPlacement, Transform const& _requestedVRPlacement, RoomType const* _roomType, DoorType const* _newDoorDoorType, tchar const* _proposedName)
{
	scoped_call_stack_info(TXT("grow into room"));

	ASSERT_NOT_SYNC;

#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("grow into room dr'%p of d%p"), this, get_door());
#endif

	Room* orgRoom = get_in_room();
	return grow_into_thing(_requestedPlacement, _requestedVRPlacement, _newDoorDoorType,
		[_roomType, _proposedName, orgRoom](Room* corridor, DoorInRoom* a, DoorInRoom* b)
		{
			corridor->set_room_type(_roomType);
			if (_proposedName)
			{
				corridor->set_name(String(_proposedName));
			}
			Random::Generator nrg = orgRoom->get_individual_random_generator();
			nrg.advance_seed(10852, 47923975);
			corridor->set_origin_room(orgRoom->get_origin_room());
			corridor->set_individual_random_generator(nrg.spawn());
		});
}

DoorInRoom* DoorInRoom::grow_into_vr_corridor(Optional<Transform> const& _requestedPlacement, Transform const& _requestedVRPlacement, VR::Zone const& _beInZone, float _tileSize)
{
	scoped_call_stack_info(TXT("grow into vr corridor"));

	ASSERT_NOT_SYNC;

#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("grow into vr corridor dr'%p of d%p"), this, get_door());
#endif

	return grow_into_thing(_requestedPlacement, _requestedVRPlacement, nullptr,
		[_beInZone, _tileSize](Room* corridor, DoorInRoom* a, DoorInRoom* b)
		{
			Door::make_vr_corridor(corridor, a, b, _beInZone, _tileSize);
		});
}

DoorInRoom* DoorInRoom::grow_into_thing(Optional<Transform> const& _requestedPlacement, Transform const& _requestedVRPlacement,
	OPTIONAL_ DoorType const* _useNewDoorType, std::function<void(Room* corridor, DoorInRoom* a, DoorInRoom* b)> _perform_on_thing)
{
	an_assert(GameStaticInfo::get()->is_sensitive_to_vr_anchor());

	if (!GameStaticInfo::get()->is_sensitive_to_vr_anchor())
	{
		return nullptr;
	}

	scoped_call_stack_info(TXT("grow into thing"));

	ASSERT_NOT_SYNC;

#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("grow into thing dr'%p of d%p"), this, get_door());
#endif

	/**
	 *	Changes into something (like vr corridor)
	 *
	 *	[door] this -> room
	 *
	 *	into
	 *	
	 *	[door] this -> vr corridor <- doorA [new door] doorB -> room
	 *
	 *	doorB is returned as a result
	 */
	Room* corridor;
	Door* secondDoor;

	// da and db is used to determine how the new room should behave
	// if we're a world separator, we completely ignore the other door as it should have nothing to say in this part of the world
	DoorInRoom* da = this;
	DoorInRoom* db = get_door()->is_world_separator_door()? nullptr : get_door_on_other_side();
	DoorInRoom* dSource = da;
	DoorInRoom* dOther = db;

	float doorVRPriority = 0.0f;
	if (dOther)
	{
		// check which one has priority and how big it is
		Random::Generator rg = get_in_room()->get_individual_random_generator();
		rg.advance_seed(2358976, 5497);
		float daPriority = 0.0f;
		float dbPriority = -1.0f;
		if (da->get_in_room())
		{
			daPriority = da->get_in_room()->get_door_vr_corridor_priority(rg, daPriority);
		}
		daPriority = da->get_door()->get_door_vr_corridor_priority(rg, daPriority);
		if (db)
		{
			if (db->get_in_room())
			{
				dbPriority = db->get_in_room()->get_door_vr_corridor_priority(rg, dbPriority);
			}
			dbPriority = db->get_door()->get_door_vr_corridor_priority(rg, dbPriority);
		}
		if (daPriority >= dbPriority)
		{
			doorVRPriority = daPriority;
			dSource = da;
			dOther = db;
		}
		else if (daPriority < dbPriority)
		{
			doorVRPriority = dbPriority;
			dSource = db;
			dOther = da;
		}
	}

	DoorInRoom* doorA;
	DoorInRoom* doorB;

	bool passWorldSeparatorMarkToSecondDoor = false;
	if (dSource->get_door() &&
		dSource->get_door()->is_world_separator_door())
	{
		passWorldSeparatorMarkToSecondDoor = dSource->get_door()->get_room_separated_from_world() == dSource->get_in_room();
	}

	Game::get()->push_activation_group(get_activation_group());
	Game::get()->perform_sync_world_job(TXT("create vr corridor"), [&corridor, &secondDoor, &doorA, &doorB, this, dSource, dOther, _useNewDoorType, passWorldSeparatorMarkToSecondDoor]()
	{
		// make corridor exist in same region
		corridor = new Room(dSource->get_in_room()->get_in_sub_world(), dSource->get_in_room()->get_in_region(), nullptr /* we should setup room type later*/, Random::Generator().be_zero_seed() /* we should setup room type later*/ );
		ROOM_HISTORY(corridor, TXT("grow into a thing"));
#ifdef AN_OUTPUT_WORLD_GENERATION
		output(TXT("new room on grow into thing r%p"), corridor);
#endif
		Door::set_vr_corridor_tags(corridor, dSource->get_in_room(), 1);
		if (dOther)
		{
			Door::set_vr_corridor_tags(corridor, dOther->get_in_room(), 1);
		}
		auto* toRoom = get_in_room();

		// create second door - copy of this one
		secondDoor = new Door(dSource->get_in_room()->get_in_sub_world(), _useNewDoorType? _useNewDoorType : dSource->get_door()->get_door_type());
#ifdef AN_OUTPUT_WORLD_GENERATION
		output(TXT("new door on grow into thing d%p"), secondDoor);
#endif
		// we want this:
		//		[door] this -> vr corridor <- doorA [second door] doorB -> room

		// but first we place doorA in corridor (which is where it should be)
		// and doorB in corridor but only to swap it with this in room
		doorA = new DoorInRoom(corridor);
		doorB = new DoorInRoom(corridor);
		DOOR_IN_ROOM_HISTORY(doorA, TXT("growing door into a thing"));
		DOOR_IN_ROOM_HISTORY(doorB, TXT("growing door into a thing"));
		// right now we have
		//		[door] this -> room
		//		[no door linked] doorB -> vr corridor <- doorA [no door linked]  ---- note that doorA and doorB are swapped here on purpose, so you can track them below

		// at this point we want to still tags from this to doorB as doorB is going to replace us in where we came from
		doorB->access_tags().set_tags_from(get_tags());
		access_tags().clear();

		// make them in the right rooms we want to keep us with the old door in the old room
		// and in the same place in the array - hence putting doorB in corridor in first place and then swapping doorB and this around
		corridor->replace_door(doorB, this);
		toRoom->replace_door(this, doorB);
		doorB->inRoom = toRoom;
		inRoom = corridor;
		// now we have
		//		[door] this -> vr corridor <- doorA [no door linked]
		//		[no door linked] doorB -> room

		// and link newly created door-in-rooms to new door
		secondDoor->link(doorA);
		secondDoor->link(doorB);
		// we end up with what we wanted
		//		[door] this -> vr corridor <- doorA [second door] doorB -> room

		if (passWorldSeparatorMarkToSecondDoor)
		{
			dSource->get_door()->pass_being_world_separator_to(secondDoor);
		}

		// update cached values
		if (get_door_on_other_side())
		{
			this->on_connection_through_door_created();
			get_door_on_other_side()->on_connection_through_door_created();
		}
		doorA->on_connection_through_door_created();
		doorB->on_connection_through_door_created();

		an_assert(corridor->get_all_doors().does_contain(this));
		an_assert(corridor->get_all_doors().does_contain(doorA));
		an_assert(toRoom->get_all_doors().does_contain(doorB));

		an_assert(this->get_door() != secondDoor);
		an_assert(doorA->get_door() == secondDoor);
		an_assert(doorB->get_door() == secondDoor);
		an_assert(this->get_door()->is_linked(this));
		an_assert(doorA->get_door()->is_linked(doorA));
		an_assert(doorB->get_door()->is_linked(doorB));
	});
	Game::get()->pop_activation_group(get_activation_group());

	{
		static int corridorIdx = 0;
		++corridorIdx;
		String corridorName = String::printf(TXT("grown %i"), corridorIdx);
		corridor->set_name(corridorName);

#ifdef AN_DEVELOPMENT
		if (corridorIdx == 2)
		{
			//AN_BREAK;
		}
#endif
	}

	corridor->set_door_vr_corridor_priority(doorVRPriority);

	if (_requestedPlacement.is_set())
	{
		doorB->set_placement(_requestedPlacement.get());
	}
	doorB->set_vr_space_placement(_requestedVRPlacement);
	doorA->update_vr_placement_from_door_on_other_side();

	/*
#ifdef AN_DEVELOPMENT
	{	// visualisation code when door is outside the zone
		if (!_beInZone.does_contain(_requestedVRPlacement.get_translation().to_vector2(), doorWidth * 0.4f))
		{
			DebugVisualiserPtr dv(new DebugVisualiser(corridor->get_name()));
			dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
			if (debugThis)
			{
				dv->activate();
			}
			dv->add_text(Vector2::zero, String(TXT("door outside!")), Colour::red, 0.3f);
			DebugVisualiserUtils::add_doors_to_debug_visualiser(dv, _beInZone,
				doorB, _requestedVRPlacement.get_translation().to_vector2(), _requestedVRPlacement.get_axis(Axis::Forward).to_vector2(),
				doorWidth);
			dv->end_gathering_data();
			dv->show_and_wait_for_key();

			an_assert(_beInZone.does_contain(_requestedVRPlacement.get_translation().to_vector2(), doorWidth * 0.4f));
		}
	}
#endif
	*/

	// update this one's placement only after we updated doorA and doorB
	if (get_door_on_other_side() &&
		get_door_on_other_side()->is_vr_space_placement_set())
	{
		if (is_vr_placement_immovable())
		{
			make_vr_placement_movable();
			update_vr_placement_from_door_on_other_side();
			make_vr_placement_immovable();
		}
		else
		{
			update_vr_placement_from_door_on_other_side();
		}
	}

	_perform_on_thing(corridor, this, doorA);

	reinit_meshes_if_init();

	doorA->init_meshes();
	doorB->init_meshes();

	if (auto* g = Game::get())
	{
		g->on_newly_created_door_in_room(doorA);
		g->on_newly_created_door_in_room(doorB);
	}

	return doorB;
}

void DoorInRoom::on_set_placement()
{
	outboundMatrix = placement.to_matrix();
	inboundMatrix = turn_matrix_xy_180(outboundMatrix);
	inboundPlacement = inboundMatrix.to_transform();
	// calculate plane (from inbound)
	plane.set(inboundMatrix.get_y_axis(), inboundMatrix.get_translation());
	// update "apply side placement" that is used to modify wings and frame (if needed)
	applySidePlacement = placement.to_local((get_side() == DoorSide::A ? outboundMatrix : inboundMatrix).to_transform());
	update_scaled_placement_matrix();
}

bool DoorInRoom::is_inside_hole(Vector3 const & _point) const
{
	// doors could be unlinked but still active?
	if (!get_hole())
	{
		return false;
	}
	REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(TXT("is inside hole dr'%p"), this);
	Vector3 const pointLS = get_outbound_matrix().location_to_local(_point);
	return get_hole()->is_point_inside(get_side(), get_hole_scale(), pointLS);
}

bool DoorInRoom::is_inside_hole(SegmentSimple const & _segment) const
{
	// doors could be unlinked but still active?
	if (!get_hole())
	{
		return false;
	}
	REMOVE_AS_SOON_AS_POSSIBLE_ scoped_call_stack_info_str_printf(TXT("is inside hole dr'%p"), this);
	Vector3 const point0LS = get_outbound_matrix().location_to_local(_segment.get_start());
	Vector3 const point1LS = get_outbound_matrix().location_to_local(_segment.get_end());
	return get_hole()->is_segment_inside(get_side(), get_hole_scale(), SegmentSimple(point0LS, point1LS));
}

bool DoorInRoom::is_within_radius(Vector3 const & _point, float _dist, Optional<Range> const & _inFront) const
{
	Range inFrontRange = _inFront.get(Range(-0.001f, _dist));
	float inFront = plane.get_in_front(_point);
	if (inFront < inFrontRange.min || inFront >= inFrontRange.max)
	{
		return false;
	}

	Vector3 const pointLS = get_outbound_matrix().location_to_local(_point);
	return get_hole()->is_point_inside(get_side(), get_hole_scale(), pointLS, _dist);
}

bool DoorInRoom::is_in_radius(bool _checkFront, Vector3 const & _point, float _dist, OUT_ Vector3 & _onDoorPlaneInsideDoor, OUT_ float & _distLeft, Optional<float> const & _offHoleDistMultiplier, Optional<float> const & _minInFront) const
{
	float minInFront = _minInFront.get(-0.001f);
	float inFront = plane.get_in_front(_point);
	if (_checkFront && (inFront < minInFront || inFront >= _dist))
	{
		return false;
	}

	float distOfPoint = calculate_dist_of(_point, inFront, OUT_ _onDoorPlaneInsideDoor, _offHoleDistMultiplier.get(1.0f));
	_distLeft = _dist - distOfPoint;

	return _distLeft > 0.0f;
}

bool DoorInRoom::is_in_radius(bool _checkFront, SegmentSimple const & _segment, float _dist, OUT_ SegmentSimple & _onDoorPlaneInsideDoor, OUT_ float & _distLeft, Optional<float> const & _offHoleDistMultiplier, Optional<float> const & _minInFront) const
{
	float minInFront = _minInFront.get(-0.001f);
	an_assert(door);
	float inFrontA = plane.get_in_front(_segment.get_start());
	float inFrontB = plane.get_in_front(_segment.get_end());
	if (_checkFront && ((inFrontA < minInFront && inFrontB < minInFront) || (inFrontA >= _dist && inFrontB >= _dist)))
	{
		return false;
	}

	float const offHoleDistMultiplier = _offHoleDistMultiplier.get(1.0f);

	// this is not a good solution but I have no idea for non trivial cases what could be fine, this should work for most common cases, though
	// it works as moving points of segment into door and pretending that the closer one is enough
	todo_note(TXT("this is not a good solution but... <see comment above>"));
	Vector3 onDoorPlaneInsideDoorA;
	Vector3 onDoorPlaneInsideDoorB;
	float distOfPointA = calculate_dist_of(_segment.get_start(), inFrontA, OUT_ onDoorPlaneInsideDoorA, offHoleDistMultiplier);
	float distOfPointB = calculate_dist_of(_segment.get_end(), inFrontB, OUT_ onDoorPlaneInsideDoorB, offHoleDistMultiplier);

	_onDoorPlaneInsideDoor = SegmentSimple(onDoorPlaneInsideDoorA, onDoorPlaneInsideDoorB);

	float distOfPoint = min(distOfPointA, distOfPointB);
	_distLeft = _dist - distOfPoint;

	return _distLeft > 0.0f;
}

bool DoorInRoom::is_in_radius(bool _checkFront, Collision::QueryPrimitivePoint const & _point, float _dist, Optional<float> const & _offHoleDistMultiplier, Optional<float> const & _minInFront, OPTIONAL_ OUT_ float * _distOfPoint) const
{
	float minInFront = _minInFront.get(-0.001f);
	float inFront = plane.get_in_front(_point.location);
	if (_checkFront && (inFront < minInFront || inFront >= _dist))
	{
		return false;
	}

	float distOfPoint = calculate_dist_of(_point.location, inFront, _offHoleDistMultiplier.get(1.0f));

	assign_optional_out_param(_distOfPoint, distOfPoint);

	return _dist - distOfPoint > 0.0f;
}

bool DoorInRoom::is_in_radius(bool _checkFront, Collision::QueryPrimitiveSegment const & _segment, float _dist, Optional<float> const & _offHoleDistMultiplier, Optional<float> const & _minInFront, OPTIONAL_ OUT_ float * _distOfPoint) const
{
	float minInFront = _minInFront.get(-0.001f);
	float inFrontA = plane.get_in_front(_segment.locationA);
	float inFrontB = plane.get_in_front(_segment.locationB);
	if (_checkFront && ((inFrontA < minInFront && inFrontB < minInFront) || (inFrontA >= _dist && inFrontB >= _dist)))
	{
		return false;
	}

	float const offHoleDistMultiplier = _offHoleDistMultiplier.get(1.0f);

	// this is version of QueryPrimitiveSegment::get_gradient
	todo_future(TXT("make this code a template or define?"));

	float validInFrontA = clamp(inFrontA, 0.0f, _dist);
	float validInFrontB = clamp(inFrontB, 0.0f, _dist);

	// calculate (relative) location of valid A and B
	float inFrontA2B = inFrontB - inFrontA;
	float invInFrontA2B = inFrontA2B != 0.0f ? 1.0f / inFrontA2B : 0.0f;
	float locA = inFrontA2B != 0.0f ? (validInFrontA - inFrontA) * invInFrontA2B : 0.0f;
	float locB = inFrontA2B != 0.0f ? (validInFrontB - inFrontA) * invInFrontA2B : 0.0f;

	float const minDistance = 0.0f;
	float const delta = 0.01f;
	float const invDelta = 1.0f / delta;
	Vector3 const a = _segment.locationA;
	Vector3 const a2b = _segment.locationB - _segment.locationA;
	Vector3 const a2bd = a2b * delta;

	Vector3 const qA = a + a2b * locA;
	Vector3 const qB = a + a2b * locB;
	Vector3 const qdA(qA + a2bd);
	Vector3 const qdB(qB - a2bd);

	float const distA = calculate_dist_of(qA, offHoleDistMultiplier);
	float const distB = calculate_dist_of(qB, offHoleDistMultiplier);
	float const distdA = calculate_dist_of(qdA, offHoleDistMultiplier);
	float const distdB = calculate_dist_of(qdB, offHoleDistMultiplier);

	float dA = (distdA - distA) * invDelta;
	float dB = (distdB - distB) * invDelta;

	float closestDist = _dist;

	if (distA > minDistance && distA < distB && dA > 0.0f)
	{
		// if at A it grows (when heading to centre) and A is closer than B, choose A
		closestDist = distA;
	}
	else if (distB > minDistance && distB < distA && dB > 0.0f)
	{
		// if at B it grows (when heading to centre) and B is closer than A, choose B
		closestDist = distB;
	}
	else
	{
		// find using derevative
		float curLoc = distA > distB ? 0.0f : 1.0f;
		float epsilon = 0.001f;
		float dStep = 0.5f;

		// but when we get below min distance, try to stop at min distance
		float prevLoc = curLoc;
		float lastDCur = 0.0f;
		int triesLeft = 10;
		while (triesLeft-- > 0)
		{
			Vector3 locCur = a + a2b * (curLoc);
			float distCur = calculate_dist_of(locCur, offHoleDistMultiplier);
			if (distCur <= minDistance)
			{
				// don't go so far, try to stop at the edge
				dStep *= 0.5f;
				curLoc = clamp(prevLoc - lastDCur * dStep, 0.0f, 1.0f);
				if (dStep < 0.001f)
				{
					break;
				}
				continue;
			}
			else
			{
				closestDist = distCur;
			}
			Vector3 locdCur(locCur + a2bd);
			float distdCur = calculate_dist_of(locdCur, offHoleDistMultiplier);
			float dCur = (distdCur - distCur) * invDelta;
			lastDCur = dCur;
			prevLoc = curLoc;
			curLoc = clamp(curLoc - dCur * dStep, 0.0f, 1.0f);
			if (abs(dCur) < epsilon)
			{
				break;
			}
		}
	}

	assign_optional_out_param(_distOfPoint, closestDist);

	return _dist - closestDist > 0.0f;
}

void DoorInRoom::skip_hole_check_on_moving_through()
{
	skipHoleCheckOnMovingThrough = true;
}

void DoorInRoom::update_movement_info()
{
	canMoveThrough = door? door->can_move_through() : false;
	relevantForMovement = door ? door->is_relevant_for_movement() : false;
	relevantForVR = door ? door->is_relevant_for_vr() : false;
}

bool DoorInRoom::does_go_through_hole(Vector3 const & _currLocation, Vector3 const & _nextLocation, OUT_ Vector3 & _destLocation, float _minInFront, float _isInsideDoorExt) const
{
	float currInFront = get_plane().get_in_front(_currLocation);
	//float currInFront2 = -get_outbound_matrix().location_to_local(_currLocation).y;
	if (currInFront >= _minInFront)
	{
		float nextInFront = get_plane().get_in_front(_nextLocation);
		if (nextInFront < currInFront && nextInFront <= 0.0f) // going through
		{
			if (skipHoleCheckOnMovingThrough)
			{
				return true;
			}
			float newT = currInFront / (-nextInFront + currInFront);
			if (newT <= 1.0f + EPSILON)
			{
				Vector3 const atDoorHole = Vector3::blend(_currLocation, _nextLocation, newT);
				Vector3 const atDoorHoleLS = get_outbound_matrix().location_to_local(atDoorHole);
				if (get_hole()->is_point_inside(get_side(), get_hole_scale(), atDoorHoleLS, _isInsideDoorExt))
				{
					_destLocation = atDoorHole;
					return true;
				}
			}
		}
	}
	return false;
}

bool DoorInRoom::check_segment(REF_ Segment & _segment, REF_ CheckSegmentResult & _result, float _minInFront) const
{
	Vector3 const currLocation = _segment.get_start();
	float currInFront = get_plane().get_in_front(currLocation);
	if (currInFront >= _minInFront)
	{
		Vector3 const nextLocation = _segment.get_end();
		float nextInFront = get_plane().get_in_front(nextLocation);
		if (nextInFront < currInFront && nextInFront <= 0.0f) // going through
		{
			float newT = max(0.0f, currInFront / (-nextInFront + currInFront));
			if (_segment.would_collide_at(newT))
			{
				Vector3 const atDoorHole = Vector3::blend(currLocation, nextLocation, newT);
				Vector3 const atDoorHoleLS = get_outbound_matrix().location_to_local(atDoorHole);
				if (get_hole()->is_point_inside(get_side(), get_hole_scale(), atDoorHoleLS))
				{
					return _segment.collide_at(newT);
				}
			}
		}
	}
	return false;
}

Vector3 DoorInRoom::find_inside_hole_for_string_pulling(Vector3 const & _a, Vector3 const & _b, Optional<Vector3> const & _lastInHole, float _moveInside) const
{
	Vector3 lastInHole = _lastInHole.is_set() ? _lastInHole.get() : get_hole_centre_WS();
	float aDist = (lastInHole - _a).length();
	float bDist = (lastInHole - _b).length();
	float t = aDist / (aDist + bDist);
	Vector3 onDoorPlaneRS = Vector3::blend(_a, _b, t);
	// drop it onto door plane - this way we should get most approximate string pulled location
	onDoorPlaneRS = get_plane().get_dropped(onDoorPlaneRS);

	// get into local space of door
	Vector3 onDoorPlaneLS = outboundMatrix.location_to_local(onDoorPlaneRS);
	// get location inside hole
	auto* hole = get_hole();
	Vector3 const insideDoorHoleLS = hole? hole->move_point_inside(get_side(), get_hole_scale(), onDoorPlaneLS, _moveInside) : Vector3::zero;
	// calculate
	Vector3 onDoorPlaneInsideDoorRS = outboundMatrix.location_to_world(insideDoorHoleLS);

	return onDoorPlaneInsideDoorRS;
}

Vector3 DoorInRoom::find_inside_hole(Vector3 const & _a, float _moveInside) const
{
	Vector3 onDoorPlaneRS = get_plane().get_dropped(_a);

	// get into local space of door
	Vector3 onDoorPlaneLS = outboundMatrix.location_to_local(onDoorPlaneRS);
	// get location inside hole
	auto* hole = get_hole();
	Vector3 const insideDoorHoleLS = hole ? hole->move_point_inside(get_side(), get_hole_scale(), onDoorPlaneLS, _moveInside) : Vector3::zero;;
	// calculate
	Vector3 onDoorPlaneInsideDoorRS = outboundMatrix.location_to_world(insideDoorHoleLS);

	return onDoorPlaneInsideDoorRS;
}

void DoorInRoom::debug_draw_hole(bool _front, Colour const & _colour) const
{
	debug_push_transform(outboundMatrix.to_transform()); // hole is stored as outbound
	if (DoorHole const * doorHole = get_hole())
	{
		doorHole->debug_draw(side, get_hole_scale(), _front, _colour);
	}
	debug_pop_transform();
}

void DoorInRoom::clear_meshes()
{
	wings.clear();
	mesh.clear();
	movementCollision.clear();
	preciseCollision.clear();
	usedMeshes.clear();
}

void DoorInRoom::reinit_meshes_if_init()
{
	if (meshesInit)
	{
		init_meshes();
	}
}

void DoorInRoom::init_meshes()
{
	an_assert(door, TXT("has to be linked to door"));

	GeneratedMeshDeterminants determinants;
	determinants.gather_for(this, door->get_door_type()->get_generated_mesh_determinants());
	if (door->get_door_type() == meshesDoneForDoorType &&
		door->get_secondary_door_type() == meshesDoneForSecondaryDoorType &&
		determinants == meshesDoneForMeshDeterminants)
	{
		updatedForDoorOpenFactor = -9999.0f;
		update_wings();
		return;
	}

	meshesInit = true;
	meshesReady = false;

	Game::get()->perform_sync_world_job(TXT("clear door meshes"), [this]()
	{
		clear_meshes();
	});
	meshesDoneForDoorType = door->get_door_type();
	meshesDoneForSecondaryDoorType = door->get_secondary_door_type();
	meshesDoneForMeshDeterminants = determinants;

	if (door->get_door_type()->get_wings().is_empty() &&
		!door->get_door_type()->get_frame_mesh_for(this, determinants) &&
		(!door->get_secondary_door_type() ||	(door->get_secondary_door_type()->get_wings().is_empty() &&
												!door->get_secondary_door_type()->get_frame_mesh_for(this, determinants))))
	{
		// no meshes
		update_collision_bounding_boxes();
		return;
	}
	Array<DoorWingType const *> wingsInType;
	for_every(dwt, door->get_door_type()->get_wings())
	{
		wingsInType.push_back(dwt);
	}
	if (door->get_secondary_door_type())
	{
		for_every(dwt, door->get_secondary_door_type()->get_wings())
		{
			wingsInType.push_back(dwt);
		}
	}

	for_count(int, iType, 2)
	{
		auto doorType = iType == 0 ? door->get_door_type() : door->get_secondary_door_type();
		if (doorType)
		{
			UsedLibraryStored<Mesh> fromMesh(doorType->get_frame_mesh_for(this, determinants));
			if (fromMesh.is_set())
			{
				if (inRoom)
				{
					Mesh const * replacedFromMesh = fromMesh.get();
					inRoom->apply_replacer_to_mesh(REF_ replacedFromMesh);
					fromMesh = replacedFromMesh;
				}
				if (fromMesh.is_set())
				{
					usedMeshes.push_back(fromMesh);
					Transform framePlacement = doorType->get_frame_mesh_placement();
					// if we're half, we do not need to apply side placement, we're good with local placement
					// for not half, we need to apply side placement though, to appear reversed on one side which will give us full frame when combined
					if (!doorType->is_frame_as_half())
					{
						framePlacement = applySidePlacement.to_world(framePlacement);
					}
					if (Meshes::IMesh3D const * mesh3d = fromMesh->get_mesh())
					{
						Game::get()->perform_sync_world_job(TXT("add mesh"), [this, mesh3d, framePlacement, fromMesh]()
						{
							Meshes::Mesh3DInstance* meshInstance = nullptr;
							if (Skeleton const * skeleton = fromMesh->get_skeleton())
							{
								meshInstance = mesh.add(mesh3d, skeleton->get_skeleton());
							}
							else
							{
								meshInstance = mesh.add(mesh3d);
							}
							meshInstance->set_placement(framePlacement);
							MaterialSetup::apply_material_setups(fromMesh->get_material_setups(), *meshInstance);
						});
					}
					bool addedPreciseCollision = false;
					if (CollisionModel const * cm = fromMesh->get_precise_collision_model())
					{
						if (Collision::Model const * c_m = cm->get_model())
						{
							Game::get()->perform_sync_world_job(TXT("add precise collision model"), [this, c_m, framePlacement]()
							{
								Collision::ModelInstance* collisionInstance = preciseCollision.add(c_m);
								collisionInstance->set_placement(framePlacement);
							});
							addedPreciseCollision = true;
						}
					}
					if (CollisionModel const * cm = fromMesh->get_movement_collision_model())
					{
						if (Collision::Model const * c_m = cm->get_model())
						{
							Game::get()->perform_sync_world_job(TXT("add movement collision model"), [this, c_m, framePlacement]()
							{
								Collision::ModelInstance* collisionInstance = movementCollision.add(c_m);
								collisionInstance->set_placement(framePlacement);
							});
							if (!addedPreciseCollision)
							{
								Game::get()->perform_sync_world_job(TXT("add precise collision model (using movement)"), [this, c_m, framePlacement]()
								{
									Collision::ModelInstance* collisionInstance = preciseCollision.add(c_m);
									collisionInstance->set_placement(framePlacement);
								});
							}
						}
					}
				}
			}
		}
	}

	Array<DoorWingInRoom> newWings;
	newWings.make_space_for(wingsInType.get_size());
	for_every_ptr(wingInType, wingsInType)
	{
		newWings.set_size(newWings.get_size() + 1);
		DoorWingInRoom & wing = newWings.get_last();
		wing.wingInType = wingInType;
		wing.meshInstanceIndex = NONE;
		wing.movementCollisionInstanceIndex = NONE;
		wing.preciseCollisionInstanceIndex = NONE;
		UsedLibraryStored<Mesh> fromMesh(wingInType->get_mesh_for(this));
		if (fromMesh.is_set())
		{
			if (inRoom)
			{
				Mesh const * replacedFromMesh = fromMesh.get();
				inRoom->apply_replacer_to_mesh(REF_ replacedFromMesh);
				fromMesh = replacedFromMesh;
			}
			if (fromMesh.is_set())
			{
				wing.mesh = fromMesh;
				wing.skeleton = fromMesh->get_skeleton();
				if (Meshes::IMesh3D const * mesh3d = fromMesh->get_mesh())
				{
					wing.meshInstanceIndex = mesh.get_instances_num();
					Meshes::Mesh3DInstance* meshInstance = nullptr;
					if (Skeleton const * skeleton = fromMesh->get_skeleton())
					{
						meshInstance = mesh.add(mesh3d, skeleton->get_skeleton());
					}
					else
					{
						meshInstance = mesh.add(mesh3d);
					}

					meshInstance->set_placement(Transform::identity); // will be updated in update_wings
					MaterialSetup::apply_material_setups(fromMesh->get_material_setups(), *meshInstance);
					meshInstance->access_can_be_combined_as_immovable().set(false); // should be movable - force it
					meshInstance->access_clip_planes().add(wingInType->clipPlanes);
				}
				bool addedPreciseCollision = false;
				if (CollisionModel const * cm = fromMesh->get_precise_collision_model())
				{
					if (Collision::Model const * c_m = cm->get_model())
					{
						wing.preciseCollisionInstanceIndex = preciseCollision.get_instances_num();
						Collision::ModelInstance* collisionInstance = preciseCollision.add(c_m);
						collisionInstance->set_placement(Transform::identity); // will be updated in update_wings
						addedPreciseCollision = true;
					}
				}
				if (CollisionModel const * cm = fromMesh->get_movement_collision_model())
				{
					if (Collision::Model const * c_m = cm->get_model())
					{
						wing.movementCollisionInstanceIndex = movementCollision.get_instances_num();
						Collision::ModelInstance* collisionInstance = movementCollision.add(c_m);
						collisionInstance->set_placement(Transform::identity); // will be updated in update_wings
						if (!addedPreciseCollision)
						{
							wing.preciseCollisionInstanceIndex = preciseCollision.get_instances_num();
							Collision::ModelInstance* collisionInstance = preciseCollision.add(c_m);
							collisionInstance->set_placement(Transform::identity); // will be updated in update_wings
						}
					}
				}
				if (wing.skeleton.get())
				{
					todo_important(TXT("for case of skeleton, copy bones"));
				}
			}
		}
		else
		{
			wing.skeleton.set(nullptr);
		}
	}

	Game::get()->perform_sync_world_job(TXT("set wings"), [this, &newWings]()
	{
		wings = newWings;
	});

	// update wings only if hasn't been activated yet
	if (!is_world_active())
	{
		updatedForDoorOpenFactor = -9999.0f;
		update_wings();
	}

	update_collision_bounding_boxes();
}

void DoorInRoom::update_wings()
{
	if (!door)
	{
		return;
	}
	if (wings.is_empty())
	{
		return;
	}
	
	Vector3 doorWholeScale = Vector3::one;
	{
		Vector2 rs = door->get_relative_size();
		doorWholeScale.x *= rs.x;
		doorWholeScale.z *= rs.y;
	}
	float doorOpenFactor = door->get_open_factor();
	if (updatedForDoorOpenFactor == doorOpenFactor)
	{
		return;
	}
	updatedForDoorOpenFactor = doorOpenFactor;

	for_every(wing, wings)
	{
		if (wing->meshInstanceIndex != NONE ||
			(wing->movementCollisionInstanceIndex != NONE ||
			 wing->preciseCollisionInstanceIndex != NONE))
		{
			Vector3 scaleDoor = Vector3::one;
			if (wing->wingInType->nominalDoorSize.is_set())
			{
				Vector2 divBy = wing->wingInType->nominalDoorSize.get();
				scaleDoor = Vector3(Door::get_nominal_door_width().get(0.0f), 1.0f, Door::get_nominal_door_height());
				if (door && door->get_door_type() &&
					door->get_door_type()->should_use_height_from_width())
				{
					scaleDoor.z = Door::get_nominal_door_width().get(0.0f);
				}
				if (divBy.x == 0.0f)
				{
					scaleDoor.x = 1.0f;
				}
				else
				{
					scaleDoor.x /= divBy.x;
					if (scaleDoor.x == 0.0f) scaleDoor.x = 1.0f;
				}
				if (divBy.y == 0.0f)
				{
					scaleDoor.z = 1.0f;
				}
				else
				{
					scaleDoor.z /= divBy.y;
					if (scaleDoor.z == 0.0f) scaleDoor.z = 1.0f;
				}
			}
			scaleDoor *= doorWholeScale;

			// apply offset
			float mappedOpenFactor = clamp(wing->wingInType->mapOpenPt.get_pt_from_value(abs(doorOpenFactor)), 0.0f, 1.0f) * sign(doorOpenFactor);
			Vector3 slideOffset = wing->wingInType->slideOffset * scaleDoor * mappedOpenFactor;
			Transform offsetPlacement = Transform(slideOffset, Quat::identity);

			Transform wingTypePlacement = wing->wingInType->placement;
			wingTypePlacement.set_translation(wingTypePlacement.get_translation() * scaleDoor);
			Transform wingPlacement = offsetPlacement.to_world(wingTypePlacement);

			// if half it should be left as it is, in local placement
			// asymmetrical though should be reversed if required
			if (!wing->wingInType->doorType->is_frame_as_half() ||
				wing->wingInType->asymmetrical)
			{
				wingPlacement = applySidePlacement.to_world(wingPlacement);
			}
			if (wing->meshInstanceIndex != NONE)
			{
				auto * meshInstance = mesh.access_instance(wing->meshInstanceIndex);
				meshInstance->set_placement(wingPlacement);
				if (!wing->wingInType->clipPlanes.is_empty() || ! wing->wingInType->autoClipPlanes.is_empty())
				{
					float mulX = wing->wingInType->asymmetrical && side == DoorSide::B ? -1.0f : 1.0f;
					struct PrepareClipPlane
					{
						static Plane prepare(float mulX, Vector3 const& _anchor, Vector3 const& _normal, Vector3 const & _scaleDoor)
						{
							Vector3 o = _anchor * Vector3(mulX, 1.0f, 1.0f);
							Vector3 n = _normal * Vector3(mulX, 1.0f, 1.0f);

							// we take plane, take two points on it, rescale, and recalculate normal

							Vector3 r(n.z, n.y, -n.x); // rotate right
							Vector3 o2 = o + r;
							o *= _scaleDoor;
							o2 *= _scaleDoor;
							r = o2 - o;

							n = Vector3(-r.z, r.y, r.x); // rotate left

							return Plane(n.normal(), o);
						}
					};
					PlaneSet clipPlanes;
					for_every(plane, wing->wingInType->clipPlanes.planes)
					{
						clipPlanes.add(PrepareClipPlane::prepare(mulX, plane->get_anchor(), plane->get_normal(), scaleDoor));
					}
					for_every(meshNodeName, wing->wingInType->autoClipPlanes)
					{
						for_every_ref(usedMesh, usedMeshes)
						{
							if (auto*mn = usedMesh->find_mesh_node(*meshNodeName))
							{
								Vector3 o = Vector3(mulX, 1.0f, 1.0f) * mn->placement.get_translation();
								Vector3 n = Vector3(mulX, 1.0f, 1.0f) * mn->placement.get_axis(Axis::Forward).normal();
								clipPlanes.add(Plane(n, o));
								break;
							}
						}
					}
					meshInstance->access_clip_planes().transform_to_local_of(clipPlanes, wingPlacement); // we need to update clip planes as we move wing
				}
			}
			if (wing->movementCollisionInstanceIndex != NONE)
			{
				movementCollision.access_instance(wing->movementCollisionInstanceIndex)->set_placement(wingPlacement);
			}
			if (wing->preciseCollisionInstanceIndex != NONE)
			{
				preciseCollision.access_instance(wing->preciseCollisionInstanceIndex)->set_placement(wingPlacement);
			}
			if (wing->skeleton.get())
			{
				todo_important(TXT("for case of skeleton, apply bones"));
			}
		}
	}

	update_collision_bounding_boxes();
}

void DoorInRoom::debug_draw_collision(int _againstCollision)
{
	debug_push_transform(get_placement());
	if (_againstCollision == NONE || _againstCollision == AgainstCollision::Movement)
	{
		get_movement_collision().debug_draw(true, false, meshesReady? Colour::orange : Colour::grey, 0.1f);
	}
	if (_againstCollision == NONE || _againstCollision == AgainstCollision::Precise)
	{
		get_precise_collision().debug_draw(true, false, meshesReady ? Colour::orange : Colour::grey, 0.1f);
	}
	debug_pop_transform();
}

bool DoorInRoom::check_segment_against(AgainstCollision::Type _againstCollision, REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context) const
{
	auto const & againstCollision = get_against_collision(_againstCollision);
	if (meshesReady &&
		!againstCollision.is_empty())
	{
		if (_context.should_check(this))
		{
			Segment localInLinkRay = get_placement().to_local(_segment);
			debug_push_transform(get_placement());
			if (againstCollision.check_segment(REF_ localInLinkRay, REF_ _result, _context))
			{
				todo_note(TXT("check if hit happens on proper side of presence link"));
				if (_segment.collide_at(localInLinkRay.get_end_t()))
				{
					_result.to_world_of(get_placement());
					_result.object = this;
					_result.presenceLink = nullptr;
					debug_pop_transform();
					return true;
				}
			}
			debug_pop_transform();
		}
	}
	return false;
}

void DoorInRoom::world_activate()
{
	if (is_world_active())
	{
		// already activated
		return;
	}
	if (!is_ready_to_world_activate())
	{
		// doesn't do anything for door in room... and we're lazy
		ready_to_world_activate();
	}
	an_assert(meshesInit, TXT("do you remember to init meshes for door?"));
	an_assert(door);
	an_assert(door->is_world_active());
	if (Room* room = inRoom)
	{
		DOOR_IN_ROOM_HISTORY(this, TXT("readd on activation"));
		room->remove_door(this);
		room->add_door(this);
		room->mark_nav_mesh_creation_required();
	}
	WorldObject::world_activate();
}

bool DoorInRoom::is_in_front_of(Plane const & _plane, float _minDist) const
{
	if (auto const * hole = get_hole())
	{
		Plane const planeLS = get_outbound_matrix().to_local(_plane);
		return hole->is_in_front_of(get_side(), get_hole_scale(), planeLS, _minDist);
	}
	else
	{
		return true;
	}
}

Vector3 DoorInRoom::get_hole_centre_WS() const
{
	if (auto const * hole = get_hole())
	{
		Vector2 holeScale = Vector2::one;
		if (door)
		{
			holeScale = door->get_hole_scale();
		}
		return get_placement().location_to_world(hole->get_centre(get_side()) * holeScale.to_vector3_xz(1.0f));
	}
	else
	{
		return get_placement().get_translation();
	}
}

void DoorInRoom::update_collision_bounding_boxes()
{
	movementCollision.update_bounding_box();
	preciseCollision.update_bounding_box();
	meshesReady = true;
}

void DoorInRoom::add_to(Nav::Mesh* _navMesh)
{
	if (door && ! door->get_door_type()->is_nav_connector())
	{
		return;
	}

	an_assert(Game::get()->get_nav_system()->is_in_writing_mode());

	outdate_nav();

	navDoorNode = Nav::Nodes::Point::get_one();
	navDoorNode->be_door_in_room(this);
	navDoorNode->place_WS(placement, nullptr);
	_navMesh->add(navDoorNode.get());
	navDoorNode->set_flags_from_nav_mesh();

	navEntranceNode = Nav::Nodes::Point::get_one();
	navEntranceNode->be_open_node(true, -Vector3::yAxis);
	navEntranceNode->place_WS(placement, nullptr);
	_navMesh->add(navEntranceNode.get());
	navEntranceNode->set_flags_from_nav_mesh();

#ifdef AN_DEVELOPMENT
	navDoorNode->set_name(NAME(door));
	navEntranceNode->set_name(NAME(doorEntrace));
#endif

	navSharedData = navDoorNode->connect(navEntranceNode.get());
}

void DoorInRoom::nav_connect_to(DoorInRoom* _other)
{
	if (door && !door->get_door_type()->is_nav_connector())
	{
		return;
	}
	if (_other && _other->door && !_other->door->get_door_type()->is_nav_connector())
	{
		return;
	}

	// if outdated for any reason (like we outdated
	if (navEntranceNode.get() && _other->navEntranceNode.get())
	{
		navEntranceNode->connect(_other->navEntranceNode.get());
	}
	else
	{
		error_dev_investigate(TXT("can't connect entrances as there are entrace nodes missing"));
	}
}

void DoorInRoom::outdate_nav()
{
	if (navSharedData.is_set())
	{
		navSharedData->outdate();
		navSharedData.clear();
	}
	if (navDoorNode.is_set())
	{
		navDoorNode->outdate();
		navDoorNode.clear();
	}
	if (navEntranceNode.is_set())
	{
		navEntranceNode->outdate();
		navEntranceNode.clear();
	}

	set_nav_group_id(NONE);
}

void DoorInRoom::start_crawling_destruction(bool _keepDoorInRoom)
{
	Room* inRoom = get_in_room();
	Room* room = get_room_on_other_side();

	bool deleteRoom = false;

	Game::get()->perform_sync_world_job(TXT("mark room to be deleted [crawling destruction]"), [room, &deleteRoom]()
	{
#ifdef AN_OUTPUT_WORLD_GENERATION
		output(TXT("issue crawling destruction of room r%p\"%S\""), room, room->get_name().to_char());
#endif
		if (room && !room->is_deletion_pending())
		{
			room->mark_to_be_deleted();
			deleteRoom = true;
		}
	});

	for_every_ptr(dir, room->get_all_doors())
	{
		// crawling destruction uses invisible doors too!
		if (dir->get_room_on_other_side() &&
			dir->get_room_on_other_side() != inRoom &&
			! dir->get_room_on_other_side()->is_deletion_pending())
		{
			dir->start_crawling_destruction();
		}
	}

	Game::get()->perform_sync_world_job(TXT("delete doors"), [this, _keepDoorInRoom]()
	{
		// delete both door in other rooms
		if (auto* other = get_door_on_other_side())
		{
			delete other;
		}
		delete_and_clear(door);
		// finally delete this
		if (!_keepDoorInRoom)
		{
			delete this;
		}
	});

	if (deleteRoom)
	{
		Game::get()->add_sync_world_job(TXT("destroy room and issue crawling destruction of door in it"), [room]()
		{
#ifdef AN_OUTPUT_WORLD_GENERATION
			output(TXT("crawling destruction - destroy actual room \"%S\""), room->get_name().to_char());
#endif
			// and destroy room
			delete room;
		});
	}
}

Range2 DoorInRoom::calculate_size() const
{
	return door && door->get_hole() ? door->get_hole()->calculate_size(side, get_hole_scale()) : Range2(Range(0.0f), Range(0.0f));
}

Range2 DoorInRoom::calculate_compatible_size() const
{
	return door? door->calculate_compatible_size(side) : Range2(Range(0.0f), Range(0.0f));
}

VR::Zone DoorInRoom::calc_vr_zone_outbound(Optional<float> const& _spaceRequiredBeyondDoor) const
{
	VR::Zone resultZone;
	float vrWidth = get_door()->calculate_vr_width();
	todo_note(TXT("we assume symetric doors, that's why there's zero in offset.x!"));
	float spaceRequiredBeyondDoor = _spaceRequiredBeyondDoor.get(Door::get_nominal_door_width().get(1000.0f)); // it won't fit if not known
	resultZone.be_rect(Vector2(vrWidth, spaceRequiredBeyondDoor), Vector2(0.0f, spaceRequiredBeyondDoor * 0.5f));
	resultZone.place_at(get_vr_space_placement().get_translation().to_vector2(), get_vr_space_placement().get_axis(Axis::Y).to_vector2());
	return resultZone;
}

VR::Zone DoorInRoom::calc_vr_zone_outbound_if_were_at(Transform const & _vrPlacement, Optional<float> const& _spaceRequiredBeyondDoor) const
{
	VR::Zone resultZone;
	float vrWidth = get_door()->calculate_vr_width();
	todo_note(TXT("we assume symetric doors, that's why there's zero in offset.x!"));
	float spaceRequiredBeyondDoor = _spaceRequiredBeyondDoor.get(Door::get_nominal_door_width().get(1000.0f)); // it won't fit if not known
	resultZone.be_rect(Vector2(vrWidth, spaceRequiredBeyondDoor), Vector2(0.0f, spaceRequiredBeyondDoor * 0.5f));
	resultZone.place_at(_vrPlacement.get_translation().to_vector2(), _vrPlacement.get_axis(Axis::Y).to_vector2());
	return resultZone;
}

bool DoorInRoom::has_world_active_room_on_other_side() const
{
	if (auto* ros = roomOnOtherSide.get())
	{
		return ros->is_world_active();
	}
	return false;
}

Room* DoorInRoom::get_world_active_room_on_other_side() const
{
	if (auto* ros = roomOnOtherSide.get())
	{
		return ros->is_world_active() ? ros : nullptr;
	}
	return nullptr;
}

void DoorInRoom::create_space_blocker(REF_ SpaceBlockers& _spaceBlockers, bool _localPlacement)
{
	if (is_placed() && get_door() && get_door()->get_door_type())
	{
		Transform at = _localPlacement ? Transform::identity : get_placement(); // outbound

		get_door()->get_door_type()->create_space_blocker(_spaceBlockers, at, side, get_hole_scale());
	}
}

bool DoorInRoom::same_with_orientation_for_vr(Transform const& _aVR, Transform const& _bVR, Optional<float> const& _translationThreshold, Optional<float> const& _scaleThreshold, Optional<float> const& _orientationThreshold)
{
	if (Transform::same_with_orientation(_aVR, _bVR, _translationThreshold, _scaleThreshold, _orientationThreshold))
	{
		return true;
	}
	Transform b180VR = Transform::do180.to_world(_bVR);
	if (Transform::same_with_orientation(_aVR, b180VR, _translationThreshold, _scaleThreshold, _orientationThreshold))
	{
		return true;
	}
	return false;
}

bool DoorInRoom::is_visible() const
{
	if (!door)
	{
		return false;
	}
	return door->is_visible();
}

bool DoorInRoom::can_see_through_it_now() const
{
	if (!door)
	{
		return false;
	}
	if (auto* dt = door->get_door_type())
	{
		if (dt->is_fake_one_way_window() && door->is_linked_as_b(this))
		{
			// skip one way window from B to A
			return false;
		}
	}
	if ((door->get_open_factor() != 0.0f || door->can_see_through_when_closed()) && !door->get_hole_scale().is_zero())
	{
		return true;
	}
	return false;
}

void DoorInRoom::update_scaled_placement_matrix()
{
	float scale = get_door_scale();
	if (scale == 1.0f)
	{
		scaledOutboundMatrix = outboundMatrix;
	}
	else
	{
		scaledOutboundMatrix = outboundMatrix.to_world(scale_matrix(Vector3::one * door->get_door_scale()));
	}
}
