#include "world.h"

#include "..\..\core\concurrency\scopedMRSWGuard.h"
#include "..\..\core\types\names.h"
#include "..\..\core\performance\performanceUtils.h"

#include "subWorld.h"

#include "..\module\moduleAI.h"
#include "..\module\moduleAppearance.h"
#include "..\module\moduleCollision.h"
#include "..\module\moduleCustom.h"
#include "..\module\moduleGameplay.h"
#include "..\module\moduleMovement.h"
#include "..\module\modulePresence.h"
#include "..\jobSystem\jobSystem.h"

#include "presenceLink.h"
#include "..\render\presenceLinkProxy.h"

#include "..\ai\aiMessage.h"
#include "..\ai\aiMind.h"
#include "..\ai\aiMindInstance.h"

#include "..\game\game.h"

#include "..\object\actor.h"
#include "..\object\scenery.h"

#include "..\..\framework\debug\previewGame.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define LOG_WORLD_DESTRUCTION

//#define AN_CHECK_PRESENCE_LINK_PLACEMENT_MISMATCH

#ifdef BUILD_NON_RELEASE
	#define WORLD_ADVANCE_PERFORMANCE_GUARD(_info) \
		::System::ScopedTimeStampLimitGuard temp_variable_named(timeGuard) (0.001f, _info);
	#define WORLD_ADVANCE_PERFORMANCE_GUARD_EX(_info, _limit) \
		::System::ScopedTimeStampLimitGuard temp_variable_named(timeGuard) (_limit, _info);
#else
	#define WORLD_ADVANCE_PERFORMANCE_GUARD(_info)
	#define WORLD_ADVANCE_PERFORMANCE_GUARD_EX(_info, _limit)
#endif

#ifndef AN_OUTPUT_WORLD_ACTIVATION
	#ifdef INSPECT_ACTIVATION_GROUP
		#define AN_OUTPUT_WORLD_ACTIVATION
	#endif
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(time);
DEFINE_STATIC_NAME(rhythmBeatA);
DEFINE_STATIC_NAME(rhythmBeatALength);

// destruction suspension reasons
DEFINE_STATIC_NAME(worldBeingDestroyed);
DEFINE_STATIC_NAME(readyToActivate);

//

World::World()
: worldTime(0.0)
, time(0.0f)
, timePeriod(1024.0f)
{
#ifdef AN_ALLOW_BULLSHOTS
	BullshotSystem::set_world(this);
#endif
}

void World::suspend_ai_messages(bool _suspend)
{
	aiMessagesSuspended = _suspend;
}

World::~World()
{
	scoped_call_stack_info(TXT("~world"));
	
#ifdef AN_ALLOW_BULLSHOTS
	BullshotSystem::clear_world(this);
#endif

	Game::get()->suspend_destruction(NAME(worldBeingDestroyed));

	beingDestroyed = true;

	ASSERT_SYNC_OR_ASYNC;
	suspend_ai_messages(true);

	// the rule of the thumb:
	// destroy starting with the last

#ifdef LOG_WORLD_DESTRUCTION
	output(TXT("destroying world"));
	output(TXT("  sub worlds left: %i"), subWorlds.get_size());
	output(TXT("  objects left: %i"), allObjects.get_size());
	output(TXT("  rooms left: %i"), allRooms.get_size());
	output(TXT("  environments left: %i"), allEnvironments.get_size());
	output(TXT("  doors left: %i"), allDoors.get_size());
#endif
	{
#ifdef LOG_WORLD_DESTRUCTION
		output(TXT("destroying world : objects : cease_to_exist (%i)"), allObjects.get_size()); 
#endif
		scoped_call_stack_info(TXT("objects : cease_to_exist"));
		// make all objects cease to exist first, then destruct them
		// do it reversed, so we can have the same order as below (mostly for debug purposes)
		// do it with direct array as maybe something could get changed?
		for (int i = allObjects.get_size() - 1; i >= 0; -- i)
		{
			if (i >= allObjects.get_size())
			{
				continue;
			}
			allObjects[i]->cease_to_exist(true);
		}
	}

	{
#ifdef LOG_WORLD_DESTRUCTION
		output(TXT("destroying world : objects : destruct_and_delete (%i)"), allObjects.get_size()); 
#endif
		scoped_call_stack_info(TXT("objects : destruct_and_delete"));
		while (!allObjects.is_empty())
		{
			// start from last as with remove_fast we will be reordering
			allObjects.get_last()->destruct_and_delete(); // this will properly close and delete object
		}
	}

	{
		scoped_call_stack_info(TXT("subworlds"));
		// first destroy all nested subworlds
		while (!subWorlds.is_empty())
		{
			delete (subWorlds.get_last());
		}
	}

#ifdef LOG_WORLD_DESTRUCTION
	output(TXT("destroying world (post sub world)"));
	output(TXT("  sub worlds left: %i"), subWorlds.get_size());
	output(TXT("  objects left: %i"), allObjects.get_size());
	output(TXT("  rooms left: %i"), allRooms.get_size());
	output(TXT("  environments left: %i"), allEnvironments.get_size());
	output(TXT("  doors left: %i"), allDoors.get_size());
#endif

	{
		scoped_call_stack_info(TXT("environments"));
		// and now continue with stuff at this level (if everything was added to subworld, we should not have any issue with this
		while (!allEnvironments.is_empty()) // before rooms, so they will be removed from rooms
		{
			delete (allEnvironments.get_last());
		}
	}
	{
		scoped_call_stack_info(TXT("rooms"));
		while (!allRooms.is_empty())
		{
			delete (allRooms.get_last());
		}
	}
	{
		scoped_call_stack_info(TXT("doors"));
		while (!allDoors.is_empty())
		{
			delete (allDoors.get_last());
		}
	}

	Game::get()->resume_destruction(NAME(worldBeingDestroyed));
}

void World::add(SubWorld* _subWorld)
{
	ASSERT_SYNC;
	an_assert(_subWorld->get_in_world() == nullptr, TXT("should not be in other world"));
	_subWorld->set_in_world(this);
	{
		MRSW_GUARD_WRITE_SCOPE(subWorldsGuard);
		subWorlds.push_back(_subWorld);
	}
}

void World::remove(SubWorld* _subWorld)
{
	ASSERT_SYNC_OR(beingDestroyed);
	_subWorld->set_in_world(nullptr);
	{
		MRSW_GUARD_WRITE_SCOPE(subWorldsGuard);
		subWorlds.remove_fast(_subWorld);
	}
	aiMessages.invalidate_to(_subWorld);
}

