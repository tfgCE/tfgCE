#include "modulesOwner.h"

#include "..\advance\advanceContext.h"
#include "..\module\moduleAppearance.h"
#include "..\module\moduleSound.h"
#include "..\module\modules.h"
#include "..\object\actor.h"
#include "..\object\item.h"
#include "..\object\object.h"
#include "..\object\scenery.h"
#include "..\object\temporaryObject.h"
#include "..\interfaces\aiObject.h"
#include "..\world\presenceLink.h"
#include "..\world\subWorld.h"
#include "..\world\world.h"

#include "..\..\core\performance\performanceUtils.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define INSPECT_DESTROY
	#endif
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(ai);
DEFINE_STATIC_NAME(appearance);
DEFINE_STATIC_NAME(collision);
DEFINE_STATIC_NAME(controller);
DEFINE_STATIC_NAME(custom);
DEFINE_STATIC_NAME(gameplay);
DEFINE_STATIC_NAME(movement);
DEFINE_STATIC_NAME(navElement);
DEFINE_STATIC_NAME(presence);
DEFINE_STATIC_NAME(sound);
DEFINE_STATIC_NAME(temporaryObjects);

//

int IModulesOwner::Timer::s_uniqueId = 0;

//

REGISTER_FOR_FAST_CAST(IModulesOwner);

IModulesOwner::IModulesOwner()
: SafeObject<IModulesOwner>(nullptr)
{
	scoped_call_stack_info(TXT("IModulesOwner::IModulesOwner"));
#ifdef DEBUG_WORLD_LIFETIME
#ifdef AN_DEVELOPMENT
	if (! Framework::is_preview_game())
#endif
	output(TXT("IModulesOwner [new] o%p"), this);
#endif
}

IModulesOwner::~IModulesOwner()
{
	an_assert(autonomous, TXT("make sure object deleted is autonomous first - if you're the one who made it not autonomous, change that!"));
	an_assert(!inSubWorld, TXT("should be not set at this point!"));
	an_assert(!get_in_world(), TXT("should be not set at this point!"));
#ifdef DEBUG_WORLD_LIFETIME
#ifdef AN_DEVELOPMENT
	if (!Framework::is_preview_game())
#endif
	output(TXT("IModulesOwner [deleted] o%p"), this);
#endif
}

void IModulesOwner::mark_no_longer_advanceable(int _reasonFlag)
{
#ifdef DEBUG_WORLD_LIFETIME
#ifdef AN_DEVELOPMENT
	if (!Framework::is_preview_game())
#endif
	output(TXT("IModulesOwner [mark no longer advanceable] o%p"), this);
#endif
	advanceable = false;
	notAdvanceableReasons |= _reasonFlag;
}

void IModulesOwner::restore_being_advanceable(int _reasonFlag)
{
#ifdef DEBUG_WORLD_LIFETIME
#ifdef AN_DEVELOPMENT
	if (!Framework::is_preview_game())
#endif
	output(TXT("IModulesOwner [restore being advanceable] o%p"), this);
#endif
	notAdvanceableReasons &= (~_reasonFlag);
	if (!notAdvanceableReasons)
	{
		mark_advanceable();
	}
}

void IModulesOwner::mark_advanceable()
{
	advanceable = true;
	notAdvanceableReasons = 0;
}

void IModulesOwner::mark_advanceable_continuously(bool _advanceableContinuously)
{
	advanceableContinuously = _advanceableContinuously;
}

void IModulesOwner::init(SubWorld* _subWorld)
{
	asObject.setup(this);
	asActor.setup(this);
	asItem.setup(this);
	asScenery.setup(this);
	asTemporaryObject.setup(this);
	asAIObject.setup(this);
	asCollisionCollidableObject.setup(this);
	//
	cache_ai_object_pointers();
}

void IModulesOwner::on_destruct()
{
	// when removing from world
	make_safe_object_unavailable();
}

