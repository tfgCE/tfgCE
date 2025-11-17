#include "temporaryObject.h"

#include "..\framework.h"
#include "..\appearance\controllers\ac_particlesUtils.h"
#include "..\module\modules.h"
#include "..\module\custom\mc_lightSources.h"
#include "..\modulesOwner\modulesOwnerImpl.inl"
#include "..\world\world.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define DEBUG_TEMPORARY_OBJECT_PLACEMENT
	#endif
#endif

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifdef DEBUG_WORLD_LIFETIME
		#define DEBUG_WORLD_LIFETIME_TEMPORARY_OBJECT_STATES
	#endif
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(temporaryObject);

//

REGISTER_FOR_FAST_CAST(TemporaryObject);

Name const & TemporaryObject::get_object_type_name() const
{
	return NAME(temporaryObject);
}

TemporaryObject::TemporaryObject(TemporaryObjectType const * _objectType)
: objectType(_objectType)
, initialised(false)
{
	SET_EXTRA_DEBUG_INFO(performOnActivate, TXT("TemporaryObject.performOnActivate"));

	an_assert(_objectType);
	tags.base_on(&objectType->get_tags());

	name = _objectType->get_name().to_string();

	variables.set_from(objectType->get_custom_parameters());

	advancementSuspendable = false;
}

TemporaryObject::~TemporaryObject()
{
	an_assert(!is_active(), TXT("deactivate first"));
	close_modules();
	an_assert(!inSubWorld, TXT("first call close()"));
}

void TemporaryObject::make_safe_object_available(TemporaryObject* _object)
{
	IModulesOwner::make_safe_object_available(_object);
	SafeObject<Collision::ICollidableObject>::make_safe_object_available(_object);
}

void TemporaryObject::make_safe_object_unavailable()
{
	IModulesOwner::make_safe_object_unavailable();
	SafeObject<Collision::ICollidableObject>::make_safe_object_unavailable();
}

void TemporaryObject::on_destruct()
{
	set_state_internal(TemporaryObjectState::NotAdded);
	if (inSubWorld)
	{
		inSubWorld->remove(this);
		an_assert(!inSubWorld);
	}
}

void TemporaryObject::init(SubWorld* _subWorld)
{
	an_assert(!inSubWorld);
	an_assert(_subWorld && _subWorld->get_in_world());
	_subWorld->add(this);

	initialised = true;
	requiresReset = true;
#ifdef WITH_TEMPORARY_OBJECT_HISTORY
	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String(TXT("init -> requiresReset = true;")));
	}
#endif
	base::init(_subWorld);
	reset(); // will call initialise modules and rest will also ready modules for activation
}

#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
void TemporaryObject::store_history(String const& _history)
{
	Concurrency::ScopedSpinLock lock(toHistoryLock);
	toHistory.push_back(_history);
}
#endif

void TemporaryObject::reset()
{
	if (!requiresReset)
	{
#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
		++numberOfTimesModulesResetUnnecessary;
#endif
		return;
	}

	// make it clean again
	variables.clear();
	if (objectType)
	{
		variables.set_from(objectType->get_custom_parameters());
	}

	base::reset();

	requiresReset = false;

#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String(TXT("reset -> requiresReset = false;")));
	}
	++numberOfTimesModulesReset;
#endif
}

void TemporaryObject::initialise_modules(SetupModule _setup_module_pre)
{
	requiresReset = false;
#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String(TXT("initialise_modules -> requiresReset = false;")));
	}
#endif

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
	if (presence == nullptr)
	{
		presence = Modules::presence.create(this, objectType->get_presence(), _setup_module_pre);
		if (presence)
		{
			presence->set_create_ai_messages(false);
		}
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

	base::initialise_modules(_setup_module_pre);

	update_appearance_cached();
}

void TemporaryObject::learn_from(SimpleVariableStorage & _parameters)
{
	if (auto * ai = get_ai())
	{
		ai->learn_from(_parameters);
	}
}

void TemporaryObject::mark_to_activate()
{
	an_assert(inSubWorld);
	an_assert(state == TemporaryObjectState::Inactive);
	set_state(TemporaryObjectState::ToBeActivated);
	inSubWorld->add_to_activate(this);
	mark_advanceable();
	make_safe_object_available(this);
}

