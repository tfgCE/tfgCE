#include "gsc_playerPreferences.h"

#include "..\registeredGameScriptConditions.h"

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

DEFINE_STATIC_NAME(imperial);
DEFINE_STATIC_NAME(metric);

//

bool Conditions::PlayerPreferences::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	measurementSystem.load_from_xml(_node, TXT("measurementSystem"));
	flickeringLights.load_from_xml(_node, TXT("flickeringLights"));
	showTipsDuringGame.load_from_xml(_node, TXT("showTipsDuringGame"));
	crouchingAllowed.load_from_xml(_node, TXT("crouchingAllowed"));

	return result;
}

bool Conditions::PlayerPreferences::check(Framework::GameScript::ScriptExecution const& _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	if (measurementSystem.is_set())
	{
		if (auto* ps = TeaForGodEmperor::PlayerSetup::access_current_if_exists())
		{
			auto ms = ps->get_preferences().get_measurement_system();
			if (measurementSystem.get() == NAME(imperial))
			{
				bool ok = ms == MeasurementSystem::Imperial;
				anyOk |= ok;
				anyFailed |= !ok;
			}
			else if (measurementSystem.get() == NAME(metric))
			{
				bool ok = ms == MeasurementSystem::Metric;
				anyOk |= ok;
				anyFailed |= !ok;
			}
			else
			{
				todo_implement;
			}
		}
	}

	if (flickeringLights.is_set())
	{
		bool allowed = TeaForGodEmperor::PlayerPreferences::are_currently_flickering_lights_allowed();
		bool ok = flickeringLights.get() == allowed;
		anyOk |= ok;
		anyFailed |= !ok;
	}

	if (showTipsDuringGame.is_set())
	{
		bool allowed = TeaForGodEmperor::PlayerPreferences::should_show_tips_during_game();
		bool ok = showTipsDuringGame.get() == allowed;
		anyOk |= ok;
		anyFailed |= !ok;
	}

	if (crouchingAllowed.is_set())
	{
		bool allowed = TeaForGodEmperor::PlayerPreferences::is_crouching_allowed();
		bool ok = crouchingAllowed.get() == allowed;
		anyOk |= ok;
		anyFailed |= !ok;
	}

	return anyOk && !anyFailed;
}
