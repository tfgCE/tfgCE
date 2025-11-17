#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\cachedPointer.h"
#include "..\..\core\fastCast.h"
#include "..\..\core\lifetimeIndicator.h"
#include "..\..\core\concurrency\immediateJobFunc.h"
#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\random\random.h"
#include "..\..\core\wheresMyPoint\iWMPOwner.h"

#include "..\module\moduleRareAdvance.h"

#include "..\interfaces\aiObject.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"

struct Name;

namespace Collision
{
	interface_class ICollidableObject;
};

namespace Concurrency
{
	class JobExecutionContext;
};

namespace WheresMyPoint
{
	struct Value;
};

namespace Framework
{
	class Module;
	class ModuleAI;
	class ModuleAppearance;
	class ModuleCollision;
	class ModuleController;
	class ModuleCustom;
	class ModuleGameplay;
	class ModuleMovement;
	class ModuleNavElement;
	class ModulePresence;
	class ModuleSound;
	class ModuleTemporaryObjects;

	class AdvanceContext;
	class Object;
	class Actor;
	class Item;
	class Room;
	class Scenery;
	class TemporaryObject;
	class SubWorld;
	class World;
	
	struct LibraryName;

	typedef std::function<void(Module* _module)> SetupModule;

	struct IModulesOwnerAdvancementCache
	{
		ArrayStatic<Name, 4> blockRAMove;
		mutable ModuleRareAdvance raMove; // we do lots of different modules with it

		bool raPoseAuto = true;
		mutable ModuleRareAdvance raPose; // we do lots of different modules with it
		mutable bool raPoseAdvance = true;
		mutable float raPoseAdvanceDeltaTime = 0.0f;

		bool poisRequireAdvancement = false;
		
		bool soundsRequireAdvancement = false;

		bool requiresUpdateEyeLookRelative = false;

		bool appearanceHasSkeleton = false;
		bool appearanceSynchronisesToOtherAppearance = false;
		bool appearanceHasControllers = false;
		bool appearanceCopiesMaterialParameters = false;

		int roomDistanceToRecentlySeenByPlayer = NONE;
	
		IModulesOwnerAdvancementCache()
		{
			SET_EXTRA_DEBUG_INFO(blockRAMove, TXT("IModulesOwnerAdvancementCache.blockRAMove"));
		}
	};

