#include "gse_timer.h"

#include "..\gameScript.h"

#include "..\..\ai\aiMessage.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\modulePresence.h"
#include "..\..\world\room.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"

#include "..\..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool Timer::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	resetTimer = _node->get_name_attribute(TXT("reset"), resetTimer);
	resetIfNotActive = _node->get_name_attribute(TXT("resetIfNotActive"), resetIfNotActive);
	removeTimer = _node->get_name_attribute(TXT("remove"), removeTimer);

	return result;
}

ScriptExecutionResult::Type Timer::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (resetTimer.is_valid())
	{
		_execution.reset_timer(resetTimer);
	}
	if (resetIfNotActive.is_valid())
	{
		_execution.reset_timer(resetIfNotActive, true);
	}
	if (removeTimer.is_valid())
	{
		_execution.remove_timer(removeTimer);
	}

	return ScriptExecutionResult::Continue;
}
