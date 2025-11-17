#include "gse_goToLabel.h"

#include "gse_label.h"

#include "..\gameScript.h"
#include "..\registeredGameScriptConditions.h"

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

bool GoToLabel::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	id = _node->get_name_attribute_or_from_child(TXT("name"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("\"id\" for label required"));
	if (auto* node = _node->first_child_named(TXT("if")))
	{
		if (auto* condNode = node->first_child())
		{
			ifCondition = RegisteredScriptConditions::create(condNode->get_name().to_char());
			if (ifCondition.is_set())
			{
				result &= ifCondition->load_from_xml(condNode, _lc);
			}
		}
	}

	return result;
}

bool GoToLabel::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (ifCondition.is_set())
	{
		result &= ifCondition->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

ScriptExecutionResult::Type GoToLabel::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (ifCondition.is_set())
	{
		if (!ifCondition->check(_execution))
		{
			// just skip it
			return ScriptExecutionResult::Continue;
		}
	}

	int labelAt = NONE;
	if (auto* script = _execution.get_script())
	{
		for_every_ref(e, script->get_elements())
		{
			if (auto* label = fast_cast<Label>(e))
			{
				if (label->get_id() == id)
				{
					labelAt = for_everys_index(e);
					break;
				}
			}
		}
	}

	if (labelAt != NONE)
	{
		_execution.set_at(labelAt);
	}
	else
	{
		error(TXT("did not find label \"%S\""), id.to_char());
	}

	return ScriptExecutionResult::SetNextInstruction;
}
