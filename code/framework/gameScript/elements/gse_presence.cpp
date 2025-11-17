#include "gse_presence.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\module\modulePresence.h"
#include "..\..\world\room.h"

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

//

bool Presence::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	zeroVRAnchor.load_from_xml(_node, TXT("zeroVRAnchor"));
	lockAsBase.load_from_xml(_node, TXT("lockAsBase"));
	attachAsIsToObjectVar.load_from_xml(_node, TXT("attachAsIsToObjectVar"));
	attachToObjectVar.load_from_xml(_node, TXT("attachToObjectVar"));
	attachToSocket.load_from_xml(_node, TXT("attachToSocket"));
	relativePlacement.load_from_xml(_node, TXT("relativePlacement"));
	detach.load_from_xml(_node, TXT("detach"));

	storeInRoomVar.load_from_xml(_node, TXT("storeInRoomVar"));

	setVelocityWS.load_from_xml(_node, TXT("setVelocityWS"));
	velocityImpulseWS.load_from_xml(_node, TXT("velocityImpulseWS"));
	setVelocityRotation.load_from_xml(_node, TXT("setVelocityRotation"));
	orientationImpulse.load_from_xml(_node, TXT("orientationImpulse"));

	return result;
}

bool Presence::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type Presence::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				if (auto* p = imo->get_presence())
				{
					if (zeroVRAnchor.is_set() && zeroVRAnchor.get())
					{
						p->zero_vr_anchor();
					}
					if (lockAsBase.is_set())
					{
						if (lockAsBase.get())
						{
							p->lock_as_base(true, true); // vr and player
						}
						else
						{
							p->unlock_as_base();
						}
					}
					if (detach.is_set() && detach.get())
					{
						p->detach();
					}
					if (storeInRoomVar.is_set() &&
						storeInRoomVar.get().is_valid())
					{
						_execution.access_variables().access< SafePtr<Framework::Room> >(storeInRoomVar.get()) = p->get_in_room();
					}
					if (setVelocityWS.is_set())
					{
						p->set_velocity_linear(setVelocityWS.get());
					}
					if (setVelocityRotation.is_set())
					{
						p->set_velocity_rotation(setVelocityRotation.get());
					}
					if (velocityImpulseWS.is_set())
					{
						p->add_velocity_impulse(velocityImpulseWS.get());
					}
					if (orientationImpulse.is_set())
					{
						p->add_orientation_impulse(orientationImpulse.get());
					}

					if (attachAsIsToObjectVar.is_set())
					{
						if (auto* exAtPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(attachAsIsToObjectVar.get()))
						{
							if (auto* atImo = exAtPtr->get())
							{
								if (attachToSocket.is_set())
								{
									p->request_attach_to_socket_using_in_room_placement(atImo, attachToSocket.get(), false, p->get_placement().to_world(relativePlacement.get(Transform::identity)), false);
								}
								else
								{
									p->request_attach_to_using_in_room_placement(atImo, false, p->get_placement().to_world(relativePlacement.get(Transform::identity)), false);
								}
							}
						}
					}
					if (attachToObjectVar.is_set())
					{
						if (auto* exAtPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(attachToObjectVar.get()))
						{
							if (auto* atImo = exAtPtr->get())
							{
								if (attachToSocket.is_set())
								{
									p->request_attach_to_socket(atImo, attachToSocket.get(), false, relativePlacement.get(Transform::identity), false);
								}
								else
								{
									p->request_attach(atImo, false, relativePlacement.get(Transform::identity), false);
								}
							}
						}
					}
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
