#include "aiCommon.h"

#include "logics\tasks\aiLogicTask_scan.h"
#include "logics\tasks\aiLogicTask_leaveRoom.h"
#include "logics\tasks\aiLogicTask_scriptedMoveToPOI.h"

#include "..\..\framework\ai\aiTaskHandle.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// sound tags
DEFINE_STATIC_NAME(talk);

// scripted behaviours
DEFINE_STATIC_NAME(gameScript);
DEFINE_STATIC_NAME(scan);
DEFINE_STATIC_NAME(leaveRoom);
DEFINE_STATIC_NAME(moveToPOI);

//

AI::Common* AI::Common::s_common = nullptr;

void AI::Common::initialise_static()
{
	an_assert(!s_common);
	s_common = new AI::Common();

	s_common->soundsTalk.set_tag(NAME(talk));
}

void AI::Common::close_static()
{
	delete_and_clear(s_common);
}

#define SWITCH_TO_TASK(newTask) \
	if (!_currentTask.is_running(newTask)) \
	{ \
		nextTask.propose(newTask); \
		_currentTask.stop(); \
		_currentTask.start_latent_task(_mind, nextTask); \
	}

bool AI::Common::handle_scripted_behaviour(::Framework::AI::MindInstance* _mind, Name const& _scriptedBehaviour, ::Framework::AI::LatentTaskHandle& _currentTask)
{
	if (_scriptedBehaviour.is_valid())
	{
		::Framework::AI::LatentTaskInfo nextTask;
		if (_scriptedBehaviour == NAME(gameScript))
		{
			_currentTask.stop();
		}
		else if (_scriptedBehaviour == NAME(scan))
		{
			SWITCH_TO_TASK(AI_LATENT_TASK_FUNCTION(Logics::Tasks::scan));
		}
		else if (_scriptedBehaviour == NAME(leaveRoom))
		{
			SWITCH_TO_TASK(AI_LATENT_TASK_FUNCTION(Logics::Tasks::leave_room));
		}
		else if (_scriptedBehaviour == NAME(moveToPOI))
		{
			SWITCH_TO_TASK(AI_LATENT_TASK_FUNCTION(Logics::Tasks::scripted_move_to_poi));
		}
		else
		{
			todo_implement;
		}

		return true;
	}
	else
	{
		return false;
	}
}

void AI::Common::set_scripted_behaviour(REF_ Name& _scriptedBehaviourVar, Name const& _value)
{
	_scriptedBehaviourVar = _value;
}

void AI::Common::unset_scripted_behaviour(REF_ Name& _scriptedBehaviourVar, Name const& _unset)
{
	if (_scriptedBehaviourVar == _unset)
	{
		unset_scripted_behaviour(REF_ _scriptedBehaviourVar);
	}
}

void AI::Common::unset_scripted_behaviour(REF_ Name& _scriptedBehaviourVar)
{
	_scriptedBehaviourVar = Name::invalid();
}
