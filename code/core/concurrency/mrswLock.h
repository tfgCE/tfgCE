#pragma once

#include "spinLock.h"
#include "counter.h"
#include "semaphore.h"

#define WAIT_LIMIT 0.0002f

namespace Concurrency
{

	class MRSWLock // multi read single write
	{
	public:
		MRSWLock();
		explicit MRSWLock(tchar const * _info, float _waitLimit = WAIT_LIMIT);
		// they actually do not copy
		MRSWLock(MRSWLock const& _other);
		MRSWLock& operator=(MRSWLock const& _other);

		void acquire_read(tchar const * _reason = nullptr);
		void release_read();

		void acquire_write(tchar const* _reason = nullptr);
		void release_write();

		bool is_acquired_write_on_this_thread() const { return writeLock.is_locked_on_this_thread(); }
		bool is_acquired() const { return writeLock.is_locked_on_this_thread() || accessLock.is_locked_on_this_thread(); }
		bool is_acquired_to_read() const { return activeReaders.get() > 0; }
		
#ifdef AN_CONCURRENCY_STATS
		void do_not_report_stats();
#endif

	private:
		Semaphore readSemaphore;
		Semaphore writeSemaphore;
		SpinLock acquireLock = SpinLock(SYSTEM_SPIN_LOCK);
		SpinLock accessLock = SpinLock(SYSTEM_SPIN_LOCK);
		Counter activeReaders;
		Counter waitingWriters;
		SpinLock writeLock = SpinLock(SYSTEM_SPIN_LOCK); // double confirmation and to check if we're writing
#ifdef AN_CONCURRENCY_STATS
		bool reportStats = true;
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		tchar const* info = nullptr;
		float waitLimit = WAIT_LIMIT;
		tchar const* lockWriteReason = nullptr;
#endif

	private: friend class ThreadManager;
		void slow_safe_acquire_write(tchar const* _reason); // if we are not registered thread, use this, only ThreadManager should use it

	private:
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		void report_performance_limit_exceeded(bool _toWrite, Performance::Timer const& _timer, tchar const* _wasLockedReason, tchar const* _lockReason) const;
#endif			
	};

};
