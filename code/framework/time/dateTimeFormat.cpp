#include "dateTimeFormat.h"

#include "..\..\core\io\xml.h"
#include "..\..\core\serialisation\serialiser.h"

#include "..\text\localisedString.h"

using namespace Framework;

//

BEGIN_SERIALISER_FOR_ENUM(Framework::DateFormat::Type, byte)
	ADD_SERIALISER_FOR_ENUM(0, Framework::DateFormat::Default)
	ADD_SERIALISER_FOR_ENUM(1, Framework::DateFormat::MDY)
	ADD_SERIALISER_FOR_ENUM(2, Framework::DateFormat::DMY)
	ADD_SERIALISER_FOR_ENUM(3, Framework::DateFormat::YMD)
END_SERIALISER_FOR_ENUM(Framework::DateFormat::Type)

BEGIN_SERIALISER_FOR_ENUM(Framework::TimeFormat::Type, byte)
	ADD_SERIALISER_FOR_ENUM(0, Framework::TimeFormat::Default)
	ADD_SERIALISER_FOR_ENUM(1, Framework::TimeFormat::H12)
	ADD_SERIALISER_FOR_ENUM(2, Framework::TimeFormat::H24)
END_SERIALISER_FOR_ENUM(Framework::TimeFormat::Type)

//

DateFormat::Type DateFormat::parse(String const & _string)
{
	if (_string == TXT("MDY")) return DateFormat::MDY;
	if (_string == TXT("DMY")) return DateFormat::DMY;
	if (_string == TXT("YMD")) return DateFormat::YMD;
	return DateFormat::Default;
}

String DateFormat::to_string(Type _type)
{
	if (_type == DateFormat::MDY) return String(TXT("MDY"));
	if (_type == DateFormat::DMY) return String(TXT("DMY"));
	if (_type == DateFormat::YMD) return String(TXT("YMD"));
	return String(TXT("default"));
}

Array<String> DateFormat::get_option_strings()
{
	Array<String> result;
	result.set_size(max(result.get_size(), Default + 1)); result[Default] = LOC_STR(Name(TXT("date format; option; default")));
	result.set_size(max(result.get_size(), MDY + 1)); result[MDY] = LOC_STR(Name(TXT("date format; option; mdy")));
	result.set_size(max(result.get_size(), DMY + 1)); result[DMY] = LOC_STR(Name(TXT("date format; option; dmy")));
	result.set_size(max(result.get_size(), YMD + 1)); result[YMD] = LOC_STR(Name(TXT("date format; option; ymd")));
	return result;
}

//

TimeFormat::Type TimeFormat::parse(String const & _string)
{
	if (_string == TXT("12H")) return TimeFormat::H12;
	if (_string == TXT("24H")) return TimeFormat::H24;
	return TimeFormat::Default;
}

String TimeFormat::to_string(Type _type)
{
	if (_type == TimeFormat::H12) return String(TXT("12H"));
	if (_type == TimeFormat::H24) return String(TXT("24H"));
	return String(TXT("default"));
}

Array<String> TimeFormat::get_option_strings()
{
	Array<String> result;
	result.set_size(max(result.get_size(), Default + 1)); result[Default] = LOC_STR(Name(TXT("time format; option; default")));
	result.set_size(max(result.get_size(), H12 + 1)); result[H12] = LOC_STR(Name(TXT("time format; option; h12")));
	result.set_size(max(result.get_size(), H24 + 1)); result[H24] = LOC_STR(Name(TXT("time format; option; h24")));
	return result;
}

//

DateTimeFormat DateTimeFormat::s_none;

bool DateTimeFormat::load_from_xml(IO::XML::Node const * _node, tchar const * _childName)
{
	if (!_node)
	{
		return true;
	}

	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		date = DateFormat::parse(node->get_string_attribute_or_from_child(TXT("date"), DateFormat::to_string(date)));
		time = TimeFormat::parse(node->get_string_attribute_or_from_child(TXT("time"), TimeFormat::to_string(time)));
	}

	return result;
}

bool DateTimeFormat::save_to_xml(IO::XML::Node * _node, tchar const * _childName) const
{
	if (date != DateFormat::Default ||
		time != TimeFormat::Default)
	{
		if (IO::XML::Node * node = _node->add_node(_childName))
		{
			if (date != DateFormat::Default)
			{
				node->set_string_to_child(TXT("date"), DateFormat::to_string(date));
			}
			if (time != TimeFormat::Default)
			{
				node->set_string_to_child(TXT("time"), TimeFormat::to_string(time));
			}
		}
	}

	return true;
}

bool DateTimeFormat::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, date);
	result &= serialise_data(_serialiser, time);

	return result;
}
