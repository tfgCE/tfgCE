#include "gsc_gameModifiers.h"

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

bool Conditions::GameModifiers::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	isOn = _node->get_name_attribute_or_from_child(TXT("isOn"), isOn);

	return result;
}

bool Conditions::GameModifiers::check(Framework::GameScript::ScriptExecution const& _execution) const
{
	bool result = false;

	if (isOn.is_valid())
	{
		if (auto* ps = PlayerSetup::get_current_if_exists())
		{
			if (auto* gs = ps->get_active_game_slot())
			{
				auto& rs = gs->runSetup;
				if (rs.activeGameModifiers.get_tag(isOn))
				{
					result = true;
				}
			}
		}
	}

	return result;
}
