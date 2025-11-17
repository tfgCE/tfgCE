#include "threadSystemUtils.h"

#include "..\mainConfig.h"

#ifdef AN_LINUX_OR_ANDROID
#include <unistd.h>
#include <sched.h>
#endif

#ifdef AN_WINDOWS
#include <Windows.h>
#endif

#include <time.h>

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Concurrency;

//

int32 ThreadSystemUtils::get_current_thread_system_id()
{
	int32 systemId = 0;
#ifdef AN_WINDOWS
	systemId = GetCurrentThreadId();
	// TODO map it to better value starting with 0?
#else
#ifdef AN_LINUX_OR_ANDROID
	systemId = gettid();
#else
	#error TODO implement get current thread id
#endif
#endif
	return systemId;
}

int32 ThreadSystemUtils::get_number_of_cores(bool _allAvailable)
{
	if (!_allAvailable && MainConfig::global().get_thread_number_forced())
	{
		return MainConfig::global().get_thread_number_forced();
	}
	static int32 numberOfProcessors = 0;
	if (numberOfProcessors == 0)
	{
#ifdef AN_WINDOWS
		// based on: http://stackoverflow.com/questions/150355/programmatically-find-the-number-of-cores-on-a-machine
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		numberOfProcessors = systemInfo.dwNumberOfProcessors;
#else
#ifdef AN_LINUX_OR_ANDROID
		numberOfProcessors = sysconf(_SC_NPROCESSORS_ONLN);
#else
		#error TODO implement get number of processors
#endif
#endif
	}
	return numberOfProcessors;
}

void ThreadSystemUtils::yield()
{
#ifdef AN_LINUX_OR_ANDROID
	sched_yield();
#endif
}