void World::add(Room* _room)
{
	ASSERT_SYNC;
	an_assert(_room->get_in_world() == nullptr, TXT("should not be in other world"));
	_room->set_in_world(this);
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

void World::activate(Room* _room)
{
	ASSERT_SYNC;
	an_assert(_room->is_world_active());
	{
		MRSW_GUARD_WRITE_SCOPE(roomsGuard);
		an_assert(!rooms.does_contain(_room));
		rooms.push_back(_room);
	}
	if (Environment* environment = fast_cast<Environment>(_room))
	{
		activate(environment);
	}
}

void World::remove(Room* _room)
{
	an_assert(!Game::get()->is_destruction_suspended() || beingDestroyed);
	ASSERT_SYNC_OR(beingDestroyed);
	_room->set_in_world(nullptr);
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
	aiMessages.invalidate_to(_room);
}

void World::add(Environment* _environment)
{
	ASSERT_SYNC;
	an_assert(_environment->get_in_world() == this, TXT("should be already added to this world"));
	{
		MRSW_GUARD_WRITE_SCOPE(allEnvironmentsGuard);
		allEnvironments.push_back(_environment);
	}
	if (_environment->is_world_active())
	{
		environments.push_back(_environment);
	}
}

void World::activate(Environment* _environment)
{
	ASSERT_SYNC;
	an_assert(_environment->is_world_active());
	{
		MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
		an_assert(!environments.does_contain(_environment));
		environments.push_back(_environment);
	}
}

void World::remove(Environment* _environment)
{
	an_assert(!Game::get()->is_destruction_suspended() || beingDestroyed);
	ASSERT_SYNC_OR(beingDestroyed);
	{
		MRSW_GUARD_WRITE_SCOPE(allEnvironmentsGuard);
		allEnvironments.remove_fast(_environment);
	}
	{
		MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
		environments.remove_fast(_environment);
	}
}

void World::add(Door* _door)
{
	ASSERT_SYNC;
	an_assert(_door->get_in_world() == nullptr, TXT("should not be in other world"));
	_door->set_in_world(this);
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

void World::activate(Door* _door)
{
	ASSERT_SYNC;
	an_assert(_door->is_world_active());
	{
		MRSW_GUARD_WRITE_SCOPE(doorsGuard);
		an_assert(!doors.does_contain(_door));
		doors.push_back(_door);
	}
}

void World::remove(Door* _door)
{
	an_assert(!Game::get()->is_destruction_suspended() || beingDestroyed);
	ASSERT_SYNC_OR(beingDestroyed);
	_door->set_in_world(nullptr);
	{
		MRSW_GUARD_WRITE_SCOPE(allDoorsGuard);
		allDoors.remove_fast(_door);
	}
	{
		MRSW_GUARD_WRITE_SCOPE(doorsGuard);
		doors.remove_fast(_door);
	}
}

void World::add(Object* _object)
{
	ASSERT_SYNC;
	an_assert(_object->get_in_world() == nullptr, TXT("should not be in other world"));
	_object->set_in_world(this);
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

void World::activate(Object* _object)
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
	if (Scenery * scenery = fast_cast<Scenery>(_object))
	{
		MRSW_GUARD_WRITE_SCOPE(sceneriesGuard);
		sceneries.push_back(scenery);
	}

	bool advanceOnce = false;
	if (auto* type = _object->get_object_type())
	{
		advanceOnce = type->should_advance_once();
	}

	if (advanceOnce)
	{
		_object->mark_advanceable_continuously(false);
		objectsToAdvanceOnce.push_back(_object);
	}
	else
	{
		_object->mark_advanceable_continuously();
		objectsToAdvance.push_back(_object);
	}
}

void World::remove(Object* _object)
{
	an_assert(!Game::get()->is_destruction_suspended() || beingDestroyed);
	ASSERT_SYNC_OR(beingDestroyed);
	_object->set_in_world(nullptr);
	{
		MRSW_GUARD_WRITE_SCOPE(allObjectsGuard);
		allObjects.remove_fast(_object);
	}
	{
		objectsToAdvance.remove_fast(_object);
		objectsToAdvanceOnce.remove_fast(_object);
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
	if (Scenery * scenery = fast_cast<Scenery>(_object))
	{
		{
			MRSW_GUARD_WRITE_SCOPE(sceneriesGuard);
			allSceneries.remove_fast(scenery);
		}
		{
			MRSW_GUARD_WRITE_SCOPE(sceneriesGuard);
			sceneries.remove_fast(scenery);
		}
	}
	aiMessages.invalidate_to(_object);
}

static inline bool should_ai__process_ai_messages(IModulesOwner const* _object)
{
	if (_object->is_advanceable())
	{
		if (auto* ai = _object->get_ai())
		{
			return ai->does_require_process_ai_messages();
		}
	}
	return false;
}

static inline bool should_ra_move__collision__update_collisions(IModulesOwner const* _object, float _deltaTime)
{
	if (!_object->is_advancement_suspended() &&
		_object->is_advanceable())
	{
		_object->update_rare_move_advance(_deltaTime);
		{
			auto* c = _object->get_collision();
			auto* m = _object->get_movement();
			if (!m || /* attached objects? */
				!m->is_immovable() ||
				m->is_immovable_but_update_collisions())
			{
				scoped_call_stack_info(TXT("should_ra_move__collision__update_collisions"));
				scoped_call_stack_info(_object->ai_get_name().to_char());
				if (c && c->should_update_collisions())
				{
					scoped_call_stack_info(TXT("passed should_update_collisions"));
					if (_object->does_require_move_advance())
					{
						scoped_call_stack_info(TXT("passed does_require_move_advance"));
						return true;
					}
				}
			}
			if (c)
			{
				c->update_collision_skipped();
				return false;
			}
		}
	}
	return false;
}

static inline bool should_ra_move__presence__update_presence(IModulesOwner const* _object, bool _forTemporaryObject)
{
	scoped_call_stack_ptr(_object);
	if (!_object->is_advancement_suspended() &&
		_object->is_advanceable())
	{
		if (_object->does_require_move_advance())
		{
			auto* m = _object->get_movement();
			if (_forTemporaryObject || (m && ! m->is_immovable()))
			{
				if (auto* p = _object->get_presence())
				{
					scoped_call_stack_ptr_info(p, TXT("presence"));
					if (p->does_require_update_presence())
					{
						return true;
					}
					else
					{
						p->quick_pre_update_presence();
						return false;
					}
				}
			}
		}
	}
	return false;
}

static inline bool should_ai__advance_perception(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended() &&
		_object->is_advanceable())
	{
		if (auto* ai = _object->get_ai())
		{
			return ai->does_require_perception_advance();
		}
	}
	return false;
}

static bool should_ai__advance_logic(IModulesOwner const* _object, float _deltaTime)
{
	if (!_object->is_advancement_suspended() &&
		_object->is_advanceable())
	{
		if (auto* ai = _object->get_ai())
		{
			ai->update_rare_logic_advance(_deltaTime);
			return ai->access_rare_logic_advance().update(_deltaTime);
		}
	}
	return false;
}

static inline bool should_ai__advance_locomotion(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended() &&
		_object->is_advanceable())
	{
		if (auto* ai = _object->get_ai())
		{
			if (auto* mind = ai->get_mind())
			{
				return mind->get_mind() && mind->get_mind()->does_use_locomotion();
			}
		}
	}
	return false;
}

static inline bool should_ra_move__movement__prepare_move(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		if (_object->does_require_move_advance())
		{
			if (auto* m = _object->get_movement())
			{
				if (!m->is_immovable() &&
					m->does_require_prepare_move())
				{
					if (auto *p = _object->get_presence())
					{
						if (!p->is_attached())
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

static inline bool should_apperance__calculate_preliminary_pose(IModulesOwner const* _object, float _deltaTime)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		_object->update_rare_pose_advance(_deltaTime); // it goes first, so we do calculations here to skip update in other steps
		return _object->does_require_pose_advance() && _object->does_appearance_require(ModuleAppearance::advance__calculate_preliminary_pose);
	}
	return false;
}

static inline bool should_apperance__advance_controllers_and_adjust_preliminary_pose(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		return _object->does_require_pose_advance() && _object->does_appearance_require(ModuleAppearance::advance__advance_controllers_and_adjust_preliminary_pose);
	}
	return false;
}

static inline bool should_apperance__calculate_final_pose_and_attachments(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		return _object->does_require_pose_advance() && _object->does_appearance_require(ModuleAppearance::advance__calculate_final_pose_and_attachments);
	}
	return false;
}

static inline bool should_customs__advance_post(IModulesOwner const* _object, float _deltaTime)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		ModuleCustom::all_customs__update_rare_advance(_object, _deltaTime);
		return ModuleCustom::all_customs__does_require(ModuleCustom::all_customs__advance_post, _object);
	}
	return false;
}

static inline bool should_ra_move__presence__prepare_move(IModulesOwner const* _object)
{
	if (_object->is_advanceable())
	{
		if (_object->get_presence()->does_request_teleport() || _object->get_presence()->has_teleported() /* teleport immovable */)
		{
			return true;
		}
		if (!_object->is_advancement_suspended() &&
			_object->does_require_move_advance())
		{
			if ((_object->get_movement() && !_object->get_movement()->is_immovable())
				|| _object->get_presence()->does_require_forced_prepare_move() /* might be forced to update controls for immovable objects controlled by the player */)
			{
				return true;
			}
		}
	}
	return false;
}

