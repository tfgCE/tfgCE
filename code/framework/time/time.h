#pragma once

#include "..\..\core\fastCast.h"
#include "..\..\core\types\string.h"
#include "..\..\core\math\math.h"
#include "..\text\stringFormatter.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;

	struct Time
	: public ::Framework::ICustomStringFormatter
	{
		FAST_CAST_DECLARE(Time);
		FAST_CAST_BASE(::Framework::ICustomStringFormatter);
		FAST_CAST_END();

	public:
		static Time const & none() { return s_none; }
		static Time const & zero() { return s_zero; }
		static Time const & one_day() { return s_day; }
		static Time create_none() { Time none; none.isNone = true; return none; }
		static Time half_of(Time const & _value) { return from_seconds(_value.to_seconds() / 2); }
		static bool is_zero(Time const & _value) { return _value.is_zero(); }
		static Time get_random(Random::Generator & _generator, Time const & _min, Time const & _max);
		static Time parse_from(String const & _string, Time const & _defValue) { Time result = _defValue; result.parse_from(_string); return result; }

		Time() : isNone(true) {}
		explicit Time(String const & _text) { parse_from(_text); }
		explicit Time(int _hour, int _minute = 0, int _second = 0) : hour(_hour), minute(_minute), second(_second) {}
		static Time from_seconds(int _time);
		static Time from_seconds(float _time);
		static Time from_minutes(int _time) { return from_seconds(_time * 60); }
		static Time from_minutes(float _time) { return from_seconds(_time * 60.0f); }
		static Time from_hours(int _time) { return from_minutes(_time * 60); }
		static Time from_hours(float _time) { return from_minutes(_time * 60.0f); }
		static Time from_days(int _days);
		static Time from_days(float _days) { return from_hours(_days * 24.0f); }

		float to_hours() const { an_assert(is_ok()); return isNone ? 0.0f : to_seconds() / 3600.0f; }
		float to_minutes() const { an_assert(is_ok()); return isNone ? 0.0f : to_seconds() / 60.0f; }
		float to_seconds() const { an_assert(is_ok()); return isNone ? 0 : (float)(second + (60 * (minute + 60 * hour))) + ms; }
		int to_seconds_int() const { an_assert(is_ok()); return isNone ? 0 : second + (60 * (minute + 60 * hour)); }
		String get_compact_hour_minute_string() const { return String::printf(TXT("%S%2i:%02i"), is_negative() ? TXT("-") : TXT(""), abs(hour), abs(minute)); }
		String get_full_hour_minute_string() const { return String::printf(TXT("%S%02i:%02i"), is_negative() ? TXT("-") : TXT(""), abs(hour), abs(minute)); }
		String get_compact_hour_minute_second_string() const { return String::printf(TXT("%S%2i:%02i:%02i"), is_negative() ? TXT("-") : TXT(""), abs(hour), abs(minute), abs(second)); }
		String get_full_hour_minute_second_string() const { return String::printf(TXT("%S%02i:%02i:%02i"), is_negative() ? TXT("-") : TXT(""), abs(hour), abs(minute), abs(second)); }
		String get_simple_compact_hour_minute_second_string() const; // hh:mm:ss or mm:ss or ss
		String get_descriptive_day_hour_minute_second_string() const;
		String get_string_short_time(int _secondSubnumbers = 0) const;
		String get_for_parse() const;

		bool parse_from(String const & _text);
		bool parse_time_of_day_from(String const & _text);

		void set_time(int _hour, int _minute = 0, int _second = 0);

		bool is_valid() const { return ! isNone; }
		bool is_ok() const { return ! is_valid() || (minute >= -60 && minute < 60 && second >= -60 && second < 60); }
		bool is_zero() const { return hour == 0 && minute == 0 && second == 0; }

		bool is_positive() const { return hour > 0 || minute > 0 || second > 0; }
		bool is_negative() const { return hour < 0 || minute < 0 || second < 0; }

		int get_days() const { return mod_raw(hour, 24); }
		int get_hour() const { return hour; }
		int get_minute() const { return minute; }
		int get_second() const { return second; }
		float get_ms() const { return ms; }

		void advance_ms(float _ms);
		void advance_second(int _seconds);

		Time operator - () const { Time result = Time(-hour, -minute, -second); return result; }

		bool operator < (Time const & _a) const { return to_seconds() < _a.to_seconds(); }
		bool operator <= (Time const & _a) const { return to_seconds() <= _a.to_seconds(); }
		bool operator > (Time const & _a) const { return to_seconds() > _a.to_seconds(); }
		bool operator >= (Time const & _a) const { return to_seconds() >= _a.to_seconds(); }
		bool operator == (Time const & _a) const { return to_seconds() == _a.to_seconds(); }
		bool operator != (Time const & _a) const { return to_seconds() != _a.to_seconds(); }

		Time operator * (float _a) const { return from_seconds((int)((float)to_seconds_int() * _a)); }
		Time & operator *= (float _a) { *this = from_seconds((int)((float)to_seconds_int() * _a)); return *this; }

		Time operator + (Time const & _a) const { return from_seconds(to_seconds_int() + _a.to_seconds_int()); }
		Time operator - (Time const & _a) const { return from_seconds(to_seconds_int() - _a.to_seconds_int()); }
		Time & operator += (Time const & _a) { *this = from_seconds(to_seconds_int() + _a.to_seconds_int()); return *this; }
		Time & operator -= (Time const & _a) { *this = from_seconds(to_seconds_int() - _a.to_seconds_int()); return *this; }

		bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const * _node);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrName);
		bool load_from_xml(IO::XML::Attribute const * _attr);

	public:
		bool serialise(Serialiser & _serialiser);

	public: // ::Framework::ICustomStringFormatter
		implement_ ICustomStringFormatter const * get_hard_copy() const { return new CustomStringFormatterStoredValue<Time>(*this); }
		implement_ String format_custom(tchar const * _format, Framework::StringFormatterParams & _params = Framework::StringFormatterParams().temp_ref()) const; //~compact~compact_hm~compact_hms~full~full_hm~full_hms~descriptive_dhm

	private:
		static const Time s_none;
		static const Time s_zero;
		static const Time s_day;

		bool isNone = false;

		int hour = 0;
		int minute = 0;
		int second = 0;
		float ms = 0.0f; // not always used

		inline void make_valid();
	};

};

template <>
inline Framework::Time mod_raw<Framework::Time>(Framework::Time const _a, Framework::Time const _b)
{
	return Framework::Time::from_seconds(mod_raw(_a.to_seconds_int(), _b.to_seconds_int()));
}

template <>
inline Framework::Time mod<Framework::Time>(Framework::Time const _a, Framework::Time const _b)
{
	return Framework::Time::from_seconds(mod(_a.to_seconds_int(), _b.to_seconds_int()));
}
