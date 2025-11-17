#include "steamworksImpl.h"

//

// steamworks API uses some deprecated functions
#define _CRT_SECURE_NO_WARNINGS

#ifdef AN_STEAM
#include <steam_api.h>
#endif

//

using namespace GameEnvironment;

//

bool SteamworksImpl::init()
{
#ifdef AN_STEAM
	return SteamAPI_Init();
#else
	return false;
#endif
}

void SteamworksImpl::shutdown()
{
#ifdef AN_STEAM
	SteamAPI_Shutdown();
#endif
}

void SteamworksImpl::set_achievement(char const* _achievementName)
{
#ifdef AN_STEAM
	auto* stats = SteamUserStats();
	bool achieved = false;
	stats->GetAchievement(_achievementName, &achieved);
	if (!achieved)
	{
		stats->SetAchievement(_achievementName);
		// and immediately store them!
		stats->StoreStats();
	}
#endif
}

char const* SteamworksImpl::get_language()
{
#ifdef AN_STEAM
	if (auto* sa = SteamApps())
	{
		return sa->GetCurrentGameLanguage();
	}
#endif

	return nullptr;
}