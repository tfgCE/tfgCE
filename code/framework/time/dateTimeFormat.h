#pragma once

#include "..\..\core\types\string.h"
#include "..\..\core\serialisation\serialiserBasics.h"

namespace Framework
{
	namespace DateFormat
	{
		enum Type
		{
			Default,
			MDY,
			DMY,
			YMD
		};

		Type parse(String const & _string);
		String to_string(Type _type);
		Array<String> get_option_strings();
	};

	namespace TimeFormat
	{
		enum Type
		{
			Default,
			H12,
			H24
		};

		Type parse(String const & _string);
		String to_string(Type _type);
		Array<String> get_option_strings();
	};

	struct DateTimeFormat
	{
		static DateTimeFormat const & none() { return s_none; }
		DateFormat::Type date = DateFormat::Default;
		TimeFormat::Type time = TimeFormat::Default;

		DateTimeFormat() {}
		DateTimeFormat(DateFormat::Type _date, TimeFormat::Type _time) : date(_date), time(_time) {}

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _childName = TXT("dateTimeFormat"));
		bool save_to_xml(IO::XML::Node * _node, tchar const * _childName = TXT("dateTimeFormat")) const;

		bool serialise(Serialiser & _serialiser);

		bool operator != (DateTimeFormat const & _other) const { return date != _other.date || time != _other.time; }

	private:
		static DateTimeFormat s_none;
	};

};

DECLARE_SERIALISER_FOR(Framework::DateFormat::Type);
DECLARE_SERIALISER_FOR(Framework::TimeFormat::Type);