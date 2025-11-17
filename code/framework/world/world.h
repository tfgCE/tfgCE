#pragma once

#include "..\..\core\concurrency\mrswGuard.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"

#include "subWorld.h"

#include "..\object\object.h"

#include "..\ai\aiMessageQueue.h"

#ifdef AN_OUTPUT
#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define LOG_WORLD_GENERATION
#endif
#endif

namespace Framework
{
	class ActivationGroup;
	class JobSystem;

	class Room;

	namespace BuildPresenceLinks
	{
		enum Flag
		{
			Clear = 1, // clear existing
			Add = 2, // add to existing - do not clear
			Objects = 4,
			TemporaryObjects = 8,
			//
			Add_Objects = Add | Objects,
			Add_TemporaryObjects = Add | TemporaryObjects,
			//
			All = Objects | TemporaryObjects,
		};
		typedef int Flags;
	};

	/*
	 *	World contains ALL objects, rooms, doors.
	 *	Is responsible for advancement - TODO maybe sub world should be responsible for advancement as subworlds could be put to sleep?
	 */
	class World
	{
	public:
		World();
		~World();

		bool is_being_destroyed() const { return beingDestroyed; }

	public:
		float get_world_time() const { return (float)worldTime; }

		void add(SubWorld* _subWorld);
		void remove(SubWorld* _subWorld);

		void suspend_ai_messages(bool _suspend = true); // will stop creating new ai messages etc

		void ready_to_advance(SubWorld* _limitToSubWorld);
		void advance_logic(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld);
		void advance_physically(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld);
		void advance_temporary_objects(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld);
		void advance_build_presence_links(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld, BuildPresenceLinks::Flags _flags = BuildPresenceLinks::All);
		void advance_finalise_frame(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld);
		void finalise_advance(SubWorld* _limitToSubWorld);

		::System::ShaderProgramBindingContext const & get_shader_program_binding_context() const { return shaderProgramBindingContext; }
		::System::ShaderProgramBindingContext & access_shader_program_binding_context() { return shaderProgramBindingContext; }

		AI::Message* create_ai_message(Name const & _name);

		Array<Object*> & get_all_objects() { return allObjects; }
		Array<Object*> & get_objects() { return objects; }

		Array<Room*> & get_all_rooms() { return allRooms; }
		Array<Room*> & get_rooms() { return rooms; }

		void for_every_object_with_id(Name const & _variable, int _id, OnFoundObject _for_every_object_with_id);

		bool ready_to_activate_all_inactive(ActivationGroup* _activationGroup); // returns true if sure that no other objects requires readying
		void activate_all_inactive(ActivationGroup* _activationGroup);

		bool does_have_active_room(Room* _room) const;

		void log(LogInfoContext & _log);

		int get_objects_to_advance_count() const { return objectsToAdvance.get_size(); }
		int get_objects_to_advance_once_count() const { return objectsToAdvanceOnce.get_size(); }
		int get_objects_to_advanced_last_frame_count() const { return advancedObjectsLastFrame; }

#ifdef AN_DEVELOPMENT
		void dev_check_if_ok(SubWorld* _limitToSubWorld = nullptr) const;
#endif

	protected: friend class SubWorld;

	protected: friend class Room;
		void add(Room* _room);
		void activate(Room* _room);
		void remove(Room* _room);

	protected: friend class Environment;
		void add(Environment* _environment);
		void activate(Environment* _environment);
		void remove(Environment* _environment);

	protected: friend class Door;
		void add(Door* _door);
		void activate(Door* _door);
		void remove(Door* _door);

	protected: friend class Object;
		void add(Object* _object);
		void activate(Object* _object);
		void remove(Object* _object);

	private:
		MRSW_GUARD_MUTABLE(subWorldsGuard);
		MRSW_GUARD_MUTABLE(doorsGuard);
		MRSW_GUARD_MUTABLE(allDoorsGuard);
		MRSW_GUARD_MUTABLE(roomsGuard);
		MRSW_GUARD_MUTABLE(allRoomsGuard);
		MRSW_GUARD_MUTABLE(environmentsGuard);
		MRSW_GUARD_MUTABLE(allEnvironmentsGuard);
		MRSW_GUARD_MUTABLE(objectsGuard);
		MRSW_GUARD_MUTABLE(allObjectsGuard);
		MRSW_GUARD_MUTABLE(actorsGuard);
		MRSW_GUARD_MUTABLE(allActorsGuard);
		MRSW_GUARD_MUTABLE(sceneriesGuard);
		MRSW_GUARD_MUTABLE(allSceneriesGuard);

