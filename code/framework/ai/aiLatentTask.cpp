#include "aiLatentTask.h"

#include "aiMindInstance.h"

#include "aiTaskHandle.h"

#include "..\ai\aiLogics.h"
#include "..\modulesOwner\modulesOwner.h"

#include "..\..\core\debug\logInfoContext.h"
#include "..\..\core\latent\latentFrameInspection.h"
#include "..\..\core\latent\latentStackVariables.h"
#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AI;

//

bool LatentTaskInfo::propose(LatentFunctionInfo const& _taskFunctionInfo, Optional<int> _priority, Optional<bool> _onlyIfPriorityIsHigher, Optional<bool> _allowToInterrupt, Optional<bool> _startImmediately, Optional<float> _timeLimit)
{
	if (!taskFunctionInfo.taskFunction || priority <= _priority.get(0))
	{
		priority = _priority.get(0);
		onlyIfPriorityIsHigher = _onlyIfPriorityIsHigher.get(false);
		taskFunctionInfo = _taskFunctionInfo;
		allowToInterrupt = _allowToInterrupt.get(true);
		startImmediately = _startImmediately.get(false);
		timeLimit = _timeLimit;
		return true;
	}
	return false;
}

bool LatentTaskInfo::propose(Name const & _taskFunction, Optional<int> _priority, Optional<bool> _onlyIfPriorityIsHigher, Optional<bool> _allowToInterrupt, Optional<bool> _startImmediately, Optional<float> _timeLimit)
{
	auto function = Framework::AI::Logics::get_latent_task(_taskFunction);
	if (function)
	{
		return propose(LatentFunctionInfo(function, _taskFunction.to_char()), _priority, _onlyIfPriorityIsHigher, _allowToInterrupt, _startImmediately, _timeLimit);
	}
	return false;
}

void LatentTaskInfo::log(LogInfoContext& _context) const
{
	_context.log(TXT("latent task info"));
	LOG_INDENT(_context);
	_context.log(TXT("priority %i%S"), priority, onlyIfPriorityIsHigher? TXT(" only if higher") : TXT(""));
	_context.log(TXT("task function %p \"%S\""), taskFunctionInfo.taskFunction, taskFunctionInfo.get_info());
	_context.log(allowToInterrupt? TXT("allow to interrupt") : TXT("no interruptions"));
	_context.log(startImmediately? TXT("start immediately") : TXT("start at any time"));
	if (timeLimit.is_set())
	{
		_context.log(TXT("time limit %.2fs"), timeLimit.get());
	}
	else
	{
		_context.log(TXT("no time limit"));
	}
}

//

bool LatentTaskInfoWithParams::propose(LatentFunctionInfo const& _taskFunctionInfo, Optional<int> _priority, Optional<bool> _onlyIfPriorityIsHigher, Optional<bool> _allowToInterrupt, Optional<bool> _startImmediately, Optional<float> _timeLimit)
{
	if (base::propose(_taskFunctionInfo, _priority, _onlyIfPriorityIsHigher, _allowToInterrupt, _startImmediately, _timeLimit))
	{
		params.remove_all_variables();
		return true;
	}
	return false;
}

bool LatentTaskInfoWithParams::propose(Name const & _taskFunction, Optional<int> _priority, Optional<bool> _onlyIfPriorityIsHigher, Optional<bool> _allowToInterrupt, Optional<bool> _startImmediately, Optional<float> _timeLimit)
{
	auto function = Framework::AI::Logics::get_latent_task(_taskFunction);
	if (function)
	{
		return propose(LatentFunctionInfo(function, _taskFunction.to_char()), _priority, _onlyIfPriorityIsHigher, _allowToInterrupt, _startImmediately, _timeLimit);
	}
	return false;
}

//

REGISTER_FOR_FAST_CAST(LatentTask);

LatentTask::LatentTask()
: base()
{
}

void LatentTask::setup(MindInstance* _mindInstance, LatentTaskHandle* _latentTaskHandle, LatentFunctionInfo const & _taskFunctionInfo)
{
	an_assert(_taskFunctionInfo.taskFunction);

	activeTime = 0.0f;

	mindInstance = _mindInstance;
	taskFunctionInfo = _taskFunctionInfo;
	
	SETUP_LATENT(latentFrame);

	ADD_LATENT_PARAM(latentFrame, MindInstance*, _mindInstance);
	if (_latentTaskHandle)
	{
		ADD_LATENT_PARAM(latentFrame, LatentTaskHandle*, _latentTaskHandle);
	}
}

