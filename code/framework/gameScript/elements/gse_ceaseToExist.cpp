#include "gse_ceaseToExist.h"

#include "..\..\game\game.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\module\modulePresence.h"
#include "..\..\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool CeaseToExist::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute_or_from_child(TXT("objectVar"), objectVar);

	return result;
}

bool CeaseToExist::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type CeaseToExist::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				imo->cease_to_exist(false);
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
