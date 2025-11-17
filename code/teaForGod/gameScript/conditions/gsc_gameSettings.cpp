#include "gsc_gameSettings.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\gameSettings.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

DEFINE_STATIC_NAME(playerImmortal);
DEFINE_STATIC_NAME(autoMapOnly);
DEFINE_STATIC_NAME(showDirectionsOnlyToObjective);
DEFINE_STATIC_NAME(hostilesAllowed);
DEFINE_STATIC_NAME(aiSpeedBasedModifier);
DEFINE_STATIC_NAME(simplerPuzzles);
DEFINE_STATIC_NAME(playerDamageTaken);

//

bool Conditions::GameSettings::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	setting.load_from_xml(_node, TXT("setting"));
	greaterThan.load_from_xml(_node, TXT("greaterThan"));
	lessThan.load_from_xml(_node, TXT("lessThan"));

	return result;
}

bool Conditions::GameSettings::check(Framework::GameScript::ScriptExecution const& _execution) const
{
	bool result = false;

	auto& gs = TeaForGodEmperor::GameSettings::get();


	if (setting.is_set())
	{
		if (setting.get() == NAME(playerImmortal))
		{
			return gs.difficulty.playerImmortal;
		}
		else if (setting.get() == NAME(autoMapOnly))
		{
			return gs.difficulty.autoMapOnly;
		}
		else if (setting.get() == NAME(showDirectionsOnlyToObjective))
		{
			return gs.difficulty.showDirectionsOnlyToObjective;
		}
		else if (setting.get() == NAME(hostilesAllowed))
		{
			return gs.difficulty.npcs > TeaForGodEmperor::GameSettings::NPCS::NoHostiles;
		}
		else if (setting.get() == NAME(simplerPuzzles))
		{
			return gs.difficulty.simplerPuzzles;
		}
		else if (setting.get() == NAME(aiSpeedBasedModifier))
		{
			return compare_value(gs.difficulty.aiSpeedBasedModifier);
		}
		else if (setting.get() == NAME(playerDamageTaken))
		{
			return compare_value(gs.difficulty.playerDamageTaken);
		}
		else
		{
			todo_implement(TXT("implement support for other settings"));
		}
	}

	return result;
}

bool Conditions::GameSettings::compare_value(float _value) const
{
	if (greaterThan.is_set())
	{
		return _value > greaterThan.get();
	}
	else if (lessThan.is_set())
	{
		return _value < lessThan.get();
	}
	else
	{
		warn_dev_investigate(TXT("what am I supposed to do with this value?"));
		return _value > 0.0f;
	}
}
