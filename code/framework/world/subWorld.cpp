#include "subWorld.h"

#include "presenceLink.h"
#include "world.h"

#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"
#include "..\library\library.h"
#include "..\module\modulePresence.h"
#include "..\module\moduleAppearance.h"
#include "..\object\actor.h"
#include "..\object\item.h"
#include "..\object\scenery.h"
#include "..\object\temporaryObject.h"
#include "..\render\presenceLinkProxy.h"

#include "..\..\core\performance\performanceUtils.h"

#include "..\..\core\concurrency\scopedMRSWGuard.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define MEASURE_TO_TIMES
#endif

//

using namespace Framework;

//

#define PRECREATE_TEMPORARY_OBJECTS

SubWorld::SubWorld(World* _inWorld)
: inWorld(nullptr)
, inSubWorld(nullptr)
{
	ASSERT_SYNC;
	an_assert(_inWorld);
	_inWorld->add(this);
	set_defaults();

#ifdef AN_ALLOW_BULLSHOTS
	BullshotSystem::set_sub_world(this);
#endif
}

SubWorld::SubWorld(SubWorld* _inSubWorld)
: inWorld(nullptr)
, inSubWorld(nullptr)
{
	ASSERT_SYNC;
	an_assert(_inSubWorld);
	_inSubWorld->add(this);
	_inSubWorld->get_in_world()->add(this);
	set_defaults();

#ifdef AN_ALLOW_BULLSHOTS
	BullshotSystem::set_sub_world(this);
#endif
}

void SubWorld::set_defaults()
{
	/*
		Collision::CollisionInfoProvider defaultCollisionInfoProvider;
		defaultCollisionInfoProvider.set_gravity_dir_based(-Vector3::zAxis, 9.81f);
		collisionInfoProvider.fill_missing_with(defaultCollisionInfoProvider);
	*/
}

void SubWorld::init()
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	isInitialised = true;
#endif
#ifdef PRECREATE_TEMPORARY_OBJECTS
	an_assert(temporaryObjectTypes.is_empty());
	// first build an array of tots, then create them. otherwise we will move them through memory and hilarious things may happen (if someone finds memory overrun hilarious)
	Library::get_current()->do_for_every_of_type(TemporaryObjectType::library_type(),
		[this](LibraryStored * _libraryStored)
	{
		if (auto * tot = fast_cast<TemporaryObjectType>(_libraryStored))
		{
			TemporaryObjectTypeInSubWorld totisw;
			totisw.type = tot;
			temporaryObjectTypes.push_back(totisw);
		}
	});
	for_every(tot, temporaryObjectTypes)
	{
		for_count(int, i, tot->type->get_precreate_count())
		{
			create_inactive(tot->type);
		}
	}
#endif
}

void SubWorld::clear_gravity()
{
	collisionInfoProvider.clear_gravity();
}

void SubWorld::set_gravity(float _force)
{
	Collision::CollisionInfoProvider defaultCollisionInfoProvider;
	if (_force != 0.0f)
	{
		defaultCollisionInfoProvider.set_gravity_dir_based(-Vector3::zAxis, _force);
	}
	else
	{
		defaultCollisionInfoProvider.set_zero_gravity();
	}
	collisionInfoProvider.fill_missing_with(defaultCollisionInfoProvider);
}

