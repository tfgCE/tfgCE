#include "thread.h"

#include "threadManager.h"

#include "..\system\core.h"

#ifdef AN_ANDROID
#include "..\system\javaEnv.h"
#endif

#ifdef AN_LINUX_OR_ANDROID
#include <unistd.h>
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_OUTPUT
//#define AN_DEBUG_CREATE_THREADS
#endif

#ifdef AN_DEBUG_CREATE_THREADS
	#define output_debug(...) output(__VA_ARGS__);
#else
	#define output_debug(...)
#endif
//

using namespace Concurrency;

struct ThreadStartContext
{
	ThreadManager& manager;
	Thread* thread;
	Thread::ThreadFunc func;
	void * data;

	ThreadStartContext(ThreadManager& _manager, Thread* _thread, Thread::ThreadFunc _func, void * _data)
	: manager(_manager)
	, thread(_thread)
	, func(_func)
	, data(_data)
	{}
};

#ifdef AN_WINDOWS
DWORD WINAPI Thread::run_thread_func( LPVOID _threadContext ) 
#endif
#ifdef AN_LINUX_OR_ANDROID
void* Thread::run_thread_func(void* _threadContext)
#endif
{
#ifdef AN_CHECK_MEMORY_LEAKS
	an_assert(is_memory_leak_detection_supressed(), TXT("haven't you forgotten to mark main thread running?"));
	// because by default, all threads have it suppressed
	resume_memory_leak_detection();
#endif
	// register thread, collect data, delete context and run
	ThreadStartContext* threadStartContext = (ThreadStartContext*)_threadContext;
	Thread* thread = threadStartContext->thread;
	if (!thread->offSystem)
	{
		threadStartContext->manager.register_thread(thread); // no output messages before this, please!
		output_debug(TXT("(%p) registered thread"), _threadContext);
	}
	else
	{
		output_debug(TXT("(%p) started off system thread"), _threadContext);
	}
	Thread::ThreadFunc func = threadStartContext->func;
	void * data = threadStartContext->data;
	ThreadManager& manager = threadStartContext->manager;
	output_debug(TXT("(%p) delete thread setup"), _threadContext);
	delete threadStartContext;
	thread->isUp = true;
	output_debug(TXT("(%p) run thread"), _threadContext);
#ifdef AN_ANDROID
	System::JavaEnv::get_env(); // to attach
#endif
	func(data);
	if (!thread->offSystem)
	{
		output_debug(TXT("(%p) unregister"), _threadContext);
		manager.unregister_thread(thread);
	}
	else
	{
		output_debug(TXT("(%p) off system thread done"), _threadContext);
	}
#ifdef AN_ANDROID
	System::JavaEnv::drop_env();
#endif
	thread->isDone = true;
	thread->isUp = false;
#ifdef AN_WINDOWS
	return 0;
#endif
#ifdef AN_LINUX_OR_ANDROID
	pthread_exit(nullptr);
#endif
}

Thread::Thread(ThreadManager& _mgr)
: SafeObject(this)
, threadManager(&_mgr)
, autoKill(false)
, isUp(true)
, isDone(false)
{
	output_debug(TXT("main thread"));
#ifdef AN_WINDOWS
	SECURITY_ATTRIBUTES securityAttrs;
	securityAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);

	threadSystemId = GetCurrentThreadId();
#else
#ifdef AN_LINUX_OR_ANDROID
	threadSystemId = gettid();
#else
#error implement
#endif
#endif
}

Thread::Thread(ThreadManager& _mgr, ThreadFunc _func, void * _data, Optional<ThreadPriority::Type> const & _preferedThreadPriority, bool _offSystem)
: SafeObject(this)
, preferedThreadPriority(_preferedThreadPriority)
, threadManager(&_mgr)
, offSystem(_offSystem)
, autoKill(true)
, isUp(false)
, isDone(false)
{
	output_debug(TXT("create thread"));
	auto* data = new ThreadStartContext(_mgr, this, _func, _data);
	output_debug(TXT("(%p) start context created"), data);
#ifdef AN_WINDOWS
	SECURITY_ATTRIBUTES securityAttrs;
	securityAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);

	DWORD tid;
	CreateThread(nullptr, 0, run_thread_func, data, 0, &tid);
	threadSystemId = (int32)tid;
