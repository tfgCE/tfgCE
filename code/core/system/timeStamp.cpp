#include "timeStamp.h"

#ifdef AN_SDL
#include "SDL.h"
#endif
#ifdef AN_LINUX_OR_ANDROID
#include "core.h"
#include <unistd.h>
#include "..\timeUtils.h"
#endif

#include "..\debug\debug.h"
#include "..\math\mathUtils.h"
#include "..\memory\memory.h"

using namespace System;

TimeStamp::TimeStamp()
{
	reset();
}

void TimeStamp::reset(float _offset)
{
	offset = _offset;
#ifdef AN_SDL
	timePoint = SDL_GetPerformanceCounter();
#else
#ifdef AN_LINUX_OR_ANDROID
	clock_gettime(CLOCK_MONOTONIC, &timePoint);
#else
#error implement
#endif
#endif
}

float TimeStamp::get_time_since() const
{
#ifdef AN_SDL
	uint64 currentTimePoint = SDL_GetPerformanceCounter();
	uint64 tickFrequency = SDL_GetPerformanceFrequency();
	return max(0.0f, (float)(currentTimePoint - timePoint) / (float)tickFrequency - offset);
#else
#ifdef AN_LINUX_OR_ANDROID
	TimePoint currentTimePoint;
	clock_gettime(CLOCK_MONOTONIC, &currentTimePoint);
	return max(0.0f, (float)timespec_diff_to_double(timePoint, currentTimePoint) - offset);
#else
	#error implement for non sdl
#endif
#endif
}

float TimeStamp::get_time_to_zero() const
{
#ifdef AN_SDL
	uint64 currentTimePoint = SDL_GetPerformanceCounter();
	uint64 tickFrequency = SDL_GetPerformanceFrequency();
	return max(0.0f, -((float)(currentTimePoint - timePoint) / (float)tickFrequency - offset));
#else
#ifdef AN_LINUX_OR_ANDROID
	TimePoint currentTimePoint;
	clock_gettime(CLOCK_MONOTONIC, &currentTimePoint);
	return max(0.0f, -((float)timespec_diff_to_double(timePoint, currentTimePoint) - offset));
#else
#error implement for non sdl
#endif
#endif
}

//

ScopedTimeStampLimitGuard::ScopedTimeStampLimitGuard(float _limit, tchar const * _info, bool _breakOnLimit)
: limit(_limit)
, breakOnLimit(_breakOnLimit)
{
	int len = min<int>(get_MAX_LENGTH(), (int)tstrlen(_info));
	memory_copy(info, _info, sizeof(tchar) * (len + 1));
	info[len] = 0;
	secondInfo[0] = 0;
}

ScopedTimeStampLimitGuard::ScopedTimeStampLimitGuard(float _limit, tchar const* _info, tchar const* _secondInfo, bool _breakOnLimit)
: limit(_limit)
, breakOnLimit(_breakOnLimit)
{
	{
		int len = min<int>(get_MAX_LENGTH(), (int)tstrlen(_info));
		memory_copy(info, _info, sizeof(tchar) * (len + 1));
		info[len] = 0;
	}
	{
		int len = min<int>(get_MAX_LENGTH(), (int)tstrlen(_secondInfo));
		memory_copy(secondInfo, _secondInfo, sizeof(tchar) * (len + 1));
		secondInfo[len] = 0;
	}
}

ScopedTimeStampLimitGuard::~ScopedTimeStampLimitGuard()
{
	if (ts.get_time_since() > limit)
	{
#ifndef AN_DEBUG
#ifndef AN_DEVELOPMENT
		// in debug and development everything will be slow
		warn(TXT("SLOW %.3f > %.3f on %S %S"), ts.get_time_since(), limit, info, secondInfo);
#endif
#endif
		if (breakOnLimit)
		{
			AN_BREAK;
		}
	}
}

//

ScopedTimeStampOutput::ScopedTimeStampOutput(tchar const * _info)
{
	int len = min<int>(get_MAX_LENGTH(), (int)tstrlen(_info));
	memory_copy(info, _info, sizeof(tchar) * (len + 1));
	info[len] = 0;
}

ScopedTimeStampOutput::~ScopedTimeStampOutput()
{
	float timeTaken = ts.get_time_since();
	output(TXT("%12.3fms : %S"), timeTaken * 1000.0f, info);
}

