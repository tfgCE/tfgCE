#include "gsc_lastMissionResult.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\missionResult.h"
#include "..\..\game\playerSetup.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::LastMissionResult::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	success.load_from_xml(_node, TXT("success"));
	failed.load_from_xml(_node, TXT("failed"));

	return result;
}

bool Conditions::LastMissionResult::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	MissionResult* lastMissionResult = nullptr;
	if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
	{
		lastMissionResult = gs->get_last_mission_result();
	}

	if (success.is_set())
	{
		bool matches = lastMissionResult && lastMissionResult->is_success();
		anyOk |= matches;
		anyFailed |= !matches;
	}

	if (failed.is_set())
	{
		bool matches = lastMissionResult && lastMissionResult->is_failed();
		anyOk |= matches;
		anyFailed |= !matches;
	}

	return anyOk && !anyFailed;
}
