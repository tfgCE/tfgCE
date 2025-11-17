#include "immediateJobList.h"

#include "..\debugSettings.h"

#ifdef AN_DEBUG_JOB_SYSTEM
#include "threadManager.h"
#endif

using namespace Concurrency;

ImmediateJobList::ImmediateJobList()
{
	jobs.make_space_for(100);
	jobsLeftToStart = 0;
	jobsLeftToFinish = 0;
#ifdef AN_DEVELOPMENT
#ifdef AN_CONCURRENCY_STATS
	listInUse.do_not_report_stats();
#endif
#endif
}

void ImmediateJobList::ready_for_job_addition()
{
	an_assert_immediate(jobsLeftToStart == 0);
	an_assert_immediate(jobsLeftToFinish == 0);
#ifdef AN_DEVELOPMENT
	an_assert_immediate(!listInUse.is_locked(), TXT("are we using same list in by two different threads to do some work?"));
	listInUse.acquire();
#endif
	// to start new queue we have to have no accessors (they will bail out if no jobs are available)
	// this is one place we may be left locked for a bit
	// we may sometimes stuck here for a cycle because jobsLeftToFinish was decreased already but we didn't leave it and decrease accessors
	accessors.wait_till(0);
	jobs.clear();
#ifdef AN_DEVELOPMENT
	id.increase();
#endif
}

void ImmediateJobList::add_job(ImmediateJobFunc _func, void* _data)
{
	an_assert_immediate(jobsLeftToStart == 0);
	an_assert_immediate(jobsLeftToFinish == 0);
	jobs.push_back(ImmediateJob(_func, _data));
}

void ImmediateJobList::ready_for_job_execution()
{
	an_assert_immediate(jobsLeftToStart == 0);
	an_assert_immediate(jobsLeftToFinish == 0);
	jobIndex = NONE;
	// first finish as start blocks executors
#ifdef AN_DEVELOPMENT
	jobsExecuted = 0;
#endif
	jobsLeftToFinish = jobs.get_size();
	jobsLeftToStart = jobs.get_size(); // this will allow executors to do stuff
}

void ImmediateJobList::end_job_execution()
{
	an_assert_immediate(jobsLeftToStart == 0);
	an_assert_immediate(jobsLeftToFinish == 0);
#ifdef AN_DEVELOPMENT
	listInUse.release();
#endif
#ifdef AN_DEVELOPMENT
	an_assert_immediate(jobsExecuted == jobs.get_size());
#endif

}

bool ImmediateJobList::execute_job_if_available(JobExecutionContext const * _executionContext)
{
	if (jobsLeftToStart == 0)
	{
#ifdef AN_DEBUG_JOB_SYSTEM
		output(TXT("{%i} wanted to execute job, but no jobs left"), Concurrency::ThreadManager::get_current_thread_id());
#endif
	return false;
	}

	accessors.increase(); // to let know we're looking into the list and it should be not modified
#ifdef AN_DEVELOPMENT
	int wasId = id.get();
#endif
	bool result = false;
	if (jobsLeftToStart > 0) // in case something changed on the way (we could be late with accessors update)
	{
#ifdef AN_DEVELOPMENT
		an_assert_immediate(wasId == id.get());
#endif
		int32 currentJobIndex = jobIndex.increase();
		if (jobs.is_index_valid(currentJobIndex))
		{
			jobsLeftToStart.decrease();
#ifdef AN_DEVELOPMENT
			an_assert_immediate(wasId == id.get());
#endif
#ifdef AN_DEBUG_JOB_SYSTEM
			output(TXT("{%i} execute job #%i"), Concurrency::ThreadManager::get_current_thread_id(), currentJobIndex);
#endif
			// we may end up beyond jobs in some cases?
			jobs[currentJobIndex].execute(_executionContext);
#ifdef AN_DEBUG_JOB_SYSTEM
			output(TXT("{%i} done job #%i"), Concurrency::ThreadManager::get_current_thread_id(), currentJobIndex);
#endif
			result = true;
#ifdef AN_DEVELOPMENT
			an_assert_immediate(wasId == id.get());
			jobsExecuted.increase();
#endif
			an_assert(jobsLeftToStart.get() >= 0);
			// we could decrease accessors but it is far more important to let know that jobs are finished
			jobsLeftToFinish.decrease();
		}
#ifdef AN_DEBUG_JOB_SYSTEM
		else
		{
			output(TXT("{%i} wanted to execute job, but someone took it"), Concurrency::ThreadManager::get_current_thread_id());
		}
#endif
	}
	accessors.decrease();
	return result;
}
