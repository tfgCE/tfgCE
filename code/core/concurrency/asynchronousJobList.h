#pragma once

#include "..\globalDefinitions.h"

#include "asynchronousJob.h"
#include "synchronisationLounge.h"
#include "spinLock.h"

#ifdef AN_WINDOWS
	#define ASYNCHRONOUS_JOB_LIST_USES_SYNCHRONISATION_LOUNGE
#endif

namespace Concurrency
{
	class AsynchronousJobList
	{
	public:
		AsynchronousJobList();
		~AsynchronousJobList();

		void register_executor() { ++executorsCount; }
		int get_executors_count() const { return executorsCount; }

		void add_job(AsynchronousJobFunc _func, AsynchronousJobData* _jobData);
		void add_job(AsynchronousJobFunc _func, void* _data = nullptr);
		void add_job(std::function<void()> _func);

		bool is_any_job_available() const { return jobsAvailable != nullptr; }
		bool is_any_job_being_executed() const { return jobsBeingExecuted != nullptr; }
		bool has_finished() const { return jobsAvailable == nullptr && jobsBeingExecuted == nullptr; }
		bool execute_job_if_available(JobExecutionContext const * _executionContext);

		void wait_until_all_finished();

	private:
		int executorsCount = 0;

		AsynchronousJobPointer jobsAvailable;
		AsynchronousJobPointer lastJobAvailable;
		AsynchronousJobPointer jobsBeingExecuted;
		AsynchronousJobPointer jobsClean;

		SpinLock accessingJobs = SpinLock(TXT("Concurrency.AsynchronousJobList.accessingJobs"), 0.1f);
#ifdef ASYNCHRONOUS_JOB_LIST_USES_SYNCHRONISATION_LOUNGE
		SynchronisationLounge allJobsDoneSynchronisationLounge;
#endif

		void delete_jobs(AsynchronousJobPointer& _jobs);

		AsynchronousJob* get_clean_job();
		void make_job_available(AsynchronousJob*);

	};

};
