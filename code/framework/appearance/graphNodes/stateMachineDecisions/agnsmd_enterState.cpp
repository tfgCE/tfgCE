#include "agnsmd_enterState.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;
using namespace StateMachineDecisionsLib;

//--

REGISTER_FOR_FAST_CAST(EnterState);

IStateMachineDecision* EnterState::create_decision()
{
	return new EnterState();
}

bool EnterState::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	XML_LOAD_NAME_ATTR(_node, id);
	XML_LOAD_FLOAT_ATTR(_node, blendTime);
	XML_LOAD_BOOL_ATTR(_node, forceUseFrozenPose);

	return result;
}

bool EnterState::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = true;

	return result;
}

bool EnterState::execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const
{
	_stateMachine->change_state(id, blendTime, forceUseFrozenPose, _runtime);

	return true;
}
