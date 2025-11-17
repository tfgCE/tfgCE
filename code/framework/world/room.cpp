#include "room.h"

#include "..\..\core\types\names.h"

#include "door.h"
#include "doorInRoom.h"
#include "world.h"
#include "presenceLink.h"
#include "roomRegionVariables.inl"
#include "roomType.h"
#include "worldAddress.h"

#include "roomGenerators\roomGenerator_roomPieceCombinerInfo.h"

#include "..\game\bullshotSystem.h"

#include "..\appearance\mesh.h"
#include "..\collision\checkCollisionCache.h"
#include "..\collision\collisionModel.h"
#include "..\debug\debugVisualiserUtils.h"
#include "..\debug\previewGame.h"
#include "..\module\moduleAppearance.h"
#include "..\module\moduleCollision.h"
#include "..\object\object.h"
#include "..\object\scenery.h"

#include "..\nav\navMesh.h"
#include "..\nav\navSystem.h"
#include "..\nav\tasks\navTask_BuildNavMesh.h"
#include "..\nav\tasks\navTask_DestroyNavMesh.h"

#include "..\render\renderContext.h"

#include "..\library\library.h"
#include "..\library\libraryStoredReplacer.inl"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\performance\performanceUtils.h"

#include "..\..\core\concurrency\threadSystemUtils.h"
#include "..\..\core\concurrency\scopedMRSWLock.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\vertexFormat.h"
#include "..\..\core\mesh\mesh3d.h"
#include "..\..\core\mesh\mesh3dBuilder.h"
#include "..\..\core\mesh\combinedMesh3dInstanceSet.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"

#include "..\game\game.h"
#include "..\game\gameConfig.h"
#include "..\jobSystem\jobSystem.h"

#include "..\debug\testConfig.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define AN_INSPECT_NEW_ROOM
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define INSPECT_DESTROY
	#endif
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define INSPECT_REMOVE_FROM_WORLD
	#endif
#endif

#ifdef AN_OUTPUT_WORLD_GENERATION
	#define AN_OUTPUT_READY_FOR_GAME
#endif

//#define AN_OUTPUT_GATHERED_TO_READY_FOR_GAME

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

DEFINE_STATIC_NAME(skyBox);

// environment name
DEFINE_STATIC_NAME(own);

// collision flags
DEFINE_STATIC_NAME(worldSolidFlags);

// for plane generation
DEFINE_STATIC_NAME(nav);
DEFINE_STATIC_NAME(convexPolygon);
DEFINE_STATIC_NAME(navFlags);

// generation issues
DEFINE_STATIC_NAME(room_failedGeneration);

// room tags
DEFINE_STATIC_NAME(vrElevatorAltitude);
DEFINE_STATIC_NAME(vrCorridorNested);
DEFINE_STATIC_NAME(inaccessible);
DEFINE_STATIC_NAME(vrInaccessible);

// POI tags
DEFINE_STATIC_NAME(volumetricLimit);

// POI names
DEFINE_STATIC_NAME_STR(vrAnchor, TXT("vr anchor"));

// test tags
DEFINE_STATIC_NAME(testRoom);

//

struct StaticRoomsData
{
	struct PerThread
	{
		Array<Room*> rooms;
	};
	Array<PerThread*> perThread;

	Array<Room*> combinedRooms;

	StaticRoomsData() {}
	~StaticRoomsData()
	{
		for_every_ptr(pt, perThread)
		{
			delete pt;
		}
	}

	static void store_room_to_combine(Room* _room, int _threadIdx)
	{
		an_assert(s_one);
		s_one->perThread[_threadIdx]->rooms.push_back_unique(_room);
	}

	static StaticRoomsData* s_one;
};

StaticRoomsData* StaticRoomsData::s_one = nullptr;

//

REGISTER_FOR_FAST_CAST(Room);

const float Room::c_timeSinceSeenByPlayerLimit = 0.2f;
const float Room::c_timeSinceVisitedByPlayerLimit = 0.2f;

void Room::initialise_static()
{
	an_assert(!StaticRoomsData::s_one);
	StaticRoomsData::s_one = new StaticRoomsData();
}

void Room::close_static()
{
	an_assert(StaticRoomsData::s_one);
	delete_and_clear(StaticRoomsData::s_one);
}

void Room::before_building_presence_links()
{
	an_assert(StaticRoomsData::s_one);
	while (StaticRoomsData::s_one->perThread.get_size() < Concurrency::ThreadSystemUtils::get_number_of_cores())
	{
		StaticRoomsData::s_one->perThread.push_back(new StaticRoomsData::PerThread());
	}
	for_every_ptr(pt, StaticRoomsData::s_one->perThread)
	{
		pt->rooms.clear();
	}
}

Array<Room*> const& Room::get_rooms_to_combine_presence_links_post_building_presence_links()
{
	MEASURE_PERFORMANCE(getRoomsToCombinePresenceLinks);

	an_assert(StaticRoomsData::s_one);
	Array<Room*>& rooms = StaticRoomsData::s_one->combinedRooms;
	rooms.clear();

	for_every_ptr(pt, StaticRoomsData::s_one->perThread)
	{
		for_every_ptr(r, pt->rooms)
		{
			rooms.push_back_unique(r);
		}
	}

	return rooms;
}

Room::Room(SubWorld* _inSubWorld, Region* _inRegion, RoomType const * _roomType, Random::Generator const & _rg)
: SafeObject<Room>(nullptr)
, roomType(nullptr)
, inSubWorld(nullptr)
, inRegion(nullptr)
, combinedAppearance(nullptr)
, presenceLinks(nullptr)
, environmentUsage(this)
, ownEnvironment(nullptr)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Room [new] r%p"), this);
#endif

	ROOM_HISTORY(this, TXT("created r%p [%i]"), this, get_activation_group()? get_activation_group()->get_id() : -1);

	set_individual_random_generator(_rg, ! _rg.is_zero_seed());

	make_safe_object_available(this);

	an_assert(Collision::DefinedFlags::has(NAME(worldSolidFlags)));
	set_collision_flags(Collision::DefinedFlags::get_existing_or_basic(NAME(worldSolidFlags)));

	an_assert(_inSubWorld && _inSubWorld->get_in_world());
	_inSubWorld->get_in_world()->add(this);
	_inSubWorld->add(this);
	if (_inRegion)
	{
		_inRegion->add(this);
	}

	// create empty thread safe presence links holder
	threadPresenceLinks.set_size(Concurrency::ThreadSystemUtils::get_number_of_cores());
	for_every(link, threadPresenceLinks)
	{
		*link = nullptr;
	}

	if (_roomType)
	{
		set_room_type(_roomType);
	}

#ifdef AN_INSPECT_NEW_ROOM
	output(TXT("new room %S (%p) type \"%S\""), get_individual_random_generator().get_seed_string().to_char(), this, roomType? roomType->get_name().to_string().to_char() : TXT("<no type>"));
#endif
}

Room::~Room()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Room [delete] r%p"), this);
#endif
#ifdef INSPECT_DESTROY
	output(TXT("destroy room r%p"), this);
#endif
	ASSERT_SYNC_OR(! get_in_world() || get_in_world()->is_being_destroyed());

	make_safe_object_unavailable();

	mark_to_be_deleted();
	invalidate_presence_links();

	clear_inside();
	if (inSubWorld)
	{
		inSubWorld->remove(this);
	}
	if (auto* inWorld = get_in_world())
	{
		inWorld->remove(this);
	}
	if (inRegion)
	{
		inRegion->remove(this);
	}
	while (!allDoors.is_empty())
	{
		delete allDoors.get_first();
	}
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Room [deleted] r%p"), this);
#endif
}

void Room::invalidate_presence_links()
{
	if (presenceLinks)
	{
		presenceLinks->invalidate_in_room();
		presenceLinks = nullptr;
	}
}

void Room::sync_destroy_all_doors()
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	while (!allDoors.is_empty())
	{
		delete allDoors.get_first();
	}
}

void Room::make_safe_object_available(Room* _object)
{
	SafeObject<Room>::make_safe_object_available(_object);
	SafeObject<Collision::ICollidableObject>::make_safe_object_available(_object);
}

void Room::make_safe_object_unavailable()
{
	SafeObject<Room>::make_safe_object_unavailable();
	SafeObject<Collision::ICollidableObject>::make_safe_object_unavailable();
}

void Room::set_room_type(RoomType const * _roomType)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Room [set type] r%p is \"%S\""), this, _roomType? _roomType->get_name().to_string().to_char() : TXT("--"));
#endif
	an_assert(!is_generated());
	tags.base_on(nullptr);
	environmentOverlays.clear();
	roomType = _roomType;
	predefinedRoomOcclusionCulling.clear();
	if (roomType)
	{
		shouldHaveNavMesh = roomType->should_have_nav_mesh();
		shouldBeAvoidedToNavigateInto = roomType->should_be_avoided_to_navigate_into();
		tags.base_on(&roomType->get_tags());
		set_door_type_replacer(roomType->get_door_type_replacer());
		environmentOverlays.add_from(roomType->get_environment_overlays());
		// directly from room type only!
		if (roomType->get_tags().has_tag(NAME(vrElevatorAltitude)))
		{
			vrElevatorAltitude = roomType->get_tags().get_tag(NAME(vrElevatorAltitude));
		}
		predefinedRoomOcclusionCulling = roomType->get_predefined_occlusion_culling();
	}
	setup_reverb_from_type();
	if (individualRandomGeneratorSet)
	{
		// without individual random generator it doesn't make sense
		update_door_type_replacer(); // from type, from room generator info
	}

	pulseEnvironment = false;
	if (roomType)
	{
		RoomType::UseEnvironment const& useEnvironment = roomType->get_use_environment();
		if (auto* pulseET = useEnvironment.pulseEnvironmentType.get())
		{
			pulseEnvironment = true;
		}
	}

	ROOM_HISTORY(this, TXT("room type \"%S\""), roomType? roomType->get_name().to_string().to_char() : TXT("--"));
}

void Room::destroy_all_objects_inside()
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());

#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
	output(TXT("destroy_all_objects_inside of room \"%S\""), get_name().to_char());
#endif
	{
		Concurrency::ScopedMRSWLockWrite lock(objectsLock);
		while (! allObjects.is_empty())
		{
#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
			output(TXT("  cease to exist? o%p \"%S\""), allObjects.get_first(), allObjects.get_first()->ai_get_name().to_char());
#endif
			auto* imo = allObjects.get_first();
			// first try to remove it while ceasing to exist, this will detach stuff, cease objects dependent on this one and so on
			ModulePresence* presence = imo->get_presence();
			an_assert(presence);
			{
				// if we're attached to an object that is in a different room (that is not destruction pending), move us there and clear being destroyed
				if (auto* at = presence->get_attached_to())
				{
					// go to the top one as eventually we're attached there
					while (true)
					{
						if (auto* higher = at->get_presence()->get_attached_to())
						{
							at = higher;
						}
						else
						{
							break;
						}
					}
					{
						auto* atr = at->get_presence()->get_in_room();
						if (atr && atr != this && ! atr->is_deletion_pending())
						{
							imo->restore_being_advanceable(IModulesOwner::NoLongerAdvanceableFlag::RoomBeingDestroyed);
							if (imo->is_advanceable())
							{
								presence->remove_from_room();
								presence->add_to_room(atr);
								presence->issue_reattach();
								continue; // skip ceasing
							}
						}
					}
				}
			}
			imo->cease_to_exist(true);
			// remove it from the room explicitly, don't take any chances, cease to exist could have keep it in the room but we want it to be gone for good
			// how this could happen so this is required?
			//	object ceases to exist without being removed from world, is added to destruction pending queue
			//	room in which it was is set to be destroyed
			//	cease_to_exist called above won't remove it as the object is marked as already destroyed (cease_to_exist calls destroy)
			//	this object will still remember that it is in this room
			//	when the world is to be destroyed, this object will try to leave this room, only to access memory that no longer describes this room
			{
				// if objects are detached, this will make no difference (well, will consume a bit of CPU)
				presence->detach_attached();
				presence->detach();
#ifdef INSPECT_REMOVE_FROM_WORLD
				output(TXT("remove from room on room destroy"));
				output(TXT("       from room %S"), presence->get_in_room() ? presence->get_in_room()->get_name().to_char() : TXT("--"));
#endif
				presence->remove_from_room(Names::destroyed);
			}
			if (!allObjects.is_empty() &&
				allObjects.get_first() == imo)
			{
				an_assert_log_always(false, TXT("object o%p not removed from r%p on destroy all objects inside"), imo, this);
				error(TXT("object (%S) ordered to cease to exist with removal from the world, but stayed in the room (%S)"), imo->get_name().to_char(), get_name().to_char());
				allObjects.pop_front();
			}
		}
	}

	{
		Concurrency::ScopedMRSWLockWrite lock(temporaryObjectsLock);
		for_every_ptr(temporaryObject, temporaryObjectsToActivate)
		{
			temporaryObject->mark_to_deactivate(true);
		}
		temporaryObjectsToActivate.clear();

		while (!allTemporaryObjects.is_empty())
		{
			allTemporaryObjects.get_first()->cease_to_exist(true);
		}
	}
#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
	output(TXT("destroy_all_objects_inside of room \"%S\" [done]"), get_name().to_char());
#endif
}

Meshes::Mesh3DInstanceSet & Room::access_appearance_for_rendering()
{
	return combinedAppearance ? combinedAppearance->access_as_instance_set() : appearance;
}

Meshes::Mesh3DInstanceSet const & Room::get_appearance_for_rendering() const
{
	return combinedAppearance ? combinedAppearance->get_as_instance_set() : appearance;
}

void Room::clear_inside()
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());

#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
	output(TXT("clear_inside of room \"%S\""), get_name().to_char());
#endif

	defer_combined_apperance_delete();

	for_every_ptr(roomPartInstance, roomPartInstances)
	{
		delete roomPartInstance;
	}
	roomPartInstances.clear();
	appearance.clear();
	appearanceSpaceBlockers.clear();
	spaceBlockers.clear();
	movementCollision.clear();
	preciseCollision.clear();
	usedMeshes.clear();
	pois.clear();

	set_environment(nullptr);
	delete_and_clear(ownEnvironment);
	if (presenceLinks)
	{
		presenceLinks->sync_release_in_room_clear_in_object();
		presenceLinks = nullptr;
	}

	destroy_all_objects_inside();

	Game::get()->get_nav_system()->new_nav_mesh_for_room_no_longer_needed(this);

	{
		Concurrency::ScopedMRSWLockWrite lock(navMeshesLock);

		Game::get()->get_nav_system()->cancel_related_to(this);

		for_every_ref(navMesh, navMeshes)
		{
			Game::get()->get_nav_system()->add(Nav::Tasks::DestroyNavMesh::new_task(navMesh));
		}
		navMeshes.clear();
	}

#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
	output(TXT("clear_inside of room - done \"%S\""), get_name().to_char());
#endif
}

void Room::advance__clear_presence_links(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(Room, room)
	{
		MEASURE_PERFORMANCE(clearPresenceLinks);

		// release all presence links
		if (room->presenceLinks)
		{
			room->presenceLinks->release_for_building_in_room_ignore_for_object();
		}
	}
}

bool Room::does_require_combine_presence_links() const
{
#ifdef VERY_DETAILED_PRESENCE_LINKS
	String tpl;
	for_every(presenceLink, threadPresenceLinks)
	{
		tpl += *presenceLink ? TXT("+") : TXT("-");
	}
	output(TXT("r%p does require combine presence links %S"), this, tpl.to_char());
#endif
	for_every(presenceLink, threadPresenceLinks)
	{
		if (*presenceLink)
		{
			return true;
		}
	}
	return false;
}

void Room::advance__combine_presence_links(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(Room, room)
	{
		MEASURE_PERFORMANCE(combinePresenceLinks);

		PresenceLink* lastLink = room->presenceLinks;

		// add to common list and remove thread presence links
		for_every(presenceLink, room->threadPresenceLinks)
		{
#ifdef VERY_DETAILED_PRESENCE_LINKS
			output(TXT("r%p combine presence link @%i %c"), room, for_everys_index(presenceLink), *presenceLink? '+' : '-');
#endif
			if (*presenceLink)
			{
				an_assert_log_always((*presenceLink)->prevInRoom == nullptr, TXT("should not be connected at this point"));
				if (lastLink)
				{
					while (lastLink->nextInRoom)
					{
						lastLink = lastLink->nextInRoom;
					}
					an_assert_log_always(lastLink->nextInRoom == nullptr, TXT("should not be connected at this point"));
					lastLink->nextInRoom = *presenceLink;
					(*presenceLink)->prevInRoom = lastLink;
				}
				else
				{
					an_assert_log_always(room->presenceLinks == nullptr, TXT("should not be connected at this point"));
					room->presenceLinks = *presenceLink;
				}
				lastLink = *presenceLink;
			}
			*presenceLink = nullptr;
		}
	}
}

bool Room::does_require_finalise_frame() const 
{
#ifdef AN_DEVELOPMENT
	FOR_EVERY_DOOR_IN_ROOM(door, this)
	{
		an_assert(!door->has_world_active_room_on_other_side() || door->get_other_room_transform().is_ok(), TXT("other room transform is not ok dr'%p"), door);
	}
#endif

	if (AVOID_CALLING_ACK_ get_distance_to_recently_seen_by_player() <= 1)
	{
		// ready for rendering
		return true;
	}

	if (sounds.does_require_advance() ||
		pulseEnvironment ||
		!temporaryObjectSpawnBlocking.is_empty())
	{
		return true;
	}

	return false;
}

void Room::advance__finalise_frame(IMMEDIATE_JOB_PARAMS)
{
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(Room, room)
	{
		MEASURE_PERFORMANCE(roomFinaliseFrame);

		float const deltaTime = ac->get_delta_time();

		{
			Concurrency::ScopedSpinLock lock(room->temporaryObjectSpawnBlockingLock);

			for (int i = 0; i < room->temporaryObjectSpawnBlocking.get_size(); ++i)
			{
				auto& tosb = room->temporaryObjectSpawnBlocking[i];
				if (tosb.timeLeft.is_set())
				{
					tosb.timeLeft = tosb.timeLeft.get() - deltaTime;
					if (tosb.timeLeft.get() <= 0.0f)
					{
						room->temporaryObjectSpawnBlocking.remove_fast_at(i);
						--i;
					}
				}
			}
		}

		if (room->roomType)
		{
			RoomType::UseEnvironment const & useEnvironment = room->roomType->get_use_environment();
			if (auto* pulseET = useEnvironment.pulseEnvironmentType.get())
			{
				if (room->ownEnvironment && room->ownEnvironment->get_environment_type())
				{
					for_every(param, pulseET->get_info().get_params().get_params())
					{
						if (auto *oeParam = room->ownEnvironment->get_environment_type()->get_info().get_params().get_param(param->get_name()))
						{
							EnvironmentParam epCopy = *oeParam;
							epCopy.lerp_with(0.5f + 0.5f * sin_deg(360.0f * mod(System::Core::get_timer(), useEnvironment.pulsePeriod)), *param);
							if (auto * exParam = room->ownEnvironment->access_info().access_params().access_param(param->get_name()))
							{
								*exParam = epCopy;
							}
						}
					}
				}
				else
				{
					error(TXT("room of type \"%S\" with pulse environment but without own environment or own environment type"), room->roomType->get_name().to_string().to_char());
				}
			}
		}

		// ready environments for rendering
		room->environmentUsage.ready_for_rendering(deltaTime);

		room->sounds.advance(deltaTime);
	}
}

void Room::add_presence_link(PresenceLinkBuildContext const & _context, PresenceLink* _link)
{
#ifdef VERY_DETAILED_PRESENCE_LINKS
	output(TXT("r%p add presence link (to combine @%i) %p for %p"), this, _context.presenceLinkListIdx, _link, _link->get_modules_owner());
#endif
	StaticRoomsData::store_room_to_combine(this, _context.presenceLinkListIdx);
	PresenceLink*& threadPresenceLink = threadPresenceLinks[_context.presenceLinkListIdx];
	an_assert_log_always(_link->prevInRoom == nullptr);
	_link->nextInRoom = threadPresenceLink;
	if (threadPresenceLink)
	{
		threadPresenceLink->prevInRoom = _link;
	}
	threadPresenceLink = _link;
}

void Room::removed_first_presence_link(PresenceLink* _link, PresenceLink* _replaceWith)
{
	an_assert_log_always(presenceLinks == _link, TXT("this should be the first!"));
	an_assert_log_always(_replaceWith == nullptr || _replaceWith->prevInRoom == nullptr, TXT("this should be the first!"));
	presenceLinks = _replaceWith;
}

void Room::sync_remove_presence_link(PresenceLink* _link)
{
	ASSERT_SYNC; // should be called only during destruction of a room! and that should happen only in sync

	auto* presenceLink = presenceLinks;
	while (presenceLink)
	{
		if (presenceLink == _link)
		{
			if (presenceLink->prevInRoom)
			{
				presenceLink->prevInRoom->nextInRoom = presenceLink->nextInRoom;
			}
			else
			{
				presenceLinks = presenceLink->nextInRoom;
			}
			if (presenceLink->nextInRoom)
			{
				presenceLink->nextInRoom->prevInRoom = presenceLink->prevInRoom;
			}
			presenceLink->prevInRoom = nullptr;
			presenceLink->nextInRoom = nullptr;
			return;
		}
		presenceLink = presenceLink->nextInRoom;
	}

	an_assert(false, TXT("not found?"));
}

RoomPartInstance* Room::add_room_part(RoomPart const * _roomPart, Transform const & _placement)
{
	RoomPartInstance* roomPartInstance = new RoomPartInstance(_roomPart);
	roomPartInstances.push_back(roomPartInstance);
	roomPartInstance->place(this, _placement);
	return roomPartInstance;
}

