#include "gse_movementControl.h"

#include "..\gameScript.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\io\xml.h"

#include "..\..\module\moduleController.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

// object variables (runtime/execution)
DEFINE_STATIC_NAME(gseLocomotionPath);
DEFINE_STATIC_NAME(gseLocomotionPathTask);

//

bool MovementControl::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	switchTo.load_from_xml(_node, TXT("switchTo"));
	
	requestedRelativeVelocity.load_from_xml(_node, TXT("requestedRelativeVelocity"));
	requestedVelocityWS.load_from_xml(_node, TXT("requestedVelocityWS"));
	requestedRelativeRotation.load_from_xml(_node, TXT("requestedRelativeRotation"));

	return result;
}

bool MovementControl::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type MovementControl::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			if (switchTo.is_set())
			{
				imo->activate_movement(switchTo.get());
			}
			if (auto* c = imo->get_controller())
			{
				if (requestedRelativeVelocity.is_set())
				{
					c->clear_requested_velocity_linear();
					//
					c->set_relative_requested_movement_direction(requestedRelativeVelocity.get().normal());
					Framework::MovementParameters mp;
					mp.absoluteSpeed = requestedRelativeVelocity.get().length();
					c->set_requested_movement_parameters(mp);
				}
				if (requestedVelocityWS.is_set())
				{
					c->clear_requested_movement_direction();
					c->set_requested_movement_parameters(Framework::MovementParameters());
					//
					c->set_requested_velocity_linear(requestedVelocityWS.get());
				}
				if (requestedRelativeRotation.is_set())
				{
					c->set_requested_velocity_orientation(requestedRelativeRotation.get());
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
