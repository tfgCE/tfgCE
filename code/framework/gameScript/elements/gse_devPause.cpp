#include "gse_devPause.h"

#include "..\gameScript.h"

#include "..\..\..\core\system\core.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool DevPause::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	return result;
}

bool DevPause::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type DevPause::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	::System::Core::one_frame_time_pace();
#endif

	return ScriptExecutionResult::Continue;
}
