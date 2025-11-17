#include "threadManager.h"
#include "threadSystemUtils.h"
#include "scopedSpinLock.h"

#include "..\utils.h"

#include "..\mainConfig.h"

#include "..\random\random.h"
#include "..\system\core.h"

#ifdef AN_LINUX_OR_ANDROID
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <sys/resource.h>
#endif

#define AN_THREAD_ALLOW_AFFINITY_MASK
#define AN_THREAD_ALLOW_PRIORITIES

#ifdef AN_WINDOWS
#include <Windows.h>
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Concurrency;

//

int mainThreadPriorityNice = 0; // used as a reference for thread priorities, when setting them up

//

ThreadManager* ThreadManager::s_currentThreadManager = nullptr;

ThreadManager::ThreadManager()
: mainThread(*this)
{
#ifdef AN_CONCURRENCY_STATS
	threadsLock.do_not_report_stats();
#endif

	an_assert(s_currentThreadManager == nullptr, TXT("there should be only one thread manager"));
	s_currentThreadManager = this;

	cpuCount = ThreadSystemUtils::get_number_of_cores();
#ifdef AN_WINDOWS
	cpuFromLast = true;
	cpuOffset = 0;// Random::get_int_from_range(1, cpuCount - 1); // try to avoid putting on cpu #0
#else
	cpuFromLast = false;
	cpuOffset = 0;
#endif

#ifdef AN_LINUX_OR_ANDROID
	//mainThreadPriorityNice = getpriority(PRIO_PROCESS, 0);
	mainThreadPriorityNice = -10; // hardcoded for time being
#endif

	// register main thread
	register_thread(&mainThread, true);
}

ThreadManager::~ThreadManager()
{
	an_assert(s_currentThreadManager == this, TXT("there should be only one thread manager"));
	s_currentThreadManager = nullptr;
}

Thread* ThreadManager::create_thread(Thread::ThreadFunc _func, void * _data, Optional<ThreadPriority::Type> const& _threadPriority, bool _offSystem)
{
	return new Thread(*this, _func, _data, _threadPriority, _offSystem);
}

void ThreadManager::log_thread_info(LogInfoContext & _log, Optional<bool> const& _header, Optional<int> const & _tid, tchar const* _info)
{
	if (_header.get(false))
	{
		_log.log(TXT("    _tid_ cpu prio nice policy"));
		return;
	}
#ifdef AN_LINUX_OR_ANDROID
	int tid = _tid.get(gettid());
	int pid = _tid.is_set()? 0 : getpid();
	Optional<int> oncpu = sched_getcpu();
#ifdef PROFILE_PERFORMANCE
	if (_tid.is_set() && get())
	{
		for_count(int, tIdx, get()->threadCount)
		{
			oncpu = get()->threads[tIdx].onCPU;
		}
	}
#endif
	int policy = sched_getscheduler(tid);
	int priority = NONE;
	{
		sched_param param;
		param.sched_priority = NONE;
		if (sched_getparam(tid, &param) == 0)
		{
			priority = param.sched_priority;
		}
	}
	int priorityNice = getpriority(PRIO_PROCESS, tid);
	if (_header.is_set())
	{
		_log.log(TXT("%S %5i %3i %4i %4i %i (%X)"),
			_info? _info : TXT("thread"),
			tid, oncpu.get(), priority, priorityNice, policy & 0xffff, policy);
	}
	else
	{
		_log.log(TXT("[%S] tid:%5i pid:%5i cpu:%2i prio:%2i nice:%3i policy:%i (%X)"),
			_info ? _info : TXT("thread"),
			tid, pid, oncpu.get(), priority, priorityNice, policy & 0xffff, policy);
	}
#endif
}

#ifdef AN_LINUX_OR_ANDROID
void set_priority_nice(int _priorityNice)
{
	int tid = gettid();
	int pid = getpid();
	output(TXT("request thread (tid:%i pid:%i) priority nice to %i"), tid, pid, _priorityNice);
	int callRet = setpriority(PRIO_PROCESS, 0, _priorityNice);
	if (callRet != 0)
	{
		error(TXT("can't set priority nice to %i [error:%i]"), _priorityNice, errno);
	}
	else
	{
		output(TXT("set thread priority nice %i"), _priorityNice);
	}
}

