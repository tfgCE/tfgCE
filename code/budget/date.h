#pragma once

#include "..\core\globalDefinitions.h"

struct String;

namespace IO
{
	class File;
};

namespace Budget
{
	struct Date
	{
	public:
		int year;
		int month;
		int day;

		int hour = 0;
		int minute = 0;
		int second = 0;

	private:
		mutable int64 dateSec = 0;
		mutable int ds_year;
		mutable int ds_month;
		mutable int ds_day;
		mutable int ds_hour = 0;
		mutable int ds_minute = 0;
		mutable int ds_second = 0;

	public:
		Date();

		bool load_from_gdocs_csv(IO::File* file);
		bool load_from(IO::File* file);
		bool load_from_auto_hh_mm_ss(IO::File* file);
		bool load_from_dd_mm_yyyy_hh_mm_ss(IO::File* file);
		bool load_from_yyyy_mm_dd_hh_mm_ss(IO::File* file);
		bool load_from_month_dd_yyyy(IO::File* file);

		bool operator==(Date const & _other) const { return year == _other.year &&
															month == _other.month &&
															day == _other.day &&
															hour == _other.hour &&
															minute == _other.minute &&
															second == _other.second; }
		bool operator<(Date const & _other) const { return year < _other.year ||
														  (year == _other.year && 
														   (month < _other.month ||
														    (month == _other.month &&
															 (day < _other.day ||
														      (day == _other.day &&
															   (hour < _other.hour ||
														        (hour == _other.hour &&
															     (minute < _other.minute ||
														          (minute == _other.minute &&
															       (second < _other.second)))))))))); }

		bool operator>(Date const & _other) const { return year > _other.year ||
														  (year == _other.year && 
														   (month > _other.month ||
														    (month == _other.month &&
															 (day > _other.day ||
														      (day == _other.day &&
															   (hour > _other.hour ||
														        (hour == _other.hour &&
															     (minute > _other.minute ||
														          (minute == _other.minute &&
															       (second > _other.second)))))))))); }

		bool operator<=(Date const & _other) const { return *this < _other || *this == _other; }
		bool operator>=(Date const & _other) const { return *this > _other || *this == _other; }

		static int distance_between(Date const & a, Date const & b);
		static int difference_between(Date const & a, Date const & b); // if a is earlier than b it is negative
		static bool almost(Date const & a, Date const & b);

		void parse_yyyy_mm_dd_minus(String const & _txt);
		void parse_yyyy_mm_dd_minus_hh_mm_ss(String const & _txt);
		void parse_dd_mm_yyyy_dot(String const & _txt);
		void parse_hh_mm_ss(String const & _txt);

		String to_string_date() const;
		String to_string_dd_mm_yyyy_hh_mm_ss() const;
		String to_string_universal() const;

		int64 to_seconds() const;

		void advance_day();
		void advance_hour();
		void advance_second();
	};
};
