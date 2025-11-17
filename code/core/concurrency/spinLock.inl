#ifdef AN_WINDOWS
#include <Windows.h>
#endif

bool SpinLock::is_locked_on_this_thread() const
{
	int threadSystemId = get_current_thread_system_id();
	if (lock)
	{
		return threadSystemId == lockedOnThreadSystemId;
	}
	return false;
}

bool SpinLock::acquire_if_not_locked(tchar const* _reason)
{
	int threadSystemId = get_current_thread_system_id();
#ifdef AN_DEVELOPMENT
	if (lock)
	{
		do_assert(threadSystemId != lockedOnThreadSystemId, TXT("we're locking from the same thread!"));
	}
#endif
#ifdef AN_WINDOWS
	if (InterlockedCompareExchange(&lock, s_locked, s_available) == s_available)
#else
#ifdef AN_LINUX_OR_ANDROID
	bool available = false; // compare_exchange_weak on failure will modify "available" variable, we don't want to modify s_available!
	if (lock.compare_exchange_weak(available, true, std::memory_order_acquire))
#else
	#error implement
#endif
#endif
	{
#ifdef STORE_SPIN_LOCK_LOCK_REASON
		lockReason = _reason;
#endif
		lockedOnThreadSystemId = threadSystemId;
		return true;
	}
	else
	{
		return false;
	}
}

#ifdef AN_CONCURRENCY_STATS
#define SPIN_LOCK_CHECK_WAITED
#else
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
#define SPIN_LOCK_CHECK_WAITED
#endif
#endif

void SpinLock::acquire(tchar const* _reason)
{
	int threadSystemId = get_current_thread_system_id();
#ifdef AN_DEVELOPMENT
	if (lock)
	{
		do_assert(threadSystemId != lockedOnThreadSystemId, TXT("we're locking from the same thread!"));
	}
#endif
#ifdef SPIN_LOCK_CHECK_WAITED
	bool waited = false;
#ifdef AN_PERFORMANCE_MEASURE
	Performance::Timer performanceTimer;
	if (
#ifdef AN_CONCURRENCY_STATS
		reportStats &&
#endif
		!systemLock)
	{
		performanceTimer.start();
	}
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	Performance::Timer performanceLimitTimer;
	if (
#ifdef AN_CONCURRENCY_STATS
		reportStats &&
#endif
		!systemLock && info)
	{
		performanceLimitTimer.start();
	}
#endif
#endif

#ifdef AN_WINDOWS
	while (InterlockedCompareExchange(&lock, s_locked, s_available) != s_available)
#else
#ifdef AN_LINUX_OR_ANDROID
	bool available = false; // compare_exchange_weak on failure will modify "available" variable
	while (! lock.compare_exchange_weak(available, s_locked, std::memory_order_acquire))
#else
#error implement
#endif
#endif
	{
#ifdef AN_LINUX_OR_ANDROID
		available = false;
#endif
#ifdef SPIN_LOCK_CHECK_WAITED
#ifdef AN_CONCURRENCY_STATS
		if (reportStats) // this is to catch when actually waiting
#endif
		{
			waited = true;
		}
#endif
	}

#ifdef STORE_SPIN_LOCK_LOCK_REASON
	tchar const* wasLockedReason = lockReason;
	lockReason = _reason;
#endif

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	if (
#ifdef AN_CONCURRENCY_STATS
		reportStats &&
#endif
		!systemLock && info)
	{
		performanceLimitTimer.stop();
		if (waited && performanceLimitTimer.get_time_ms() > waitLimit)
		{
			report_performance_limit_exceeded(performanceLimitTimer, wasLockedReason, _reason);
		}
	}
#endif
#ifdef AN_CONCURRENCY_STATS
	if (reportStats)
	{
#ifdef AN_PERFORMANCE_MEASURE
		if (! systemLock)
		{
			performanceTimer.stop();
			if (waited)
			{
				report_performance_measure_lock(performanceTimer);
			}
		}
#endif
		Stats::waited_in_spin_lock(waited);
	}
#endif

	lockedOnThreadSystemId = threadSystemId;
}

void SpinLock::release()
{
	lockedOnThreadSystemId = NONE;
#ifdef AN_WINDOWS
	lock = 0;
#else
#ifdef AN_LINUX_OR_ANDROID
	lock.store(s_available, std::memory_order_release);
#else
#error implement
#endif
#endif
}
