#include "gse_changeScript.h"

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

bool ChangeScript::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	result &= script.load_from_xml(_node, TXT("to"), _lc);

	error_loading_xml_on_assert(script.is_name_valid(), result, _node, TXT("to script \"to\" provided"));

	return result;
}

bool ChangeScript::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= script.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void ChangeScript::load_on_demand_if_required()
{
	base::load_on_demand_if_required();
	if (auto* s = script.get())
	{
		s->load_on_demand_if_required();
		s->load_elements_on_demand_if_required();
	}
}

ScriptExecutionResult::Type ChangeScript::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				if (auto* ai = fast_cast<ModuleAI>(imo->get_ai()))
				{
					ai->start_game_script(script.get());
				}
				else
				{
					error(TXT("no module AI to change script"));
				}
			}
		}
		else
		{
			error(TXT("no object to change script"));
		}
		return ScriptExecutionResult::Continue;
	}
	else
	{
		_execution.start(script.get());
		return ScriptExecutionResult::SetNextInstruction;
	}
}
