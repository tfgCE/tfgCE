#include "gse_emissive.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\modules\moduleAI.h"
#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\framework\gameScript\gameScript.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool Emissive::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	stop = _node->get_bool_attribute_or_from_child_presence(TXT("stop"), stop);

	activate = _node->get_name_attribute(TXT("activate"), activate);
	deactivate = _node->get_name_attribute(TXT("deactivate"), deactivate);
	
	blendTime.load_from_xml(_node, TXT("blendTime"));

	return result;
}

bool Emissive::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Emissive::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			if (auto* ec = imo->get_custom<CustomModules::EmissiveControl>())
			{
				if (stop)
				{
					ec->emissive_deactivate_all(0.0f);
				}
				if (activate.is_valid())
				{
					ec->emissive_activate(activate, blendTime);
				}
				if (deactivate.is_valid())
				{
					ec->emissive_deactivate(deactivate, blendTime);
				}
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
