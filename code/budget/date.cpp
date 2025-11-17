#include "date.h"

#include <locale>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "..\core\io\file.h"
#include "..\core\other\parserUtils.h"

using namespace Budget;

Date::Date()
: year(0)
, month(0)
, day(0)
{
}

bool Date::load_from_gdocs_csv(IO::File* file)
{
	// omit end of line, start with new line
	tchar date[11];
	while (file->get_last_read_char() == 10 ||
		   file->get_last_read_char() == 13)
	{
		file->read_char();
	}
	date[0] = file->get_last_read_char();
	for (int i = 1; i < 10; ++i)
	{
		date[i] = file->read_char();
	}
	date[4] = 0;
	date[7] = 0;
	date[10] = 0;
	year = ParserUtils::parse_int(&date[0]);
	month = ParserUtils::parse_int(&date[5]);
	day = ParserUtils::parse_int(&date[8]);
	return file->has_last_read_char();
}

bool Date::load_from_auto_hh_mm_ss(IO::File* file)
{
	// omit end of line, start with new line, "
	tchar date[20]; // DD_MM_YYYY_HH_MM_SS
					// YYYY_MM_DD_HH_MM_SS
					// 01234567890123456789
					// 0         1
	bool quotes = false;
	while (file->get_last_read_char() == 10 ||
		   file->get_last_read_char() == 13 ||
		   file->get_last_read_char() == '"')
	{
		quotes |= file->get_last_read_char() == '"';
		file->read_char();
	}
	date[0] = file->get_last_read_char();
	for (int i = 1; i < 19; ++i)
	{
		date[i] = file->read_char();
	}
	bool yyyymmdd = date[4] == '-' || date[4] == ' ' || date[4] == '.';
	if (yyyymmdd)
	{
		date[4] = 0;
		date[7] = 0;
	}
	else
	{
		date[2] = 0;
		date[5] = 0;
	}
	date[10] = 0;
	date[13] = 0;
	date[16] = 0;
	date[19] = 0;
	if (yyyymmdd)
	{
		year = ParserUtils::parse_int(&date[0]);
		month = ParserUtils::parse_int(&date[5]);
		day = ParserUtils::parse_int(&date[8]);
	}
	else
	{
		year = ParserUtils::parse_int(&date[6]);
		month = ParserUtils::parse_int(&date[3]);
		day = ParserUtils::parse_int(&date[0]);
	}
	hour = ParserUtils::parse_int(&date[11]);
	minute = ParserUtils::parse_int(&date[14]);
	second = ParserUtils::parse_int(&date[17]);
	if (quotes)
	{
		file->read_char();
	}
	return file->has_last_read_char();
}

bool Date::load_from_dd_mm_yyyy_hh_mm_ss(IO::File* file)
{
	// omit end of line, start with new line, "
	tchar date[20]; // DD_MM_YYYY_HH_MM_SS
					// 01234567890123456789
					// 0         1
	bool quotes = false;
	while (file->get_last_read_char() == 10 ||
		   file->get_last_read_char() == 13 ||
		   file->get_last_read_char() == '"')
	{
		quotes |= file->get_last_read_char() == '"';
		file->read_char();
	}
	date[0] = file->get_last_read_char();
	for (int i = 1; i < 19; ++i)
	{
		date[i] = file->read_char();
	}
	date[2] = 0;
	date[5] = 0;
	date[10] = 0;
	date[13] = 0;
	date[16] = 0;
	date[19] = 0;
	year = ParserUtils::parse_int(&date[6]);
	month = ParserUtils::parse_int(&date[3]);
	day = ParserUtils::parse_int(&date[0]);
	hour = ParserUtils::parse_int(&date[11]);
	minute = ParserUtils::parse_int(&date[14]);
	second = ParserUtils::parse_int(&date[17]);
	if (quotes)
	{
		file->read_char();
	}
	return file->has_last_read_char();
}

bool Date::load_from_yyyy_mm_dd_hh_mm_ss(IO::File* file)
{
	// omit end of line, start with new line, "
	tchar date[20]; // YYYY_MM_DD_HH_MM_SS
					// 01234567890123456789
					// 0         1
	bool quotes = false;
	while (file->get_last_read_char() == 10 ||
		   file->get_last_read_char() == 13 ||
		   file->get_last_read_char() == '"')
	{
		quotes |= file->get_last_read_char() == '"';
		file->read_char();
	}
	date[0] = file->get_last_read_char();
	for (int i = 1; i < 19; ++i)
	{
		date[i] = file->read_char();
	}
	date[4] = 0;
	date[7] = 0;
	date[10] = 0;
	date[13] = 0;
	date[16] = 0;
	date[19] = 0;
	year = ParserUtils::parse_int(&date[0]);
	month = ParserUtils::parse_int(&date[5]);
	day = ParserUtils::parse_int(&date[8]);
	hour = ParserUtils::parse_int(&date[11]);
	minute = ParserUtils::parse_int(&date[14]);
	second = ParserUtils::parse_int(&date[17]);
	if (quotes)
	{
		file->read_char();
	}
	return file->has_last_read_char();
}

