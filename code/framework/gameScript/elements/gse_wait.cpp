#include "gse_wait.h"

#include "..\..\..\core\random\random.h"
#include "..\..\..\core\system\core.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

// variable
DEFINE_STATIC_NAME(wait);

//

bool Wait::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	time.load_from_xml(_node, TXT("time"));
	time.load_from_xml(_node, TXT("for"));

	if (time.is_empty() && !_node->is_empty())
	{
		error_loading_xml(_node, TXT("wait with content but no for/time, use <wait for=\"in seconds\"/>"));
	}

	return result;
}

ScriptExecutionResult::Type Wait::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (time.is_empty())
	{
		// stay here
		return ScriptExecutionResult::Repeat;
	}
	else
	{
		float& waitVar = _execution.access_variables().access<float>(NAME(wait));

		if (is_flag_set(_flags, ScriptExecution::Entered))
		{
			waitVar = Random::get(time);
		}
		else
		{
			float deltaTime = ::System::Core::get_delta_time();
			waitVar -= deltaTime;
		}

		if (waitVar <= 0.0f)
		{
			return ScriptExecutionResult::Continue;
		}
		else
		{
			return ScriptExecutionResult::Repeat;
		}
	}
}
