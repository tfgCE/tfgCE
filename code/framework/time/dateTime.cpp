#include "dateTime.h"

#include "time.h"

#include "..\text\localisedString.h"
#include "..\game\gameConfig.h"

#include <time.h>

using namespace Framework;

//

DEFINE_STATIC_NAME(day);
DEFINE_STATIC_NAME(month);
DEFINE_STATIC_NAME(year);

DEFINE_STATIC_NAME_STR(today, TXT("time; today"));

DEFINE_STATIC_NAME(days);
DEFINE_STATIC_NAME(hours);
DEFINE_STATIC_NAME(minutes);

DEFINE_STATIC_NAME_STR(timeDays, TXT("time; days"));
DEFINE_STATIC_NAME_STR(timeHours, TXT("time; hours"));
DEFINE_STATIC_NAME_STR(timeMinutes, TXT("time; minutes"));

DEFINE_STATIC_NAME_STR(lsFormatDMY, TXT("date time; full date DMY"));
DEFINE_STATIC_NAME_STR(lsFormatMDY, TXT("date time; full date MDY"));

DEFINE_STATIC_NAME_STR(lsMonday, TXT("date time; week day; monday"));
DEFINE_STATIC_NAME_STR(lsTuesday, TXT("date time; week day; tuesday"));
DEFINE_STATIC_NAME_STR(lsWednesday, TXT("date time; week day; wednesday"));
DEFINE_STATIC_NAME_STR(lsThursday, TXT("date time; week day; thursday"));
DEFINE_STATIC_NAME_STR(lsFriday, TXT("date time; week day; friday"));
DEFINE_STATIC_NAME_STR(lsSaturday, TXT("date time; week day; saturday"));
DEFINE_STATIC_NAME_STR(lsSunday, TXT("date time; week day; sunday"));

DEFINE_STATIC_NAME_STR(lsJanuary, TXT("date time; month; january"));
DEFINE_STATIC_NAME_STR(lsFebruary, TXT("date time; month; february"));
DEFINE_STATIC_NAME_STR(lsMarch, TXT("date time; month; march"));
DEFINE_STATIC_NAME_STR(lsApril, TXT("date time; month; april"));
DEFINE_STATIC_NAME_STR(lsMay, TXT("date time; month; may"));
DEFINE_STATIC_NAME_STR(lsJune, TXT("date time; month; june"));
DEFINE_STATIC_NAME_STR(lsJuly, TXT("date time; month; july"));
DEFINE_STATIC_NAME_STR(lsAugust, TXT("date time; month; august"));
DEFINE_STATIC_NAME_STR(lsSeptember, TXT("date time; month; september"));
DEFINE_STATIC_NAME_STR(lsOctober, TXT("date time; month; october"));
DEFINE_STATIC_NAME_STR(lsNovember, TXT("date time; month; november"));
DEFINE_STATIC_NAME_STR(lsDecember, TXT("date time; month; december"));

//

REGISTER_FOR_FAST_CAST(DateTime);

DateTime const DateTime::s_none = DateTime::create_none();
DateTime const DateTime::baseYearsFirstDay = DateTime(DateTime::baseYear, 1, 1);

DateTime::DateTime()
: isNone(true)
{
}

DateTime::DateTime(int _year, int _month, int _day)
: isNone(false)
, year(_year)
, month(_month)
, day(_day)
{
}

DateTime DateTime::create_none()
{
	DateTime dt;
	dt.isNone = true;
	return dt;
}

DateTime DateTime::get_current_from_system()
{
	time_t currentTime = time(0);
	tm currentTimeInfo;
#ifdef AN_WINDOWS
	localtime_s(&currentTimeInfo, &currentTime);
#else
	if (auto * cti = localtime(&currentTime))
	{
		currentTimeInfo = *cti;
	}
	else
	{
		an_assert(false, TXT("did not get time info"));
		return DateTime();
	}
#endif

	DateTime currDateTime;
	currDateTime.year = currentTimeInfo.tm_year + 1900;
	currDateTime.month = currentTimeInfo.tm_mon + 1;
	currDateTime.day = currentTimeInfo.tm_mday;
	currDateTime.hour = currentTimeInfo.tm_hour;
	currDateTime.minute = currentTimeInfo.tm_min;
	currDateTime.second = currentTimeInfo.tm_sec;

	currDateTime.isNone = false;

	return currDateTime;
}

