#include "time.h"

#include "..\text\localisedString.h"

#include "..\..\core\other\parserUtils.h"
#include "..\..\core\random\random.h"

#include <time.h>

using namespace Framework;

//

DEFINE_STATIC_NAME(value);
DEFINE_STATIC_NAME_STR(timeDays, TXT("time; days"));
DEFINE_STATIC_NAME_STR(timeHours, TXT("time; hours"));
DEFINE_STATIC_NAME_STR(timeMinutes, TXT("time; minutes"));
DEFINE_STATIC_NAME_STR(timeSeconds, TXT("time; seconds"));
DEFINE_STATIC_NAME(days);
DEFINE_STATIC_NAME(hours);
DEFINE_STATIC_NAME(minutes);
DEFINE_STATIC_NAME(seconds);

//

REGISTER_FOR_FAST_CAST(Time);

Time const Time::s_none = Time::create_none();
Time const Time::s_zero = Time(0, 0, 0);
Time const Time::s_day = Time(24, 0, 0);

Time Time::from_days(int _days)
{
	Time result;
	result.isNone = false;
	result.hour = _days * 24;
	result.minute = 0;
	result.second = 0;
	result.ms = 0.0f;
	return result;
}

Time Time::from_seconds(int _time)
{
	Time result;
	result.isNone = false;
	int sign = _time >= 0 ? 1 : -1;
	_time = abs(_time);
	result.second = _time % 60;
	_time = (_time - result.second) / 60;
	result.minute = _time % 60;
	_time = (_time - result.minute) / 60;
	result.hour = _time;
	//
	result.hour *= sign;
	result.minute *= sign;
	result.second *= sign;
	//
	return result;
}

Time Time::from_seconds(float _time)
{
	Time result;
	result.isNone = false;
	int time = (int)floor_to_zero(_time);
	int sign = time >= 0.0f ? 1 : -1;
	time = abs(time);
	result.second = time % 60;
	time = (time - result.second) / 60;
	result.minute = time % 60;
	time = (time - result.minute) / 60;
	result.hour = time;
	result.ms = _time - floor_to_zero(_time);
	//
	result.hour *= sign;
	result.minute *= sign;
	result.second *= sign;
	result.ms *= sign;
	//
	return result;
}

