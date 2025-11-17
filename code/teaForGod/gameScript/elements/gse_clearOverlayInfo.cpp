#include "gse_clearOverlayInfo.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool ClearOverlayInfo::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	if (_node->has_attribute(TXT("keepId")))
	{
		Name keepId = _node->get_name_attribute(TXT("keepId"));
		if (keepId.is_valid())
		{
			keepIds.push_back(keepId);
		}
	}
	for_every(node, _node->children_named(TXT("keep")))
	{
		Name keepId = node->get_name_attribute(TXT("id"));
		if (keepId.is_valid())
		{
			keepIds.push_back(keepId);
		}
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type ClearOverlayInfo::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* ois = OverlayInfo::System::get())
	{
		if (keepIds.is_empty())
		{
			ois->deactivate_all();
			if (!ois->is_empty())
			{
				return Framework::GameScript::ScriptExecutionResult::Repeat;
			}
		}
		else
		{
			if (!ois->deactivate_but_keep(keepIds))
			{
				return Framework::GameScript::ScriptExecutionResult::Repeat;
			}
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