void TemporaryObject::mark_to_deactivate(bool _removeFromWorldImmediately)
{
	an_assert(inSubWorld);
	an_assert_info(state == TemporaryObjectState::Active || state == TemporaryObjectState::Inactive ||
		 state == TemporaryObjectState::ToBeActivated ||
		(state == TemporaryObjectState::ToBeDeactivated && _removeFromWorldImmediately && get_presence() && get_presence()->get_in_room()), // we allow call mark_to_deactivate, when we want to remove from world immediately but just a moment ago we just wanted to deactivate
		TXT("this should be still okay?"));
	make_safe_object_unavailable();
	// why did you try to do it in other state?
	if (state == TemporaryObjectState::Active || state == TemporaryObjectState::Inactive || state == TemporaryObjectState::ToBeActivated)
	{
		// no longer advanceable
#ifdef DEBUG_WORLD_LIFETIME_TEMPORARY_OBJECT_STATES
		if (!Framework::is_preview_game())
		{
			output(TXT("TemporaryObject - mark o%p to deactivate (was %S)"), this, TemporaryObjectState::to_char(state));
		}
#endif
		mark_no_longer_advanceable();
		set_state(TemporaryObjectState::ToBeDeactivated);
		inSubWorld->add_to_deactivate(this);
	}

	if (_removeFromWorldImmediately)
	{
		if (ModulePresence * presence = get_presence())
		{
			if (presence->get_in_room())
			{
				presence->detach_attached();
				presence->detach();
				presence->remove_from_room();
				// it's unsafe to invalidate presence links here, it will be done when projectiles are advanced/deactivated, try to scale them down before deactivating maybe?
			}
		}
	}
}

void TemporaryObject::desire_to_deactivate()
{
	if (state == TemporaryObjectState::Active ||
		state == TemporaryObjectState::ToBeDeactivated)
	{
		Framework::ParticlesUtils::desire_to_deactivate(this);
	}
	else if (state == TemporaryObjectState::ToBeActivated ||
			 state == TemporaryObjectState::ToBeDeactivated)
	{
		mark_to_deactivate(false);
	}
	else
	{
		an_assert_immediate(false, TXT("we shouldn't call desire to deactivate in any other state"));
	}
}

void TemporaryObject::set_state_internal(TemporaryObjectState::Type _state)
{
#ifdef DEBUG_WORLD_LIFETIME_TEMPORARY_OBJECT_STATES
	if (!Framework::is_preview_game())
	{
		output(TXT("TemporaryObject - o%p set state %S->%S"), this, TemporaryObjectState::to_char(state), TemporaryObjectState::to_char(_state));
	}
#endif
	if (subWorldEntry)
	{
		bool wasAvailableForSpawn = TemporaryObjectState::is_available_for_spawn(state);
		bool isAvailableForSpawn = TemporaryObjectState::is_available_for_spawn(_state);

		if (wasAvailableForSpawn ^ isAvailableForSpawn)
		{
			an_assert(subWorldEntry->am_I_myself());

			if (isAvailableForSpawn)
			{
				subWorldEntry->active.decrease();
			}
			else
			{
				subWorldEntry->active.increase();
			}
		}
	}
#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
	if (state != _state)
	{
		if (state == TemporaryObjectState::Active)
		{
			{
				Concurrency::ScopedSpinLock lock(toHistoryLock);
				toHistory.push_back(String(TXT("activated")));
			}
			++numberOfTimesActivated;
		}
	}
	else if (_state == TemporaryObjectState::Active)
	{
		an_assert_immediate(false, TXT("activating active"));
	}
	stateHistory.push_back(state);
	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String::printf(TXT("changed state to %S"),
			state == TemporaryObjectState::NotAdded? TXT("not added") : (
			state == TemporaryObjectState::BeingInitialised ? TXT("being initialised") : (
			state == TemporaryObjectState::Inactive ? TXT("inactive") : (
			state == TemporaryObjectState::ToBeActivated ? TXT("to be activated") : (
			state == TemporaryObjectState::Active ? TXT("active") : (
			state == TemporaryObjectState::ToBeDeactivated ? TXT("to be deactivated") : TXT("??"))))))));
	}
#endif
	state = _state;

	mark_desired_to_deactivate(state == TemporaryObjectState::ToBeDeactivated);
}

