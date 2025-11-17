#include "agnsmd_controller.h"

#include "..\..\..\module\moduleAppearance.h"
#include "..\..\..\module\moduleController.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;
using namespace StateMachineDecisionsLib;

//--

REGISTER_FOR_FAST_CAST(Controller);

IStateMachineDecision* Controller::create_decision()
{
	return new Controller();
}

bool Controller::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD(_node, requestedMovementDirection);
	XML_LOAD(_node, requestedMovementDirectionYaw);
	XML_LOAD(_node, requestedGait);
	XML_LOAD(_node, requestedGaitNot);

	return result;
}

bool Controller::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = base::prepare_runtime(_runtime);

	return result;
}

bool Controller::execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const
{
	bool result = true;
	{
		bool anyOk = false;
		bool anyFailed = false;

		if (auto* controller = _runtime.imoOwner->get_controller())
		{
			if (requestedMovementDirection.is_set())
			{
				bool ok = requestedMovementDirection.get() == controller->get_relative_requested_movement_direction().is_set();
				anyOk |= ok;
				anyFailed |= !ok;
			}
			if (requestedMovementDirectionYaw.is_set())
			{
				bool ok = false;
				auto movDir = controller->get_relative_requested_movement_direction();
				if (movDir.is_set())
				{
					float movDirYaw = Rotator3::get_yaw(movDir.get());
					if (!ok && requestedMovementDirectionYaw.get().does_contain(movDirYaw))
					{
						ok = true;
					}
					else if (movDirYaw > requestedMovementDirectionYaw.get().max)
					{
						float relToMax = movDirYaw - requestedMovementDirectionYaw.get().max;
						relToMax = mod(relToMax, 360.0f) - 360.0f; // -360 as mod will bring it to (max,max+360) and we want to check if it is lower than max
						movDirYaw = requestedMovementDirectionYaw.get().max + relToMax;
						ok = requestedMovementDirectionYaw.get().does_contain(movDirYaw);
					}
					else if (movDirYaw < requestedMovementDirectionYaw.get().min)
					{
						float relToMin = movDirYaw - requestedMovementDirectionYaw.get().min;
						relToMin = mod(relToMin, 360.0f);
						movDirYaw = requestedMovementDirectionYaw.get().min + relToMin;
						ok = requestedMovementDirectionYaw.get().does_contain(movDirYaw);
					}
				}
				anyOk |= ok;
				anyFailed |= !ok;
			}
			if (requestedGait.is_set())
			{
				bool ok = requestedGait.get() == controller->get_requested_movement_parameters().gaitName;
				anyOk |= ok;
				anyFailed |= !ok;
			}
			if (requestedGaitNot.is_set())
			{
				bool ok = requestedGaitNot.get() != controller->get_requested_movement_parameters().gaitName;
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
