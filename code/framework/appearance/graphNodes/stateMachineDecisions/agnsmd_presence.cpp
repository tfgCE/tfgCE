#include "agnsmd_presence.h"

#include "..\..\..\module\moduleAppearance.h"
#include "..\..\..\module\modulePresence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;
using namespace StateMachineDecisionsLib;

//--

REGISTER_FOR_FAST_CAST(Presence);

IStateMachineDecision* Presence::create_decision()
{
	return new Presence();
}

bool Presence::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD(_node, currentSpeedXY);
	XML_LOAD(_node, currentSpeedZ);

	return result;
}

bool Presence::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = base::prepare_runtime(_runtime);

	return result;
}

bool Presence::execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const
{
	bool result = true;
	{
		bool anyOk = false;
		bool anyFailed = false;

		if (auto* presence = _runtime.imoOwner->get_presence())
		{
			if (currentSpeedXY.is_set())
			{
				float speedXY = presence->get_velocity_linear().drop_using(presence->get_placement().get_axis(Axis::Up)).length();
				bool ok = currentSpeedXY.get().does_contain(speedXY);
				anyOk |= ok;
				anyFailed |= !ok;
			}
			if (currentSpeedZ.is_set())
			{
				Vector3 up = presence->get_placement().get_axis(Axis::Up).normal();
				float speedZ = Vector3::dot(up, presence->get_velocity_linear().along(up));
				bool ok = currentSpeedZ.get().does_contain(speedZ);
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