void set_sched_policy_priority(int _policy, int _priority = 0)
{
	int minPriority = sched_get_priority_min(_policy & 0xffff);
	int maxPriority = sched_get_priority_max(_policy & 0xffff);
	int tid = gettid();
	int pid = getpid();
	output(TXT("request thread (tid:%i pid:%i) policy to %i (%X) priority to %i [%i-%i]"), tid, pid, _policy & 0xffff, _policy, _priority, minPriority, maxPriority);
	sched_param schedParam;
	schedParam.sched_priority = clamp(_priority, minPriority, maxPriority);
	int callRet = sched_setscheduler(tid, _policy, &schedParam);
	if (callRet != 0)
	{
		error(TXT("can't set sched policy %i [error:%i]"), _policy, errno);
	}
	else
	{
		output(TXT("set thread policy to %i priority to %i"), _policy, schedParam.sched_priority);
		if (schedParam.sched_priority != _priority)
		{
			warn(TXT("while setting sched policy %i, priority was changed %i -> %i"), _policy, _priority, schedParam.sched_priority);
		}
	}
	{
		int policy = sched_getscheduler(tid);
		output(TXT("thread policy is %X"), policy);
	}
	{
		sched_param param;
		param.sched_priority = NONE;
		if (sched_getparam(tid, &param) == 0)
		{
			output(TXT("thread priority is %X"), param.sched_priority);
		}
	}
}
#endif

void set_thread_priority(ThreadPriority::Type threadPriority
#ifdef AN_WINDOWS
	, HANDLE threadHandle
#endif
)
{
	// relative to main thread!
	if (threadPriority == ThreadPriority::Highest)
	{
#ifdef AN_WINDOWS
		SetThreadPriority(threadHandle, THREAD_PRIORITY_HIGHEST);
#else
#ifdef AN_LINUX_OR_ANDROID
		int priorityNice = getpriority(PRIO_PROCESS, 0);
		int newPriorityNice = mainThreadPriorityNice - 10;
		output(TXT("set thread priority nice (%i) -> highest %i (main: %i) to %i"), gettid(), priorityNice, mainThreadPriorityNice, newPriorityNice);
		//set_sched_policy_priority(SCHED_FIFO | SCHED_RESET_ON_FORK, 50);
		set_priority_nice(newPriorityNice);
#else
#error TODO implement bind thread to processor
#endif
#endif
	}
	else if (threadPriority == ThreadPriority::Higher)
	{
#ifdef AN_WINDOWS
		SetThreadPriority(threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);
#else
#ifdef AN_LINUX_OR_ANDROID
		int priorityNice = getpriority(PRIO_PROCESS, 0);
		int newPriorityNice = mainThreadPriorityNice - 5;
		output(TXT("set thread priority nice (%i) -> higher %i (main: %i) to %i"), gettid(), priorityNice, mainThreadPriorityNice, newPriorityNice);
		//set_sched_policy_priority(SCHED_FIFO | SCHED_RESET_ON_FORK, 30);
		set_priority_nice(newPriorityNice);
#else
#error TODO implement bind thread to processor
#endif
#endif
	}
	else if (threadPriority == ThreadPriority::Normal)
	{
#ifdef AN_WINDOWS
		SetThreadPriority(threadHandle, THREAD_PRIORITY_NORMAL);
#else
#ifdef AN_LINUX_OR_ANDROID
		int priorityNice = getpriority(PRIO_PROCESS, 0);
		int newPriorityNice = mainThreadPriorityNice;
		output(TXT("set thread priority nice (%i) -> normal %i (main: %i) to %i"), gettid(), priorityNice, mainThreadPriorityNice, newPriorityNice);
		//set_sched_policy_priority(SCHED_NORMAL | SCHED_RESET_ON_FORK, 20);
		set_priority_nice(newPriorityNice);
#else
#error TODO implement bind thread to processor
#endif
#endif
	}
	else if (threadPriority == ThreadPriority::Lower)
	{
#ifdef AN_WINDOWS
		SetThreadPriority(threadHandle, THREAD_PRIORITY_BELOW_NORMAL);
#else
#ifdef AN_LINUX_OR_ANDROID
		int priorityNice = getpriority(PRIO_PROCESS, 0);
		int newPriorityNice = mainThreadPriorityNice + 5;
		output(TXT("set thread priority nice (%i) -> lower %i (main: %i) to %i"), gettid(), priorityNice, mainThreadPriorityNice, newPriorityNice);
		//set_sched_policy_priority(SCHED_NORMAL | SCHED_RESET_ON_FORK, 10);
		set_priority_nice(newPriorityNice);
#else
#error TODO implement bind thread to processor
#endif
#endif
	}
	else if (threadPriority == ThreadPriority::Lowest)
	{
#ifdef AN_WINDOWS
		SetThreadPriority(threadHandle, THREAD_PRIORITY_LOWEST);
#else
#ifdef AN_LINUX_OR_ANDROID
		int priorityNice = getpriority(PRIO_PROCESS, 0);
		int newPriorityNice = mainThreadPriorityNice + 15;
		output(TXT("set thread priority nice (%i) -> lowest %i (main: %i) to %i"), gettid(), priorityNice, mainThreadPriorityNice, newPriorityNice);
		//set_sched_policy_priority(SCHED_NORMAL | SCHED_RESET_ON_FORK, 0);
		set_priority_nice(newPriorityNice);
#else
#error TODO implement bind thread to processor
#endif
#endif
	}

#ifdef AN_LINUX_OR_ANDROID
	{
		int priorityNice = getpriority(PRIO_PROCESS, 0);
		output(TXT("thread priority nice after change (%i) = %i"), gettid(), priorityNice);
	}
#endif
}

