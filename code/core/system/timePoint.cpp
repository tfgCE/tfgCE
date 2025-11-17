#include "timePoint.h"

System::TimePoint System::zero_time_point()
{
#ifdef AN_SDL
	return 0;
#else
#ifdef AN_LINUX_OR_ANDROID
	System::TimePoint tp;
	tp.tv_nsec = 0;
	tp.tv_sec = 0;
	return tp;
#else
#error implement
#endif
#endif
}

bool System::is_time_point_zero(TimePoint const & _tp)
{
#ifdef AN_SDL
	return _tp == 0;
#else
#ifdef AN_LINUX_OR_ANDROID
	return _tp.tv_nsec == 0 && _tp.tv_sec == 0;
#else
#error implement
#endif
#endif
}

