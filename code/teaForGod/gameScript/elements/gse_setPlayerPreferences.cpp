#include "gse_setPlayerPreferences.h"

#include "..\..\teaEnums.h"

#include "..\..\game\playerSetup.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool SetPlayerPreferences::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	healthIndicator.load_from_xml(_node, TXT("healthIndicator"));
	healthIndicatorIntensity.load_from_xml(_node, TXT("healthIndicatorIntensity"));
	hitIndicatorIntensity.load_from_xml(_node, TXT("hitIndicatorIntensity"));
	if (auto* attr = _node->get_attribute(TXT("hitIndicatorType")))
	{
		hitIndicatorType = HitIndicatorType::parse(attr->get_as_string());
	}

	return result;
}

#define APPLY_PREFERENCE(what) if (what.is_set()) pp.what = what.get();

Framework::GameScript::ScriptExecutionResult::Type SetPlayerPreferences::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	auto& pp = PlayerSetup::access_current().access_preferences();

	APPLY_PREFERENCE(healthIndicator);
	APPLY_PREFERENCE(healthIndicatorIntensity);
	APPLY_PREFERENCE(hitIndicatorIntensity);
	APPLY_PREFERENCE(hitIndicatorType);

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
