#include "gse_getCellCoord.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\world\room.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool GetCellCoord::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);
	roomVar = _node->get_name_attribute(TXT("roomVar"), roomVar);

	if (!objectVar.is_valid() && !roomVar.is_valid())
	{
		objectVar = NAME(self);
	}

	storeAsVar = _node->get_name_attribute(TXT("storeAsVar"), storeAsVar);

	error_loading_xml_on_assert(storeAsVar.is_valid(), result, _node, TXT("no \"storeAsVar\" provided"));

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type GetCellCoord::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		Optional<VectorInt2> cellCoord;
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				cellCoord = piow->find_cell_at(imo);
			}
		}
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
		{
			if (auto* room = exPtr->get())
			{
				cellCoord = piow->find_cell_at(room);
			}
		}
		if (cellCoord.is_set())
		{
			if (storeAsVar.is_valid())
			{
				_execution.access_variables().access<VectorInt2>(storeAsVar) = cellCoord.get();
			}
		}
		else
		{
			error(TXT("could not find cell coord"));
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