SubWorld::~SubWorld()
{
	scoped_call_stack_info(TXT("~subworld"));
#ifdef AN_ALLOW_BULLSHOTS
	BullshotSystem::clear_sub_world(this);
#endif

	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());

	// make all objects cease to exist first, then destruct them
	{
		scoped_call_stack_info(TXT("objects : cease_to_exist"));
		MRSW_GUARD_READ_SCOPE(allObjectsGuard);
		for_every_ptr(object, allObjects)
		{
			object->cease_to_exist(true);
		}
	}

	{
		scoped_call_stack_info(TXT("objects : destruct_and_delete"));
		while (!allObjects.is_empty())
		{
			allObjects.get_first()->destruct_and_delete(); // this will properly close and delete object
		}
	}

	{
		scoped_call_stack_info(TXT("temporary objects : cease_to_exist"));
		MRSW_GUARD_READ_SCOPE(temporaryObjectsActiveGuard);
		for_every_ptr(object, allTemporaryObjects)
		{
			if (object->is_active())
			{
				object->cease_to_exist(true);
			}
		}
	}
	{
		scoped_call_stack_info(TXT("temporary objects : deactivate"));
		deactivate_temporary_objects();
	}

	{
		scoped_call_stack_info(TXT("temporary objects : destruct_and_delete"));
		while (!allTemporaryObjects.is_empty())
		{
			allTemporaryObjects.get_first()->destruct_and_delete();
		}
		allTemporaryObjects.clear();
	}

	// first destroy all nested subworlds
	{
		scoped_call_stack_info(TXT("subworlds"));
		while (!subWorlds.is_empty())
		{
			delete* (subWorlds.begin());
		}
	}

	// and now continue with stuff at this level
	{
		scoped_call_stack_info(TXT("environments"));
		while (!allEnvironments.is_empty()) // before rooms, so they will be removed from rooms
		{
			delete* (allEnvironments.begin());
		}
	}
	{
		scoped_call_stack_info(TXT("rooms"));
		while (!allRooms.is_empty())
		{
			delete* (allRooms.begin());
		}
	}
	{
		scoped_call_stack_info(TXT("regions"));
		while (!regions.is_empty())
		{
			delete* (regions.begin());
		}
	}
	{
		scoped_call_stack_info(TXT("doors"));
		while (!allDoors.is_empty())
		{
			delete* (allDoors.begin());
		}
	}
	temporaryObjectsInactive.clear();
	temporaryObjectsInactiveProvided.clear();
	temporaryObjectsActive.clear();
	temporaryObjectsToActivate.clear();
	temporaryObjectsToDeactivate.clear();

	// remove this subworld from subworld and world
	if (inSubWorld)
	{
		inSubWorld->remove(this);
	}
	if (inWorld)
	{
		inWorld->remove(this);
	}
}

void SubWorld::add(SubWorld* _subWorld)
{
	ASSERT_SYNC;
	MRSW_GUARD_WRITE_SCOPE(subWorldsGuard);
	an_assert(_subWorld->get_in_sub_world() == nullptr, TXT("should not be in other sub world"));
	_subWorld->set_in_sub_world(this);
	subWorlds.push_back(_subWorld);
}

void SubWorld::remove(SubWorld* _subWorld)
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	MRSW_GUARD_WRITE_SCOPE(subWorldsGuard);
	_subWorld->set_in_sub_world(nullptr);
	subWorlds.remove_fast(_subWorld);
}

void SubWorld::add(Room* _room)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(isInitialised, TXT("call init() post creation"));
#endif
	ASSERT_SYNC;
	an_assert(_room->get_in_sub_world() == nullptr, TXT("should not be in other sub world"));
	_room->set_in_sub_world(this);
	{
		MRSW_GUARD_WRITE_SCOPE(allRoomsGuard);
		allRooms.push_back(_room);
	}
	if (_room->is_world_active())
	{
		MRSW_GUARD_WRITE_SCOPE(roomsGuard);
		rooms.push_back(_room);
	}
	if (Environment* environment = fast_cast<Environment>(_room))
	{
		add(environment);
	}
}

void SubWorld::activate(Room* _room)
{
	ASSERT_SYNC;
	an_assert(_room->is_world_active());
	an_assert(! rooms.does_contain(_room));
	{
		MRSW_GUARD_WRITE_SCOPE(roomsGuard);
		rooms.push_back(_room);
	}
	if (Environment* environment = fast_cast<Environment>(_room))
	{
		activate(environment);
	}
}

void SubWorld::remove(Room* _room)
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	_room->set_in_sub_world(nullptr);
	{
		MRSW_GUARD_WRITE_SCOPE(allRoomsGuard);
		allRooms.remove_fast(_room);
	}
	{
		MRSW_GUARD_WRITE_SCOPE(roomsGuard);
		rooms.remove_fast(_room);
	}
	if (Environment* environment = fast_cast<Environment>(_room))
	{
		remove(environment);
	}
}

