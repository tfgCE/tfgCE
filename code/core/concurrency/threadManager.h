#pragma once

#include "..\globalDefinitions.h"
#include "..\types\optional.h"
#include "spinLock.h"
#include "thread.h"

namespace Concurrency
{
	class ThreadManager
	{
		friend class Thread;

	public:
		ThreadManager();
		~ThreadManager();

		static inline ThreadManager* get() { return s_currentThreadManager; }

		static bool is_main_thread();
		static bool is_current_thread_registered();
		static int32 get_current_thread_id(bool _allowNone = false); // 0 to thread count - 1
		static int32 get_thread_count();

		static void log_thread_info(LogInfoContext& _log, Optional<bool> const & _header = NP, Optional<int> const& _tid = NP, tchar const* _info = nullptr);

#ifdef PROFILE_PERFORMANCE
		static void store_current_thread_on_cpu();
#endif

		Thread* create_thread(Thread::ThreadFunc _func, void * _data = nullptr, Optional<ThreadPriority::Type> const & _threadPriority = NP, bool _offSystem = false);

		void register_thread(Thread* _thread, bool _main = false); // to add system thread id
		void unregister_thread(Thread* _thread); // marks as not alive

		void kill_other_threads(bool _immediate = false);
		void suspend_kill_other_threads();

		int32 get_thread_system_id(int _threadIdx);
		int32 get_thread_policy(int _threadIdx);
		int32 get_thread_priority(int _threadIdx);
		int32 get_thread_priority_nice(int _threadIdx);
#ifdef PROFILE_PERFORMANCE
		int get_thread_system_cpu(int _threadIdx);
#endif

	private:
		static ThreadManager* s_currentThreadManager;

		// map for threading
		struct RegisteredThread
		{
			bool alive;
			bool mainThread;
#ifdef AN_WINDOWS
			int32 threadSystemId;
#else
#ifdef AN_LINUX_OR_ANDROID
			pthread_t threadSystemId;
#else
#error implement
#endif
#endif
#ifdef PROFILE_PERFORMANCE
			int onCPU;
#endif
			SafePtr<Thread> thread;
			RegisteredThread() : alive(false), mainThread(false), thread(nullptr) {}
			RegisteredThread(int32 _threadSystemId, Thread* _thread, bool _main)
			: alive(true)
			, mainThread(_main)
			, threadSystemId(_threadSystemId)
			, thread(_thread)
			{}
		};
		RegisteredThread threads[MAX_THREAD_COUNT];
		Thread mainThread;

		int threadCount = 0;
		SpinLock threadsLock = SpinLock(SYSTEM_SPIN_LOCK);

		// these will be overriden in the constructor
		int cpuOffset = 0; // offset number to affinate differently
		int cpuCount = 1;
		bool cpuFromLast = true; // should we start from first or from last?

		bool is_thread_idx_valid(int _threadIdx) const;
	};

};
