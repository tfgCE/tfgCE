#pragma once

#include "concurrencyStats.h"

#include "..\globalDefinitions.h"

#include "..\performance\performanceTimer.h"

struct Name;

#define SYSTEM_SPIN_LOCK true

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
#define SPIN_LOCK_SYSTEM_MEMBER
#else
#ifdef AN_PERFORMANCE_MEASURE
#define SPIN_LOCK_SYSTEM_MEMBER
#endif
#endif

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
#define STORE_SPIN_LOCK_LOCK_REASON
#endif

#define WAIT_LIMIT 0.0002f

namespace Concurrency
{

	class SpinLock
	{
	public:
#ifdef AN_CONCURRENCY_STATS
		static void report_performance_measure_lock(Performance::Timer const & _timer);
#endif			

		SpinLock();
		explicit SpinLock(bool _systemLock);
		explicit SpinLock(tchar const * _info, float _waitLimit = WAIT_LIMIT);
		SpinLock(SpinLock const& _other); // don't copy, just bring functionality over
		SpinLock& operator=(SpinLock const& _other); // don't copy, just bring functionality over

		inline bool is_locked() const { return lock == s_locked; }
		inline bool is_locked_on_this_thread() const;

		inline bool acquire_if_not_locked(tchar const* _reason = nullptr);
		inline void acquire(tchar const* _reason = nullptr);
		inline void release();

#ifdef AN_CONCURRENCY_STATS
		void do_not_report_stats() { reportStats = false; }
#endif

	private:
#ifdef AN_WINDOWS
		volatile uint32 lock;
		static uint32 s_locked;
		static uint32 s_available;
#else
#ifdef AN_LINUX_OR_ANDROID
		std::atomic_bool lock;
		static bool s_locked;
		static bool s_available;
#else
#error implement
#endif
#endif
#ifdef AN_CONCURRENCY_STATS
		bool reportStats = true;
#endif
#ifdef SPIN_LOCK_SYSTEM_MEMBER
		bool systemLock = false;
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		tchar const* info = nullptr;
		float waitLimit = WAIT_LIMIT;
#endif
#ifdef STORE_SPIN_LOCK_LOCK_REASON
		tchar const* lockReason = nullptr; // stores last or current lock reason
#endif
		volatile int lockedOnThreadSystemId = NONE;

		int get_current_thread_system_id() const;
#ifdef AN_DEVELOPMENT
		void do_assert(bool _condition, tchar const * _text) const;
#endif

	private:
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		void report_performance_limit_exceeded(Performance::Timer const& _timer, tchar const* _wasLockedReason, tchar const* _lockReason) const;
#endif			
	};

	#include "spinLock.inl"

};