void Room::add_mesh(Mesh const * _mesh, Transform const & _placement, OUT_ Meshes::Mesh3DInstance** _meshInstance, OUT_ int* _movementCollisionId, OUT_ int* _preciseCollisionId)
{
	Meshes::Mesh3DInstance* meshInstance = nullptr;
	apply_replacer_to_mesh(REF_ _mesh);
	if (!_mesh)
	{
		return;
	}
	usedMeshes.push_back(UsedLibraryStored<Mesh>(_mesh));
	if (Skeleton const * skeleton = _mesh->get_skeleton())
	{
		meshInstance = access_appearance().add(_mesh->get_mesh(), skeleton->get_skeleton());
	}
	else
	{
		meshInstance = access_appearance().add(_mesh->get_mesh());
	}
	assign_optional_out_param(_meshInstance, meshInstance);
	meshInstance->set_placement(_placement);
	
	MaterialSetup::apply_material_setups(_mesh->get_material_setups(), *meshInstance);

	if (CollisionModel const * cm = _mesh->get_movement_collision_model())
	{
		Collision::ModelInstance* collisionInstance = access_movement_collision().add(cm->get_model());
		mark_requires_update_bounding_box();
		assign_optional_out_param(_movementCollisionId, collisionInstance->get_id_within_set());
		collisionInstance->set_placement(_placement);
	}

	if (CollisionModel const * cm = _mesh->get_precise_collision_model())
	{
		Collision::ModelInstance* collisionInstance = access_precise_collision().add(cm->get_model());
		mark_requires_update_bounding_box();
		assign_optional_out_param(_preciseCollisionId, collisionInstance->get_id_within_set());
		collisionInstance->set_placement(_placement);
	}

	if (!_mesh->get_pois().is_empty())
	{
		update_bounding_box(); // we may need it for POIs
		Random::Generator rg = get_individual_random_generator();
		rg.advance_seed(239478 + usedMeshes.get_size(), 234 * pois.get_size());
		pois.make_space_for_additional(_mesh->get_pois().get_size());
		for_every_ref(poi, _mesh->get_pois())
		{
			pois.push_back(PointOfInterestInstancePtr(Game::get()->get_customisation().create_point_of_interest_instance()));
			pois.get_last()->access_tags().set_tags_from(markPOIsInside);
			pois.get_last()->create_instance(rg.spawn().temp_ref(), poi, this, nullptr, pois.get_size() - 1, _placement, _mesh, meshInstance);
			poisRequireAdvancement |= pois.get_last()->does_require_advancement();
		}
	}
	if (!_mesh->get_space_blockers().blockers.is_empty())
	{
		Concurrency::ScopedMRSWLockWrite lockasb(appearanceSpaceBlockersLock);
		appearanceSpaceBlockers.add(_mesh->get_space_blockers(), _placement);
	}
	if (is_world_active())
	{
		update_volumetric_limit();
	}
}

void Room::combine_appearance()
{
	defer_combined_apperance_delete();

	Meshes::CombinedMesh3DInstanceSet* newlyCombinedAppearance = new Meshes::CombinedMesh3DInstanceSet();
	newlyCombinedAppearance->combine(appearance);
	combinedAppearance = newlyCombinedAppearance;

	auto& iset = combinedAppearance->get_as_instance_set();
	for_count(int, i, iset.get_instances_num())
	{
		if (auto* inst = iset.get_instance(i))
		{
			if (auto* m3d = fast_cast<Meshes::Mesh3D>(inst->get_mesh()))
			{
				m3d->mark_to_load_to_gpu();
			}
		}
	}
}

void Room::update_bounding_box()
{
	if (requiresUpdateBoundingBox)
	{
		access_movement_collision().update_bounding_box();
		access_precise_collision().update_bounding_box();
		requiresUpdateBoundingBox = false;
	}
}

void Room::add_door(DoorInRoom* _door)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("add dr'%p to room r%p"), _door, this);
#endif

	ROOM_HISTORY(this, TXT("adding dr'%p"), _door);
	DOOR_IN_ROOM_HISTORY(_door, TXT("adding to r%p%S"), this, _door->get_door() && _door->get_door()->is_world_active() ? TXT(" [world active]") : TXT(""));
	assert_we_may_modify_doors();

	Concurrency::ScopedMRSWLockWrite lock(doorsLock);

	an_assert(!_door->has_room_on_other_side() || _door->get_other_room_transform().is_ok() || Game::get()->does_want_to_cancel_creation(), TXT("other room transform is not ok (and not wanting to end creation)"));

	allDoors.push_back(_door);
	if (_door->get_door() && _door->get_door()->is_world_active())
	{
		doors.push_back(_door);
		update_small_room();
	}
}

void Room::remove_door(DoorInRoom* _door)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("remove dr'%p from room r%p"), _door, this);
#endif

	ROOM_HISTORY(this, TXT("removing dr'%p"), _door);
	DOOR_IN_ROOM_HISTORY(_door, TXT("removing from r%p%S"), this, _door->get_door() && _door->get_door()->is_world_active() ? TXT(" [world active]") : TXT(""));
	assert_we_may_modify_doors();

	if (presenceLinks)
	{
		presenceLinks->invalidate_in_room_objects_through_door(_door);
	}

	Concurrency::ScopedMRSWLockWrite lock(doorsLock, true);

	doors.remove_fast(_door);
	allDoors.remove_fast(_door);
	update_small_room();
}

void Room::replace_door(DoorInRoom* _replace, DoorInRoom* _with)
{
	ROOM_HISTORY(this, TXT("replacing dr'%p with dr'%p"), _replace, _with);
	DOOR_IN_ROOM_HISTORY(_replace, TXT("replaced in r%p with dr'%p"), this, _with);
	DOOR_IN_ROOM_HISTORY(_with, TXT("replaced r%p in dr'%p"), _replace, this);
	assert_we_may_modify_doors();

	Concurrency::ScopedMRSWLockWrite lock(doorsLock);

	for_every(door, doors)
	{
		if (*door == _replace)
		{
			*door = _with;
			break;
		}
	}
	for_every(door, allDoors)
	{
		if (*door == _replace)
		{
			*door = _with;
			break;
		}
	}
}

void Room::add_object(IModulesOwner* _object)
{
	assert_we_may_modify_objects();

	if (Object* asObject = fast_cast<Object>(_object))
	{
		Concurrency::ScopedMRSWLockWrite lock(objectsLock);
		an_assert(!allObjects.does_contain(asObject), TXT("object should not be already in room"));
		allObjects.push_back(asObject);
		if (asObject->is_world_active())
		{
			objects.push_back(asObject);
		}
	}
	else if (TemporaryObject* asTemporaryObject = fast_cast<TemporaryObject>(_object))
	{
		Concurrency::ScopedMRSWLockWrite lock(temporaryObjectsLock);
		an_assert(!allTemporaryObjects.does_contain(asTemporaryObject), TXT("object should not be already in room"));
		allTemporaryObjects.push_back(asTemporaryObject);
		if (asTemporaryObject->is_active())
		{
			temporaryObjects.push_back(asTemporaryObject);
		}
	}
	else
	{
		todo_important(TXT("implement_"));
	}
	/*
	unowned Projectile? asProjectile = _object as unowned Projectile;
	if (asProjectile != null)
	{
		an_assert(! projectiles.contains(asProjectile));
		projectiles.add(asProjectile);
	}
	*/
	//aiObjects.add(_object);
}

void Room::remove_object(IModulesOwner* _object)
{
	scoped_call_stack_info(TXT("Room::remove_object"));
	scoped_call_stack_info_str_printf(TXT("room r%p"), this);
#ifdef DEBUG_WORLD_LIFETIME_INDICATOR
	scoped_call_stack_info_str_printf(TXT("__lifetimeIndicator %S"), __lifetimeIndicator.to_char());
#endif
	scoped_call_stack_info_str_printf(TXT("object o%p"), _object);
#ifdef DEBUG_WORLD_LIFETIME_INDICATOR
	scoped_call_stack_info_str_printf(TXT("__lifetimeIndicator %S"), _object? _object->__lifetimeIndicator.to_char() : TXT("no-obj"));
#endif

#ifdef DEBUG_WORLD_LIFETIME_INDICATOR
	REMOVE_AS_SOON_AS_POSSIBLE_ //uncomment this output(TXT("on Room::remove_object __lifetimeIndicator %S"), __lifetimeIndicator.to_char());
#endif
	assert_we_may_modify_objects();

	if (Object* asObject = fast_cast<Object>(_object))
	{
		scoped_call_stack_info(TXT("as object"));
		Concurrency::ScopedMRSWLockWrite lock(objectsLock, true);
		objects.remove(asObject);
		allObjects.remove_fast(asObject);
	}
	else if (TemporaryObject* asTemporaryObject = fast_cast<TemporaryObject>(_object))
	{
		scoped_call_stack_info(TXT("as temporary object"));
		Concurrency::ScopedMRSWLockWrite lock(temporaryObjectsLock, true);
		temporaryObjects.remove(asTemporaryObject);
		allTemporaryObjects.remove_fast(asTemporaryObject);
	}
	else
	{
		todo_important(TXT("implement_"));
	}
	/*
	unowned Projectile? asProjectile = _object as unowned Projectile;
	if (asProjectile != null)
	{
		projectiles.remove(asProjectile);
	}
	*/
	//aiObjects.remove(_object);
}

bool Room::move_through_doors(Transform const & _placement, REF_ Transform & _nextPlacement, REF_ Room *& _intoRoom, int _moveThroughFlags) const
{
	Transform intoRoomTransform;
	if (move_through_doors(_placement, _nextPlacement, _intoRoom, intoRoomTransform, nullptr, nullptr, nullptr, _moveThroughFlags))
	{
		_nextPlacement = intoRoomTransform.to_local(_nextPlacement);
		return true;
	}
	else
	{
		_intoRoom = cast_to_nonconst(this);
	}
	return false;
}

bool Room::move_through_doors(Transform const & _placement, Transform const & _nextPlacement, OUT_ Room *& _intoRoom, OUT_ Transform & _intoRoomTransform, DoorInRoom ** _exitThrough, DoorInRoom ** _enterThrough, OUT_ DoorInRoomArrayPtr * _throughDoors, int _moveThroughFlags) const
{
	Transform placement = _placement;
	Transform nextPlacement = _nextPlacement;
	Transform intoRoomTransform = Transform::identity;
	Room * inRoom = cast_to_nonconst(this);
	DoorInRoom* exitThrough = nullptr;
	DoorInRoom* enterThrough = nullptr;

	if (!inRoom)
	{
		return false;
	}

	bool wentThroughDoor = false;

#ifdef AN_ASSERT
	int wentThroughDoors = 0;
#endif
	int32 triesLeft = 10;
	while ((triesLeft--) > 0)
	{
		DoorInRoom* goingThroughDoor = nullptr;

		Vector3 const currLocation = placement.get_translation();
		Vector3 const nextLocation = nextPlacement.get_translation();
		Vector3 destLocation = nextPlacement.get_translation();

		// find closest door on the way
		for_every_ptr(door, inRoom->get_doors())
		{
			if (!door->is_visible())
			{
				continue;
			}
			auto* d = door->get_door();
			an_assert(d); // if there's no door linked to door in room, is_visible will return false
			if (!is_flag_set(_moveThroughFlags, MoveThroughDoorsFlag::ForceMoveThrough))
			{
				if (!door->can_move_through())
				{
					continue;
				}
				if (d->get_open_factor() == 0.0f && !d->can_see_through_when_closed())
				{
					// skip this, we can't go through
					continue;
				}
			}
			Vector3 newDestLocation;
			if (door->has_world_active_room_on_other_side() &&
				door->does_go_through_hole(currLocation, nextLocation, OUT_ newDestLocation))
			{
				goingThroughDoor = door;
				destLocation = newDestLocation;
			}
		}

		if (goingThroughDoor)
		{
			// this shouldn't happen as we do check above but... for some reason it did happen? doors are decoupled in sync activity
			auto* nextRoom = goingThroughDoor->get_world_active_room_on_other_side();
			if (!nextRoom)
			{
				break;
			}
#ifdef AN_ASSERT
			++ wentThroughDoors;
#endif
			if (_throughDoors)
			{
				_throughDoors->push_door(goingThroughDoor);
			}
			wentThroughDoor = true;
			if (!exitThrough)
			{
				exitThrough = goingThroughDoor;
			}
			enterThrough = goingThroughDoor->get_door_on_other_side();

			placement.set_translation(destLocation);
			Transform const & doorTransform = goingThroughDoor->get_other_room_transform();
			placement = doorTransform.to_local(placement);
			nextPlacement = doorTransform.to_local(nextPlacement);
			inRoom = nextRoom;
			intoRoomTransform = intoRoomTransform.to_world(doorTransform);
		}
		else
		{
			break;
		}
	}
	// copy results
	_intoRoom = inRoom;
	_intoRoomTransform = intoRoomTransform;
	an_assert(!exitThrough || !enterThrough || exitThrough->get_door_on_other_side() == enterThrough || wentThroughDoors > 1);
	an_assert(!exitThrough || !enterThrough || exitThrough->get_door_on_other_side() == enterThrough || (! _exitThrough && ! _enterThrough) || _throughDoors, TXT("seems that exit and enter doors do not match, please provide _throughDoors to avoid skipping through rooms"));
	if (_exitThrough)
	{
		*_exitThrough = exitThrough;
	}
	if (_enterThrough)
	{
		*_enterThrough = enterThrough;
	}
	return wentThroughDoor;
}

void Room::set_individual_random_generator(Random::Generator const & _individualRandomGenerator, bool _lock)
{
	an_assert(!individualRandomGeneratorLocked);
	individualRandomGenerator = _individualRandomGenerator;
	if (_lock)
	{
		individualRandomGeneratorLocked = _lock;
	}
	individualRandomGeneratorSet = true;
	if (roomType)
	{
		update_door_type_replacer();
	}
#ifdef AN_INSPECT_NEW_ROOM
	output(TXT("new rg for room %S (room:%p) type \"%S\""), get_individual_random_generator().get_seed_string().to_char(), this, roomType ? roomType->get_name().to_string().to_char() : TXT("<no type>"));
#endif
}

RoomGeneratorInfo const * Room::get_room_generator_info() const
{
	an_assert(individualRandomGeneratorSet);
	// will give same result always
	Random::Generator rg = get_individual_random_generator();
	rg.advance_seed(983840, 497234);
	return get_room_generator_info_internal(rg);
}

RoomGeneratorInfo const * Room::get_room_generator_info_internal(Random::Generator & _rg) const
{
	RoomGeneratorInfo const* result = roomGeneratorInfo;
	if (!result)
	{
		result = roomGeneratorInfoSet.get(_rg);
	}
	if (!result && roomType)
	{
		result = roomType->get_room_generator_info(_rg);
	}
	return result;
}

RoomType const * Room::get_door_vr_corridor_room_room_type(Random::Generator & _rg) const
{
	RoomType const * result = doorVRCorridor.get_room(_rg);
	if (!result && roomType)
	{
		result = roomType->get_door_vr_corridor_room_room_type(_rg);
	}
	if (!result && inRegion)
	{
		result = inRegion->get_door_vr_corridor_room_room_type(_rg);
	}
	return result;
}

RoomType const * Room::get_door_vr_corridor_elevator_room_type(Random::Generator & _rg) const
{
	RoomType const * result = doorVRCorridor.get_elevator(_rg);
	if (!result && roomType)
	{
		result = roomType->get_door_vr_corridor_elevator_room_type(_rg);
	}
	if (!result && inRegion)
	{
		result = inRegion->get_door_vr_corridor_elevator_room_type(_rg);
	}
	return result;
}

Range Room::get_door_vr_corridor_priority_range() const
{
	Range temp;
	if (doorVRCorridor.get_priority_range(OUT_ temp)) return temp;
	if (roomType)
	{
		if (roomType->get_door_vr_corridor_priority_range(OUT_ temp))
		{
			return temp;
		}
	}
	if (inRegion)
	{
		return inRegion->get_door_vr_corridor_priority_range();
	}
	return Range::empty;
}

float Room::get_door_vr_corridor_priority(Random::Generator& _rg, float _default) const
{
	float temp;
	if (doorVRCorridor.get_priority(_rg, temp)) return temp;
	if (roomType)
	{
		float result = 0.0f;
		if (roomType->get_door_vr_corridor_priority(_rg, OUT_ result))
		{
			return result;
		}
	}
	if (inRegion)
	{
		return inRegion->get_door_vr_corridor_priority(_rg, _default);
	}
	return _default;
}

void Room::generate_environments()
{
	if (!ownEnvironment &&
		environmentUsage.environment)
	{
		todo_note(TXT("create own copy of environment only if parameter says so, for now keep same"));
		//create_own_copy_of_environment();
		if (!environmentUsage.environment->is_generated())
		{
			environmentUsage.environment->generate();
		}
	}
	if (ownEnvironment)
	{
		todo_future(TXT("queue for background generation? what if room is no longer available?"));
		ownEnvironment->generate();
	}
}

void Room::update_door_type_replacer()
{
	an_assert(individualRandomGeneratorSet);
	an_assert(roomType, TXT("having a room type would be nice, we need to get that room generator info from somewhere!"));
	if (RoomGeneratorInfo const * useRoomGeneratorInfo = get_room_generator_info())
	{
		if (auto * dtr = useRoomGeneratorInfo->get_door_type_replacer(this))
		{
			internal_update_door_type_replacer_with(dtr);
		}
	}
}

void Room::internal_update_door_type_replacer_with(DoorTypeReplacer* dtr)
{
	DoorTypeReplacer* useDTR = dtr;
	if (auto* rt = get_room_type())
	{
		if (auto* rtDTR = rt->get_door_type_replacer())
		{
			if (!useDTR || (rtDTR->get_priority() > useDTR->get_priority()))
			{
				useDTR = rtDTR;
			}
		}
	}
	set_door_type_replacer(useDTR);
}

bool Room::generate_room_using_room_generator(REF_ RoomGeneratingContext& _roomGeneratingContext)
{
	ROOM_HISTORY(this, TXT("generate_room_using_room_generator"));

	scoped_call_stack_info(TXT("generate room using room generator"));

	bool result = true;

	get_use_environment_from_type(); // store use environment

	result &= run_wheres_my_point_processor_setup_parameters();

	result &= run_wheres_my_point_processor_pre_generate(); // to allow affecting environment

#ifdef LOG_WORLD_GENERATION
	::System::TimeStamp startedGenerationTimeStamp;
	startedGenerationTimeStamp.reset();
	output(TXT("generating using room type %S"), roomType->get_name().to_string().to_char());
#endif
	scoped_call_stack_info_str_printf(TXT("room type: %S"), roomType->get_name().to_string().to_char());
	RoomGeneratorInfo const * useRoomGeneratorInfo = get_room_generator_info();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	generatedWithRoomGenerator = useRoomGeneratorInfo;
#endif

	if (!useRoomGeneratorInfo)
	{
		error(TXT("no room generator info for this room?"));
		return false;
	}
	{
		scoped_call_stack_info(TXT("->apply_generation_parameters_to"));
		result &= useRoomGeneratorInfo->apply_generation_parameters_to(this);
	}
	{
		scoped_call_stack_info(TXT("->run_wheres_my_point_processor_on_generate"));
		result &= run_wheres_my_point_processor_on_generate();
	}
	{
		scoped_call_stack_info(TXT("generate!"));
		result &= useRoomGeneratorInfo->generate(this, false, REF_ _roomGeneratingContext);
	}
	if (auto * dtr = useRoomGeneratorInfo->get_door_type_replacer(this))
	{
		internal_update_door_type_replacer_with(dtr); // use door type replacer only if RGI provides it
	}

	setup_using_use_environment(); // create environment

	generate_environments();

	result &= useRoomGeneratorInfo->post_generate(this, REF_ _roomGeneratingContext);

	return result;
}


bool Room::generate()
{
	ROOM_HISTORY(this, TXT("generate"));

	scoped_call_stack_info(TXT("generate room"));

	an_assert(individualRandomGeneratorSet, TXT("random generator was not set, but generation was requested"));

	if (is_generated())
	{
		return true;
	}

	if (Game::get()->does_want_to_cancel_creation())
	{
		// skip this
		return true;
	}

	RoomGeneratorInfo const * useRoomGeneratorInfo = get_room_generator_info();

	if (!useRoomGeneratorInfo ||
		(roomType && roomType->should_use_fallback_generation()))
	{
		return generate_fallback_room();
	}

	if (!start_generation())
	{
		// wait, as it has already started
		return true;
	}

#ifdef LOG_WORLD_GENERATION
	::System::TimeStamp startedGenerationTimeStamp;
	startedGenerationTimeStamp.reset();
	output(TXT("generating room r%p \"%S\" [%S]"), this, get_name().to_char(), get_individual_random_generator().get_seed_string().to_char());
#endif

	Game::get()->push_activation_group(get_activation_group());

	RoomGeneratingContext roomGeneratingContext;
	bool result = generate_room_using_room_generator(REF_ roomGeneratingContext);

#ifdef LOG_WORLD_GENERATION
	float generationTime = startedGenerationTimeStamp.get_time_since();
	output(TXT("generated in %.3fs"), generationTime);
#endif

	Game::get()->pop_activation_group(get_activation_group());

	if (!result)
	{
		// try fallback generation then
		an_assert(false, TXT("could not generate for room type \"%S\", using fallback"), roomType ? roomType->get_name().to_string().to_char() : TXT("<no room type>"));
		generationProcessStarted = 0;
#ifdef LOG_WORLD_GENERATION
		output(TXT("room generation failed (room type \"%S\"), continue with room fallback"), roomType ? roomType->get_name().to_string().to_char() : TXT("<no room type>"));
#endif
#ifdef AN_HANDLE_GENERATION_ISSUES
		Game::get()->add_generation_issue(NAME(room_failedGeneration));
#endif
		return generate_fallback_room();
	}

	place_pending_doors_on_pois();

	end_generation(); // aRGI should call mark_vr_arranged and/or mark_mesh_generated

	return result;
}

