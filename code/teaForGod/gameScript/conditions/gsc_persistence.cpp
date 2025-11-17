#include "gsc_persistence.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\persistence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::Persistence::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	tooManyWeapons.load_from_xml(_node, TXT("tooManyWeapons"));

	return result;
}

bool Conditions::Persistence::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

bool Conditions::Persistence::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	auto& p = TeaForGodEmperor::Persistence::get_current();
	{
		if (tooManyWeapons.is_set())
		{
			bool areThereTooManyWeapons = p.get_weapons_in_armoury().get_size() > p.get_max_weapons_in_armoury();
			bool matches = areThereTooManyWeapons == tooManyWeapons.get();
			anyOk |= matches;
			anyFailed |= !matches;
		}
	}

	return anyOk && !anyFailed;
}