String DateTime::get_string_system_date() const
{
	return isNone ?
		String::printf(TXT("----/--/--")) :
		String::printf(TXT("%04i/%02i/%02i"), year, month, day);
}

String DateTime::get_string_system_date_time() const
{
	return isNone ?
		String::printf(TXT("----/--/-- --:--:--")) :
		String::printf(TXT("%04i/%02i/%02i %02i:%02i:%02i"), year, month, day, hour, minute, second);
}

String DateTime::get_string_full_date(DateTimeFormat _dateTimeFormat) const
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	if (_dateTimeFormat.date == DateFormat::YMD ||
		_dateTimeFormat.date == DateFormat::DMY)
	{
		if (isNone)
		{
			return String::printf(TXT("---- --, ----"));
		}
		return Framework::StringFormatter::format_sentence_loc_str(NAME(lsFormatDMY),
			Framework::StringFormatterParams()
			.add(NAME(day), day)
			.add(NAME(month), month)
			.add(NAME(year), year));
		//return String::printf(TXT("%i %S, %04i"), day, get_string_month_full().to_char(), year);
	}
	if (isNone)
	{
		return String::printf(TXT("-- ----, ----"));
	}
	return Framework::StringFormatter::format_sentence_loc_str(NAME(lsFormatMDY),
		Framework::StringFormatterParams()
		.add(NAME(day), day)
		.add(NAME(month), month)
		.add(NAME(year), year));
	//return String::printf(TXT("%S %i, %04i"), get_string_month_full().to_char(), day, year);
}

String DateTime::get_string_full_date_time(DateTimeFormat _dateTimeFormat, bool _withSeconds) const
{
	return get_string_full_date(_dateTimeFormat).trim() + TXT(", ") + get_string_hour_minute(_dateTimeFormat, _withSeconds).trim();
}

int DateTime::get_string_compact_date_time_length(DateTimeFormat _dateTimeFormat, bool _withSeconds)
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	int length = 9; // this is fixed "xx/xx/xx"
	if (_dateTimeFormat.time == TimeFormat::H24)
	{
		length += 5; // xx:xx
	}
	else
	{
		length += 7; // xx:xxPM
	}
	if (_withSeconds)
	{
		length += 3; // :xx
	}
	return length;
}

int DateTime::get_string_wide_compact_date_time_length(DateTimeFormat _dateTimeFormat, bool _withSeconds)
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	int length = 11; // this is fixed "xxxx/xx/xx"
	if (_dateTimeFormat.time == TimeFormat::H24)
	{
		length += 5; // xx:xx
	}
	else
	{
		length += 7; // xx:xxPM
	}
	if (_withSeconds)
	{
		length += 3; // :xx
	}
	return length;
}

int DateTime::get_string_compact_date_length(DateTimeFormat _dateTimeFormat)
{
	return 8; // this is fixed "xx/xx/xx"
}

int DateTime::get_string_wide_compact_date_length(DateTimeFormat _dateTimeFormat)
{
	return 10; // this is fixed "xxxx/xx/xx"
}

String DateTime::get_string_compact_date_time(DateTimeFormat _dateTimeFormat, bool _withSeconds) const
{
	return get_string_compact_date(_dateTimeFormat) + TXT(" ") + get_string_compact_time(_dateTimeFormat, _withSeconds);
}

String DateTime::get_string_wide_compact_date_time(DateTimeFormat _dateTimeFormat, bool _withSeconds) const
{
	return get_string_wide_compact_date(_dateTimeFormat) + TXT(" ") + get_string_compact_time(_dateTimeFormat, _withSeconds);
}

int DateTime::get_string_compact_time_length(DateTimeFormat _dateTimeFormat, bool _withSeconds)
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	int length = _withSeconds ? 3 : 0; // :xx
	if (_dateTimeFormat.time == TimeFormat::H24)
	{
		length += 5; // xx:xx
	}
	else
	{
		length += 7; // xx:xxPM
	}
	return length;
}

