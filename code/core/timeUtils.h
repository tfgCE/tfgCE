#pragma once

#ifdef AN_LINUX_OR_ANDROID

#include "math\math.h"

template <>
inline timespec min<timespec>(timespec const _a, timespec const _b)
{
	if (_a.tv_sec < _b.tv_sec)
	{
		return _a;
	}
	if (_a.tv_sec > _b.tv_sec)
	{
		return _b;
	}
	if (_a.tv_nsec < _b.tv_sec)
	{
		return _a;
	}
	if (_a.tv_nsec > _b.tv_nsec)
	{
		return _b;
	}
	return _a;
}

template <>
inline timespec max<timespec>(timespec const _a, timespec const _b)
{
	if (_a.tv_sec > _b.tv_sec)
	{
		return _a;
	}
	if (_a.tv_sec < _b.tv_sec)
	{
		return _b;
	}
	if (_a.tv_nsec > _b.tv_sec)
	{
		return _a;
	}
	if (_a.tv_nsec < _b.tv_nsec)
	{
		return _b;
	}
	return _a;
}

inline timespec prepare_timespec(float _seconds)
{
	timespec ts;
	ts.tv_sec = (int)(floor(_seconds) + 0.1f);
	ts.tv_nsec = (_seconds - (float)ts.tv_sec) * 1000000000.0f;
	if (ts.tv_sec == 0 && ts.tv_nsec == 0)
	{
		ts.tv_nsec = 1;
	}
	return ts;
}

inline timespec prepare_timespec_nano_clamped(int _nanoSeconds)
{
	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = clamp<int>(_nanoSeconds, 0, 999999999);
	if (ts.tv_sec == 0 && ts.tv_nsec == 0)
	{
		ts.tv_nsec = 1;
	}
	return ts;
}

inline double timespec_to_double_ms(timespec const & ts)
{
	return double(ts.tv_nsec) * 0.000001 +
		   double(ts.tv_sec) * 1000.0;
}

// in miliseconds
inline double timespec_diff_to_double_ms(timespec const & start, timespec const& end)
{
	return double(end.tv_nsec - start.tv_nsec) * 0.000001 +
		   double(end.tv_sec - start.tv_sec) * 1000.0;
}

// in seconds
inline double timespec_diff_to_double(timespec const& start, timespec const& end)
{
	return double(end.tv_nsec - start.tv_nsec) * 0.000000001 +
		double(end.tv_sec - start.tv_sec);
}

#endif
