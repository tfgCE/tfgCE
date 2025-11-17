#include "aiTaskHandle.h"

#include "aiLatentTask.h"
#include "aiLogic.h"
#include "aiLogics.h"
#include "aiMindInstance.h"

using namespace Framework;
using namespace AI;

REGISTER_FOR_FAST_CAST(TaskHandle);

TaskHandle::TaskHandle()
{
}

TaskHandle::TaskHandle(TaskHandle const & _other)
: mindInstance(_other.mindInstance)
, task(_other.task)
{
	if (task)
	{
		task->add_owner(this);
	}
}

TaskHandle & TaskHandle::operator=(TaskHandle const & _other)
{
	if (task != _other.task)
	{
		drop_task();
		mindInstance = _other.mindInstance;
		task = _other.task;
		if (task)
		{
			task->add_owner(this);
		}
	}
	return *this;
}

TaskHandle::~TaskHandle()
{
	drop_task();
}

void TaskHandle::drop_task()
{
	if (task)
	{
		int handleOwners = 0;
		for_every_ptr(owner, task->get_owners())
		{
			if (fast_cast<TaskHandle>(owner))
			{
				++handleOwners;
			}
		}
		if (handleOwners == 1)
		{
			stop();
		}
		else
		{
			task->remove_owner(this);
		}
	}
	task = nullptr;
	mindInstance = nullptr;
}

void TaskHandle::on_task_release(ITask* _task)
{
	if (_task == task)
	{
		task->remove_owner(this);
		task = nullptr;
	}
}

LatentTask* TaskHandle::start_latent_task(MindInstance* _mindInstance, LatentTaskHandle* _latentTaskHandle, LatentFunctionInfo const & _taskFunctionInfo)
{
	LatentTask* newTask = LatentTask::get_one();
	newTask->setup(_mindInstance, _latentTaskHandle, _taskFunctionInfo);

	start(_mindInstance, newTask);

	return newTask;
}

LatentTask* TaskHandle::switch_latent_task(MindInstance* _mindInstance, LatentTaskHandle* _latentTaskHandle, LatentFunctionInfo const& _taskFunctionInfo)
{
	LatentTask* newTask = LatentTask::get_one();
	newTask->setup(_mindInstance, _latentTaskHandle, _taskFunctionInfo);

	switch_to(_mindInstance, newTask);

	return newTask;
}

void TaskHandle::start(MindInstance* _mindInstance, ITask* _task)
{
	stop();

	start_new(_mindInstance, _task);
}

void TaskHandle::switch_to(MindInstance* _mindInstance, ITask* _task)
{
	if (task)
	{
		task->remove_owner(this);
		task = nullptr;
	}

	start_new(_mindInstance, _task);
}

void TaskHandle::start_new(MindInstance* _mindInstance, ITask* _task)
{
	an_assert_log_always(task == nullptr);
	mindInstance = _mindInstance;
	task = _task;

	if (task)
	{
		task->add_owner(this);
		an_assert_log_always(mindInstance);
		mindInstance->access_execution().start_task(task);
	}
}

void TaskHandle::stop()
{
	if (is_running())
	{
		an_assert_log_always(task); // actually this is the same, it is kept here as a reminder
		an_assert_log_always(mindInstance);
		mindInstance->access_execution().stop_task(task);
		an_assert_log_always(task == nullptr); // should be handled through on_task_release
	}
}

bool TaskHandle::is_running(LATENT_FUNCTION_VARIABLE(_taskFunction)) const
{
	if (auto* latentTask = fast_cast<LatentTask>(task))
	{
		return latentTask->get_task_function() == _taskFunction;
	}
	return _taskFunction == nullptr;
}

bool TaskHandle::is_running(LatentFunctionInfo const& _taskFunctionInfo) const
{
	return is_running(_taskFunctionInfo.taskFunction);
}

void TaskHandle::log(LogInfoContext& _context) const
{
	if (task)
	{
		_context.log(TXT("has task"));
	}
	else
	{
		_context.log(TXT("no task"));
	}
}

