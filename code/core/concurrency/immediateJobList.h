#pragma once

#include "..\globalDefinitions.h"
#include "..\containers\array.h"
#include "counter.h"
#include "immediateJob.h"

namespace Concurrency
{
	class ImmediateJobList
	{
	public:
		ImmediateJobList();

		void register_executor() { ++executorsCount; }
		int get_executors_count() const { return executorsCount; }

		/** those methods should be called on single thread */
		void ready_for_job_addition();
		void add_job(ImmediateJobFunc _func, void* _data = nullptr);
		void ready_for_job_execution();
		void end_job_execution();

		bool is_any_job_available() const { return jobsLeftToStart > 0; }
		bool is_any_job_being_executed() const { return jobsLeftToFinish > 0; }
		bool has_finished() const { return ! is_any_job_available() && ! is_any_job_being_executed(); }
		bool execute_job_if_available(JobExecutionContext const * _executionContext);

	private:
		int executorsCount = 0;

#ifdef AN_DEVELOPMENT
		SpinLock listInUse = SpinLock(TXT("Concurrency.ImmediateJobList.listInUse"));
#endif
		Array<ImmediateJob> jobs;
		Counter id;
		Counter jobIndex;
		Counter accessors; // number of accessors that are checking if there is something to do, if any accessor is in, we should not modify our list
		Counter jobsLeftToStart; // how many jobs are still there
		Counter jobsLeftToFinish; // how many jobs are still there
#ifdef AN_DEVELOPMENT
		Counter jobsExecuted;
#endif
	};

};