static inline bool should_ra_move__presence__do_move(IModulesOwner const* _object)
{
	if (_object->is_advanceable())
	{
		if (_object->get_presence()->does_request_teleport() || _object->get_presence()->has_teleported() /* teleport immovable */)
		{
			return true;
		}
		if (_object->does_require_move_advance())
		{
			if ((_object->get_movement() && !_object->get_movement()->is_immovable()) ||
				_object->get_presence()->get_based_on()) // some "based on" objects may not have movement module (shields) but should move with the base
			{
				if (_object->get_presence()->does_require_do_move())
				{
					return true;
				}
			}
		}
	}
	return false;
}

static inline bool should_ra_move__presence__post_move(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		if (_object->does_require_move_advance())
		{
		/*
		if (_object->get_movement()
		&& !_object->get_movement()->is_immovable())*/
			{
				if (auto* p = _object->get_presence())
				{
					if (p->does_require_post_move())
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

static inline bool should_ra_move__gameplay__post_logic(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		if (auto* g = _object->get_gameplay())
		{
			if (g->does_require(ModuleGameplay::advance__post_logic))
			{
				if (g->should_update_with_attached_to())
				{
					auto* obj = _object->get_presence()->get_attached_to();
					while (obj)
					{
						if (auto* g = obj->get_gameplay())
						{
							if (g->does_require(ModuleGameplay::advance__post_logic) &&
								g->should_update_attached())
							{
								return false; // will be handled by that object's gameplay pre move
							}
						}
						obj = obj->get_presence()->get_attached_to();
					}
				}
				return true;
			}
		}
	}
	return false;
}

static inline bool should_ra_move__gameplay__post_move(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		if (_object->does_require_move_advance())
		{
			if (auto* g = _object->get_gameplay())
			{
				if (g->does_require(ModuleGameplay::advance__post_move))
				{
					if (g->should_update_with_attached_to())
					{
						auto* obj = _object->get_presence()->get_attached_to();
						while (obj)
						{
							if (auto* g = obj->get_gameplay())
							{
								if (g->does_require(ModuleGameplay::advance__post_move) &&
									g->should_update_attached())
								{
									return false; // will be handled by that object's gameplay post move
								}
							}
							obj = obj->get_presence()->get_attached_to();
						}
					}
					return true;
				}
			}
		}
	}
	return false;
}

static inline bool should__finalise_frame(IModulesOwner const* _object)
{
	if (!_object->is_advancement_suspended()
		&& _object->is_advanceable())
	{
		if (_object->does_require_finalise_frame())
		{
			return true;
		}
	}
	return false;
}

#define scoped_do_immediate_jobs_info(_info) \
	scoped_call_stack_info(_info); \
	scoped_job_system_performance_measure_info(_info);

void World::advance_logic(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::advance_logic"));

#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors_and_objects(MultithreadCheck::OnlyRead);
	multithread_check__set__accessing_world(MultithreadCheck::OnlyRead);
#endif

	MRSW_GUARD_READ_SCOPE(_limitToSubWorld? _limitToSubWorld->objectsGuard : objectsGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->doorsGuard : doorsGuard);

#ifdef AN_DEVELOPMENT
	dev_check_if_ok(_limitToSubWorld);
#endif

	Array<Door*> & onDoors = _limitToSubWorld ? _limitToSubWorld->doors : doors;

	float deltaTime = _jobSystem->access_advance_context().get_delta_time();

	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_timeAndRhythm"));
		scoped_call_stack_info(TXT("worldAdvance_timeAndRhythm"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_timeAndRhythm, Colour::grey);

		// advance world time, time and set in shader program binding context
		worldTime += (double)deltaTime;
		time += deltaTime;
		if (timePeriod > 0.0f)
		{
			time = fmodf(time, timePeriod);
		}
		shaderProgramBindingContext.access_shader_param(NAME(time)).param.set_uniform(time);

		todo_future(TXT("implement_ proper rhythm"));
		// fake rhythm for now
		float rhythmBeatALength = 16.0f;
		float rhythmBeatA = fmodf(time * (140.0f /*bpm*/ / 60.0f), rhythmBeatALength);
		shaderProgramBindingContext.access_shader_param(NAME(rhythmBeatA)).param.set_uniform(rhythmBeatA);
		shaderProgramBindingContext.access_shader_param(NAME(rhythmBeatALength)).param.set_uniform(rhythmBeatALength);
	}

	// distribute ai messages first, process them and then release processed messages
	if (! aiMessages.is_empty())
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_aiMessages"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_aiMessages"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_aiMessages, Colour::zxBlue);
		// note that ATM it adds every message to every possible recipient - on threads we will find who may process each message
		// we also process delays before we actually send it to processing
		distribute_ai_messages(deltaTime);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAI::advance__process_ai_messages, objectsToAdvance,
			[](IModulesOwner const * _object) { return should_ai__process_ai_messages(_object); }); // process even if is_advancement_suspended as any message my wake us up
		remove_processed_ai_messages();
	}

	// update collision gradients
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD_EX(TXT("worldAdvance_prepareMove_collision"), 0.003f);
		scoped_do_immediate_jobs_info(TXT("worldAdvance_prepareMove_collision"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_prepareMove_collision, Colour::zxCyanBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleCollision::advance__update_collisions, objectsToAdvance,
			[deltaTime](IModulesOwner const* _object) { return should_ra_move__collision__update_collisions(_object, deltaTime); });
	}

	// update presence
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_updatePresence"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_updatePresence"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_updatePresence, Colour::zxYellowBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__update_presence, objectsToAdvance,
			[](IModulesOwner const* _object) { return should_ra_move__presence__update_presence(_object, false /* not temporary object */); });
	}

	// update ai perception
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_advancePerception"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_advancePerception"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_advancePerception, Colour::zxRed);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAI::advance__advance_perception, objectsToAdvance,
			[](IModulesOwner const * _object) { return should_ai__advance_perception(_object); });
	}

	// update ai logic
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_advanceLogic"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_advanceLogic"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_advanceLogic, Colour::zxBlue);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAI::advance__advance_logic, objectsToAdvance,
			[deltaTime](IModulesOwner const * _object) { return should_ai__advance_logic(_object, deltaTime); });
	}

	// gameplay post logic
	if (ModuleGameplay::does_use__advance__post_logic())
	{
		// for attached objects we should call post move using attached system
		// the main object goes first, then attached
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_gameplayPostLogic"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_gameplayPostLogic"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_gameplayPostLogic, Colour::zxRedBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleGameplay::advance__post_logic, objectsToAdvance,
			[](IModulesOwner const* _object) { return _object->get_gameplay() != nullptr && should_ra_move__gameplay__post_logic(_object); });
	}

	// advance locomotion to override_ any other input
	todo_future(TXT("maybe advance locomotion could be part of advance logic?"));
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_advanceLocomotion"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_advanceLocomotion"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_advanceLocomotion, Colour::c64Brown);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAI::advance__advance_locomotion, objectsToAdvance,
			[](IModulesOwner const * _object) { return should_ai__advance_locomotion(_object); });
	}

	// operation of doors
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_doorsOperation"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_doorsOperation"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_doorsOperation, Colour::c64Orange);
		_jobSystem->do_immediate_jobs<Door>(_workerJobList, Door::advance__operation, onDoors,
			[](Door const* _object) { return _object->does_require_advance_operation(); });
	}

	// prepare next velocity
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_prepareMove_movement"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_prepareMove_movement"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_prepareMove_movement, Colour::zxGreenBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleMovement::advance__prepare_move, objectsToAdvance,
			[](IModulesOwner const* _object) { return should_ra_move__movement__prepare_move(_object); });
	}

	// advance appearance calculate preliminary pose - this is pose from logic only - it might be required by controllers
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_calculatePreliminaryPose"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_calculatePreliminaryPose"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_calculatePreliminaryPose, Colour::c64Green);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__calculate_preliminary_pose, objectsToAdvance,
			[deltaTime](IModulesOwner const * _object) { return should_apperance__calculate_preliminary_pose(_object, deltaTime); });
	}

	// advance appearance controllers (here to allow modifying displacements)
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_appearanceControllersAndAdjustPreliminaryPose"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_appearanceControllersAndAdjustPreliminaryPose"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_appearanceControllersAndAdjustPreliminaryPose, Colour::c64LightBlue);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__advance_controllers_and_adjust_preliminary_pose, objectsToAdvance,
			[](IModulesOwner const * _object) { return should_apperance__advance_controllers_and_adjust_preliminary_pose(_object); });
	}

	// prepare for move (will finalise velocity and displacement)
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_prepareMove_presence"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_prepareMove_presence"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_prepareMove_presence, Colour::zxMagenta);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__prepare_move, objectsToAdvance,
			[](IModulesOwner const * _object) { return should_ra_move__presence__prepare_move(_object); });
	}

