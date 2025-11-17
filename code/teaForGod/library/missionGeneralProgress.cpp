#include "missionGeneralProgress.h"

#include "..\game\persistence.h"

#include "..\utils\unlockableCustomUpgrades.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(MissionGeneralProgress);
LIBRARY_STORED_DEFINE_TYPE(MissionGeneralProgress, missionGeneralProgress);

MissionGeneralProgress::MissionGeneralProgress(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

MissionGeneralProgress::~MissionGeneralProgress()
{
}

bool MissionGeneralProgress::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	requiresToUnlock.clear();
	playingGives.clear();
	successGives.clear();
	failureGives.clear();
	unlockCost = 0; // merit points


	// load
	//
	requiresToUnlock.load_from_xml_attribute_or_child_node(_node, TXT("requiresToUnlock"));
	playingGives.load_from_xml_attribute_or_child_node(_node, TXT("playingGives"));
	successGives.load_from_xml_attribute_or_child_node(_node, TXT("successGives"));
	failureGives.load_from_xml_attribute_or_child_node(_node, TXT("failureGives"));
	unlockCost = _node->get_int_attribute_or_from_child(TXT("unlockCost"), unlockCost);


	return result;
}

bool MissionGeneralProgress::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool MissionGeneralProgress::is_available_to_unlock(OPTIONAL_ UnlockableCustomUpgradesContext const* _context) const
{
	auto& p = Persistence::get_current();
	if (p.is_mission_general_progress_unlocked(this))
	{
		// already unlocked
		return false;
	}
	if (! requiresToUnlock.check(p.get_mission_general_progress_info()))
	{
		return false;
	}
	if (_context)
	{
		bool present = false;
		for_every(am, _context->availableMissions)
		{
			if (am->mission && am->mission->get_mission_general_progress() == this)
			{
				present = true;
				break;
			}
		}
		if (!present)
		{
			return false;
		}
	}
	return true;
}

bool MissionGeneralProgress::is_available_to_play() const
{
	auto& p = Persistence::get_current();
	if (p.is_mission_general_progress_unlocked(this))
	{
		// already unlocked
		return true;
	}
	if (!does_require_unlocking())
	{
		return true;
	}
	return false;
}
