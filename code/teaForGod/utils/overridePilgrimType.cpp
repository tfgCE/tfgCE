#include "overridePilgrimType.h"

#include "..\game\gameState.h"
#include "..\library\gameDefinition.h"

#include "..\..\framework\debug\testConfig.h"
#include "..\..\framework\library\library.h"

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(testPilgrim);

//

void TeaForGodEmperor::override_pilgrim_type(REF_ Framework::ActorType*& pilgrimType, GameState* _fromGameState)
{
	if (auto* gd = GameDefinition::get_chosen())
	{
		if (auto* at = gd->get_pilgrim_actor_override())
		{
			pilgrimType = at;
		}
	}
	if (_fromGameState &&
		_fromGameState->pilgrimActorType.get())
	{
		pilgrimType = _fromGameState->pilgrimActorType.get();
	}
#ifndef BUILD_PUBLIC_RELEASE
	{
		if (auto* testPilgrim = Framework::TestConfig::get_params().get_existing<Framework::LibraryName>(NAME(testPilgrim)))
		{
			if (auto* p = Framework::Library::get_current()->get_actor_types().find(*testPilgrim))
			{
				pilgrimType = p;
			}
		}
	}
#endif
}


