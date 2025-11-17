#include "jobSystemExecutor.h"
#include "jobSystem.h"

#include "..\debugSettings.h"

#include "..\..\core\concurrency\asynchronousJobList.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

JobSystemExecutor::JobSystemExecutor(JobSystem* _system)
:	system( _system )
{
}

JobSystemExecutor::~JobSystemExecutor()
{
	for_every_ptr(part, parts)
	{
		delete part;
	}
}

void JobSystemExecutor::add_list(Name const & _name, int _priority)
{
	Part* newPart = nullptr;
	if (Concurrency::ImmediateJobList* immediateJobList = system->get_immediate_list(_name))
	{
		newPart = new Part(_name, _priority, immediateJobList);
	}
	else if (Concurrency::AsynchronousJobList* asynchronousJobList = system->get_asynchronous_list(_name))
	{
		newPart = new Part(_name, _priority, asynchronousJobList);
	}
	if (newPart)
	{
		int index = parts.get_size();
		for_every_reverse_ptr(part, parts)
		{
			if (part->priority >= _priority)
			{
				break;
			}
			-- index;
		}
		parts.insert_at(index, newPart);
	}
}

bool JobSystemExecutor::execute_immediate_job(Concurrency::ImmediateJobList* _fromList, Concurrency::JobExecutionContext const * _executionContext, OPTIONAL_ JobSystemExecutor* _forExecutor)
{
	if (_fromList && _fromList->is_any_job_available())
	{
#ifdef AN_DEBUG_JOB_SYSTEM
		output(TXT("{%i} has immediate job"), Concurrency::ThreadManager::get_current_thread_id());
#endif
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		if (_forExecutor)
		{
			_forExecutor->executingImmediateJob = true;
		}
#endif
		bool result = _fromList->execute_job_if_available(_executionContext);
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		if (_forExecutor)
		{
			_forExecutor->executingImmediateJob = false;
		}
#endif
		return result;
	}
	return false;
}

bool JobSystemExecutor::execute_asynchronous_job(Concurrency::AsynchronousJobList* _fromList, Concurrency::JobExecutionContext const * _executionContext, OPTIONAL_ JobSystemExecutor* _forExecutor)
{
	if (_fromList && _fromList->is_any_job_available())
	{
#ifdef AN_DEBUG_JOB_SYSTEM
		output(TXT("{%i} has asynchronous job"), Concurrency::ThreadManager::get_current_thread_id());
#endif
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		if (_forExecutor)
		{
			_forExecutor->executingAsynchronousJob = true;
		}
#endif
		bool result = _fromList->execute_job_if_available(_executionContext);
#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		if (_forExecutor)
		{
			_forExecutor->executingAsynchronousJob = false;
		}
#endif
		return result;
	}
	return false;
}

bool JobSystemExecutor::does_handle(Name const& _name) const
{
	for_every_ptr(part, parts)
	{
		if (part->name == _name)
		{
			return true;
		}
	}
	return false;
}

bool JobSystemExecutor::execute(Concurrency::JobExecutionContext const * _executionContext)
{
	for_every_ptr(part, parts)
	{
		if (execute_asynchronous_job(part->asynchronousJobList, _executionContext, this))
		{
			return true;
		}
		if (execute_immediate_job(part->immediateJobList, _executionContext, this))
		{
			return true;
		}
	}
	return false;
}

Name const& JobSystemExecutor::find_part_name(Concurrency::ImmediateJobList* _immediateJobList) const
{
	for_every_ptr(part, parts)
	{
		if (part->immediateJobList == _immediateJobList)
		{
			return part->name;
		}
	}
	return Name::invalid();
}

Name const& JobSystemExecutor::find_part_name(Concurrency::AsynchronousJobList* _asynchronousJobList) const
{
	for_every_ptr(part, parts)
	{
		if (part->asynchronousJobList == _asynchronousJobList)
		{
			return part->name;
		}
	}
	return Name::invalid();
}

void JobSystemExecutor::log(LogInfoContext& _log) const
{
	_log.log(TXT("job system executor"));
	{
		LOG_INDENT(_log);
		for_every_ptr(p, parts)
		{
			_log.log(TXT("+- %C %02i %S"), p->immediateJobList? 'i' : (p->asynchronousJobList? 'a' : '?'), p->priority, p->name.to_char());
		}
	}
}

//

JobSystemExecutor::Part::Part(Name const & _name, int _priority, Concurrency::ImmediateJobList* _immediateJobList)
:	name( _name )
,	priority( _priority )
,	immediateJobList( _immediateJobList )
,	asynchronousJobList( nullptr )
{
	immediateJobList->register_executor();
}

JobSystemExecutor::Part::Part(Name const & _name, int _priority, Concurrency::AsynchronousJobList* _asynchronousJobList)
:	name( _name )
,	priority( _priority )
,	immediateJobList( nullptr )
,	asynchronousJobList( _asynchronousJobList )
{
	asynchronousJobList->register_executor();
}