bool LatentTask::advance(float _deltaTime)
{
	scoped_call_stack_info(TXT("advance latent task"));
	activeTime += _deltaTime;
	rareAdvanceWait.clear();
	if (taskFunctionInfo.taskFunction)
	{
		MEASURE_PERFORMANCE(latentTask);
		MEASURE_PERFORMANCE_CONTEXT(taskFunctionInfo.get_info());
		if (timeLimit.is_set())
		{
			if (activeTime >= timeLimit.get())
			{
				MEASURE_PERFORMANCE(endTaskDueToTime);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("BREAK LATENT DUE TO TIME LIMIT %.3f >= %.3f taskFunction RawPtr %p"), activeTime, timeLimit.get(), taskFunctionInfo.taskFunction, taskFunctionInfo.get_info());
#endif
				timeLimit.clear();
				scoped_call_stack_info(TXT("break latent"));
				scoped_call_stack_info(taskFunctionInfo.get_info());
				return BREAK_LATENT(latentFrame, taskFunctionInfo.taskFunction);
			}
		}
		bool result;
		{
			scoped_call_stack_info(TXT("perform latent"));
			scoped_call_stack_info(taskFunctionInfo.get_info());
			PERFORMANCE_GUARD_LIMIT_2(0.005f, TXT("[latent task]"), taskFunctionInfo.get_info());
			PERFORMANCE_LIMIT_GUARD_START();
			result = PERFORM_LATENT(latentFrame, _deltaTime, taskFunctionInfo.taskFunction);
			PERFORMANCE_LIMIT_GUARD_STOP(0.0005f, TXT("latent task for \"%S\""), mindInstance? mindInstance->get_owner_as_modules_owner()->ai_get_name().to_char() : TXT("--"));
		}
		if (latentFrame.how_long_does_want_to_wait() > 0.0f)
		{
			rareAdvanceWait = latentFrame.how_long_does_want_to_wait();
		}

		return result;
	}
	else
	{
		return true;
	}
}

void LatentTask::on_break()
{
	scoped_call_stack_info(TXT("break latent (on break)"));
	scoped_call_stack_info(taskFunctionInfo.get_info());
	BREAK_LATENT(latentFrame, taskFunctionInfo.taskFunction);
}

void LatentTask::on_get()
{
	get_new_id();
}

void LatentTask::on_release()
{
	taskFunctionInfo.clear();
	latentFrame.reset();
}

void LatentTask::log(LogInfoContext & _log, Latent::FramesInspection & _framesInspection) const
{
	bool thisOne = _framesInspection.add(&latentFrame);
#ifdef AN_INSPECT_LATENT
	_log.log(thisOne ? TXT("=== %S [%S]") : TXT("+-> %S [%S]"), latentFrame.get_info().to_char(), taskFunctionInfo.get_info());
#else
	_log.log(thisOne ? TXT("=== [%S]") : TXT("+-> [%S]"), taskFunctionInfo.get_info());
#endif

	LOG_INDENT(_log);
#ifdef AN_INSPECT_LATENT
	Latent::StackVariableEntry const * variable;
	int varCount = 0;
	latentFrame.get_stack_variables_to_inspect().get_variables(variable, varCount);
	while (varCount > 0)
	{
		if (variable->size == sizeof(LatentTaskHandle) &&
			(variable->destructFunc == ObjectHelpers<LatentTaskHandle>::static_call_destructor_on ||
			 variable->copyConstructFunc == ObjectHelpers<LatentTaskHandle>::call_copy_constructor_on))
		{
			LatentTaskHandle* latentTask = plain_cast<LatentTaskHandle>(variable->get_data());
			latentTask->log(_log, _framesInspection);
		}
		if (variable->size == sizeof(RegisteredLatentTaskHandles) &&
			(variable->destructFunc == ObjectHelpers<RegisteredLatentTaskHandles>::static_call_destructor_on ||
			 variable->copyConstructFunc == ObjectHelpers<RegisteredLatentTaskHandles>::call_copy_constructor_on))
		{
			RegisteredLatentTaskHandles* registeredLatentTaskHandles = plain_cast<RegisteredLatentTaskHandles>(variable->get_data());
			registeredLatentTaskHandles->log(_log, _framesInspection);
		}
		--varCount;
		variable++;
	}
#endif
}