void TaskHandle::log(LogInfoContext & _log, Latent::FramesInspection & _framesInspection) const
{
	if (auto* latentTask = fast_cast<LatentTask>(task))
	{
		latentTask->log(_log, _framesInspection);
	}
}

//

LatentTaskHandle::LatentTaskHandle()
{
}

LatentTaskHandle::LatentTaskHandle(LatentTaskHandle const & _other)
: base(_other)
, info(_other.info)
{
}

LatentTaskHandle& LatentTaskHandle::operator=(LatentTaskHandle const & _other)
{
	base::operator=(_other);
	info = _other.info;
	return *this;
}

LatentTask* LatentTaskHandle::start_latent_task(MindInstance* _mindInstance, LatentTaskInfo const & _info)
{
	LatentTask* task = base::start_latent_task(_mindInstance, this, _info.taskFunctionInfo);
	info = _info;
	result.clear();
	task->set_time_limit(info.timeLimit);
	return task;
}

LatentTask* LatentTaskHandle::start_latent_task(MindInstance* _mindInstance, LatentTaskInfoWithParams const & _info)
{
	LatentTask* task = start_latent_task(_mindInstance, (LatentTaskInfo)(_info));
	ADD_LATENT_PARAMS(task->access_latent_frame(), _info.params);
	return task;
}

LatentTask* LatentTaskHandle::switch_latent_task(MindInstance* _mindInstance, LatentTaskInfo const & _info)
{
	LatentTask* task = base::switch_latent_task(_mindInstance, this, _info.taskFunctionInfo);
	auto& prevInfo = info;
	info = _info;
	info.priority = prevInfo.priority;
	info.onlyIfPriorityIsHigher = prevInfo.onlyIfPriorityIsHigher;
	result.clear();
	task->set_time_limit(info.timeLimit);
	return task;
}

LatentTask* LatentTaskHandle::switch_latent_task(MindInstance* _mindInstance, LatentTaskInfoWithParams const & _info)
{
	LatentTask* task = switch_latent_task(_mindInstance, (LatentTaskInfo)(_info));
	ADD_LATENT_PARAMS(task->access_latent_frame(), _info.params);
	return task;
}

void LatentTaskHandle::drop_task()
{
	base::drop_task();
	info = LatentTaskInfo();
}

void LatentTaskHandle::on_task_release(ITask* _task)
{
	base::on_task_release(_task);
	if (!is_running())
	{
		info = LatentTaskInfo();
	}
}

void LatentTaskHandle::log(LogInfoContext& _context) const
{
	base::log(_context);
	info.log(_context);
}

//

bool RegisteredLatentTasksInfo::load_from_xml(IO::XML::Node const * _node, tchar const * _childName)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		Name task = Name::invalid();
		task = node->get_name_attribute_or_from_child(TXT("function"), task);
		task = node->get_name_attribute_or_from_child(TXT("task"), task);
		if (task.is_valid())
		{
			tasks.push_back(task);
		}
		else
		{
			error_loading_xml(node, TXT("latent task not named"));
			result = false;
		}
	}

	return result;
}

//

RegisteredLatentTaskHandles::RegisteredLatentTaskHandles()
{
}

RegisteredLatentTaskHandles::~RegisteredLatentTaskHandles()
{
	tasks.clear();
}

void RegisteredLatentTaskHandles::add(MindInstance* _mindInstance, Name const & _latentTaskFunction)
{
	auto function = Logics::get_latent_task(_latentTaskFunction);
	if (function)
	{
		LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(LatentFunctionInfo(function, _latentTaskFunction.to_char()));

		tasks.grow_size(1);
		tasks.get_last().start_latent_task(_mindInstance, taskInfo);
	}
}

void RegisteredLatentTaskHandles::add(MindInstance* _mindInstance, RegisteredLatentTasksInfo const & _info)
{
	for_every(task, _info.tasks)
	{
		add(_mindInstance, *task);
	}
}

void RegisteredLatentTaskHandles::log(LogInfoContext & _log, Latent::FramesInspection & _framesInspection) const
{
	for_every(task, tasks)
	{
		task->log(_log, _framesInspection);
	}
}
