#include "navTask_PathTask.h"

#ifdef AN_LOG_NAV_TASKS
#include "..\..\..\core\other\parserUtils.h"
#endif

using namespace Framework;
using namespace Nav;
using namespace Tasks;

//

REGISTER_FOR_FAST_CAST(PathTask);

PathTask::PathTask(bool _writer)
: base(_writer)
{
}

#ifdef AN_LOG_NAV_TASKS
void PathTask::log_internal(LogInfoContext& _log) const
{
	base::log_internal(_log);

#ifdef AN_64
	_log.log(TXT("path %S"), ParserUtils::uint64_to_hex((uint64)path.get()).to_char());
#else
	_log.log(TXT("path %S"), ParserUtils::uint_to_hex((uint)path.get()).to_char());
#endif
}
#endif
