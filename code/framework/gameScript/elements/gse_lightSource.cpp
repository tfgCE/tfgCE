#include "gse_lightSource.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\module\moduleAI.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\module\modulePresence.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\module\custom\mc_lightSources.h"

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

bool Framework::GameScript::Elements::LightSource::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	activate = _node->get_name_attribute(TXT("activate"), activate);
	deactivate = _node->get_name_attribute(TXT("deactivate"), deactivate);
	
	blendTime.load_from_xml(_node, TXT("blendTime"));

	return result;
}

bool Framework::GameScript::Elements::LightSource::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type Framework::GameScript::Elements::LightSource::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			if (auto* ls = imo->get_custom<Framework::CustomModules::LightSources>())
			{
				if (activate.is_valid())
				{
					ls->add(activate, true);
				}
				if (deactivate.is_valid())
				{
					ls->fade_out(deactivate, blendTime);
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
