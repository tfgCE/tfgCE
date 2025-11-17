#include "gse_controller.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\module\moduleController.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Controller::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	clearRequestedLook.load_from_xml(_node, TXT("clearRequestedLook"));

	return result;
}

bool Controller::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type Controller::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				if (auto* c = imo->get_controller())
				{
					if (clearRequestedLook.is_set() && clearRequestedLook.get())
					{
						c->clear_requested_relative_look();
						c->clear_requested_look_orientation();
					}
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