#ifdef AN_DEVELOPMENT
	dev_check_if_ok(_limitToSubWorld);
#endif

#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors_and_objects();
	multithread_check__set__accessing_world();
#endif
}

void World::advance_physically(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::advance_physically"));
#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors(MultithreadCheck::OnlyRead);
	multithread_check__set__accessing_rooms_objects(MultithreadCheck::AllowWrite);
	multithread_check__set__accessing_world(MultithreadCheck::OnlyRead);
#endif

	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->objectsGuard : objectsGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->actorsGuard : actorsGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->sceneriesGuard : sceneriesGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->doorsGuard : doorsGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->roomsGuard : roomsGuard);

	Array<Room*> & onRooms = _limitToSubWorld ? _limitToSubWorld->rooms : rooms;
	Array<Door*> & onDoors = _limitToSubWorld ? _limitToSubWorld->doors : doors;

	float deltaTime = _jobSystem->access_advance_context().get_delta_time();

#ifdef AN_DEVELOPMENT
	dev_check_if_ok(_limitToSubWorld);
#endif

	// move objects
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_doMove"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_doMove"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_doMove, Colour::zxRedBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__do_move, objectsToAdvance,
			[](IModulesOwner const* _object) { return should_ra_move__presence__do_move(_object); });
	}

	// post move objects
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_postMove"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_postMove"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_postMove, Colour::zxRedBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__post_move, objectsToAdvance,
			[](IModulesOwner const* _object) { return should_ra_move__presence__post_move(_object); });
	}

	// open/close doors
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_doorOpenClose"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_doorOpenClose"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_doorOpenClose, Colour::zxMagenta);
		_jobSystem->do_immediate_jobs<Door>(_workerJobList, Door::advance__physically, onDoors,
			[](Door const* _object) { return _object->does_require_advance_physically(); });
	}

	// room pois
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_pois_room"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_pois_room"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_pois_room, Colour::zxGreen);
		_jobSystem->do_immediate_jobs<Room>(_workerJobList, Room::advance__pois, onRooms,
			[](Room const * _room) { return _room->do_pois_require_advancement(); });
	}

	// object pois
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_pois_object"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_pois_object"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_pois_object, Colour::zxCyan);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__pois, objectsToAdvance,
			[](IModulesOwner const * _object) { return !_object->is_advancement_suspended() && _object->is_advanceable() && _object->do_pois_require_advancement(); });
	}

	// gameplay post move
	{
		// for attached objects we should call post move using attached system
		// the main object goes first, then attached
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_gameplayPostMove"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_gameplayPostMove"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_gameplayPostMove, Colour::zxRedBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleGameplay::advance__post_move, objectsToAdvance,
			[](IModulesOwner const* _object) { return _object->get_gameplay() != nullptr && should_ra_move__gameplay__post_move(_object); });
	}

	// advance appearance calculate (final) pose - it might be just as easy as copying preliminary pose but if there are controllers, they will do more
	// it also manages attachments
	// actually it is sort of ping pong between presence and appearance, but starts in appearance:
	//		for each object that is not attached to anything:
	//			1. calculate final pose (0)
	//			2. update all attachments (1)
	//			3. for each attachment calculate final pose (0) and update their attachments (1) and so on
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_finalPoseAndAttachments"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_finalPoseAndAttachments"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_finalPoseAndAttachments, Colour::c64LightGreen);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__calculate_final_pose_and_attachments, objectsToAdvance,
			[](IModulesOwner const* _object) { return should_apperance__calculate_final_pose_and_attachments(_object); });
	}

	// custom post
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_custom_post"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_custom_post"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_custom_post, Colour::zxMagenta);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleCustom::all_customs__advance_post, objectsToAdvance,
			[deltaTime](IModulesOwner const* _object) { return should_customs__advance_post(_object, deltaTime); });
	}

	// timers (post attachments as we may want to detach someone at the end of frame)
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_timers"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_timers"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_timers, Colour::zxYellowBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, IModulesOwner::advance__timers, objectsToAdvance,
			[](IModulesOwner const * _object) { return _object->is_advanceable() && _object->has_timers(); }); // even if suspended
	}

#ifdef AN_DEVELOPMENT
	dev_check_if_ok(_limitToSubWorld);
#endif

#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors_and_objects();
	multithread_check__set__accessing_world();
#endif
}

void World::advance_temporary_objects(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects"));
	MEASURE_PERFORMANCE_COLOURED(worldAdvance_temporaryObjects, Colour::white);

	if (_limitToSubWorld)
	{
		_limitToSubWorld->activate_temporary_objects();
		advance_temporary_objects_in(_jobSystem, _workerJobList, _limitToSubWorld);
		_limitToSubWorld->deactivate_temporary_objects();
	}
	else
	{
		for_every_ptr(subWorld, subWorlds)
		{
			subWorld->activate_temporary_objects();
			advance_temporary_objects(_jobSystem, _workerJobList, subWorld);
			subWorld->deactivate_temporary_objects();
		}
	}
}

void World::advance_temporary_objects_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects_in"));
	MRSW_GUARD_READ_SCOPE(_subWorld->temporaryObjectsActiveGuard);

	Array<TemporaryObject*> & onObjects = _subWorld->temporaryObjectsActive;

	if (onObjects.is_empty())
	{
		return;
	}

#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors_and_objects(MultithreadCheck::OnlyRead);
	multithread_check__set__accessing_world(MultithreadCheck::OnlyRead);
#endif

	float deltaTime = _jobSystem->access_advance_context().get_delta_time();

	// update collision gradients
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_prepareMove_collision"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_prepareMove_collision"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_prepareMove_collision, Colour::zxCyanBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleCollision::advance__update_collisions, onObjects,
			[deltaTime](IModulesOwner const* _object) { return should_ra_move__collision__update_collisions(_object, deltaTime); });
	}

	// update presence
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_updatePresence"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_updatePresence"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_updatePresence, Colour::zxYellowBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__update_presence, onObjects,
			[](IModulesOwner const* _object) { return should_ra_move__presence__update_presence(_object, true /* temporary object */); });
	}

	// TODO ai perception

	// update ai logic
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_advanceLogic"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_advanceLogic"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_advanceLogic, Colour::zxBlue);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAI::advance__advance_logic, onObjects,
			[deltaTime](IModulesOwner const * _object) { return should_ai__advance_logic(_object, deltaTime); });
	}

	// advance locomotion to override_ any other input
	todo_future(TXT("maybe advance locomotion could be part of advance logic?"));
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_advanceLocomotion"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_advanceLocomotion"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_advanceLocomotion, Colour::c64Brown);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAI::advance__advance_locomotion, onObjects,
			[](IModulesOwner const* _object) { return should_ai__advance_locomotion(_object); });
	}

	// prepare next velocity
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_prepareMove_movement"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_prepareMove_movement"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_prepareMove_movement, Colour::zxGreenBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleMovement::advance__prepare_move, onObjects,
			[](IModulesOwner const* _object) { return should_ra_move__movement__prepare_move(_object); });
	}

	// advance appearance calculate preliminary pose - this is pose from logic only - it might be required by controllers
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_calculatePreliminaryPose"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_calculatePreliminaryPose"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_calculatePreliminaryPose, Colour::c64Green);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__calculate_preliminary_pose, onObjects,
			[deltaTime](IModulesOwner const* _object) { return should_apperance__calculate_preliminary_pose(_object, deltaTime); });
	}

	// advance appearance controllers
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_appearanceControllersAndAdjustPreliminaryPose"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_appearanceControllersAndAdjustPreliminaryPose"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_appearanceControllersAndAdjustPreliminaryPose, Colour::c64LightBlue);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__advance_controllers_and_adjust_preliminary_pose, onObjects,
			[](IModulesOwner const* _object) { return should_apperance__advance_controllers_and_adjust_preliminary_pose(_object); });
	}

	// prepare for move
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_prepareMove_presence"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_prepareMove_presence"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_prepareMove_presence, Colour::zxMagenta);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__prepare_move, onObjects,
			[](IModulesOwner const* _object) { return should_ra_move__presence__prepare_move(_object); });
	}

	//
	//
	//	physical advancement
	//
	//

