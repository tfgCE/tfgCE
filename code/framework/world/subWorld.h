#pragma once

#include "..\..\core\collision\collisionInfoProvider.h"
#include "..\..\core\concurrency\mrswGuard.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"

#include "door.h"
#include "doorInRoom.h"
#include "room.h"
#include "region.h"
#include "environment.h"

#include "..\object\object.h"
#include "..\object\temporaryObject.h"

namespace Framework
{
	class BullshotSystem;
	class JobSystem;

	class Room;
	class SubWorld;
	class TemporaryObject;
	class TemporaryObjectType;
	class World;
	
	struct TemporaryObjectTypeInSubWorld
	{
	private:
#ifdef AN_DEVELOPMENT
		int babeCheck = 0x00babe00;
		bool am_I_myself() const { return babeCheck == 0x00babe00; }
#endif

		TemporaryObjectType* type;
		Concurrency::Counter count; // all existent, active and non active, to know how many did we create
		Concurrency::Counter active; // all active, to be activated or to be deactivated
		Concurrency::Counter pendingCreation; // if we added a job to create inactive, we mark it here

		friend class SubWorld;
		friend class TemporaryObject;
	};

	/*
	 *	Sub world is part of the world that doesn't have to be directly connected to other sub worlds (but might be)
	 *	It divides world into advancable parts (TODO)
	 */
	class SubWorld
	{
	public:
		SubWorld(World* _inWorld);
		SubWorld(SubWorld* _inSubWorld);
		~SubWorld();

		void init();

		void clear_gravity();
		void set_gravity(float _force);

		Random::Generator const & get_individual_random_generator() const { return individualRandomGenerator; }
		void set_individual_random_generator(Random::Generator const & _individualRandomGenerator);

		inline World* get_in_world() const { return inWorld; }
		inline SubWorld* get_in_sub_world() const { return inSubWorld; }

		Collision::CollisionInfoProvider const & get_collision_info_provider() const { return collisionInfoProvider; }
		Collision::CollisionInfoProvider & access_collision_info_provider() { return collisionInfoProvider; }

		Array<Room*> const & get_all_rooms() const { return allRooms; }
		Array<Room*> const & get_rooms() const { return rooms; }

		Door* find_door_tagged(Name const & _tag) const;
		Room* find_room_tagged(Name const & _tag) const;

		Array<Object*> const & get_all_objects() const { return allObjects; }
		Array<Actor*> const & get_all_actors() const { return allActors; }
		Array<Item*> const & get_all_items() const { return allItems; }
		Array<Scenery*> const & get_all_sceneries() const { return allSceneries; }

		Array<Object*> const & get_objects() const { return objects; }
		Array<Actor*> const & get_actors() const { return actors; }
		Array<Item*> const & get_items() const { return items; }
		Array<Scenery*> const & get_sceneries() const { return sceneries; }

		Array<Region*> const & get_regions() const { return regions; }

		void for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _for_every_point_of_interest);
		bool find_any_point_of_interest(Optional<Name> const & _poiName, OUT_ PointOfInterestInstance* & _outPOI, Random::Generator * _randomGenerator = nullptr);

		::System::ShaderProgramBindingContext const & get_shader_program_binding_context() const { return shaderProgramBindingContext; }
		::System::ShaderProgramBindingContext & access_shader_program_binding_context() { return shaderProgramBindingContext; }

		void manage_inactive_temporary_objects(); // this is to make sure we have plenty of them ready to be used, also reset any to that is inactive

#ifdef AN_ALLOW_BULLSHOTS
	private: friend class BullshotSystem;
		  Array<TemporaryObject*> const& get_active_temporary_objects() { return temporaryObjectsActive; }
#endif

	public:
#ifdef AN_DEVELOPMENT
		bool testSubWorld = false;
		void be_test_sub_world(bool _testSubWorld = true) { testSubWorld = _testSubWorld; }
		bool is_test_sub_world() const { return testSubWorld; }
#endif

	public:
		void add(SubWorld* _subWorld);
		void remove(SubWorld* _subWorld);

		void add(Room* _room);
		void activate(Room* _room);
		void remove(Room* _room);

