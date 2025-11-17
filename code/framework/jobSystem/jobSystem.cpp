#include "jobSystem.h"

#include "..\..\core\concurrency\asynchronousJobList.h"
#include "..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

ScopedJobSystemPerformanceMeasureInfo::ScopedJobSystemPerformanceMeasureInfo(tchar const* _info)
{
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	JobSystem::set_performance_measure_info(_info);
#endif
}

ScopedJobSystemPerformanceMeasureInfo::~ScopedJobSystemPerformanceMeasureInfo()
{
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	JobSystem::clear_performance_measure_info();
#endif
}

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool JobSystem::useBatches = true;
//#define AN_LOG_EXECUTE_IMMEDIATE_JOBS_FOR_TIME
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
tchar const* JobSystem::performanceMeasureInfo = nullptr;
#endif

JobSystem::JobSystem()
:	wantsToEnd( false )
{
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
	mainThreadSystemId = get_current_thread_system_id();
#endif
}

JobSystem::~JobSystem()
{
	for_every_ptr(thread, offSystemJobThreads)
	{
		delete thread;
	}
	for_every_ptr(thread, threads)
	{
		delete thread;
	}
	for_every_ptr(part, parts)
	{
		delete part;
	}
	for_every_ptr(executor, executors)
	{
		delete executor;
	}
}

void JobSystem::end()
{
	wantsToEnd = true;
}

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
void JobSystem::start_lock_checker()
{
	an_assert(!lockCheckerThread);
	lockCheckerThread = threadManager.create_thread(check_for_locks, this, ThreadPriority::Lowest);
}

void JobSystem::pause_lock_checker()
{
	lockCheckerPaused = true;
}

void JobSystem::resume_lock_checker()
{
	lockCheckerPaused = false;
}

void JobSystem::check_for_locks(void* _data)
{
	JobSystem* self = plain_cast<JobSystem>(_data);

	output(TXT("lock checker started, initial wait"));

	// initial wait to get the things started
	::System::Core::sleep(5.0f);

	output(TXT("lock checker active"));
	self->aliveAndKickingTS.reset();

	ArrayStatic<int, CallStackInfo::maxThreads> lockedThreads; SET_EXTRA_DEBUG_INFO(lockedThreads, TXT("JobSystem::check_for_locks.lockedThreads"));
	String lockedThreadsInfo;

	while (!self->wants_to_end())
	{
		::System::Core::sleep(1.0f);

		::Concurrency::memory_fence();

		if (self->lockCheckerPaused ||
			!::System::Core::is_vr_paused()) // if vr paused, we're stuck in process vr events, ignore
		{
			continue;
		}

		lockedThreads.clear();

		// check main thread, it should be doing only immediate jobs, never should be dead for more than 1 second
		{
			float akTime = self->aliveAndKickingTS.get_time_since();
			float akTimeLimit = 1.0f;
			if (akTime > 0.0f && akTime < 10000.0f && // prevent from cases when misreading the data 
				akTime > akTimeLimit)
			{
				lockedThreads.push_back(self->mainThreadSystemId);
				lockedThreadsInfo += String::printf(TXT("(%07X) %8.2fs > %8.2fs\n"), self->mainThreadSystemId, akTime, akTimeLimit);
			}
		}

		for_every_ptr(jst, self->threads)
		{
			float akTime = jst->aliveAndKickingTS.get_time_since();
			float akTimeLimit = 1.0f;
			if (!jst->executor->is_executing_immediate_job() &&
				jst->executor->is_executing_asynchronous_job())
			{
				// really, really long
				// although with a job this long we would already have problems
				akTimeLimit = 60.0f; 
			}
			if (akTime > 0.0f && akTime < 10000.0f && // prevent from cases when misreading the data
				akTime > akTimeLimit)
			{
				if (lockedThreads.has_place_left())
				{
					lockedThreads.push_back(jst->threadSystemId);
					lockedThreadsInfo += String::printf(TXT("(%07X) %8.2fs > %8.2fs\n"), jst->threadSystemId, akTime, akTimeLimit);
				}
			}
		}

		if (! lockedThreads.is_empty())
		{
			String callStackInfo = get_call_stack_info_for_threads(lockedThreads);
			error_dev_ignore(TXT("LOCKED THREADS!\n%S%S"), lockedThreadsInfo.to_char(), callStackInfo.to_char());
			lockedThreadsInfo = String::empty();
		}
	}

	output(TXT("lock checker done"));
}
#endif

