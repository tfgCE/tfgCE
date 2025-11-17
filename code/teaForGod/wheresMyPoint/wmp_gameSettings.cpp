#include "wmp_gameSettings.h"

#include "..\game\gameSettings.h"

#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\framework\framework.h"
#include "..\..\framework\meshGen\meshGenGenerationContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// settings
DEFINE_STATIC_NAME(aiSpeedBasedModifier);
DEFINE_STATIC_NAME(linearLevels);

//

bool GameSettingsForWheresMyPoint::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	setting = _node->get_name_attribute(TXT("setting"), setting);

	return result;
}

bool GameSettingsForWheresMyPoint::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	if (setting == NAME(aiSpeedBasedModifier))
	{
		_value.set<float>(TeaForGodEmperor::GameSettings::get().difficulty.aiSpeedBasedModifier);
	}
	else if (setting == NAME(linearLevels))
	{
		_value.set<bool>(TeaForGodEmperor::GameSettings::get().difficulty.linearLevels);
	}
	else
	{
		error_processing_wmp_tool(this, TXT("could not recognise game setting \"%S\""), setting.to_char());
		result = false;
	}

	return result;
}
