#pragma once

#include "..\..\core\functionParamsStruct.h"

#include "..\..\framework\ai\aiMind.h"
#include "..\..\framework\game\delayedObjectCreation.h"

//

namespace Framework
{
	class Region;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace ManagerUtils
		{
			struct SpawnAiManagerParams
			{
				ADD_FUNCTION_PARAM_PLAIN_INITIAL(SpawnAiManagerParams, Framework::AI::Mind*, mind, with_mind, nullptr);
				ADD_FUNCTION_PARAM_PLAIN_INITIAL(SpawnAiManagerParams, Framework::IModulesOwner*, imoOwner, for_owner, nullptr);
				ADD_FUNCTION_PARAM_PLAIN_INITIAL(SpawnAiManagerParams, Framework::Room*, room, in_room, nullptr);
				ADD_FUNCTION_PARAM_PLAIN_INITIAL(SpawnAiManagerParams, Framework::Region*, region, for_region, nullptr);
			};
			bool queue_spawn_ai_manager(REF_ RefCountObjectPtr<Framework::DelayedObjectCreation>& _doc, SpawnAiManagerParams const & _params);
		};
	};
};
