#pragma once

#include "..\..\..\..\core\containers\array.h"

namespace TeaForGodEmperor
{
	struct PlayerStats;
};

namespace Loader
{
	struct HubScreenOption;

	namespace Utils
	{
		enum PlayerStatsFlags
		{
			Global = 1,
			Detailed = 2,
		};
		void add_options_from_player_stats(TeaForGodEmperor::PlayerStats const& _stats, Array<HubScreenOption>& _options, int _flags = 0);
	};
};
