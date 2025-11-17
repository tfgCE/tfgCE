#include "interactiveDialHandler.h"

#include "..\modules\custom\mc_grabable.h"
#include "..\modules\custom\mc_pickup.h"

#include "..\..\framework\module\moduleMovementDial.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\world\world.h"

using namespace TeaForGodEmperor;

//

// vars
DEFINE_STATIC_NAME(interactiveDeviceId);

// tags
DEFINE_STATIC_NAME(interactiveDial);

//

void InteractiveDialHandler::initialise(Framework::IModulesOwner* _owner, Name const & _interactiveDeviceIdVarInOwner, Name const & _interactiveDeviceIdVarInInteractives)
{
	Name interactiveDeviceIdVarInInteractives = _interactiveDeviceIdVarInInteractives;
	if (!interactiveDeviceIdVarInInteractives.is_valid())
	{
		interactiveDeviceIdVarInInteractives = hardcoded NAME(interactiveDeviceId);
	}
	dials.clear();
	if (auto* id = _owner->get_variables().get_existing<int>(_interactiveDeviceIdVarInOwner))
	{
		_owner->get_in_world()->for_every_object_with_id(interactiveDeviceIdVarInInteractives, *id,
			[this](Framework::Object* _object)
		{
			if (_object->get_tags().get_tag(NAME(interactiveDial)) &&
				fast_cast<Framework::ModuleMovementDial>(_object->get_movement()))
			{
				dials.push_back(SafePtr<Framework::IModulesOwner>(_object));
			}
			return false; // keep going on
		});
	}
}

void InteractiveDialHandler::advance(float _deltaTime)
{
	absoluteDialAt = 0;
	isGrabbed = false;
	Framework::IModulesOwner* newGrabbedBy = nullptr;
	{
		for_every_ref(dialObject, dials)
		{
			if (!dialObject) continue;
			if (auto * mmd = fast_cast<Framework::ModuleMovementDial>(dialObject->get_movement()))
			{
				absoluteDialAt = mmd->get_absolute_dial();
			}
			if (auto* g = dialObject->get_custom<CustomModules::Grabable>())
			{
				if (g->is_grabbed())
				{
					isGrabbed = true;
					if (!newGrabbedBy)
					{
						auto& grabbedBy = g->get_grabbed_by();
						if (!grabbedBy.is_empty())
						{
							newGrabbedBy = grabbedBy.get_first().get();
						}
					}
				}
			}
		}
	}
	grabbedBy = newGrabbedBy;

}
