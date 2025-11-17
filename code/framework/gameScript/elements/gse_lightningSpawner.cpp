#include "gse_lightningSpawner.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\module\custom\mc_lightningSpawner.h"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"

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

bool LightningSpawner::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	start.load_from_xml(_node, TXT("start"));
	stop.load_from_xml(_node, TXT("stop"));

	return result;
}

bool LightningSpawner::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type LightningSpawner::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			if (auto* ls = imo->get_custom<Framework::CustomModules::LightningSpawner>())
			{
				if (start.is_set())
				{
					ls->start(start.get());
				}
				if (stop.is_set())
				{
					ls->stop(stop.get());
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