#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors(MultithreadCheck::OnlyRead);
	multithread_check__set__accessing_rooms_objects(MultithreadCheck::AllowWrite);
#endif

	// move objects
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_doMove"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_doMove"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_doMove, Colour::zxRedBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__do_move, onObjects,
			[](IModulesOwner const* _object) { return should_ra_move__presence__do_move(_object); });
	}

	// post move objects
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_postMove"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_postMove"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_postMove, Colour::zxRedBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__post_move, onObjects,
			[](IModulesOwner const* _object) { return should_ra_move__presence__post_move(_object); });
	}

	// object pois
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_pois_object"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_pois_object"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_pois_object, Colour::zxCyan);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__pois, onObjects,
			[](IModulesOwner const * _object) { return _object->is_advanceable() && _object->do_pois_require_advancement(); });
	}

	// gameplay post move
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_gameplayPostMove"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_gameplayPostMove"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_gameplayPostMove, Colour::zxRedBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleGameplay::advance__post_move, onObjects,
			[](IModulesOwner const* _object) { return _object->get_gameplay() != nullptr && should_ra_move__gameplay__post_move(_object); });
	}

	// timers
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_timers"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_timers"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_timers, Colour::zxYellowBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, IModulesOwner::advance__timers, onObjects,
			[](IModulesOwner const * _object) { return _object->is_advanceable() && _object->has_timers(); });
	}

	// final poses (attachments should not happen for temporary objects
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_finalPoseAndAttachments"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_finalPoseAndAttachments"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_finalPoseAndAttachments, Colour::c64LightGreen);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleAppearance::advance__calculate_final_pose_and_attachments, onObjects,
			[](IModulesOwner const* _object) { return should_apperance__calculate_final_pose_and_attachments(_object); });
	}

	// custom post
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] worldAdvance_custom_post"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_custom_post"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_custom_post, Colour::zxMagenta);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModuleCustom::all_customs__advance_post, onObjects,
			[deltaTime](IModulesOwner const* _object) { return should_customs__advance_post(_object, deltaTime); });
	}

#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors_and_objects();
	multithread_check__set__accessing_world();
#endif
}

void World::advance_build_presence_links(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld, BuildPresenceLinks::Flags _flags)
{
	scoped_call_stack_info(TXT("world::advance_build_presence_links"));
#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors_and_objects(MultithreadCheck::OnlyRead);
	multithread_check__set__accessing_world(MultithreadCheck::OnlyRead);
#endif

	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->objectsGuard : objectsGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->roomsGuard : roomsGuard);

	Array<Room*> & onRooms = _limitToSubWorld ? _limitToSubWorld->rooms : rooms;

	an_assert((is_flag_set(_flags, BuildPresenceLinks::Add) && !is_flag_set(_flags, BuildPresenceLinks::Clear)) ||
		   (! is_flag_set(_flags, BuildPresenceLinks::Add) && is_flag_set(_flags, BuildPresenceLinks::Clear)) ||
		   (!is_flag_set(_flags, BuildPresenceLinks::Add) && !is_flag_set(_flags, BuildPresenceLinks::Clear)), TXT("can't mix Add and Clear"));

	Room::before_building_presence_links();

	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_determineBuildPresenceLinks"));
		// determine what is required to be built
		if (is_flag_set(_flags, BuildPresenceLinks::Objects))
		{
			WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_determineBuildPresenceLinks_objects"));
			Concurrency::Counter excessPresenceLinksLeft = 30;
			for_every_ptr(o, objectsToAdvance)
			{
				o->get_presence()->update_does_require_building_presence_links(&excessPresenceLinksLeft);
			}
		}
		if (is_flag_set(_flags, BuildPresenceLinks::TemporaryObjects))
		{
			WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_determineBuildPresenceLinks_temporaryObjects"));
			advance_temporary_objects_determine_build_presence_links(_jobSystem, _workerJobList, _limitToSubWorld);
		}
	}

	// clear presence links lists in rooms
	if (!is_flag_set(_flags, BuildPresenceLinks::Add) ||
		is_flag_set(_flags, BuildPresenceLinks::Clear))
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_clearPresenceLinks"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_clearPresenceLinks"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_clearPresenceLinks, Colour::zxCyan);
		_jobSystem->do_immediate_jobs<Room>(_workerJobList, Room::advance__clear_presence_links, onRooms,
			[](Room const* _object) { return _object->does_require_clear_presence_links(); });
	}

	// build presence links
	if (is_flag_set(_flags, BuildPresenceLinks::Objects))
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_buildPresenceLinks"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_buildPresenceLinks"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_buildPresenceLinks, Colour::zxBlueBright);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__build_presence_links, objectsToAdvance,
			[](IModulesOwner const * _object) { return _object->get_presence()->does_require_building_presence_links(PresenceLinkOperation::Building); });
	}

	if (is_flag_set(_flags, BuildPresenceLinks::TemporaryObjects))
	{
		advance_temporary_objects_build_presence_links(_jobSystem, _workerJobList, _limitToSubWorld);
	}

	// join presence links lists
	if (true)
	{
		auto & onCombinedRooms = Room::get_rooms_to_combine_presence_links_post_building_presence_links();
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_combinePresenceLinks"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_combinePresenceLinks"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_combinePresenceLinks, Colour::zxGreen);
		_jobSystem->do_immediate_jobs<Room>(_workerJobList, Room::advance__combine_presence_links, onCombinedRooms);
	}
	else
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_combinePresenceLinks"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_combinePresenceLinks"));
		MEASURE_PERFORMANCE_COLOURED(worldAdvance_combinePresenceLinks, Colour::zxGreen);
		_jobSystem->do_immediate_jobs<Room>(_workerJobList, Room::advance__combine_presence_links, onRooms,
			[](Room const* _object) { return _object->does_require_combine_presence_links(); });
	}

#ifdef AN_DEVELOPMENT
	if (is_flag_set(_flags, BuildPresenceLinks::Objects))
	{
		for_every_ptr(object, objectsToAdvance)
		{
			PresenceLink::check_if_object_has_its_own_presence_links(object);
#ifdef AN_CHECK_PRESENCE_LINK_PLACEMENT_MISMATCH
			if (auto* p = object->get_presence())
			{
				int matches = 0;
				int presenceLinks = 0;
				PresenceLink const * pl = p->get_presence_links();
				while (pl)
				{
					++presenceLinks;
					if (pl->get_in_room() == p->get_in_room() &&
						Transform::same_with_orientation(pl->get_placement_in_room(), p->get_placement()))
					{
						++matches;
					}
					pl = pl->get_next_in_object();
				}
				if (matches == 0 && presenceLinks > 0)
				{
					warn_dev_investigate(TXT("object \"%S\" has no valid presence links"), object->ai_get_name().to_char());
				}
			}
#endif
		}
	}
	if (is_flag_set(_flags, BuildPresenceLinks::TemporaryObjects))
	{
		check_if_all_temporary_objects_have_their_own_presences(_limitToSubWorld);
	}