void SubWorld::add(Region* _region)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(isInitialised, TXT("call init() post creation"));
#endif
	ASSERT_SYNC;
	an_assert(_region->get_in_sub_world() == nullptr);
	{
		MRSW_GUARD_WRITE_SCOPE(regionsGuard);
		an_assert(!regions.does_contain(_region), TXT("should be already added to this sub world"));
		regions.push_back(_region);
	}
	_region->set_in_sub_world(this);
}

void SubWorld::remove(Region* _region)
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	an_assert(_region->get_in_sub_world() == this);
	{
		MRSW_GUARD_WRITE_SCOPE(regionsGuard);
		regions.remove_fast(_region);
	}
	_region->set_in_sub_world(nullptr);
}

void SubWorld::add(Environment* _environment)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(isInitialised, TXT("call init() post creation"));
#endif
	ASSERT_SYNC;
	{
		MRSW_GUARD_WRITE_SCOPE(allEnvironmentsGuard);
		an_assert(!allEnvironments.does_contain(_environment), TXT("should be already added to this sub world"));
		allEnvironments.push_back(_environment);
	}
	if (_environment->is_world_active())
	{
		MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
		environments.push_back(_environment);
	}
}

void SubWorld::activate(Environment* _environment)
{
	ASSERT_SYNC;
	an_assert(_environment->is_world_active());
	{
		MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
		an_assert(!environments.does_contain(_environment));
		environments.push_back(_environment);
	}
}

void SubWorld::remove(Environment* _environment)
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	{
		MRSW_GUARD_WRITE_SCOPE(allEnvironmentsGuard);
		allEnvironments.remove_fast(_environment);
	}
	{
		MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
		environments.remove_fast(_environment);
	}
}

void SubWorld::add(Door* _door)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(isInitialised, TXT("call init() post creation"));
#endif
	ASSERT_SYNC;
	an_assert(_door->get_in_sub_world() == nullptr, TXT("should not be in other sub world"));
	_door->set_in_sub_world(this);
	{
		MRSW_GUARD_WRITE_SCOPE(allDoorsGuard);
		allDoors.push_back(_door);
	}
	if (_door->is_world_active())
	{
		MRSW_GUARD_WRITE_SCOPE(doorsGuard);
		doors.push_back(_door);
	}
}

void SubWorld::activate(Door* _door)
{
	ASSERT_SYNC;
	an_assert(_door->is_world_active());
	{
		MRSW_GUARD_WRITE_SCOPE(doorsGuard);
		an_assert(!doors.does_contain(_door));
		doors.push_back(_door);
	}
}

void SubWorld::remove(Door* _door)
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	_door->set_in_sub_world(nullptr);
	{
		MRSW_GUARD_WRITE_SCOPE(allDoorsGuard);
		allDoors.remove_fast(_door);
	}
	{
		MRSW_GUARD_WRITE_SCOPE(doorsGuard);
		doors.remove_fast(_door);
	}
}

void SubWorld::add(Object* _object)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(isInitialised, TXT("call init() post creation"));
#endif
	ASSERT_SYNC;
	if (_object->get_in_sub_world())
	{
		_object->get_in_sub_world()->remove(_object);
	}
	_object->set_in_sub_world(this);
	{
		MRSW_GUARD_WRITE_SCOPE(allObjectsGuard);
		allObjects.push_back(_object);
	}
	if (_object->is_world_active())
	{
		MRSW_GUARD_WRITE_SCOPE(objectsGuard);
		objects.push_back(_object);
	}
	if (Actor * actor = fast_cast<Actor>(_object))
	{
		{
			MRSW_GUARD_WRITE_SCOPE(allActorsGuard);
			allActors.push_back(actor);
		}
		if (actor->is_world_active())
		{
			MRSW_GUARD_WRITE_SCOPE(actorsGuard);
			actors.push_back(actor);
		}
	}
	if (Item * item = fast_cast<Item>(_object))
	{
		{
			MRSW_GUARD_WRITE_SCOPE(allItemsGuard);
			allItems.push_back(item);
		}
		if (item->is_world_active())
		{
			MRSW_GUARD_WRITE_SCOPE(itemsGuard);
			items.push_back(item);
		}
	}
	if (Scenery * scenery = fast_cast<Scenery>(_object))
	{
		{
			MRSW_GUARD_WRITE_SCOPE(allSceneriesGuard);
			allSceneries.push_back(scenery);
		}
		if (scenery->is_world_active())
		{
			MRSW_GUARD_WRITE_SCOPE(sceneriesGuard);
			sceneries.push_back(scenery);
		}
	}
}

