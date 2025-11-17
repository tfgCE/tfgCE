#pragma once

#include "..\framework.h"
#include "..\..\core\enums.h"
#include "..\..\core\system\timeStamp.h"
#include "..\..\core\types\optional.h"

namespace Concurrency
{
	class Thread;
	class ThreadManager;
};

namespace Framework
{
	class JobSystem;
	class JobSystemExecutor;

	class JobSystemThread
	{
	private:
		static void thread_func(void * _data);

		JobSystemExecutor* executor;
		Concurrency::Thread* thread;
		Optional<ThreadPriority::Type> preferredThreadPriority;

	private: friend class JobSystem;
		JobSystemThread(Concurrency::ThreadManager* _threadManager, JobSystemExecutor* _executor, Optional<ThreadPriority::Type> const& _preferredThreadPriority);
		~JobSystemThread();

		bool is_done() const { return isDone; }
		bool is_up() const { return isUp; }

		bool is_thread_system_id(int _threadSystemId) const;

	private:
		bool isUp = false;
		bool isDone = false;

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		::System::TimeStamp aliveAndKickingTS;
		int threadSystemId = NONE;

		friend class JobSystem;
#endif

	};
};