#define ON_DESTROY(module) if (module != nullptr) { module->on_owner_destroy(); }

void IModulesOwner::on_destroy()
{
	scoped_call_stack_info(TXT("destroy"));
	scoped_call_stack_info(ai_get_name().to_char());
#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
	output(TXT("IModulesOwner::on_destroy o%p"), this);
#endif
#ifdef INSPECT_DESTROY
	output(TXT("on destroy \"%S\""), ai_get_name().to_char());
#endif

	// all modules
	ON_DESTROY(ai);
	for_every_ptr(appearance, appearances)
	{
		ON_DESTROY(appearance);
	}
	ON_DESTROY(collision);
	ON_DESTROY(controller);
	for_every_ptr(custom, customs)
	{
		ON_DESTROY(custom);
	}
	ON_DESTROY(gameplay);
	for_every_ptr(movement, movements)
	{
		ON_DESTROY(movement);
	}
	ON_DESTROY(navElement);
	ON_DESTROY(presence);
	ON_DESTROY(sound);
	ON_DESTROY(temporaryObjects);

	// always become autonomous when being destroyed, owner should have safeptr to us, not direct one!
	be_autonomous();
}

#define ON_RESETUP(module) if (module != nullptr && _moduleTypes.does_contain(NAME(module))) { module->resetup(); }

void IModulesOwner::resetup(Array<Name> const& _moduleTypes)
{
	if (_moduleTypes.is_empty())
	{
		return;
	}

	// all modules
	ON_RESETUP(ai);
	for_every_ptr(appearance, appearances)
	{
		ON_RESETUP(appearance);
	}
	ON_RESETUP(collision);
	ON_RESETUP(controller);
	for_every_ptr(custom, customs)
	{
		ON_RESETUP(custom);
	}
	ON_RESETUP(gameplay);
	for_every_ptr(movement, movements)
	{
		ON_RESETUP(movement);
	}
	ON_RESETUP(navElement);
	ON_RESETUP(presence);
	ON_RESETUP(sound);
	ON_RESETUP(temporaryObjects);
}

void IModulesOwner::make_safe_object_available(IModulesOwner* _object)
{
	SafeObject<IModulesOwner>::make_safe_object_available(_object);
	SafeObject<IAIObject>::make_safe_object_available(_object);
}

void IModulesOwner::make_safe_object_unavailable()
{
	SafeObject<IModulesOwner>::make_safe_object_unavailable();
	SafeObject<IAIObject>::make_safe_object_unavailable();
}

bool IModulesOwner::does_require_finalise_frame() const
{
	if (advancementCache.soundsRequireAdvancement ||
		advancementCache.requiresUpdateEyeLookRelative ||
		((advancementCache.appearanceHasSkeleton || advancementCache.appearanceCopiesMaterialParameters) && get_room_distance_to_recently_seen_by_player() <= 1))
	{
		return true;
	}
#ifdef AN_NO_RARE_ADVANCE_AVAILABLE
	if (Framework::access_no_rare_advance()
#ifdef AN_ALLOW_BULLSHOTS
		|| Framework::BullshotSystem::is_active()
#endif
		)
	{
		return true;
	}
#endif

	return false;
}

void IModulesOwner::advance__finalise_frame(IMMEDIATE_JOB_PARAMS)
{
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(modulesOwnerFinaliseFrame);
		if (modulesOwner->allowRenderingSinceCoreTime.is_set() && ::System::Core::get_timer_as_double() > modulesOwner->allowRenderingSinceCoreTime.get() + 5)
		{
			modulesOwner->allowRenderingSinceCoreTime.clear();
		}
		if (ModulePresence* presence = modulesOwner->get_presence())
		{
			presence->update_eye_look_relative();
		}
		for_every_ptr(appearance, modulesOwner->get_appearances())
		{
			// ready appearances for rendering
			appearance->ready_for_rendering();
		}
		if (ModuleSound* sound = modulesOwner->get_sound())
		{
			sound->advance_sounds(ac->get_delta_time());
		}
	}
}

