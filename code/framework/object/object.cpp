#include "object.h"

#include "..\world\world.h"
#include "..\module\modules.h"

#include "..\..\core\wheresMyPoint\wmp_context.h"

//

#ifdef AN_OUTPUT_WORLD_GENERATION
	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		#define OUTPUT_GENERATION
	#endif
#endif
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define INSPECT_REMOVE_FROM_WORLD
	#endif
#endif

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifndef BUILD_PUBLIC_RELEASE
		#ifndef DEBUG_WORLD_LIFETIME
			#define DEBUG_WORLD_LIFETIME
		#endif
	#endif
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Object);

Object::Object(ObjectType const * _objectType, String const & _name)
: objectType(_objectType)
, initialised(false)
, name(_name)
{
	scoped_call_stack_info(TXT("Object::Object"));
	an_assert(!_objectType->does_require_further_loading());
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Object [new] o%p \"%S\""), this, _name.to_char());
#endif
#ifdef OUTPUT_GENERATION
	output(TXT("new object \"%S\" of type \"%S\""), _name.to_char(), _objectType? _objectType->get_name().to_string().to_char() : TXT("--"));
#endif
	make_safe_object_available(this);

	an_assert(_objectType);
	tags.base_on(&objectType->get_tags());
	individualityFactor = objectType->get_individuality_factor();

	variables.set_missing_from(_objectType->get_custom_parameters());

	// all gameplay objects have to have individual random generator set
	access_individual_random_generator().be_zero_seed();

	/*	TODO
		set_in_adventure_with_id(_inAdventure, out id);
		inAdventure.objects.add(this);
		//
		libraryCacheInternal = new LibraryCache(Library.current, this);
	*/

	advancementSuspendable = _objectType->is_advancement_suspendable();
}

Object::~Object()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Object [delete] o%p"), this);
#endif
	make_safe_object_unavailable();
	close_modules();
	an_assert(!inSubWorld, TXT("first call on_destruct()"));
	an_assert(!get_in_world(), TXT("first call on_destruct()"));
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Object [deleted] o%p"), this);
#endif
}

void Object::on_destruct()
{
	make_safe_object_unavailable();
	IModulesOwner::on_destruct();
	if (inSubWorld)
	{
		inSubWorld->remove(this);
		an_assert(!inSubWorld);
	}
	if (auto* inWorld = get_in_world())
	{
		inWorld->remove(this);
		an_assert(!get_in_world());
	}
}

void Object::make_safe_object_available(Object* _object)
{
	IModulesOwner::make_safe_object_available(_object);
	SafeObject<Collision::ICollidableObject>::make_safe_object_available(_object);
}

void Object::make_safe_object_unavailable()
{
	IModulesOwner::make_safe_object_unavailable();
	SafeObject<Collision::ICollidableObject>::make_safe_object_unavailable();
}

Transform Object::ai_get_placement() const
{
	return presence ? presence->get_placement().to_world(presence->get_centre_of_presence_os()) : Transform::identity;
}

Room* Object::ai_get_room() const
{
	return presence ? presence->get_in_room() : nullptr;
}

void Object::init(SubWorld* _subWorld)
{
	an_assert(_subWorld && _subWorld->get_in_world());
	_subWorld->get_in_world()->add(this);
	_subWorld->add(this);

	initialised = true;

	IModulesOwner::init(_subWorld);
}

void Object::learn_from(SimpleVariableStorage & _parameters)
{
	if (auto * ai = get_ai())
	{
		ai->learn_from(_parameters);
	}
}