void TemporaryObject::set_state(TemporaryObjectState::Type _state)
{
	if (_state == state)
	{
		return;
	}

	auto prevState = state;

	set_state_internal(_state);

	// change state here as modules may expect state already set
	if (_state == TemporaryObjectState::Active)
	{
		on_activate();
	}
	if (_state == TemporaryObjectState::Inactive)
	{
		if (prevState >= TemporaryObjectState::ToBeActivated)
		{
			on_deactivate();
		}
	}
}

#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
void TemporaryObject::ready_modules_to_activate()
{
	an_assert(state == TemporaryObjectState::Active, TXT("should be set as active"));

	base::ready_modules_to_activate();

	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String(TXT("ready_modules_to_activate")));
	}
	++numberOfTimesModulesReadiedToActivate;
}

void TemporaryObject::activate_modules()
{
	an_assert(state == TemporaryObjectState::Active, TXT("should be set as active"));

	base::activate_modules();

	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String(TXT("activate_modules")));
	}
	++numberOfTimesModulesActivated;
}
#endif

void TemporaryObject::on_activate()
{
	if (auto* appearance = get_appearance())
	{
		appearance->use_another_mesh();
	}
	if (presence)
	{
		Transform intoRoomTransform = Transform::identity;
		bool placed = false;
		if (onActivationParams.attachTo.is_set() &&
			onActivationParams.attachTo->get_presence() &&
			onActivationParams.attachTo->get_presence()->get_in_room())
		{
			if (onActivationParams.boneName.is_valid())
			{
				// we call post attach do movement manually here
				presence->attach_to_bone(onActivationParams.attachTo.get(), onActivationParams.boneName, onActivationParams.attachToFollowing, onActivationParams.relativePlacement, false);
				// if we have zero scaled bones, deal with it
				presence->make_attachment_valid_on_zero_scaled_bones();
				presence->do_post_attach_to_movement();
				placed = true;
			}
			else if (onActivationParams.socketName.is_valid())
			{
				presence->attach_to_socket(onActivationParams.attachTo.get(), onActivationParams.socketName, onActivationParams.attachToFollowing, onActivationParams.relativePlacement);
				placed = true;
			}
			else
			{
				presence->attach_to(onActivationParams.attachTo.get(), onActivationParams.attachToFollowing, onActivationParams.relativePlacement);
				placed = true;
			}
			todo_future(TXT("update into room transform?"));
		}
		else if (onActivationParams.placeAt.get())
		{
#ifdef DEBUG_TEMPORARY_OBJECT_PLACEMENT
			output(TXT("place to'%p at a scoket"), this);
#endif
			intoRoomTransform = presence->place_at(onActivationParams.placeAt.get(), onActivationParams.socketName, onActivationParams.relativePlacement, onActivationParams.addAbsoluteLocationOffset);
			placed = true;
		}
		else if (onActivationParams.placeIn.get())
		{
			Transform placeAt = onActivationParams.placement;
			placeAt.set_translation(placeAt.get_translation() + onActivationParams.addAbsoluteLocationOffset.get(Vector3::zero));
			presence->place_in_room(onActivationParams.placeIn.get(), placeAt);
			placed = true;
		}
		if (!placed &&
			onActivationParams.failSafePlaceIn.get())
		{
			presence->place_in_room(onActivationParams.failSafePlaceIn.get(), onActivationParams.failSafePlacement);
			placed = true;
		}
		// face towards
		if (onActivationParams.faceTowardsInRoomLocWS.is_set())
		{
			if (placed)
			{
				Vector3 at = onActivationParams.faceTowardsInRoomLocWS.get();
				Vector3 loc = presence->get_placement().get_translation();
				Vector3 fwd = (at - loc).normal();
				Transform newPlacement;
				if (onActivationParams.faceTowardsUpDirWS.is_set())
				{
					Vector3 up = onActivationParams.faceTowardsUpDirWS.get().normal();
					fwd = fwd.drop_using_normalised(up);
					newPlacement = look_matrix(loc, fwd, up).to_transform();
				}
				else
				{
					newPlacement = look_matrix_no_up(loc, fwd).to_transform();
				}
				presence->place_within_room(newPlacement);
			}
			else
			{
				error(TXT("can't face towards if was not placed"));
			}
		}
		// velocity
		if (onActivationParams.absoluteVelocity.is_set())
		{
			presence->set_velocity_linear(intoRoomTransform.vector_to_local(onActivationParams.absoluteVelocity.get() + onActivationParams.addAbsoluteVelocity.get(Vector3::zero)));
		}
		else if (onActivationParams.relativeVelocity.is_set())
		{
			presence->set_velocity_linear(presence->get_placement().vector_to_world(onActivationParams.relativeVelocity.get()) + onActivationParams.addAbsoluteVelocity.get(Vector3::zero));
		}
		// velocity rotation
		if (onActivationParams.velocityRotation.is_set())
		{
			presence->set_velocity_rotation(onActivationParams.velocityRotation.get());
		}
	}

	ready_modules_to_activate();

	particlesActive = 0;
	activate_modules();
	
	for_every(poa, performOnActivate)
	{
		(*poa)(this);
	}
	performOnActivate.clear();

#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String(TXT("on_activated")));
	}
	++numberOfTimesOnActivated;