void Room::reset_generation()
{
#ifdef AN_DEVELOPMENT
	{
		Concurrency::ScopedSpinLock lock(generationProcessStartedLock);
		an_assert(generationProcessStarted == 0);
	}
#endif
	isVRArranged = false;
	isMeshGenerated = false;
}

bool Room::start_generation()
{
	if (is_generated())
	{
		return false;
	}

	{
		Concurrency::ScopedSpinLock lock(generationProcessStartedLock);
		if (generationProcessStarted != 0)
		{
			// we already generate, wait
			return false;
		}
		generationProcessStarted.increase();
	}

	return true;
}

void Room::end_generation(bool _markGenerated)
{
	place_pending_doors_on_pois();

	// finally, combine meshes
	combine_appearance();

	// collision bounding boxes
	update_bounding_box();

	mark_nav_mesh_creation_required();
	
	if (_markGenerated)
	{
		mark_vr_arranged();
		mark_mesh_generated();
	}
	else
	{
		an_assert(is_vr_arranged(), TXT("when ending generation we at least expect vr generation to be complete"));
	}

	an_assert(pendingDoorsOnPOIs.is_empty(), TXT("we shouldn't have any doors pending placement at this point!"));

	if (auto* g = Game::get())
	{
		g->on_generated_room(this);
	}
}

void Room::mark_nav_mesh_creation_required()
{
	if (deletionPending || ! is_world_active())
	{
		return;
	}
	if (shouldHaveNavMesh)
	{
		Game::get()->get_nav_system()->mark_new_nav_mesh_for_room_required(this);
	}
}

void Room::queue_build_nav_mesh_task()
{
	if (deletionPending)
	{
		return;
	}
	todo_note(TXT("this is hack but as I have no idea what kind of nav meshes I want to have, I will create one universal nav mesh here"));
	Game::get()->get_nav_system()->cancel_related_to(this); // cancel all related to this
	Game::get()->get_nav_system()->add(Nav::Tasks::BuildNavMesh::new_task(this));
}

void Room::mark_vr_arranged()
{
	isVRArranged = true;
}

void Room::mark_mesh_generated()
{
	isMeshGenerated = true;
}

bool Room::generate_fallback_room()
{
	if (Game::get()->does_want_to_cancel_creation())
	{
		// skip this
		return true;
	}

	scoped_call_stack_info(TXT("generate fallback room"));

#ifdef LOG_WORLD_GENERATION
	output(TXT("generate room fallback %S..."), roomType ? roomType->get_name().to_string().to_char() : TXT("??"));
#endif

	warn(TXT("generate room fallback %S..."), roomType ? roomType->get_name().to_string().to_char() : TXT("??"));

#ifndef BUILD_NON_RELEASE
	AN_FAULT;
#endif

	if (!start_generation())
	{
		output(TXT("generate_fallback_room BREAK"));
		AN_BREAK;
		// wait, as it has already started
		return true;
	}

	if (! Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testRoom), Framework::LibraryName()).is_valid())
	{
		error(TXT("we should not be using fallback rooms, BREAK"));
		AN_BREAK;
	}

	get_use_environment_from_type(); // store use environment

	run_wheres_my_point_processor_setup_parameters();

	run_wheres_my_point_processor_pre_generate(); // to allow affecting environment

	setup_using_use_environment(); // create environment

	generate_environments();

#ifdef LOG_WORLD_GENERATION
	output(TXT("generating room fallback %S..."), roomType ? roomType->get_name().to_string().to_char() : TXT("??"));
#endif

	bool result = false;

	// this is fallback solution
	// because doors are in front of walls, walls may not catch collision properly

	// create mesh for this room
	float doorSpacing = 2.0f;
	float length = 0.0f; // because we have doorSpacing from last door divided into two halves
	float height = 0.0f;
	float veryLowBelow = 0.0f;
	float collisionSize = 1.0f;
	float halfCollisionSize = collisionSize * 0.5f;
	float doorFromWallDistance = 0.01f;
	for_every_ptr(door, allDoors)
	{
		Range2 doorSize = door->get_hole()->calculate_size(door->get_side(), door->get_hole_scale());
		length += doorSize.x.length() + doorSpacing;
		height = max(height, doorSize.y.max);
		veryLowBelow = min(veryLowBelow, doorSize.y.min);
	}
	height += 1.0f;
	veryLowBelow -= 1.0f;
	float doorWidth = 0.5f;

	float width = 5.0f;
	Library* library = roomType && roomType->get_library() ? roomType->get_library() : Framework::Library::get_current();
	{
		Framework::MeshStatic * tm = library->get_meshes_static().create_temporary();
		Framework::CollisionModel* tc = library->get_collision_models().create_temporary();

		Meshes::Mesh3DBuilder* m3db = new Meshes::Mesh3DBuilder();
		m3db->setup(Meshes::Primitive::Triangle, ::System::vertex_format().with_colour_rgb().with_normal().with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float), 3);

		Colour wallColour = Colour::c64Red;
		switch (Random::get_int(10))
		{
		case 0: wallColour = Colour::c64Red; break;
		case 1: wallColour = Colour::c64Blue; break;
		case 2: wallColour = Colour::c64Cyan; break;
		case 3: wallColour = Colour::c64Green; break;
		case 4: wallColour = Colour::c64Violet; break;
		case 5: wallColour = Colour::c64Orange; break;
		case 6: wallColour = Colour::c64Yellow; break;
		case 7: wallColour = Colour::c64Brown; break;
		case 8: wallColour = Colour::c64Grey2; break;
		case 9: wallColour = Colour::c64Black; break;
		}
		// floor
		m3db->normal(Vector3(0.0f, 0.0f, 1.0f));
		m3db->colour(Colour::c64Orange);
		m3db->uv(Vector2(0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, 0.0f));
		m3db->done_triangle();
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, 0.0f));
		m3db->done_triangle();

		// ceiling
		m3db->normal(Vector3(0.0f, 0.0f, -1.0f));
		m3db->colour(Colour::blackWarm);
		m3db->uv(Vector2(0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, height));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, height));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, height));
		m3db->done_triangle();
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, height));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, height));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, height));
		m3db->done_triangle();

		// wall for doors for outside doors (if we would see something below)
		m3db->normal(Vector3(0.0f, 1.0f, 0.0f));
		m3db->colour(Colour::c64Grey1);
		m3db->uv(Vector2(0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -2.0f * doorFromWallDistance, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -2.0f * doorFromWallDistance, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -2.0f * doorFromWallDistance, veryLowBelow));
		m3db->done_triangle();
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -2.0f * doorFromWallDistance, veryLowBelow));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -2.0f * doorFromWallDistance, veryLowBelow));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -2.0f * doorFromWallDistance, 0.0f));
		m3db->done_triangle();

		// wall for doors
		m3db->normal(Vector3(0.0f, -1.0f, 0.0f));
		m3db->colour(wallColour);
		m3db->uv(Vector2(0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, height));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, height));
		m3db->done_triangle();
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, height));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, 0.0f));
		m3db->done_triangle();

		// other walls
		m3db->normal(Vector3(0.0f, 1.0f, 0.0f));
		m3db->colour(wallColour);
		m3db->uv(Vector2(0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, height));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, height));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, 0.0f));
		m3db->done_triangle();
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, height));
		m3db->done_triangle();

		m3db->normal(Vector3(1.0f, 0.0f, 0.0f));
		m3db->colour(wallColour);
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, 0.0f));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, height));
		m3db->done_triangle();
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f, -width, height));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, height));
		m3db->add_point();	m3db->location(Vector3(-length * 0.5f,   0.0f, 0.0f));
		m3db->done_triangle();

		m3db->normal(Vector3(-1.0f, 0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, height));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, 0.0f));
		m3db->done_triangle();
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, 0.0f));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f,   0.0f, height));
		m3db->add_point();	m3db->location(Vector3( length * 0.5f, -width, height));
		m3db->done_triangle();

		float doorLeftPoint = -length * 0.5f + doorSpacing * 0.5f;
		for_every_ptr(door, allDoors)
		{
			Range2 doorSize = door->get_hole()->calculate_size(door->get_side(), door->get_hole_scale());

			float a = doorLeftPoint - doorSpacing * 0.5f;
			float b = doorLeftPoint;
			float c = b + doorSize.x.length();
			float d = c + doorSpacing * 0.5f;
			float dh = doorSize.y.max;

			// around door
			m3db->normal(Vector3(0.0f, -1.0f, 0.0f));
			m3db->colour(wallColour);
			m3db->uv(Vector2(0.0f, 0.0f));
			m3db->add_point();	m3db->location(Vector3(a, -doorWidth, 0.0f));
			m3db->add_point();	m3db->location(Vector3(a, -doorWidth, height));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, height));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, height));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, 0.0f));
			m3db->add_point();	m3db->location(Vector3(a, -doorWidth, 0.0f));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, 0.0f));
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, height));
			m3db->add_point();	m3db->location(Vector3(d, -doorWidth, height));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(d, -doorWidth, height));
			m3db->add_point();	m3db->location(Vector3(d, -doorWidth, 0.0f));
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, 0.0f));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, dh));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, height));
			m3db->add_point();	m3db->location(Vector3(d, -doorWidth, height));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, height));
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, dh));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, dh));
			m3db->done_triangle();

			// around inside door
			m3db->colour(Colour::c64Grey2);
			m3db->normal(Vector3(1.0f, 0.0f, 0.0f));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, 0.0f));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, dh));
			m3db->add_point();	m3db->location(Vector3(b,       0.0f, dh));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(b,       0.0f, dh));
			m3db->add_point();	m3db->location(Vector3(b,       0.0f, 0.0f));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, 0.0f));
			m3db->done_triangle();
			m3db->normal(Vector3(-1.0f, 0.0f, 0.0f));
			m3db->add_point();	m3db->location(Vector3(c, 0.0f, dh));
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, dh));
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, 0.0f));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, 0.0f));
			m3db->add_point();	m3db->location(Vector3(c,       0.0f, 0.0f));
			m3db->add_point();	m3db->location(Vector3(c,       0.0f, dh));
			m3db->done_triangle();
			m3db->normal(Vector3(0.0f, 0.0f, -1.0f));
			m3db->add_point();	m3db->location(Vector3(b, 0.0f, dh));
			m3db->add_point();	m3db->location(Vector3(b, -doorWidth, dh));
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, dh));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(c, -doorWidth, dh));
			m3db->add_point();	m3db->location(Vector3(c,       0.0f, dh));
			m3db->add_point();	m3db->location(Vector3(b,       0.0f, dh));
			m3db->done_triangle();

			tc->get_model()->add(Collision::Box().set(Box(Vector3((a + b) * 0.5f, -doorWidth * 0.5f, height*0.5f), Vector3((b-a)* 0.5f, doorWidth * 0.5f, height*0.5f + halfCollisionSize), Vector3::xAxis, Vector3::yAxis)));
			tc->get_model()->add(Collision::Box().set(Box(Vector3((b + c) * 0.5f, -doorWidth * 0.5f, dh + (height-dh)*0.5f), Vector3((c - b)* 0.5f, doorWidth * 0.5f, (height-dh)*0.5f), Vector3::xAxis, Vector3::yAxis)));
			tc->get_model()->add(Collision::Box().set(Box(Vector3((c + d) * 0.5f, -doorWidth * 0.5f, height*0.5f), Vector3((d - c)* 0.5f, doorWidth * 0.5f, height*0.5f + halfCollisionSize), Vector3::xAxis, Vector3::yAxis)));

			doorLeftPoint += doorSize.x.length() + doorSpacing;
		}

		//
		m3db->build();

		tm->make_sure_mesh_exists();

		if (Meshes::Mesh3D * m3ds = fast_cast<Meshes::Mesh3D>(tm->get_mesh()))
		{
			m3ds->load_builder(m3db);
		}

		delete_and_clear(m3db);

		tc->get_model()->add(Collision::Box().set(Box(Vector3(0.0f, -width * 0.5f, veryLowBelow * 0.5f), Vector3(length * 0.5f + halfCollisionSize, width * 0.5f + halfCollisionSize, -veryLowBelow * 0.5f), Vector3::xAxis, Vector3::yAxis)));
		tc->get_model()->add(Collision::Box().set(Box(Vector3(0.0f, -width * 0.5f, height+halfCollisionSize), Vector3(length * 0.5f + halfCollisionSize, width * 0.5f + halfCollisionSize, halfCollisionSize), Vector3::xAxis, Vector3::yAxis)));
		tc->get_model()->add(Collision::Box().set(Box(Vector3(-length * 0.5f - halfCollisionSize, -width * 0.5f, height*0.5f), Vector3(halfCollisionSize, width * 0.5f + halfCollisionSize, height*0.5f + halfCollisionSize), Vector3::xAxis, Vector3::yAxis)));
		tc->get_model()->add(Collision::Box().set(Box(Vector3( length * 0.5f + halfCollisionSize, -width * 0.5f, height*0.5f), Vector3(halfCollisionSize, width * 0.5f + halfCollisionSize, height*0.5f + halfCollisionSize), Vector3::xAxis, Vector3::yAxis)));
		tc->get_model()->add(Collision::Box().set(Box(Vector3(0.0f, -width - halfCollisionSize, height*0.5f), Vector3(length * 0.5f + halfCollisionSize, halfCollisionSize, height*0.5f + halfCollisionSize), Vector3::xAxis, Vector3::yAxis)));
		tc->get_model()->add(Collision::Box().set(Box(Vector3(0.0f,          halfCollisionSize, height*0.5f), Vector3(length * 0.5f + halfCollisionSize, halfCollisionSize, height*0.5f + halfCollisionSize), Vector3::xAxis, Vector3::yAxis)));

		tm->set_movement_collision_model(tc);

		tm->set_missing_materials(Framework::Library::get_current());
		Framework::Library::get_current()->prepare_for_game_during_async(tm);

		// add this mesh to this room
		add_mesh(tm);
	}

	float doorLeftPoint = -length * 0.5f + doorSpacing * 0.5f;
	for_every_ptr(door, allDoors)
	{
		Range2 doorSize = door->get_hole()->calculate_size(door->get_side(), door->get_hole_scale());

		Vector3 doorPlacement = Vector3::zero;
		doorPlacement.y = -doorFromWallDistance;
		doorPlacement.x = doorLeftPoint - doorSize.x.min;
		door->set_placement(look_at_matrix(doorPlacement, doorPlacement + Vector3::yAxis, Vector3::zAxis).to_transform());

		doorLeftPoint += doorSize.x.length() + doorSpacing;
	}

	end_generation(true);

	return result;
}

void Room::add_sky_mesh(Material* _usingMaterial, Optional<Vector3> const& _centre, Optional<float> const& _radiusOpt)
{
	Vector3 centre = _centre.get(Vector3::zero);
	float _radius = _radiusOpt.get(13000.0f);
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif

	Library* library = roomType && roomType->get_library() ? roomType->get_library() : Framework::Library::get_current();

	Framework::MeshStatic * tm = library->get_meshes_static().create_temporary();

	Meshes::Mesh3DBuilder* m3db = new Meshes::Mesh3DBuilder();
	m3db->setup(Meshes::Primitive::Triangle, ::System::vertex_format().with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float), 12);

	m3db->uv(Vector2(0.0f, 0.0f));

	int xAmount = 8;
	int yAmount = 4;
	for_count(int, x, xAmount)
	{
		for_count(int, y, yAmount)
		{
			float ax0 = ((float)x / (float)xAmount) * 360.0f;
			float ax1 = ((float)(x + 1) / (float)xAmount) * 360.0f;
			float ay0 = -90.0f + ((float)y / (float)yAmount) * 180.f;
			float ay1 = -90.0f + ((float)(y + 1) / (float)yAmount) * 180.f;

			Vector3 p00 = Rotator3(ay0, ax0, 0.0f).get_forward() * _radius + centre;
			Vector3 p10 = Rotator3(ay0, ax1, 0.0f).get_forward() * _radius + centre;
			Vector3 p01 = Rotator3(ay1, ax0, 0.0f).get_forward() * _radius + centre;
			Vector3 p11 = Rotator3(ay1, ax1, 0.0f).get_forward() * _radius + centre;

			m3db->add_point();	m3db->location(p00);
			m3db->add_point();	m3db->location(p01);
			m3db->add_point();	m3db->location(p10);
			m3db->done_triangle();
			m3db->add_point();	m3db->location(p01);
			m3db->add_point();	m3db->location(p11);
			m3db->add_point();	m3db->location(p10);
			m3db->done_triangle();
		}
	}

	//
	m3db->build();

	tm->make_sure_mesh_exists();

	if (Meshes::Mesh3D * m3ds = fast_cast<Meshes::Mesh3D>(tm->get_mesh()))
	{
		m3ds->load_builder(m3db);
	}

	delete_and_clear(m3db);

	if (_usingMaterial)
	{
		tm->set_material(0, _usingMaterial);
	}

	tm->set_missing_materials(Framework::Library::get_current());
	Framework::Library::get_current()->prepare_for_game_during_async(tm);

	// add this mesh to this room
	Meshes::Mesh3DInstance* meshInstance;
	add_mesh(tm, Transform::identity, &meshInstance);
	meshInstance->set_rendering_order_priority(10000); // render last
	meshInstance->set_preferred_rendering_buffer(NAME(skyBox));
}

void Room::add_clouds(Material* _usingMaterial, Optional<Vector3> const& _centre, Optional<float> const& _radiusOpt, float _lowAngle, float _highAngle)
{
	return; // no clouds for time being

	Vector3 centre = _centre.get(Vector3::zero);
	float _radius = _radiusOpt.get(13000.0f);
#ifdef AN_DEVELOPMENT
	if (is_world_active())
	{
		ASSERT_SYNC;
	}
#endif

	Library* library = roomType && roomType->get_library() ? roomType->get_library() : Framework::Library::get_current();

	Framework::MeshStatic * tm = library->get_meshes_static().create_temporary();

	Meshes::Mesh3DBuilder* m3db = new Meshes::Mesh3DBuilder();
	m3db->setup(Meshes::Primitive::Triangle, ::System::vertex_format().with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float), 12);

	Random::Generator rg = get_individual_random_generator();
	rg.advance_seed(127943, 97234);

	int cloudCount = rg.get_int_from_range(4, 10);
	for (int i = 0; i < cloudCount; ++i)
	{
		Array<Rotator3> points;

		Rotator3 at;
		at.yaw = rg.get_float(-180.0f, 180.0f);
		at.pitch = rg.get_float(_lowAngle, _highAngle);
		at.roll = 0.0f;
		points.push_back(at);

		float dir = rg.get_chance(0.5f) ? 1.0f : -1.0f;

		int count = rg.get_int_from_range(2, 12);
		for (int n = 0; n < count; ++n)
		{
			at.yaw += dir * rg.get_float(10.0f, 35.0f);
			at.pitch = at.pitch + rg.get_float(-15.0f, 15.0f);
			int triesLeft = 20;
			while ((at.pitch < _lowAngle ||
				    at.pitch > _highAngle) &&
				   triesLeft)
			{
				at.pitch = at.pitch + rg.get_float(-15.0f, 15.0f);
				--triesLeft;
			}
			at.pitch = clamp(at.pitch, _lowAngle, _highAngle);

			points.push_back(at);

			if (n < count - 1)
			{
				at.yaw += rg.get_float(-5.0f, 5.0f);
				at.pitch += rg.get_float(-2.0f, 2.0f);

				points.push_back(at);
			}
		}

		m3db->uv(Vector2(0.0f, 0.0f));
		
		for (int i = 0; i < points.get_size() - 2; ++ i)
		{
			m3db->add_point();	m3db->location(points[i + 0].get_forward() * _radius * 0.95f + centre);
			m3db->add_point();	m3db->location(points[i + 1].get_forward() * _radius * 0.95f + centre);
			m3db->add_point();	m3db->location(points[i + 2].get_forward() * _radius * 0.95f + centre);
			m3db->done_triangle();
		}

	}

	//
	m3db->build();

	tm->make_sure_mesh_exists();

	if (Meshes::Mesh3D * m3ds = fast_cast<Meshes::Mesh3D>(tm->get_mesh()))
	{
		m3ds->load_builder(m3db);
	}

	delete_and_clear(m3db);

	if (_usingMaterial)
	{
		tm->set_material(0, _usingMaterial);
	}

	tm->set_missing_materials(Framework::Library::get_current());
	Framework::Library::get_current()->prepare_for_game_during_async(tm);

	// add this mesh to this room
	Meshes::Mesh3DInstance* meshInstance;
	add_mesh(tm, Transform::identity, &meshInstance);
	meshInstance->set_rendering_order_priority(-10000); // render first
}

bool Room::generate_plane_room(Material * _planeMaterial, PhysicalMaterial * _planePhysMaterial, float _u, bool _withCeiling, float _zOffset)
{
	scoped_call_stack_info(TXT("generate plane room"));

	if (!start_generation())
	{
		// wait, as it has already started
		return true;
	}

	bool result = generate_plane(_planeMaterial, _planePhysMaterial, _u, _withCeiling, _zOffset);

	end_generation(true);

	return result;
}

