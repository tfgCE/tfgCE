#pragma once

#include "..\..\core\fastCast.h"

class SimpleVariableStorage;

namespace TeaForGodEmperor
{
	interface_class IGameStateSensitive
	{
		FAST_CAST_DECLARE(IGameStateSensitive);
		FAST_CAST_END();

	public:
		virtual ~IGameStateSensitive() {}

		virtual void store_for_game_state(SimpleVariableStorage& _variables) const = 0;
		virtual void restore_from_game_state(SimpleVariableStorage const& _variables) = 0;
	};
};
