#include "gse_startSubScript.h"

#include "..\gameScript.h"
#include "..\registeredGameScriptConditions.h"

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

bool StartSubScript::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);

	result &= script.load_from_xml(_node, TXT("script"), _lc);

	error_loading_xml_on_assert(script.is_name_valid(), result, _node, TXT("no \"script\" provided"));

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

bool StartSubScript::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= script.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	if (ifCondition.is_set())
	{
		result &= ifCondition->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void StartSubScript::load_on_demand_if_required()
{
	base::load_on_demand_if_required();
	if (auto* s = script.get())
	{
		s->load_on_demand_if_required();
		s->load_elements_on_demand_if_required();
	}
}

ScriptExecutionResult::Type StartSubScript::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (ifCondition.is_set())
	{
		if (!ifCondition->check(_execution))
		{
			// just skip it
			return ScriptExecutionResult::Continue;
		}
	}

	if (auto* s = script.get())
	{
		_execution.start_sub_script(id, s);
	}
	return ScriptExecutionResult::Continue;
}
