#include "gse_hideInputPrompt.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// overlay element id
DEFINE_STATIC_NAME(inputPrompt);

//

bool HideInputPrompt::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type HideInputPrompt::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->deactivate(NAME(inputPrompt));
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
