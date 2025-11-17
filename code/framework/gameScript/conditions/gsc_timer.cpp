#include "gsc_timer.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool Timer::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	timerID = _node->get_name_attribute(TXT("timer"), timerID);
	timerID = _node->get_name_attribute(TXT("id"), timerID);

	lessOrEqual.load_from_xml(_node, TXT("lessOrEqual"));
	greaterOrEqual.load_from_xml(_node, TXT("greaterOrEqual"));

	return result;
}

bool Timer::check(ScriptExecution const & _execution) const
{
	bool anyOk = false;
	bool anyFailed = false;

	float t = _execution.get_timer(timerID);
	
	if (lessOrEqual.is_set())
	{
		if (t <= lessOrEqual.get())
		{
			anyOk = true;
		}
		else
		{
			anyFailed = true;
		}
	}

	if (greaterOrEqual.is_set())
	{
		if (t >= greaterOrEqual.get())
		{
			anyOk = true;
		}
		else
		{
			anyFailed = true;
		}
	}

	return anyOk && ! anyFailed;
}
