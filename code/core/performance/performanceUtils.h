#pragma once

#include "performanceBlock.h"
#include "performanceSystem.h"
#include "performanceTimer.h"
#include "scopedMeasureAndShow.h"
#include "scopedMeasureBlock.h"
#include "scopedMeasureLongJob.h"
#include "..\system\timeStamp.h"

#ifdef AN_PERFORMANCE_MEASURE
#define USE_PERFORMANCE_ADDITIONAL_INFO
#endif

namespace Performance
{
	void finish_rendering();

	bool is_enabled();
	void enable(bool _enable);

	class Performances
	{
	public:
		static BlockTag get_tag(Name const & _tag, BlockType::Type _type = BlockType::Normal, Colour const & _colour = Colour::alpha(0.0f));
		static BlockTag get_tag(tchar const * _tag, BlockType::Type _type = BlockType::Normal, Colour const & _colour = Colour::alpha(0.0f));
		static BlockTag get_tag(String const & _contextInfo, BlockType::Type _type = BlockType::Normal, Colour const & _colour = Colour::alpha(0.0f));
	};
};

#ifdef AN_PERFORMANCE_MEASURE
	#define PERFORMANCE_MEASURE_ENABLE(_enable) Performance::enable(_enable);
	#define PERFORMANCE_MARKER(colour) \
		{ Performance::Timer t; t.start_and_stop(); t.set_colour(colour); Performance::System::add_marker(t); }
	#define MEASURE_PERFORMANCE(name, ...) \
		DEFINE_STATIC_NAME(name) \
		Performance::ScopedMeasureBlock temp_variable_named(measureBlock)(Performance::Performances::get_tag(NAME(name)), ##__VA_ARGS__);
	#define MEASURE_PERFORMANCE_STR(nameStr, ...) \
		Performance::ScopedMeasureBlock temp_variable_named(measureBlock)(Performance::Performances::get_tag(nameStr), ##__VA_ARGS__);
	#define MEASURE_PERFORMANCE_STR_COLOURED(nameStr, colour, ...) \
		Performance::ScopedMeasureBlock temp_variable_named(measureBlock)(Performance::Performances::get_tag(nameStr, Performance::BlockType::Normal, colour), ##__VA_ARGS__);
	#define MEASURE_PERFORMANCE_COLOURED(name, colour, ...) \
		DEFINE_STATIC_NAME(name) \
		Performance::ScopedMeasureBlock temp_variable_named(measureBlock)(Performance::Performances::get_tag(NAME(name), Performance::BlockType::Normal, colour), ##__VA_ARGS__);
	#define MEASURE_PERFORMANCE_LOCK(name, ...) \
		DEFINE_STATIC_NAME(name) \
		Performance::ScopedMeasureBlock temp_variable_named(measureBlock)(Performance::Performances::get_tag(NAME(name), Performance::BlockType::Lock), ##__VA_ARGS__);
	#define MEASURE_PERFORMANCE_CONTEXT(string, ...) \
		Performance::ScopedMeasureBlock temp_variable_named(measureBlock)(Performance::Performances::get_tag(string), ##__VA_ARGS__);
	#define MEASURE_AND_OUTPUT_PERFORMANCE(name, ...) Performance::ScopedMeasureAndShow temp_variable_named(measureBlock)(name, ##__VA_ARGS__);
	#define MEASURE_AND_OUTPUT_PERFORMANCE_MIN(minTime, name, ...) Performance::ScopedMeasureAndShow temp_variable_named(measureBlock)(minTime, name, ##__VA_ARGS__);
	#define MEASURE_PERFORMANCE_FINISH_RENDERING() Performance::finish_rendering()
	#define MEASURE_PERFORMANCE_LONG_JOB(nameChar, ...) \
		Performance::ScopedMeasureLongJob temp_variable_named(measureBlock)(nameChar);
#ifdef AN_PERFORMANCE_MEASURE_GUARD_LIMITS
	#define PERFORMANCE_GUARD_LIMIT(_limit, _info) ::System::ScopedTimeStampLimitGuard temp_variable_named(timeGuard) (_limit, _info);
	#define PERFORMANCE_GUARD_LIMIT_2(_limit, _info, _secondInfo) ::System::ScopedTimeStampLimitGuard temp_variable_named(timeGuard) (_limit, _info, _secondInfo);
	#define PERFORMANCE_GUARD_LIMIT_BREAK(_limit, _info) ::System::ScopedTimeStampLimitGuard temp_variable_named(timeGuard) (_limit, _info, true);
	#define PERFORMANCE_GUARD_LIMIT_BREAK_OPT(_limit, _info, _breakOnLimit) ::System::ScopedTimeStampLimitGuard temp_variable_named(timeGuard) (_limit, _info, _breakOnLimit);
#else
	#define PERFORMANCE_GUARD_LIMIT(_limit, _info)
	#define PERFORMANCE_GUARD_LIMIT_2(_limit, _info, _secondInfo)
	#define PERFORMANCE_GUARD_LIMIT_BREAK(_limit, _info)
	#define PERFORMANCE_GUARD_LIMIT_BREAK_OPT(_limit, _info, _breakOnLimit)
#endif
#else
	#define PERFORMANCE_MEASURE_ENABLE(enable)
	#define PERFORMANCE_MARKER(colour)
	#define MEASURE_PERFORMANCE(name, ...)
	#define MEASURE_PERFORMANCE_STR(name, ...)
	#define MEASURE_PERFORMANCE_STR_COLOURED(name, ...)
	#define MEASURE_PERFORMANCE_COLOURED(name, colour, ...)
	#define MEASURE_PERFORMANCE_LOCK(name, ...);
	#define MEASURE_PERFORMANCE_CONTEXT(string, ...);
	#define MEASURE_AND_OUTPUT_PERFORMANCE(name, ...)
	#define MEASURE_AND_OUTPUT_PERFORMANCE_MIN(minTime, name, ...)
	#define MEASURE_PERFORMANCE_FINISH_RENDERING()
	#define MEASURE_PERFORMANCE_LONG_JOB(nameChar, ...)
	#define PERFORMANCE_GUARD_LIMIT(_limit, _info)
	#define PERFORMANCE_GUARD_LIMIT_2(_limit, _info, _secondInfo)
	#define PERFORMANCE_GUARD_LIMIT_BREAK(_limit, _info)
	#define PERFORMANCE_GUARD_LIMIT_BREAK_OPT(_limit, _info, _breakOnLimit)
#endif
#ifdef USE_PERFORMANCE_ADDITIONAL_INFO
	#define ADDITIONAL_PERFORMANCE_INFO(id, info) Performance::System::set_additional_info(id, info);
#else
	#define ADDITIONAL_PERFORMANCE_INFO(id, info)
#endif

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	struct ScopedPerformanceLimitGuard
	{
		::System::TimeStamp performanceGuard;
		float limit;
		tchar const * info = nullptr;
		ScopedPerformanceLimitGuard(float _limit, tchar const * _info)
		: limit(_limit)
		, info(_info)
		{}
		~ScopedPerformanceLimitGuard()
		{
			float timeSince = performanceGuard.get_time_since();
			if (timeSince > limit)
			{
				warn(TXT("[performance guard] exceeded limit of %.2fms (%.2fms) : %S"), limit * 1000.0f, timeSince * 1000.0f, info? info : TXT(""));
			}
		}
	};

	#define PERFORMANCE_LIMIT_GUARD_START() ::System::TimeStamp __performanceGuard;
	#define PERFORMANCE_LIMIT_GUARD_STOP(_limit, ...) \
		{ \
			float timeSince = __performanceGuard.get_time_since(); \
			if (timeSince > _limit) \
			{ \
				String info; \
				info = String::printf(__VA_ARGS__); \
				warn(TXT("[performance guard] exceeded limit of %.2fms (%.2fms) : %S"), _limit * 1000.0f, timeSince * 1000.0f, info.to_char()); \
			} \
		}
	#define SCOPED_PERFORMANCE_LIMIT_GUARD(_limit, _getInfo) ScopedPerformanceLimitGuard __scopedPerformanceGuard(_limit, _getInfo);
#else
	#define PERFORMANCE_LIMIT_GUARD_START()
	#define PERFORMANCE_LIMIT_GUARD_STOP(_limit, ...)
	#define SCOPED_PERFORMANCE_LIMIT_GUARD(_limit, _getInfo)
#endif