#pragma once

#include "..\..\core\globalDefinitions.h"

namespace Framework
{
	class ActorType;
};

namespace TeaForGodEmperor
{
	struct GameState;

	void override_pilgrim_type(REF_ Framework::ActorType*& pilgrimType, GameState* _fromGameState);
};
