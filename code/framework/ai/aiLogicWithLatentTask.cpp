#include "aiLogicWithLatentTask.h"

#include "aiMindInstance.h"

using namespace Framework;
using namespace AI;

REGISTER_FOR_FAST_CAST(LogicWithLatentTask);

LogicWithLatentTask::LogicWithLatentTask(MindInstance* _mind, LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction))
: base(_mind, _logicData)
, executeFunction(_executeFunction)
{
}
	 
LogicWithLatentTask::~LogicWithLatentTask()
{
	an_assert(!currentlyRunningTask.is_running(), TXT("didn't call end?"));
}

void LogicWithLatentTask::advance(float _deltaTime)
{
	scoped_call_stack_info(TXT("LogicWithLatentTask::advance"));
	base::advance(_deltaTime);
	if (! currentlyRunningTask.is_running() &&
		executeFunction)
	{
		LatentTask * task = currentlyRunningTask.start_latent_task(get_mind(), nullptr, LatentFunctionInfo(executeFunction, TXT("[execute function]")));
		setup_latent_task(task);
	}
}

void LogicWithLatentTask::setup_latent_task(LatentTask* _task)
{
	ADD_LATENT_PARAM(_task->access_latent_frame(), Logic*, this);
}

void LogicWithLatentTask::stop_currently_running_task()
{
	currentlyRunningTask.stop();
}

void LogicWithLatentTask::end()
{
	stop_currently_running_task();
	base::end();
}

void LogicWithLatentTask::log_latent_funcs(LogInfoContext & _log, Latent::FramesInspection & _framesInspection) const
{
	if (currentlyRunningTask.is_running())
	{
		currentlyRunningTask.log(_log, _framesInspection);
	}
}

void LogicWithLatentTask::register_latent_task(Name const & _name, LatentFunction _function)
{
	registeredLatentTasks[_name] = _function;
}

LatentFunction LogicWithLatentTask::get_latent_task(Name const & _name)
{
	if (auto* functionPtr = registeredLatentTasks.get_existing(_name))
	{
		return *functionPtr;
	}
	else
	{
		error(TXT("task \"%S\" not registered for \"%S\""), _name.to_char(), get_mind()->get_owner_as_modules_owner()->ai_get_name().to_char());
		return nullptr;
	}
}
