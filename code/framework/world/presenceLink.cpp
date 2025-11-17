#include "presenceLink.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\performance\performanceUtils.h"

#include "..\module\moduleAppearance.h"
#include "..\module\moduleCollision.h"
#include "..\module\modulePresence.h"
#include "..\object\scenery.h"
#include "..\world\world.h"
#include "room.h"

#include "..\game\game.h"

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

using namespace Framework;

PresenceLinkBuildContext::PresenceLinkBuildContext()
{
	an_assert(Concurrency::ThreadManager::get() != nullptr, TXT("requires multi threading or at least thread manager"));
	presenceLinkListIdx = Concurrency::ThreadManager::get_current_thread_id();
}

PresenceLink::PresenceLink()
: owner(nullptr)
, ownerPresence(nullptr)
, inRoom(nullptr)
, nextInRoom(nullptr)
, prevInRoom(nullptr)
, nextInObject(nullptr)
, prevInObject(nullptr)
{
}

void PresenceLink::release_if_not_used()
{
	if (nextInRoom == nullptr &&
		prevInRoom == nullptr &&
		nextInObject == nullptr &&
		prevInObject == nullptr &&
		ownerPresence == nullptr &&
		inRoom == nullptr)
	{
#ifdef VERY_DETAILED_PRESENCE_LINKS
		{
			output(TXT("  RELEASE %p for %p"), this, owner);
		}
#endif
		MEASURE_PERFORMANCE_COLOURED(releasePresenceLink, Colour::black);
		// release this link
		release();
	}
}

PresenceLink* PresenceLink::create(PresenceLinkBuildContext const & _context, ModulePresence* _ownerPresence, PresenceLink* _linkTowardsOwner, Room* _inRoom, DoorInRoom* _throughDoor, Transform const & _transformPlacementToRoom, bool _isActualPresence, bool _validForCollision, bool _validForCollisionGradient, bool _validForCollisionDetection, bool _validForLightSources)
{
	if (_inRoom == nullptr || ! _inRoom->is_world_active())
	{
		// no presence link
		return nullptr;
	}
	an_assert_immediate(_ownerPresence != nullptr);

	PresenceLink* link;
	{
		MEASURE_PERFORMANCE_COLOURED(createPresenceLink_getOne, Colour::black);
		link = PresenceLink::get_one();
	}

	an_assert_immediate(link->nextInObject == nullptr && link->prevInObject == nullptr);
	an_assert_immediate(link->nextInRoom == nullptr && link->prevInRoom == nullptr);
	an_assert_immediate(link->owner == nullptr);
	an_assert_immediate(!link->invalidated);
	an_assert_immediate(!link->released);

	// setup link
#ifdef AN_DEVELOPMENT
	link->builtFor = _ownerPresence->get_owner();
	link->debugBuiltPresenceLinksAtCoreFrame = ::System::Core::get_frame();
#endif
	link->owner = _ownerPresence->get_owner();
#ifdef DETAILED_PRESENCE_LINKS
	if (link->owner)
	{
		output(TXT("PresenceLink::create %p for %p : %S"), link, link->owner, link->owner->ai_get_name().to_char());
	}
#endif
	link->ownerPresence = _ownerPresence;
	link->linkTowardsOwner = _linkTowardsOwner;
	link->inRoom = _inRoom;
	link->throughDoor = _throughDoor;
	link->isActualPresence = _isActualPresence;
	link->validForCollision = _validForCollision;
	link->validForCollisionGradient = _validForCollisionGradient;
	link->validForCollisionDetection = _validForCollisionDetection;
	link->validForLightSources = _validForLightSources;
	link->presenceOffset = _ownerPresence->presenceLinkCentreOS;
	link->presenceRadius = _ownerPresence->presenceLinkRadius;
	link->presenceCentreDistance = _ownerPresence->presenceLinkCentreDistance;
	link->presenceInSingleRoom = _ownerPresence->presenceInSingleRoom;
	link->transformPlacementToRoom = _transformPlacementToRoom;
	link->transformPlacementToRoomMat = _transformPlacementToRoom.to_matrix();
	link->placementInRoom = _transformPlacementToRoom.to_local(_ownerPresence->get_placement());
	link->placementInRoomMat = link->placementInRoom.to_matrix();
	link->placementInRoomForRendering = link->placementInRoom;
	link->placementInRoomForRenderingMat = link->placementInRoomMat;
	link->placementInRoomForPresencePrimitive = link->placementInRoom.to_world(link->presenceOffset);
	link->clipPlanes.clear();
	link->clipPlanesForCollision.clear();
	if (auto const * cm = link->owner->get_collision())
	{
		Range3 bbox = cm->get_movement_collision().get_bounding_box();
		if (&cm->get_movement_collision() != &cm->get_precise_collision())
		{
			bbox.include(cm->get_precise_collision().get_bounding_box());
		}
		link->collisionBoundingBox.construct_from_placement_and_range3(link->placementInRoom, bbox);
	}

	// add 
	_ownerPresence->add_presence_link(_context, link);
	_inRoom->add_presence_link(_context, link);

	an_assert_immediate(link->owner != nullptr);
	an_assert_immediate(link->owner == _ownerPresence->get_owner());

	return link;
}

