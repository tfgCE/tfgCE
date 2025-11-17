#include "gse_teleport.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\module\moduleAI.h"

#include "..\..\ai\aiMessage.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\modulePresence.h"
#include "..\..\world\room.h"

#include "..\..\..\core\io\xml.h"

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

bool Teleport::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	absoluteRotation.load_from_xml(_node, TXT("absoluteRotation"));
	absoluteLocationOffset.load_from_xml(_node, TXT("absoluteLocationOffset"));
	relativePlacement.load_from_xml(_node, TXT("relativePlacement"));

	toPlacement.load_from_xml(_node, TXT("toPlacement"));
	toPlacementVar = _node->get_name_attribute(TXT("toPlacementVar"), toPlacementVar);

	toRoomVar = _node->get_name_attribute(TXT("toRoomVar"), toRoomVar);
	toObjectVar = _node->get_name_attribute(TXT("toObjectVar"), toObjectVar);
	toSocket = _node->get_name_attribute(TXT("toSocket"), toSocket);
	toPOI = _node->get_name_attribute(TXT("toPOI"), toPOI);

	offsetPlacementRelativelyToParam = Name::invalid();
	for_every(node, _node->children_named(TXT("offsetPlacementRelatively")))
	{
		offsetPlacementRelativelyToParam = node->get_name_attribute(TXT("toParam"));
	}

	silent = _node->get_bool_attribute_or_from_child_presence(TXT("silent"), silent);
	keepVelocityOS = _node->get_bool_attribute_or_from_child_presence(TXT("keepVelocityOS"), keepVelocityOS);

	return result;
}

bool Teleport::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

Room* Teleport::get_to_room(ScriptExecution& _execution) const
{
	if (toRoomVar.is_valid())
	{
		if (auto* ptr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(toRoomVar))
		{
			return ptr->get();
		}
	}
	return nullptr;
}

ScriptExecutionResult::Type Teleport::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());

			Framework::Room* toRoom = imo->get_presence()->get_in_room();
			if (auto* r = get_to_room(_execution))
			{
				toRoom = r;
			}

			Optional<Transform> tp = toPlacement;
			if (toPlacementVar.is_valid())
			{
				if (auto* ptr = _execution.get_variables().get_existing<Transform>(toPlacementVar))
				{
					tp = *ptr;
				}
			}

			if (toObjectVar.is_valid())
			{
				if (auto* ptr = _execution.get_variables().get_existing< SafePtr<Framework::IModulesOwner>>(toObjectVar))
				{
					if (auto* toImo = ptr->get())
					{
						if (auto* p = toImo->get_presence())
						{
							Transform newPlacement = p->get_placement();
							if (toSocket.is_valid())
							{
								if (auto* a = toImo->get_appearance())
								{
									newPlacement = newPlacement.to_world(a->calculate_socket_os(toSocket));
								}
							}
							tp = newPlacement.to_world(tp.get(Transform::identity));
						}
					}
				}
			}

			if (toPOI.is_valid())
			{
				Framework::PointOfInterestInstance * foundPOI = nullptr;
				if (toRoom->find_any_point_of_interest(toPOI, foundPOI))
				{
					tp = foundPOI->calculate_placement();
				}
			}

			alter_target_placement(REF_ tp);

			if (tp.is_set())
			{
				if (offsetPlacementRelativelyToParam.is_valid() && toRoom)
				{
					Transform offsetPlacement = toRoom->get_variables().get_value<Transform>(offsetPlacementRelativelyToParam, Transform::identity);
					tp = offsetPlacement.to_world(tp.get());
				}
			}

			if (tp.is_set() && absoluteLocationOffset.is_set())
			{
				tp.access().set_translation(tp.get().get_translation() + absoluteLocationOffset.get());
			}

			if (tp.is_set() && absoluteRotation.is_set())
			{
				tp.access().set_orientation(absoluteRotation.get().to_quat());
			}

			if (tp.is_set() && relativePlacement.is_set())
			{
				tp = tp.get().to_world(relativePlacement.get());
			}

			if (tp.is_set())
			{
				if (auto* p = imo->get_presence())
				{
					Framework::ModulePresence::TeleportRequestParams params;
					params.silent_teleport(silent);
					params.keep_velocity_os(keepVelocityOS);

					if (toRoom && toRoom != p->get_in_room())
					{
						p->request_teleport(toRoom, tp.get());
					}
					else
					{
						p->request_teleport_within_room(tp.get());
					}
					p->set_velocity_linear(Vector3::zero);
					p->set_velocity_rotation(Rotator3::zero);
				}
			}
			else
			{
				if (toRoom)
				{
					error(TXT("provided room to teleport to but no placement"));
				}
			}
		}
	}
	else
	{
		warn(TXT("missing object"));
	}

	return ScriptExecutionResult::Continue;
}