		bool beingDestroyed = false;

		// all worlds?
		Array<SubWorld*> subWorlds;

		// all doors, rooms and objects
		Array<Door*> doors, allDoors;
		Array<Room*> rooms, allRooms;
		Array<Environment*> environments, allEnvironments; // are added to both rooms and environments
		Array<Object*> objects, allObjects;
		Array<Actor*> actors, allActors;
		Array<Scenery*> sceneries, allSceneries;

		int advancedObjectsLastFrame = 0;
		Array<Object*> objectsToAdvance; // this currently breaks limitToSubWorld but we just have to deal with that until it is required otherwise)
		Array<Object*> objectsToAdvanceOnce;
		Array<Object*> objectsAdvancingOnce; // dropped after advancement

		bool aiMessagesSuspended = false;
		AI::MessageQueue aiMessages;
		AI::MessageQueue aiProcessedMessages;

		double worldTime;

		int updateSuspendingAdvanceRoomsStartIdx = 0;
		RUNTIME_ Array<Room*> updateSuspendingAdvanceRooms; // we only update in a limited amount of rooms, to prevent unnecessary calls

		::System::ShaderProgramBindingContext shaderProgramBindingContext; // for any parameters
		float time;
		float timePeriod; // length of time period

		void distribute_ai_messages(float _deltaTime);
		void remove_processed_ai_messages();

		void advance_temporary_objects_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld);

#ifdef AN_DEVELOPMENT
		void check_if_all_temporary_objects_have_their_own_presences(SubWorld* _subWorld);
		void check_if_all_temporary_objects_have_their_own_presences_in(SubWorld* _subWorld);
#endif

		void advance_temporary_objects_determine_build_presence_links(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld);
		void advance_temporary_objects_determine_build_presence_links_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld);
		void advance_temporary_objects_build_presence_links(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld);
		void advance_temporary_objects_build_presence_links_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld);

		void advance_temporary_objects_finalise_frame(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld);
		void advance_temporary_objects_finalise_frame_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld);

#ifdef AN_DEVELOPMENT
	public:
		struct MultithreadCheck
		{
			enum State
			{
				Undefined, // most likely we may read, writing might be an issue
				OnlyRead,
				AllowWrite
			};
			// within room(s)
			State roomsDoorsState = Undefined;
			State roomsObjectsState = Undefined;

			// world, subworld, region
			State worldState = Undefined;

			bool writingWhileUndefinedOnlyWhileInSync = false;
		};
	private:
		MultithreadCheck multithreadCheck;
	public:
		// within room(s)
		bool multithread_check__reading_rooms_doors_is_safe() const;
		bool multithread_check__writing_rooms_doors_is_allowed() const;
		bool multithread_check__reading_rooms_objects_is_safe() const;
		bool multithread_check__writing_rooms_objects_is_allowed() const;

		void multithread_check__set__accessing_rooms_doors(MultithreadCheck::State _state = MultithreadCheck::Undefined) { multithreadCheck.roomsDoorsState = _state; }
		void multithread_check__set__accessing_rooms_objects(MultithreadCheck::State _state = MultithreadCheck::Undefined) { multithreadCheck.roomsObjectsState = _state; }
		void multithread_check__set__accessing_rooms_doors_and_objects(MultithreadCheck::State _state = MultithreadCheck::Undefined) { multithreadCheck.roomsDoorsState = multithreadCheck.roomsObjectsState = _state; }

		// world, subworld, region
		bool multithread_check__reading_world_is_safe() const;
		bool multithread_check__writing_world_is_allowed() const;

		void multithread_check__set__accessing_world(MultithreadCheck::State _state = MultithreadCheck::Undefined) { multithreadCheck.worldState = _state; }

		void multithread_check__set__writing_while_undefined_only_while_in_sync(bool _writingWhileUndefinedOnlyWhileInSync) { multithreadCheck.writingWhileUndefinedOnlyWhileInSync = _writingWhileUndefinedOnlyWhileInSync; }
#endif
	};
};