void IModulesOwner::advance_vr_important(float _deltaTime)
{
	ModulePresence::advance_vr_important__looks(this);
	ModuleAppearance::advance_vr_important__controllers_and_attachments(this, _deltaTime);
	if (ModulePresence * presence = get_presence())
	{
		presence->update_eye_look_relative();
	}
	for_every_ptr(appearance, get_appearances())
	{
		// ready appearances for rendering
		appearance->ready_for_rendering();
	}
}

ModuleAppearance* IModulesOwner::get_appearance(Name const & _name) const
{
	if (appearances.get_size() == 1)
	{
		return appearances.get_first();
	}
	for_every_ptr(appearance, appearances)
	{
		if (appearance->get_name() == _name)
		{
			return appearance;
		}
	}
	return nullptr;
}

ModuleAppearance* IModulesOwner::get_appearance_named(Name const & _name) const
{
	for_every_ptr(appearance, appearances)
	{
		if (appearance->get_name() == _name)
		{
			return appearance;
		}
	}
	return nullptr;
}

ModuleAppearance* IModulesOwner::get_visible_appearance() const
{
	ModuleAppearance* secondChoice = nullptr;
	for_every_ptr(appearance, appearances)
	{
		if (appearance->is_visible())
		{
			if (appearance->is_main_appearance())
			{
				return appearance;
			}
			if (!secondChoice)
			{
				secondChoice = appearance;
			}
		}
	}
	return secondChoice;
}

ModuleAppearance* IModulesOwner::get_appearance() const
{
	for_every_ptr(appearance, appearances)
	{
		if (appearance->is_main_appearance())
		{
			return appearance;
		}
	}
	return nullptr;
}

#define INITIALISE_MODULE(module) \
	if (module != nullptr) \
	{ \
		module->initialise(); \
	}
 		//REMOVE_AS_SOON_AS_POSSIBLE_ MEASURE_AND_OUTPUT_PERFORMANCE(TXT("initialise %S"), TXT(#module));

void IModulesOwner::initialise_modules(SetupModule _setup_module_pre)
{
	set_default_active_movement();

	// initialise all modules
	INITIALISE_MODULE(ai);
	for_every_ptr(appearance, appearances)
	{
		INITIALISE_MODULE(appearance);
	}
	INITIALISE_MODULE(collision);
	INITIALISE_MODULE(controller);
	for_every_ptr(custom, customs)
	{
		INITIALISE_MODULE(custom);
	}
	INITIALISE_MODULE(gameplay);
	for_every_ptr(movement, movements)
	{
		INITIALISE_MODULE(movement);
	}
	INITIALISE_MODULE(navElement);
	INITIALISE_MODULE(presence);
	INITIALISE_MODULE(sound);
	INITIALISE_MODULE(temporaryObjects);
}

