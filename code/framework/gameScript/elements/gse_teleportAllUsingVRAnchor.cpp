#include "gse_teleportAllUsingVRAnchor.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\module\moduleAI.h"

#include "..\..\ai\aiMessage.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\modulePresence.h"
#include "..\..\object\object.h"
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

bool TeleportAllUsingVRAnchor::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	mainObjectVar = _node->get_name_attribute(TXT("mainObjectVar"), mainObjectVar);

	useRoomVRAnchors = _node->get_bool_attribute_or_from_child_presence(TXT("useRoomVRAnchors"), useRoomVRAnchors);

	fromRoomVar = _node->get_name_attribute(TXT("fromRoomVar"), fromRoomVar);
	toRoomVar = _node->get_name_attribute(TXT("toRoomVar"), toRoomVar);

	tagged.load_from_xml_attribute(_node, TXT("tagged"));

	atPOI = _node->get_name_attribute(TXT("atPOI"), atPOI);

	return result;
}

bool TeleportAllUsingVRAnchor::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type TeleportAllUsingVRAnchor::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	Framework::Room* fromRoom = nullptr;
	Framework::Room* toRoom = nullptr;
	Framework::IModulesOwner* mainObject = nullptr;
	if (mainObjectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(mainObjectVar))
		{
			mainObject = exPtr->get();
		}
	}
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(fromRoomVar))
	{
		fromRoom = exPtr->get();
	}
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(toRoomVar))
	{
		toRoom = exPtr->get();
	}
	if (!fromRoom && mainObject)
	{
		fromRoom = mainObject->get_presence()->get_in_room();
	}
	if (!fromRoom)
	{
		error(TXT("no room to teleport from"));
	}
	if (!toRoom)
	{
		error(TXT("no room to teleport to"));
	}
	if (fromRoom && toRoom)
	{
		Array<Framework::IModulesOwner*> toTeleport;
		{
			toTeleport.make_space_for(fromRoom->get_objects().get_size());
			for_every_ptr(imo, fromRoom->get_objects())
			{
				if (imo->get_presence() &&
					!imo->get_presence()->is_attached())
				{
					if (tagged.check(imo->get_tags()))
					{
						toTeleport.push_back(imo);
					}
				}
			}
		}

		Transform fromRefPoint = fromRoom->get_vr_anchor();
		Transform toRefPoint = toRoom->get_vr_anchor();
		{
			if (!useRoomVRAnchors)
			{
				if (mainObject)
				{
					if (mainObject->get_presence()->get_in_room() == fromRoom)
					{
						fromRefPoint = mainObject->get_presence()->get_vr_anchor();
					}
					else
					{
						error(TXT("main object for teleportAll-- not in the \"fromRoom\""));
					}
				}
			}
			if (Game::is_using_sliding_locomotion() && atPOI.is_valid())
			{
				Framework::PointOfInterestInstance* poi = nullptr;
				if (fromRoom->find_any_point_of_interest(atPOI, poi))
				{
					fromRefPoint = poi->calculate_placement();
				}
				if (toRoom->find_any_point_of_interest(atPOI, poi))
				{
					toRefPoint = poi->calculate_placement();
				}
			}
		}

		Framework::ModulePresence::TeleportRequestParams params;
		for_every_ptr(imo, toTeleport)
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			if (auto* p = imo->get_presence())
			{
				Transform placement = p->get_placement();
				placement = fromRefPoint.to_local(placement);
				placement = toRefPoint.to_world(placement);
				params.find_vr_anchor(imo == mainObject);
				p->request_teleport(toRoom, placement, params);
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