	/**
	 *	It's no longer an interface, as it has modules inside.
	 *
	 *	Modules lifetime:
	 *		1. create
	 *		2. initialise_modules
	 *		3. reset & initialise_modules
	 *		4. close_modules
	 */
	interface_class IModulesOwner
	: public SafeObject<IModulesOwner>
	, public IAIObject
	, public WheresMyPoint::IOwner
	// IDebugableObject,
	// IEventHandlerObject,
	// IStatsModifiersProvider,
	// IAIObject
	{
		FAST_CAST_DECLARE(IModulesOwner);
		FAST_CAST_BASE(IAIObject);
		FAST_CAST_BASE(WheresMyPoint::IOwner);
		FAST_CAST_END();

#ifdef DEBUG_WORLD_LIFETIME_INDICATOR
	public:
		LifetimeIndicator __lifetimeIndicator;
#endif

	public:
		typedef std::function<void(IModulesOwner* _imo)> TimerFunc;

	public:
		IModulesOwner();
		virtual ~IModulesOwner();

	public:
		enum NoLongerAdvanceableFlag
		{
			BeingDestroyed = 1 << 0,
			RoomBeingDestroyed = 1 << 1
		};
		void mark_no_longer_advanceable(int _reasonFlag = NoLongerAdvanceableFlag::BeingDestroyed);
		void restore_being_advanceable(int _reasonFlag = NoLongerAdvanceableFlag::BeingDestroyed);
		void mark_advanceable();
		bool is_advanceable() const { return advanceable; }

		void mark_advanceable_continuously(bool _advanceableContinuously = true);
		bool is_advanceable_continuously() const { return advanceableContinuously; }

	public:
		void make_advancement_suspendable() { advancementSuspendable = true; }
		void make_advancement_not_suspendable() { advancementSuspendable = false; advancementSuspended = false; }
		bool is_advancement_suspended() const { return advancementSuspended; }
		void suspend_advancement();
		void resume_advancement();
		
		int get_game_related_system_flags() const { return gameRelatedSystemFlags; }
		bool is_game_related_system_flag_set(int _flag) const { return gameRelatedSystemFlags & _flag; }
		void set_game_related_system_flag(int _flag);
		void clear_game_related_system_flag(int _flag);

		// use with MODULE_OWNER_LOCK_*
		Concurrency::SpinLock & access_lock() { return modulesOwnerLock; } // check modulesOwnerLock

		void set_creator(IModulesOwner* _creator) { creator = _creator; }
		inline IModulesOwner* get_creator(bool _notUs = false) const { return creator.is_set() || _notUs? creator.get() : cast_to_nonconst(this); } // get creator - something that created this object (or us)

		void set_instigator(IModulesOwner* _instigator);
		inline IModulesOwner* get_instigator(bool _notUs = false) const { return instigator.is_set() || _notUs? instigator.get() : cast_to_nonconst(this); } // get direct instigator - a gun that shoot this projectile, owner of a particle effect
		IModulesOwner* get_top_instigator(bool _notUs = false) const; // get top most instigator - user of the gun that shoot this projectile
		IModulesOwner* get_instigator_first_actor(bool _notUs = false) const; // get instigator, first actor (it might be barker triggered by pilgrim)
		IModulesOwner* get_valid_top_instigator() const { if (get_instigator(true)) return get_top_instigator(true); else if (validTopInstigator.get()) return validTopInstigator.get(); else return cast_to_nonconst(this); }
		IModulesOwner* get_instigator_first_actor_or_valid_top_instigator() const { if (auto* ifa = get_instigator_first_actor(false)) return ifa; else if (validTopInstigator.get()) return validTopInstigator.get(); else return cast_to_nonconst(this); }

		bool is_autonomous() const { return autonomous; }
		void be_autonomous() { autonomous = true; }
		void be_non_autonomous() { autonomous = false; }

		// most of the modules shouldn't change during game
		//virtual ModuleActionHandler* get_action_handler() const { return nullptr; }
		ModuleAI* get_ai() const { return ai; }
		ModuleAppearance* get_visible_appearance() const;
		ModuleAppearance* get_appearance() const;
		ModuleAppearance* get_appearance(Name const & _name) const; // or any other
		ModuleAppearance* get_appearance_named(Name const & _name) const;
		Array<ModuleAppearance*> const & get_appearances() const { return appearances; }
		ModuleCollision* get_collision() const { return collision; }
		ModuleController* get_controller() const { return controller; }
		template <typename Class>
		Class* get_gameplay_for_attached_to(); // goes through attachment path
		template <typename Class>
		Class* get_custom_for_attached_to(); // goes through attachment path
		template <typename Class>
		Class* get_custom() const;
		template <typename Class>
		void for_all_custom(std::function<void(Class*)>) const;
		Array<ModuleCustom*> const & get_customs() const { return customs; }
		ModuleGameplay* get_gameplay() const { return gameplay; }
		Name const& get_movement_name() const;
		ModuleMovement* get_movement() const { return activeMovement; }
		ModuleMovement* get_movement(Name const & _name) const;
		Array<ModuleMovement*> const & get_movements() const { return movements; }
		ModuleNavElement* get_nav_element() const { return navElement; }
		ModulePresence* get_presence() const { return presence; }
		ModuleSound* get_sound() const { return sound; }
		ModuleTemporaryObjects* get_temporary_objects() const { return temporaryObjects; }

		template <typename Class>
		Class* get_gameplay_as() const { return fast_cast<Class>(get_gameplay()); } // include modulesOwnerImpl.inl

		static void log(LogInfoContext & _log, ::SafePtr<IModulesOwner> const & _imo);

	public:
		World* get_in_world() const; // we should be fine with subWorld's world but in case we're not in a subWorld, we access through world object
		SubWorld* get_in_sub_world() const;

	protected: friend class World;
			   friend class SubWorld;
		void set_in_sub_world(SubWorld* _subWorld) { inSubWorld = _subWorld; }

#ifdef AN_PERFORMANCE_MEASURE
	public:
		virtual String const & get_performance_context_info() const { return String::empty(); }
#endif

	public: // SafeObject
		inline bool is_available_as_safe_object() const { return SafeObject<IModulesOwner>::is_available_as_safe_object(); }

	protected: // SafeObject
		void make_safe_object_available(IModulesOwner* _object);
		void make_safe_object_unavailable();

	public: // IAIObject
		override_ IAIObject const * ai_instigator() const { return get_valid_top_instigator(); }

	public: // WheresMyPoint::IOwner
		override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
		override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
		override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const; // shouldn't be called!

		override_ WheresMyPoint::IOwner* get_wmp_owner();
		void set_fallback_wmp_owner(WheresMyPoint::IOwner* _fallbackWMPOwner);

	public:
		void set_timer(int _frameDelay, Name const & _name, TimerFunc _timerFunc) { set_timer(_frameDelay, 0.0f, _name, _timerFunc); }
		void set_timer(float _timeLeft, Name const & _name, TimerFunc _timerFunc) { set_timer(0, _timeLeft, _name, _timerFunc); }
		void set_timer(int _frameDelay, TimerFunc _timerFunc) { set_timer(_frameDelay, Name::invalid(), _timerFunc); }
		void set_timer(float _timeLeft, TimerFunc _timerFunc) { set_timer(_timeLeft, Name::invalid(), _timerFunc); }
		void clear_timer(Name const & _name);
		void clear_timers();
		bool has_timers() const { return !timers.is_empty(); }
		bool has_timer(Name const& _name, OPTIONAL_ OUT_ float * _timeLeft = nullptr);

		void set_timer(int _frameDelay, float _timeLeft, Name const& _name, TimerFunc _timerFunc);

	public:
		virtual void init(SubWorld* _subWorld); // to add to subworld, setup modules

		virtual void initialise_modules(SetupModule _setup_module_pre = nullptr); // initialise modules as they were setup
		virtual void ready_modules_to_activate(); // before object is activated but after it is placed
		virtual void activate_modules(); // when object is being activated
		virtual void reset(); // get ready to game, do all post initialise stuff - setup initial state of modules etc. calls initialise_modules too
		virtual void close_modules();

		void resetup(Array<Name> const & _moduleTypes); // will call resetup (which is setup_with) for modules of given types

		// this is not destruction! this is removal from living world, will remove from world immediately if requested or will stay until able to remove
		// try to avoid removing from the world immediately, in most cases we may wait
		virtual void cease_to_exist(bool _removeFromWorldImmediately) = 0;

		virtual bool does_allow_to_attach(IModulesOwner const * _imo) const = 0; // to separate temporary objects from objects
		virtual bool should_do_calculate_final_pose_attachment_with(IModulesOwner const * _imo) const = 0; // to separate temporary objects from objects

	protected:
		virtual void on_destruct(); // when removing from world
		void on_destroy();

	public:
		//

		Random::Generator const & get_individual_random_generator() const { an_assert(!individualRandomGenerator.is_zero_seed(), TXT("not initialised properly?")); return individualRandomGenerator; }
		Random::Generator & access_individual_random_generator() { return individualRandomGenerator; }
		void randomise_individual_random_generator() { individualRandomGenerator = Random::Generator(); }

		SimpleVariableStorage const & get_variables() const { return variables; }
		SimpleVariableStorage & access_variables() { return variables; }

		//

		virtual Name const & get_object_type_name() const = 0;
		virtual LibraryName const & get_library_name() const = 0;

		Object * get_as_object() { return asObject.get(); }
		Actor * get_as_actor() { return asActor.get(); }
		Item * get_as_item() { return asItem.get(); }
		Scenery * get_as_scenery() { return asScenery.get(); }
		TemporaryObject * get_as_temporary_object() { return asTemporaryObject.get(); }
		IAIObject * get_as_ai_object() { return asAIObject.get(); }
		Collision::ICollidableObject * get_as_collision_collidable_object() { return asCollisionCollidableObject.get(); }

		Object const * get_as_object() const { return asObject.get(); }
		Actor const * get_as_actor() const { return asActor.get(); }
		Item const * get_as_item() const { return asItem.get(); }
		Scenery const * get_as_scenery() const { return asScenery.get(); }
		TemporaryObject const * get_as_temporary_object() const { return asTemporaryObject.get(); }
		IAIObject const * get_as_ai_object() const { return asAIObject.get(); }
		Collision::ICollidableObject const * get_as_collision_collidable_object() const { return asCollisionCollidableObject.get(); }

	public: // rendering
		bool should_be_rendered() const;
		void set_allow_rendering_since_core_time(double _timer);

	public:
		inline bool is_important_to_player() const { return get_room_distance_to_recently_seen_by_player() >= 0 && get_room_distance_to_recently_seen_by_player() < 2; }
		inline bool was_recently_seen_by_player() const { return get_room_distance_to_recently_seen_by_player() == 0; }
		inline int get_room_distance_to_recently_seen_by_player() const { return advancementCache.roomDistanceToRecentlySeenByPlayer; }
		inline void set_room_distance_to_recently_seen_by_player(int _roomDistanceToRecentlySeenByPlayer) { advancementCache.roomDistanceToRecentlySeenByPlayer = _roomDistanceToRecentlySeenByPlayer; }
		void update_room_distance_to_recently_seen_by_player();
		//
		void update_appearance_cached();
		//
		void update_presence_cached();
		//
		inline bool do_pois_require_advancement() const { return advancementCache.poisRequireAdvancement; }
		inline void clear_pois_require_advancement() { advancementCache.poisRequireAdvancement = false; }
		inline void mark_pois_require_advancement() { advancementCache.poisRequireAdvancement = true; }
		//
		inline void clear_sounds_require_advancement() { advancementCache.soundsRequireAdvancement = false; }
		inline void mark_sounds_require_advancement() { advancementCache.soundsRequireAdvancement = true; }

	public: // advancements (does require)
		void update_rare_logic_advance(float _deltaTime) const;
		bool does_require_logic_advance() const;
		//
		void allow_rare_move_advance(bool _allowRAMove, Name const& _reason);
		void update_rare_move_advance(float _deltaTime) const;
		bool does_require_move_advance() const;
		float get_move_advance_delta_time() const { return advancementCache.raMove.get_advance_delta_time(); }
		//
		bool does_appearance_require(Concurrency::ImmediateJobFunc _jobFunc) const;
		ModuleRareAdvance const& get_ra_pose() const { return advancementCache.raPose; }
		void update_rare_pose_advance(float _deltaTime) const;
		void set_rare_pose_advance_override(Optional<Range> const& _rare); // if none, reverts to auto
		bool does_require_pose_advance() const { return advancementCache.raPoseAdvance; }
		float get_pose_advance_delta_time() const { return advancementCache.raPoseAdvanceDeltaTime; }
		//
		bool does_require_finalise_frame() const;

	public: // advancements
		// advance - begin
		static void advance__timers(IMMEDIATE_JOB_PARAMS); // timers are advanced at the end of the frame
		static void advance__finalise_frame(IMMEDIATE_JOB_PARAMS);
		// advance - end

		void advance_vr_important(float _deltaTime); // this should be called explicitly after vr controls are processed which should happen just before building render scene, check render scene

	public: // changing movement
		void activate_movement(ModuleMovement* _movement);
		bool activate_movement(Name const& _movement = Name::invalid(), bool _mayFail = false);
		void activate_default_movement();

	protected:
		Concurrency::SpinLock modulesOwnerLock = Concurrency::SpinLock(TXT("Framework.IModulesOwner.modulesOwnerLock")); // this is mostly for gameplay code that may happen from time to time and has to be processed immediately! if something happens very often, it should be considered to be done without any locks

		int gameRelatedSystemFlags = 0; // any super game related flags (like ability to cease by spawn managers)

		// derived classes should handle setting this on their own
		SubWorld* inSubWorld = nullptr;
		::SafePtr<Room> fallbackWMPOwnerRoom;
		::SafePtr<IModulesOwner> fallbackWMPOwnerObject;

		bool advanceable = true; // you may also want to look into advanceOnce as it forces to just advance on object once
		bool advanceableContinuously = true; // if advanced just once, won't be advanced continuously
		int notAdvanceableReasons = 0; // if marking to be destroyed we add reasons that we can undo with restore being advanceable, mark_advanceable clears this
		bool advanced = false;

		bool advancementSuspended = false; // ignored for temporary objects?
		bool advancementSuspendable = true; // can be suspended

		bool autonomous = true; // non autonomous have some special treatment in some cases, they can be still destroyed by someone else and owner should have safe pointer to them

		IModulesOwnerAdvancementCache advancementCache;

		Optional<double> allowRenderingSinceCoreTime;

		Random::Generator individualRandomGenerator; // used as a base
		SimpleVariableStorage variables; // used for various modules (eg. when creating mesh via generator, appearance controllers)

		::SafePtr<IModulesOwner> creator; // creator of this modules owner (for explosion it is someone who exploded while instigator is someone who made it explode)
		::SafePtr<IModulesOwner> instigator; // responsible for this modules owner (for projectile this is a gun, top instigator would be someone who is using the gun)
		::SafePtr<IModulesOwner> validTopInstigator; // this is last valid instigator - current one or last one before nullifying
		ModuleAI* ai = nullptr;
		Array<ModuleAppearance*> appearances; // first appearance is main
		ModuleCollision* collision = nullptr;
		ModuleController* controller = nullptr;
		ModuleGameplay* gameplay = nullptr;
		ModuleMovement* activeMovement = nullptr;
		Array<ModuleMovement*> movements;
		ModuleNavElement* navElement = nullptr;
		ModulePresence* presence = nullptr;
		ModuleSound* sound = nullptr;
		ModuleTemporaryObjects* temporaryObjects = nullptr;
		Array<ModuleCustom*> customs;
		
	protected:
		struct Timer
		{
			static int s_uniqueId; // we shouldn't overlap with ids, we're not going to have one timer stuck for this long
			int id;
			Name name;
			// both counters need to reach 0
			int framesLeft = 0;
			float timeLeft = 0.0f;
			TimerFunc timerFunc = nullptr;
		};
		Concurrency::SpinLock timersLock = Concurrency::SpinLock(TXT("Framework.IModulesOwner.timersLock"));
		Array<Timer> timers;

	private:
		TypedCachedPointer<Object> asObject;
		TypedCachedPointer<Actor> asActor;
		TypedCachedPointer<Item> asItem;
		TypedCachedPointer<Scenery> asScenery;
		TypedCachedPointer<TemporaryObject> asTemporaryObject;
		TypedCachedPointer<IAIObject> asAIObject;
		TypedCachedPointer<Collision::ICollidableObject> asCollisionCollidableObject;

		void set_default_active_movement(); // 
	};
};

DECLARE_REGISTERED_TYPE(SafePtr<Framework::IModulesOwner>);