String DateTime::get_string_compact_time(DateTimeFormat _dateTimeFormat, bool _withSeconds) const
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	if (_dateTimeFormat.time == TimeFormat::H24)
	{
		return isNone ?
			(_withSeconds ? String::printf(TXT("--:--:--"))
						  : String::printf(TXT("--:--"))) :
			(_withSeconds ? String::printf(TXT("%02i:%02i:%02i"), hour, minute, second)
						  : String::printf(TXT("%02i:%02i"), hour, minute));
	}
	else
	{
		int hourDisplay = hour % 12;
		if (hourDisplay == 0)
		{
			hourDisplay = 12;
		}
		return isNone ?
			(_withSeconds ? String::printf(TXT("--:--:----"))
						  : String::printf(TXT("--:----"))) :
			(_withSeconds ? String::printf(TXT("%2i:%02i:%02i%S"), hourDisplay, minute, second, hour < 12 ? TXT("am") : TXT("pm"))
						  : String::printf(TXT("%2i:%02i%S"), hourDisplay, minute, hour < 12 ? TXT("am") : TXT("pm")));
	}
}

String DateTime::get_string_compact_date(DateTimeFormat _dateTimeFormat) const
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	if (_dateTimeFormat.date == DateFormat::YMD)
	{
		return isNone ?
			String::printf(TXT("--/--/--")) :
			String::printf(TXT("%02i/%02i/%02i"), year % 100, month, day);
	}
	else if (_dateTimeFormat.date == DateFormat::DMY)
	{
		return isNone ?
			String::printf(TXT("--/--/--")) :
			String::printf(TXT("%02i/%02i/%02i"), day, month, year % 100);
	}
	else
	{
		return isNone ?
			String::printf(TXT("--/--/--")) :
			String::printf(TXT("%02i/%02i/%02i"), month, day, year % 100);
	}
}

String DateTime::get_string_wide_compact_date(DateTimeFormat _dateTimeFormat) const
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	if (_dateTimeFormat.date == DateFormat::YMD)
	{
		return isNone ?
			String::printf(TXT("----/--/--")) :
			String::printf(TXT("%04i/%02i/%02i"), year, month, day);
	}
	else if (_dateTimeFormat.date == DateFormat::DMY)
	{
		return isNone ?
			String::printf(TXT("--/--/----")) :
			String::printf(TXT("%02i/%02i/%04i"), day, month, year);
	}
	else
	{
		return isNone ?
			String::printf(TXT("--/--/----")) :
			String::printf(TXT("%02i/%02i/%04i"), month, day, year);
	}
}

String DateTime::format_string_week_day(int _weekDay, tchar const * _format, Framework::StringFormatterParams & _params)
{
	static Name weekDays[] = {
		NAME(lsMonday),
		NAME(lsTuesday),
		NAME(lsWednesday),
		NAME(lsThursday),
		NAME(lsFriday),
		NAME(lsSaturday),
		NAME(lsSunday)
	};

	return Framework::StringFormatter::format_sentence_loc_str(weekDays[_weekDay], _format, _params);
}

String DateTime::get_string_week_day_short(int _weekDay)
{
	todo_note(TXT("well \"short\" format is hardcoded here!"));
	return format_string_week_day(_weekDay, TXT("short"), Framework::StringFormatterParams().temp_ref());
}

String DateTime::get_string_week_day_full(int _weekDay)
{
	return format_string_week_day(_weekDay, nullptr, Framework::StringFormatterParams().temp_ref());
}

String DateTime::format_string_month(int _month, tchar const * _format, Framework::StringFormatterParams & _params)
{
	static Name months[] = {
		NAME(lsJanuary),
		NAME(lsFebruary),
		NAME(lsMarch),
		NAME(lsApril),
		NAME(lsMay),
		NAME(lsJune),
		NAME(lsJuly),
		NAME(lsAugust),
		NAME(lsSeptember),
		NAME(lsOctober),
		NAME(lsNovember),
		NAME(lsDecember)
	};

	return Framework::StringFormatter::format_sentence_loc_str(months[_month - 1], _format, _params);
}

String DateTime::get_string_month_short(int _month)
{
	todo_note(TXT("well \"short\" format is hardcoded here!"));
	return format_string_month(_month, TXT("short"), Framework::StringFormatterParams().temp_ref());
}