#else
#ifdef AN_LINUX_OR_ANDROID
	#ifdef AN_DEVELOPMENT
		int result =
	#endif
	pthread_create(&threadSystemId, nullptr, run_thread_func, data);
	#ifdef AN_DEVELOPMENT
		an_assert(result == 0);
	#endif
#else
#error implement
#endif
#endif
	output_debug(TXT("(%p) created thread"), data);
}

Thread::~Thread()
{
	if (autoKill)
	{
		kill(System::Core::is_performing_quick_exit());
	}

	make_safe_object_unavailable();
}

void Thread::kill(bool _immediate)
{
	if (!isUp)
	{
		return;
	}
#ifdef AN_WINDOWS
	HANDLE threadHandle = OpenThread(THREAD_TERMINATE, false, threadSystemId);
	if (!_immediate)
	{
		// waiting for thread to really finish, we should already exit it, just wait for system to handle its stuff
		WaitForSingleObject(threadHandle, INFINITE);
	}
	else
	{
		TerminateThread(threadHandle, 0);
	}
	// closing handle
	CloseHandle(threadHandle);
#else
#ifdef AN_LINUX_OR_ANDROID
	if (!_immediate)
	{
		// waiting for thread to really finish, we should already exit it, just wait for system to handle its stuff
		void* returnValue;
		pthread_join(threadSystemId, &returnValue);
	}
	else
	{
		pthread_kill(threadSystemId, 0);
	}
#else
#error implement
#endif
#endif
	isUp = false;
}

void Thread::suspend_kill()
{
	if (!isUp)
	{
		return;
	}
#ifdef AN_WINDOWS
	HANDLE threadHandle = OpenThread(THREAD_TERMINATE, false, threadSystemId);
	SuspendThread(threadHandle);
#else
#ifdef AN_LINUX_OR_ANDROID
	pthread_kill(threadSystemId, 0);
#else
#error implement
#endif
#endif
	isUp = false;
}

int32 Thread::get_current_thread_id(ThreadManager const & _mgr)
{
	return _mgr.get_current_thread_id();
}

struct OffSystemThread
{
	std::function<void()> perform;

	OffSystemThread(std::function<void()> _perform) : perform(_perform) {}

#ifdef AN_WINDOWS
	static DWORD WINAPI run_perform_and_delete_self(void* _data)
#endif
#ifdef AN_LINUX_OR_ANDROID
	static void* run_perform_and_delete_self(void* _data)
#endif
	{
#ifdef AN_ANDROID
		System::JavaEnv::get_env(); // to attach
#endif

		OffSystemThread* ost = (OffSystemThread*)_data;
		ost->perform();
		delete ost;

#ifdef AN_ANDROID
		System::JavaEnv::drop_env();
#endif

#ifdef AN_WINDOWS
		return 0;
#endif
#ifdef AN_LINUX_OR_ANDROID
		pthread_exit(nullptr);
#endif
	}
};

void Thread::start_off_system_thread(std::function<void()> _perform)
{
	output_debug(TXT("create thread"));

	OffSystemThread* ost = new OffSystemThread(_perform);
#ifdef AN_WINDOWS
	SECURITY_ATTRIBUTES securityAttrs;
	securityAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);

	DWORD tid;
	CreateThread(nullptr, 0, OffSystemThread::run_perform_and_delete_self, ost, 0, &tid);
#else
#ifdef AN_LINUX_OR_ANDROID
	pthread_t threadSystemId;
#ifdef AN_DEVELOPMENT
	int result =
#endif
		pthread_create(&threadSystemId, nullptr, OffSystemThread::run_perform_and_delete_self, ost);
#ifdef AN_DEVELOPMENT
	an_assert(result == 0);
#endif
#else
#error implement
#endif
#endif
}
