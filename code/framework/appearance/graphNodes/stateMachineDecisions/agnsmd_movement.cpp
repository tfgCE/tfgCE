#include "agnsmd_movement.h"

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

REGISTER_FOR_FAST_CAST(Movement);

IStateMachineDecision* Movement::create_decision()
{
	return new Movement();
}

bool Movement::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD(_node, current);
	XML_LOAD(_node, currentNot);
	XML_LOAD(_node, currentGait);
	XML_LOAD(_node, currentGaitNot);

	return result;
}

bool Movement::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = base::prepare_runtime(_runtime);

	return result;
}

bool Movement::execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const
{
	bool result = true;
	{
		bool anyOk = false;
		bool anyFailed = false;

		if (auto* movement = _runtime.imoOwner->get_movement())
		{
			if (current.is_set())
			{
				bool ok = movement->get_name() == current.get();
				anyOk |= ok;
				anyFailed |= !ok;
			}
			if (currentNot.is_set())
			{
				bool ok = movement->get_name() != currentNot.get();
				anyOk |= ok;
				anyFailed |= !ok;
			}
			if (currentGait.is_set())
			{
				bool ok = movement->get_current_gait_name() == currentGait.get();
				anyOk |= ok;
				anyFailed |= !ok;
			}
			if (currentGaitNot.is_set())
			{
				bool ok = movement->get_current_gait_name() != currentGaitNot.get();
				anyOk |= ok;
				anyFailed |= !ok;
			}
		}

		result = anyOk && ! anyFailed;
	}

	if (result)
	{
		result = base::execute(_stateMachine, _runtime);
	}

	return result;
}