void PresenceLink::release_for_building_in_room_ignore_for_object()
{
	// perform it in a loop to avoid long stack wandering
	PresenceLink* toRelease = this;
	while (toRelease)
	{
		// store next to release (in case we would or we would not release it)
		PresenceLink* nextToRelease = toRelease->nextInRoom;

		toRelease->internal_release_for_building_in_room_ignore_for_object();

		toRelease = nextToRelease;
	}
}

void PresenceLink::internal_release_for_building_in_room_ignore_for_object()
{
#ifdef VERY_DETAILED_PRESENCE_LINKS
	if (owner)
	{
		output(TXT("PresenceLink::internal_release_for_building_in_room_ignore_for_object %p for %p"), this, owner);
	}
#endif
	// was invalidated (just remove it) or requires building
	if (!owner || ! owner->get_presence() || !ownerPresence || ownerPresence->does_require_building_presence_links(PresenceLinkOperation::Clearing))
	{
		// relink
		if (nextInRoom)
		{
			nextInRoom->prevInRoom = prevInRoom;
		}
		if (prevInRoom)
		{
			prevInRoom->nextInRoom = nextInRoom;
		}
		// replace first
		if (!prevInRoom && inRoom)
		{
			inRoom->removed_first_presence_link(this, nextInRoom);
		}
		// clear
		nextInRoom = nullptr;
		prevInRoom = nullptr;
		inRoom = nullptr;

		release_if_not_used();
	}
#ifdef VERY_DETAILED_PRESENCE_LINKS
	else
	{
		output(TXT("  KEEP %p for %p"), this, owner);
	}
#endif
}

void PresenceLink::release_for_building_in_object_ignore_for_room()
{
	// perform it in a loop to avoid long stack wandering
	PresenceLink* toRelease = this;
	while (toRelease)
	{
		// store next to release (in case we would or we would not release it)
		PresenceLink* nextToRelease = toRelease->nextInRoom;

		toRelease->internal_release_for_building_in_object_ignore_for_room();

		toRelease = nextToRelease;
	}
}

void PresenceLink::internal_release_for_building_in_object_ignore_for_room()
{
#ifdef VERY_DETAILED_PRESENCE_LINKS
	if (owner)
	{
		output(TXT("PresenceLink::internal_release_for_building_in_object_ignore_for_room %p for %p"), this, owner);
	}
#endif
	{
		// relink
		if (nextInObject)
		{
			nextInObject->prevInObject = prevInObject;
		}
		if (prevInObject)
		{
			prevInObject->nextInObject = nextInObject;
		}
		// clear
		nextInObject = nullptr;
		prevInObject = nullptr;
		owner = nullptr;
		ownerPresence = nullptr;

		release_if_not_used();
	}
#ifdef VERY_DETAILED_PRESENCE_LINKS
	else
	{
		output(TXT("  KEEP %p for %p"), this, owner);
	}
#endif
}

