#include "gsc_missionState.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\missionState.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::MissionState::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	isActive.load_from_xml(_node, TXT("isActive"));
	isReady.load_from_xml(_node, TXT("isReady"));
	doesRequireToBringItems.load_from_xml(_node, TXT("doesRequireToBringItems"));

	return result;
}

bool Conditions::MissionState::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	bool msActive = false;
	bool msReady = false;
	bool msRequiresToBringItems = false;
	if (auto* ms = TeaForGodEmperor::MissionState::get_current())
	{
		msActive = true;
		msReady = ms->is_ready_to_use();
		msRequiresToBringItems = ms->does_require_items_to_bring();
	}

	if (isActive.is_set())
	{
		bool matches = isActive.get() == msActive;
		anyOk |= matches;
		anyFailed |= !matches;
	}

	if (isReady.is_set())
	{
		bool matches = isReady.get() == msReady;
		anyOk |= matches;
		anyFailed |= !matches;
	}

	if (doesRequireToBringItems.is_set())
	{
		bool matches = doesRequireToBringItems.get() == msRequiresToBringItems;
		anyOk |= matches;
		anyFailed |= !matches;
	}

	return anyOk && !anyFailed;
}