void SubWorld::activate(Object* _object)
{
	ASSERT_SYNC;
	an_assert(_object->is_world_active());
	{
		MRSW_GUARD_WRITE_SCOPE(objectsGuard);
		an_assert(!objects.does_contain(_object));
		objects.push_back(_object);
	}
	if (Actor * actor = fast_cast<Actor>(_object))
	{
		MRSW_GUARD_WRITE_SCOPE(actorsGuard);
		actors.push_back(actor);
	}
	if (Item * item = fast_cast<Item>(_object))
	{
		MRSW_GUARD_WRITE_SCOPE(itemsGuard);
		items.push_back(item);
	}
	if (Scenery * scenery = fast_cast<Scenery>(_object))
	{
		MRSW_GUARD_WRITE_SCOPE(sceneriesGuard);
		sceneries.push_back(scenery);
	}
}

void SubWorld::remove(Object* _object)
{
	ASSERT_SYNC_OR(!get_in_world() || get_in_world()->is_being_destroyed());
	_object->set_in_sub_world(nullptr);
	{
		MRSW_GUARD_WRITE_SCOPE(allObjectsGuard);
		allObjects.remove_fast(_object);
	}
	{
		MRSW_GUARD_WRITE_SCOPE(objectsGuard);
		objects.remove_fast(_object);
	}
	if (Actor * actor = fast_cast<Actor>(_object))
	{
		{
			MRSW_GUARD_WRITE_SCOPE(allActorsGuard);
			allActors.remove_fast(actor);
		}
		{
			MRSW_GUARD_WRITE_SCOPE(actorsGuard);
			actors.remove_fast(actor);
		}
	}
	if (Item * item = fast_cast<Item>(_object))
	{
		{
			MRSW_GUARD_WRITE_SCOPE(allItemsGuard);
			allItems.remove_fast(item);
		}
		{
			MRSW_GUARD_WRITE_SCOPE(itemsGuard);
			items.remove_fast(item);
		}
	}
	if (Scenery * scenery = fast_cast<Scenery>(_object))
	{
		{
			MRSW_GUARD_WRITE_SCOPE(allSceneriesGuard);
			allSceneries.remove_fast(scenery);
		}
		{
			MRSW_GUARD_WRITE_SCOPE(sceneriesGuard);
			sceneries.remove_fast(scenery);
		}
	}
}

void SubWorld::for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _for_every_point_of_interest)
{
	ASSERT_NOT_ASYNC_OR(get_in_world()->multithread_check__reading_world_is_safe());
	MRSW_GUARD_READ_SCOPE(allRoomsGuard);
	for_every_ptr(room, allRooms)
	{
		room->for_every_point_of_interest(_poiName, _for_every_point_of_interest);
	}
}

