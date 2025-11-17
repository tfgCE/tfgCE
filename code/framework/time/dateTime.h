#pragma once

#include "dateTimeFormat.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\types\string.h"
#include "..\..\core\math\math.h"
#include "..\text\stringFormatter.h"

namespace Framework
{
	struct LibraryLoadingContext;
	struct Time;

	struct DateTime
	: public ::Framework::ICustomStringFormatter
	{
		FAST_CAST_DECLARE(DateTime);
		FAST_CAST_BASE(::Framework::ICustomStringFormatter);
		FAST_CAST_END();

	public: // formatting functions
		static String get_string_short_time(float _time, int _secondSubnumbers = 2);

	public:
		static DateTime const & none() { return s_none; }

		DateTime();
		explicit DateTime(int _year, int _month, int _day);

		static DateTime get_current_from_system();
		static DateTime create_none();

		void set_date(int _year, int _month = 1, int _day = 1);
		void set_time(int _hour, int _minute = 0, int _second = 0);
		void set_time(Time const & _time);
		void be_start_of_day() { set_time(0, 0, 0); }
		void be_end_of_day() { set_time(23, 59, 59); }

		void be_none() { isNone = true; }
		bool is_valid() const { return ! isNone; }

		int get_year() const { return year; }
		int get_month() const { return month; }
		int get_day() const { return day; }
		int get_hour() const { return hour; }
		int get_minute() const { return minute; }
		int get_second() const { return second; }

		float to_hours_24() const { an_assert(is_valid()); return isNone ? 0.0f : (float)to_seconds_24() / 3600.0f; }
		int to_seconds_24() const { an_assert(is_valid()); return isNone ? 0 : second + (60 * (minute + 60 * hour)); }

		int get_days_since_base() const; // 1st january of baseyear is 0
		int get_weeks_since_base() const { return mod(get_days_since_base(), 7) / 7; }

		int get_week_day() const; // 0 monday, 6 sunday
		DateTime get_start_of_the_week() const; // keeps time

		Time get_time() const;

		void advance_ms(float _ms);
		void advance_second(int _seconds);
		void advance_day(int _day);

		DateTime get_in_following_24h(Time const & _timeOfDay) const; // in 24h from now
		DateTime get_in_following_24h(Time const & _timeOfDay, Time const & _canBeLate) const; // can be late tells that it is in -canBeLate to 24-canBeLate

		String get_string_system_date() const;
		String get_string_system_date_time() const;

		// global, depending on current settings?
		String get_string_full_date(DateTimeFormat _dateTimeFormat) const;
		String get_string_full_date_time(DateTimeFormat _dateTimeFormat, bool _withSeconds = false) const;
		static int get_string_wide_compact_date_time_length(DateTimeFormat _dateTimeFormat, bool _withSeconds = false);
		String get_string_wide_compact_date_time(DateTimeFormat _dateTimeFormat, bool _withSeconds = false) const;
		static int get_string_compact_date_time_length(DateTimeFormat _dateTimeFormat, bool _withSeconds = false);
		String get_string_compact_date_time(DateTimeFormat _dateTimeFormat, bool _withSeconds = false) const;
		static int get_string_wide_compact_date_length(DateTimeFormat _dateTimeFormat);
		String get_string_wide_compact_date(DateTimeFormat _dateTimeFormat) const;
		static int get_string_compact_date_length(DateTimeFormat _dateTimeFormat);
		String get_string_compact_date(DateTimeFormat _dateTimeFormat) const;
		static int get_string_compact_time_length(DateTimeFormat _dateTimeFormat, bool _withSeconds = false);
		String get_string_compact_time(DateTimeFormat _dateTimeFormat, bool _withSeconds = false) const;
		String get_string_hour_minute(DateTimeFormat _dateTimeFormat, bool _withSeconds = false) const;

		static String format_string_week_day(int _weekDay, tchar const * _format, Framework::StringFormatterParams & _params);
		static String get_string_week_day_short(int _weekDay);
		static String get_string_week_day_full(int _weekDay);

		String format_string_week_day(tchar const * _format, Framework::StringFormatterParams & _params) const { return format_string_week_day(get_week_day(), _format, _params); }
		String get_string_week_day_short() const { return get_string_week_day_short(get_week_day()); }
		String get_string_week_day_full() const { return get_string_week_day_full(get_week_day()); }

		static String format_string_month(int _month, tchar const * _format, Framework::StringFormatterParams & _params);
		static String get_string_month_short(int _month);
		static String get_string_month_full(int _month);

		String format_string_month(tchar const * _format, Framework::StringFormatterParams & _params) const { return format_string_month(get_month(), _format, _params); }
		String get_string_month_short() const { return get_string_month_short(get_month()); }
		String get_string_month_full() const { return get_string_month_full(get_month()); }
		String get_string_days_to(DateTime const & _dateInFuture) const; // returns today, 1 day, 2 days etc
		String get_string_single_unit_to(DateTime const & _dateInFuture) const; // returns 1 day, 2 days etc or 1 hour, 2 hours etc or 1 minute, 2 minutes etc

		static int calculate_days_of_month(int _year, int _month);
		static int calculate_days_of_year(int _year);

		bool operator < (DateTime const & _a) const;
		bool operator <= (DateTime const & _a) const;
		bool operator > (DateTime const & _a) const;
		bool operator >= (DateTime const & _a) const;
		bool operator == (DateTime const & _a) const;
		bool operator != (DateTime const & _a) const { return !operator==(_a); }

		static bool is_same_day(DateTime const & _a, DateTime const & _b);
		bool is_same_day(DateTime const & _a) const { return is_same_day(*this, _a); }
		bool is_earlier_day_than(DateTime const & _a) const;

		Time operator - (DateTime const & _a) const;

		DateTime operator + (Time const & _a) const;
		DateTime operator - (Time const & _a) const;
		DateTime & operator += (Time const & _a) { *this = *this + _a; return *this; }
		DateTime & operator -= (Time const & _a) { *this = *this - _a; return *this; }

		bool load_from_xml(IO::XML::Node const * _node);
		bool save_to_xml(IO::XML::Node * _node) const;

	public:
		bool serialise(Serialiser & _serialiser);

	public: // ::Framework::ICustomStringFormatter
		implement_ ICustomStringFormatter const * get_hard_copy() const { return new CustomStringFormatterStoredValue<DateTime>(*this); }
		implement_ String format_custom(tchar const * _format, Framework::StringFormatterParams & _params = Framework::StringFormatterParams().temp_ref()) const; //~full~full_date~full_date_time~compact~compact_date~compact_date_time

	private:
		static const DateTime s_none;
		static const int baseYear = 1989;
		static const int baseYearsFirstDayDayOfWeek = 7;
		static const DateTime baseYearsFirstDay;

		bool isNone = false;

		int year = 1989;
		int month = 1;
		int day = 1;

		int hour = 0;
		int minute = 0;
		int second = 0;
		float ms = 0.0f; // not always used

		int to_days_within_year() const; // starts with 0
	};

};

DECLARE_REGISTERED_TYPE(Framework::DateTime);
