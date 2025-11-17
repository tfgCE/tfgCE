#include "xmlAttribute.h"

#include "io.h"
#include "xmlDocument.h"
#include "..\other\parserUtils.h"

using namespace IO;
using namespace IO::XML;

Attribute::Attribute(String const & _name)
: name(_name)
, parentNode(nullptr)
, nextAttribute(nullptr)
{
}

Attribute::~Attribute()
{
}

Attribute * const Attribute::load_xml(Interface * const _io, REF_ int & _atLine, REF_ bool & _error)
{
	_io->omit_spaces_and_special_characters(true, &_atLine);

	if (_io->get_last_read_char() == 0)
	{
		// end of file
		return nullptr;
	}

	if (_io->get_last_read_char() == '/' ||
		_io->get_last_read_char() == '>')
	{
		// no more attributes
		return nullptr;
	}

	String attributeName;

	while (tchar const ch = _io->get_last_read_char())
	{
		if (ch == '=' ||
			Document::is_separator(ch))
		{
			break;
		}
		attributeName += ch;

		_io->read_char(&_atLine);
	}

	if (_io->get_last_read_char() == '=')
	{
		_io->omit_spaces_and_special_characters(false, &_atLine);

		if (_io->get_last_read_char() != '"')
		{
			// syntax error
			return nullptr;
		}

		// skip "
		_io->read_char(&_atLine);

		String value;

		while (tchar const ch = _io->get_last_read_char())
		{
			if (ch == '&')
			{
				tchar add = Document::decode_char(_io, &_atLine);
				if (add != 0)
				{
					value += add;
				}
			}
			else if (ch == '"')
			{
				break;
			}
			else
			{
				value += ch;
			}

			_io->read_char(&_atLine);
		}

		// skip "
		_io->read_char(&_atLine);

		Attribute * const attribute = new Attribute(attributeName);
		attribute->set_value(value);
		return attribute;
	}
	else
	{
		// something went wrong
		return nullptr;
	}
}

bool Attribute::save_xml(Interface * const _io) const
{
	_io->write_text(name);
	_io->write_text(TXT("=\""));
	bool convertSpaces = false;
	for (uint i = 0; i < (uint)value.get_length(); ++i)
	{
		String characterEncoded;
		tchar chRaw = value[i];
		if (chRaw == ' ')
		{
			if (i + 1 < (uint)value.get_length() &&
				value[i + 1] == ' ')
			{
				convertSpaces = true;
			}
			if (convertSpaces)
			{
				characterEncoded = Document::encode_char(chRaw);
			}
			else
			{
				characterEncoded = String::space();
			}
		}
		else
		{
			convertSpaces = false;
			characterEncoded = Document::encode_char(chRaw);
		}
		_io->write_text(characterEncoded);
	}
	_io->write_text(TXT("\""));

	return true;
}

String const & Attribute::get_as_string() const
{
	return value;
}

Name const Attribute::get_as_name() const
{
	return Name(value);
}

int Attribute::get_as_int() const
{
	return ParserUtils::parse_int(value);
}

bool Attribute::get_as_bool() const
{
	return ParserUtils::parse_bool(value);
}

float Attribute::get_as_float() const
{
	return ParserUtils::parse_float(value);
}
