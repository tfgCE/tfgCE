#include "aiManagerLogic.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\region.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define LOG_MORE
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

REGISTER_FOR_FAST_CAST(AIManager);

AIManager::AIManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction))
: base(_mind, _logicData, _executeFunction)
{
}

AIManager::~AIManager()
{
	unregister_ai_manager();
}

void AIManager::register_ai_manager_in(Framework::Room* r)
{
	an_assert(!registeredInRegion.get());
	register_ai_manager_in(r->get_in_region());
}

void AIManager::register_ai_manager_in(Framework::Region* r)
{
	an_assert(!registeredInRegion.get());

	if (r)
	{
		r->register_ai_manager(get_imo());
		registeredInRegion = r;
	}
}

void AIManager::register_ai_manager_in_top_region(Framework::Region* r)
{
	an_assert(!registeredInRegion.get());
	if (r)
	{
		while (auto* up = r->get_in_region())
		{
			r = up;
		}
		register_ai_manager_in(r);
	}
}

void AIManager::unregister_ai_manager()
{
	if (auto* r = registeredInRegion.get())
	{
		if (auto* imo = get_imo())
		{
			r->unregister_ai_manager(get_imo());
		}
	}
}
