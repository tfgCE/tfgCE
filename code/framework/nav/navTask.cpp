#include "navTask.h"

#include "..\game\game.h"

#include "..\..\core\performance\performanceUtils.h"

#ifdef AN_LOG_NAV_TASKS
#include "..\..\core\other\parserUtils.h"
#endif

using namespace Framework;
using namespace Nav;

//

REGISTER_FOR_FAST_CAST(Task);

Task::Task(bool _writer)
: writer(_writer)
{
}

Task::~Task()
{
}

void Task::perform_job(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	MEASURE_PERFORMANCE(navTask_performJob);
	scoped_call_stack_info(TXT("perform nav task"));

	Concurrency::AsynchronousJobData* taskJobData = plain_cast<Concurrency::AsynchronousJobData>(_data);
	Task* task = (Task*)taskJobData;

	an_assert(task->is_queued_job());
	task->state = TaskState::Running;

#ifdef AN_LOG_NAV_TASKS
	task->log_to_output(TXT("perform"));
#endif

	task->perform();

	an_assert(task->is_done(), TXT("use end_* function"));
}

void Task::reset_to_new()
{
	state = TaskState::Pending;
	cancelRequested = false;
#ifdef AN_LOG_NAV_TASKS
	log_to_output(TXT("reset to new"));
#endif
}

void Task::end_cancelled()
{
	state = TaskState::Cancelled;
#ifdef AN_LOG_NAV_TASKS
	log_to_output(TXT("cancelled"));
#endif
}

void Task::end_failed()
{
	state = TaskState::Failed;
#ifdef AN_LOG_NAV_TASKS
	log_to_output(TXT("failed"));
#endif
}

void Task::end_success()
{
	state = TaskState::Success;
#ifdef AN_LOG_NAV_TASKS
	log_to_output(TXT("success"));
#endif
}

void Task::queue_async_job(Game* _game)
{
	an_assert(is_pending());
	state = TaskState::QueuedJob;
	add_ref(); // to allow release_job_data to release_ref
	Game::async_game_background_job(_game, Task::perform_job, this);
}

#ifdef AN_LOG_NAV_TASKS
bool Task::log_to_output(tchar const* _info) const
{
#ifndef AN_DEVELOPMENT
	if (should_log())
	{
		LogInfoContext log;

#ifdef AN_64
		log.log(TXT("path task %S (%S)"), ParserUtils::uint64_to_hex((uint64)this).to_char(), _info);
#else
		log.log(TXT("path task %S (%S)"), ParserUtils::uint_to_hex((uint)this).to_char(), _info);
#endif

		log_internal(log);

		log.output_to_output_as_single_line();
	}
#endif

	return true;
}

void Task::log_internal(LogInfoContext& _log) const
{
	_log.log(TXT("state %S"), TaskState::to_char(state));
}

void Task::destroy_ref_count_object()
{
	log_to_output(TXT("destroying task"));
	RefCountObject::destroy_ref_count_object();
}
#endif

void Task::log_task(LogInfoContext& _log) const
{

}
