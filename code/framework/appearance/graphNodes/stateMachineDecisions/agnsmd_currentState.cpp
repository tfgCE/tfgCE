#include "agnsmd_currentState.h"

#include "..\..\..\module\moduleMovement.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;
using namespace StateMachineDecisionsLib;

//--

REGISTER_FOR_FAST_CAST(CurrentState);

IStateMachineDecision* CurrentState::create_decision()
{
	return new CurrentState();
}

bool CurrentState::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD(_node, isId);
	XML_LOAD(_node, notId);
	XML_LOAD(_node, timeActive);

	return result;
}

bool CurrentState::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = base::prepare_runtime(_runtime);

	return result;
}

bool CurrentState::execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const
{
	bool result = true;
	{
		bool anyOk = false;
		bool anyFailed = false;

		if (isId.is_set())
		{
			bool ok = _stateMachine->get_current_state(_runtime) == isId.get();
			anyOk |= ok;
			anyFailed |= !ok;
		}
		if (notId.is_set())
		{
			bool ok = _stateMachine->get_current_state(_runtime) != notId.get();
			anyOk |= ok;
			anyFailed |= !ok;
		}
		if (timeActive.is_set())
		{
			bool ok = timeActive.get().does_contain(_stateMachine->get_current_state_time_active(_runtime));
			anyOk |= ok;
			anyFailed |= !ok;
		}

		result = anyOk && ! anyFailed;
	}

	if (result)
	{
		result = base::execute(_stateMachine, _runtime);
	}

	return result;
}