void PresenceLink::sync_release_in_room_clear_in_object()
{
	auto* pl = this;
	an_assert_log_always(!prevInRoom, TXT("sync_release_in_room_clear_in_object"));

	// shouldn't ever happen but there are crashes
	while (pl->prevInRoom)
	{
		pl = pl->prevInRoom;
	}

	while (pl)
	{
		ASSERT_SYNC_OR(! pl->owner || !pl->owner->get_in_world() || pl->owner->get_in_world()->is_being_destroyed());

#ifdef VERY_DETAILED_PRESENCE_LINKS
		if (pl->owner)
		{
			output(TXT("PresenceLink::sync_release_in_room_clear_in_object %p for %p"), pl, pl->owner);
		}
#endif

		an_assert(pl->prevInRoom == nullptr, TXT("should start removal at first in room"));

		// store next to release
		PresenceLink* nextToRelease = pl->nextInRoom;
		pl->nextInRoom = nullptr;

		an_assert_log_always(pl->nextInObject == nullptr || (pl->owner && pl->owner->get_presence() && pl->owner->get_presence()->presenceLinks != nullptr));
		an_assert_log_always(pl->prevInObject == nullptr || (pl->owner && pl->owner->get_presence() && pl->owner->get_presence()->presenceLinks != nullptr));

		if (pl->owner &&
			pl->owner->get_presence())
		{
			pl->owner->get_presence()->sync_remove_presence_link(pl);
		}

		an_assert_log_always(pl->nextInObject == nullptr || pl->owner == nullptr);
		an_assert_log_always(pl->prevInObject == nullptr || pl->owner == nullptr);

#ifdef VERY_DETAILED_PRESENCE_LINKS
		{
			output(TXT("  RELEASE %p for %p"), pl, pl->owner);
		}
#endif

		// release this link
		pl->release();

		// release stored
		if (nextToRelease)
		{
			nextToRelease->prevInRoom = nullptr;
			pl = nextToRelease;
		}
		else
		{
			break;
		}
	}
}

void PresenceLink::invalidate_in_room()
{
	PresenceLink* pl = this;
	an_assert_log_always(!prevInRoom, TXT("invalidate_in_room"));

	// shouldn't ever happen but there are crashes
	while (pl->prevInRoom)
	{
		pl = pl->prevInRoom;
	}

	while (pl)
	{
		PresenceLink* nextToInvalidate = pl->nextInRoom;

		if (pl->owner)
		{
#ifdef AN_DEVELOPMENT
			pl->invalidated = true;
#endif

#ifdef DETAILED_PRESENCE_LINKS
			if (pl->owner)
			{
				output(TXT("PresenceLink::invalidate_in_room_objects_through_door %p for %p"), pl, pl->owner);
			}
#endif

			pl->owner = nullptr;
			pl->ownerPresence = nullptr;
			pl->linkTowardsOwner = nullptr;
		}

		if (nextToInvalidate)
		{
			pl = nextToInvalidate;
		}
		else
		{
			break;
		}
	}
}

void PresenceLink::invalidate_in_room_objects_through_door(DoorInRoom* _door)
{
	PresenceLink* pl = this;
	an_assert_log_always(!prevInRoom, TXT("invalidate_in_room_objects_through_door"));

	// shouldn't ever happen but there are crashes
	while (pl->prevInRoom)
	{
		pl = pl->prevInRoom;
	}

	while (pl)
	{
		PresenceLink* nextToInvalidate = pl->nextInRoom;

		if (pl->owner && pl->throughDoor == _door)
		{
#ifdef AN_DEVELOPMENT
			pl->invalidated = true;
#endif

#ifdef DETAILED_PRESENCE_LINKS
			if (pl->owner)
			{
				output(TXT("PresenceLink::invalidate_in_room_objects_through_door %p for %p"), pl, pl->owner);
			}
#endif

			pl->owner = nullptr;
			pl->ownerPresence = nullptr;
			pl->linkTowardsOwner = nullptr;
		}

		if (nextToInvalidate)
		{
			pl = nextToInvalidate;
		}
		else
		{
			break;
		}
	}
}

void PresenceLink::invalidate_object()
{
	PresenceLink* pl = this;
	an_assert_log_always(!prevInObject, TXT("invalidate_object"));

	// shouldn't ever happen but there are crashes
	while (pl->prevInObject)
	{
		pl = pl->prevInObject;
	}

	while (pl)
	{
		an_assert_immediate(pl->owner == nullptr || pl->ownerPresence->get_owner());
		an_assert_immediate(pl->owner == nullptr || !pl->invalidated);
		an_assert_immediate(pl->owner == nullptr || !pl->released);
#ifdef AN_DEVELOPMENT
		pl->invalidated = true;
#endif
#ifdef DETAILED_PRESENCE_LINKS
		if (pl->owner)
		{
			output(TXT("PresenceLink::invalidate_object %p for %p"), pl, pl->owner);
		}
#endif
	
		PresenceLink* nextToInvalidate = pl->nextInObject;

		pl->owner = nullptr;
		pl->ownerPresence = nullptr;
		pl->linkTowardsOwner = nullptr;

		if (nextToInvalidate)
		{
			an_assert_immediate(nextToInvalidate->ownerPresence == pl->ownerPresence || ! pl->ownerPresence || !nextToInvalidate->ownerPresence);
			pl = nextToInvalidate;
		}
		else
		{
			break;
		}
	}
}