#endif
}

void TemporaryObject::perform_on_activate(std::function<void(TemporaryObject* _to)> _performOnActivate)
{
	performOnActivate.push_back(_performOnActivate);
}

void TemporaryObject::on_deactivate()
{
	onActivationParams = OnActivationParams(); // clear for next use
	get_presence()->detach();
	get_presence()->remove_from_room();
	get_presence()->invalidate_presence_links();

	// this is called only on the actual deactivate
	requiresReset = true;
#ifdef STORE_TEMPORARY_OBJECTS_HISTORY
	{
		Concurrency::ScopedSpinLock lock(toHistoryLock);
		toHistory.push_back(String(TXT("on_deactivate -> requiresReset = true;")));
	}
#endif
}

bool TemporaryObject::does_handle_ai_message(Name const& _messageName) const
{
	return ai ? ai->does_handle_ai_message(_messageName) : false;
}

AI::MessagesToProcess * TemporaryObject::access_ai_messages_to_process()
{
	return ai ? ai->access_ai_messages_to_process() : nullptr;
}

Transform TemporaryObject::ai_get_placement() const
{
	return presence ? presence->get_placement().to_world(presence->get_centre_of_presence_os()) : Transform::identity;
}

Room* TemporaryObject::ai_get_room() const
{
	return presence ? presence->get_in_room() : nullptr;
}

void TemporaryObject::on_activate_place_in(Room* _room, Transform const & _placement)
{
	onActivationParams = OnActivationParams();
	onActivationParams.placeIn = _room;
	onActivationParams.placement = _placement;
}

void TemporaryObject::on_activate_place_in_fail_safe(Room* _room, Transform const & _placement)
{
	onActivationParams.failSafePlaceIn = _room;
	onActivationParams.failSafePlacement = _placement;
}

void TemporaryObject::on_activate_place_at(IModulesOwner* _object, Name const & _socketName, Transform const & _relativePlacement)
{
	// because object may disappear, update placement immediately
	if (_object &&
		get_presence())
	{
		Framework::Room* inRoom;
		Transform placement;
		if (get_presence()->get_placement_to_place_at(_object, _socketName, _relativePlacement, OUT_ inRoom, OUT_ placement))
		{
			onActivationParams.failSafePlaceIn = inRoom;
			onActivationParams.failSafePlacement = placement;
		}
	}
	onActivationParams.placeAt = _object;
	onActivationParams.socketName = _socketName;
	onActivationParams.relativePlacement = _relativePlacement;
	an_assert_immediate(onActivationParams.relativePlacement.is_ok());
}

void TemporaryObject::on_activate_face_towards(Vector3 const& _inRoomLocWS, Optional<Vector3> const& _upDirWS)
{
	onActivationParams.faceTowardsInRoomLocWS = _inRoomLocWS;
	onActivationParams.faceTowardsUpDirWS = _upDirWS;
}

void TemporaryObject::on_activate_set_velocity(Vector3 const & _velocity)
{
	onActivationParams.absoluteVelocity = _velocity;
}

void TemporaryObject::on_activate_add_velocity(Vector3 const& _velocity)
{
	if (! onActivationParams.addAbsoluteVelocity.is_set())
	{
		onActivationParams.addAbsoluteVelocity = Vector3::zero;
	}
	onActivationParams.addAbsoluteVelocity = onActivationParams.addAbsoluteVelocity.get() + _velocity;
}

