#include "asynchronousJobList.h"

#include "..\concurrency\scopedSpinLock.h"
#include "..\debug\debug.h"
#include "..\memory\memory.h"

#include "..\debugSettings.h"

#include "..\system\core.h"

#ifdef AN_DEBUG_JOB_SYSTEM
#include "threadManager.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Concurrency;

//

AsynchronousJobList::AsynchronousJobList()
: jobsAvailable(nullptr)
, lastJobAvailable(nullptr)
, jobsBeingExecuted(nullptr)
, jobsClean(nullptr)
{
}

AsynchronousJobList::~AsynchronousJobList()
{
	delete_jobs(jobsAvailable);
	delete_jobs(jobsBeingExecuted);
	delete_jobs(jobsClean);
}

void AsynchronousJobList::delete_jobs(AsynchronousJobPointer& _jobs)
{
	while (_jobs)
	{
		AsynchronousJobPointer job = _jobs;
		_jobs = _jobs->next;
		job->next = nullptr;
		delete job;
	}
}

void AsynchronousJobList::add_job(AsynchronousJobFunc _func, AsynchronousJobData* _jobData)
{
	Concurrency::ScopedSpinLock lock(accessingJobs);

	// get new job
	AsynchronousJob* job = (AsynchronousJob*)get_clean_job();
#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("new job [%p:job data:%p]"), job, _jobData);
#endif
	// setup it
	job->setup(_func, _jobData);
	// after it is setup
	make_job_available(job);
}

void AsynchronousJobList::add_job(AsynchronousJobFunc _func, void* _data)
{
	Concurrency::ScopedSpinLock lock(accessingJobs);

	// get new job
	AsynchronousJob* job = (AsynchronousJob*)get_clean_job();
#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("new job [%p:data:%p]"), job, _data);
#endif
	// setup it
	job->setup(_func, _data);
	// after it is setup
	make_job_available(job);
}

void AsynchronousJobList::add_job(std::function<void()> _func)
{
	Concurrency::ScopedSpinLock lock(accessingJobs);

	// get new job
	AsynchronousJob* job = (AsynchronousJob*)get_clean_job();
#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("new job [%p:data:%p]"), job, _data);
#endif
	// setup it
	job->setup(_func);
	// after it is setup
	make_job_available(job);
}

AsynchronousJob* AsynchronousJobList::get_clean_job()
{
	an_assert(accessingJobs.is_locked_on_this_thread());

	if (jobsClean)
	{
		AsynchronousJob* job = (AsynchronousJob*)jobsClean;
		jobsClean = jobsClean->next;
		job->next = nullptr;
		return job;
	}
	return new AsynchronousJob();
}

void AsynchronousJobList::make_job_available(AsynchronousJob* job)
{
	an_assert(accessingJobs.is_locked_on_this_thread());

	// add it at the end of queue
	if (lastJobAvailable)
	{
		lastJobAvailable->next = job;
	}
	else
	{
		jobsAvailable = job;
	}
	lastJobAvailable = job;
}

bool AsynchronousJobList::execute_job_if_available(JobExecutionContext const* _executionContext)
{
	if (!jobsAvailable)
	{
#ifdef AN_DEBUG_JOB_SYSTEM
		output(TXT("{%i} no async jobs"), Concurrency::ThreadManager::get_current_thread_id());
#endif
		return false;
	}

	AsynchronousJob* job = nullptr;

	{	// get first job to execute and add it to jobs being executed queue
		Concurrency::ScopedSpinLock lock(accessingJobs);
		job = (AsynchronousJob*)jobsAvailable;
		if (!job)
		{
#ifdef AN_DEBUG_JOB_SYSTEM
			output(TXT("{%i} no longer has async jobs"), Concurrency::ThreadManager::get_current_thread_id());
#endif
			// we could lose job between first check and locking, bail out here
			return false;
		}
		jobsAvailable = jobsAvailable->next;
		if (!jobsAvailable)
		{
			lastJobAvailable = nullptr;
		}
		job->next = jobsBeingExecuted;
		jobsBeingExecuted = job;
	}
	
#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("{%i} execute async job [%p]"), Concurrency::ThreadManager::get_current_thread_id(), job);
#endif

	// execute
	{
		scoped_call_stack_info(TXT("execute asynchronous job"));
		job->execute(_executionContext);
	}

#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("{%i} done async job [%p]"), Concurrency::ThreadManager::get_current_thread_id(), job);
#endif

	{	// remove it from being executed and add to clean
		Concurrency::ScopedSpinLock lock(accessingJobs);
		AsynchronousJob* jobBeingExecuted = (AsynchronousJob*)jobsBeingExecuted;
		AsynchronousJob* prevJobBeingExecuted = nullptr;
		while (jobBeingExecuted)
		{
			if (jobBeingExecuted == job)
			{
				if (prevJobBeingExecuted)
				{
					prevJobBeingExecuted->next = job->next;
				}
				else
				{
					jobsBeingExecuted = job->next;
				}
				break;
			}
			prevJobBeingExecuted = jobBeingExecuted;
			jobBeingExecuted = (AsynchronousJob*)jobBeingExecuted->next;
		}
		job->next = nullptr;
	}

	// clean it (it is possible to trigger adding job so we can't be locked during clean())
	job->clean();

	{	// add to jobs clean
		Concurrency::ScopedSpinLock lock(accessingJobs);
		job->next = jobsClean;
		jobsClean = job;
	}

#ifdef ASYNCHRONOUS_JOB_LIST_USES_SYNCHRONISATION_LOUNGE
	if (has_finished())
	{
		allJobsDoneSynchronisationLounge.wake_up_all();
	}
#endif

#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("{%i} done async job cleanup"), Concurrency::ThreadManager::get_current_thread_id());
#endif

	return true;
}

void AsynchronousJobList::wait_until_all_finished()
{
	// check with interval if all jobs are finished (because we could skip between has_finished and rest)
#ifndef ASYNCHRONOUS_JOB_LIST_USES_SYNCHRONISATION_LOUNGE
	int sleepTime = 1;
#endif
	while (! has_finished())
	{
#ifdef ASYNCHRONOUS_JOB_LIST_USES_SYNCHRONISATION_LOUNGE
		allJobsDoneSynchronisationLounge.rest(0.001f); // rest no longer than 1ms
#else
		::System::Core::sleep_u(sleepTime);
		sleepTime = min(32, sleepTime + 1);
#endif
	}
}