void ThreadManager::register_thread(Thread* _thread, bool _main)
{
	ScopedSpinLock lock(threadsLock);

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();
#ifdef AN_WINDOWS
	auto currentThread = GetCurrentThread();
#endif

	// add only if not registered
	bool alreadyRegistered = false;
	for_count(int, threadIdx, threadCount)
	{
		if (threads[threadIdx].threadSystemId == systemId)
		{
			alreadyRegistered = true;
		}
	}
	if (! alreadyRegistered)
	{
		// register in a free spot
		for_count(int, threadIdx, threadCount)
		{
			auto& thread = threads[threadIdx];
			if (!thread.alive)
			{
				thread.threadSystemId = systemId;
				thread.thread = _thread;
				thread.mainThread = true;
				alreadyRegistered = true;
			}
		}
		if (!alreadyRegistered)
		{
			// store
			threads[threadCount] = RegisteredThread(systemId, _thread, _main);
			++threadCount;
		}
	}

#ifdef AN_THREAD_ALLOW_AFFINITY_MASK
	if (MainConfig::global().should_threads_use_affinity_mask())
	{
		int beOnCPU = threadCount;
		beOnCPU = (beOnCPU + cpuOffset) % cpuCount;
		if (cpuFromLast)
		{
			beOnCPU = cpuCount - 1 - beOnCPU;
		}
#ifdef AN_WINDOWS
		DWORD_PTR prevMask = SetThreadAffinityMask(currentThread, bit(beOnCPU));
		if (prevMask == 0)
		{
			error(TXT("could not set affinity mask for thread"));
		}
#else
#ifdef AN_LINUX_OR_ANDROID
		cpu_set_t mask;
		int status;

		CPU_ZERO(&mask);
		CPU_SET(beOnCPU, &mask);
		status = sched_setaffinity(0, sizeof(mask), &mask);
#else
#error TODO implement bind thread to processor
#endif
#endif
	}
#endif
#ifdef AN_THREAD_ALLOW_PRIORITIES
	if (_thread->preferedThreadPriority.is_set())
	{
		// preferred priority
		output(TXT("set thread priority using provided (%i)"), _thread->threadSystemId);
		set_thread_priority(_thread->preferedThreadPriority.get()
#ifdef AN_WINDOWS
			, currentThread
#endif
		);
	}
	else if (_main)
	{
		// main thread
		output(TXT("set thread priority for main thread (%i)"), _thread->threadSystemId);
		set_thread_priority(MainConfig::global().get_main_thread_priority()
#ifdef AN_WINDOWS
			, currentThread
#endif
		);
	}
	else
	{
		// other threads
		output(TXT("set thread priority for other thread (%i)"), _thread->threadSystemId);
		set_thread_priority(MainConfig::global().get_other_threads_priority()
#ifdef AN_WINDOWS
			, currentThread
#endif
		);
	}
#endif

	{
		LogInfoContext log;
		log_thread_info(log);
		log.output_to_output();
	}
}

void ThreadManager::unregister_thread(Thread* _thread)
{
	ScopedSpinLock lock(threadsLock);

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();

	for_count(int, threadIdx, threadCount)
	{
		auto& thread = threads[threadIdx];
		if ((_thread == nullptr && thread.threadSystemId == systemId) ||
			(_thread && thread.thread == _thread))
		{
			thread.alive = false;
		}
	}
}

void ThreadManager::kill_other_threads(bool _immediate)
{
	ScopedSpinLock lock(threadsLock);

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();

	for_count(int, threadIdx, threadCount)
	{
		auto& thread = threads[threadIdx];
		if (thread.threadSystemId != systemId &&
			thread.alive)
		{
			if (thread.mainThread)
			{
				warn(TXT("killing main thread may lead to a crash (as some stuff is stored in that thread!)"));
			}
			if (auto* t = thread.thread.get())
			{
				t->kill(_immediate);
			}
		}
	}
}

void ThreadManager::suspend_kill_other_threads()
{
	ScopedSpinLock lock(threadsLock);

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();

	for_count(int, threadIdx, threadCount)
	{
		auto& thread = threads[threadIdx];
		if (thread.threadSystemId != systemId &&
			thread.alive)
		{
#ifdef AN_LINUX_OR_ANDROID
			if (thread.mainThread)
			{
				output(TXT("can't suspend/kill main thread"));
			}
			else
#endif
			if (auto* t = thread.thread.get())
			{
				t->suspend_kill();
			}
		}
	}
}

