#pragma once

#include "..\..\core\concurrency\asynchronousJobFunc.h"
#include "..\..\core\concurrency\immediateJobFunc.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\concurrency\synchronisationLounge.h"
#include "..\..\core\concurrency\threadManager.h"
#include "..\..\core\concurrency\threadSystemUtils.h"
#include "..\..\core\containers\map.h"

#include "..\advance\advanceContext.h"
#include "jobSystemExecutor.h"
#include "jobSystemsImmediateJobPerformer.h"
#include "jobSystemThread.h"

// .inl
#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\concurrency\immediateJobList.h"
#include "..\..\core\concurrency\memoryFence.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\concurrency\immediateJobFunc.h"

#include <functional>

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	#define scoped_job_system_performance_measure_info(_info) ::Framework::ScopedJobSystemPerformanceMeasureInfo temp_variable_named(tempJobSystemInfo)(_info);
#else
	#define scoped_job_system_performance_measure_info(_info)
#endif

#ifdef AN_WINDOWS
	#define JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
#endif

namespace Concurrency
{
	class SynchronisationLounge;
	class AsynchronousJobData;
};

namespace Framework
{
	namespace DoAsynchronousJob
	{
		enum Type
		{
			DontWait,
			Wait, // wait until all finished, this doesn't guarantee a memory barrier, use Gate with wait() + allow_one()
		};
	};

	struct ScopedJobSystemPerformanceMeasureInfo
	{
		ScopedJobSystemPerformanceMeasureInfo(tchar const* _info);
		~ScopedJobSystemPerformanceMeasureInfo();
	};

	/**
	 *	Contains all job lists (both immediate and asynchronous)
	 */
	class JobSystem
	{
	public:
		JobSystem();
		~JobSystem();

#ifdef AN_DEVELOPMENT_OR_PROFILER
		static bool does_use_batches() { return useBatches; }
		static void use_batches(bool _useBatches) { useBatches = _useBatches; }
#endif

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		static void set_performance_measure_info(tchar const* _performanceMeasureInfo) { performanceMeasureInfo = _performanceMeasureInfo; }
		static void clear_performance_measure_info() { performanceMeasureInfo = nullptr; }
		static tchar const* get_performance_measure_info() { return performanceMeasureInfo? performanceMeasureInfo : TXT("--"); }
#else
		static tchar const* get_performance_measure_info() { return TXT("--"); }
#endif

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		void start_lock_checker();
		void pause_lock_checker();
		void resume_lock_checker();
#endif

		JobSystemExecutor* create_executor();

		bool is_on_thread_that_handles(Name const& _name, Optional<Name> const& _otherName = NP) const;
			
		void set_main_executor(JobSystemExecutor* _executor) { mainExecutor = _executor; }
		JobSystemExecutor* get_main_executor() { return mainExecutor; }
		int execute_main_executor(uint _howMany = 0); // 0 - execute all

		void end();
		void wait_for_all_threads_to_be_down();
		inline bool wants_to_end() const { return wantsToEnd; }

		void create_thread(JobSystemExecutor* _executor, Optional<ThreadPriority::Type> const& _preferredThreadPriority = NP);
		void wait_for_all_threads_to_be_up();

		void add_immediate_list(Name const & _name);
		void add_asynchronous_list(Name const & _name);
		Concurrency::ImmediateJobList* get_immediate_list(Name const & _name);
		Concurrency::AsynchronousJobList* get_asynchronous_list(Name const & _name);

		AdvanceContext & access_advance_context() { return advanceContext; }
#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
		Concurrency::SynchronisationLounge& get_synchronisation_lounge() { return synchronisationLounge; }
#endif

		// adding/executing
		template <typename ElementClass, typename Container>
		void do_immediate_jobs(Name const & _listName, Concurrency::ImmediateJobFunc _jobFunc, Container & _container, std::function<bool (ElementClass const *)> _check = nullptr, int _batchLimit = NONE); // add immediate jobs (for each object in container, container of pointers) to selected list and execute them immediately

		void do_asynchronous_job(Name const & _listName, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data, DoAsynchronousJob::Type _type = DoAsynchronousJob::DontWait);
		void do_asynchronous_job(Name const & _listName, Concurrency::AsynchronousJobFunc _jobFunc, void* _data = nullptr, DoAsynchronousJob::Type _type = DoAsynchronousJob::DontWait);
		void do_asynchronous_job(Name const & _listName, std::function<void()> _func, DoAsynchronousJob::Type _type = DoAsynchronousJob::DontWait);

		void clean_up_finished_off_system_jobs();
		void do_asynchronous_off_system_job(Concurrency::Thread::ThreadFunc _jobFunc, void* _data = nullptr); // perform a job that is not a part of the system - creates thread for that

		// execute jobs while waiting for something else, from a single list
		void execute_immediate_jobs(Name const & _listName, float _forTime, std::function<bool()> _hasFinished = nullptr);

	private:
#ifdef AN_DEVELOPMENT_OR_PROFILER
		static bool useBatches;
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		static tchar const* performanceMeasureInfo;
#endif

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		::System::TimeStamp aliveAndKickingTS;
		int mainThreadSystemId = NONE;
		bool lockCheckerPaused = false;

		JobSystemThread* get_current_job_system_thread() const;
#endif

		struct Part
		{
		public:
			Name name;
			Concurrency::ImmediateJobList* immediateJobList;
			Concurrency::AsynchronousJobList* asynchronousJobList;

			static Part* create_immediate_list(Name const & _name);
			static Part* create_asynchronous_list(Name const & _name);
			~Part();

		private:
			Part(Name const & _name);
		};

		bool wantsToEnd;

		Map<Name, Part*> parts;
		Array<JobSystemThread*> threads;
		Array<JobSystemExecutor*> executors;
		JobSystemExecutor* mainExecutor;

		Concurrency::SpinLock offSystemJobThreadsLock = Concurrency::SpinLock(TXT("Framework.JobSystem.offSystemJobThreadsLock"));
		Array<Concurrency::Thread*> offSystemJobThreads;

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		Concurrency::Thread* lockCheckerThread = nullptr;
#endif

		AdvanceContext advanceContext;

		Concurrency::ThreadManager threadManager;

#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
		Concurrency::SynchronisationLounge synchronisationLounge;
#endif

		static void check_for_locks(void* _data);
	};

	#include "jobSystem.inl"
};