void Time::advance_ms(float _ms)
{
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

void Time::advance_second(int _seconds)
{
	int advSeconds = _seconds % 60;
	second += advSeconds;
	int _minutes = (_seconds - advSeconds) / 60;
	int advMinutes = _minutes % 60;
	minute += advMinutes;
	int _hours = (_minutes - advMinutes) / 60;
	hour += _hours;

	make_valid();
}

void Time::make_valid()
{
	int sign = is_negative() ? -1 : 1;
	second = abs(second);
	minute = abs(minute);
	hour = abs(hour);
	if (second >= 60)
	{
		minute += (second - (second % 60)) / 60;
		second = second % 60;
	}
	if (minute >= 60)
	{
		hour += (minute - (minute % 60)) / 60;
		minute = minute % 60;
	}
	second *= sign;
	minute *= sign;
	hour *= sign;
}

void Time::set_time(int _hour, int _minute, int _second)
{
	isNone = false;
	hour = _hour;
	minute = _minute;
	second = _second;
	ms = 0.0f;
}

String Time::get_for_parse() const
{
	String result;
	result = String::printf(TXT("%ih%im%is"), hour, minute, second);
	if (ms != 0.0f)
	{
		result += String::printf(TXT("%.6fu"), ms);
	}
	return result;
}

bool Time::parse_from(String const & _text)
{
	isNone = false;
	int dAt = _text.find_first_of('d');
	if (dAt == NONE)
	{
		dAt = _text.find_first_of('D');
	}
	String time;
	hour = 0;
	minute = 0;
	second = 0;
	ms = 0.0f;
	if (dAt != NONE)
	{
		int days = ParserUtils::parse_int(_text.get_left(dAt));
		hour = abs(days) * 24;
		time = _text.get_sub(dAt + 1);
	}
	else
	{
		time = _text;
	}
	bool processTimeAsUsual = true;
	int hAt = time.find_first_of('h');
	if (hAt == NONE)
	{
		hAt = time.find_first_of('H');
	}
	if (hAt != NONE)
	{
		hour = abs(ParserUtils::parse_int(time.get_left(hAt)));
		time = time.get_sub(hAt + 1);
		processTimeAsUsual = false;
	}
	int mAt = time.find_first_of('m');
	if (mAt == NONE)
	{
		mAt = time.find_first_of('M');
	}
	if (mAt != NONE)
	{
		minute = abs(ParserUtils::parse_int(time.get_left(mAt)));
		time = time.get_sub(mAt + 1);
		processTimeAsUsual = false;
	}
	else if (hAt != NONE)
	{
		// 1h30
		minute = abs(ParserUtils::parse_int(time));
		time = String::empty();
		processTimeAsUsual = false;
	}
	int sAt = time.find_first_of('s');
	if (sAt == NONE)
	{
		sAt = time.find_first_of('S');
	}
	if (sAt != NONE)
	{
		second = abs(ParserUtils::parse_int(time.get_left(sAt)));
		time = time.get_sub(sAt + 1);
		processTimeAsUsual = false;
	}
	else if (mAt != NONE)
	{
		// 1m30
		second = abs(ParserUtils::parse_int(time));
		time = String::empty();
		processTimeAsUsual = false;
	}
	int uAt = time.find_first_of('u');
	if (uAt == NONE)
	{
		uAt = time.find_first_of('U');
	}
	if (uAt != NONE)
	{
		if (sAt != NONE)
		{
			ms = abs(ParserUtils::parse_float(time.get_left(uAt)));
			time = time.get_sub(uAt + 1);
			processTimeAsUsual = false;
		}
		else
		{
			error(TXT("if there is \"u\" component there should be \"s\" component too, in \"%S\""), _text.to_char());
		}
	}
	if (processTimeAsUsual)
	{
		List<String> parts;
		time.split(String(TXT(":")), parts, 3);
		int partIdx = 0;
		if (parts.get_size() > 2)
		{
			hour += ParserUtils::parse_int(parts[partIdx]);
			++partIdx;
		}
		if (parts.get_size() > 1)
		{
			minute += ParserUtils::parse_int(parts[partIdx]);
			++partIdx;
		}
		if (parts.get_size() > 0)
		{
			second += ParserUtils::parse_int(parts[partIdx]);
			++partIdx;
		}
	}
	hour = abs(hour);
	minute = abs(minute);
	second = abs(second);
	ms = abs(ms);
	while (ms >= 1.0f)
	{
		++ second;
		ms -= 1.0f;
	}
	if (second >= 60)
	{
		minute += (second - (second % 60)) / 60;
		second = second % 60;
	}
	if (minute >= 60)
	{
		hour += (minute - (minute % 60)) / 60;
		minute = minute % 60;
	}
	if (time.find_first_of('-') != NONE)
	{
		hour = -hour;
		minute = -minute;
		second = -second;
	}
	return is_ok();
}

bool Time::parse_time_of_day_from(String const & _text)
{
	List<String> parts;
	_text.split(String(TXT(":")), parts, 3);
	int partIdx = 0;
	isNone = false;
	hour = 0;
	minute = 0;
	second = 0;
	ms = 0.0f;
	if (parts.get_size() > 0)
	{
		hour += ParserUtils::parse_int(parts[partIdx]);
		++partIdx;
	}
	if (parts.get_size() > 1)
	{
		minute += ParserUtils::parse_int(parts[partIdx]);
		++partIdx;
	}
	if (parts.get_size() > 2)
	{
		second += ParserUtils::parse_int(parts[partIdx]);
		++partIdx;
	}
	return is_ok();
}

Time Time::get_random(Random::Generator & _generator, Time const & _min, Time const & _max)
{
	return from_seconds(_generator.get_int_from_range(_min.to_seconds_int(), _max.to_seconds_int()));
}

String Time::format_custom(tchar const * _format, Framework::StringFormatterParams & _params) const
{
	::Framework::StringFormatParser formatParser(_format);

	while (tchar const * format = formatParser.get_next())
	{
		if (String::compare_icase(format, TXT("hours")))
		{
			return AVOID_CALLING_ACK_ Framework::StringFormatter::format_sentence(String::printf(TXT("%%value%S%%"), _format), Framework::StringFormatterParams()
				.add(NAME(value), to_hours()));
		}
		if (String::compare_icase(format, TXT("compact")) ||
			String::compare_icase(format, TXT("compactHM")))
		{
			return get_compact_hour_minute_string();
		}
		if (String::compare_icase(format, TXT("compactSec")) ||
			String::compare_icase(format, TXT("compactHMS")))
		{
			return get_compact_hour_minute_second_string();
		}
		if (String::compare_icase(format, TXT("full")) ||
			String::compare_icase(format, TXT("fullHM")))
		{
			return get_full_hour_minute_string();
		}
		if (String::compare_icase(format, TXT("fullSec")) ||
			String::compare_icase(format, TXT("fullHMS")))
		{
			return get_full_hour_minute_second_string();
		}
		if (String::compare_icase(format, TXT("desc")) ||
			String::compare_icase(format, TXT("descDHMS")))
		{
			return get_descriptive_day_hour_minute_second_string();
		}
	}

	return get_full_hour_minute_second_string();
}

String Time::get_simple_compact_hour_minute_second_string() const
{
	String result(is_negative() ? TXT("-") : TXT(""));
	tchar const * leading = TXT("%i");
	tchar const * following = TXT(":%02i");
	tchar const * current = leading;
	if (hour != 0)
	{
		result += String::printf(current, abs(hour));
		current = following;
	}
	if (current == following ||
		minute != 0)
	{
		result += String::printf(current, abs(minute));
		current = following;
	}
	//if (current == following ||
	//	second != 0)
	{
		result += String::printf(current, abs(second));
		current = following;
	}
	return result;
}

String Time::get_descriptive_day_hour_minute_second_string() const
{
	String result;
	int asHours = hour;
	int asDays = (asHours - mod_raw(asHours, 24)) / 24;
	asHours -= asDays * 24;
	if (asDays)
	{
		result += Framework::StringFormatter::format_sentence_loc_str((NAME(timeDays)), Framework::StringFormatterParams().add(NAME(days), abs(asDays)));
	}
	if (asHours)
	{
		if (!result.is_empty())
		{
			result += String::space();
		}
		result += Framework::StringFormatter::format_sentence_loc_str((NAME(timeHours)), Framework::StringFormatterParams().add(NAME(hours), abs(asHours)));
	}
	if (minute)
	{
		if (!result.is_empty())
		{
			result += String::space();
		}
		result += Framework::StringFormatter::format_sentence_loc_str((NAME(timeMinutes)), Framework::StringFormatterParams().add(NAME(minutes), abs(minute)));
	}
	if (second)
	{
		if (!result.is_empty())
		{
			result += String::space();
		}
		result += Framework::StringFormatter::format_sentence_loc_str((NAME(timeSeconds)), Framework::StringFormatterParams().add(NAME(seconds), abs(second)));
	}
	if (is_negative())
	{
		result = String(TXT("-")) + result;
	}
	return result;
}

String Time::get_string_short_time(int _secondSubnumbers) const
{
	float time = to_seconds();
	float allSecondsAsFloat = floor(time);
	float subSeconds = time - allSecondsAsFloat;
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

bool Time::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	return load_from_xml(_node);
}

bool Time::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return true;
	}
	bool result = true;

	if (_node->has_attribute(TXT("hour")) ||
		_node->has_attribute(TXT("minute")) ||
		_node->has_attribute(TXT("second")))
	{
		isNone = false;
		hour = _node->get_int_attribute(TXT("hour"));
		minute = _node->get_int_attribute(TXT("minute"));
		second = _node->get_int_attribute(TXT("second"));
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("hm")))
	{
		result &= parse_time_of_day_from(attr->get_as_string());
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("hhmm")))
	{
		result &= parse_time_of_day_from(attr->get_as_string());
	}

	if (!is_valid())
	{
		error_loading_xml(_node, TXT("did not load time properly!"));
		result = false;
	}

	return result;
}

bool Time::load_from_xml(IO::XML::Node const* _node, tchar const* _attrName)
{
	return load_from_xml(_node->get_attribute(_attrName));
}

bool Time::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return true;
	}
	bool result = true;

	result &= parse_from(_attr->get_as_string());

	if (!is_valid())
	{
		error_loading_xml(_attr->get_parent_node(), TXT("did not load time properly!"));
		result = false;
	}

	return result;
}

bool Time::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, isNone);
	result &= serialise_data(_serialiser, hour);
	result &= serialise_data(_serialiser, minute);
	result &= serialise_data(_serialiser, second);
	result &= serialise_data(_serialiser, ms);

	return result;
}

