#include "gse_triggerTrap.h"

#include "..\..\..\core\io\xml.h"

#include "..\gameScript.h"
#include "..\registeredGameScriptConditions.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

REGISTER_FOR_FAST_CAST(TriggerTrap);

bool TriggerTrap::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	error_loading_xml_on_assert(name.is_valid(), result, _node, TXT("\"name\" for trap required"));

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

bool TriggerTrap::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (ifCondition.is_set())
	{
		result &= ifCondition->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

ScriptExecutionResult::Type TriggerTrap::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (check_condition(_execution))
	{
		auto* script = _execution.get_script();
		auto at = _execution.get_at();

		GameScript::ScriptExecution::trigger_execution_trap(name);

		if (script == _execution.get_script() &&
			at == _execution.get_at())
		{
			// move on
			return ScriptExecutionResult::Continue;
		}
		else
		{
			// indicate we changed
			return ScriptExecutionResult::SetNextInstruction;
		}
	}

	// move on
	return ScriptExecutionResult::Continue;
}

bool TriggerTrap::check_condition(ScriptExecution& _execution) const
{
	if (ifCondition.is_set())
	{
		return ifCondition->check(_execution);
	}
	else
	{
		return true;
	}
}