bool SubWorld::find_any_point_of_interest(Optional<Name> const & _poiName, OUT_ PointOfInterestInstance* & _outPOI, Random::Generator * _randomGenerator)
{
	ARRAY_STACK(PointOfInterestInstance*, pois, 32);
	Random::Generator randomGenerator;
	if (!_randomGenerator)
	{
		_randomGenerator = &randomGenerator;
	}

	{
		MRSW_GUARD_READ_SCOPE(allRoomsGuard);
		for_every_ptr(room, allRooms)
		{
			room->for_every_point_of_interest(_poiName, [&pois, _randomGenerator](PointOfInterestInstance* _fpoi) {pois.push_back_or_replace(_fpoi, *_randomGenerator); });
		}
	}
	if (pois.get_size() > 0)
	{
		_outPOI = pois[_randomGenerator->get_int(pois.get_size())];
		return true;
	}
	else
	{
		_outPOI = nullptr;
		return false;
	}
}

Door* SubWorld::find_door_tagged(Name const & _tag) const
{
	ASSERT_NOT_ASYNC_OR(get_in_world()->multithread_check__reading_world_is_safe());
	MRSW_GUARD_READ_SCOPE(allRoomsGuard);
	for_every_ptr(room, allRooms)
	{
		if (Door* door = room->find_door_tagged(_tag))
		{
			return door;
		}
	}
	return nullptr;
}

Room* SubWorld::find_room_tagged(Name const & _tag) const
{
	MRSW_GUARD_READ_SCOPE(allRoomsGuard);
	for_every_ptr(room, allRooms)
	{
		if (room->get_tags().get_tag(_tag) != 0.0f)
		{
			return room;
		}
	}
	return nullptr;
}

TemporaryObject* SubWorld::get_one_for(TemporaryObjectType* _type, Room* _room, Optional<Vector3> const& _locWS)
{
	if (!_type)
	{
		return nullptr;
	}

	if (_room &&
		AVOID_CALLING_ACK_ ! _room->was_recently_seen_by_player() &&
		_type->should_skip_for_non_recently_rendered_room())
	{
		return nullptr;
	}

	if (_room && _locWS.is_set())
	{
		float blockingDist = 0.0f;
		Optional<float> blockingTime;
		Optional<int> blockingCount;
		if (_type->get_spawn_blocking_info(OUT_ blockingDist, OUT_ blockingTime, OUT_ blockingCount))
		{
#ifdef MEASURE_TO_TIMES
			MEASURE_AND_OUTPUT_PERFORMANCE(TXT("check object spawn block"));
#endif
			bool shouldSpawn = _room->check_temporary_object_spawn_block(_type, _locWS.get(), blockingDist, blockingTime, blockingCount);
			if (!shouldSpawn)
			{
				return nullptr;
			}
		}
	}

	{
		TemporaryObject* t = nullptr;
		{
#ifdef MEASURE_TO_TIMES
			MEASURE_AND_OUTPUT_PERFORMANCE(TXT("check inactive"));
#endif
			Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::get_one_for  check inactive"));
#ifdef MEASURE_TO_TIMES
			MEASURE_AND_OUTPUT_PERFORMANCE(TXT("check inactive post-lock %i"), temporaryObjectsInactive.get_size());
#endif
			for_every_ptr(to, temporaryObjectsInactive)
			{
				if (to->get_object_type() == _type)
				{
					t = to;
					an_assert(!temporaryObjectsInactiveProvided.does_contain(t));
					temporaryObjectsInactiveProvided.push_back(t);
					temporaryObjectsInactive.remove_fast_at(for_everys_index(to));
					break;
				}
			}
		}
		if (t)
		{
#ifdef WITH_TEMPORARY_OBJECT_HISTORY
			t->store_history(String(TXT("get_one_for")));
#endif
#ifdef MEASURE_TO_TIMES
			MEASURE_AND_OUTPUT_PERFORMANCE(TXT("reuse t \"%S\""), t->get_name().to_char());
#endif
			if ((!t->get_object_type() ||
				 t->get_object_type()->should_reset_on_reuse()) &&
				t->does_require_reset())
			{
#ifdef MEASURE_TO_TIMES
				MEASURE_AND_OUTPUT_PERFORMANCE(TXT("reset t \"%S\""), t->get_name().to_char());
#endif
#ifdef WITH_TEMPORARY_OBJECT_HISTORY
				t->store_history(String(TXT("get_one_for -> reset")));
#endif
				t->reset();
			}
			return t;
		}
	}

	// story:	I actually had a case in which array was partially initialised, reservedElements was set to 18 and elements was still 0,
	//			it was simultaneous SubWorld::get_one_for and there was a call from ParticlesBase::initialise, particles.set_size while
	//			the other thread was removing that very array as it was reseting temporary object
	// create it here, add to being initialised
	create_inactive(_type);

	return get_one_for(_type, _room); // get again to move to provided, don't provide _locWS as if we did do that, we're already pass the blocking system
}