String DateTime::get_string_month_full(int _month)
{
	return format_string_month(_month, nullptr, Framework::StringFormatterParams().temp_ref());
}

String DateTime::get_string_hour_minute(DateTimeFormat _dateTimeFormat, bool _withSeconds) const
{
	GameConfig::update_date_time_format(REF_ _dateTimeFormat);
	if (_dateTimeFormat.time == TimeFormat::H24)
	{
		return isNone ?
			(_withSeconds? String::printf(TXT("--:--:--"))
						 : String::printf(TXT("--:--"))) :
			(_withSeconds? String::printf(TXT("%02i:%02i:%02i"), hour, minute, second)
						 : String::printf(TXT("%02i:%02i"), hour, minute));
	}
	else
	{
		int hourDisplay = hour % 12;
		if (hourDisplay == 0)
		{
			hourDisplay = 12;
		}
		return isNone ?
			(_withSeconds? String::printf(TXT("--:--:-- --"))
						 : String::printf(TXT("--:-- --"))) :
			(_withSeconds? String::printf(TXT("%2i:%02i:%02i %S"), hourDisplay, minute, second, hour < 12 ? TXT("am") : TXT("pm"))
						 : String::printf(TXT("%2i:%02i %S"), hourDisplay, minute, hour < 12 ? TXT("am") : TXT("pm")));
	}
}

void DateTime::advance_ms(float _ms)
{
	an_assert(is_valid());
	if (_ms > 0.0f)
	{
		ms += _ms;
		while (ms >= 1.0f)
		{
			ms -= 1.0f;
			advance_second(1);
		}
	}
	else if (_ms < 0.0f)
	{
		ms += _ms;
		while (ms < 0.0f)
		{
			ms += 1.0f;
			advance_second(-1);
		}
	}
}

void DateTime::advance_second(int _seconds)
{
	an_assert(is_valid());
	int advSeconds = _seconds % 60;
	second += advSeconds;
	int _minutes = (_seconds - advSeconds) / 60;
	int advMinutes = _minutes % 60;
	minute += advMinutes;
	int _hours = (_minutes - advMinutes) / 60;
	int advHours = _hours % 24;
	hour += advHours;
	int advDays = (_hours - advHours) / 24;
	day += advDays;

	// correct if we jumped seconds/minuts/hours
	while (second >= 60)
	{
		second -= 60;
		++minute;
	}
	while (minute >= 60)
	{
		minute -= 60;
		++hour;
	}
	while (hour >= 24)
	{
		hour -= 24;
		++day;
	}
	while (second < 0)
	{
		second += 60;
		--minute;
	}
	while (minute < 0)
	{
		minute += 60;
		--hour;
	}
	while (hour < 0)
	{
		hour += 24;
		--day;
	}

	// correct days/months/years
	while (day > calculate_days_of_month(year, month))
	{
		day -= calculate_days_of_month(year, month);
		++month;
		while (month > 12)
		{
			month -= 12;
			++year;
		}
	}
	while (day < 1)
	{
		--month;
		while (month < 1)
		{
			month += 12;
			--year;
		}
		day += calculate_days_of_month(year, month);
	}
}

void DateTime::advance_day(int _day)
{
	an_assert(is_valid());
	int advDays = _day;
	day += advDays;

	// correct days/months/years
	while (day > calculate_days_of_month(year, month))
	{
		day -= calculate_days_of_month(year, month);
		++month;
		while (month > 12)
		{
			month -= 12;
			++year;
		}
	}
	while (day < 1)
	{
		--month;
		while (month < 1)
		{
			month += 12;
			--year;
		}
		day += calculate_days_of_month(year, month);
	}
}

int DateTime::calculate_days_of_month(int _year, int _month)
{
	if (_month == 2)
	{
		if ((_year % 4) == 0 && ((_year % 100) != 0 || (_year % 400) == 0))
		{
			return 29;
		}
		else
		{
			return 28;
		}
	}
	if (_month == 1 || _month == 3 || _month == 5 || _month == 7 || _month == 8 || _month == 10 || _month == 12)
	{
		return 31;
	}
	else
	{
		return 30;
	}
}