bool Room::generate_plane(Material * _planeMaterial, PhysicalMaterial * _planePhysMaterial, float _u, bool _withCeiling, float _zOffset)
{
#ifdef LOG_WORLD_GENERATION
	output(TXT("generating room fallback %S..."), roomType ? roomType->get_name().to_string().to_char() : TXT("??"));
#endif

	bool result = false;

	// just generate plane
	Library* library = roomType && roomType->get_library() ? roomType->get_library() : Framework::Library::get_current();
	{
		Framework::MeshStatic * tm = library->get_meshes_static().create_temporary();
		Framework::CollisionModel* tc = library->get_collision_models().create_temporary();

		Meshes::Mesh3DBuilder* m3db = new Meshes::Mesh3DBuilder();
		m3db->setup(Meshes::Primitive::Triangle, ::System::vertex_format().with_normal().with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float), 3);

		float length = 10000.0f;
		float halfLength = length * 0.5f;
		{
			// distant, lower
			m3db->normal(Vector3(0.0f, 0.0f, 1.0f));
			m3db->uv(Vector2(_u, 0.0f));
			m3db->add_point();	m3db->location(Vector3(-halfLength, -halfLength, -0.1f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(-halfLength, halfLength, -0.1f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(halfLength, -halfLength, -0.1f + _zOffset));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(halfLength, -halfLength, -0.1f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(-halfLength, halfLength, -0.1f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(halfLength, halfLength, -0.1f + _zOffset));
			m3db->done_triangle();
		}
		{
			// actual floor
			m3db->normal(Vector3(0.0f, 0.0f, 1.0f));
			m3db->uv(Vector2(_u, 0.0f));
			m3db->add_point();	m3db->location(Vector3(-halfLength, -halfLength, 0.0f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(-halfLength, halfLength, 0.0f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(halfLength, -halfLength, 0.0f + _zOffset));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(halfLength, -halfLength, 0.0f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(-halfLength, halfLength, 0.0f + _zOffset));
			m3db->add_point();	m3db->location(Vector3(halfLength, halfLength, 0.0f + _zOffset));
			m3db->done_triangle();
		}

		float const ceilingAt = 5.0f;
		
		if (_withCeiling)
		{
			m3db->normal(Vector3(0.0f, 0.0f, -1.0f));
			m3db->uv(Vector2(_u, 0.0f));
			m3db->add_point();	m3db->location(Vector3(-halfLength, -halfLength, ceilingAt));
			m3db->add_point();	m3db->location(Vector3(halfLength, -halfLength, ceilingAt));
			m3db->add_point();	m3db->location(Vector3(-halfLength, halfLength, ceilingAt));
			m3db->done_triangle();
			m3db->add_point();	m3db->location(Vector3(halfLength, -halfLength, ceilingAt));
			m3db->add_point();	m3db->location(Vector3(halfLength, halfLength, ceilingAt));
			m3db->add_point();	m3db->location(Vector3(-halfLength, halfLength, ceilingAt));
			m3db->done_triangle();
		}

		//
		m3db->build();

		tm->make_sure_mesh_exists();

		if (Meshes::Mesh3D * m3ds = fast_cast<Meshes::Mesh3D>(tm->get_mesh()))
		{
			m3ds->load_builder(m3db);
		}

		delete_and_clear(m3db);

		float depth = 1.0f;
		{
			Collision::Box box;
			box.set(Box(Vector3(0.0f, 0.0f, -depth * 0.5f), Vector3(length, length, depth * 0.5f), Vector3::xAxis, Vector3::yAxis));
			if (_planePhysMaterial)
			{
				box.use_material(_planePhysMaterial);
			}
			tc->get_model()->add(box);
		}
		if (_withCeiling)
		{
			Collision::Box box;
			box.set(Box(Vector3(0.0f, 0.0f, ceilingAt + depth * 0.5f), Vector3(length, length, depth * 0.5f), Vector3::xAxis, Vector3::yAxis));
			if (_planePhysMaterial)
			{
				box.use_material(_planePhysMaterial);
			}
			tc->get_model()->add(box);
		}

		{	// navmesh through mesh nodes
			todo_hack(TXT("hardcoded navFlags"));
			auto& meshNodes = tm->access_mesh_nodes();
			{
				MeshNode* n = new MeshNode();
				n->tags.set_tag(NAME(nav));
				n->tags.set_tag(NAME(convexPolygon));
				n->variables.access<String>(NAME(navFlags)) = String(TXT("poseNormal poseCrouch"));
				n->placement = Transform(Vector3(-halfLength, -halfLength, 0.0f), Quat::identity);
				meshNodes.push_back(MeshNodePtr(n));
			}
			{
				MeshNode* n = new MeshNode();
				n->tags.set_tag(NAME(nav));
				n->tags.set_tag(NAME(convexPolygon));
				n->variables.access<String>(NAME(navFlags)) = String(TXT("poseNormal poseCrouch"));
				n->placement = Transform(Vector3(-halfLength, halfLength, 0.0f), Quat::identity);
				meshNodes.push_back(MeshNodePtr(n));
			}
			{
				MeshNode* n = new MeshNode();
				n->tags.set_tag(NAME(nav));
				n->tags.set_tag(NAME(convexPolygon));
				n->variables.access<String>(NAME(navFlags)) = String(TXT("poseNormal poseCrouch"));
				n->placement = Transform(Vector3(halfLength, halfLength, 0.0f), Quat::identity);
				meshNodes.push_back(MeshNodePtr(n));
			}
			{
				MeshNode* n = new MeshNode();
				n->tags.set_tag(NAME(nav));
				n->tags.set_tag(NAME(convexPolygon));
				n->variables.access<String>(NAME(navFlags)) = String(TXT("poseNormal poseCrouch"));
				n->placement = Transform(Vector3(halfLength, -halfLength, 0.0f), Quat::identity);
				meshNodes.push_back(MeshNodePtr(n));
			}
		}

		tm->set_movement_collision_model(tc);

		if (_planeMaterial)
		{
			tm->set_material(0, _planeMaterial);
		}
		tm->set_missing_materials(Framework::Library::get_current());
		Framework::Library::get_current()->prepare_for_game_during_async(tm);

		// add this mesh to this room
		add_mesh(tm);
	}

	return result;
}

bool Room::generate_hills(Material* _planeMaterial, PhysicalMaterial* _planePhysMaterial, float _u, float _height, float _size, float _step, Vector3 const& _at, int _tryCount)
{
#ifdef LOG_WORLD_GENERATION
	output(TXT("generating room hills %S..."), roomType ? roomType->get_name().to_string().to_char() : TXT("??"));
#endif

	bool result = false;

	// just generate plane
	Library* library = roomType && roomType->get_library() ? roomType->get_library() : Framework::Library::get_current();
	{
		Framework::MeshStatic* tm = library->get_meshes_static().create_temporary();
		Framework::CollisionModel* tc = library->get_collision_models().create_temporary();

		Meshes::Mesh3DBuilder* m3db = new Meshes::Mesh3DBuilder();
		m3db->setup(Meshes::Primitive::Triangle, ::System::vertex_format().with_normal().with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float), 3);

		int stepCount = max(2, TypeConversions::Normal::f_i_closest(_size / _step) + 1);
		Array<float> alt;
		alt.set_size(stepCount * stepCount);
		Random::Generator rg;
		for_every(a, alt) { *a = rg.get_float(0.0f, _height); }
		for_count(int, tryIdx, _tryCount)
		{
			for (int x = 0; x < stepCount; ++x)
			{
				for (int y = 0; y < stepCount; ++y)
				{
					if (x > 0 && x < stepCount - 1 &&
						y > 0 && y < stepCount - 1)
					{
						float xp = alt[(y + 0) * stepCount + (x + 1)];
						float xm = alt[(y - 0) * stepCount + (x - 1)];
						float yp = alt[(y + 1) * stepCount + (x + 0)];
						float ym = alt[(y - 1) * stepCount + (x - 0)];

						float& a = alt[y * stepCount + x];
						float keepWeight = 0.99f;
						a = a * keepWeight + (1.0f - keepWeight) * ((xp + xm + yp + ym) / 4.0f);
					}
					else
					{
						alt[y * stepCount + x] = 0;
					}
				}
			}
		}
		for (int x = 0; x < stepCount - 1; ++x)
		{
			for (int y = 0; y < stepCount - 1; ++y)
			{
				float lc = alt[(y + 0) * stepCount + (x + 0)] + _at.z;
				float rc = alt[(y + 0) * stepCount + (x + 1)] + _at.z;
				float lf = alt[(y + 1) * stepCount + (x + 0)] + _at.z;
				float rf = alt[(y + 1) * stepCount + (x + 1)] + _at.z;

				float l = -_size * 0.5f + (float)(x + 0) * _step + _at.x;
				float r = -_size * 0.5f + (float)(x + 1) * _step + _at.x;
				float c = -_size * 0.5f + (float)(y + 0) * _step + _at.y;
				float f = -_size * 0.5f + (float)(y + 1) * _step + _at.y;

				Vector3 vlc = Vector3(l, c, lc);
				Vector3 vrc = Vector3(r, c, rc);
				Vector3 vlf = Vector3(l, f, lf);
				Vector3 vrf = Vector3(r, f, rf);

				// up = Vector3::cross(right, fwd);

				m3db->uv(Vector2(_u, 0.0f));
				m3db->normal(Vector3::cross(vrf - vlc , vlf - vlc).normal());
				m3db->add_point();	m3db->location(vlc);
				m3db->add_point();	m3db->location(vlf);
				m3db->add_point();	m3db->location(vrf);
				m3db->done_triangle();
				m3db->normal(Vector3::cross(vrc - vlc, vrf - vlc).normal());
				m3db->add_point();	m3db->location(vlc);
				m3db->add_point();	m3db->location(vrf);
				m3db->add_point();	m3db->location(vrc);
				m3db->done_triangle();
			}
		}

		//
		m3db->build();

		tm->make_sure_mesh_exists();

		if (Meshes::Mesh3D* m3ds = fast_cast<Meshes::Mesh3D>(tm->get_mesh()))
		{
			m3ds->load_builder(m3db);

			if (auto* p = m3ds->get_part(0))
			{
				Collision::Mesh mesh;
				p->for_every_triangle([&mesh](Vector3 const& _a, Vector3 const& _b, Vector3 const& _c)
					{
						mesh.add_just_triangle(_a, _b, _c);
					});
				mesh.build();
				if (_planePhysMaterial)
				{
					mesh.use_material(_planePhysMaterial);
				}
				tc->get_model()->add(mesh);
			}

		}

		delete_and_clear(m3db);

		tm->set_movement_collision_model(tc);

		if (_planeMaterial)
		{
			tm->set_material(0, _planeMaterial);
		}
		tm->set_missing_materials(Framework::Library::get_current());
		Framework::Library::get_current()->prepare_for_game_during_async(tm);

		// add this mesh to this room
		add_mesh(tm);
	}

	return result;
}

bool Room::check_segment_against(AgainstCollision::Type _againstCollision, REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, Room const * & _endsInRoom, OUT_ Transform & _intoEndRoomTransform, OUT_ DoorInRoomArrayPtr * _throughDoors) const
{
	MEASURE_PERFORMANCE(room_checkSegmentAgainst);
	ARRAY_STACK(DoorInRoom const *, throughDoors, 32);
	bool ret = check_segment_against_worker(_againstCollision, REF_ _segment, REF_ _result, _context, OUT_ throughDoors, 32);
	if (!ret)
	{
		_result.hitLocation = _segment.get_hit();
		_result.hitNormal = -_segment.get_start_to_end_dir();
	}
	else if (_result.hitNormal.is_zero())
	{
		_result.hitNormal = -_segment.get_start_to_end_dir();
	}
	if (throughDoors.is_empty())
	{
		_endsInRoom = this;
		_intoEndRoomTransform = Transform::identity;
	}
	else
	{
		_endsInRoom = throughDoors.get_last()->get_world_active_room_on_other_side();
		_intoEndRoomTransform = Transform::identity;
		for_every_ptr(door, throughDoors)
		{
			_intoEndRoomTransform = door->get_other_room_transform().to_local(_intoEndRoomTransform);
			if (_throughDoors)
			{
				_throughDoors->push_door(door);
			}
		}
		_segment = _intoEndRoomTransform.to_world(_segment);
	}
	_result.inRoom = _endsInRoom;
	_result.intoInRoomTransform = _intoEndRoomTransform;
	_result.to_world_of(_intoEndRoomTransform);
	return ret;
}

bool Room::check_segment_against_worker(AgainstCollision::Type _againstCollision, REF_ Segment & _segment, REF_ CheckSegmentResult & _result, CheckCollisionContext & _context, OUT_ ArrayStack<DoorInRoom const *> & _throughDoors, int _depthLeft) const
{
	debug_context(this);
	Segment localRay = _segment;
	CheckSegmentResult localDoorResult = _result; // just for doors
	DoorInRoom const * goesThroughDoor = nullptr;
	if (is_world_active())
	{
		MEASURE_PERFORMANCE(room_checkSegmentAgainst_door_goesThrough);
		for_every_const_ptr(door, doors)
		{
			if (door->is_visible() &&
				door->has_world_active_room_on_other_side() &&
				door->can_move_through() &&
				door->check_segment(REF_ localRay, REF_ localDoorResult))
			{
				goesThroughDoor = door;
			}
		}
	}

	bool didHitSomething = false;
	if (!_context.should_check_only_doors())
	{
		if (_context.should_check_room())
		{
			MEASURE_PERFORMANCE(room_checkSegmentAgainst_room);

			if (get_against_collision(_againstCollision).check_segment(REF_ localRay, REF_ _result, _context))
			{
				if (_segment.collide_at(localRay.get_end_t()))
				{
					// apply collision info from room type and subworld
					if (_context.is_collision_info_needed())
					{
						if (roomType)
						{
							roomType->get_collision_info_provider().apply_to(_result);
						}
						inSubWorld->get_collision_info_provider().apply_to(_result);
					}
					didHitSomething = true;
					_result.presenceLink = nullptr;
				}
			}
		}

		// check segment against all presence links
		if (is_world_active())
		{
			auto const * cache = _context.get_cache();
			if (cache && cache->is_valid() && cache->get_for_room() == this)
			{
				MEASURE_PERFORMANCE(room_checkSegmentAgainst_presenceLinks_cached);
				for_every_ptr(presenceLink, cache->get_cached_presence_links())
				{
					if (presenceLink->check_segment_against(_againstCollision, REF_ localRay, REF_ _result, _context))
					{
						_segment.collide_at(localRay.get_end_t());
						didHitSomething = true;
					}
				}
			}
			else if (PresenceLink const * presenceLink = presenceLinks) // fallback/standard method
			{
				MEASURE_PERFORMANCE(room_checkSegmentAgainst_presenceLinks);
				while (presenceLink)
				{
					if (presenceLink->is_valid_for_collision() &&
						presenceLink->check_segment_against(_againstCollision, REF_ localRay, REF_ _result, _context))
					{
						_segment.collide_at(localRay.get_end_t());
						didHitSomething = true;
					}
					presenceLink = presenceLink->get_next_in_room();
				}
			}
		}
		else
		{
			// if not activated yet, we don't have a list of presence links and we have to deal with objects
			Concurrency::ScopedMRSWLockRead lock(objectsLock);
			for_every_ptr(object, allObjects)
			{
				if (PresenceLink::check_segment_against(object, object->get_presence()->get_placement(), _againstCollision, REF_ localRay, REF_ _result, _context))
				{
					_segment.collide_at(localRay.get_end_t());
					didHitSomething = true;
				}
			}
		}

		if (_context.should_check_doors())
		{
			if (is_world_active())
			{
				MEASURE_PERFORMANCE(room_checkSegmentAgainst_doors);
				for_every_ptr(door, get_doors())
				{
					if (door->is_visible() &&
						door->check_segment_against(_againstCollision, REF_ localRay, REF_ _result, _context))
					{
						_segment.collide_at(localRay.get_end_t());
						didHitSomething = true;
					}
				}
			}
		}
	}

	debug_no_context();

	if (didHitSomething)
	{
		return true;
	}

	// we didn't hit anything in room
	if (goesThroughDoor && _depthLeft)
	{
		_throughDoors.push_back(goesThroughDoor);
		Segment beyondDoor(goesThroughDoor->get_other_room_transform().location_to_local(_segment.get_start()),
						   goesThroughDoor->get_other_room_transform().location_to_local(_segment.get_end()));
		beyondDoor.set_start_end(localRay.get_end_t(), _segment.get_end_t());
		CheckCollisionCache const * swappedCached = _context.swap_cache(nullptr);
		bool ret = goesThroughDoor->get_world_active_room_on_other_side()->check_segment_against_worker(_againstCollision, REF_ beyondDoor, REF_ _result, _context, OUT_ _throughDoors, _depthLeft - 1);
		_context.swap_cache(swappedCached);
		if (ret)
		{
			_segment.set_end(beyondDoor.get_end_t());
			_result.to_world_of(goesThroughDoor->get_other_room_transform());
		}
		return ret;
	}
	else
	{
		// if doesn't go through door, just fill where it ends
		return false;
	}
}

Optional<Vector3> Room::get_environmental_dir(Name const& _environmentalID) const
{
	// no lock here
	for_count(int, i, environmentalDirs.get_size())
	{
		auto& ed = environmentalDirs[i];
		if (ed.id == _environmentalID)
		{
			return ed.dir;
		}
	}
	return NP;
}

void Room::set_environmental_dir(Name const& _environmentalID, Vector3 const& _dir)
{
	Concurrency::ScopedSpinLock lock(environmentalDirsLock);
	for_every(ed, environmentalDirs)
	{
		if (ed->id == _environmentalID)
		{
			ed->dir = _dir;
			return;
		}
	}
	an_assert(environmentalDirs.has_place_left());
	EnvironmentalDir ed;
	ed.id = _environmentalID;
	ed.dir = _dir;
	environmentalDirs.push_back(ed);
}

Optional<Vector3> Room::get_gravity_dir() const
{
	Optional<Vector3> gravityDir;
	Optional<float> gravityForce;
	if (roomType)
	{
		roomType->get_collision_info_provider().apply_gravity_to(REF_ gravityDir, REF_ gravityForce);
	}
	inSubWorld->get_collision_info_provider().apply_gravity_to(REF_ gravityDir, REF_ gravityForce);

	return gravityDir;
}

void Room::update_gravity(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _gravityForce)
{
	if (roomType)
	{
		roomType->get_collision_info_provider().apply_gravity_to(REF_ _gravityDir, REF_ _gravityForce);
	}
	inSubWorld->get_collision_info_provider().apply_gravity_to(REF_ _gravityDir, REF_ _gravityForce);
}

void Room::update_kill_gravity_distance(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _killGravityDistance)
{
	// clear and start from clean state
	_gravityDir.clear();
	_killGravityDistance.clear();

	Optional<Name> _killGravityAnchorPOI;
	Optional<Name> _killGravityAnchorParam;
	Optional<Name> _killGravityOverrideAnchorPOI;
	Optional<Name> _killGravityOverrideAnchorParam;

	if (roomType)
	{
		roomType->get_collision_info_provider().apply_kill_gravity_distance_to(REF_ _gravityDir, REF_ _killGravityDistance, REF_ _killGravityAnchorPOI, REF_ _killGravityAnchorParam, REF_ _killGravityOverrideAnchorPOI, REF_ _killGravityOverrideAnchorParam);
	}
	inSubWorld->get_collision_info_provider().apply_kill_gravity_distance_to(REF_ _gravityDir, REF_ _killGravityDistance, REF_ _killGravityAnchorPOI, REF_ _killGravityAnchorParam, REF_ _killGravityOverrideAnchorPOI, REF_ _killGravityOverrideAnchorParam);

	Concurrency::ScopedSpinLock lock(killGravityCacheLock);

	if (_gravityDir.is_set() &&
		(_killGravityAnchorPOI.is_set() ||
		 _killGravityAnchorParam.is_set() ||
		 _killGravityOverrideAnchorPOI.is_set() ||
		 _killGravityOverrideAnchorParam.is_set()))
	{
		Optional<float> killGravityAnchorDistance;
		if (_killGravityAnchorPOI.is_set())
		{
			if (killGravityAnchorPOI != _killGravityAnchorPOI.get())
			{
				killGravityAnchorPOI = _killGravityAnchorPOI.get();
				killGravityAnchorPOIDistance.clear();
				for_every_point_of_interest(killGravityAnchorPOI, [this, _gravityDir](PointOfInterestInstance* _poi)
					{
						Transform placement = _poi->calculate_placement();
						killGravityAnchorPOIDistance = Vector3::dot(placement.get_translation(), _gravityDir.get());
					}, true /* include objects, sceneries */);
			}
			if (killGravityAnchorPOIDistance.is_set())
			{
				killGravityAnchorDistance = killGravityAnchorPOIDistance.get();
			}
		}
		if (_killGravityAnchorParam.is_set())
		{
			if (killGravityAnchorParam != _killGravityAnchorParam.get())
			{
				killGravityAnchorParam = _killGravityAnchorParam.get();
				killGravityAnchorParamDistance.clear();
				if (has_value<Transform>(killGravityAnchorParam))
				{
					Transform anchorPlacement = get_value<Transform>(killGravityAnchorParam, Transform::identity);
					killGravityAnchorParamDistance = Vector3::dot(anchorPlacement.get_translation(), _gravityDir.get());
				}
			}
			if (killGravityAnchorParamDistance.is_set())
			{
				killGravityAnchorDistance = killGravityAnchorParamDistance.get();
			}
		}
		if (_killGravityOverrideAnchorPOI.is_set())
		{
			if (killGravityOverrideAnchorPOI != _killGravityOverrideAnchorPOI.get())
			{
				killGravityOverrideAnchorPOI = _killGravityOverrideAnchorPOI.get();
				killGravityOverrideAnchorPOIDistance.clear();
				for_every_point_of_interest(killGravityOverrideAnchorPOI, [this, _gravityDir](PointOfInterestInstance* _poi)
					{
						Transform placement = _poi->calculate_placement();
						killGravityOverrideAnchorPOIDistance = Vector3::dot(placement.get_translation(), _gravityDir.get());
					}, true /* include objects, sceneries */);
			}
			if (killGravityOverrideAnchorPOIDistance.is_set())
			{
				_killGravityDistance = 0.0f; // we override everything
				killGravityAnchorDistance = killGravityOverrideAnchorPOIDistance.get();
			}
		}
		if (_killGravityOverrideAnchorParam.is_set())
		{
			if (killGravityOverrideAnchorParam != _killGravityOverrideAnchorParam.get())
			{
				killGravityOverrideAnchorParam = _killGravityOverrideAnchorParam.get();
				killGravityOverrideAnchorParamDistance.clear();
				if (has_value<Transform>(killGravityOverrideAnchorParam))
				{
					Transform anchorPlacement = get_value<Transform>(killGravityOverrideAnchorParam, Transform::identity);
					killGravityOverrideAnchorParamDistance = Vector3::dot(anchorPlacement.get_translation(), _gravityDir.get());
				}
			}
			if (killGravityOverrideAnchorParamDistance.is_set())
			{
				_killGravityDistance = 0.0f; // we override everything
				killGravityAnchorDistance = killGravityOverrideAnchorParamDistance.get();
			}
		}
		if (killGravityAnchorDistance.is_set())
		{
			_killGravityDistance = _killGravityDistance.get(0.0f) + killGravityAnchorDistance.get();
		}
	}
}

void Room::on_environment_destroyed(Environment * _environment)
{
	if (ownEnvironment == _environment)
	{
		ownEnvironment = nullptr;
	}
	if (environmentUsage.environment == _environment)
	{
		set_environment(nullptr);
	}
}

void Room::set_environment(Environment* _environment, Transform const & _environmentPlacement)
{
	if (environmentUsage.environment == _environment)
	{
		return;
	}
	if (environmentUsage.environment)
	{
		environmentUsage.environment->remove_room(this);
	}
	environmentUsage.environment = _environment;
	environmentUsage.requestedPlacement = _environmentPlacement;
	if (environmentUsage.environment)
	{
		environmentUsage.environment->add_room(this);
	}
	update_own_environment_parent();
}

void Room::create_own_copy_of_environment()
{
	if (ownEnvironment == nullptr && environmentUsage.environment)
	{
		Game::get()->perform_sync_world_job(TXT("create own copy of environment"), 
		[this]()
		{
			auto rg = get_individual_random_generator();
			rg.advance_seed(80654, 23947);
			ownEnvironment = new Environment(NAME(own), environmentUsage.environment->get_name(), inSubWorld, inRegion, environmentUsage.environment->get_environment_type(), rg);
		});
		ownEnvironment->set_parent_environment(environmentUsage.environment);
		if (environmentUsage.environment)
		{
			// this way parent environment may have control over this one
			ownEnvironment->access_info().access_params().clear();
		}
		set_environment(ownEnvironment, environmentUsage.requestedPlacement);
	}
}

void Room::debug_draw_collision(int _againstCollision, bool _objectsOnly, Object* _skipObject)
{
	debug_context(this);
	if (!_objectsOnly)
	{
		if (_againstCollision == NONE || _againstCollision == AgainstCollision::Movement)
		{
			movementCollision.debug_draw(true, false, Colour::red, 0.1f);
		}
		if (_againstCollision == NONE || _againstCollision == AgainstCollision::Precise)
		{
			preciseCollision.debug_draw(true, false, Colour::cyan, 0.1f);
		}
		for_every_ptr(door, allDoors)
		{
			door->debug_draw_collision(_againstCollision);
		}
	}
	debug_no_context();
	// objects got their own context
	{
		Concurrency::ScopedMRSWLockRead lock(objectsLock);
		for_every_ptr(object, allObjects)
		{
			if (object != _skipObject)
			{
				if (ModuleCollision const * oc = object->get_collision())
				{
					oc->debug_draw(_againstCollision);
				}
			}
		}
	}
	{
		Concurrency::ScopedMRSWLockRead lock(temporaryObjectsLock);
		for_every_ptr(object, allTemporaryObjects)
		{
			if (ModuleCollision const * oc = object->get_collision())
			{
				oc->debug_draw(_againstCollision);
			}
		}
	}
}

void Room::debug_draw_door_holes()
{
	debug_context(this);
	for_every_ptr(door, allDoors)
	{
		Colour colour = door->get_side() == DoorSide::A ? Colour::green : Colour::blue;
		door->debug_draw_hole(true, colour);
		debug_draw_text(true, Colour::green, door->get_hole_centre_WS(), Vector2::half, true, 0.6f, NP, TXT("%i"), for_everys_index(door));
		debug_draw_sphere(true, false, colour, 0.2f, Sphere(door->get_placement().get_translation(), 0.1f));
		debug_draw_line(true, colour, door->get_placement().get_translation(), door->get_placement().get_translation() + door->get_plane().get_normal() * 0.3f);
	}
	debug_no_context();
}

void Room::debug_draw_volumetric_limit()
{
	debug_context(this);
	volumetricLimit.debug_draw(true, false, Colour::orange, 0.4f);
	debug_no_context();
}

void Room::debug_draw_skeletons()
{
	debug_context(this);
	//debug_no_context(); ??
	// objects got their own context
	{
		Concurrency::ScopedMRSWLockRead lock(objectsLock); 
		for_every_ptr(object, allObjects)
		{
			if (ModuleAppearance const * oc = object->get_appearance())
			{
				oc->debug_draw_skeleton();
			}
		}
	}
	{
		Concurrency::ScopedMRSWLockRead lock(temporaryObjectsLock);
		for_every_ptr(object, allTemporaryObjects)
		{
			if (ModuleAppearance const * oc = object->get_appearance())
			{
				oc->debug_draw_skeleton();
			}
		}
	}
	debug_no_context();
}

void Room::debug_draw_sockets()
{
	debug_context(this);
	// objects got their own context
	{
		Concurrency::ScopedMRSWLockRead lock(objectsLock); 
		for_every_ptr(object, allObjects)
		{
			if (ModuleAppearance const * oc = object->get_appearance())
			{
				oc->debug_draw_sockets();
			}
		}
	}
	{
		Concurrency::ScopedMRSWLockRead lock(temporaryObjectsLock);
		for_every_ptr(object, allTemporaryObjects)
		{
			if (ModuleAppearance const * oc = object->get_appearance())
			{
				oc->debug_draw_sockets();
			}
		}
	}
	debug_no_context();
}

void Room::debug_draw_pois(bool _setDebugContext)
{
	// pois got their own context
	for_every_ref(poi, pois)
	{
		if (!poi->is_cancelled())
		{
			poi->debug_draw(_setDebugContext);
		}
	}
	for_every_ptr(roomPartInstance, roomPartInstances)
	{
		roomPartInstance->debug_draw_pois(_setDebugContext);
	}
	{
		Concurrency::ScopedMRSWLockRead lock(objectsLock); 
		for_every_ptr(object, allObjects)
		{
			if (auto* appearance = object->get_appearance())
			{
				appearance->debug_draw_pois(_setDebugContext);
			}
		}
	}
}

void Room::debug_draw_mesh_nodes()
{
#ifdef AN_DEBUG_RENDERER
	debug_context(this);
	// pois got their own context
	for_every_ptr(meshInstance, get_appearance().get_instances())
	{
		if (auto * mesh = find_used_mesh_for(meshInstance->get_mesh()))
		{
			for_every_ref(mn, mesh->get_mesh_nodes())
			{
				mn->debug_draw(meshInstance->get_placement());
			}
		}
	}
	for_every_ptr(roomPartInstance, roomPartInstances)
	{
		roomPartInstance->debug_draw_mesh_nodes();
	}
	debug_no_context();
#endif
}

void Room::debug_draw_nav_mesh()
{
	debug_filter(navMesh);
	debug_context(this);

	{
		Concurrency::ScopedMRSWLockRead lock(navMeshesLock);

		for_every_ref(navMesh, navMeshes)
		{
			navMesh->debug_draw();
		}
	}

	debug_no_context();
	debug_no_filter();
}

void Room::debug_draw_space_blockers()
{
	debug_context(this);

	update_space_blockers();

	{
		Concurrency::ScopedMRSWLockRead lock(spaceBlockersLock);

		spaceBlockers.debug_draw();
	}

	debug_no_context();
}

void Room::add_poi(PointOfInterest* _poi, Transform const & _placement)
{
	ASSERT_NOT_ASYNC_OR(get_in_world()->multithread_check__reading_world_is_safe());

	Random::Generator rg = get_individual_random_generator();
	rg.advance_seed(239478 + usedMeshes.get_size(), 234 * pois.get_size());

	pois.push_back(PointOfInterestInstancePtr(Game::get()->get_customisation().create_point_of_interest_instance()));
	pois.get_last()->access_tags().set_tags_from(markPOIsInside);
	pois.get_last()->create_instance(rg.spawn().temp_ref(), _poi, this, nullptr, pois.get_size() - 1, _placement, nullptr, nullptr);
	poisRequireAdvancement |= pois.get_last()->does_require_advancement();
}

void Room::add_volumetric_limit_poi(Vector3 const & _at)
{
	PointOfInterest* poi = new PointOfInterest();
	poi->tags.set_tag(NAME(volumetricLimit));
	add_poi(poi, Transform(_at, Quat::identity));
}

void Room::add_volumetric_limit_pois(Range3 const & _bbox)
{
	PointOfInterest* poi = new PointOfInterest();
	poi->tags.set_tag(NAME(volumetricLimit));
	add_poi(poi, Transform(Vector3(_bbox.x.min, _bbox.y.min, _bbox.z.min), Quat::identity));
	add_poi(poi, Transform(Vector3(_bbox.x.max, _bbox.y.min, _bbox.z.min), Quat::identity));
	add_poi(poi, Transform(Vector3(_bbox.x.min, _bbox.y.max, _bbox.z.min), Quat::identity));
	add_poi(poi, Transform(Vector3(_bbox.x.max, _bbox.y.max, _bbox.z.min), Quat::identity));
	add_poi(poi, Transform(Vector3(_bbox.x.min, _bbox.y.min, _bbox.z.max), Quat::identity));
	add_poi(poi, Transform(Vector3(_bbox.x.max, _bbox.y.min, _bbox.z.max), Quat::identity));
	add_poi(poi, Transform(Vector3(_bbox.x.min, _bbox.y.max, _bbox.z.max), Quat::identity));
	add_poi(poi, Transform(Vector3(_bbox.x.max, _bbox.y.max, _bbox.z.max), Quat::identity));
}

bool Room::find_any_point_of_interest(Optional<Name> const& _poiName, OUT_ PointOfInterestInstance*& _outPOI, Optional<bool> const& _includeObjects, Random::Generator* _randomGenerator, IsPointOfInterestValid _is_valid)
{
	ARRAY_STACK(PointOfInterestInstance*, pois, 32);
	Random::Generator randomGenerator;
	if (!_randomGenerator)
	{
		_randomGenerator = &randomGenerator;
	}

	for_every_point_of_interest(_poiName, [&pois, _randomGenerator](PointOfInterestInstance* _fpoi) {pois.push_back_or_replace(_fpoi, *_randomGenerator); }, _includeObjects, _is_valid);

	if (pois.get_size() > 0)
	{
		int idx = _randomGenerator->get_int(pois.get_size());
		_outPOI = pois[idx];
		return true;
	}
	else
	{
		_outPOI = nullptr;
		return false;
	}
}

void Room::for_every_point_of_interest(Optional<Name> const& _poiName, OnFoundPointOfInterest _fpoi, Optional<bool> const & _includeObjects, IsPointOfInterestValid _is_valid) const
{
	ASSERT_NOT_ASYNC_OR(get_in_world()->multithread_check__reading_world_is_safe());
	for_every_ref(poi, pois)
	{
		if (!poi->is_cancelled() &&
			(!_poiName.is_set() || poi->get_name() == _poiName.get()) &&
			(! _is_valid || _is_valid(poi)))
		{
			_fpoi(poi);
		}
	}
	for_every_const_ptr(roomPartInstance, roomPartInstances)
	{
		roomPartInstance->for_every_point_of_interest(_poiName, _fpoi, this, _is_valid);
	}
	if (_includeObjects.get(true))
	{
		Concurrency::ScopedMRSWLockRead lock(objectsLock);
		for_every_const_ptr(object, allObjects) // we use all objects as we mostly use POIs when dealing with object creation
		{
			if (auto* appearance = object->get_appearance())
			{
				appearance->for_every_point_of_interest(_poiName, _fpoi, _is_valid);
			}
		}
	}
}

void Room::mark_POIs_inside(Tags const & _tags)
{
	if (_tags.is_empty() ||
		_tags.is_contained_within(markPOIsInside))
	{
		return;
	}
	markPOIsInside.set_tags_from(_tags);
	for_every_ref(poi, pois)
	{
		poi->access_tags().set_tags_from(markPOIsInside);
	}
	for_every_const_ptr(roomPartInstance, roomPartInstances)
	{
		for_every_ref(poi, roomPartInstance->get_pois())
		{
			poi->access_tags().set_tags_from(markPOIsInside);
		}
	}
}

void Room::update_volumetric_limit()
{
	volumetricLimit.clear();

	bool added = false;
	for_every_ref(poi, pois)
	{
		if (poi->get_tags().get_tag(NAME(volumetricLimit)))
		{
			volumetricLimit.add(poi->calculate_placement().get_translation());
			added = true;
		}
	}
	for_every_const_ptr(roomPartInstance, roomPartInstances)
	{
		for_every_ref(poi, roomPartInstance->get_pois())
		{
			if (poi->get_tags().get_tag(NAME(volumetricLimit)))
			{
				volumetricLimit.add(poi->calculate_placement().get_translation());
				added = true;
			}
		}
	}

	if (added)
	{
		if (volumetricLimit.get_vertex_count() == 0)
		{
			warn(TXT("volumetric limit empty, room type \"%S\""), volumetricLimit.get_vertex_count(), roomType ? roomType->get_name().to_string().to_char() : TXT("??"));
			// doesn't make much sense
			volumetricLimit.clear();
		}
		else if (volumetricLimit.get_vertex_count() <= 3)
		{
			warn(TXT("volumetric limit with %i point count, room type \"%S\""), volumetricLimit.get_vertex_count(), roomType ? roomType->get_name().to_string().to_char() : TXT("??"));
			// add random ones
			if (volumetricLimit.get_vertex_count() == 3)
			{
				Vector3 a, b, c;
				volumetricLimit.get_vertex(0, a);
				volumetricLimit.get_vertex(1, b);
				volumetricLimit.get_vertex(2, c);
				Plane p(a, b, c);
				volumetricLimit.add((a + b + c) / 3.0f + p.get_normal() * 0.1f);
				volumetricLimit.build();
			}
			else
			{
				while (volumetricLimit.get_vertex_count() <= 3)
				{
					Vector3 p;
					if (volumetricLimit.get_vertex(volumetricLimit.get_vertex_count() - 1, p))
					{
						volumetricLimit.add(p + Vector3(0.01f, 0.01f, 0.01f));
					}
				}
				volumetricLimit.build();
			}
		}
		else
		{
			volumetricLimit.build();
		}
	}
}

void Room::set_in_region(Region* _inRegion)
{
	inRegion = _inRegion;
	update_own_environment_parent();
}

void Room::update_own_environment_parent()
{
	// change parent for environments
	if (ownEnvironment)
	{
		ownEnvironment->update_parent_environment(inRegion);
	}
}

void Room::setup_reverb_from_type()
{
	if (roomType)
	{
		useReverb = roomType->get_reverb();
	}
	if (!useReverb.is_set() && inRegion && inRegion->get_region_type())
	{
		useReverb = inRegion->get_region_type()->get_reverb();
	}
}

void Room::get_use_environment_from_type()
{
	useEnvironment.clear();
	if (roomType)
	{
		useEnvironment = roomType->get_use_environment();
	}
}

void Room::setup_using_use_environment()
{
	if (useEnvironment.is_set())
	{
		Transform environmentAnchor = Transform::identity;
		if (useEnvironment.get().anchorPOI.is_valid())
		{
			for_every_point_of_interest(useEnvironment.get().anchorPOI, [&environmentAnchor](PointOfInterestInstance* _poi)
				{
					environmentAnchor = _poi->calculate_placement();
				});
		}
		if (useEnvironment.get().anchor.is_set())
		{
			environmentAnchor = useEnvironment.get().anchor.get(this, Transform::identity, AllowToFail);
		}
		an_assert(environmentAnchor.get_scale() > 0.5f);
		if (useEnvironment.get().useFromRegion.is_valid())
		{
			// find in region and use it
			an_assert(useEnvironment.get().placement.get_scale() > 0.5f);
			if (inRegion)
			{
				if (auto* e = inRegion->find_environment(useEnvironment.get().useFromRegion, false, true))
				{
					set_environment(e, environmentAnchor.to_world(useEnvironment.get().placement));
					return;
				}
			}
		}
		if (useEnvironment.get().useFromFirstDoor)
		{
			if (get_all_doors().get_size() > 0)
			{
				if (auto* room = get_all_doors().get_first()->get_world_active_room_on_other_side())
				{
					an_assert(useEnvironment.get().placement.get_scale() > 0.5f);
					set_environment(room->get_environment(), environmentAnchor.to_world(useEnvironment.get().placement));
					create_own_copy_of_environment(); // we should always have a copy of this environment
					return;
				}
				else
				{
					error(TXT("no room beyond first door"));
				}
			}
			else
			{
				error(TXT("no first door"));
			}
		}
		if (useEnvironment.get().useEnvironmentType.get())
		{
			Game::get()->perform_sync_world_job(TXT("create own environment"),
				[this]()
			{
				delete_and_clear(ownEnvironment);
				auto rg = get_individual_random_generator();
				rg.advance_seed(80654, 23947);
				ownEnvironment = new Environment(NAME(own), useEnvironment.get().parentEnvironment, inSubWorld, inRegion, useEnvironment.get().useEnvironmentType.get(), rg);
			});
			an_assert(useEnvironment.get().placement.get_scale() > 0.5f);
			set_environment(ownEnvironment, environmentAnchor.to_world(useEnvironment.get().placement));
			return;
		}
	}
	// fallback/default path: find default in region and use it
	set_environment(inRegion ? inRegion->get_default_environment() : nullptr);
}

struct Room_DeleteCombinedAppearance
: public Concurrency::AsynchronousJobData
{
	Meshes::CombinedMesh3DInstanceSet* combinedAppearance;
	Room_DeleteCombinedAppearance(Meshes::CombinedMesh3DInstanceSet* _combinedAppearance)
	: combinedAppearance(_combinedAppearance)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		Room_DeleteCombinedAppearance* data = (Room_DeleteCombinedAppearance*)_data;
		delete data->combinedAppearance;
	}
};

void Room::defer_combined_apperance_delete()
{
	if (combinedAppearance)
	{
		Game::async_system_job(Game::get(), Room_DeleteCombinedAppearance::perform, new Room_DeleteCombinedAppearance(combinedAppearance));
		// here we may consider it deleted
		combinedAppearance = nullptr;
	}
}

void Room::apply_replacer_to_mesh(REF_ Mesh const* & _mesh) const
{
	apply_replacer_to<Mesh const>(REF_ _mesh);
}

template <typename Class>
void Room::apply_replacer_to(REF_ Class* & _object) const
{
	if (roomType && roomType->get_library_stored_replacer().apply_to<Class>(REF_ _object))
	{
		return;
	}
	Region const * region = get_in_region();
	while (region)
	{
		if (RegionType const * regionType = region->get_region_type())
		{
			if (regionType->get_library_stored_replacer().apply_to<Class>(REF_ _object))
			{
				return;
			}
		}
		region = region->get_in_region();
	}
}

Door* Room::find_door_tagged(Name const & _tag)
{
	ASSERT_NOT_ASYNC_OR(get_in_world()->multithread_check__reading_world_is_safe());
	for_every_ptr(dir, allDoors)
	{
		if (Door* door = dir->get_door())
		{
			if (dir->get_tags().get_tag(_tag))
			{
				return door;;
			}
			if (door->get_tags().get_tag(_tag))
			{
				return door;
			}
		}
	}
	return nullptr;
}

void Room::world_activate()
{
	Concurrency::ScopedSpinLock lock(worldActivationLock);

#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("r%p world activate"), this);
#endif

	update_volumetric_limit();

	WorldObject::world_activate();
	if (auto* inWorld = get_in_world())
	{
		inWorld->activate(this);
	}
	if (inSubWorld)
	{
		inSubWorld->activate(this);
	}

	{
		Concurrency::ScopedMRSWLockRead lock(temporaryObjectsLock);
		for_every_ptr(temporaryObject, temporaryObjectsToActivate)
		{
			temporaryObject->mark_to_activate();
		}
		temporaryObjectsToActivate.clear();
	}

	mark_nav_mesh_creation_required();
}

bool Room::get_doors(int & _doorCount, ArrayStack<DoorInRoom*>* _doors) const
{
	Concurrency::ScopedMRSWLockRead lock(doorsLock);
	if (_doors)
	{
		_doorCount = min(_doorCount, doors.get_size());
		_doors->set_size(_doorCount);
		memory_copy(_doors->get_data(), doors.get_data(), sizeof(DoorInRoom*) * _doorCount);
	}
	else
	{
		_doorCount = doors.get_size();
	}
	return _doorCount != 0;
}

bool Room::get_all_doors(int & _doorCount, ArrayStack<DoorInRoom*>* _doors) const
{
	Concurrency::ScopedMRSWLockRead lock(doorsLock);
	if (_doors)
	{
		_doorCount = min(_doorCount, allDoors.get_size());
		_doors->set_size(_doorCount);
		memory_copy(_doors->get_data(), allDoors.get_data(), sizeof(DoorInRoom*) * _doorCount);
	}
	else
	{
		_doorCount = allDoors.get_size();
	}
	return _doorCount != 0;
}

bool Room::get_objects(int & _objectCount, ArrayStack<Object*>* _objects) const
{
	Concurrency::ScopedMRSWLockRead lock(objectsLock);
	if (_objects)
	{
		_objectCount = min(_objectCount, objects.get_size());
		_objects->set_size(_objectCount);
		memory_copy(_objects->get_data(), objects.get_data(), sizeof(Object*) * _objectCount);
	}
	else
	{
		_objectCount = objects.get_size();
	}
	return _objectCount != 0;
}

bool Room::get_all_objects(int & _objectCount, ArrayStack<Object*>* _objects) const
{
	Concurrency::ScopedMRSWLockRead lock(objectsLock);
	if (_objects)
	{
		_objectCount = min(_objectCount, allObjects.get_size());
		_objects->set_size(_objectCount);
		memory_copy(_objects->get_data(), allObjects.get_data(), sizeof(Object*) * _objectCount);
	}
	else
	{
		_objectCount = allObjects.get_size();
	}
	return _objectCount != 0;
}

bool Room::get_temporary_objects(int & _objectCount, ArrayStack<TemporaryObject*>* _objects) const
{
	Concurrency::ScopedMRSWLockRead lock(temporaryObjectsLock);
	if (_objects)
	{
		_objectCount = min(_objectCount, temporaryObjects.get_size());
		_objects->set_size(_objectCount);
		memory_copy(_objects->get_data(), temporaryObjects.get_data(), sizeof(TemporaryObject*) * _objectCount);
	}
	else
	{
		_objectCount = temporaryObjects.get_size();
	}
	return _objectCount != 0;
}

#ifdef AN_DEVELOPMENT
void Room::assert_we_dont_modify_doors() const
{
	an_assert_immediate(!is_world_active() || !get_in_world() || get_in_world()->multithread_check__reading_rooms_doors_is_safe() || Game::get_as<PreviewGame>());
}

void Room::assert_we_may_modify_doors() const
{
	an_assert_immediate(!is_world_active() || !get_in_world() || get_in_world()->multithread_check__writing_rooms_doors_is_allowed() || Game::get_as<PreviewGame>());
}

void Room::assert_we_dont_modify_objects() const
{
	an_assert_immediate(!is_world_active() || !get_in_world() || get_in_world()->multithread_check__reading_rooms_objects_is_safe() || Game::get_as<PreviewGame>());
}

void Room::assert_we_may_modify_objects() const
{
	an_assert_immediate(!is_world_active() || !get_in_world() || get_in_world()->multithread_check__writing_rooms_objects_is_allowed() || Game::get_as<PreviewGame>());
}
#endif

struct ReadyRoomForGame
{
	Room* room = nullptr;
	DoorInRoom* throughDoorInRoom = nullptr; // in this room
	RoomGeneratorInfo const* roomGeneratorInfo = nullptr;
	int generationPriority = 0;

	ReadyRoomForGame() {}
	ReadyRoomForGame(Room* _room, DoorInRoom* _throughDoorInRoom = nullptr)
		: room(_room)
		, throughDoorInRoom(_throughDoorInRoom)
		, roomGeneratorInfo(room->get_room_generator_info())
		, generationPriority(roomGeneratorInfo ? roomGeneratorInfo->get_room_generator_priority() : 0)
	{
	}

	static void gather(Array<ReadyRoomForGame>& _rooms, Room* _room)
	{
		if (!_room)
		{
			return;
		}
#ifdef AN_OUTPUT_READY_FOR_GAME
		output(TXT("gather to ready room for vr"));
#endif
		_rooms.clear();
		_rooms.push_back(ReadyRoomForGame(_room));
		while (true)
		{
			Room* roomToAdd = nullptr;
			DoorInRoom* roomToAddThroughDoor = nullptr;
			for (int i = 0; i < _rooms.get_size(); ++i)
			{
				Room* room = _rooms[i].room;
				DoorInRoom* throughdir = _rooms[i].throughDoorInRoom;
				for_every_ptr(dir, room->get_all_doors())
				{
					// invisible doors are fine here
					if (dir != throughdir)
					{
						if (Room* toRoom = dir->get_room_on_other_side())
						{
							bool alreadyAdded = false;
							for_every(r, _rooms)
							{
								if (r->room == toRoom)
								{
									alreadyAdded = true;
									break;
								}
							}
							if (!alreadyAdded)
							{
								if (!roomToAdd)
								{
									roomToAdd = toRoom;
									roomToAddThroughDoor = dir->get_door_on_other_side();
								}
							}
						}
					}
				}
			}
			if (roomToAdd)
			{
				_rooms.push_back(ReadyRoomForGame(roomToAdd, roomToAddThroughDoor));
			}
			else
			{
				break;
			}
		}
#ifdef AN_OUTPUT_READY_FOR_GAME
#ifdef AN_OUTPUT_GATHERED_TO_READY_FOR_GAME
		for_every(r, _rooms)
		{
			output(TXT(" + r%p \"%S\" [%i]"), r->room, r->room->get_name().to_char(), r->room->get_activation_group()? r->room->get_activation_group()->get_id() : -1);
		}
#endif
#endif

	}
};

void Room::check_if_doors_valid(Optional<VR::Zone> _beInZone) const
{
	if (get_tags().get_tag(NAME(inaccessible)) ||
		get_tags().get_tag(NAME(vrInaccessible)))
	{
		// no need to check
		return;
	}
	if (!_beInZone.is_set())
	{
		_beInZone = Game::get()->get_vr_zone(nullptr /* in general */);
	}
	if (checkIfDoorsAreTooClose &&
		get_all_doors().get_size() == 2 &&
		get_all_doors()[0]->is_placed() &&
		get_all_doors()[1]->is_placed())
	{
		// check if aren't too close
		Transform d0 = get_all_doors()[0]->get_placement();
		Transform d1 = get_all_doors()[1]->get_placement();
		Vector2 d0f = d0.get_axis(Axis::Forward).to_vector2();
		Vector2 d1f = d1.get_axis(Axis::Forward).to_vector2();
		if (Vector2::dot(d0f, d1f) < -0.8f)
		{
			// opposite dirs
			Vector2 d1rel0 = d0.location_to_local(d1.get_translation()).to_vector2();
			if (auto* d = get_all_doors()[0]->get_door())
			{
				float doorWidth = d->calculate_vr_width();
				if (d1rel0.y < 0.0f && d1rel0.y > -doorWidth + 0.02f)
				{
					// really close

					bool wide = abs(d1rel0.x) > doorWidth * 0.5f;
					if ((! wide && abs(d1rel0.x) > abs(d1rel0.y) * 0.5f) ||
						(wide && abs(d1rel0.y) < doorWidth * 0.75f))
					{
#ifdef AN_DEVELOPMENT
						DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("check room: doors too close!"))));
						dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
						dv->activate();
						dv->add_text(Vector2::zero, get_name(), Colour::red, 0.3f);
						_beInZone.get().debug_visualise(dv.get(), Colour::black.with_alpha(0.4f));
						for_every_ptr(dir, get_all_doors())
						{
							// invisible doors are fine here
							if (!dir->is_vr_space_placement_set())
							{
								continue;
							}
							if (auto* d = dir->get_door())
							{
								float doorWidth = d->calculate_vr_width();
								Transform vrSpacePlacementD = dir->get_vr_space_placement();
								Colour colour = Colour::red;
								DebugVisualiserUtils::add_door_to_debug_visualiser(dv, dir, vrSpacePlacementD.get_translation().to_vector2(), vrSpacePlacementD.get_axis(Axis::Forward).to_vector2(), doorWidth, colour);
							}
						}
						dv->end_gathering_data();
						dv->show_and_wait_for_key();
#endif
#ifdef AN_OUTPUT_WORLD_GENERATION
						error(TXT("doors too close and not aligned \"%S\""), get_name().to_char());
#endif
					}
				}
			}
		}
	}
	// check if doors are inside the play area
	{
#ifdef AN_OUTPUT_WORLD_GENERATION
		output(TXT("check if doors fit"));
#endif
		bool allFit = true;
		for_every_ptr(dir, get_all_doors())
		{
			// invisible doors are fine here
			if (!dir->is_vr_space_placement_set() ||
				!dir->can_move_through() ||
				!dir->is_relevant_for_movement() ||
				!dir->is_relevant_for_vr() ||
				dir->may_ignore_vr_placement())
			{
				continue;
			}
			if (auto* d = dir->get_door())
			{
				float doorWidth = d->calculate_vr_width();
				Transform vrSpacePlacementD = dir->get_vr_space_placement();
				if (!_beInZone.get().does_contain(vrSpacePlacementD.get_translation().to_vector2(), doorWidth * 0.4f))
				{
					allFit = false;
				}
			}
		}
		if (!allFit)
		{
#ifdef AN_DEVELOPMENT
			DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("check room: door outside!"))));
			dv->set_background_colour(Colour::lerp(0.05f, Colour::greyLight, Colour::red));
			dv->activate();
			dv->add_text(Vector2::zero, get_name(), Colour::red, 0.3f);
			_beInZone.get().debug_visualise(dv.get(), Colour::black.with_alpha(0.4f));
			for_every_ptr(dir, get_all_doors())
			{
				// invisible doors are fine here
				if (!dir->is_vr_space_placement_set() ||
					!dir->can_move_through() ||
					!dir->is_relevant_for_movement() ||
					!dir->is_relevant_for_vr() ||
					dir->may_ignore_vr_placement())
				{
					continue;
				}
				if (auto* d = dir->get_door())
				{
					float doorWidth = d->calculate_vr_width();
					Transform vrSpacePlacementD = dir->get_vr_space_placement();
					Colour colour = Colour::greenDark;
					if (!_beInZone.get().does_contain(vrSpacePlacementD.get_translation().to_vector2(), doorWidth * 0.4f))
					{
						colour = Colour::red;
					}
					DebugVisualiserUtils::add_door_to_debug_visualiser(dv, dir, vrSpacePlacementD.get_translation().to_vector2(), vrSpacePlacementD.get_axis(Axis::Forward).to_vector2(), doorWidth, colour);
				}
			}
			dv->end_gathering_data();
			dv->show_and_wait_for_key();
