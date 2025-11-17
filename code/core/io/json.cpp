#include "json.h"

#include "..\other\parserUtils.h"

#include "..\other\typeConversions.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace IO;
using namespace JSON;

//

Node& Node::access_sub(tchar const* _sub)
{
	for_every(subValue, subValues)
	{
		if (subValue->name == _sub)
		{
			return *subValue;
		}
	}
	subValues.push_back(Node(_sub));
	return subValues.get_last();
}

Node* Node::add_array_element(tchar const* _name)
{
	for_every(subValue, subValues)
	{
		if (subValue->name == _name)
		{
			an_assert(subValue->isArray);
			subValue->elements.push_back(Node());
			return &subValue->elements.get_last();
		}
	}
	subValues.push_back(Node());
	Node& elementArray = subValues.get_last();
	elementArray.name = _name;
	elementArray.isArray = true;
	elementArray.elements.push_back(Node());
	return &elementArray.elements.get_last();
}

List<Node> const* Node::get_array_elements(tchar const* _name) const
{
	for_every(subValue, subValues)
	{
		if (subValue->name == _name)
		{
			an_assert(subValue->isArray);
			return &subValue->elements;
		}
	}
	return nullptr;
}

void Node::set(tchar const* _attribute, tchar const* _value)
{
	access_sub(_attribute).value = _value;
}

void Node::set(tchar const* _attribute, String const& _value)
{
	access_sub(_attribute).clear_value().value = _value;
}

void Node::set(tchar const* _attribute, bool const _value)
{
	access_sub(_attribute).clear_value().valueBool = _value;
}

void Node::set(tchar const* _attribute, int const _value)
{
	access_sub(_attribute).clear_value().valueInt = _value;
}

void Node::set(tchar const* _attribute, float const _value)
{
	access_sub(_attribute).clear_value().valueFloat = _value;
}

Node & Node::clear_value()
{
	value = String::empty();
	valueBool.clear();
	valueInt.clear();
	valueFloat.clear();
	valueNumber.clear();
	isArray = false;
	elements.clear();
	subValues.clear();

	return *this;
}

Node const * Node::get_sub(tchar const* _sub) const
{
	for_every(subValue, subValues)
	{
		if (subValue->name == _sub)
		{
			return subValue;
		}
	}
	return nullptr;
}

String const& Node::get(tchar const* _attribute) const
{
	if (auto * attr = get_sub(_attribute))
	{
		return attr->value;
	}
	return String::empty();
}

bool Node::get_as_bool(tchar const* _attribute, bool _defaultValue) const
{
	if (auto * attr = get_sub(_attribute))
	{
		return attr->valueBool.get(_defaultValue);
	}
	return _defaultValue;
}

int Node::get_as_int(tchar const* _attribute, int _defaultValue) const
{
	if (auto * attr = get_sub(_attribute))
	{
		if (attr->valueInt.is_set())
		{
			return attr->valueInt.get();
		}
		if (attr->valueFloat.is_set())
		{
			return TypeConversions::Normal::f_i_closest(attr->valueFloat.get());
		}
		return _defaultValue;
	}
	return _defaultValue;
}

float Node::get_as_float(tchar const* _attribute, float _defaultValue) const
{
	if (auto * attr = get_sub(_attribute))
	{
		if (attr->valueFloat.is_set())
		{
			return attr->valueFloat.get();
		}
		if (attr->valueInt.is_set())
		{
			return (float)attr->valueInt.get();
		}
		return _defaultValue;
	}
	return _defaultValue;
}

String Node::to_string() const
{
	String result;
	result += isArray? TXT("[ ") : TXT("{ ");
	bool first = true;
	for_every(subValue, (isArray? elements : subValues))
	{
		if (!first)
		{
			result += TXT(", ");
		}
		first = false;
		if (!isArray)
		{
			result += TXT("\"");
			result += subValue->name;
			result += TXT("\":");
		}
		if (subValue->subValues.is_empty())
		{
			if (subValue->isArray)
			{
				result += subValue->to_string();
			}
			else if (subValue->valueBool.is_set())
			{
				result += subValue->valueBool.get() ? TXT("true") : TXT("false");
			}
			else if (subValue->valueNumber.is_set())
			{
				// if was read as a number, write it in the exact way it was read
				result += subValue->valueNumber.get();
			}
			else if (subValue->valueInt.is_set())
			{
				result += String::printf(TXT("%i"), subValue->valueInt.get());
			}
			else if (subValue->valueFloat.is_set())
			{
				result += String::printf(TXT("%.3f"), subValue->valueFloat.get());
			}
			else
			{
				result += TXT("\"");
				String modifiedValue = subValue->value;
				modifiedValue = modifiedValue.replace(String(TXT("\\")), String(TXT("|!@#|")));
				modifiedValue = modifiedValue.replace(String(TXT("\"")), String(TXT("\\\"")));
				modifiedValue = modifiedValue.replace(String(TXT("\n")), String(TXT("\\n")));
				modifiedValue = modifiedValue.replace(String(TXT("|!@#|")), String(TXT("\\\\")));
				result += modifiedValue;
				result += TXT("\"");
			}
		}
		else
		{
			result += subValue->to_string();
		}
	}
	result += isArray ? TXT(" ]") : TXT(" }");

	return result;
}

