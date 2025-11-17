#pragma once

#include "..\fastCast.h"
#include "..\memory\refCountObject.h"

#include "sensations.h"

namespace PhysicalSensations
{
	interface_class IPhysicalSensations
	: public RefCountObject
	{
		FAST_CAST_DECLARE(IPhysicalSensations);
		FAST_CAST_END();

	public:
		static IPhysicalSensations* get() { return s_instance.get() && s_instance->is_ok()? s_instance.get() : nullptr; }
		static IPhysicalSensations* get_as_is() { return s_instance.get(); }

		static void initialise(bool _autoInit = false); // we still need to call async_init! or do_async_init_thread_func!
		static void terminate();

		static void do_async_init_thread_func(void* _data); // if there is an instance

		IPhysicalSensations();
		virtual ~IPhysicalSensations();

		virtual void async_init() {} // this is a method to initialise the system asynchronously

		bool is_ok() const { return isOk; }

		Sensation::ID start_sensation(SingleSensation const & _sensation);
		Sensation::ID start_sensation(OngoingSensation const& _sensation);
		void stop_sensation(Sensation::ID _id);
		void stop_all_sensations();

		void advance(float _deltaTime);

	protected:
		virtual void internal_start_sensation(REF_ SingleSensation & _sensation) = 0;
		virtual void internal_start_sensation(REF_ OngoingSensation & _sensation) = 0;
		virtual void internal_sustain_sensation(Sensation::ID _id) {}
		virtual void internal_stop_sensation(Sensation::ID _id) = 0;
		virtual bool internal_stop_all_sensations() { return false; } // returns true if stopped all in a single call, otherwise they will be stopped one by one
		virtual void internal_advance(float _deltaTime) {}

	protected:
		static RefCountObjectPtr<IPhysicalSensations> s_instance;

		bool isOk = false;

		Concurrency::SpinLock sensationsLock = Concurrency::SpinLock(TXT("PhysicalSensations.IPhysicalSensations.sensationsLock"));

		Array<OngoingSensation> ongoingSensations;

	private:
		static Sensation::ID nextSensationID;
		static Concurrency::SpinLock nextSenstationIDLock;

		Sensation::ID get_next_sensation_id();
	};

};