#endif
#ifdef AN_OUTPUT_WORLD_GENERATION
			error(TXT("doors do not fit the play area for \"%S\""), get_name().to_char());
#endif
		}
		else
		{
#ifdef AN_OUTPUT_WORLD_GENERATION
			output(TXT("doors fit the play area"));
#endif
		}
	}
}

void Room::ready_for_game()
{
#ifdef AN_OUTPUT_READY_FOR_GAME
	output(TXT("start ready_for_game with room r%p %S \"%S\" [%i]"), this, get_individual_random_generator().get_seed_string().to_char(), get_name().to_char(), get_activation_group()? get_activation_group()->get_id() : -1);
#endif
	float tileSize = 0.0f;
	if (Game::get()->is_sensitive_to_vr_anchor())
	{
		tileSize = Game::get()->get_vr_tile_size();
	}

	Array<ReadyRoomForGame> readyRooms;
	ReadyRoomForGame::gather(readyRooms, this);

	// handle each room once
	// we have to remember which one we did visit as some rooms after generation may already be vr arranged and generated
	// but we are more interested in how rooms connect to each other by means of vr corridors.
	Array<Room*> handledRooms;

	while (true)
	{
		Room* nextRoom = nullptr;
		DoorInRoom* nextRoomThroughDoor = nullptr;
		int nextRoomGenerationPriority = 0;
		int nextRoomVRCorridorNested = 0;

		// first handle rooms that were not generated at all
		// then, when all rooms are generated or vr arranged, do vr arranged (they can join rooms or break)
		for_count(int, acceptVRArranged, 2)
		{
			for_every(readyRoom, readyRooms)
			{
				if (Game::get()->does_want_to_cancel_creation())
				{
					return;
				}

				if (!handledRooms.does_contain(readyRoom->room))
				{
					// alter only rooms that are not world active
					if (!readyRoom->room->is_world_active() &&
						(acceptVRArranged || !readyRoom->room->is_vr_arranged()))
					{
						if (!readyRoom->throughDoorInRoom ||
							(readyRoom->throughDoorInRoom->get_room_on_other_side() && readyRoom->throughDoorInRoom->get_room_on_other_side()->is_vr_arranged()) ||
							!readyRoom->roomGeneratorInfo->requires_external_vr_placement())
						{
							int readyRoomVRCorridorNested = readyRoom->room->get_tags().get_tag_as_int(NAME(vrCorridorNested));

							// we require at least one door to have vr placement set, to set to other doors, propagate
							if (!nextRoom ||
								(nextRoomVRCorridorNested > readyRoomVRCorridorNested) || // prefer less nested
								(nextRoomVRCorridorNested == readyRoomVRCorridorNested && // if same level of nestness, base on priority
									nextRoomGenerationPriority < readyRoom->generationPriority))
							{
								nextRoom = readyRoom->room;
								nextRoomThroughDoor = readyRoom->throughDoorInRoom;
								nextRoomGenerationPriority = readyRoom->generationPriority;
								nextRoomVRCorridorNested = readyRoomVRCorridorNested;
							}
						}
					}
				}
			}
			if (nextRoom)
			{
				break;
			}
		}

		if (!nextRoom)
		{
			// all done
			break;
		}

		// check if we have vr placement for door we entered to this room (makes sense only if room was not generated, otherwise we're in deep trouble)
		if (Game::get()->is_sensitive_to_vr_anchor())
		{
			if (nextRoomThroughDoor)
			{
				if (!nextRoomThroughDoor->is_vr_space_placement_set() && !nextRoomThroughDoor->may_ignore_vr_placement())
				{
					an_assert(!nextRoom->is_generated(), TXT("if it was generated, it should have set vr placement for all doors"));
					auto const* otherDoor = nextRoomThroughDoor->get_door_on_other_side();
					an_assert(otherDoor);
					// ok, maybe this room was not generated as well, check other door
					if (!otherDoor->is_vr_space_placement_set())
					{
						// check if any other door has vr space placement arranged
						for_every_ptr(dir, nextRoom->get_all_doors())
						{
							if (Game::get()->does_want_to_cancel_creation())
							{
								return;
							}

							// invisible doors are fine here
							if (dir->is_vr_space_placement_set())
							{
								// this one has it! use it instead
								otherDoor = nullptr;
								nextRoomThroughDoor = dir;
								break;
							}
							if (auto const* od = dir->get_door_on_other_side())
							{
								if (od->is_vr_space_placement_set())
								{
									// this one has it! get placement from it
									otherDoor = od;
									nextRoomThroughDoor = dir;
									break;
								}
							}
						}
					}
					if (otherDoor && otherDoor->is_vr_space_placement_set())
					{
						scoped_call_stack_info(TXT("Room::ready_for_game [1]"));
						Transform vrPlacementFromOtherDoor = Door::reverse_door_placement(otherDoor->get_vr_space_placement());
						nextRoomThroughDoor->set_vr_space_placement(vrPlacementFromOtherDoor);
					}
				}
			}
		}

#ifdef AN_OUTPUT_READY_FOR_GAME
		output(TXT("ready_for_game room r%p %S \"%S\""), nextRoom, nextRoom->get_individual_random_generator().get_seed_string().to_char(), nextRoom->get_name().to_char());
		for_every_ptr(dir, nextRoom->get_all_doors())
		{
			auto* r = dir->get_room_on_other_side();
			output(TXT("  door %i ([%p) to room %S \"%S\" %S"), for_everys_index(dir), dir->get_door(),
				r ? r->get_individual_random_generator().get_seed_string().to_char() : TXT("--"), r ? r->get_name().to_char() : TXT("--"),
				dir->get_door() ? (dir->get_door()->is_world_separator_door() ? TXT("world separator") : TXT("usual")) : TXT("no door"));
		}
#endif

		/*
		if (!nextRoom->vrElevatorAltitude.is_set())
		{
			// set from doors (if has any)
			float vrElAlt = 0.0f;
			int count = 0;
			for_every_ptr(dir, nextRoom->get_all_doors())
			{
				// invisible doors are fine here
				if (auto* d = dir->get_door())
				{
					if (d->get_vr_elevator_altitude().is_set())
					{
						vrElAlt += d->get_vr_elevator_altitude().get();
						++count;
					}
					else if (auto* r = dir->get_room_on_other_side())
					{
						if (r->get_vr_elevator_altitude().is_set())
						{
							vrElAlt += r->get_vr_elevator_altitude().get();
							++count;
						}
					}
				}
			}
			if (count > 0)
			{
				vrElAlt /= (float)count;
				nextRoom->set_vr_elevator_altitude(vrElAlt);
			}
		}
		*/

		ROOM_HISTORY(this, TXT("ready for game"));

		if (Game::get()->does_want_to_cancel_creation())
		{
			return;
		}

		// generate it now - after we set vr placement
		// this way, generator may override_ vr placement (if it does so, in next step we will create vr corridor)
		// nextRoomThroughDoor will have updated vr placement
		if (!nextRoom->is_generated())
		{
#ifdef AN_OUTPUT_READY_FOR_GAME
			output(TXT("generate"));
#endif
			nextRoom->generate();
#ifdef AN_OUTPUT_READY_FOR_GAME
			output(TXT("generated"));
#endif
		}

		if (Game::get()->is_sensitive_to_vr_anchor())
		{
			VR::Zone beInZone = Game::get()->get_vr_zone(nextRoom);
			nextRoom->check_if_doors_valid(beInZone);
		}

		if (Game::get()->does_want_to_cancel_creation())
		{
			return;
		}

		// check if there are doors that require placement in vr space
		// this will assert if we marked we are vr arranged but we are not
		if (Game::get()->is_sensitive_to_vr_anchor())
		{
			bool didAnything = nextRoom->set_door_vr_placement_if_not_set(nextRoomThroughDoor);
			if (didAnything && nextRoom->is_vr_arranged())
			{
				warn(TXT("it shouldn't be vr arranged if a door does not have vr placement set (room: %S)"), nextRoom->get_name().to_char());
			}

			nextRoom->mark_vr_arranged();
		}
		else
		{
			// lie
			nextRoom->mark_vr_arranged();
		}

		bool handled = true;
		// check if corridors are required
		if (Game::get()->is_sensitive_to_vr_anchor())
		{
			VR::Zone beInZone = Game::get()->get_vr_zone(nextRoom);
			for_every_ptr(dir, nextRoom->get_all_doors())
			{
				if (Game::get()->does_want_to_cancel_creation())
				{
					return;
				}

				// invisible doors are fine here
				if (!dir->can_move_through() ||
					!dir->is_relevant_for_movement() ||
					!dir->is_relevant_for_vr()) continue;
				if (auto * otherDoor = dir->get_door_on_other_side())
				{
					if (dir->is_vr_space_placement_set() &&
						otherDoor->is_vr_space_placement_set())
					{
						scoped_call_stack_info(TXT("Room::ready_for_game [2]"));
						Transform vrPlacementFromOtherDoor = Door::reverse_door_placement(otherDoor->get_vr_space_placement());
						if (!DoorInRoom::same_with_orientation_for_vr(vrPlacementFromOtherDoor, dir->get_vr_space_placement(), 0.005f))
						{
							if (dir->get_door()->is_important_vr_door())
							{
								if (otherDoor->get_in_room()->is_world_active())
								{
									dir->grow_into_vr_corridor(dir->get_placement_if_placed(), dir->get_vr_space_placement(), beInZone, tileSize);
									handled = false;
									break;
								}
								else
								{
									otherDoor->grow_into_vr_corridor(otherDoor->get_placement_if_placed(), otherDoor->get_vr_space_placement(), beInZone, tileSize);
									handled = false;
									break;
								}
							}
							else
							{
								dir->get_door()->change_into_vr_corridor(beInZone, tileSize);
							}
						}
					}
				}
			}
		}

		if (handled)
		{
			handledRooms.push_back(nextRoom);
		}

		// regather rooms as we could add rooms or remove them
		ReadyRoomForGame::gather(readyRooms, this);
	}

	// add all missing doors
	if (Game::get()->is_sensitive_to_vr_anchor())
	{
		while (true)
		{
			bool allOk = true;
			ReadyRoomForGame::gather(readyRooms, this);
			for_every(readyRoom, readyRooms)
			{
				auto* room = readyRoom->room;
				VR::Zone beInZone = Game::get()->get_vr_zone(room);
				for_every_ptr(dir, room->get_all_doors())
				{
					if (Game::get()->does_want_to_cancel_creation())
					{
						return;
					}
					// invisible doors are fine here
					if (!dir->can_move_through() ||
						!dir->is_relevant_for_movement() ||
						!dir->is_relevant_for_vr()) continue;
					if (auto const * otherDoor = dir->get_door_on_other_side())
					{
						if (dir->is_vr_space_placement_set() &&
							otherDoor->is_vr_space_placement_set())
						{
							scoped_call_stack_info(TXT("Room::ready_for_game [3]"));
							Transform vrPlacementFromOtherDoor = Door::reverse_door_placement(otherDoor->get_vr_space_placement());
							if (!DoorInRoom::same_with_orientation_for_vr(vrPlacementFromOtherDoor, dir->get_vr_space_placement(), 0.005f))
							{
								allOk = false;
								dir->get_door()->change_into_vr_corridor(beInZone, tileSize);
							}
						}
					}
					if (!allOk)
					{
						break;
					}
				}
				if (!allOk)
				{
					break;
				}
			}
			if (allOk)
			{
				break;
			}
		}

#ifdef AN_DEVELOPMENT
		ReadyRoomForGame::gather(readyRooms, this);
		for_every(readyRoom, readyRooms)
		{
			auto* room = readyRoom->room;
			for_every_ptr(dir, room->get_all_doors())
			{
				// invisible doors are fine here
				if (!dir->can_move_through() ||
					!dir->is_relevant_for_movement() ||
					!dir->is_relevant_for_vr()) continue;
				if (auto const * otherDoor = dir->get_door_on_other_side())
				{
					if (dir->is_vr_space_placement_set() &&
						otherDoor->is_vr_space_placement_set())
					{
						scoped_call_stack_info(TXT("Room::ready_for_game [4]"));
						Transform vrPlacementFromOtherDoor = Door::reverse_door_placement(otherDoor->get_vr_space_placement());
						an_assert(DoorInRoom::same_with_orientation_for_vr(vrPlacementFromOtherDoor, dir->get_vr_space_placement(), 0.005f));
					}
				}
			}
		}
#endif
	}

#ifdef AN_OUTPUT_READY_FOR_GAME
	output(TXT("done ready_for_game with room r%p %S \"%S\""), this, get_individual_random_generator().get_seed_string().to_char(), get_name().to_char());
#endif
}