bool Date::load_from_month_dd_yyyy(IO::File* file)
{
	// omit end of line, start with new line, "
	tchar date[13]; // MNT_DD,_YYYY
					// 0123456789012
					// 0         1
	bool quotes = false;
	while (file->get_last_read_char() == 10 ||
		file->get_last_read_char() == 13 ||
		file->get_last_read_char() == '"')
	{
		quotes |= file->get_last_read_char() == '"';
		file->read_char();
	}
	date[0] = file->get_last_read_char();
	for (int i = 1; i < 12; ++i)
	{
		date[i] = file->read_char();
	}
	date[3] = 0;
	date[6] = 0;
	date[12] = 0;
	year = ParserUtils::parse_int(&date[8]);
	month = 1;
	String monthStr(&date[0]);
	monthStr = monthStr.to_lower();
	if (monthStr == TXT("jan")) month = 1; else
	if (monthStr == TXT("feb")) month = 2; else
	if (monthStr == TXT("mar")) month = 3; else
	if (monthStr == TXT("apr")) month = 4; else
	if (monthStr == TXT("may")) month = 5; else
	if (monthStr == TXT("jun")) month = 6; else
	if (monthStr == TXT("jul")) month = 7; else
	if (monthStr == TXT("aug")) month = 8; else
	if (monthStr == TXT("sep")) month = 9; else
	if (monthStr == TXT("oct")) month = 10; else
	if (monthStr == TXT("nov")) month = 11; else
	if (monthStr == TXT("dec")) month = 12; else
	if (! monthStr.is_empty())
	{
		error(TXT("not recognised month %S"), monthStr.to_char());
	}
	day = ParserUtils::parse_int(&date[4]);
	hour = 0;
	minute = 0;
	second = 0;
	if (quotes)
	{
		file->read_char();
	}
	return file->has_last_read_char();
}

bool read_any_token(IO::Interface* _file, Token & _token);

bool Date::load_from(IO::File* file)
{
	Token token;
	if (read_any_token(file, token))
	{
		int i = 0;
		tchar const * ch = token.to_char();
		int reading = 0;
		String readNumber;
		while (i < token.get_length())
		{
			if (*ch == '-')
			{
				if (reading == 0)
				{
					year = ParserUtils::parse_int(readNumber);
				}
				if (reading == 1)
				{
					month = ParserUtils::parse_int(readNumber);
				}
				if (reading == 2)
				{
					day = ParserUtils::parse_int(readNumber);
				}
				readNumber = String::empty();
				++reading;
				if (reading > 2)
				{
					error(TXT("incorrect date %S"), token.to_char());
					return false;
				}
			}
			else
			{
				readNumber += *ch;
			}
			++i;
			++ch;
		}
		if (reading == 0)
		{
			year = ParserUtils::parse_int(readNumber);
		}
		if (reading == 1)
		{
			month = ParserUtils::parse_int(readNumber);
		}
		if (reading == 2)
		{
			day = ParserUtils::parse_int(readNumber);
		}

		return true;
	}
	return false;
}

String Date::to_string_date() const
{
	String result;
	result += String::printf(TXT("%04i"), year);
	if (month != 0)
	{
		result += String::printf(TXT("-%02i"), month);
		if (day != 0)
		{
			result += String::printf(TXT("-%02i"), day);
		}
	}
	return result;
}

String Date::to_string_dd_mm_yyyy_hh_mm_ss() const
{
	String result;
	result += String::printf(TXT("%02i"), day);
	result += String::printf(TXT("-%02i"), month);
	result += String::printf(TXT("-%04i"), year);
	result += ' ';
	result += String::printf(TXT("%02i"), hour);
	result += String::printf(TXT(":%02i"), minute);
	result += String::printf(TXT(":%02i"), second);
	return result;
}

