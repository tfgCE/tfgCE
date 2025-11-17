#include "gse_tutInput.h"

#include "..\..\tutorials\tutorialSystem.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

Framework::GameScript::ScriptExecutionResult::Type TutAllowAllInput::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->allow_all_input();

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

//

Framework::GameScript::ScriptExecutionResult::Type TutBlockAllInput::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->block_all_input();

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

//

bool TutInput::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	{
		inputs.clear();
		String inputString = _node->get_string_attribute(TXT("input"));
		List<String> tokens;
		inputString.split(String::comma(), tokens);
		for_every(token, tokens)
		{
			inputs.push_back(Name(token->trim()));
		}
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TutAllowInput::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	for_every(input, inputs)
	{
		TutorialSystem::get()->allow_input(*input);
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

//

Framework::GameScript::ScriptExecutionResult::Type TutBlockInput::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	for_every(input, inputs)
	{
		TutorialSystem::get()->block_input(*input);
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

//

Framework::GameScript::ScriptExecutionResult::Type TutAllowHubInput::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->allow_hub_input();

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

//

Framework::GameScript::ScriptExecutionResult::Type TutBlockHubInput::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->block_hub_input();

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

//

Framework::GameScript::ScriptExecutionResult::Type TutAllowOverWidget::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->allow_over_widget_explicit();

	return Framework::GameScript::ScriptExecutionResult::Continue;
}

//

Framework::GameScript::ScriptExecutionResult::Type TutRestoreOverWidget::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	TutorialSystem::get()->restore_over_widget_explicit();

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
