#pragma once

#include "..\globalDefinitions.h"
#include "..\memory\safeObject.h"
#include "..\types\optional.h"
#include "..\mainConfig.h"

#ifdef AN_WINDOWS
#include <Windows.h>
#endif

#ifdef AN_LINUX_OR_ANDROID
#include <pthread.h>
#endif

namespace Concurrency
{
	class ThreadManager;

	class Thread
	: public SafeObject<Thread>
	{
	public:
		~Thread();

		typedef void (*ThreadFunc)(void * _data);

		static int32 get_current_thread_id(ThreadManager const& _mgr);
		
		int32 get_thread_system_id() const { return threadSystemId; }

		bool is_done() const { return isDone; }

		static void start_off_system_thread(std::function<void()> _perform); // doesn't even create a seperate thread for it

	private:
	#ifdef AN_WINDOWS
		int32 threadSystemId;
	#else
	#ifdef AN_LINUX_OR_ANDROID
		pthread_t threadSystemId;
	#else
		#error implement
	#endif
	#endif

		Optional<ThreadPriority::Type> preferedThreadPriority;

		ThreadManager* threadManager;
		bool offSystem = false;
		bool autoKill = false;

		bool isUp = false;
		bool isDone = false; // main thread is never done

	private: friend class ThreadManager;
		Thread(ThreadManager& _mgr); // main thread
		Thread(ThreadManager& _mgr, ThreadFunc _func, void * _data = nullptr, Optional<ThreadPriority::Type> const& _threadPriority = NP, bool _offSystem = false);

		void kill(bool _immediate = false);
		void suspend_kill();

#ifdef AN_WINDOWS
		static DWORD WINAPI run_thread_func(LPVOID _threadContext);
#endif
#ifdef AN_LINUX_OR_ANDROID
		static void* run_thread_func(void* _threadContext);
#endif
	};

};
