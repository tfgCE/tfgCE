#pragma once

#include "synchronisationLounge.h"
#include "spinLock.h"

#ifdef AN_WINDOWS
#define SEMAPHORE_USES_SYNCHRONISATION_LOUNGE
#endif

namespace Concurrency
{
	/*
	 *	"allow" - allow waiting to pass through
	 *	"stop" - stop all that want to pass through
	 *	"wait" - wait for allowance or go through if allowed
	 */
	class Semaphore
	{
	public:
		Semaphore(bool _stop = true);

		void allow(); // allows other to pass
		void stop(); // has to be not stopped
		void stop_if_not_stopped();
		void wait();
		void slow_safe_wait(); // if we are not registered thread, use this

		bool occupy(); // if not set (all are allowed), will stop others and return true, if set (already occupied) will return false
		void wait_and_occupy(); // will wait until may occupy

		bool wait_for(float _time); // returns false if time exceeded

		bool should_wait() const; // return true if should wait longer
		void wait_a_little(); // wait a little

#ifdef AN_CONCURRENCY_STATS
		void do_not_report_stats();
#endif

	private:
		SpinLock oneAtATime = SpinLock(SYSTEM_SPIN_LOCK);
#ifdef SEMAPHORE_USES_SYNCHRONISATION_LOUNGE
		SynchronisationLounge lounge;
#endif
		volatile bool semaphore;

#ifdef AN_CONCURRENCY_STATS
		bool reportStats = true;
#endif
	};

};