#define READY_MODULE_TO_ACTIVATE(module)\
	{ \
		scoped_call_stack_info(TXT(#module)); \
		if (module != nullptr) { module->ready_to_activate(); } \
	}

void IModulesOwner::ready_modules_to_activate()
{
	scoped_call_stack_info_str_printf(TXT("ready_modules_to_activate o%p"), this);
	// activate all modules
	READY_MODULE_TO_ACTIVATE(ai);
	for_every_ptr(appearance, appearances)
	{
		READY_MODULE_TO_ACTIVATE(appearance);
	}
	READY_MODULE_TO_ACTIVATE(collision);
	READY_MODULE_TO_ACTIVATE(controller);
	for_every_ptr(custom, customs)
	{
		READY_MODULE_TO_ACTIVATE(custom);
	}
	READY_MODULE_TO_ACTIVATE(gameplay);
	for_every_ptr(movement, movements)
	{
		READY_MODULE_TO_ACTIVATE(movement);
	}
	READY_MODULE_TO_ACTIVATE(navElement);
	READY_MODULE_TO_ACTIVATE(presence);
	READY_MODULE_TO_ACTIVATE(sound);
	READY_MODULE_TO_ACTIVATE(temporaryObjects);
}


#define ACTIVATE_MODULE(module) \
	{ \
		scoped_call_stack_info(TXT(#module)); \
		if (module != nullptr) { module->activate(); } \
	}

void IModulesOwner::activate_modules()
{
	scoped_call_stack_info_str_printf(TXT("activate_modules o%p"), this);
	// activate all modules
	ACTIVATE_MODULE(ai);
	for_every_ptr(appearance, appearances)
	{
		ACTIVATE_MODULE(appearance);
	}
	ACTIVATE_MODULE(collision);
	ACTIVATE_MODULE(controller);
	for_every_ptr(custom, customs)
	{
		ACTIVATE_MODULE(custom);
	}
	ACTIVATE_MODULE(gameplay);
	for_every_ptr(movement, movements)
	{
		ACTIVATE_MODULE(movement);
	}
	ACTIVATE_MODULE(navElement);
	ACTIVATE_MODULE(presence);
	ACTIVATE_MODULE(sound);
	ACTIVATE_MODULE(temporaryObjects);
}

template <typename ModuleClass>
void close_module(ModuleClass & _module)
{
	delete_and_clear(_module);
}

template <typename ModuleClass>
void just_close_module(ModuleClass _module)
{
	if (_module)
	{
		delete _module;
	}
}

void IModulesOwner::close_modules()
{
	// TODO add other modules
	close_module(ai);
	for_every_ptr(appearance, appearances)
	{
		just_close_module(appearance);
	}
	appearances.clear();
	close_module(collision);
	close_module(controller);
	for_every_ptr(custom, customs)
	{
		just_close_module(custom);
	}
	customs.clear();
	close_module(gameplay);
	for_every_ptr(movement, movements)
	{
		just_close_module(movement);
	}
	movements.clear();
	close_module(navElement);
	close_module(presence);
	close_module(sound);
	close_module(temporaryObjects);
}

#define RESET_MODULE(module) \
	if (module != nullptr) \
	{ \
		module->reset(); \
	}

void IModulesOwner::reset()
{
	set_creator(nullptr);
	set_instigator(nullptr);
	clear_timers();

	RESET_MODULE(ai);
	for_every_ptr(appearance, appearances)
	{
		RESET_MODULE(appearance);
	}
	RESET_MODULE(collision);
	RESET_MODULE(controller);
	for_every_ptr(custom, customs)
	{
		RESET_MODULE(custom);
	}
	RESET_MODULE(gameplay);
	for_every_ptr(movement, movements)
	{
		RESET_MODULE(movement);
	}
	RESET_MODULE(navElement);
	RESET_MODULE(presence);
	RESET_MODULE(sound);
	RESET_MODULE(temporaryObjects);

	// other modules may change collisions, reset collisions once more here
	RESET_MODULE(collision);

	// and initialise modules again
	initialise_modules();
}

void IModulesOwner::update_rare_logic_advance(float _deltaTime) const
{
	if (ai)
	{
		ai->update_rare_logic_advance(_deltaTime);
	}
}

bool IModulesOwner::does_require_logic_advance() const
{
	return ai ? ai->access_rare_logic_advance().should_advance() : false;
}

bool IModulesOwner::does_require_move_advance() const
{
	if (advancementCache.raMove.should_advance()) return true; // rare advance check
	if (auto* p = get_presence())
	{
		if (p->does_request_teleport() ||
			p->has_teleported() ||
			(p->is_base_through_doors() && !p->get_based().is_empty()))
		{
			return true;
		}
	}
	return false;
}

void IModulesOwner::allow_rare_move_advance(bool _allowRAMove, Name const& _reason)
{
	MODULE_OWNER_LOCK_FOR_IMO(this, TXT("IModulesOwner::allow_rare_move_advance"));
	if (_allowRAMove)
	{
		advancementCache.blockRAMove.remove_fast(_reason);
	}
	else
	{
		advancementCache.blockRAMove.push_back_unique(_reason);
	}
}

void IModulesOwner::update_rare_move_advance(float _deltaTime) const
{
	bool rare = advancementCache.blockRAMove.is_empty();
	if (rare)
	{
		if (auto* m = get_movement())
		{
			if (m->does_require_continuous_movement())
			{
				rare = false;
			}
		}
	}
	if (rare)
	{
		if (auto* p = get_presence())
		{
			rare = !p->does_request_teleport();
			if (rare &&
				get_room_distance_to_recently_seen_by_player() <= 1)
			{
				rare = false;
			}
		}
	}
	if (rare)
	{
		advancementCache.raMove.override_interval(Range(0.05f, 0.15f));
	}
	else
	{
		advancementCache.raMove.override_interval(NP);
	}
	advancementCache.raMove.update(_deltaTime);
}

bool IModulesOwner::does_appearance_require(Concurrency::ImmediateJobFunc _jobFunc) const
{
	if (_jobFunc == ModuleAppearance::advance__calculate_preliminary_pose)
	{
		return advancementCache.appearanceHasSkeleton;
	}
	if (_jobFunc == ModuleAppearance::advance__advance_controllers_and_adjust_preliminary_pose)
	{
		return advancementCache.appearanceHasSkeleton || advancementCache.appearanceSynchronisesToOtherAppearance || advancementCache.appearanceHasControllers;
	}
	if (_jobFunc == ModuleAppearance::advance__calculate_final_pose_and_attachments)
	{
		if (advancementCache.appearanceHasSkeleton || advancementCache.appearanceSynchronisesToOtherAppearance || advancementCache.appearanceHasControllers)
		{
			return true;
		}
		if (auto* p = get_presence())
		{
			if (p->has_attachments() || p->does_require_update_of_presence_link_setup())
			{
				return true;
			}
		}
		return false;
	}

	an_assert(false, TXT("should not end up here!"));
	for_every_ptr(appearance, appearances)
	{
		if (appearance->does_require(_jobFunc))
		{
			return true;
		}
	}
	return false;
}

void IModulesOwner::set_instigator(IModulesOwner* _instigator)
{
	if (_instigator == nullptr && instigator.is_set())
	{
		validTopInstigator = instigator->get_valid_top_instigator();
	}
	instigator = _instigator;
	if (instigator.is_set())
	{
		validTopInstigator = instigator->get_valid_top_instigator();
	}
}

IModulesOwner* IModulesOwner::get_instigator_first_actor(bool _notUs) const
{
	if (_notUs && !instigator.is_set())
	{
		return nullptr;
	}
	IModulesOwner const * i = this;
	while (i->instigator.get())
	{
		i = i->instigator.get();
		if (auto* o = i->get_as_object())
		{
			if (!o->is_sub_object())
			{
				// first actor!
				break;
			}
		}
	}
	return cast_to_nonconst(i);
}

IModulesOwner* IModulesOwner::get_top_instigator(bool _notUs) const
{
	if (_notUs && !instigator.is_set())
	{
		return nullptr;
	}
	IModulesOwner const * i = this;
	while (i->instigator.get())
	{
		i = i->instigator.get();
	}
	return cast_to_nonconst(i);
}

bool IModulesOwner::has_timer(Name const& _name, OPTIONAL_ OUT_ float* _timeLeft)
{
	Concurrency::ScopedSpinLock lock(timersLock);

	for_every(timer, timers)
	{
		if (timer->name == _name)
		{
			assign_optional_out_param(_timeLeft, max(timer->timeLeft, timer->framesLeft * 0.01f)); // rough approximation for frames
			return true;
		}
	}

	return false;
}

void IModulesOwner::set_timer(int _frameDelay, float _timeLeft, Name const & _name, TimerFunc _timerFunc)
{
	Concurrency::ScopedSpinLock lock(timersLock);

	if (_name.is_valid())
	{
		for_every(timer, timers)
		{
			if (timer->name == _name)
			{
				timer->framesLeft = _frameDelay;
				timer->timeLeft = _timeLeft;
				timer->timerFunc = _timerFunc;
				return;
			}
		}
	}
	Timer timer;
	timer.id = Timer::s_uniqueId;
	++ Timer::s_uniqueId;
	timer.name = _name;
	timer.framesLeft = _frameDelay;
	timer.timeLeft = _timeLeft;
	timer.timerFunc = _timerFunc;
	timers.push_back(timer);
	//an_assert_immediate(timers.get_size() < 2, TXT("it's not actual error. I just got strange things happening with cease to exist timers issued from health"));
}


void IModulesOwner::clear_timer(Name const & _name)
{
	Concurrency::ScopedSpinLock lock(timersLock);

	an_assert(_name.is_valid());
	for_every(timer, timers)
	{
		if (timer->name == _name)
		{
			timers.remove_at(for_everys_index(timer));
			break;
		}
	}
}

void IModulesOwner::clear_timers()
{
	Concurrency::ScopedSpinLock lock(timersLock);

	timers.clear();
}

void IModulesOwner::advance__timers(IMMEDIATE_JOB_PARAMS)
{
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		if (modulesOwner->timers.is_empty())
		{
			continue;
		}
		MEASURE_PERFORMANCE(advanceTimers);
		// use prealloc array as array stack in a loop is unreliable
		ARRAY_PREALLOC_SIZE(int, timerIds, modulesOwner->timers.get_size()); // use this array with ids to run only timers that were already set and not skip one of them
		for_every(timer, modulesOwner->timers)
		{
			timerIds.push_back(timer->id);
		}
		Framework::AdvanceContext const* ac = plain_cast<Framework::AdvanceContext const>(_executionContext);
		float const deltaTime = ac->get_delta_time();
		for_every(timerId, timerIds)
		{
			for_every(timer, modulesOwner->timers)
			{
				if (timer->id == *timerId)
				{
					timer->timeLeft = max(0.0f, timer->timeLeft - deltaTime);
					if (timer->framesLeft <= 0 &&
						timer->timeLeft <= 0.0f)
					{
						modulesOwner->resume_advancement();
						timer->timerFunc(modulesOwner);
						modulesOwner->timers.remove_at(for_everys_index(timer));
					}
					else
					{
						timer->framesLeft = max(0, timer->framesLeft - 1);
					}
				}
			}
		}
	}
}

bool IModulesOwner::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	TypeId id = _value.get_type();
	SimpleVariableInfo const & info = variables.find(_byName, id);
	an_assert(RegisteredType::is_plain_data(id));
	memory_copy(info.access_raw(), _value.get_raw(), RegisteredType::get_size_of(id));
	return true;
}

bool IModulesOwner::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	TypeId id;
	void const * rawData;
	if (variables.get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
	{
		_value.set_raw(id, rawData);
		return true;
	}
	else
	{
		return false;
	}
}

bool IModulesOwner::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return variables.convert_existing(_byName, _to);
}

bool IModulesOwner::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return variables.rename_existing(_from, _to);
}

