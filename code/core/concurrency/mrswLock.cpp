#include "mrswLock.h"

#include "scopedSpinLock.h"

#include "..\debug\debug.h"
#include "..\system\android\androidApp.h"

//

using namespace Concurrency;

//

/**
 *	It's quite naive implementation, but if we would require to use it more and it would appear to be a bottleneck,
 *	it should be investigated to improve.
 */

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
void MRSWLock::report_performance_limit_exceeded(bool _toWrite, Performance::Timer const& _timer, tchar const* _wasLockedReason, tchar const* _lockReason) const
{
#ifdef AN_ANDROID
	if (!::System::Android::App::get().get_activity())
	{
		return;
	}
#endif
	float timeSince = _timer.get_time_ms();
	warn(TXT("[performance guard] [mrsw lock] [%S] exceeded limit of %.2fms (%.2fms) : %S [%S -> %S]"),
		_toWrite? TXT("write") : TXT("read"),
		waitLimit * 1000.0f, timeSince * 1000.0f,
		safe_tchar_ptr(info), safe_tchar_ptr(_wasLockedReason), safe_tchar_ptr(_lockReason));
}
#endif			

MRSWLock::MRSWLock()
: readSemaphore(false)
, writeSemaphore(false)
{
}

MRSWLock::MRSWLock(tchar const* _info, float _waitLimit)
: readSemaphore(false)
, writeSemaphore(false)
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
, info(_info)
, waitLimit(_waitLimit)
#endif
{
}

MRSWLock::MRSWLock(MRSWLock const & _other)
: readSemaphore(false)
, writeSemaphore(false)
{
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	info = _other.info;
	waitLimit = _other.waitLimit;
#endif
}

MRSWLock & MRSWLock::operator=(MRSWLock const & _other)
{
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	info = _other.info;
	waitLimit = _other.waitLimit;
#endif
	return *this;
}

void MRSWLock::acquire_read(tchar const* _reason)
{
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	bool waited = false;
	Performance::Timer performanceLimitTimer;
	tchar const* waitedReason = nullptr;
	if (info)
	{
		performanceLimitTimer.start();
	}
#endif
	{
		ScopedSpinLock lockAcquire(acquireLock, _reason); // to make sure we're next to do stuff - we will release it if we have to wait on readSemaphore for writers to finish their writing

#ifdef AN_CONCURRENCY_STATS
		if (reportStats)
		{
			Stats::waited_in_mrsw_lock_read(readSemaphore.should_wait());
		}
#endif

		// to allow waiting writers to do their job, note that writers when will acquire acquireLock, they will stay there waiting for all readers to be gone
		while (readSemaphore.should_wait())
		{
			acquireLock.release();
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
			waited = true;
#endif
			readSemaphore.wait();
			acquireLock.acquire(_reason);
		}

		ScopedSpinLock lockAccess(accessLock, _reason);
		writeSemaphore.stop_if_not_stopped(); // can't write now!
		activeReaders.increase(); // one more reader reading!

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		waitedReason = lockWriteReason;
		lockWriteReason = nullptr;
#endif
	}
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	if (info)
	{
		performanceLimitTimer.stop();
		if (waited && performanceLimitTimer.get_time_ms() > waitLimit)
		{
			report_performance_limit_exceeded(false, performanceLimitTimer, waitedReason, _reason);
		}
	}
#endif
}

void MRSWLock::release_read()
{
	ScopedSpinLock lockAccess(accessLock, TXT("release read"));
	activeReaders.decrease();
	if (activeReaders == 0)
	{
		writeSemaphore.allow(); // let writers write as there is no active reader
	}
}

void MRSWLock::acquire_write(tchar const* _reason)
{
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	bool waited = false;
	Performance::Timer performanceLimitTimer;
	tchar const* waitedReason = nullptr;
	if (info)
	{
		performanceLimitTimer.start();
	}
#endif
	{
		{
			ScopedSpinLock lockAccess(accessLock, _reason); // lock to prevent release_write
			readSemaphore.stop_if_not_stopped(); // so readers will get stuck in their loop
			waitingWriters.increase();
		}

		ScopedSpinLock lockAcquire(acquireLock, _reason); // to make sure we're next to do stuff - readers will lock and release if they have to

#ifdef AN_CONCURRENCY_STATS
		if (reportStats)
		{
			Stats::waited_in_mrsw_lock_write(writeSemaphore.should_wait());
		}
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		waited = writeSemaphore.should_wait();
		waitedReason = lockWriteReason;
#endif

		writeSemaphore.wait(); // wait until we can write (readers will allow us with last active reader gone, other readers

		ScopedSpinLock lockAccess(accessLock, _reason);
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		lockWriteReason = _reason;
#endif
		waitingWriters.decrease();
		writeSemaphore.stop(); // to block any other writers
		writeLock.acquire();
	}
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	if (info)
	{
		performanceLimitTimer.stop();
		if (waited && performanceLimitTimer.get_time_ms() > waitLimit)
		{
			report_performance_limit_exceeded(true, performanceLimitTimer, waitedReason, _reason);
		}
	}
#endif
}

void MRSWLock::slow_safe_acquire_write(tchar const* _reason)
{
	{
		ScopedSpinLock lockAccess(accessLock, _reason); // lock to prevent release_write
		readSemaphore.stop_if_not_stopped(); // so readers will get stuck in their loop
		waitingWriters.increase();
	}

	ScopedSpinLock lockAcquire(acquireLock, _reason); // to make sure we're next to do stuff - readers will lock and release if they have to

#ifdef AN_CONCURRENCY_STATS
	if (reportStats)
	{
		Stats::waited_in_mrsw_lock_write(writeSemaphore.should_wait());
	}
#endif

	writeSemaphore.slow_safe_wait(); // wait until we can write (readers will allow us with last active reader gone, other readers

	ScopedSpinLock lockAccess(accessLock, _reason);
	waitingWriters.decrease();
	writeSemaphore.stop(); // to block any other writers
	writeLock.acquire();
}

void MRSWLock::release_write()
{
	ScopedSpinLock lockAccess(accessLock, TXT("release write"));
	writeLock.release();
	if (waitingWriters == 0)
	{
		readSemaphore.allow();
	}
	writeSemaphore.allow();
}

#ifdef AN_CONCURRENCY_STATS
void MRSWLock::do_not_report_stats()
{
	reportStats = false;
	readSemaphore.do_not_report_stats();
	writeSemaphore.do_not_report_stats();
	acquireLock.do_not_report_stats();
	accessLock.do_not_report_stats();
}
#endif
