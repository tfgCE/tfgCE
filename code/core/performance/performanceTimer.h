#pragma once

#include "..\globalDefinitions.h"
#include "..\globalSettings.h"
#include "..\globalInclude.h"

struct Colour;
struct Range;

namespace Performance
{

	class Timer
	{
	public:
#ifdef AN_WINDOWS
		typedef int64 stamp;
#endif

#ifdef AN_LINUX_OR_ANDROID
		typedef timespec stamp;
#endif

	public:
		Timer();

		void start();
		void stop();
		void start_and_stop();

		bool has_finished() const;
		float get_time_ms() const { return timeMS; }
		Range get_relative_range(Timer const & _timer) const;

		void set_based_zero(float _lengthMS);
		void set_from_relative(Timer const& _timer, Range const& _range, bool _finished = true);

		void include(Timer const& _other);

		void set_colour(Colour const& _colour);
		Colour get_colour() const;

	private:
		float timeMS;

		stamp startStamp;
		stamp endStamp;

		// colour
		float colour_r = 1.0f;
		float colour_g = 1.0f;
		float colour_b = 1.0f;
		float colour_a = 1.0f;

		void update_time_ms();
	};

};