bool IModulesOwner::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return store_value_for_wheres_my_point(_byName, _value);
}

bool IModulesOwner::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool IModulesOwner::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	an_assert(false, TXT("no use for this"));
	return false;
}

WheresMyPoint::IOwner* IModulesOwner::get_wmp_owner()
{
	if (creator.is_set())
	{
		return creator.get();
	}
	if (presence)
	{
		if (auto* r = presence->get_in_room())
		{
			return r;
		}
		todo_important(TXT("in which room it will be placed?"));
	}
	if (fallbackWMPOwnerRoom.is_set())
	{
		return fallbackWMPOwnerRoom.get();
	}
	if (fallbackWMPOwnerObject.is_set())
	{
		return fallbackWMPOwnerObject.get();
	}
	return nullptr;
}

void IModulesOwner::set_fallback_wmp_owner(WheresMyPoint::IOwner* _fallbackOwner)
{
	if (auto * r = fast_cast<Room>(_fallbackOwner))
	{
		fallbackWMPOwnerRoom = r;
	}
	else if (auto * imo = fast_cast<IModulesOwner>(_fallbackOwner))
	{
		fallbackWMPOwnerObject = imo;
	}
	else if (_fallbackOwner)
	{
		error(TXT("fallback owner not safe!"));
	}
}