#endif

#ifdef AN_DEVELOPMENT
	multithread_check__set__accessing_rooms_doors_and_objects();
	multithread_check__set__accessing_world();
#endif
}

#ifdef AN_DEVELOPMENT
void World::check_if_all_temporary_objects_have_their_own_presences(SubWorld* _limitToSubWorld)
{
	if (_limitToSubWorld)
	{
		check_if_all_temporary_objects_have_their_own_presences_in(_limitToSubWorld);
	}
	else
	{
		for_every_ptr(subWorld, subWorlds)
		{
			check_if_all_temporary_objects_have_their_own_presences_in(subWorld);
		}
	}
}

void World::check_if_all_temporary_objects_have_their_own_presences_in(SubWorld* _subWorld)
{
	Array<TemporaryObject*>& onObjects = _subWorld->temporaryObjectsActive;

	if (onObjects.is_empty())
	{
		return;
	}

	for_every_ptr(object, onObjects)
	{
		PresenceLink::check_if_object_has_its_own_presence_links(object);
	}
}
#endif

void World::advance_temporary_objects_determine_build_presence_links(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects_determine_build_presence_links"));
	MEASURE_PERFORMANCE_COLOURED(worldAdvance_temporaryObjects_buildPresenceLinks, Colour::white);

	if (_limitToSubWorld)
	{
		advance_temporary_objects_determine_build_presence_links_in(_jobSystem, _workerJobList, _limitToSubWorld);
	}
	else
	{
		for_every_ptr(subWorld, subWorlds)
		{
			advance_temporary_objects_determine_build_presence_links_in(_jobSystem, _workerJobList, subWorld);
		}
	}
}

void World::advance_temporary_objects_determine_build_presence_links_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects_determine_build_presence_links_in"));
	MRSW_GUARD_READ_SCOPE(_subWorld->temporaryObjectsActiveGuard);

	Array<TemporaryObject*> & onObjects = _subWorld->temporaryObjectsActive;

	if (onObjects.is_empty())
	{
		return;
	}

	// build presence links
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("advance__build_presence_links"));
		scoped_do_immediate_jobs_info(TXT("advance__build_presence_links"));
		for_every_ptr(to, onObjects)
		{
			to->get_presence()->update_does_require_building_presence_links(nullptr); // no excess links
		}
	}
}

void World::advance_temporary_objects_build_presence_links(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects_build_presence_links"));
	MEASURE_PERFORMANCE_COLOURED(worldAdvance_temporaryObjects_buildPresenceLinks, Colour::white);

	if (_limitToSubWorld)
	{
		advance_temporary_objects_build_presence_links_in(_jobSystem, _workerJobList, _limitToSubWorld);
	}
	else
	{
		for_every_ptr(subWorld, subWorlds)
		{
			advance_temporary_objects_build_presence_links_in(_jobSystem, _workerJobList, subWorld);
		}
	}
}

void World::advance_temporary_objects_build_presence_links_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects_build_presence_links_in"));
	MRSW_GUARD_READ_SCOPE(_subWorld->temporaryObjectsActiveGuard);

	Array<TemporaryObject*> & onObjects = _subWorld->temporaryObjectsActive;

	if (onObjects.is_empty())
	{
		return;
	}

	// build presence links
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("advance__build_presence_links"));
		scoped_do_immediate_jobs_info(TXT("advance__build_presence_links"));
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, ModulePresence::advance__build_presence_links, onObjects,
			[](IModulesOwner const * _object) { return (_object->get_presence()->does_require_building_presence_links(PresenceLinkOperation::Building)); });
	}
}

void World::advance_finalise_frame(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("World::advance_finalise_frame"));
	MEASURE_PERFORMANCE_COLOURED(worldAdvance_finaliseFrame, Colour::zxRedBright);

	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->objectsGuard : objectsGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->roomsGuard : roomsGuard);
	MRSW_GUARD_READ_SCOPE(_limitToSubWorld ? _limitToSubWorld->doorsGuard : doorsGuard);

	Array<Room*> & onRooms = _limitToSubWorld ? _limitToSubWorld->rooms : rooms;
	Array<Door*> & onDoors = _limitToSubWorld ? _limitToSubWorld->doors : doors;

	float deltaTime = _jobSystem->access_advance_context().get_delta_time();

#ifdef AN_DEVELOPMENT
	dev_check_if_ok(_limitToSubWorld);
#endif

	// do it before as we're readying for rendering too
	{
		scoped_call_stack_info(TXT("worldAdvance_finaliseFrame_roomsVisibility"));
		MEASURE_PERFORMANCE(worldAdvance_finaliseFrame_roomsVisibility);
		Room::advance__player_related(onRooms, deltaTime);
	}

	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("worldAdvance_finaliseFrame_roomsSuspending"));
		scoped_do_immediate_jobs_info(TXT("worldAdvance_finaliseFrame_roomsSuspending"));
		{
			// we don't have to update all every frame
			MEASURE_PERFORMANCE(worldAdvance_finaliseFrame_roomsSuspending_prepareList);
			int const onRoomsAmount = onRooms.get_size();
			int const updateLimit = 30;
			int const updateMinAmount = 5; // if we have 200 rooms, we will need 200/5 frames, 40 frames. that's a bit more than half a second. really good enough in my book
			int left = min(updateLimit, onRoomsAmount);
			updateSuspendingAdvanceRooms.clear();
			updateSuspendingAdvanceRooms.make_space_for(left);
			// always add:
			// - special rooms that always suspend advancement
			// - rooms that are visible
			// we want to suspend immediately if we're supposed to due to some enforced rules
			// and we want to resume objects that are already visible
			for_every_ptr(r, onRooms)
			{
				if (r->should_always_suspend_advancement() ||
					r->was_recently_seen_by_player())
				{
					updateSuspendingAdvanceRooms.push_back(r);
				}
			}

			// now let's check how many other rooms can we add
			left = max(left - updateSuspendingAdvanceRooms.get_size(), // fill up to limit
					   updateMinAmount); // but have at least a few
			left = min(left, onRoomsAmount - updateSuspendingAdvanceRooms.get_size()); // do not exceed how many rooms do we have left actually

			if (left > 0 && onRoomsAmount > 0)
			{
				updateSuspendingAdvanceRoomsStartIdx = mod(updateSuspendingAdvanceRoomsStartIdx, onRoomsAmount);
				while (left > 0)
				{
					auto* r = onRooms[updateSuspendingAdvanceRoomsStartIdx];
					if (!updateSuspendingAdvanceRooms.does_contain(r))
					{
						updateSuspendingAdvanceRooms.push_back(r);
						--left;
					}
					updateSuspendingAdvanceRoomsStartIdx = mod(updateSuspendingAdvanceRoomsStartIdx + 1, onRoomsAmount);
				}
			}
		}
		{
			MEASURE_PERFORMANCE(worldAdvance_finaliseFrame_roomsSuspending);
			_jobSystem->do_immediate_jobs<Room>(_workerJobList, Room::advance__suspending_advancement, updateSuspendingAdvanceRooms);
		}
	}

	{
		scoped_do_immediate_jobs_info(TXT("worldAdvance_finaliseFrame_objects"));
		MEASURE_PERFORMANCE(worldAdvance_finaliseFrame_objects);
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, IModulesOwner::advance__finalise_frame, objectsToAdvance,
			[](IModulesOwner const * _object) { return should__finalise_frame(_object); });
	}

	{
		scoped_call_stack_info(TXT("worldAdvance_finaliseFrame_temporaryObjects"));
		MEASURE_PERFORMANCE(worldAdvance_finaliseFrame_temporaryObjects);
		advance_temporary_objects_finalise_frame(_jobSystem, _workerJobList, _limitToSubWorld);
	}

	{
		scoped_do_immediate_jobs_info(TXT("worldAdvance_finaliseFrame_rooms"));
		MEASURE_PERFORMANCE(worldAdvance_finaliseFrame_rooms);
		_jobSystem->do_immediate_jobs<Room>(_workerJobList, Room::advance__finalise_frame, onRooms,
			[](Room const* _object) { return _object->does_require_finalise_frame(); });
	}

	{
		scoped_do_immediate_jobs_info(TXT("worldAdvance_finaliseFrame_doors"));
		MEASURE_PERFORMANCE(worldAdvance_finaliseFrame_doors);
		_jobSystem->do_immediate_jobs<Door>(_workerJobList, Door::advance__finalise_frame, onDoors,
			[](Door const* _object) { return _object->does_require_finalise_frame(); });
	}
}

