#include "semaphore.h"

#include "scopedSpinLock.h"

#include "..\math\mathUtils.h"
#include "..\debug\debug.h"
#include "..\system\core.h"
#include "..\system\timeStamp.h"

using namespace Concurrency;

Semaphore::Semaphore(bool _stop)
: semaphore(_stop)
{
}

void Semaphore::allow()
{
	ScopedSpinLock lock(oneAtATime);
	semaphore = false;
#ifdef SEMAPHORE_USES_SYNCHRONISATION_LOUNGE
	lounge.wake_up_all();
#endif
}

void Semaphore::stop()
{
	ScopedSpinLock lock(oneAtATime);
	an_assert(!semaphore);
	semaphore = true;
}

void Semaphore::stop_if_not_stopped()
{
	ScopedSpinLock lock(oneAtATime);
	semaphore = true;
}

bool Semaphore::occupy()
{
	ScopedSpinLock lock(oneAtATime);
	if (semaphore)
	{
		return false;
	}
	semaphore = true;
	return true;
}

void Semaphore::wait_and_occupy()
{
	while (true)
	{
		if (occupy())
		{
			// success!
			return;
		}
		wait_a_little();
	}
}

void Semaphore::wait()
{
#ifdef AN_CONCURRENCY_STATS
	if (reportStats)
	{
		Stats::waited_in_semaphore(should_wait());
	}
#endif
	while (should_wait())
	{
		wait_a_little();
	}
}

bool Semaphore::wait_for(float _time)
{
#ifdef AN_CONCURRENCY_STATS
	if (reportStats)
	{
		Stats::waited_in_semaphore(should_wait());
	}
#endif
	::System::TimeStamp ts;
	while (should_wait())
	{
		wait_a_little();
		if (ts.get_time_since() >= _time)
		{
			return false;
		}
	}
	return true;
}

void Semaphore::slow_safe_wait()
{
#ifdef AN_CONCURRENCY_STATS
	if (reportStats)
	{
		Stats::waited_in_semaphore(should_wait());
	}
#endif
	while (should_wait())
	{
		::System::Core::sleep_u(1);
	}
}

bool Semaphore::should_wait() const
{
	return semaphore;
}

void Semaphore::wait_a_little()
{
#ifdef SEMAPHORE_USES_SYNCHRONISATION_LOUNGE
	lounge.rest_a_little();
#else
	::System::Core::sleep_u(1);
#endif
}

#ifdef AN_CONCURRENCY_STATS
void Semaphore::do_not_report_stats()
{
	oneAtATime.do_not_report_stats();
	reportStats = false;
}
#endif