bool Node::parse(String const& _source)
{
	name = String::empty();
	clear_value();
	tchar const* at = _source.to_char();
	if (*at == '[')
	{
		isArray = true; // if it starts with [, force it to be array, otherwise we have no place to mark it as an array
	}
	return parse(at);
}

#define NEXT() \
	{ \
		if (*_ch == 0) return false; \
		++_ch; \
		if (*_ch == 0) return false; \
	}

#define EXPECTED(ch) \
	{ if (*_ch != (ch)) return false; }

static bool skip_non_white_space(tchar const*& _ch)
{
	while (true)
	{
		if (*_ch == 0)
		{
			return false;
		}
		if (*_ch == ' ' ||
			*_ch == '\r' || 
			*_ch == '\n')
		{
			++_ch;
			continue;
		}
		break;
	}
	return true;
}

#define SKIP_NON_WHITE_SPACE() \
	{ \
		if (!skip_non_white_space(_ch)) \
		{ \
			return false; \
		} \
	}

static String read_string_quote_to_quote(tchar const*& _ch)
{
	if (*_ch != '\"')
	{
		return String::empty();
	}
	++_ch;
	bool slash = false;
	String result;
	while (*_ch != 0)
	{
		if (*_ch == 'n' && slash)
		{
			result += '\n';
			slash = false;
		}
		else if (*_ch == '\"')
		{
			if (slash)
			{
				result += '\"';
			}
			else
			{
				// this is end!
				break;
			}
			slash = false;
		}
		else if (*_ch == '\\')
		{
			if (slash)
			{
				result += '\\';
				slash = false;
			}
			else
			{
				slash = true;
			}
		}
		else
		{
			if (slash)
			{
				result += '\\';
			}
			else
			{
				result += *_ch;
			}
			slash = false;
		}
		++ _ch;
	}
	if (*_ch != '\"')
	{
		// terminated too early
		return String::empty();
	}
	return result;
}

bool Node::parse(tchar const*& _ch)
{
	SKIP_NON_WHITE_SPACE();
	EXPECTED(isArray ? '[' : '{'); NEXT();

	while (true)
	{
		SKIP_NON_WHITE_SPACE();

		if (!isArray)
		{
			Node child;
			EXPECTED('\"');
			child.name = read_string_quote_to_quote(_ch);
			if (child.name.is_empty())
			{
				return false;
			}
			EXPECTED('\"'); NEXT();
			subValues.push_back(child);
			SKIP_NON_WHITE_SPACE();
			EXPECTED(':'); NEXT();
			SKIP_NON_WHITE_SPACE();
		}
		else
		{
			elements.push_back(Node());
		}
		auto& child = isArray? elements.get_last() : subValues.get_last();

		if (*_ch == '[')
		{
			child.isArray = true;
			if (!child.parse(_ch))
			{
				return false;
			}
		}
		else if (*_ch == '{')
		{
			if (!child.parse(_ch))
			{
				return false;
			}
		}
		else if (*_ch == '\"')
		{
			child.clear_value();
			child.value = read_string_quote_to_quote(_ch);
			EXPECTED('\"'); NEXT();
		}
		else if (*_ch == 't')
		{
			EXPECTED('t'); NEXT();
			EXPECTED('r'); NEXT();
			EXPECTED('u'); NEXT();
			EXPECTED('e'); NEXT();
			child.clear_value();
			child.valueBool = true;
		}
		else if (*_ch == 'f')
		{
			EXPECTED('f'); NEXT();
			EXPECTED('a'); NEXT();
			EXPECTED('l'); NEXT();
			EXPECTED('s'); NEXT();
			EXPECTED('e'); NEXT();
			child.clear_value();
			child.valueBool = false;
		}
		else
		{
			String readNumber;
			if (*_ch == '-')
			{
				readNumber += *_ch;
				NEXT();
			}
			while ((*_ch >= '0' && *_ch <= '9') || *_ch == '.')
			{
				readNumber += *_ch;
				NEXT();
			}
			child.clear_value();
			child.valueNumber = readNumber;
			child.valueInt = ParserUtils::parse_int(readNumber);
			child.valueFloat = ParserUtils::parse_float(readNumber);
		}

		SKIP_NON_WHITE_SPACE();
		if (*_ch == ',')
		{
			NEXT();
			continue;
		}
		else
		{
			// reached end
			break;
		}
	}

	SKIP_NON_WHITE_SPACE();
	EXPECTED(isArray ? ']' : '}'); NEXT();

	return true;
}