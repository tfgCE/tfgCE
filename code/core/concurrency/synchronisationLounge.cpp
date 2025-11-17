#include "synchronisationLounge.h"

#include "spinLock.h"

#include "..\math\mathUtils.h"
#include "..\timeUtils.h"
#include "..\utils.h"

#include "..\containers\array.h"

#include "..\memory\memory.h"

#include "threadSystemUtils.h"
#include "threadManager.h"

//

using namespace Concurrency;

//

#ifdef AN_WINDOWS
// based on: http://msdn.microsoft.com/en-us/library/windows/desktop/ms686903(v=vs.85).aspx

#include <Windows.h>

struct Concurrency::SynchronisationLoungeImpl
{
private:
	SpinLock offSystemJobSpinLock; // just in case there are more, we accept being stuck in a spin lock for this one, though, as it should be a rare event

public:
	SynchronisationLoungeImpl()
	{
		InitializeConditionVariable(&wakeUpCall);
#ifdef AN_CHECK_MEMORY_LEAKS
		supress_memory_leak_detection();
#endif
		lounges.set_size(ThreadSystemUtils::get_number_of_cores() + 1 /* extra one for offsystem jobs, shouldn't be more than one! */);
#ifdef AN_CHECK_MEMORY_LEAKS
		resume_memory_leak_detection();
#endif
		for_every(lounge, lounges)
		{
			InitializeCriticalSection(lounge);
		}
	}

	~SynchronisationLoungeImpl()
	{
#ifdef AN_CHECK_MEMORY_LEAKS
		supress_memory_leak_detection();
		lounges.clear_and_prune(); // clear it this way to prevent from deleting a non registered memory pointer
		resume_memory_leak_detection();
#endif
	}

	bool rest(float _forTime = 0.0f)
	{
		int currentThreadId = ThreadManager::get_current_thread_id(true);
		if (currentThreadId == NONE)
		{
			offSystemJobSpinLock.acquire();
			currentThreadId = lounges.get_size() - 1; // extra one for offsystem jobs (shared, via spin lock)
		}
		CRITICAL_SECTION & lounge = lounges[currentThreadId];
		bool result;
		EnterCriticalSection(&lounge); // enter and while sleeping we will leave it
		result = SleepConditionVariableCS(&wakeUpCall, &lounge, _forTime <= 0.0f ? INFINITE : (int)(min<float>(0.001f, _forTime) * 1000.0f)) != 0;
		LeaveCriticalSection(&lounge); // we've entered it while we were sleeping
		if (currentThreadId == NONE)
		{
			offSystemJobSpinLock.release();
		}
		return result;
	}

	void wake_up_all()
	{
		WakeAllConditionVariable(&wakeUpCall); // wake all threads!
	}

private:
	CONDITION_VARIABLE wakeUpCall;
	Array<CRITICAL_SECTION> lounges;
};
#else
#ifdef AN_LINUX_OR_ANDROID
struct Concurrency::SynchronisationLoungeImpl
{
private:
	SpinLock offSystemJobSpinLock; // just in case there are more, we accept being stuck in a spin lock for this one, though, as it should be a rare event

public:
	SynchronisationLoungeImpl()
	{
		pthread_cond_init(&wakeUpCall, nullptr);
#ifdef AN_CHECK_MEMORY_LEAKS
		supress_memory_leak_detection();
#endif
		lounges.set_size(ThreadSystemUtils::get_number_of_cores() + 1 /* extra one for offsystem jobs, shouldn't be more than one! */);
#ifdef AN_CHECK_MEMORY_LEAKS
		resume_memory_leak_detection();
#endif
		for_every(lounge, lounges)
		{
			pthread_mutex_init(lounge, nullptr);
		}
	}

	~SynchronisationLoungeImpl()
	{
		pthread_cond_destroy(&wakeUpCall);
		for_every(lounge, lounges)
		{
			pthread_mutex_destroy(lounge);
		}
	}

	bool rest(float _forTime = 0.0f)
	{
		int currentThreadId = ThreadManager::get_current_thread_id(true);
		if (currentThreadId == NONE)
		{
			offSystemJobSpinLock.acquire();
			currentThreadId = lounges.get_size() - 1; // extra one for offsystem jobs (shared, via spin lock)
		}
		pthread_mutex_t& lounge = lounges[currentThreadId];
		bool result;
		pthread_mutex_lock(&lounge); // enter and while sleeping we will leave it
		if (_forTime <= 0.0f)
		{
			result = pthread_cond_wait(&wakeUpCall, &lounge) != 0;
		}
		else
		{
			timespec waitingTime = prepare_timespec(_forTime);
			result = pthread_cond_timedwait(&wakeUpCall, &lounge, &waitingTime) != 0;
		}
		pthread_mutex_unlock(&lounge); // we've entered it while we were sleeping
		if (currentThreadId == NONE)
		{
			offSystemJobSpinLock.release();
		}
		return result;
	}

	void wake_up_all()
	{
		pthread_cond_broadcast(&wakeUpCall); // wake all threads!
	}

private:
	pthread_cond_t wakeUpCall;
	Array<pthread_mutex_t> lounges;
};
#else
#error TODO implement synchronisation lounge
#endif
#endif

SynchronisationLounge::SynchronisationLounge()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	supress_memory_leak_detection();
#endif
	impl = new SynchronisationLoungeImpl();
#ifdef AN_CHECK_MEMORY_LEAKS
	resume_memory_leak_detection();
#endif
}

SynchronisationLounge::~SynchronisationLounge()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	supress_memory_leak_detection();
#endif
	delete impl;
#ifdef AN_CHECK_MEMORY_LEAKS
	resume_memory_leak_detection();
#endif
}

bool SynchronisationLounge::rest(float _forTime)
{
	return impl->rest(_forTime);
}

bool SynchronisationLounge::rest_a_little()
{
	return rest(0.001f);
}

void SynchronisationLounge::wake_up_all()
{
	impl->wake_up_all();
}