void SubWorld::create_inactive(TemporaryObjectType* _type)
{
#ifdef MEASURE_TO_TIMES
	MEASURE_AND_OUTPUT_PERFORMANCE(TXT("create_inactive"));
#endif

	PERFORMANCE_GUARD_LIMIT_2(0.005f, TXT("[SubWorld::create_inactive (tempobj)]"), _type->get_name().to_string().to_char());

	TemporaryObject* t = new TemporaryObject(_type);
	t->init(this); // will add to this subworld and will initialise modules

#ifdef PRECREATE_TEMPORARY_OBJECTS
	{
		for_every(toe, temporaryObjectTypes)
		{
			if (toe->type == _type)
			{
				t->set_subworld_entry(toe);
			}
		}
	}
#endif

	// and now add to inactive
	on_end_initialisation_add_to_inactive(t);
}

void SubWorld::add(TemporaryObject* _to)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(isInitialised, TXT("call init() post creation"));
#endif
	Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::add"));

	an_assert(_to->get_in_sub_world() == nullptr);
	_to->set_in_sub_world(this);

	an_assert(_to->get_state() == TemporaryObjectState::NotAdded);
	an_assert(! allTemporaryObjects.does_contain(_to));
	allTemporaryObjects.push_back(_to);
	_to->set_state(TemporaryObjectState::BeingInitialised);
}

void SubWorld::on_end_initialisation_add_to_inactive(TemporaryObject* _to)
{
#ifdef MEASURE_TO_TIMES
	MEASURE_AND_OUTPUT_PERFORMANCE(TXT("on_end_initialisation_add_to_inactive"));
#endif
	Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::on_end_initialisation_add_to_inactive"));
#ifdef MEASURE_TO_TIMES
	MEASURE_AND_OUTPUT_PERFORMANCE(TXT("on_end_initialisation_add_to_inactive post-lock"));
#endif

	an_assert(_to->get_in_sub_world() == this);

	an_assert(_to->get_state() == TemporaryObjectState::BeingInitialised);
	an_assert(allTemporaryObjects.does_contain(_to));
	an_assert(! temporaryObjectsInactive.does_contain(_to));
	temporaryObjectsInactive.push_back(_to);
	_to->set_state(TemporaryObjectState::Inactive);

#ifdef WITH_TEMPORARY_OBJECT_HISTORY
	_to->store_history(String(TXT("created inactive")));
#endif

}

void SubWorld::remove(TemporaryObject* _to)
{
	Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::remove"));

	an_assert(_to->get_in_sub_world() == this);
	_to->set_in_sub_world(nullptr);

	allTemporaryObjects.remove_fast(_to);
	temporaryObjectsInactive.remove_fast(_to);
	temporaryObjectsInactiveProvided.remove_fast(_to);
	{
		MRSW_GUARD_WRITE_SCOPE(temporaryObjectsActiveGuard);
		temporaryObjectsActive.remove_fast(_to);
	}
	temporaryObjectsToActivate.remove_fast(_to);
	temporaryObjectsToDeactivate.remove_fast(_to);
}

void SubWorld::add_to_activate(TemporaryObject* _to)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	an_assert(isInitialised, TXT("call init() post creation"));
#endif
	Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::add_to_activate"));

	an_assert(!temporaryObjectsToActivate.does_contain(_to));
	temporaryObjectsToActivate.push_back(_to);
}

