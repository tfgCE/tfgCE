#include "math.h"

#include "..\containers\list.h"
#include "..\io\xml.h"
#include "..\other\parserUtils.h"
#include "..\types\string.h"
#include "..\serialisation\serialiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

bool Range::load_from_xml(IO::XML::Node const * _node, tchar const * _attr)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(_attr))
	{
		return load_from_string(attr->get_as_string());
	}
	else
	{
		return true;
	}
}

bool Range::load_from_xml_or_text(IO::XML::Node const * _node, tchar const * _attr)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(_attr))
	{
		return load_from_string(attr->get_as_string());
	}
	else
	{
		return load_from_string(_node->get_text());
	}
}

bool Range::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _attr)
{
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		if (_attr)
		{
			return load_from_xml(child, _attr);
		}
		else
		{
			return load_from_string(child->get_internal_text());
		}
	}
	else
	{
		return true;
	}
}

bool Range::load_from_attribute_or_from_child(IO::XML::Node const* _node, tchar const* _name)
{
	if (IO::XML::Node const* child = _node->first_child_named(_name))
	{
		return load_from_string(child->get_internal_text());
	}
	else
	{
		return load_from_xml(_node, _name); // attribute
	}
}

bool Range::load_from_string(String const & _string)
{
	if (_string == TXT("empty"))
	{
		*this = empty;
		return true;
	}
	List<String> tokens;
	_string.split(String::comma(), tokens);
	if (tokens.get_size() == 1)
	{
		tokens.clear();
		_string.split(String(TXT("to")), tokens);
		if (tokens.get_size() == 1)
		{
			tokens.clear();
			_string.split(String(TXT("+-")), tokens);
			if (tokens.get_size() == 2)
			{
				List<String>::Iterator iToken0 = tokens.begin();
				List<String>::Iterator iToken1 = iToken0; ++iToken1;
				float base = ParserUtils::parse_float(*iToken0, (min + max) * 0.5f);
				float off = ParserUtils::parse_float(*iToken1, (max - min) * 0.5f);
				min = base - off;
				max = base + off;
				return true;
			}
		}
	}
	if (tokens.get_size() == 2)
	{
		List<String>::Iterator iToken0 = tokens.begin();
		List<String>::Iterator iToken1 = iToken0; ++iToken1;
		min = ParserUtils::parse_float(*iToken0, min);
		max = ParserUtils::parse_float(*iToken1, max);
		return true;
	}
	else
	{
		if (! _string.is_empty() && tokens.get_size() == 1)
		{
			// single value
			min = ParserUtils::parse_float(_string, min);
			max = min;
			return true;
		}
	}
	return _string.is_empty();
}

bool Range::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, min);
	result &= serialise_data(_serialiser, max);
	return result;
}

String Range::to_debug_string() const
{
	return String::printf(TXT("min:%.10f max:%.10f"), min, max);
}

String Range::to_percent_string(int _decimals) const
{
	_decimals = ::max(0, _decimals);
	String format = String::printf(TXT("%%.%if"), _decimals);
	String minS = String::printf(format.to_char(), ::clamp(min * 100.0f, 0.0f, 100.0f));
	String maxS = String::printf(format.to_char(), ::clamp(max * 100.0f, 0.0f, 100.0f));
	if (minS != maxS)
	{
		return minS + TXT("% - ") + maxS + TXT("%");
	}
	else
	{
		return minS;
	}
}

String Range::to_string(int _decimals, tchar const * _suffix) const
{
	_decimals = ::max(0, _decimals);
	String format = String::printf(TXT("%%.%if"), _decimals);
	String minS = String::printf(format.to_char(), min);
	String maxS = String::printf(format.to_char(), max);
	if (_suffix)
	{
		if (minS != maxS)
		{
			return minS + _suffix + TXT(" - ") + maxS + _suffix;
		}
		else
		{
			return minS + _suffix;
		}
	}
	else
	{
		if (minS != maxS)
		{
			return minS + TXT(" - ") + maxS;
		}
		else
		{
			return minS;
		}
	}
}

float Range::transform(float _value, Range const & _from, Range const & _to)
{
	float pt = _from.get_pt_from_value(_value);
	return _to.get_at(pt);
}

float Range::transform_clamp(float _value, Range const & _from, Range const & _to)
{
	float pt = ::clamp(_from.get_pt_from_value(_value), 0.0f, 1.0f);
	return _to.get_at(pt);
}