void World::advance_temporary_objects_finalise_frame(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects_finalise_frame"));
	MEASURE_PERFORMANCE_COLOURED(worldAdvance_temporaryObjects_finaliseFrame, Colour::white);

	if (_limitToSubWorld)
	{
		advance_temporary_objects_finalise_frame_in(_jobSystem, _workerJobList, _limitToSubWorld);
	}
	else
	{
		for_every_ptr(subWorld, subWorlds)
		{
			advance_temporary_objects_finalise_frame_in(_jobSystem, _workerJobList, subWorld);
		}
	}
}

void World::advance_temporary_objects_finalise_frame_in(JobSystem * _jobSystem, Name const & _workerJobList, SubWorld* _subWorld)
{
	scoped_call_stack_info(TXT("world::advance_temporary_objects_finalise_frame_in"));
	MRSW_GUARD_READ_SCOPE(_subWorld->temporaryObjectsActiveGuard);

	Array<TemporaryObject*> & onObjects = _subWorld->temporaryObjectsActive;
	
	if (onObjects.is_empty())
	{
		return;
	}

	// finalise frame
	{
		WORLD_ADVANCE_PERFORMANCE_GUARD(TXT("[to] advance__finalise_frame"));
		scoped_do_immediate_jobs_info(TXT("[to] advance__finalise_frame"));
		_jobSystem->do_immediate_jobs<IModulesOwner>(_workerJobList, IModulesOwner::advance__finalise_frame, onObjects,
			[](IModulesOwner const* _object) { return should__finalise_frame(_object); });
	}
}

AI::Message* World::create_ai_message(Name const & _name)
{
	return aiMessagesSuspended ? nullptr : aiMessages.create_new(_name);
}

void World::distribute_ai_messages(float _deltaTime)
{
	aiMessages.distribute_to_process(_deltaTime, aiProcessedMessages);
}

void World::remove_processed_ai_messages()
{
	aiProcessedMessages.clear();
}

template <class Class, class BaseClass>
bool ready_to_activate(Array<Class*> & _container, ActivationGroup* _activationGroup, Concurrency::MRSWGuard * _guard = nullptr)
{
	scoped_call_stack_info(TXT("ready_to_activate"));
	bool result = true;
	int prevSize = 0;
	ARRAY_STATIC(SafePtr<BaseClass>, toReady, 100); // ready max this amount objects, if we will require more, we will redo this
	{
		scoped_call_stack_info_str_printf(TXT("suspend destruction and gather objects to ready to activate [sync]"));
		Game::get()->perform_sync_world_job(TXT("ready to activate [suspend destruction and gather]"),
		[&_container, _guard, &result, &prevSize, &toReady, _activationGroup]()
		{
			Game::get()->suspend_destruction(NAME(readyToActivate));
				
			if (_guard) _guard->acquire_write();
			prevSize = _container.get_size();
			for_every_ptr(object, _container)
			{
				if (auto* idestroyable = fast_cast<IDestroyable>(object))
				{
					// if already destined to be destroyed, ignore/skip
					if (idestroyable->is_destroyed())
					{
						continue;
					}
				}
				if (!object->is_world_active() && !object->is_ready_to_world_activate() && object->does_belong_to(_activationGroup))
				{
					if (toReady.has_place_left())
					{
						toReady.push_back(SafePtr<BaseClass>(object));
					}
					else
					{
						result = false;
						break;
					}
				}
			}
			if (_guard) _guard->release_write();
		});
	}
	{
		scoped_call_stack_info_str_printf(TXT("do list of %i object(s)"), toReady.get_size());
		for_every_ref(tr, toReady)
		{
			auto* c = fast_cast<Class>(tr);
			scoped_call_stack_info_str_printf(TXT("do %i (count:%i) x%p [%S]"), for_everys_index(tr), toReady.get_size(), tr, c? c->get_world_object_name().to_char() : TXT("--"));
			c->ready_to_world_activate();
		}
	}
	{
		scoped_call_stack_info_str_printf(TXT("check if something new to activate and resume destruction [sync]"));
		Game::get()->perform_sync_world_job(TXT("ready to activate [check and resume destruction]"),
			[&_container, _guard, &result, prevSize]()
			{
				/* check if nothing else has appeared, if so, request ready again */
				if (_guard) _guard->acquire_write();
				result &= prevSize == _container.get_size();
				if (_guard) _guard->release_write();

				Game::get()->resume_destruction(NAME(readyToActivate));
			});
	}
	return result;
}

bool World::ready_to_activate_all_inactive(ActivationGroup* _activationGroup)
{
#ifdef AN_OUTPUT_WORLD_ACTIVATION
	output(TXT("[ActivationGroup] World::ready_to_activate_all_inactive [%i]"), _activationGroup ? _activationGroup->get_id() : NONE);
#endif
	scoped_call_stack_info(TXT("world::ready_to_activate_all_inactive"));
	MEASURE_PERFORMANCE(world_readyToActivateAllInactive);
	bool result = true;
#ifdef AN_OUTPUT_WORLD_ACTIVATION
	if (!Game::get_as<::Framework::PreviewGame>())
	{
		output_colour(0, 1, 1, 0);
		output(TXT("ready to activate all inactive..."));
		output_colour();
	}
	::System::TimeStamp ts;
#endif
#ifdef AN_DEVELOPMENT
	result &= ready_to_activate<Door, Door>(allDoors, _activationGroup, &allDoorsGuard);
	result &= ready_to_activate<Room, Room>(allRooms, _activationGroup, &allRoomsGuard);
	result &= ready_to_activate<Environment, Room>(allEnvironments, _activationGroup, &allEnvironmentsGuard);
	result &= ready_to_activate<Object, IModulesOwner>(allObjects, _activationGroup, &allObjectsGuard);
#else
	{
		scoped_call_stack_info(TXT("door"));
		result &= ready_to_activate<Door, Door>(allDoors, _activationGroup);
	}
	{
		scoped_call_stack_info(TXT("room"));
		result &= ready_to_activate<Room, Room>(allRooms, _activationGroup);
	}
	{
		scoped_call_stack_info(TXT("environment"));
		result &= ready_to_activate<Environment, Room>(allEnvironments, _activationGroup);
	}
	{
		scoped_call_stack_info(TXT("object"));
		result &= ready_to_activate<Object, IModulesOwner>(allObjects, _activationGroup);
	}
#endif
#ifdef AN_OUTPUT_WORLD_ACTIVATION
	if (!Game::get_as<::Framework::PreviewGame>())
	{
		output_colour(0, 1, 1, 0);
		output(TXT("readied to activate all inactive in %.2fs"), ts.get_time_since());
		output_colour();
	}
#endif
	return result;
}

void World::activate_all_inactive(ActivationGroup* _activationGroup)
{
	scoped_call_stack_info(TXT("activate all inactive"));
	ASSERT_SYNC;

	MEASURE_PERFORMANCE(activate_all_inactive);
#ifdef AN_OUTPUT_WORLD_ACTIVATION
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("[ActivationGroup] World::activate_all_inactive [%i]"), _activationGroup ? _activationGroup->get_id() : NONE);
#endif

#ifdef AN_OUTPUT_WORLD_ACTIVATION
	::System::TimeStamp ts;