Name const& IModulesOwner::get_movement_name() const
{
	if (activeMovement)
	{
		return activeMovement->get_name();
	}

	return Name::invalid();
}

void IModulesOwner::activate_movement(ModuleMovement* _movement)
{
	if (activeMovement == _movement)
	{
		return;
	}
	if (activeMovement)
	{
		activeMovement->on_deactivate_movement(_movement);
	}
	ModuleMovement* prevMovement = activeMovement;
	activeMovement = _movement;
	if (activeMovement)
	{
		activeMovement->on_activate_movement(prevMovement);
	}
}

bool IModulesOwner::activate_movement(Name const & _movement, bool _mayFail)
{
	for_every_ptr(movement, movements)
	{
		if (movement->get_name() == _movement)
		{
			activate_movement(movement);
			return true;
		}
	}
	if (!_mayFail)
	{
		error(TXT("could not find movement module of name \"%S\""), _movement.to_char());
	}
	return false;
}

void IModulesOwner::activate_default_movement()
{
	for_every_ptr(movement, movements)
	{
		if (!movement->get_name().is_valid())
		{
			activate_movement(movement);
			return;
		}
	}
	activate_movement(movements.get_first());
}

void IModulesOwner::set_default_active_movement()
{
	activeMovement = nullptr;
	if (movements.is_empty())
	{
		return;
	}
	for_every_ptr(movement, movements)
	{
		if (!movement->get_name().is_valid())
		{
			activeMovement = movement;
			return;
		}
	}
	activeMovement = movements.get_first();
}