void JobSystem::wait_for_all_threads_to_be_down()
{
	while (true)
	{
		int threadsDone = 0;
		for_every_ptr(thread, threads)
		{
			if (thread->is_done())
			{
				++threadsDone;
			}
		}
		if (threadsDone == threads.get_size())
		{
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
			if (!lockCheckerThread || lockCheckerThread->is_done())
#endif
			{
				break;
			}
		}
#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
		get_synchronisation_lounge().rest(0.001f);
#else
		System::Core::sleep(0.001f);
#endif
	}
}

int JobSystem::execute_main_executor(uint _howMany)
{
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
	aliveAndKickingTS.reset();
#endif
	int executed = 0;
	if (_howMany == 0)
	{
		--_howMany; // because it is uint, it will get the highest value
	}
	while (_howMany != 0 && mainExecutor->execute(&access_advance_context())) { --_howMany; ++executed; }
	return executed;
}

JobSystemExecutor* JobSystem::create_executor()
{
	JobSystemExecutor* executor = new JobSystemExecutor(this);
	executors.push_back(executor);
	return executor;
}

void JobSystem::create_thread(JobSystemExecutor* _executor, Optional<ThreadPriority::Type> const & _preferredThreadPriority)
{
	an_assert(executors.does_contain(_executor));
	JobSystemThread* thread = new JobSystemThread(&threadManager, _executor, _preferredThreadPriority);
	threads.push_back(thread);
}

void JobSystem::wait_for_all_threads_to_be_up()
{
	while (true)
	{
		int threadsUp = 0;
		for_every_ptr(thread, threads)
		{
			if (thread->is_up())
			{
				++threadsUp;
			}
		}
		if (threadsUp == threads.get_size())
		{
			break;
		}
		::System::Core::sleep(0.001f);
	}
}

void JobSystem::add_immediate_list(Name const & _name)
{
	if (Part** part = parts.get_existing(_name))
	{
		// replace
		delete *part;
	}
	parts[_name] = Part::create_immediate_list(_name);
}

Concurrency::ImmediateJobList* JobSystem::get_immediate_list(Name const & _name)
{
	if (Part** part = parts.get_existing(_name))
	{
		return (*part)->immediateJobList;
	}
	return nullptr;
}

void JobSystem::add_asynchronous_list(Name const & _name)
{
	if (Part** part = parts.get_existing(_name))
	{
		// replace
		delete *part;
	}
	parts[_name] = Part::create_asynchronous_list(_name);
}

Concurrency::AsynchronousJobList* JobSystem::get_asynchronous_list(Name const & _name)
{
	if (Part** part = parts.get_existing(_name))
	{
		return (*part)->asynchronousJobList;
	}
	return nullptr;
}

void JobSystem::do_asynchronous_job(Name const & _listName, Concurrency::AsynchronousJobFunc _jobFunc, Concurrency::AsynchronousJobData* _data, DoAsynchronousJob::Type _type)
{
	Concurrency::AsynchronousJobList* asynchronousJobList = get_asynchronous_list(_listName);

	an_assert(asynchronousJobList);

#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("add async job to \"%S\" [job data:%p]"), _listName.to_char(), _data);
#endif

	asynchronousJobList->add_job(_jobFunc, _data);

#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
	synchronisationLounge.wake_up_all(); // wake up all waiting in lounge
#endif

	if (_type == DoAsynchronousJob::Wait)
	{
		asynchronousJobList->wait_until_all_finished();
	}
}

void JobSystem::do_asynchronous_job(Name const & _listName, Concurrency::AsynchronousJobFunc _jobFunc, void* _data, DoAsynchronousJob::Type _type)
{
	Concurrency::AsynchronousJobList* asynchronousJobList = get_asynchronous_list(_listName);

	an_assert(asynchronousJobList);

#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("add async job to \"%S\" [data:%p]"), _listName.to_char(), _data);
#endif

	asynchronousJobList->add_job(_jobFunc, _data);

#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
	synchronisationLounge.wake_up_all(); // wake up all waiting in lounge
#endif

	if (_type == DoAsynchronousJob::Wait)
	{
		asynchronousJobList->wait_until_all_finished();
	}
}

void JobSystem::do_asynchronous_job(Name const& _listName, std::function<void()> _func, DoAsynchronousJob::Type _type)
{
	Concurrency::AsynchronousJobList* asynchronousJobList = get_asynchronous_list(_listName);

	an_assert(asynchronousJobList);

#ifdef AN_DEBUG_JOB_SYSTEM
	output(TXT("add async job to \"%S\" [data:%p]"), _listName.to_char(), _data);
#endif

	asynchronousJobList->add_job(_func);

#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
	synchronisationLounge.wake_up_all(); // wake up all waiting in lounge
#endif

	if (_type == DoAsynchronousJob::Wait)
	{
		scoped_call_stack_info(TXT("wait for jobs to finish"));
		asynchronousJobList->wait_until_all_finished();
	}
}