#endif
	{
		scoped_call_stack_info(TXT("doors"));
		MEASURE_PERFORMANCE(doors);
		MRSW_GUARD_READ_SCOPE(allDoorsGuard);
		for_every_ptr(door, allDoors)
		{
			if (!door->is_world_active())
			{
				if (door->is_ready_to_world_activate() && door->does_belong_to(_activationGroup))
				{
					door->world_activate();
				}
			}
			else
			{
				door->world_activate_linked_door_in_rooms(_activationGroup); // they could be relinked
			}
		}
	}
	{
		scoped_call_stack_info(TXT("rooms"));
		MEASURE_PERFORMANCE(rooms);
		MRSW_GUARD_READ_SCOPE(allRoomsGuard);
		for_every_ptr(room, allRooms)
		{
			if (!room->is_world_active() && room->is_ready_to_world_activate() && room->does_belong_to(_activationGroup))
			{
				room->world_activate();
			}
		}
	}
	{
		scoped_call_stack_info(TXT("environments"));
		MEASURE_PERFORMANCE(environments);
		MRSW_GUARD_READ_SCOPE(allEnvironmentsGuard);
		for_every_ptr(environment, allEnvironments)
		{
			if (!environment->is_world_active() && environment->is_ready_to_world_activate() && environment->does_belong_to(_activationGroup))
			{
				environment->world_activate();
			}
		}
	}
	{
		scoped_call_stack_info(TXT("objects"));
		MEASURE_PERFORMANCE(objects);
		MRSW_GUARD_READ_SCOPE(allObjectsGuard);
		for_every_ptr(object, allObjects)
		{
			scoped_call_stack_info_str_printf(TXT("o%p"), object);
			if (!object->is_world_active() && object->is_ready_to_world_activate() && object->does_belong_to(_activationGroup))
			{
				object->world_activate();
			}
		}
	}
#ifdef AN_OUTPUT_WORLD_ACTIVATION
	if (!Game::get_as<::Framework::PreviewGame>())
	{
		output_colour(0, 1, 1, 0);
		output(TXT("activated all inactive in %.2fs"), ts.get_time_since());
		output_colour();
	}
#endif
}

#ifdef AN_DEVELOPMENT
bool World::multithread_check__reading_rooms_doors_is_safe() const
{
	return multithreadCheck.roomsDoorsState != MultithreadCheck::AllowWrite;
}

bool World::multithread_check__writing_rooms_doors_is_allowed() const
{
	return multithreadCheck.roomsDoorsState == MultithreadCheck::AllowWrite ||
		(!multithreadCheck.writingWhileUndefinedOnlyWhileInSync && multithreadCheck.roomsDoorsState == MultithreadCheck::Undefined) ||
		Framework::Game::get()->is_performing_sync_job();
}

bool World::multithread_check__reading_rooms_objects_is_safe() const
{
	return multithreadCheck.roomsObjectsState != MultithreadCheck::AllowWrite;
}

bool World::multithread_check__writing_rooms_objects_is_allowed() const
{
	return multithreadCheck.roomsObjectsState == MultithreadCheck::AllowWrite ||
		(!multithreadCheck.writingWhileUndefinedOnlyWhileInSync && multithreadCheck.roomsObjectsState == MultithreadCheck::Undefined) ||
		Framework::Game::get()->is_performing_sync_job();
}

bool World::multithread_check__reading_world_is_safe() const
{
	return multithreadCheck.worldState != MultithreadCheck::AllowWrite;
}

bool World::multithread_check__writing_world_is_allowed() const
{
	return multithreadCheck.worldState == MultithreadCheck::AllowWrite ||
		(! multithreadCheck.writingWhileUndefinedOnlyWhileInSync && multithreadCheck.worldState == MultithreadCheck::Undefined) ||
		Framework::Game::get()->is_performing_sync_job();
}
#endif

bool World::does_have_active_room(Room* _room) const
{
	ASSERT_NOT_ASYNC_OR(multithread_check__reading_world_is_safe());
	MRSW_GUARD_READ_SCOPE(roomsGuard);
	for_every_ptr(room, rooms)
	{
		if (room == _room)
		{
			return true;
		}
	}
	return false;
}

void World::for_every_object_with_id(Name const & _variable, int _id, OnFoundObject _for_every_object_with_id)
{
	ASSERT_NOT_ASYNC_OR(multithread_check__reading_world_is_safe());
	MRSW_GUARD_READ_SCOPE(allObjectsGuard);
	for_every_ptr(object, allObjects)
	{
		if (auto* var = object->get_variables().get_existing<int>(_variable))
		{
			if (*var == _id)
			{
				if (_for_every_object_with_id(object))
				{
					return;
				}
			}
		}
	}
}

void World::ready_to_advance(SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::ready_to_advance"));
	MEASURE_PERFORMANCE_COLOURED(worldAdvance_readyToAdvance, Colour::grey);

	// remove from the list only when they got their presence link
	for (int idx = 0; idx < objectsToAdvanceOnce.get_size(); ++idx)
	{
		auto* ao = objectsToAdvanceOnce[idx];

		{
			if (auto* p = ao->get_presence())
			{
				if (p->get_presence_links())
				{
					objectsToAdvanceOnce.remove_fast_at(idx);
					--idx;
					continue;
				}
			}
		}

		objectsToAdvance.push_back(ao);
		objectsAdvancingOnce.push_back(ao);
	}

	advancedObjectsLastFrame = objectsToAdvance.get_size();

	ADDITIONAL_PERFORMANCE_INFO(TXT("10 world advance 0 advanced"),        String::printf(TXT("objects advanced last frame : %6i"), advancedObjectsLastFrame));
	ADDITIONAL_PERFORMANCE_INFO(TXT("10 world advance 1 to advance"),      String::printf(TXT("objects to advance          : %6i"), objectsToAdvance.get_size()));
	ADDITIONAL_PERFORMANCE_INFO(TXT("10 world advance 2 to advance once"), String::printf(TXT("objects to advance once     : %6i"), objectsToAdvanceOnce.get_size()));
}

void World::finalise_advance(SubWorld* _limitToSubWorld)
{
	scoped_call_stack_info(TXT("world::finalise_advance"));
	MEASURE_PERFORMANCE_COLOURED(worldAdvance_finaliseAdvance, Colour::grey);

	for_every_ptr(ao, objectsAdvancingOnce)
	{
		objectsToAdvance.remove_fast(ao);
	}
	objectsAdvancingOnce.clear();
}

void World::log(LogInfoContext & _log)
{
	Colour headerColour = Colour::cyan;
	{
		_log.set_colour(headerColour);
		_log.log(TXT("regions"));
		_log.set_colour();
		LOG_INDENT(_log);
		for_every_ptr(subWorld, subWorlds)
		{
			for_every_ptr(region, subWorld->get_regions())
			{
				_log.log(TXT("%S"), region->get_region_type() ? region->get_region_type()->get_name().to_string().to_char() : TXT("??"));
			}
		}
	}
	_log.log(TXT(""));
	{
		_log.set_colour(headerColour);
		_log.log(TXT("rooms"));
		_log.set_colour();
		LOG_INDENT(_log);
		MRSW_GUARD_READ_SCOPE(allRoomsGuard);
		for_every_ptr(room, allRooms)
		{
			_log.log(TXT("[%c] %S \"%S\""), room->is_world_active() ? 'A' : ' ', room->get_room_type() ? room->get_room_type()->get_name().to_string().to_char() : TXT("??"), room->get_name().to_char());
		}
	}
	_log.log(TXT(""));
}

#ifdef AN_DEVELOPMENT
void World::dev_check_if_ok(SubWorld* _limitToSubWorld) const
{
	Array<Room*> const & onRooms = _limitToSubWorld ? _limitToSubWorld->rooms : rooms;

	for_every_ptr(r, onRooms)
	{
		FOR_EVERY_DOOR_IN_ROOM(door, r)
		{
			an_assert(!door->has_world_active_room_on_other_side() || door->get_other_room_transform().is_ok(), TXT("other room transform is not ok dr'%p"), door);
		}
	}
}
#endif