		void add(Region* _region);
		void remove(Region* _region);

		void add(Environment* _environment);
		void activate(Environment* _environment);
		void remove(Environment* _environment);

		void add(Door* _door);
		void activate(Door* _door);
		void remove(Door* _door);

		void add(Object* _object);
		void activate(Object* _object);
		void remove(Object* _object);
			
	public: // temporary objects have different rules
		TemporaryObject* get_one_for(TemporaryObjectType* _type, Room* _room, Optional<Vector3> const & _locWS = NP);
	protected: friend class TemporaryObject;
		void add(TemporaryObject* _object);
		void on_end_initialisation_add_to_inactive(TemporaryObject* _object);
		void remove(TemporaryObject* _object);
		void add_to_activate(TemporaryObject* _to);
		void add_to_deactivate(TemporaryObject* _to);

	protected: friend class World;
		// this should be called at proper time during frame, activate, advance, deactivate
		void activate_temporary_objects();
		void deactivate_temporary_objects();

	protected: friend class World;
		void set_in_world(World* _world) { inWorld = _world; }
		void set_in_sub_world(SubWorld* _subWorld) { inSubWorld = _subWorld; }

	private:
		World* inWorld;
		SubWorld* inSubWorld;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool isInitialised = false;
#endif

		Random::Generator individualRandomGenerator;

		MRSW_GUARD_MUTABLE(subWorldsGuard);
		MRSW_GUARD_MUTABLE(doorsGuard);
		MRSW_GUARD_MUTABLE(allDoorsGuard);
		MRSW_GUARD_MUTABLE(roomsGuard);
		MRSW_GUARD_MUTABLE(allRoomsGuard);
		MRSW_GUARD_MUTABLE(regionsGuard);
		MRSW_GUARD_MUTABLE(allRegionsGuard);
		MRSW_GUARD_MUTABLE(environmentsGuard);
		MRSW_GUARD_MUTABLE(allEnvironmentsGuard);
		MRSW_GUARD_MUTABLE(objectsGuard);
		MRSW_GUARD_MUTABLE(allObjectsGuard);
		MRSW_GUARD_MUTABLE(actorsGuard);
		MRSW_GUARD_MUTABLE(allActorsGuard);
		MRSW_GUARD_MUTABLE(itemsGuard);
		MRSW_GUARD_MUTABLE(allItemsGuard);
		MRSW_GUARD_MUTABLE(sceneriesGuard);
		MRSW_GUARD_MUTABLE(allSceneriesGuard);

		Array<SubWorld*> subWorlds;

		Array<Door*> doors, allDoors;
		Array<Room*> rooms, allRooms; // in all regions
		Array<Region*> regions;
		Array<Environment*> environments, allEnvironments; // are added to rooms, regions and environments
		Array<Object*> objects, allObjects;
		Array<Actor*> actors, allActors;
		Array<Item*> items, allItems;
		Array<Scenery*> sceneries, allSceneries;

		// temporary objects - kept per sub world
		Array<TemporaryObject*> allTemporaryObjects;
		Array<TemporaryObject*> temporaryObjectsInactive;
		Array<TemporaryObject*> temporaryObjectsInactiveProvided; // when getting one, it goes into provided
		Array<TemporaryObject*> temporaryObjectsActive;
		// temporary objects are gathered during frame and processed at proper time
		Array<TemporaryObject*> temporaryObjectsToActivate; // gathered during frame
		Array<TemporaryObject*> temporaryObjectsToDeactivate; // gathered during frame
		MRSW_GUARD_MUTABLE(temporaryObjectsActiveGuard);
		Concurrency::SpinLock temporaryObjectsLock = Concurrency::SpinLock(TXT("Framework.SubWorld.temporaryObjectsLock"));

		Array<TemporaryObjectTypeInSubWorld> temporaryObjectTypes;

		Collision::CollisionInfoProvider collisionInfoProvider;

		::System::ShaderProgramBindingContext shaderProgramBindingContext; // for any parameters

		int frameIntervalToResetTemporaryObject = 0;

		friend class World;

		void set_defaults();

		void create_inactive(TemporaryObjectType* _type);
	};
};