String Date::to_string_universal() const
{
	String result;
	result += String::printf(TXT("%04i"), year);
	result += String::printf(TXT("-%02i"), month);
	result += String::printf(TXT("-%02i"), day);
	result += 'T';
	result += String::printf(TXT("%02i"), hour);
	result += String::printf(TXT(":%02i"), minute);
	result += String::printf(TXT(":%02i"), second);
	result += 'Z';
	return result;
}

void Date::parse_yyyy_mm_dd_minus(String const & _txt)
{
	List<String> tokens;
	_txt.split(String(TXT("-")), tokens);
	if (tokens.get_size() >= 1)
	{
		year = ParserUtils::parse_int(tokens[0]);
	}
	if (tokens.get_size() >= 2)
	{
		month = ParserUtils::parse_int(tokens[1]);
	}
	if (tokens.get_size() >= 3)
	{
		day = ParserUtils::parse_int(tokens[2]);
	}
	hour = 0;
	minute = 0;
	second = 0;
}

void Date::parse_yyyy_mm_dd_minus_hh_mm_ss(String const & _txt)
{
	List<String> tokens;
	_txt.split(String(TXT(" ")), tokens);
	if (tokens.get_size() >= 1)
	{
		parse_yyyy_mm_dd_minus(tokens[0]);
	}
	if (tokens.get_size() >= 2)
	{
		parse_hh_mm_ss(tokens[1]);
	}
}

void Date::parse_dd_mm_yyyy_dot(String const & _txt)
{
	List<String> tokens;
	_txt.split(String(TXT(".")), tokens);
	if (tokens.get_size() >= 1)
	{
		day = ParserUtils::parse_int(tokens[0]);
	}
	if (tokens.get_size() >= 2)
	{
		month = ParserUtils::parse_int(tokens[1]);
	}
	if (tokens.get_size() >= 3)
	{
		year = ParserUtils::parse_int(tokens[2]);
	}
	hour = 0;
	minute = 0;
	second = 0;
}

void Date::parse_hh_mm_ss(String const& _txt)
{
	List<String> tokens;
	_txt.split(String(TXT(":")), tokens);
	if (tokens.get_size() >= 1)
	{
		hour = ParserUtils::parse_int(tokens[0]);
	}
	if (tokens.get_size() >= 2)
	{
		minute = ParserUtils::parse_int(tokens[1]);
	}
	if (tokens.get_size() >= 3)
	{
		second = ParserUtils::parse_int(tokens[2]);
	}
}

void Date::advance_day()
{
	++day;
	if ((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) && day > 31)
	{
		day = 1;
		++month;
	}
	if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30)
	{
		day = 1;
		++month;
	}
	if (month == 2)
	{
		int limit = 28;
		if ((year % 4) == 0 &&
			((year % 100) != 0 || (year % 400) != 0))
		{
			limit = 29;
		}
		if (day > limit)
		{
			day = 1;
			++month;
		}
	}
	if (month > 12)
	{
		month = 1;
		++year;
	}
}

void Date::advance_hour()
{
	++hour;
	if (hour >= 24)
	{
		hour = 0;
		advance_day();
	}
}

void Date::advance_second()
{
	++second;
	if (hour >= 60)
	{
		second = 0;
		++minute;
		if (minute >= 60)
		{
			minute = 0;
			advance_hour();
		}
	}
}

int Date::distance_between(Date const & a, Date const & b)
{
	int64 at = a.to_seconds();
	int64 bt = b.to_seconds();

	return (int)abs(at - bt);
}

int Date::difference_between(Date const & a, Date const & b)
{
	int64 at = a.to_seconds();
	int64 bt = b.to_seconds();

	return (int)(at - bt);
}

bool Date::almost(Date const & a, Date const & b)
{
	if (a == b)
	{
		return true;
	}

	if (a.year == b.year &&
		a.month == b.month &&
		a.day == b.day)
	{
		return distance_between(a, b) <= 1;
	}

	return false;
}

int64 Date::to_seconds() const
{
	if (ds_year != year ||
		ds_month != month ||
		ds_day != day ||
		ds_hour != hour ||
		ds_minute != minute ||
		ds_second != second)
	{
		tm aTM;
		memory_zero(&aTM, sizeof(tm));

		String asString = to_string_universal();
		std::istringstream aS(asString.to_char8_array().get_data());

		aS >> std::get_time(&aTM, "%Y-%m-%dT%H:%M:%S");

		dateSec = mktime(&aTM);
		ds_year = year;
		ds_month = month;
		ds_day = day;
		ds_hour = hour;
		ds_minute = minute;
		ds_second = second;
	}

	return dateSec;
}
