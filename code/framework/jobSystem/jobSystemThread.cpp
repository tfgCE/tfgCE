#include "jobSystemThread.h"

#include "jobSystem.h"
#include "jobSystemExecutor.h"

#include "..\debugSettings.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\core.h"
#include "..\..\core\vr\iVR.h"

#ifdef AN_LINUX_OR_ANDROID
#include "unistd.h"
#endif

using namespace Framework;

JobSystemThread::JobSystemThread(Concurrency::ThreadManager* _threadManager, JobSystemExecutor * _executor, Optional<ThreadPriority::Type> const& _preferredThreadPriority)
: executor(_executor)
, preferredThreadPriority(_preferredThreadPriority)
{
	thread = _threadManager->create_thread(thread_func, this, _preferredThreadPriority);
}

JobSystemThread::~JobSystemThread()
{
	an_assert(thread);
	delete_and_clear(thread);
}

bool JobSystemThread::is_thread_system_id(int _threadSystemId) const
{
	an_assert(thread);
	return thread->get_thread_system_id() == _threadSystemId;
}

void JobSystemThread::thread_func(void * _data)
{
	scoped_call_stack_info(TXT("job system thread"));
	JobSystemThread* self = (JobSystemThread *)_data;
	JobSystem* jobSystem = self->executor->get_job_system();
#ifndef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
	int sleepTimeUS = 0;
#else
	Concurrency::SynchronisationLounge& lounge = jobSystem->get_synchronisation_lounge();
#endif
	self->isUp = true;
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
	self->threadSystemId = get_current_thread_system_id();
#endif
#ifdef AN_DEBUG_JOB_SYSTEM
	bool isWaiting = false;
	int currentThreadId = Concurrency::ThreadManager::get_current_thread_id();
#endif
	output(TXT(" thread (%i) up and running"));
	if (auto* vr = VR::IVR::get())
	{
		vr->I_am_perf_thread_other();
	}
	while (true)
	{
#ifdef PROFILE_PERFORMANCE
		Concurrency::ThreadManager::store_current_thread_on_cpu();
#endif

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		self->aliveAndKickingTS.reset();
#endif
		if (self->executor->execute(&jobSystem->access_advance_context()))
		{
#ifndef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
			sleepTimeUS = 0;
#endif

#ifdef AN_DEBUG_JOB_SYSTEM
			isWaiting = false;
#endif
			// we did something, try next one
			continue;
		}
		if (jobSystem->wants_to_end())
		{
#ifdef AN_DEBUG_JOB_SYSTEM
			isWaiting = false;
#endif
			break;
		} 
#ifdef AN_DEBUG_JOB_SYSTEM
		if (!isWaiting)
		{
			output(TXT("{%i} waits"), currentThreadId);
			isWaiting = true;
		}
#endif
		// allow to be pushed somewhere else
		Concurrency::ThreadSystemUtils::yield();
		{
			//MEASURE_PERFORMANCE_COLOURED(jobSystemThreadSleep, Colour::grey);
#ifndef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
			if (self->preferredThreadPriority.is_set() && self->preferredThreadPriority.get() <= ThreadPriority::Lowest)
			{
				sleepTimeUS = min(1000, sleepTimeUS + 100);
			}
			else
			{
				sleepTimeUS = min(32, sleepTimeUS + 2);
			}
			{
				::System::Core::sleep_u(sleepTimeUS);
			}
#else
			// nothing to do, wait for better times, but no longer than 1ms
			lounge.rest_a_little();
#endif
		}
	}
#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("{%i} ended"), currentThreadId);
#endif
	self->isDone = true;
}