Transform Room::get_vr_anchor(Optional<Transform> const & _forPlacement, Optional<Transform> const & _forVRPlacement) const
{
	// check both vr anchors from POIs and from doors
	// get the closest one
	// sometimes we may have just a single vr anchor POI but the generator has taken care of vr anchors for doors

	Optional<Transform> vrAnchorPlacement;
	float bestDist = 0.0f;

	{
		for_every_point_of_interest(NAME(vrAnchor), [&vrAnchorPlacement, &bestDist, _forPlacement, _forVRPlacement](PointOfInterestInstance* _fpoi)
		{
			for_count(int, do180, 2)
			{
				Transform poiPlacement = _fpoi->calculate_placement();
				if (do180)
				{
					poiPlacement = poiPlacement.to_world(Transform::do180);
				}
				float dist = 0.0f;
				if (_forPlacement.is_set())
				{
					if (_forVRPlacement.is_set())
					{
						// this will give better results
						dist = (poiPlacement.location_to_world(_forVRPlacement.get().get_translation()) - _forPlacement.get().get_translation()).length();
					}
					else
					{
						dist = (poiPlacement.get_translation() - _forPlacement.get().get_translation()).length();
					}
				}
				if (bestDist > dist || !vrAnchorPlacement.is_set())
				{
					bestDist = dist;
					vrAnchorPlacement = poiPlacement;
				}
			}
		});
	}

	{
		DoorInRoom const* bestDir = nullptr;
		for_every_ptr(dir, get_all_doors())
		{
			// invisible doors are fine here
			if (!dir->can_move_through() ||
				!dir->is_visible() ||
				!dir->is_relevant_for_movement() ||
				!dir->is_relevant_for_vr()) continue;
			if (dir->is_vr_space_placement_set() && dir->is_placed())
			{
				float dist = 0.0f;
				if (_forPlacement.is_set())
				{
					dist = (dir->get_placement().get_translation() - _forPlacement.get().get_translation()).length();
				}
				if ((!bestDir && !vrAnchorPlacement.is_set()) || bestDist > dist)
				{
					vrAnchorPlacement.clear();
					bestDir = dir;
					bestDist = dist;
				}
			}
		}

		if (bestDir)
		{
			Transform doorVRPlacement = bestDir->get_vr_space_placement();
			Transform roomAnchorRelativeToDirVR = doorVRPlacement.to_local(Transform::identity);
			Transform roomVRAnchor = bestDir->get_placement().to_world(roomAnchorRelativeToDirVR);
			vrAnchorPlacement = roomVRAnchor;
		}
	}

	if (vrAnchorPlacement.is_set())
	{
		return vrAnchorPlacement.get();
	}
	else
	{
		return Transform::identity;
	}
}