void TemporaryObject::on_activate_add_absolute_location_offset(Vector3 const& _locationOffset)
{
	if (! onActivationParams.addAbsoluteLocationOffset.is_set())
	{
		onActivationParams.addAbsoluteLocationOffset = Vector3::zero;
	}
	onActivationParams.addAbsoluteLocationOffset = onActivationParams.addAbsoluteLocationOffset.get() + _locationOffset;
}

void TemporaryObject::on_activate_set_relative_velocity(Vector3 const & _velocity)
{
	onActivationParams.relativeVelocity = _velocity;
}

void TemporaryObject::on_activate_set_velocity_rotation(Rotator3 const & _velocity)
{
	onActivationParams.velocityRotation = _velocity;
}

void TemporaryObject::on_activate_attach_to(IModulesOwner* _object, bool _following, Transform const & _relativePlacement)
{
	onActivationParams.attachTo = _object;
	onActivationParams.attachToFollowing = _following;
	onActivationParams.relativePlacement = _relativePlacement;
	an_assert_immediate(onActivationParams.relativePlacement.is_ok());
	onActivationParams.boneName = Name::invalid();
	onActivationParams.socketName = Name::invalid();
}

void TemporaryObject::on_activate_attach_to_bone(IModulesOwner* _object, Name const & _bone, bool _following, Transform const & _relativePlacement)
{
	onActivationParams.attachTo = _object;
	onActivationParams.attachToFollowing = _following;
	onActivationParams.relativePlacement = _relativePlacement;
	an_assert_immediate(onActivationParams.relativePlacement.is_ok());
	onActivationParams.boneName = _bone;
	onActivationParams.socketName = Name::invalid();
}

void TemporaryObject::on_activate_attach_to_socket(IModulesOwner* _object, Name const & _socket, bool _following, Transform const & _relativePlacement)
{
	onActivationParams.attachTo = _object;
	onActivationParams.attachToFollowing = _following;
	onActivationParams.relativePlacement = _relativePlacement;
	an_assert_immediate(onActivationParams.relativePlacement.is_ok());
	onActivationParams.boneName = Name::invalid();
	onActivationParams.socketName = _socket;
}

void TemporaryObject::particles_activated()
{
	++particlesActive;
}

void TemporaryObject::particles_deactivated()
{
	--particlesActive;
	if (should_cease_to_exist())
	{
		cease_to_exist(false);
	}
	else
	{
		if (objectType->has_particles_based_existence() &&
			particlesActive == 0)
		{
			desire_to_deactivate();
		}
	}
}

void TemporaryObject::no_more_sounds()
{
	if (should_cease_to_exist())
	{
		cease_to_exist(false);
	}
}

bool TemporaryObject::should_do_calculate_final_pose_attachment_with(IModulesOwner const * _imo) const
{
	//return !fast_cast<Object>(_imo);
	return !_imo || ! _imo->get_as_object();
}

bool TemporaryObject::should_cease_to_exist() const
{
	if (objectType->has_particles_based_existence() ||
		objectType->has_sound_based_existence() ||
		objectType->has_light_sources_based_existence())
	{
		if (objectType->has_particles_based_existence() &&
			particlesActive > 0)
		{
			return false;
		}
		if (objectType->has_sound_based_existence() &&
			get_sound() &&
			! get_sound()->get_sounds().get_sounds().is_empty())
		{
			return false;
		}
		if (objectType->has_light_sources_based_existence())
		{
			if (auto* ls = get_custom<CustomModules::LightSources>())
			{
				if (ls->has_any_light_source_active())
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
}

void TemporaryObject::set_subworld_entry(TemporaryObjectTypeInSubWorld * _subWorldEntry)
{
	if (subWorldEntry == _subWorldEntry)
	{
		return;
	}

	if (subWorldEntry)
	{
		an_assert(subWorldEntry->am_I_myself());
		subWorldEntry->count.decrease();
		if (!TemporaryObjectState::is_available_for_spawn(state))
		{
			subWorldEntry->active.decrease();
		}
	}
	subWorldEntry = _subWorldEntry;
	if (subWorldEntry)
	{
		an_assert(subWorldEntry->am_I_myself());
		subWorldEntry->count.increase();
		if (!TemporaryObjectState::is_available_for_spawn(state))
		{
			subWorldEntry->active.increase();
		}
	}
}
