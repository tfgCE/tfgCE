#include "gsc_unloader.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\missionState.h"
#include "..\..\game\playerSetup.h"

#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::Unloader::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	areAnyPickupsOtherThanWeaponInRoomVar.load_from_xml(_node, TXT("areAnyPickupsOtherThanWeaponInRoomVar"));
	areAnyPickupsRequiredByMissionInRoomVar.load_from_xml(_node, TXT("areAnyPickupsRequiredByMissionInRoomVar"));

	return result;
}

bool Conditions::Unloader::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (areAnyPickupsOtherThanWeaponInRoomVar.is_set())
	{
		bool matches = false;
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(areAnyPickupsOtherThanWeaponInRoomVar.get()))
		{
			if (auto* room = exPtr->get())
			{
				for_every_ptr(obj, room->get_objects())
				{
					if (obj->get_custom<CustomModules::Pickup>())
					{
						if (obj->get_gameplay_as<ModuleEquipments::Gun>())
						{
							// ok
						}
						else
						{
							matches = true;
							break;
						}
					}
				}
			}
		}
		anyOk |= matches;
		anyFailed |= !matches;
	}

	if (areAnyPickupsRequiredByMissionInRoomVar.is_set())
	{
		bool matches = false;
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(areAnyPickupsRequiredByMissionInRoomVar.get()))
		{
			if (auto* room = exPtr->get())
			{
				if (!matches)
				{
					if (auto* ms = TeaForGodEmperor::MissionState::get_current())
					{
						for_every_ptr(obj, room->get_objects())
						{
							if (ms->is_item_required(obj))
							{
								matches = true;
								break;
							}
						}
					}
				}
				if (!matches)
				{
					if (auto* gs = TeaForGodEmperor::PlayerSetup::get_current().get_active_game_slot())
					{
						if (auto* lmr = gs->lastMissionResult.get())
						{
							for_every_ptr(obj, room->get_objects())
							{
								if (lmr->get_objectives().is_item_required(obj))
								{
									matches = true;
									break;
								}
							}
						}
					}
				}
			}
		}
		anyOk |= matches;
		anyFailed |= !matches;
	}

	return anyOk && !anyFailed;
}
