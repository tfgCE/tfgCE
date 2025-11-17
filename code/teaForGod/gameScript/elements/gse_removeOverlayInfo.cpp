#include "gse_removeOverlayInfo.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool RemoveOverlayInfo::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type RemoveOverlayInfo::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (id.is_valid())
	{
		if (auto* ois = OverlayInfo::System::get())
		{
			ois->deactivate(id);
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