void Object::initialise_modules(SetupModule _setup_module_pre)
{
	WheresMyPoint::Context context(this);
	context.set_random_generator(get_individual_random_generator());
	objectType->get_wheres_my_point_processor_on_init().update(context);

	if (ai == nullptr)
	{
		ai = Modules::ai.create(this, objectType->get_ai(), _setup_module_pre);
	}
	{
		while (appearances.get_size() < objectType->get_appearances().get_size())
		{
			appearances.push_back(nullptr);
		}
		int idx = 0;
		for_every(appearance, appearances)
		{
			if (*appearance == nullptr)
			{
				an_assert(objectType->get_appearances().is_index_valid(idx));
				*appearance = Modules::appearance.create(this, &objectType->get_appearances()[idx], _setup_module_pre);
			}
			++idx;
		}
		if (!appearances.is_empty())
		{
			appearances[0]->be_main_appearance();
		}
	}
	if (collision == nullptr)
	{
		collision = Modules::collision.create(this, objectType->get_collision(), _setup_module_pre);
	}
	if (controller == nullptr)
	{
		controller = Modules::controller.create(this, objectType->get_controller(), _setup_module_pre);
	}
	{
		while (customs.get_size() < objectType->get_customs().get_size())
		{
			customs.push_back(nullptr);
		}
		int idx = 0;
		for_every(custom, customs)
		{
			if (*custom == nullptr)
			{
				an_assert(objectType->get_customs().is_index_valid(idx));
				*custom = Modules::custom.create(this, &objectType->get_customs()[idx], _setup_module_pre);
			}
			++idx;
		}
	}
	if (gameplay == nullptr)
	{
		gameplay = Modules::gameplay.create(this, objectType->get_gameplay(), _setup_module_pre);
	}
	{
		while (movements.get_size() < objectType->get_movements().get_size())
		{
			movements.push_back(nullptr);
		}
		int idx = 0;
		for_every(movement, movements)
		{
			if (*movement == nullptr)
			{
				an_assert(objectType->get_movements().is_index_valid(idx));
				*movement = Modules::movement.create(this, &objectType->get_movements()[idx], _setup_module_pre);
			}
			++idx;
		}
	}
	if (navElement == nullptr)
	{
		navElement = Modules::navElement.create(this, objectType->get_nav_element(), _setup_module_pre);
	}
	if (presence == nullptr)
	{
		presence = Modules::presence.create(this, objectType->get_presence(), _setup_module_pre);
	}
	if (sound == nullptr)
	{
		sound = Modules::sound.create(this, objectType->get_sound(), _setup_module_pre);
	}
	if (temporaryObjects == nullptr)
	{
		if (auto* mtod = fast_cast<ModuleTemporaryObjectsData>(objectType->get_temporary_objects()->get_data()))
		{
			if (mtod->should_exist() || (gameplay && gameplay->does_require_temporary_objects()))
			{
				temporaryObjects = Modules::temporaryObjects.create(this, objectType->get_temporary_objects(), _setup_module_pre);
			}
		}
	}

	IModulesOwner::initialise_modules(_setup_module_pre);

	learn_from(access_variables());

	update_appearance_cached();
}

bool Object::does_handle_ai_message(Name const& _messageName) const
{
	return ai ? ai->does_handle_ai_message(_messageName) : false;
}

AI::MessagesToProcess * Object::access_ai_messages_to_process()
{
	return ai ? ai->access_ai_messages_to_process() : nullptr;
}

void Object::ready_to_world_activate()
{
	if (is_destroyed() || is_ready_to_world_activate())
	{
		return;
	}
	WorldObject::ready_to_world_activate();
	IModulesOwner::ready_modules_to_activate();
}

void Object::world_activate()
{
	Concurrency::ScopedSpinLock lock(worldActivationLock);

	WorldObject::world_activate();
	if (auto* inWorld = get_in_world())
	{
		inWorld->activate(this);
	}
	if (inSubWorld)
	{
		inSubWorld->activate(this);
	}

	IModulesOwner::activate_modules();

	for_every_ptr(temporaryObject, temporaryObjectsToActivate)
	{
		temporaryObject->mark_to_activate();
	}
	temporaryObjectsToActivate.clear();
}

void Object::remove_destroyable_from_world()
{
	scoped_call_stack_info(TXT("remove destroyable from world"));
	scoped_call_stack_info(ai_get_name().to_char());
#ifdef INSPECT_REMOVE_FROM_WORLD
	output(TXT("remove_destroyable_from_world o%p \"%S\""), this, get_name().to_char());
#endif

	if (ModulePresence* presence = get_presence())
	{
		presence->detach_attached();
		presence->detach();
#ifdef INSPECT_REMOVE_FROM_WORLD
		output(TXT("remove from room"));
		output(TXT("       from room %S"), presence->get_in_room()? presence->get_in_room()->get_name().to_char() : TXT("--"));
#endif
		presence->remove_from_room(Names::destroyed);

		//

#ifdef AN_DEVELOPMENT
		{
			LogInfoContext log;
			log.log(TXT("invalidate presence links on destruction of \"%S\""), get_name().to_char());
			presence->log_presence_links(log);
			log.output_to_output();
		}
#endif
		presence->invalidate_presence_links();
	}
}

void Object::destruct_and_delete()
{
	on_destruct();
	IDestroyable::destruct_and_delete();
}

void Object::on_destroy(bool _removeFromWorldImmediately)
{
	IDestroyable::on_destroy(_removeFromWorldImmediately);
	IModulesOwner::on_destroy();

	make_safe_object_unavailable();

#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Object on_destroy o%p"), this);
#endif
	// no longer advanceable
	mark_no_longer_advanceable();

	for_every_ptr(temporaryObject, temporaryObjectsToActivate)
	{
		temporaryObject->mark_to_deactivate(true);
	}
	temporaryObjectsToActivate.clear();
	// desire to deactivate all attached temporary objects
	// it will stop to emit but will still allow existing particles to finish whatever they were doing
	if (ModulePresence* presence = get_presence())
	{
		Concurrency::ScopedSpinLock lock(presence->access_attached_lock());
		for_every_ptr(attached, presence->get_attached())
		{
			if (auto * to = fast_cast<Framework::TemporaryObject>(attached))
			{
				to->desire_to_deactivate();
			}
		}
	}

	if (_removeFromWorldImmediately)
	{
		remove_destroyable_from_world();
	}

	if (ModulePresence * presence = get_presence())
	{
		presence->invalidate_presence_links();
	}
}

void Object::mark_to_activate(TemporaryObject* _temporaryObject)
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
