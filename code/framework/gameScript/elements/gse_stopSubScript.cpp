#include "gse_stopSubScript.h"

#include "..\gameScript.h"

#include "..\..\library\library.h"
#include "..\..\module\moduleAI.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool StopSubScript::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);

	return result;
}

bool StopSubScript::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type StopSubScript::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	_execution.stop_sub_script(id);
	return ScriptExecutionResult::Continue;
}