int DateTime::calculate_days_of_year(int _year)
{
	if ((_year % 4) == 0 && ((_year % 100) != 0 || (_year % 400) == 0))
	{
		return 366;
	}
	else
	{
		return 365;
	}
}

#ifndef AN_CLANG
	#define CHECK_IF_PREV(_what) \
		if (_what < _a.##_what) return true; \
		if (_what > _a.##_what) return false;
#else
	#define CHECK_IF_PREV(_what) \
		if (_what < _a._what) return true; \
		if (_what > _a._what) return false;
#endif

bool DateTime::operator < (DateTime const & _a) const
{
	an_assert(is_valid() && _a.is_valid());
	CHECK_IF_PREV(year);
	CHECK_IF_PREV(month);
	CHECK_IF_PREV(day);
	CHECK_IF_PREV(hour);
	CHECK_IF_PREV(minute);
	CHECK_IF_PREV(second);
	CHECK_IF_PREV(ms);
	return false; // seconds match
}

bool DateTime::operator <= (DateTime const & _a) const
{
	an_assert(is_valid() && _a.is_valid());
	CHECK_IF_PREV(year);
	CHECK_IF_PREV(month);
	CHECK_IF_PREV(day);
	CHECK_IF_PREV(hour);
	CHECK_IF_PREV(minute);
	CHECK_IF_PREV(second);
	CHECK_IF_PREV(ms);
	return true; // seconds match
}

bool DateTime::operator > (DateTime const & _a) const
{
	return !(operator <= (_a));
}

bool DateTime::operator >= (DateTime const & _a) const
{
	return !(operator < (_a));
}

bool DateTime::operator == (DateTime const & _a) const
{
	an_assert(is_valid() && _a.is_valid());
	return year == _a.year &&
		   month == _a.month &&
		   day == _a.day &&
		   hour == _a.hour &&
		   minute == _a.minute &&
		   second == _a.second;
}

String DateTime::get_string_short_time(float _time, int _secondSubnumbers)
{
	float allSecondsAsFloat = floor(_time);
	float subSeconds = _time - allSecondsAsFloat;
	int timeLeft = (int)allSecondsAsFloat;
	int seconds = timeLeft % 60;
	timeLeft = (timeLeft - seconds) / 60;
	int minutes = timeLeft % 60;
	int hours = (timeLeft - minutes) / 60;
	String result;
	if (hours)
	{
		result += String::printf(TXT("%i:"), hours);
	}
	if (minutes)
	{
		if (result.is_empty())
		{
			result += String::printf(TXT("%i:"), minutes);
		}
		else
		{
			result += String::printf(TXT("%02i:"), minutes);
		}
	}
	if (result.is_empty())
	{
		result += String::printf(TXT("%i"), seconds);
	}
	else
	{
		result += String::printf(TXT("%02i"), seconds);
	}
	if (_secondSubnumbers > 0)
	{
		String format = String::printf(TXT(".%%0%i.0f"), _secondSubnumbers);
		while (_secondSubnumbers)
		{
			subSeconds *= 10.0f;
			--_secondSubnumbers;
		}
		result += String::printf(format.to_char(), subSeconds);
	}
	return result;
}

void DateTime::set_date(int _year, int _month, int _day)
{
	if (!is_valid())
	{
		isNone = false;
		set_time(0);
	}
	year = _year;
	month = _month;
	day = _day;
}

void DateTime::set_time(int _hour, int _minute, int _second)
{
	an_assert(is_valid());
	hour = _hour;
	minute = _minute;
	second = _second;
	ms = 0.0f;
}

void DateTime::set_time(Time const & _time)
{
	set_time(_time.get_hour(), _time.get_minute(), _time.get_second());
}

int DateTime::get_week_day() const
{
	int days = get_days_since_base();
	int bDays = baseYearsFirstDay.get_days_since_base();
	return mod((baseYearsFirstDayDayOfWeek - 1 + days - bDays), 7);
}

DateTime DateTime::get_start_of_the_week() const
{
	DateTime day = *this;
	day.advance_day(-get_week_day());
	return day;
}

int DateTime::get_days_since_base() const
{
	an_assert(is_valid());
	int diffDays = 0;
	if (year >= baseYear)
	{
		for (int y = baseYear; y < year; ++y)
		{
			diffDays += calculate_days_of_year(y);
		}
		diffDays += to_days_within_year();
	}
	else if (year < baseYear)
	{
		for (int y = year; y < baseYear; ++y)
		{
			diffDays += calculate_days_of_year(y);
		}
		diffDays -= to_days_within_year();
	}
	return diffDays;
}

int DateTime::to_days_within_year() const
{
	an_assert(is_valid());
	int withinYear = 0;
	for (int m = 1; m < month; ++m)
	{
		withinYear += calculate_days_of_month(year, m);
	}
	withinYear += day;
	--withinYear; // start at 0
	return withinYear;
}

Time DateTime::operator - (DateTime const & _a) const
{
	an_assert(is_valid() && _a.is_valid());
	int diffDays = get_days_since_base() - _a.get_days_since_base();

	int diffSeconds = Time(hour, minute, second).to_seconds_int() - Time(_a.hour, _a.minute, _a.second).to_seconds_int();

	diffSeconds += diffDays * 60 * 60 * 24;

	return Time::from_seconds(diffSeconds);
}

DateTime DateTime::operator + (Time const & _a) const
{
	an_assert(is_valid());
	DateTime result = *this;

	result.advance_ms(_a.get_ms());
	result.advance_second(_a.to_seconds_int());

	return result;
}

DateTime DateTime::operator - (Time const & _a) const
{
	an_assert(is_valid());
	DateTime result = *this;

	result.advance_ms(-_a.get_ms());
	result.advance_second(-_a.to_seconds_int());

	return result;
}

String DateTime::format_custom(tchar const * _format, Framework::StringFormatterParams & _params) const
{
	::Framework::StringFormatParser formatParser(_format);

	todo_note(TXT("how to provide proper date format here?"));

	while (tchar const * format = formatParser.get_next())
	{
		if (String::compare_icase(format, TXT("weekday")) ||
			String::compare_icase(format, TXT("weekDay")) ||
			String::compare_icase(format, TXT("dayOfWeek")))
		{
			return format_string_week_day(_format, _params);
		}
		if (String::compare_icase(format, TXT("system")))
		{
			return get_string_system_date_time();
		}
		if (String::compare_icase(format, TXT("full")))
		{
			return get_string_full_date(_params.get_date_time_format());
		}
		if (String::compare_icase(format, TXT("fullWithTime")))
		{
			return get_string_full_date_time(_params.get_date_time_format());
		}
		if (String::compare_icase(format, TXT("fullWithTimeSec")))
		{
			return get_string_full_date_time(_params.get_date_time_format(), true);
		}
		if (String::compare_icase(format, TXT("compactWithTime")))
		{
			return get_string_compact_date_time(_params.get_date_time_format());
		}
		if (String::compare_icase(format, TXT("compactWithTimeSec")))
		{
			return get_string_compact_date_time(_params.get_date_time_format(), true);
		}
		if (String::compare_icase(format, TXT("compactTime")))
		{
			return get_string_compact_time(_params.get_date_time_format());
		}
		if (String::compare_icase(format, TXT("compactTimeSec")))
		{
			return get_string_compact_time(_params.get_date_time_format(), true);
		}
		if (String::compare_icase(format, TXT("compact")))
		{
			return get_string_compact_date(_params.get_date_time_format());
		}
		if (String::compare_icase(format, TXT("time")) ||
			String::compare_icase(format, TXT("timeHM")) ||
			String::compare_icase(format, TXT("fullTime")) ||
			String::compare_icase(format, TXT("fullTimeHM")))
		{
			return get_string_hour_minute(_params.get_date_time_format());
		}
	}

	return get_string_full_date_time(_params.get_date_time_format());
}

DateTime DateTime::get_in_following_24h(Time const & _timeOfDay) const
{
	return get_in_following_24h(_timeOfDay, Time::zero());
}

DateTime DateTime::get_in_following_24h(Time const & _timeOfDay, Time const & _canBeLate) const
{
	DateTime result = *this;
	result.hour = _timeOfDay.get_hour();
	result.minute = _timeOfDay.get_minute();
	result.second = _timeOfDay.get_second();
	if (result < *this)
	{
		result += Time(24);
	}
	if (! _canBeLate.is_zero() &&
		result > *this + Time(24) - _canBeLate)
	{
		result -= Time(24);
	}
	return result;
}

String DateTime::get_string_days_to(DateTime const & _dateInFuture) const
{
	int dayThis = get_days_since_base();
	int dayInFuture = _dateInFuture.get_days_since_base();
	int daysLeft = dayInFuture - dayThis;

	if (daysLeft == 0)
	{
		return LOC_STR(NAME(today));
	}
	if (daysLeft > 0)
	{
		return Framework::StringFormatter::format_sentence_loc_str((NAME(timeDays)), Framework::StringFormatterParams().add(NAME(days), daysLeft));
	}
	return String::empty();
}

String DateTime::get_string_single_unit_to(DateTime const & _dateInFuture) const
{
	if (_dateInFuture < *this)
	{
		return String::empty();
	}
	int dayThis = get_days_since_base();
	int dayInFuture = _dateInFuture.get_days_since_base();
	int daysLeft = dayInFuture - dayThis;

	if (daysLeft == 0)
	{
		Time diff = _dateInFuture - *this;
		// if it is more than 45 minutes, say it will be done in an hour
		if (diff.get_hour() >= 1 || diff.get_minute() >= 45)
		{
			// if it is less than 1:20 then it is in 1 hour
			return Framework::StringFormatter::format_sentence_loc_str((NAME(timeHours)), Framework::StringFormatterParams().add(NAME(hours), (diff - Time::from_minutes(20)).get_hour() + 1));
		}
		else
		{
			return Framework::StringFormatter::format_sentence_loc_str((NAME(timeMinutes)), Framework::StringFormatterParams().add(NAME(minutes), diff.get_minute() + 1));
		}
	}
	else
	{
		an_assert(daysLeft > 0);
		return Framework::StringFormatter::format_sentence_loc_str((NAME(timeDays)), Framework::StringFormatterParams().add(NAME(days), daysLeft));
	}
}

bool DateTime::is_same_day(DateTime const & _a, DateTime const & _b)
{
	return _a.year == _b.year &&
		   _a.month == _b.month &&
		   _a.day == _b.day;
}

bool DateTime::is_earlier_day_than(DateTime const & _a) const
{
	CHECK_IF_PREV(year);
	CHECK_IF_PREV(month);
	CHECK_IF_PREV(day);
	return false; //same day
}

bool DateTime::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return true;
	}
	bool result = true;

	if (_node->has_attribute(TXT("year")) ||
		_node->has_attribute(TXT("month")) ||
		_node->has_attribute(TXT("day")) ||
		_node->has_attribute(TXT("hour")) ||
		_node->has_attribute(TXT("minute")) ||
		_node->has_attribute(TXT("second")))
	{
		isNone = false;
		year = _node->get_int_attribute(TXT("year"), year);
		month = _node->get_int_attribute(TXT("month"), month);
		day = _node->get_int_attribute(TXT("day"), day);
		hour = _node->get_int_attribute(TXT("hour"), hour);
		minute = _node->get_int_attribute(TXT("minute"), minute);
		second = _node->get_int_attribute(TXT("second"), second);
	}
	else
	{
		isNone = true;
	}

	return result;
}

bool DateTime::save_to_xml(IO::XML::Node * _node) const
{
	if (!_node)
	{
		return false;
	}
	bool result = true;

	if (!isNone)
	{
		_node->set_int_attribute(TXT("year"), year);
		_node->set_int_attribute(TXT("month"), month);
		_node->set_int_attribute(TXT("day"), day);
		_node->set_int_attribute(TXT("hour"), hour);
		_node->set_int_attribute(TXT("minute"), minute);
		_node->set_int_attribute(TXT("second"), second);
	}

	return result;
}

Time DateTime::get_time() const
{
	return Time(hour, minute, second);
}

bool DateTime::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, isNone);
	result &= serialise_data(_serialiser, year);
	result &= serialise_data(_serialiser, month);
	result &= serialise_data(_serialiser, day);
	result &= serialise_data(_serialiser, hour);
	result &= serialise_data(_serialiser, minute);
	result &= serialise_data(_serialiser, second);
	result &= serialise_data(_serialiser, ms);

	return result;
}

