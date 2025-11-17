#include "performanceTimer.h"

#include "..\math\math.h"

#ifdef AN_WINDOWS
#include <Windows.h>
#endif

#ifdef AN_LINUX_OR_ANDROID
#include "..\timeUtils.h"
#endif

#ifdef AN_WINDOWS
static double get_freq_coef()
{
	static double freqCoef = 0.0f;
	static bool queried = false;
	if (!queried)
	{
		queried = true;
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		double freq = (double(li.QuadPart) / 1000.0);
		freqCoef = freq == 0.0f ? 1.0f : 1.0f / freq;
	}
	return freqCoef;
}
#endif

Performance::Timer::stamp stamp_zero()
{
#ifdef AN_WINDOWS
	return 0;
#else
#ifdef AN_LINUX_OR_ANDROID
	timespec ts;
	ts.tv_nsec = 0;
	ts.tv_sec = 0;
	return ts;
#else
#error implement
#endif
#endif
}

bool is_stamp_zero(Performance::Timer::stamp _stamp)
{
#ifdef AN_WINDOWS
	return _stamp == 0;
#else
#ifdef AN_LINUX_OR_ANDROID
	return _stamp.tv_nsec == 0 && _stamp.tv_sec == 0;
#else
#error implement
#endif
#endif
}

Performance::Timer::stamp get_stamp()
{
#ifdef AN_WINDOWS
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
#else
#ifdef AN_LINUX_OR_ANDROID
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts;
#else
#error implement
#endif
#endif
}

using namespace Performance;

Timer::Timer()
: timeMS(0.0f)
, startStamp(stamp_zero())
, endStamp(stamp_zero())
{}

bool Timer::has_finished() const
{
	return ! is_stamp_zero(endStamp) && !is_stamp_zero(startStamp);
}

void Timer::start()
{
	startStamp = get_stamp();
	endStamp = stamp_zero();
}

void Timer::stop()
{
	endStamp = get_stamp();

	update_time_ms();
}

void Timer::start_and_stop()
{
	startStamp = get_stamp();
	endStamp = startStamp;

	update_time_ms();
}

void Timer::set_colour(Colour const& _colour)
{
	colour_r = _colour.r;
	colour_g = _colour.g;
	colour_b = _colour.b;
	colour_a = _colour.a;
}

Colour Timer::get_colour() const
{
	return Colour(colour_r, colour_g, colour_b, colour_a);
}

void Timer::update_time_ms()
{
#ifdef AN_WINDOWS
	timeMS = float(double(endStamp - startStamp) * get_freq_coef());
#else
#ifdef AN_LINUX_OR_ANDROID
	timeMS = float(timespec_diff_to_double_ms(startStamp, endStamp));
#else
#error implement
#endif
#endif
}

Range Timer::get_relative_range(Timer const & _timer) const
{
	float length_coef = 1.0f / max(0.00001f, _timer.get_time_ms());
#ifdef AN_WINDOWS
	double freq_coef = get_freq_coef();
	return Range( clamp(float(double(startStamp - _timer.startStamp) * freq_coef) * length_coef, 0.0f, 1.0f),
				  endStamp == 0? 1.0f : clamp(float(double(endStamp - _timer.startStamp) * freq_coef) * length_coef, 0.0f, 1.0f) );
#else
#ifdef AN_LINUX_OR_ANDROID
	return Range(clamp(float(timespec_diff_to_double_ms(_timer.startStamp, startStamp)) * length_coef, 0.0f, 1.0f),
				 is_stamp_zero(endStamp) ? 1.0f : clamp(float(timespec_diff_to_double_ms(_timer.startStamp, endStamp)) * length_coef, 0.0f, 1.0f));

#else
#error TODO implement timer
#endif
#endif
}

void Timer::set_based_zero(float _lengthMS)
{
	startStamp = stamp_zero();
#ifdef AN_WINDOWS
	endStamp = stamp(double(_lengthMS) / get_freq_coef());
#else
#ifdef AN_LINUX_OR_ANDROID
	endStamp = prepare_timespec(_lengthMS);
#else
#error implement
#endif
#endif
	update_time_ms();
}

void Timer::set_from_relative(Timer const& _timer, Range const& _range, bool _finished)
{
	float timerLength = _timer.get_time_ms();
#ifdef AN_WINDOWS
	startStamp = stamp(double(_range.min * timerLength) / get_freq_coef());
	endStamp = _finished? stamp(double(_range.max * timerLength) / get_freq_coef()) : stamp_zero();
#else
#ifdef AN_LINUX_OR_ANDROID
	startStamp = prepare_timespec(_range.min * timerLength);
	endStamp = _finished ? prepare_timespec(_range.max * timerLength) : stamp_zero();
#else
#error implement
#endif
#endif
	update_time_ms();
}

void Timer::include(Timer const& _other)
{
#ifdef AN_WINDOWS
	startStamp = min(startStamp, _other.startStamp);
	endStamp = max(endStamp, _other.endStamp);
#else
#ifdef AN_LINUX_OR_ANDROID
	startStamp = min(startStamp, _other.startStamp);
	endStamp = max(endStamp, _other.endStamp);
#else
#error implement
#endif
#endif
	update_time_ms();
}