void JobSystem::execute_immediate_jobs(Name const & _listName, float _forTime, std::function<bool()> _hasFinished)
{
	MEASURE_PERFORMANCE_COLOURED(executeImmediateJobs, Colour::black.with_alpha(0.2f));

	Concurrency::ImmediateJobList* immediateJobList = get_immediate_list(_listName);

	::System::TimeStamp startedTS;
	
#ifdef AN_LOG_EXECUTE_IMMEDIATE_JOBS_FOR_TIME
	int count = 0;
#endif
	// do jobs for as much time as we have
	while (startedTS.get_time_since() < _forTime)
	{
		if (JobSystemExecutor::execute_immediate_job(immediateJobList, &access_advance_context()))
		{
#ifdef AN_LOG_EXECUTE_IMMEDIATE_JOBS_FOR_TIME
			++count;
#endif
		}
		else
		{
			if (_hasFinished && _hasFinished())
			{
				break;
			}
			float timeLeft = _forTime - startedTS.get_time_since();
			if (timeLeft > 0.0f)
			{
				// do not wait for too long
				float restingTime = min(0.0001f, timeLeft * 0.25f);
				//MEASURE_PERFORMANCE_COLOURED(executeImmediateJobsRest, Colour::white.with_alpha(0.5f));
#ifdef JOB_SYSTEM_USES_SYNCHRONISATION_LOUNGE
				synchronisationLounge.rest(restingTime);
#else
				System::Core::sleep(restingTime);
#endif
			}
		}
	}
#ifdef AN_LOG_EXECUTE_IMMEDIATE_JOBS_FOR_TIME
	float tookTime = startedTS.get_time_since();
	output(TXT("executed: %i (time %.3fms, allowed %.3fms, left %.3fms)"), count, tookTime * 1000.0f, _forTime * 1000.0f, tookTime * 1000.0f);
#endif
}

void JobSystem::clean_up_finished_off_system_jobs()
{
	if (offSystemJobThreads.is_empty())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(offSystemJobThreadsLock);

	for (int i = 0; i < offSystemJobThreads.get_size(); ++i)
	{
		if (offSystemJobThreads[i]->is_done())
		{
			delete offSystemJobThreads[i];
			offSystemJobThreads.remove_at(i);
			--i;
		}
	}
}

void JobSystem::do_asynchronous_off_system_job(Concurrency::Thread::ThreadFunc _jobFunc, void* _data)
{
	Concurrency::ScopedSpinLock lock(offSystemJobThreadsLock);
	Concurrency::Thread* thread = threadManager.create_thread(_jobFunc, _data, ThreadPriority::Lower, true);
	offSystemJobThreads.push_back(thread);
}

#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
JobSystemThread* JobSystem::get_current_job_system_thread() const
{
	int threadSystemId = get_current_thread_system_id();
	for_every_ptr(thread, threads)
	{
		if (threadSystemId == thread->threadSystemId)
		{
			return thread;
		}
	}
	return nullptr;
}
#endif

bool JobSystem::is_on_thread_that_handles(Name const& _name, Optional<Name> const& _otherName) const
{
	int threadSystemId = Concurrency::ThreadSystemUtils::get_current_thread_system_id();
	for_every_ptr(thread, threads)
	{
		if (thread->is_thread_system_id(threadSystemId))
		{
			if (thread->executor->does_handle(_name))
			{
				return true;
			}
			if (_otherName.is_set() &&
				thread->executor->does_handle(_otherName.get()))
			{
				return true;
			}
			return false;
		}
	}
	return false;
}

//

JobSystem::Part::Part(Name const & _name)
:	name( _name )
,	immediateJobList( nullptr )
,	asynchronousJobList( nullptr )
{
}

JobSystem::Part::~Part()
{
	delete_and_clear(immediateJobList);
	delete_and_clear(asynchronousJobList);
}

JobSystem::Part* JobSystem::Part::create_immediate_list(Name const & _name)
{
	Part* part = new Part(_name);
	part->immediateJobList = new Concurrency::ImmediateJobList();
	output(TXT("created immediate list \"%S\" %p"), _name.to_char(), part->immediateJobList);
	return part;
}

JobSystem::Part* JobSystem::Part::create_asynchronous_list(Name const & _name)
{
	Part* part = new Part(_name);
	part->asynchronousJobList = new Concurrency::AsynchronousJobList();
	output(TXT("created async list \"%S\" %p"), _name.to_char(), part->asynchronousJobList);
	return part;
}

