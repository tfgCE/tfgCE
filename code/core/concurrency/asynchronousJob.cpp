#include "asynchronousJob.h"
#include "..\debug\debug.h"

#include "..\performance\performanceUtils.h"

#include "..\debugSettings.h"

#ifdef AN_DEBUG_JOB_SYSTEM
#include "threadManager.h"
#endif

using namespace Concurrency;

AsynchronousJob::AsynchronousJob()
{
}

AsynchronousJob::~AsynchronousJob()
{
	an_assert(next == nullptr, TXT("remove from list first"));
	clean();
}

void AsynchronousJob::clean()
{
	execute_func = nullptr;
	execute_func_alt = nullptr;
	if (jobData)
	{
		jobData->release_job_data();
		jobData = nullptr;
	}
	data = nullptr;
}

void AsynchronousJob::setup(AsynchronousJobFunc _func, AsynchronousJobData* _jobData)
{
	an_assert(execute_func == nullptr && execute_func_alt == nullptr && ! data && ! jobData);
	an_assert(_func != nullptr, TXT("execute function not provided"));
	execute_func = _func;
	jobData = _jobData;
}

void AsynchronousJob::setup(AsynchronousJobFunc _func, void* _data)
{
	an_assert(execute_func == nullptr && execute_func_alt == nullptr && !data && !jobData);
	an_assert(_func != nullptr, TXT("execute function not provided"));
	execute_func = _func;
	data = _data;
}

void AsynchronousJob::setup(std::function<void()> _func)
{
	an_assert(execute_func == nullptr && execute_func_alt == nullptr && !data && !jobData);
	an_assert(_func != nullptr, TXT("execute function not provided"));
	execute_func_alt = _func;
}

void AsynchronousJob::execute(JobExecutionContext const * _executionContext)
{
	MEASURE_PERFORMANCE_COLOURED(asynchronousJob, Colour::red.with_alpha(0.2f));
#ifdef AN_DEBUG_JOB_SYSTEM
	if (execute_func_alt)
	{
		output(TXT("{%i} executing job [%p:func:%p]"), Concurrency::ThreadManager::get_current_thread_id(), this, execute_func_alt);
	}
	else if (jobData)
	{
		output(TXT("{%i} executing job [%p:job data:%p]"), Concurrency::ThreadManager::get_current_thread_id(), this, jobData);
	}
	else
	{
		output(TXT("{%i} executing job [%p:data:%p]"), Concurrency::ThreadManager::get_current_thread_id(), this, data);
	}
#endif
	if (execute_func_alt)
	{
		execute_func_alt();
	}
	else
	{
		execute_func(_executionContext, jobData ? jobData : data);
	}
}