int32 ThreadManager::get_current_thread_id(bool _allowNone)
{
	if (!get())
	{
		//an_assert(false, TXT("thread manager not available"));
		return 0; // assume main thread
	}

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();

	for_count(int, tryNo, 4)
	{
		{
			//Concurrency::ScopedMRSWLockRead lock(get()->threadsLock);
			RegisteredThread const* t = get()->threads;
			for_count(int, threadIdx, get()->threadCount)
			{
				if (t->threadSystemId == systemId)
				{
					return threadIdx;
				}
				++t;
			}
		}
		::System::Core::sleep(0.1f); // let other threads register - we all should be created in the same moment
	}
	an_assert_immediate(_allowNone, TXT("thread not registered"));
	return NONE;
}

bool ThreadManager::is_main_thread()
{
	if (!get())
	{
		return true;
	}

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();
	return systemId == get()->mainThread.threadSystemId;
}

bool ThreadManager::is_current_thread_registered()
{
	if (!get())
	{
		//an_assert(false, TXT("thread manager not available"));
		return true; // assume main thread
	}

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();

	RegisteredThread const* t = get()->threads;
	for_count(int, threadIdx, get()->threadCount)
	{
		if (t->threadSystemId == systemId)
		{
			return true;
		}
		++t;
	}

	return false;
}

int32 ThreadManager::get_thread_count()
{
	if (!get())
	{
		an_assert(false, TXT("thread manager not available"));
		return 0;
	}
	return get()->threadCount;
}

bool ThreadManager::is_thread_idx_valid(int _threadIdx) const
{
	if (!get())
	{
		an_assert(false, TXT("thread manager not available"));
		return false;
	}
	if (_threadIdx < 0 || _threadIdx >= get()->threadCount)
	{
		an_assert(false, TXT("thread out of bounds"));
		return false;
	}
	return true;
}

int32 ThreadManager::get_thread_system_id(int _threadIdx)
{
	if (!is_thread_idx_valid(_threadIdx))
	{
		return NONE;
	}
	return get()->threads[_threadIdx].threadSystemId;
}

int32 ThreadManager::get_thread_policy(int _threadIdx)
{
	if (!is_thread_idx_valid(_threadIdx))
	{
		return NONE;
	}
	auto& t = get()->threads[_threadIdx];
	int policy = NONE;
#ifdef AN_WINDOWS
	// implement?
#else
#ifdef AN_LINUX_OR_ANDROID
	policy = sched_getscheduler(t.threadSystemId);
#else
#error TODO implement bind thread to processor
#endif
#endif
	return policy;
}
int32 ThreadManager::get_thread_priority(int _threadIdx)
{
	if (!is_thread_idx_valid(_threadIdx))
	{
		return NONE;
	}
	auto& t = get()->threads[_threadIdx];
	int priority = NONE;
#ifdef AN_WINDOWS
	// implement?
#else
#ifdef AN_LINUX_OR_ANDROID
	sched_param param;
	param.sched_priority = NONE;
	if (sched_getparam(t.threadSystemId, &param) == 0)
	{
		priority = param.sched_priority;
	}
	else
	{
		priority = NONE;
	}
#else
#error TODO implement bind thread to processor
#endif
#endif
	return priority;
}

int32 ThreadManager::get_thread_priority_nice(int _threadIdx)
{
	if (!is_thread_idx_valid(_threadIdx))
	{
		return NONE;
	}
	auto& t = get()->threads[_threadIdx];
	int priority = NONE;
#ifdef AN_WINDOWS
	// implement?
#else
#ifdef AN_LINUX_OR_ANDROID
	priority = getpriority(PRIO_PROCESS, t.threadSystemId);
#else
#error TODO implement bind thread to processor
#endif
#endif
	return priority;
}

#ifdef PROFILE_PERFORMANCE
int ThreadManager::get_thread_system_cpu(int _threadIdx)
{
	if (!is_thread_idx_valid(_threadIdx))
	{
		return NONE;
	}
	auto& t = get()->threads[_threadIdx];
	return t.onCPU;
}

void ThreadManager::store_current_thread_on_cpu()
{
	if (!get())
	{
		//an_assert(false, TXT("thread manager not available"));
		return; // nowhere to store
	}

	int32 systemId = ThreadSystemUtils::get_current_thread_system_id();

	{
		//Concurrency::ScopedMRSWLockRead lock(get()->threadsLock);
		RegisteredThread * t = get()->threads;
		for_count(int, threadIdx, get()->threadCount)
		{
			if (t->threadSystemId == systemId)
			{
#ifdef AN_LINUX_OR_ANDROID
				t->onCPU = sched_getcpu();
#else
				t->onCPU = -1;
#endif
			}
			++t;
		}
	}
}
#endif
