#include "teaForGod.h"

#include <time.h>

//

using namespace TeaForGodEmperor;

//

bool demoMode =
#ifdef BUILD_DEMO
	true
#else
	false
#endif
	;


//

void TeaForGodEmperor::determine_demo()
{
	// completely disabled as it is no longer required. There are no publicly available full builds
	/*
#ifdef BUILD_PREVIEW_RELEASE
	if (!demoMode)
	{
		time_t currentTime = time(0);
		int runMoment = (int)(currentTime / (60 * 60 * 24));
		if (runMoment > BUILD_MOMENT + 90)
		{
			demoMode = true;
		}
	}
#endif
	*/
}

bool TeaForGodEmperor::is_demo()
{
	return demoMode;
}

void TeaForGodEmperor::be_demo()
{
	demoMode = true;
}