void PresenceLink::on_get()
{
#ifdef AN_DEVELOPMENT
	released = false;
	invalidated = false;
#endif
	an_assert_immediate(nextInObject == nullptr && prevInObject == nullptr, TXT("should be removed from object before releasing, use proper release method"));
	an_assert_immediate(nextInRoom == nullptr && prevInRoom == nullptr, TXT("should be removed from room before releasing, use proper release method"));
	an_assert_immediate(!owner);
	nextInObject = nullptr;
	prevInObject = nullptr;
	nextInRoom = nullptr;
	prevInRoom = nullptr;
	owner = nullptr;
	ownerPresence = nullptr;
	linkTowardsOwner = nullptr;
	inRoom = nullptr;
	throughDoor = nullptr;
}

void PresenceLink::on_release()
{
#ifdef DETAILED_PRESENCE_LINKS
	if (owner)
	{
		output(TXT("PresenceLink::on_release %p for %p"), this, owner);
	}
#endif
	an_assert_immediate(nextInObject == nullptr && prevInObject == nullptr, TXT("should be removed from object before releasing, use proper release method"));
	an_assert_immediate(nextInRoom == nullptr && prevInRoom == nullptr, TXT("should be removed from room before releasing, use proper release method"));
	an_assert_immediate(!owner || owner->get_presence() == nullptr || owner->get_presence() == ownerPresence); // I have no idea what this assert should test
#ifdef AN_DEVELOPMENT
	released = true;
#endif
	nextInObject = nullptr;
	prevInObject = nullptr;
	nextInRoom = nullptr;
	prevInRoom = nullptr;
	owner = nullptr;
	ownerPresence = nullptr;
	linkTowardsOwner = nullptr;
	inRoom = nullptr;
	throughDoor = nullptr;
	presenceRadius = -10000.0f;
}

bool PresenceLink::check_segment_against(AgainstCollision::Type _againstCollision, REF_ Segment& _segment, REF_ CheckSegmentResult& _result, CheckCollisionContext& _context) const
{
	if (is_valid_for_collision())
	{
		return check_segment_against(get_modules_owner(), get_placement_in_room(), _againstCollision, _segment, _result, _context, this);
	}
	return false;
}

bool PresenceLink::check_segment_against(IModulesOwner* modulesOwner, Transform const & _placementWS, AgainstCollision::Type _againstCollision, REF_ Segment& _segment, REF_ CheckSegmentResult& _result, CheckCollisionContext& _context, PresenceLink const * _presenceLink)
{
	if (modulesOwner)
	{
		if (ModuleCollision* moduleCollision = modulesOwner->get_collision())
		{
			if (((_context.should_check_objects() && modulesOwner->get_as_object() &&
				  ((_context.should_check_actors() && modulesOwner->get_as_actor()) ||
				   (_context.should_check_items() && modulesOwner->get_as_item()) ||
				   (_context.should_check_scenery() && modulesOwner->get_as_scenery()) ||
				   (_context.should_check_room_scenery() && modulesOwner->get_as_scenery() && modulesOwner->get_as_scenery()->is_room_scenery()) ||
				   (! _context.should_check_actors() && ! _context.should_check_items() && ! _context.should_check_scenery() && ! _context.should_check_room_scenery()))) ||
				 (_context.should_check_temporary_objects() && modulesOwner->get_as_temporary_object())) &&
				_context.should_check(modulesOwner->get_as_collision_collidable_object()))
			{
				Segment localInLinkRay = _placementWS.to_local(_segment);
				debug_push_transform(_placementWS);
				float increaseSize = 0.0f;
				if (modulesOwner->get_as_item())
				{
					increaseSize += _context.get_increase_size_for_item();
				}
				if (auto* appearance = modulesOwner->get_appearance())
				{
					_context.use_pose_mat_bones_ref(appearance->get_final_pose_mat_MS_ref());
				}
				bool hit = moduleCollision->get_against_collision(_againstCollision).check_segment(REF_ localInLinkRay, REF_ _result, _context, increaseSize);
				_context.clear_pose_mat_bones_ref();
				if (hit)
				{
					todo_note(TXT("check if hit happens on proper side of presence link"));
					if (_segment.collide_at(localInLinkRay.get_end_t()))
					{
						_result.to_world_of(_placementWS);
						_result.object = modulesOwner->get_as_collision_collidable_object();
						_result.presenceLink = _presenceLink;
						debug_pop_transform();
						return true;
					}
				}
				debug_pop_transform();
			}
		}
	}
	return false;
}

