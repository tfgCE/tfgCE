#pragma once

namespace GameEnvironment
{
	class SteamworksImpl
	{
	public:
		static bool init();
		static void shutdown();

		static void set_achievement(char const * _achievementName);
		static char const * get_language();
	};
};