Nav::Mesh* Room::access_nav_mesh(Name const & _navMeshId)
{
	{
		Concurrency::ScopedMRSWLockRead lock(navMeshesLock);

		for_every_ref(checkNavMesh, navMeshes)
		{
			if (checkNavMesh->get_id() == _navMeshId)
			{
				return checkNavMesh;
			}
		}
	}

	{
		Concurrency::ScopedMRSWLockWrite lock(navMeshesLock);

		Nav::MeshPtr navMesh(new Nav::Mesh(this, _navMeshId));
		navMeshes.push_back(navMesh);

		return navMesh.get();
	}
}

void Room::use_nav_mesh(Nav::Mesh* _navMesh)
{
	scoped_call_stack_info(TXT("Room::use_nav_mesh"));

	Concurrency::ScopedMRSWLockWrite lock(navMeshesLock);

	for_every(checkNavMesh, navMeshes)
	{
		if ((*checkNavMesh)->get_id() == _navMesh->get_id())
		{
			// replace
			Game::get()->get_nav_system()->add(Nav::Tasks::DestroyNavMesh::new_task(checkNavMesh->get()));
			(*checkNavMesh) = _navMesh;
			return;
		}
	}

	navMeshes.push_back(Nav::MeshPtr(_navMesh));
}

Mesh* Room::find_used_mesh_for(Meshes::IMesh3D const * _mesh) const
{
	for_every_ref(usedMesh, usedMeshes)
	{
		if (usedMesh->get_mesh() == _mesh)
		{
			return usedMesh;
		}
	}
	return nullptr;
}

Nav::PlacementAtNode Room::find_nav_location(Transform const & _placement, OUT_ float * _dist, Name const & _navMeshId, Optional<int> const & _navMeshKeepGroup) const
{
	Nav::PlacementAtNode result;

	{
		Concurrency::ScopedMRSWLockRead lock(navMeshesLock);

		for_every_ref(navMesh, navMeshes)
		{
			if (navMesh->get_id() == _navMeshId)
			{
				result = navMesh->find_location(_placement, _dist, _navMeshKeepGroup);
			}
		}
	}

	return result;
}

void Room::set_door_type_replacer(DoorTypeReplacer* _dtr)
{
	if (doorTypeReplacer.get() == _dtr)
	{
		return;
	}
	doorTypeReplacer = _dtr;
	for_every_ptr(door, allDoors)
	{
		// if it won't change, we do not have to do anything (if something has changed, will perform that automaticaly)
		door->get_door()->update_type();
	}
}

void Room::set_reverb(Reverb* _reverb)
{
	useReverb = _reverb;
}

void Room::advance__pois(IMMEDIATE_JOB_PARAMS)
{
	Framework::AdvanceContext const * ac = plain_cast<Framework::AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(Room, room)
	{
		MEASURE_PERFORMANCE(advancePOIs_room);

		room->poisRequireAdvancement = false;
		for_every_ref(poi, room->pois)
		{
			if (!poi->is_cancelled() &&
				poi->does_require_advancement())
			{
				poi->advance(*ac);
				room->poisRequireAdvancement |= poi->does_require_advancement();
			}
		}
	}
}

void Room::mark_to_activate(TemporaryObject* _temporaryObject)
{
	Concurrency::ScopedSpinLock lock(worldActivationLock);
	if (is_world_active())
	{
		_temporaryObject->mark_to_activate();
	}
	else
	{
		temporaryObjectsToActivate.push_back(_temporaryObject);
	}
}

void Room::set_origin_room(Room* _room)
{
	if (originRoom != _room)
	{
		originRoom = _room;
		an_assert(!_room || _room->get_in_region() == get_in_region(), TXT("origin room should be in the same region as we are"));

		// if we change origin room, udpate door type too (if door has two connections
		for_every_ptr(door, allDoors)
		{
			if (auto* d = door->get_door())
			{
				if (d->is_linked_on_both_sides())
				{
					d->update_type();
				}
			}
		}
#ifdef AN_DEVELOPMENT
		for_every_ptr(door, allDoors)
		{
			if (auto* d = door->get_door())
			{
				d->dev_check_if_ok();
			}
		}
#endif
	}
}

void Room::collect_variables(REF_ SimpleVariableStorage & _variables) const
{
	// first with types, set only missing!
	// then set forced from actual individual variables that should override_ everything else

	// this means that most important variables are (from most important to least)
	//	origin room (before this room type!)
	//	this room
	//	overriding-regions ((this)type->region  -->  parent(type->region) --> and so on up to the most important)
	//	in region
	//	region's parent, grand parent
	//	... (so on) ...
	//	origin room type (before this room type!)
	//	this room type
	//	in region type
	//	...
	//	top region type


	if (auto* oRoom = originRoom.get())
	{
		an_assert(oRoom->get_in_region() == get_in_region());
		if (auto* rt = oRoom->get_room_type())
		{
			_variables.set_missing_from(rt->get_custom_parameters());
		}
	}
	if (roomType)
	{
		_variables.set_missing_from(roomType->get_custom_parameters());
	}
	int inRegionsCount = 0;
	if (Region const * startRegion = get_in_region())
	{
		Region const * region = startRegion;
		while (region)
		{
			if (auto * rt = region->get_region_type())
			{
				_variables.set_missing_from(rt->get_custom_parameters());
			}
			region = region->get_in_region();
			++inRegionsCount;
		}
	}

	if (inRegionsCount > 0)
	{
		ARRAY_STACK(Region const *, inRegions, inRegionsCount);
		if (Region const * startRegion = get_in_region())
		{
			Region const * region = startRegion;
			while (region)
			{
				inRegions.push_back(region);
				region = region->get_in_region();
			}
		}
		for_every_reverse_ptr(region, inRegions)
		{
			_variables.set_from(region->get_variables());
		}
		// apply overriding regions
		{
			for_every_reverse_ptr(region, inRegions)
			{
				if (region->is_overriding_region())
				{
					if (auto* rt = region->get_region_type())
					{
						_variables.set_from(rt->get_custom_parameters());
					}
					_variables.set_from(region->get_variables());
				}
			}
		}
	}
	_variables.set_from(get_variables());
	if (auto* oRoom = originRoom.get())
	{
		_variables.set_from(oRoom->get_variables());
	}
}

bool Room::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	TypeId id = _value.get_type();
	SimpleVariableInfo const & info = variables.find(_byName, id);
	an_assert(RegisteredType::is_plain_data(id));
	memory_copy(info.access_raw(), _value.get_raw(), RegisteredType::get_size_of(id));
	return true;
}

bool Room::restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const
{
	return restore_value_for_wheres_my_point(_byName, OUT_ _value, true);
}

bool Room::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return variables.convert_existing(_byName, _to);
}

bool Room::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return variables.rename_existing(_from, _to);
}

