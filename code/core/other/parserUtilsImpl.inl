#include "../io/xml.h"
#include "../types/string.h"

template <typename Container, typename Contained>
bool ParserUtils::load_multiple_from_xml_into(IO::XML::Node const* _node, tchar const* _attrOrChild, Container& _container)
{
	bool result = true;
	String value = _node->get_string_attribute_or_from_child(_attrOrChild);
	if (!value.is_empty())
	{
		List<String> tokens;
		value.split(String::comma(), tokens);
		for_every(token, tokens)
		{
			an_assert(_container.has_place_left());
			if (!_container.has_place_left())
			{
				error_loading_xml(_node, TXT("can't fit more tokens"));
				result = false;
				break;
			}
			_container.push_back(Contained(token->trim()));
		}
	}
	return true;
}

template <typename SuperType>
bool ParserUtils::parse_using_to_char(IO::XML::Node const* _node, tchar const* _attrOrChild, typename SuperType::Type& _value)
{
	String val = _node->get_string_attribute_or_from_child(_attrOrChild);
	if (!val.is_empty())
	{
		if (parse_using_to_char(val, _value))
		{
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("not recognised value (parsing \"%S\")"), val.to_char());
		}
	}
	return true;
}

template <typename SuperType>
bool ParserUtils::parse_using_to_char(String const& _val, typename SuperType::Type& _value)
{
	if (!_val.is_empty())
	{
		for_count(int, i, SuperType::MAX)
		{
			if (_val == SuperType::to_char((typename SuperType::Type)i))
			{
				_value = (typename SuperType::Type)i;
				return true;
			}
		}
		error(TXT("not recognised value (parsing \"%S\")"), _val.to_char());
		return false;
	}
	return true;
}
