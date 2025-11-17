#include "concurrencyStats.h"

#include "counter.h"
#include "spinLock.h"

using namespace Concurrency;

Stats lastFrameStats;

Counter wasInCounter = 0;
Counter wasInSpinLock = 0;
Counter wasInSemaphore = 0;
Counter wasInMRSWLockRead = 0;
Counter wasInMRSWLockWrite = 0;
Counter waitedInCounter = 0;
Counter waitedInSpinLock = 0;
Counter waitedInSemaphore = 0;
Counter waitedInMRSWLockRead = 0;
Counter waitedInMRSWLockWrite = 0;

void Stats::next_frame()
{
	lastFrameStats.wasInCounter = ::wasInCounter.get(); ::wasInCounter = 0;
	lastFrameStats.wasInSpinLock = ::wasInSpinLock.get(); ::wasInSpinLock = 0;
	lastFrameStats.wasInSemaphore = ::wasInSemaphore.get(); ::wasInSemaphore = 0;
	lastFrameStats.wasInMRSWLockRead = ::wasInMRSWLockRead.get(); ::wasInMRSWLockRead = 0;
	lastFrameStats.wasInMRSWLockWrite = ::wasInMRSWLockWrite.get(); ::wasInMRSWLockWrite = 0;
	lastFrameStats.waitedInCounter = ::waitedInCounter.get(); ::waitedInCounter = 0;
	lastFrameStats.waitedInSpinLock = ::waitedInSpinLock.get(); ::waitedInSpinLock = 0;
	lastFrameStats.waitedInSemaphore = ::waitedInSemaphore.get(); ::waitedInSemaphore = 0;
	lastFrameStats.waitedInMRSWLockRead = ::waitedInMRSWLockRead.get(); ::waitedInMRSWLockRead = 0;
	lastFrameStats.waitedInMRSWLockWrite = ::waitedInMRSWLockWrite.get(); ::waitedInMRSWLockWrite = 0;
}

Stats const & Stats::get_last_frame()
{
	return lastFrameStats;
}

void Stats::waited_in_counter(bool _waited)
{
	::wasInCounter.increase();
	if (_waited)
	{
		::waitedInCounter.increase();
	}
}

void Stats::waited_in_spin_lock(bool _waited)
{
	::wasInSpinLock.increase();
	if (_waited)
	{
		::waitedInSpinLock.increase();
	}
}

void Stats::waited_in_semaphore(bool _waited)
{
	::wasInSemaphore.increase();
	if (_waited)
	{
		::waitedInSemaphore.increase();
	}
}

void Stats::waited_in_mrsw_lock_read(bool _waited)
{
	::wasInMRSWLockRead.increase();
	if (_waited)
	{
		::waitedInMRSWLockRead.increase();
	}
}

void Stats::waited_in_mrsw_lock_write(bool _waited)
{
	::wasInMRSWLockWrite.increase();
	if (_waited)
	{
		::waitedInMRSWLockWrite.increase();
	}
}
