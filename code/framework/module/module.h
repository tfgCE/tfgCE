#pragma once

#include "moduleData.h"

#include "..\modulesOwner\modulesOwner.h"
#include "..\..\core\collision\iCollidableObject.h"
#include "..\..\core\concurrency\immediateJobFunc.h"
#include "..\..\core\wheresMyPoint\iWMPOwner.h"

// to make sure module is doing some crucial changes and no one else does them at the same time
// should be only for very short operations
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
#define MODULE_OWNER_LOCK(_reason) Concurrency::ScopedSpinLock temp_variable_named(lock)(access_owners_lock(), true, _reason)
#define MODULE_OWNER_LOCK_FOR(_for, _reason) Concurrency::ScopedSpinLock temp_variable_named(lock)((_for)->access_owners_lock(), true, _reason)
#define MODULE_OWNER_LOCK_FOR_IMO(_for, _reason) Concurrency::ScopedSpinLock temp_variable_named(lock)((_for)->access_lock(), true, _reason)
#else
#define MODULE_OWNER_LOCK(_reason) Concurrency::ScopedSpinLock temp_variable_named(lock)(access_owners_lock(), true)
#define MODULE_OWNER_LOCK_FOR(_for, _reason) Concurrency::ScopedSpinLock temp_variable_named(lock)((_for)->access_owners_lock(), true)
#define MODULE_OWNER_LOCK_FOR_IMO(_for, _reason) Concurrency::ScopedSpinLock temp_variable_named(lock)((_for)->access_lock(), true)
#endif
#define IS_MODULE_OWNER_LOCKED_HERE an_assert(access_owners_lock().is_locked_on_this_thread())

namespace WheresMyPoint
{
	struct Value;
};

namespace Framework
{
	class Actor;
	class Object;
	class IAIObject;
	class TemporaryObject;

	template <typename BaseModuleClass> class RegisteredModule;

	/**
	 *	Each module is responsible for different functionality
	 *
	 *		ai
	 *			all ai subsystems go in here as parts of ai mind
	 *				behaviors (behavior system or hardcoded)
	 *				message system
	 *				perception
	 *				locomotion (handling move to/move around etc sending that to controller)
	 *
	 *		appearance
	 *			how object appears in the world - just rendering
	 *			it stores information about location, bones etc
	 *
	 *		collision
	 *			collision of object in the world
	 *			allows checking for current collisions and gradients
	 *			is not responsible for collision itself!
	 *
	 *		controller
	 *			proxy between AI or player controller that stores information about input
	 *			and allows other systems reading them
	 *			doesn't handle more complex actions but should be used as place to store information about them
	 *			for example: if we want character to go using a path, this is a place to store that path and result of our command
	 *
	 *		movement
	 *			decides how character should move (from animation, from input and so on)
	 *			handles how velocity etc changes over time depending on input and on collision
	 *			may take input from animation
	 *
	 *		presence
	 *			does actual movement (as presence changes due to movement)
	 *			presents how object is placed in the world (divides object into link proxies)
	 *
	 */
	class Module
	: public WheresMyPoint::IOwner
	{
		FAST_CAST_DECLARE(Module);
		FAST_CAST_BASE(WheresMyPoint::IOwner);
		FAST_CAST_END();
	public:
		// TODO add some priorities of execution and auto execution/advancement of modules?
		// nah, it should be fine with manual order of advance... for now

		Module(IModulesOwner* _owner); // set defaults in constructor
		virtual ~Module();

		ModuleData const* get_module_data() const { return moduleData; }

		void set_owner(IModulesOwner* _owner);
		IModulesOwner* get_owner() const { an_assert(owner); return owner; }
		Object* get_owner_as_object() const { return ownerAsObject; }
		TemporaryObject* get_owner_as_temporary_object() const { return ownerAsTemporaryObject; }
		bool is_owner_object() const { return ownerAsObject != nullptr; }
		bool is_owner_temporary_object() const { return ownerAsTemporaryObject != nullptr; }

		Concurrency::SpinLock & access_owners_lock() const { an_assert(owner); return owner->access_lock(); } // check access_lock()

	public:
		virtual void ready_to_activate(); // do long tasks before activating
		virtual void activate(); // initialised/reset? activate!
		virtual void reset(); // go back to default state
		virtual void setup_with(ModuleData const * _moduleData); // read parameters from moduleData
		virtual void on_owner_destroy() {}

		void resetup() { setup_with(moduleData); } // will setup the module again, useful if we alter variables and want the auto setup to be processed again

		void initialise_if_required() { if (!initialised) initialise(); }
		virtual void initialise() { initialised = true; } // post setup
		bool is_initialised() const { return initialised; }
		bool is_ready_to_activate() const { return readyToActivate; }
		bool was_activated() const { return activated; }

		virtual bool does_require(Concurrency::ImmediateJobFunc _jobFunc) const { return true; }

	public: // WheresMyPoint::IOwner
		override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
		override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
		override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const; // shouldn't be called!

		override_ WheresMyPoint::IOwner* get_wmp_owner() { return owner; }

	private: friend class ModuleData;
		SimpleVariableStorage const & get_wmp_variables() const { return wmpVariables; }

	private:
		ModuleData const * moduleData;
		IModulesOwner* owner;
		Object* ownerAsObject;
		TemporaryObject* ownerAsTemporaryObject;
		bool initialised = false;
		bool readyToActivate = false;
		bool activated = false;

		SimpleVariableStorage wmpVariables; // modules own variables, used during init etc
	};

};
