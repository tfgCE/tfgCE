#include "gse_tutHighlight.h"

#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool TutHighlight::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	what = _node->get_name_attribute(TXT("what"), what);

	error_loading_xml_on_assert(what.is_valid(), result, _node, TXT("no \"what\" provided to highlight"));

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TutHighlight::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (what.is_valid())
	{
		TutorialSystem::get()->highlight(what);
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
