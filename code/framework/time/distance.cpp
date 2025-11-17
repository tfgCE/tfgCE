#include "distance.h"

#include "..\..\core\other\parserUtils.h"

using namespace Framework;

//

Distance const Distance::s_zero = Distance(0);

Distance Distance::from_meters(int _meters)
{
	Distance result;
	result.meters = _meters;
	return result;
}

Distance Distance::from_meters(float _meters)
{
	Distance result;
	result.subMeters = mod(_meters, 1.0f);
	result.meters = TypeConversions::Normal::f_i_closest(_meters - result.subMeters);
	return result;
}

void Distance::add_meters(float _meters)
{
	subMeters += _meters;
	while (subMeters >= 1.0f)
	{
		subMeters -= 1.0f;
		++meters;
	}
}

void Distance::add_meters(int _meters)
{
	meters += _meters;
}

String Distance::get_for_parse() const
{
	String result;
	result = String::printf(TXT("%i.%06i"), meters, clamp((int)(subMeters * 1000000.0f), 0, 999999));
	return result;
}

bool Distance::parse_from(String const & _text)
{
	String distance = _text;
	int dotAt = distance.find_first_of('.');
	if (dotAt == NONE)
	{
		meters = abs(ParserUtils::parse_int(distance));
		subMeters = 0.0f;
	}
	else
	{
		meters = abs(ParserUtils::parse_int(distance.get_left(dotAt)));
		distance = String::printf(TXT("0.%S"), distance.get_sub(dotAt + 1).to_char());
		subMeters = abs(ParserUtils::parse_float(distance));
	}
	return is_ok();
}

bool Distance::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	return load_from_xml(_node);
}

bool Distance::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return true;
	}
	bool result = true;

	if (_node->has_attribute(TXT("meters")))
	{
		if (_node->has_attribute(TXT("subMeters")))
		{
			meters = _node->get_int_attribute(TXT("meters"));
			subMeters = _node->get_float_attribute(TXT("subMeters"));
		}
		else
		{
			float met = _node->get_float_attribute(TXT("meters"));
			*this = from_meters(met);
		}
	}
	else
	{
		parse_from(_node->get_text());
	}

	return result;
}

bool Distance::load_from_xml(IO::XML::Node const* _node, tchar const* _attrName)
{
	return load_from_xml(_node->get_attribute(_attrName));
}

bool Distance::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return true;
	}
	bool result = true;

	result &= parse_from(_attr->get_as_string());

	return result;
}
