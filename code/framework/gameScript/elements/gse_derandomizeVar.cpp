#include "gse_derandomizeVar.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

#include "..\..\modulesOwner\modulesOwner.h"

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

bool DerandomizeVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

ScriptExecutionResult::Type DerandomizeVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (!fromVar.is_empty() && toVar.is_valid())
	{
		if (auto* vi = _execution.get_variables().find_any_existing(fromVar[Random::get_int(fromVar.get_size())]))
		{
			if (vi->type_id() == type_id<Range3>())
			{
				auto& r = vi->get < Range3>();
				Random::Generator rg;
				Vector3* dest = plain_cast<Vector3>(_execution.access_variables().access_raw(toVar, type_id<Vector3>()));
				dest->x = rg.get(r.x);
				dest->y = rg.get(r.y);
				dest->z = rg.get(r.z);
			}
			else if (vi->type_id() == type_id<Range>())
			{
				auto& r = vi->get < Range>();
				Random::Generator rg;
				float* dest = plain_cast<float>(_execution.access_variables().access_raw(toVar, type_id<float>()));
				*dest = rg.get(r);
			}
			else
			{
				error(TXT("not implemented"));
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