World* IModulesOwner::get_in_world() const
{
	an_assert(!inSubWorld || !inSubWorld->get_in_world() ||
		   !fast_cast<WorldObject>(this) || !fast_cast<WorldObject>(this)->get_in_world() ||
		   inSubWorld->get_in_world() == fast_cast<WorldObject>(this)->get_in_world(), TXT("should be in the same world!"));
	World* inWorld = nullptr;
	if (inSubWorld)
	{
		inWorld = inSubWorld->get_in_world();
	}
	else if (auto * wo = fast_cast<WorldObject>(this))
	{
		inWorld = wo->get_in_world();
	}
	return inWorld;
}

SubWorld* IModulesOwner::get_in_sub_world() const
{
	return inSubWorld;
}

void IModulesOwner::log(LogInfoContext & _log, ::SafePtr<IModulesOwner> const & _imo)
{
	if (_imo.get())
	{
		_log.log(_imo->ai_get_name().to_char());
	}
	else
	{
		_log.log(TXT("--"));
	}
}

bool IModulesOwner::should_be_rendered() const
{
	if (!allowRenderingSinceCoreTime.is_set())
	{
		return true;
	}
	return ::System::Core::get_timer_as_double() >= allowRenderingSinceCoreTime.get();
}

void IModulesOwner::set_allow_rendering_since_core_time(double _timer)
{
	allowRenderingSinceCoreTime = _timer;
}

void IModulesOwner::set_rare_pose_advance_override(Optional<Range> const& _rare)
{
	if (_rare.is_set())
	{
		advancementCache.raPoseAuto = false;
		advancementCache.raPose.override_interval(_rare);
	}
	else
	{
		advancementCache.raPoseAuto = true;
	}
}

