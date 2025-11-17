#include "gse_endMission.h"

#include "..\..\game\game.h"
#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::EndMission::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	storeWeaponsInArmoury = _node->get_bool_attribute_or_from_child_presence(TXT("storeWeaponsInArmoury"), storeWeaponsInArmoury);

	for_every(node, _node->children_named(TXT("processItems")))
	{
		Name roomVar = node->get_name_attribute(TXT("inRoomVar"));
		if (roomVar.is_valid())
		{
			processItemsInRoomVar.push_back(roomVar);
		}
	}

	return result;
}

bool Elements::EndMission::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::EndMission::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* ms = MissionState::get_current())
	{
		MissionState::EndParams endParams;
		{
			for_every(roomVar, processItemsInRoomVar)
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(*roomVar))
				{
					if (auto* room = exPtr->get())
					{
						endParams.processItemsInRooms.push_back(room);
					}
				}
			}
		}
		ms->end(endParams);
	}

	return Framework::GameScript::ScriptExecutionResult::Yield;
}
