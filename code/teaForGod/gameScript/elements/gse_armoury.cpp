#include "gse_armoury.h"

#include "..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\system\core.h"

#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleAppearanceImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// variable
DEFINE_STATIC_NAME(self);
DEFINE_STATIC_NAME(currentTime);

//

bool Armoury::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	addGearToPersistence.load_from_xml(_node, TXT("addGearToPersistence"));
	allowDuplicates.load_from_xml(_node, TXT("allowDuplicates"));

	return result;
}

bool Armoury::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Armoury::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("gse appearance"));
				if (addGearToPersistence.is_set() &&
					addGearToPersistence.get())
				{
					if (auto* p = imo->get_gameplay_as<ModulePilgrim>())
					{
						p->add_gear_to_persistence(allowDuplicates.get(false));
					}
					else
					{
						warn_dev_investigate(TXT("invalid object for this gse armoury"));
					}
				}
			}
			else
			{
				warn_dev_investigate(TXT("missing object for gse armoury"));
			}
		}
		else
		{
			warn_dev_investigate(TXT("missing var for gse armoury"));
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