void IModulesOwner::update_presence_cached()
{
	advancementCache.requiresUpdateEyeLookRelative = false;
	if (auto* p = get_presence())
	{
		advancementCache.requiresUpdateEyeLookRelative = AVOID_CALLING_ACK_ p->does_require_update_eye_look_relative();
	}
}

void IModulesOwner::update_appearance_cached()
{
	advancementCache.appearanceHasSkeleton = false;
	advancementCache.appearanceSynchronisesToOtherAppearance = false;
	advancementCache.appearanceHasControllers = false;
	advancementCache.appearanceCopiesMaterialParameters = false;
	for_every_ptr(appearance, appearances)
	{
		if (appearance->get_skeleton())
		{
			advancementCache.appearanceHasSkeleton = true;
		}
		if (appearance->does_require_syncing_to_other_apperance())
		{
			advancementCache.appearanceSynchronisesToOtherAppearance = true;
		}
		if (!appearance->get_controllers().is_empty())
		{
			advancementCache.appearanceHasControllers = true;
		}
		if (appearance->does_require_copying_material_parameters())
		{
			advancementCache.appearanceCopiesMaterialParameters = true;
		}
	}
}

void IModulesOwner::update_rare_pose_advance(float _deltaTime) const
{
	if (advancementCache.raPoseAuto)
	{
		bool doRare = true;
		bool doReallyRare = false;
		{
			int roomDistanceToRecentlySeenByPlayer = get_room_distance_to_recently_seen_by_player();
			if (is_important_to_player() || roomDistanceToRecentlySeenByPlayer <= 0)
			{
				doRare = false;
			}
			else if (roomDistanceToRecentlySeenByPlayer > 1)
			{
				doReallyRare = true;
			}
		}
		advancementCache.raPose.override_interval(doReallyRare ? Range(0.3f, 1.0f) : (doRare ? Range(0.1f, 0.5f) : Optional<Range>()));
	}

	if (advancementCache.poisRequireAdvancement)
	{
		advancementCache.raPoseAdvance = true;
		advancementCache.raPoseAdvanceDeltaTime = _deltaTime;
		advancementCache.raPose.reset();
	}
	else
	{
		advancementCache.raPoseAdvance = advancementCache.raPose.update(_deltaTime);
		advancementCache.raPoseAdvanceDeltaTime = advancementCache.raPose.get_advance_delta_time();
	}
}

void IModulesOwner::update_room_distance_to_recently_seen_by_player()
{
	int roomDistance = 1000;
	if (auto* p = get_presence())
	{
		if (auto* r = p->get_in_room())
		{
			roomDistance = min(roomDistance, AVOID_CALLING_ACK_ r->get_distance_to_recently_seen_by_player());
		}
		auto* pl = p->get_presence_links();
		while (pl)
		{
			if (auto* r = pl->get_in_room())
			{
				roomDistance = min(roomDistance, AVOID_CALLING_ACK_ r->get_distance_to_recently_seen_by_player());
			}
			pl = pl->get_next_in_object();
		}
	}

	set_room_distance_to_recently_seen_by_player(roomDistance);
}

void IModulesOwner::suspend_advancement()
{
	if (!advancementSuspended &&
		advancementSuspendable)
	{
		advancementSuspended = advancementSuspendable;
		if (auto* ai = get_ai())
		{
			ai->on_advancement_suspended();
		}
	}
}

void IModulesOwner::resume_advancement()
{
	if (advancementSuspended)
	{
		advancementSuspended = false;
		if (auto* ai = get_ai())
		{
			ai->on_advancement_resumed();
		}
	}
}

void IModulesOwner::set_game_related_system_flag(int _flag)
{
	Concurrency::ScopedSpinLock lock(modulesOwnerLock, true);
	set_flag(gameRelatedSystemFlags, _flag);
}

void IModulesOwner::clear_game_related_system_flag(int _flag)
{
	Concurrency::ScopedSpinLock lock(modulesOwnerLock, true);
	clear_flag(gameRelatedSystemFlags, _flag);
}
