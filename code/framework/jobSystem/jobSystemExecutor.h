#pragma once

#include "..\..\core\types\name.h"

namespace Concurrency
{
	class ImmediateJobList;
	class AsynchronousJobList;
	class JobExecutionContext;
};

namespace Framework
{
	class JobSystem;

	/**
	 *	immediate and asynchronous job system executor
	 *	Contains job lists ordered by priority (different threads can have different order for job execution)
	 */
	class JobSystemExecutor
	{
	public:
		void add_list(Name const & _name, int _priority = 0);
		int get_list_count() const { return parts.get_size(); }

		bool does_handle(Name const& _name) const;

		bool execute(Concurrency::JobExecutionContext const * _executionContext); // returns true if something has been executed

		static bool execute_immediate_job(Concurrency::ImmediateJobList* _fromList, Concurrency::JobExecutionContext const * _executionContext, OPTIONAL_ JobSystemExecutor * _forExecutor = nullptr);
		static bool execute_asynchronous_job(Concurrency::AsynchronousJobList* _fromList, Concurrency::JobExecutionContext const * _executionContext, OPTIONAL_ JobSystemExecutor* _forExecutor = nullptr);

		JobSystem* get_job_system() { return system; }

		void log(LogInfoContext & _log) const ;

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		bool is_executing_immediate_job() const { return executingImmediateJob; }
		bool is_executing_asynchronous_job() const { return executingAsynchronousJob; }
#endif

	private:
		struct Part
		{
			Name name;
			int priority;
			Concurrency::ImmediateJobList* immediateJobList; // points at job system
			Concurrency::AsynchronousJobList* asynchronousJobList; // points at job system

			Part(Name const & _name, int _priority, Concurrency::ImmediateJobList* _immediateJobList);
			Part(Name const & _name, int _priority, Concurrency::AsynchronousJobList* _asynchronousJobList);
		};

		JobSystem* system;
		Array<Part*> parts;

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		bool executingImmediateJob = false;
		bool executingAsynchronousJob = false;
#endif

		JobSystemExecutor(JobSystem* _system);
		~JobSystemExecutor();

		Name const& find_part_name(Concurrency::ImmediateJobList* _immediateJobList) const;
		Name const& find_part_name(Concurrency::AsynchronousJobList* _asynchronousJobList) const;

		friend class JobSystem;
	};
};
