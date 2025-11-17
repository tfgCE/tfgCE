#include "gsc_gameplaySpecial.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\ai\logics\aiLogic_centipede.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

DEFINE_STATIC_NAME(centipedeExists);

//

bool Conditions::GameplaySpecial::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	condition.load_from_xml(_node, TXT("condition"));

	return result;
}

bool Conditions::GameplaySpecial::check(Framework::GameScript::ScriptExecution const& _execution) const
{
	bool result = false;

	if (condition.is_set())
	{
		if (condition.get() == NAME(centipedeExists))
		{
			return AI::Logics::CentipedeSelfManager::get_existing() != nullptr;
		}
	}

	return result;
}
