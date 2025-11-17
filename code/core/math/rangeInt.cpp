#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\types\string.h"
#include "..\containers\list.h"

bool RangeInt::load_from_xml(IO::XML::Node const * _node, tchar const * _attr)
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

bool RangeInt::load_from_xml_or_text(IO::XML::Node const * _node, tchar const * _attr)
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

bool RangeInt::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _attr)
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

bool RangeInt::load_from_string(String const & _string)
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
				int base = ParserUtils::parse_int(*iToken0, (min + max) / 2);
				int off = ParserUtils::parse_int(*iToken1, (max - min) / 2);
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
		min = ParserUtils::parse_int(*iToken0, min);
		max = ParserUtils::parse_int(*iToken1, max);
		return true;
	}
	else
	{
		if (!_string.is_empty() && tokens.get_size() == 1)
		{
			// single value
			min = ParserUtils::parse_int(_string, min);
			max = min;
			return true;
		}
	}
	return _string.is_empty();
}

bool RangeInt::save_to_xml(IO::XML::Node* _node, tchar const* _attr) const
{
	bool result = true;

	_node->set_attribute(_attr, to_parsable_string());

	return result;
}