void SubWorld::add_to_deactivate(TemporaryObject* _to)
{
	Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::add_to_deactivate"));

	an_assert(!temporaryObjectsToDeactivate.does_contain(_to));
	temporaryObjectsToDeactivate.push_back(_to);
}

void SubWorld::activate_temporary_objects()
{
	MEASURE_PERFORMANCE_COLOURED(activateTemporaryObjects, Colour::green);

	Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::activate_temporary_objects"));

	for_every_ptr(to, temporaryObjectsToActivate)
	{
		{
			MRSW_GUARD_WRITE_SCOPE(temporaryObjectsActiveGuard);
			an_assert_immediate(! temporaryObjectsActive.does_contain(to));
			temporaryObjectsActive.push_back(to);
		}
		an_assert_immediate(temporaryObjectsInactiveProvided.does_contain(to));
		temporaryObjectsInactiveProvided.remove_fast(to);
		to->set_state(TemporaryObjectState::Active);
	}
	temporaryObjectsToActivate.clear();
#ifdef AN_DEVELOPMENT
	static bool warningIssued = false;
	if (!temporaryObjectsInactiveProvided.is_empty())
	{
		if (!warningIssued)
		{
			warn(TXT("we provided more objects than were activated? please remember to activate object when adding it"));
		}
		warningIssued = true;
	}
	else
	{
		warningIssued = false;
	}
#endif
}

void SubWorld::deactivate_temporary_objects()
{
	MEASURE_PERFORMANCE_COLOURED(deactivateTemporaryObjects, Colour::green);

	Concurrency::ScopedSpinLock lock(temporaryObjectsLock, TXT("SubWorld::deactivate_temporary_objects"));

	for_every_ptr(to, temporaryObjectsToDeactivate)
	{
		if (auto* p = to->get_presence())
		{
			// safe to be done here (not synced but in controlled job while advancing, and also not while sync job is performed)
			p->invalidate_presence_links();
		}
		to->set_state(TemporaryObjectState::Inactive);
		{
			MRSW_GUARD_WRITE_SCOPE(temporaryObjectsActiveGuard);
			temporaryObjectsActive.remove_fast(to);
		}
		an_assert_immediate(!temporaryObjectsInactive.does_contain(to));
		temporaryObjectsInactive.push_back(to);
	}
	temporaryObjectsToDeactivate.clear();
}

void SubWorld::set_individual_random_generator(Random::Generator const & _individualRandomGenerator)
{
	individualRandomGenerator = _individualRandomGenerator;
}

void SubWorld::manage_inactive_temporary_objects()
{
	MEASURE_PERFORMANCE(manageInactiveTemporaryObjects);
#ifdef PRECREATE_TEMPORARY_OBJECTS
	{
		MEASURE_PERFORMANCE(issueCreationOfTemporaryObjects);
		for_every(tot, temporaryObjectTypes)
		{
			int createdAndPending = tot->count.get() + tot->pendingCreation.get();
			int active = tot->active.get();
			if (createdAndPending - active < tot->type->get_precreate_count())
			{
				tot->pendingCreation.increase();
				MEASURE_PERFORMANCE(issueCreation);
				Game::get()->add_async_world_job(TXT("create inactive temporary object"),
					[this, tot]()
					{
						if (!Game::get()->does_want_to_cancel_creation())
						{
							create_inactive(tot->type);
							tot->pendingCreation.decrease();
						}
					});
				return;
			}
		}
	}
#endif

	--frameIntervalToResetTemporaryObject;
	if (frameIntervalToResetTemporaryObject <= 0)
	{
		frameIntervalToResetTemporaryObject = 10; // do one every ten frames - should be enough
		MEASURE_PERFORMANCE(resetObjectsThatRequireResetting);
		todo_note(TXT("reset objects that require resetting"));
		for_every_ptr(to, temporaryObjectsInactive)
		{
			if (to->does_require_reset())
			{
				MEASURE_PERFORMANCE(reset);
#ifdef WITH_TEMPORARY_OBJECT_HISTORY
				to->store_history(String(TXT("background reset")));
#endif
				to->reset();
				return;
			}
		}
	}
}
