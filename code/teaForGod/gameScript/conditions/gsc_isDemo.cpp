#include "gsc_isDemo.h"

#include "..\..\teaForGod.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::IsDemo::check(Framework::GameScript::ScriptExecution const& _execution) const
{
	return TeaForGodEmperor::is_demo();
}