void PresenceLink::update_placement_for_rendering_by(Transform const& _placementChange)
{
	placementInRoomForRendering = placementInRoomForRendering.to_world(_placementChange);
	placementInRoomForRenderingMat = placementInRoomForRendering.to_matrix();
}

Transform PresenceLink::transform_to_owners_room_space(Transform const& _placement) const
{
	Transform relToOwnerPlacement = placementInRoom.to_local(_placement);
	return ownerPresence->get_placement().to_world(relToOwnerPlacement);
}

void PresenceLink::log_development_info(LogInfoContext& _log) const
{
	_log.log(TXT("link info for %p for %p"), this, owner);
	/*
#ifdef AN_DEVELOPMENT
	auto* imo = owner;
	auto* plBuiltFor = builtFor;
	_log.log(TXT("imo built frame:%i"), debugBuiltPresenceLinksAtCoreFrame);
	if (imo)
	{
		_log.log(TXT("imo:%S (%S:%i) (%S:%i) (%S)"), imo->ai_get_name().to_char(),
			imo->get_presence()->didRequireClearingPresenceLinks ? TXT("REQ CLEAR") : TXT("don't clear"),
			imo->get_presence()->didRequireClearingPresenceLinksCoreFrame,
			imo->get_presence()->didRequireBuildingPresenceLinks ? TXT("REQ BUILD") : TXT("don't build"),
			imo->get_presence()->didRequireBuildingPresenceLinksCoreFrame,
			imo->is_advancement_suspended() ? TXT("ADV SUSP") : TXT("adv usual"));
	}
	if (plBuiltFor)
	{
		_log.log(TXT("presence link built for:%S (%S:%i) (%S:%i) (%S)"), builtFor->ai_get_name().to_char(),
			plBuiltFor->get_presence()->didRequireClearingPresenceLinks ? TXT("REQ CLEAR") : TXT("don't clear"),
			plBuiltFor->get_presence()->didRequireClearingPresenceLinksCoreFrame,
			plBuiltFor->get_presence()->didRequireBuildingPresenceLinks ? TXT("REQ BUILD") : TXT("don't build"),
			plBuiltFor->get_presence()->didRequireBuildingPresenceLinksCoreFrame,
			plBuiltFor->is_advancement_suspended() ? TXT("ADV SUSP") : TXT("adv usual"));
	}
#endif
	*/
}

#ifdef AN_DEVELOPMENT
void PresenceLink::check_if_object_has_its_own_presence_links(IModulesOwner* imo)
{
	/*
	if (auto* p = imo->get_presence())
	{
		PresenceLink* pl = p->presenceLinks;
		while (pl)
		{
			if (pl->owner && (pl->builtFor != imo || pl->builtFor != pl->owner))
			{
				auto* plBuiltFor = pl->builtFor;
				error(TXT("imo:%S (%S:%i) (%S:%i) (%S)"), imo->ai_get_name().to_char(),
					imo->get_presence()->didRequireClearingPresenceLinks ? TXT("REQ CLEAR") : TXT("don't clear"),
					imo->get_presence()->didRequireClearingPresenceLinksCoreFrame,
					imo->get_presence()->didRequireBuildingPresenceLinks ? TXT("REQ BUILD") : TXT("don't build"),
					imo->get_presence()->didRequireBuildingPresenceLinksCoreFrame,
					imo->is_advancement_suspended() ? TXT("ADV SUSP") : TXT("adv usual"));
				error(TXT("imo built frame:%i"), p->debugBuiltPresenceLinksAtCoreFrame);
				error(TXT("presence link built for:%S (%S:%i) (%S:%i) (%S)"), pl->builtFor->ai_get_name().to_char(),
					plBuiltFor->get_presence()->didRequireClearingPresenceLinks ? TXT("REQ CLEAR") : TXT("don't clear"),
					plBuiltFor->get_presence()->didRequireClearingPresenceLinksCoreFrame,
					plBuiltFor->get_presence()->didRequireBuildingPresenceLinks ? TXT("REQ BUILD") : TXT("don't build"),
					plBuiltFor->get_presence()->didRequireBuildingPresenceLinksCoreFrame,
					plBuiltFor->is_advancement_suspended() ? TXT("ADV SUSP") : TXT("adv usual"));
				error(TXT("presence link built frame:%i"), pl->debugBuiltPresenceLinksAtCoreFrame);
				an_assert_immediate(false);
			}
			pl = pl->nextInObject;
		}
	}
	*/
}
#endif
