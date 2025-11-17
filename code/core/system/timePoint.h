#pragma once

#include "..\globalDefinitions.h"

#ifdef AN_LINUX_OR_ANDROID
#include <time.h>
#endif

namespace System
{
#ifdef AN_SDL
	typedef uint64 TimePoint;
#else
#ifdef AN_LINUX_OR_ANDROID
	typedef timespec TimePoint;
#else
#error implement
#endif
#endif
	TimePoint zero_time_point();
	bool is_time_point_zero(TimePoint const & _tp);
};