bool Room::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value, bool _checkOriginRoom) const
{
	TypeId id;
	void const * rawData;
	if (_checkOriginRoom)
	{
		if (auto* oRoom = originRoom.get())
		{
			if (oRoom->get_variables().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
		}
	}
	if (get_variables().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
	{
		_value.set_raw(id, rawData);
		return true;
	}
	{
		rawData = nullptr;
		Region const* region = inRegion;
		while (region)
		{
			if (region->is_overriding_region())
			{
				if (region->get_variables().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
				{
				}
				else if (auto* rt = region->get_region_type())
				{
					rt->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData);
				}
			}
			region = region->get_in_region();
		}
		if (rawData)
		{
			_value.set_raw(id, rawData);
			return true;
		}
	}
	{
		Region const * region = inRegion;
		while (region)
		{
			if (region->get_variables().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
			region = region->get_in_region();
		}
	}
	if (_checkOriginRoom)
	{
		if (auto* oRoom = originRoom.get())
		{
			if (oRoom->get_room_type() && oRoom->get_room_type()->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
		}
	}
	if (get_room_type() && get_room_type()->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
	{
		_value.set_raw(id, rawData);
		return true;
	}
	{
		Region const * region = inRegion;
		while (region)
		{
			if (region->get_region_type() && region->get_region_type()->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
			region = region->get_in_region();
		}
	}
	return false;
}

bool Room::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return store_value_for_wheres_my_point(_byName, _value);
}

bool Room::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool Room::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	an_assert(false, TXT("no use for this"));
	return false;
}

WheresMyPoint::IOwner* Room::get_wmp_owner()
{
	return inRegion;
}

bool Room::run_wheres_my_point_processor_setup_parameters()
{
	if (get_room_type())
	{
		WheresMyPoint::Context context(this);
		context.set_random_generator(get_individual_random_generator());
		bool result = get_room_type()->get_wheres_my_point_processor_setup_parameters().update(context);
		if (!result)
		{
			error(TXT("failed run_wheres_my_point_processor_setup_parameters"));
		}
		return result;
	}
	else
	{
		// no errors
		return true;
	}
}

bool Room::run_wheres_my_point_processor_pre_generate()
{
	if (get_room_type())
	{
		WheresMyPoint::Context context(this);
		context.set_random_generator(get_individual_random_generator());
		bool result = get_room_type()->get_wheres_my_point_processor_pre_generate().update(context);
		if (!result)
		{
			error(TXT("failed run_wheres_my_point_processor_pre_generate"));
		}
		return result;
	}
	else
	{
		// no errors
		return true;
	}
}

bool Room::run_wheres_my_point_processor_on_generate()
{
	if (get_room_type())
	{
		WheresMyPoint::Context context(this);
		context.set_random_generator(get_individual_random_generator());
		bool result = get_room_type()->get_wheres_my_point_processor_on_generate().update(context);
		if (!result)
		{
			error(TXT("failed run_wheres_my_point_processor_on_generate"));
		}
		return result;
	}
	else
	{
		// no errors
		return true;
	}
}

void Room::log(LogInfoContext & _log, Room* const & _room)
{
	if (_room)
	{
		_log.log(_room->get_name().to_char());
	}
	else
	{
		_log.log(TXT("--"));
	}
}

void Room::log(LogInfoContext & _log, ::SafePtr<Room> const & _room)
{
	if (_room.get())
	{
		_log.log(_room->get_name().to_char());
	}
	else
	{
		_log.log(TXT("--"));
	}
}

bool Room::set_door_vr_placement_if_not_set(DoorInRoom* _dir, ShouldAllowToFail _allowToFailSilently)
{
	place_pending_doors_on_pois(); // make sure all doors are placed

	DoorInRoom* dirVR = _dir && _dir->is_vr_space_placement_set() ? _dir : nullptr;
	bool requiresUpdatingVRForDIRs = false;
	for_every_ptr(dir, get_all_doors())
	{
		// invisible doors are fine here
		if (dir->is_vr_space_placement_set())
		{
			if (!dirVR)
			{
				dirVR = dir;
			}
		}
		else if (! dir->may_ignore_vr_placement())
		{
			requiresUpdatingVRForDIRs = true;
		}
	}

	if (requiresUpdatingVRForDIRs)
	{
		if (!dirVR)
		{
			if (_allowToFailSilently == DisallowToFail)
			{
				an_assert(false, TXT("at least one door has to have VR placement set"));
				todo_important(TXT("calculate placement?"));
			}
		}
		else
		{
			Transform roomAnchorRelativeToDirVR = dirVR->get_placement().to_local(Transform::identity);
			Transform roomVRAnchor = dirVR->get_vr_space_placement().to_world(roomAnchorRelativeToDirVR);

			for_every_ptr(dir, get_all_doors())
			{
				// invisible doors are fine here
				if (dir != dirVR)
				{
					if (!dir->is_vr_space_placement_set())
					{
						dir->set_vr_space_placement(roomVRAnchor.to_world(dir->get_placement()));
					}
				}
			}
		}
	}

	return requiresUpdatingVRForDIRs;
}

bool Room::set_door_vr_placement_for(DoorInRoom* _dir, ShouldAllowToFail _allowToFailSilently)
{
	DoorInRoom* dirVR = nullptr;
	float closest = 0.0f;
	Vector3 _dirHoleCentreWS = _dir->get_hole_centre_WS();
	for_every_ptr(dir, get_all_doors())
	{
		// invisible doors are fine here
		if (dir != _dir && dir->is_vr_space_placement_set() && dir->is_placed())
		{
			float dist = (_dirHoleCentreWS - dir->get_hole_centre_WS()).length_squared();
			if (!dirVR || dist < closest)
			{
				dirVR = dir;
				closest = dist;
			}
		}
	}

	if (dirVR)
	{
		Transform roomAnchorRelativeToDirVR = dirVR->get_placement().to_local(Transform::identity);
		Transform roomVRAnchor = dirVR->get_vr_space_placement().to_world(roomAnchorRelativeToDirVR);

		if (!_dir->is_vr_space_placement_set())
		{
			_dir->set_vr_space_placement(roomVRAnchor.to_world(_dir->get_placement()));
		}
		return true;
	}
	else if (_allowToFailSilently == DisallowToFail)
	{
		for_every_ptr(dir, get_all_doors())
		{
			// invisible doors are fine here
			output(TXT("door %i%S %S; %S; %S;"), for_everys_index(dir),
				dir == _dir ? TXT("<-") : TXT(""),
				dir->has_room_on_other_side() ? TXT("has room on the other side") : TXT("no room on the other side"),
				dir->is_vr_space_placement_set() ? TXT("vr-place") : TXT("no vr-place"),
				dir->is_placed() ? TXT("placed") : TXT("not placed")
			);
		}
		error(TXT("no other door to base vr placement on, room %p %S door %S"), this,
			get_room_type()? get_room_type()->get_name().to_string().to_char() : TXT("--"),
			_dir->get_door() && _dir->get_door()->get_door_type() ? _dir->get_door()->get_door_type()->get_name().to_string().to_char() : TXT("--"));
		return false;
	}
	else
	{
		return false;
	}
}

int Room::count_presence_links() const
{
	int count = 0;
	PresenceLink * pl = presenceLinks;
	while (pl)
	{
		++count;
		pl = pl->nextInRoom;
	}
	return count;
}

void Room::mark_objects_no_longer_advanceable_due_to_room_destruction()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Room [mark objects no longer advanceable] r%p"), this);
#endif
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	
	objectsNoLongerAdvanceable = true;

	{
		Concurrency::ScopedMRSWLockRead lock(objectsLock);
		for_every_ptr(object, allObjects)
		{
			if (auto* p = object->get_presence())
			{
				p->invalidate_presence_links();
			}
			object->mark_no_longer_advanceable(IModulesOwner::NoLongerAdvanceableFlag::RoomBeingDestroyed);
		}
	}
	{
		Concurrency::ScopedMRSWLockRead lock(temporaryObjectsLock);
		for_every_ptr(to, allTemporaryObjects)
		{
			if (auto* p = to->get_presence())
			{
				p->invalidate_presence_links();
			}
			to->mark_no_longer_advanceable(IModulesOwner::NoLongerAdvanceableFlag::RoomBeingDestroyed);
		}
	}
}

bool Room::is_in_region(Region* _region) const
{
	auto* reg = get_in_region();
	while (reg)
	{
		if (reg == _region)
		{
			return true;
		}
		reg = reg->get_in_region();
	}
	return false;
}

//

FoundRoom::FoundRoom(Room* _room, FoundRoom* _enteredFrom, DoorInRoom* _enteredThroughDoor, float _distance, float _distanceNotVisible, int _depth, bool _visible)
: room(_room)
, enteredFrom(_enteredFrom)
, enteredThroughDoor(_enteredThroughDoor)
, originPlacement(_enteredThroughDoor->get_other_room_transform().to_world(_enteredFrom->originPlacement))
, entrancePoint(_enteredThroughDoor->get_placement().get_translation())
, distance(_distance)
, distanceNotVisible(_distanceNotVisible)
, depth(_depth)
, scanned(false)
, visible(_visible)
{
	todo_note(TXT("do we want to have doors that scale things up/down?"));
	originPlacement.set_scale(1.0f);
}

FoundRoom::FoundRoom(Room* _room, Vector3 const & _location, bool _visible)
: room(_room)
, enteredFrom(nullptr)
, enteredThroughDoor(nullptr)
, originPlacement(_location, Quat::identity)
, entrancePoint(_location)
, distance(0.0f)
, distanceNotVisible(0.0f)
, depth(0)
, scanned(false)
, visible(_visible)
{
}

FoundRoom::FoundRoom(Room* _room, Transform const & _placement, bool _visible)
: room(_room)
, enteredFrom(nullptr)
, enteredThroughDoor(nullptr)
, originPlacement(_placement)
, entrancePoint(_placement.get_translation())
, distance(0.0f)
, distanceNotVisible(0.0f)
, depth(0)
, scanned(false)
, visible(_visible)
{
}

void FoundRoom::find(REF_ ArrayStack<FoundRoom>& _rooms, Room* _startRoom, Vector3 const& _startLoc, FindRoomContext& _context)
{
	find(_rooms, _startRoom, Transform(_startLoc, Quat::identity), _context);
}

void FoundRoom::find(REF_ ArrayStack<FoundRoom> & _rooms, Room* _startRoom, Transform const & _startPlacement, FindRoomContext& _context)
{
	MEASURE_PERFORMANCE_COLOURED(findRoom_find, Colour::zxCyan);
	_rooms.push_back(FoundRoom(_startRoom, _startPlacement, true));
	int firstRoomToScan = 0;
	while (true)
	{
		// find room to scan closest to us, remember last first room scanned as more added rooms will be after this one
		FoundRoom* scanRoom = nullptr;
		for (FoundRoom* room = &_rooms[firstRoomToScan]; room != _rooms.end(); ++room)
		{
			if (room->scanned)
			{
				if (!scanRoom)
				{
					++firstRoomToScan;
				}
			}
			else
			{
				if (!scanRoom || scanRoom->distance > room->distance)
				{
					scanRoom = room;
				}
			}
		}

		if (scanRoom)
		{
			scan_room(*scanRoom, REF_ _rooms, _context);
			continue;
		}
		else
		{
			// everything done
			break;
		}
	}
}

//#define DEBUG_SCAN_ROOM

void FoundRoom::scan_room(FoundRoom& _room, REF_ ArrayStack<FoundRoom> & _roomsToScan, FindRoomContext & _context)
{
	MEASURE_PERFORMANCE(findRoom_scanRoom);

	_room.scanned = true;

	if (_context.depthLimit.is_set() &&
		_room.depth >= _context.depthLimit.get())
	{
		// no need to look for further
		return;
	}

#ifdef DEBUG_SCAN_ROOM
	debug_context(_room.room);
#endif

	// get more rooms!
	if (_roomsToScan.has_place_left())
	{
		for_every_ptr(door, _room.room->get_doors())
		{
			if (door != _room.enteredThroughDoor &&
				door->has_world_active_room_on_other_side() &&
				door->is_visible())
			{
#ifdef DEBUG_SCAN_ROOM
				door->debug_draw_hole(true, Colour::orange);
#endif
				float increaseDistanceBy = (door->get_placement().get_translation() - _room.entrancePoint).length();
				Room* nextRoom = door->get_room_on_other_side();
				if (!nextRoom)
				{
					continue;
				}
				int alreadyExisting = 0;
				for_every(room, _roomsToScan)
				{
					if (room->room == nextRoom)
					{
						++alreadyExisting;
					}
				}
				if (alreadyExisting)
				{
					continue;
				}
				float actualIncreaseDistanceBy = increaseDistanceBy * (1.0f + (float)alreadyExisting * 1.5f);
				float distance = _room.distance + actualIncreaseDistanceBy;
				if (_context.maxDistanceNotVisible.is_set() && ! _room.visible &&
					_room.distanceNotVisible >= _context.maxDistanceNotVisible.get())
				{
					continue;
				}
				if (_context.maxDistance.is_set() && (! _context.maxDistanceNotVisible.is_set() || _room.visible) &&
					_room.distance >= _context.maxDistance.get())
				{
					continue;
				}
				bool visible = _room.visible;
				::System::ViewFrustum clippedDoorVF;
				if (_context.onlyVisible.get(false) || (_context.maxDistanceNotVisible.is_set() && visible))
				{
					if (door->get_plane().get_in_front(_room.originPlacement.get_translation()) < 0.0f)
					{
						visible = false;
					}
					else
					{
						if (auto* d = door->get_door())
						{
							if (!((d->get_open_factor() != 0.0f || d->can_see_through_when_closed()) && !d->get_hole_scale().is_zero()))
							{
								visible = false;
							}
						}
					}
					if (visible)
					{
						if (auto* doorHole = door->get_hole())
						{
							::System::ViewFrustum doorVF;
							doorHole->setup_frustum_view(door->get_side(), door->get_hole_scale(), doorVF, _room.originPlacement.inverted().to_matrix(), door->get_outbound_matrix());
							if (! clippedDoorVF.clip(&doorVF, &_room.viewFrustum))
							{
								visible = false;
							}
						}
					}
					if (_context.onlyVisible.get(false) && !visible)
					{
						continue;
					}
				}
				float distanceNotVisible = _room.distanceNotVisible + (visible ? 0.0f : actualIncreaseDistanceBy);
#ifdef DEBUG_SCAN_ROOM
				door->debug_draw_hole(true, Colour::green);
#endif


				//

				int insertAt = (int)(&_room + 1 - _roomsToScan.begin());
				for (FoundRoom const * room = &_room + 1; room != _roomsToScan.end(); ++room)
				{
					if (room->distance > distance)
					{
						break;
					}
					++insertAt;
				}
				// we store pointer to FoundRoom but as we do it in order, we should never move those rooms that we already point at
				_roomsToScan.insert_at(insertAt, FoundRoom(nextRoom, &_room, door->get_door_on_other_side(), distance, distanceNotVisible, _room.depth + 1, visible));
				if (_context.onlyVisible.get(false))
				{
					_roomsToScan[insertAt].viewFrustum = clippedDoorVF;
				}
				if (!_roomsToScan.has_place_left())
				{
					// no more space left!
					break;
				}
			}
		}
	}

#ifdef DEBUG_SCAN_ROOM
	debug_no_context();
#endif
}

bool Room::check_temporary_object_spawn_block(TemporaryObjectType const* _type, Vector3 const& _at, float _dist, Optional<float> const& _time, Optional<int> const& _count)
{
	Concurrency::ScopedSpinLock lock(temporaryObjectSpawnBlockingLock);

	for_every(tosb, temporaryObjectSpawnBlocking)
	{
		if (tosb->temporaryObjectType == _type)
		{
			float distSq = (_at - tosb->at).length_squared();
			if (distSq <= sqr(tosb->dist))
			{
				if (tosb->countLeft.is_set())
				{
					tosb->countLeft = tosb->countLeft.get() - 1;
					if (tosb->countLeft.get() <= 0)
					{
						temporaryObjectSpawnBlocking.remove_fast_at(for_everys_index(tosb));
					}
					return false;
				}
				return false;
			}
		}
	}

	TemporaryObjectSpawnBlocking tosb;
	tosb.temporaryObjectType = _type;
	tosb.at = _at;
	tosb.dist = _dist;
	tosb.timeLeft = _time;
	tosb.countLeft = _count;
	temporaryObjectSpawnBlocking.push_back(tosb);

	return true;
}

bool Room::is_close_to_collidable(Vector3 const& _loc, Collision::Flags const& _withFlags, float _maxDist) const
{
	PresenceLink const * pl = presenceLinks;
	while (pl)
	{
		float distSq = (pl->placementInRoom.get_translation() - _loc).length_squared();
		if (distSq < sqr(_maxDist + pl->presenceRadius))
		{
			if (auto* imo = pl->get_modules_owner())
			{
				if (auto* c = imo->get_collision())
				{
					if (Collision::Flags::check(_withFlags, c->get_collision_flags()))
					{
						return true;
					}
				}
			}
		}
		pl = pl->get_next_in_room();
	}

	return false;
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
void Room::mark_recently_seen_by_player(Transform const& placement)
{
	timeSinceSeenByPlayer = 0.0f;
	recentlySeenByPlayer = placement;
	recentlySeenByPlayerCandidate = placement;
	recentlySeenByPlayerCandidateDepth = 0;
}
#endif

void Room::update_player_related(Framework::IModulesOwner* _player, bool _visited, bool _seen)
{
	MEASURE_PERFORMANCE_COLOURED(updateSeenByPlayer, Colour::purple);
	if (_player)
	{
		if (auto* p = _player->get_presence())
		{
			if (Room* r = p->get_in_room())
			{
				if (_visited)
				{
					r->timeSincePlayerVisited = 0.0f;
				}
				if (_seen)
				{
					Transform playerTransform = p->get_centre_of_presence_transform_WS();
					ARRAY_STACK(Framework::FoundRoom, rooms, 64);
					Framework::FoundRoom::find(rooms, r, playerTransform, Framework::FindRoomContext().only_visible());
					for_every(fr, rooms)
					{
						Concurrency::ScopedSpinLock lock(fr->room->seenByPlayerLock);
						fr->room->timeSinceSeenByPlayer = 0.0f;
						if (!fr->room->recentlySeenByPlayerCandidate.is_set() ||
							fr->room->recentlySeenByPlayerCandidateDepth > fr->depth)
						{
							fr->room->recentlySeenByPlayerCandidate = fr->originPlacement;
						}
					}
				}
			}
		}
	}
}

/*
void Room::update_seen_by_player(Room* _room)
{
	MEASURE_PERFORMANCE_COLOURED(updateSeenByPlayer_room, Colour::purple);
	Concurrency::ScopedSpinLock lock(_room->seenByPlayerLock);
	_room->timeSinceSeenByPlayer = 0.0f;
	if (!_room->recentlySeenByPlayerCandidate.is_set())
	{
		_room->recentlySeenByPlayerCandidate = Transform::identity;
		_room->recentlySeenByPlayerCandidateDepth = 1000;
	}
}
*/

void Room::advance__player_related(Array<Room*> const& _onRooms, float _deltaTime)
{
	MEASURE_PERFORMANCE_COLOURED(advanceSeenByPlayer, Colour::purple);

	int advancementSuspensionRoomDistance = GameConfig::get() ? GameConfig::get()->get_advancement_suspension_room_distance() : 3;

	if (advancementSuspensionRoomDistance <= 0)
	{
		return;
	}

	int maxDist = _onRooms.get_size();

	ARRAY_STACK(Room*, rooms0, _onRooms.get_size());
	ARRAY_STACK(Room*, rooms1, _onRooms.get_size());
	ARRAY_STACK(Room*, rooms2, _onRooms.get_size());

	{
		MEASURE_PERFORMANCE_COLOURED(advanceSeenByPlayer_init, Colour::green);

		float seenLimit = max(c_timeSinceSeenByPlayerLimit + 10.0f, 1000.0f);
		float visitedLimit = 100000.0f;
		for_every_ptr(room, _onRooms)
		{
			room->timeSinceSeenByPlayer = min(room->timeSinceSeenByPlayer + clamp(::System::Core::get_raw_delta_time(), 0.0f, 0.05f), seenLimit);
			room->timeSincePlayerVisited = min(room->timeSincePlayerVisited + _deltaTime, visitedLimit);

			room->prevDistanceToRecentlySeenByPlayer = room->distanceToRecentlySeenByPlayer;
			bool recentlySeenByPlayer = AVOID_CALLING_ACK_ room->was_recently_seen_by_player();
			if (recentlySeenByPlayer)
			{
				room->distanceToRecentlySeenByPlayer = 0;
				rooms0.push_back(room);
				room->recentlySeenByPlayer = room->recentlySeenByPlayerCandidate;
			}
			else
			{
				room->distanceToRecentlySeenByPlayer = maxDist + 1000;
			}
			room->recentlySeenByPlayerCandidate.clear();
			room->recentlySeenByPlayerCandidateDepth = NONE;
			room->advancementSuspensionRoomDistance = advancementSuspensionRoomDistance;
		}
	}

	{
		MEASURE_PERFORMANCE_COLOURED(advanceSeenByPlayer_propagate, Colour::red);

		ArrayStack<Room*>* roomsCurr = &rooms0;
		ArrayStack<Room*>* roomsNext = &rooms1;
		ArrayStack<Room*>* roomsNextIfSmall = &rooms2;
		bool keepDoing = true;
		// we're propagating 1 by 1, atDist=0, then atDist=1, it is not possible to find a shorter way
		// but because we might be going with small rooms, we may get stuck at a dist for a while
		// we add rooms at a certain distance and only process the rooms we modify in next step, this should speed up the whole process
		// we keep collecting rooms at certain dist
		int atDist = 0;
		while (keepDoing)
		{
			MEASURE_PERFORMANCE_COLOURED(advanceSeenByPlayer_propagateIteration, Colour::orange);
			keepDoing = false;
			for_every_ptr(room, (*roomsCurr))
			{
				// it is possible that in certain situations we found something earlier but we still had the room added to roomsNext,
				// better to skip here rather than to expensive checks on the go
				if (room->distanceToRecentlySeenByPlayer == atDist)
				{
					for_every_ptr(door, room->doors)
					{
						if (auto* r = door->get_world_active_room_on_other_side())
						{
							bool smallRoom = r->is_small_room();
							int inc = (smallRoom ? 0 : 1);
							int oRoomDist = atDist + inc;
							if (r->distanceToRecentlySeenByPlayer > oRoomDist)
							{
								r->distanceToRecentlySeenByPlayer = oRoomDist;
								auto* addToRooms = (smallRoom ? roomsNextIfSmall : roomsNext);
								an_assert(!addToRooms->does_contain(r), TXT("the distance should be already set properly, so this shouldn't happen"));
								addToRooms->push_back(r);
							}
						}
					}
				}
			}
			if (! roomsNextIfSmall->is_empty())
			{
				swap(roomsCurr, roomsNextIfSmall);
				roomsCurr->clear();
				// maybe we will find more "if small" (maybe some from normal will come here)
				keepDoing = true;
			}
			else
			{
				swap(roomsCurr, roomsNext);
				roomsNext->clear();
				++atDist;
				// no small were found, up the level
				keepDoing = !roomsCurr->is_empty();
			}
		}
	}
}

void Room::advance__suspending_advancement(IMMEDIATE_JOB_PARAMS)
{
#ifdef AN_ALLOW_BULLSHOTS
	if (BullshotSystem::is_active())
	{
		return; // do not suspend advancement
	}
#endif
	MEASURE_PERFORMANCE_COLOURED(advanceSuspendingAdvancement_updateObjects, Colour::blue);
	FOR_EVERY_IMMEDIATE_JOB_DATA(Room, room)
	{
		if (room->alwaysSuspendAdvancement ||
			room->distanceToRecentlySeenByPlayer >= room->advancementSuspensionRoomDistance)
		{
			MEASURE_PERFORMANCE_COLOURED(advanceSuspendingAdvancement_suspend, Colour::greyLight);
			{
				Concurrency::ScopedMRSWLockRead lock(room->objectsLock); 
				for_every_ptr(obj, room->objects)
				{
					obj->suspend_advancement();
					obj->update_room_distance_to_recently_seen_by_player();
				}
			}
			{
				Concurrency::ScopedMRSWLockRead lock(room->temporaryObjectsLock);
				for_every_ptr(obj, room->temporaryObjects)
				{
					obj->update_room_distance_to_recently_seen_by_player();
				}
			}
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
		else if (room->distanceToRecentlySeenByPlayer == 0)
		{
			MEASURE_PERFORMANCE_COLOURED(advanceSuspendingAdvancement_resume, Colour::c64Yellow);
			{
				Concurrency::ScopedMRSWLockRead lock(room->objectsLock); 
				for_every_ptr(obj, room->objects)
				{
					obj->resume_advancement();
					obj->update_room_distance_to_recently_seen_by_player();
				}
			}
			{
				Concurrency::ScopedMRSWLockRead lock(room->temporaryObjectsLock);
				for_every_ptr(obj, room->temporaryObjects)
				{
					obj->update_room_distance_to_recently_seen_by_player();
				}
			}
		}
#endif
		else
		{
			MEASURE_PERFORMANCE_COLOURED(advanceSuspendingAdvancement_resume, Colour::c64LightBlue);
			{
				Concurrency::ScopedMRSWLockRead lock(room->objectsLock); 
				for_every_ptr(obj, room->objects)
				{
					obj->resume_advancement();
					obj->update_room_distance_to_recently_seen_by_player();
				}
			}
			{
				Concurrency::ScopedMRSWLockRead lock(room->temporaryObjectsLock);
				for_every_ptr(obj, room->temporaryObjects)
				{
					obj->update_room_distance_to_recently_seen_by_player();
				}
			}
		}
	}
}

WorldAddress Room::get_world_address() const
{
	WorldAddress wa;
	wa.build_for(this);
	return wa;
}
	
void Room::update_space_blockers()
{
	Concurrency::ScopedMRSWLockWrite lock(spaceBlockersLock);
	
	spaceBlockers.clear();

	{
		Concurrency::ScopedMRSWLockRead lockasb(appearanceSpaceBlockersLock);
		spaceBlockers.add(appearanceSpaceBlockers);
#ifdef AN_DEVELOPMENT
		for_every(sb, spaceBlockers.blockers)
		{
			sb->debugInfo = TXT("room appearance space blockers");
		}
#endif
	}

	// use "all" to be aware of objects that are already placed but not activated

	FOR_EVERY_ALL_DOOR_IN_ROOM(dir, this)
	{
#ifdef AN_DEVELOPMENT
		int soFar = spaceBlockers.blockers.get_size();
#endif
		dir->create_space_blocker(REF_ spaceBlockers);
#ifdef AN_DEVELOPMENT
		for_range(int, i, soFar, spaceBlockers.blockers.get_size() - 1)
		{
			spaceBlockers.blockers[i].debugInfo = String::printf(TXT("door in room %i to \"%S\" "), for_everys_index(dir),
				dir->get_room_on_other_side()? dir->get_room_on_other_side()->get_name().to_char() : TXT("<nowhere>")) + spaceBlockers.blockers[i].debugInfo;
		}
#endif
	}

	FOR_EVERY_ALL_OBJECT_IN_ROOM(object, this)
	{
		if (auto* sbo = fast_cast<SpaceBlockingObject>(object))
		{
#ifdef AN_DEVELOPMENT
			int soFar = spaceBlockers.blockers.get_size();
#endif
			spaceBlockers.add(sbo->get_space_blocker_local(), sbo->get_presence()->get_placement());
#ifdef AN_DEVELOPMENT
			for_range(int, i, soFar, spaceBlockers.blockers.get_size() - 1)
			{
				spaceBlockers.blockers[i].debugInfo = String::printf(TXT("object in room \"%S\""), sbo->get_name().to_char());
			}
#endif
		}
	}
}

bool Room::check_space_blockers(SpaceBlockers const& _spaceBlockers, Transform const& _placement)
{
	update_space_blockers();

	Concurrency::ScopedMRSWLockRead lock(spaceBlockersLock);

	return spaceBlockers.check(_spaceBlockers, _placement);
}

DoorTypeReplacer* Room::get_door_type_replacer() const
{
	if (auto* dtr = doorTypeReplacer.get())
	{
		return dtr;
	}
	if (auto* oRoom = originRoom.get())
	{
		return oRoom->get_door_type_replacer();
	}
	return nullptr;
}

Optional<float> Room::find_vr_elevator_altitude(Room* _awayFromRoom, Door* _throughDoor, OPTIONAL_ OUT_ int * _depth) const
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	/*
	 *	DEPTH
	 *		0	we came through that door
	 *		1	room in which we started
	 *		2+	following rooms
	 */

	struct RoomToCheck
	{
		Room const* room;
		int depth;
		RoomToCheck() {}
		RoomToCheck(Room const* _room, int _depth = NONE) : room(_room), depth(_depth) {}

		// for does_contain
		bool operator == (RoomToCheck const& _other) const { return room == _other.room; }
	};

	Array<Room const *> checkedRooms;
	Array<RoomToCheck> roomsToCheck;

	if (_awayFromRoom)
	{
		checkedRooms.push_back(_awayFromRoom);
	}

	// get door leading to _awayFromRoom, first
	for_every_ptr(door, get_all_doors())
	{
		// invisible doors are fine here
		if (!door->can_move_through() ||
			!door->is_relevant_for_movement() ||
			!door->is_relevant_for_vr()) continue;
		if (door->get_room_on_other_side() == _awayFromRoom &&
			(!_throughDoor || door->get_door() ==  _throughDoor))
		{
			if (door->get_vr_elevator_altitude().is_set())
			{
				assign_optional_out_param(_depth, 0);
				return door->get_vr_elevator_altitude();
			}
			if (auto* d = door->get_door())
			{
				if (d->get_vr_elevator_altitude().is_set())
				{
					assign_optional_out_param(_depth, 0);
					return d->get_vr_elevator_altitude();
				}
			}
		}
	}

	if (vrElevatorAltitude.is_set())
	{
		assign_optional_out_param(_depth, 1);
		return vrElevatorAltitude;
	}

	// broad search
	roomsToCheck.push_back(RoomToCheck(this, 2));
	while (!roomsToCheck.is_empty())
	{
		auto rc = roomsToCheck.get_first();
		auto* r = rc.room;
		roomsToCheck.pop_front();
		checkedRooms.push_back(r);
		// we should take the value through door we came earlier
		if (r->get_vr_elevator_altitude().is_set())
		{
			assign_optional_out_param(_depth, rc.depth);
			return r->get_vr_elevator_altitude();
		}
		for_every_ptr(door, r->get_all_doors())
		{
			// invisible doors are fine here
			if (!door->can_move_through() ||
				!door->is_relevant_for_movement() ||
				!door->is_relevant_for_vr()) continue;
			if (door->get_vr_elevator_altitude().is_set())
			{
				assign_optional_out_param(_depth, 0);
				return door->get_vr_elevator_altitude();
			}
			if (auto* d = door->get_door())
			{
				if (d->get_vr_elevator_altitude().is_set())
				{
					assign_optional_out_param(_depth, rc.depth);
					return d->get_vr_elevator_altitude();
				}
			}
		}
		for_every_ptr(door, r->get_all_doors())
		{
			// invisible doors are fine here
			if (!door->can_move_through() ||
				!door->is_relevant_for_movement() ||
				!door->is_relevant_for_vr()) continue;
			if (auto* d = door->get_door())
			{
				if (!d->is_world_separator_door())
				{
					if (auto* otherRoom = door->get_room_on_other_side())
					{
						if (!checkedRooms.does_contain(otherRoom) &&
							!roomsToCheck.does_contain(RoomToCheck(otherRoom)))
						{
							// if it is already on the list, it should have shallower depth
							roomsToCheck.push_back(RoomToCheck(otherRoom, rc.depth + 1));
						}
					}
				}
			}
		}
	}

	return NP;
}

bool Room::place_door_on_poi(DoorInRoom* _dir, Name const& _poi)
{
	if (!_dir)
	{
		return false;
	}

	Random::Generator rg = get_individual_random_generator();
	rg.advance_seed(23597, 97345);
	ARRAY_STATIC(Framework::PointOfInterestInstance*, pois, 32);
	for_every_point_of_interest(_poi, [&pois, &rg](Framework::PointOfInterestInstance* _fpoi) {pois.push_back_or_replace(_fpoi, rg); });
	if (!pois.is_empty())
	{
		Framework::PointOfInterestInstance* poi = pois[rg.get_int(pois.get_size())];
		_dir->set_placement(poi->calculate_placement());
		return true;
	}
	else
	{
		PendingDoorOnPOI dop;
		dop.door = _dir;
		dop.poi = _poi;
		pendingDoorsOnPOIs.push_back(dop);
		return false;
	}
}

bool Room::place_door_on_poi_tagged(DoorInRoom* _dir, TagCondition const& _tagged)
{
	if (!_dir)
	{
		return false;
	}

	Random::Generator rg = get_individual_random_generator();
	rg.advance_seed(23597, 97345);
	ARRAY_STATIC(Framework::PointOfInterestInstance*, pois, 32);
	for_every_point_of_interest(NP, [&pois, &rg, &_tagged](Framework::PointOfInterestInstance* _fpoi) { if(_tagged.check(_fpoi->get_tags())) pois.push_back_or_replace(_fpoi, rg); });
	if (!pois.is_empty())
	{
		Framework::PointOfInterestInstance* poi = pois[rg.get_int(pois.get_size())];
		_dir->set_placement(poi->calculate_placement());
		return true;
	}
	else
	{
		PendingDoorOnPOI dop;
		dop.door = _dir;
		dop.tagged = _tagged;
		pendingDoorsOnPOIs.push_back(dop);
		return false;
	}
}

void Room::place_pending_doors_on_pois()
{
	auto copy = pendingDoorsOnPOIs;
	pendingDoorsOnPOIs.clear();
	for_every(dop, copy)
	{
		if (dop->poi.is_valid())
		{
			place_door_on_poi(dop->door.get(), dop->poi);
		}
		else if (! dop->tagged.is_empty())
		{
			place_door_on_poi_tagged(dop->door.get(), dop->tagged);
		}
	}
}

void Room::update_small_room()
{
	bool roomIsSmall = false;
	if (get_doors().get_size() == 2)
	{
		Vector3 d0 = get_doors()[0]->get_placement().get_translation();
		Vector3 d1 = get_doors()[1]->get_placement().get_translation();
		if (Vector3::distance(d0, d1) < magic_number hardcoded 0.2f)
		{
			roomIsSmall = true;
		}
	}

	smallRoom = roomIsSmall;
}
