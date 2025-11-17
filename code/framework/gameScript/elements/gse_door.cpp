#include "gse_door.h"

#include "..\..\game\game.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\world\door.h"
#include "..\..\world\room.h"
#include "..\..\world\subWorld.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool GameScript::Elements::Door::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	doorVar = _node->get_name_attribute(TXT("doorVar"), doorVar);

	if (auto* attr = _node->get_attribute(TXT("setOperation")))
	{
		setOperation = Framework::DoorOperation::parse(attr->get_as_string());
	}
	operationForced = _node->get_bool_attribute_or_from_child_presence(TXT("operationForced"), operationForced);
	operationForced = ! _node->get_bool_attribute_or_from_child_presence(TXT("operationNotForced"), ! operationForced);
	setOperationLock.load_from_xml(_node, TXT("setOperationLock"));
	setAutoOperationDistance.load_from_xml(_node, TXT("setAutoOperationDistance"));
	
	setMute.load_from_xml(_node, TXT("setMute"));
	setVisible.load_from_xml(_node, TXT("setVisible"));
	
	placeAtPOI.load_from_xml(_node, TXT("placeAtPOI"));
	placeAtPOIInRoomVar.load_from_xml(_node, TXT("placeAtPOIInRoomVar"));

	return result;
}

bool GameScript::Elements::Door::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

ScriptExecutionResult::Type GameScript::Elements::Door::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Door>>(doorVar))
	{
		if (auto* d = exPtr->get())
		{
			if (setOperation.is_set())
			{
				d->set_operation(setOperation.get(), Framework::Door::DoorOperationParams().force_operation(operationForced));
			}
			if (setOperationLock.is_set())
			{
				d->set_operation_lock(setOperationLock.get());
			}
			if (setAutoOperationDistance.is_set())
			{
				d->set_auto_operation_distance(setAutoOperationDistance.get());
			}
			if (setMute.is_set())
			{
				d->set_mute(setMute.get());
			}
			if (setVisible.is_set())
			{
				d->set_visible(setVisible.get());
			}
			if (placeAtPOI.is_set())
			{
				if (auto* g = Game::get())
				{
					Name placeAtPOIName = placeAtPOI.get();
					SafePtr<Framework::Door> door;
					SafePtr<Framework::Room> inRoom;
					door = d;
					if (placeAtPOIInRoomVar.is_set())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(placeAtPOIInRoomVar.get()))
						{
							if (auto* r = exPtr->get())
							{
								inRoom = r;
							}
						}
					}
					g->add_immediate_sync_world_job(TXT("place door"), [placeAtPOIName, inRoom, door]()
						{
							if (auto* d = door.get())
							{
								Framework::PointOfInterestInstance* poi = nullptr;
								{
									if (auto* r = inRoom.get())
									{
										r->find_any_point_of_interest(placeAtPOIName, poi);
									}
									else
									{
										d->get_in_sub_world()->find_any_point_of_interest(placeAtPOIName, poi);
									}
								}
								if (poi)
								{
									Framework::DoorInRoom* dir = nullptr;
									if (auto* r = inRoom.get())
									{
										dir = d->get_linked_door_in(r);
									}
									if (!dir)
									{
										dir = d->get_linked_door_a();
									}
									if (dir)
									{
										dir->set_placement(poi->calculate_placement());
									}
								}
							}
						});
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
