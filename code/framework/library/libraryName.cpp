#include "libraryName.h"

#include "libraryLoadingContext.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\serialisation\serialiser.h"

using namespace Framework;

LibraryName LibraryName::c_invalid;

void LibraryName::initialise_static()
{
	c_invalid = LibraryName(); // reinit so we have it properly setup with name::invalid
}

void LibraryName::close_static()
{
}

LibraryName::LibraryName()
: group(Name::invalid())
, name(Name::invalid())
{
}

LibraryName::LibraryName(String const & _name)
{
	setup_with_string(_name);
}
		
LibraryName::LibraryName(String const & _group, String const & _name)
: group(_group)
, name(_name)
{
	check_if_valid_characters_used();
}
		
LibraryName::LibraryName(Name const & _group, Name const & _name)
: group(_group)
, name(_name)
{
	check_if_valid_characters_used();
}

LibraryName LibraryName::from_string(String const & _full)
{
	LibraryName ln;
	ln.setup_with_string(_full);
	return ln;
}

LibraryName LibraryName::from_string_with_group(String const & _full, Name const & _group)
{
	LibraryName ln;
	ln.setup_with_string(_full, _group);
	return ln;
}

LibraryName LibraryName::from_xml(IO::XML::Node const * _node, String const & _attrName, LibraryLoadingContext & _lc)
{
	LibraryName ln;
	ln.setup_with_string_loading(_node->get_string_attribute(_attrName), _lc);
	return ln;
}

bool LibraryName::is_valid() const
{
	return name.is_valid();
} 

bool LibraryName::load_from_xml(IO::XML::Node const * _node, String const & _attrName, LibraryLoadingContext & _lc)
{
	if (_node->has_attribute(_attrName.to_char()))
	{
		return setup_with_string_loading(_node->get_string_attribute(_attrName), _lc);
	}
	else
	{
		return true;
	}
}

bool LibraryName::load_from_xml(IO::XML::Node const * _node, tchar const * _attrName, LibraryLoadingContext & _lc)
{
	if (_attrName == nullptr)
	{
		return setup_with_string_loading(_node->get_text(), _lc);
	}
	else if (_node->has_attribute(_attrName))
	{
		return setup_with_string_loading(_node->get_string_attribute(_attrName), _lc);
	}
	else
	{
		return true;
	}
}

bool LibraryName::load_from_xml(IO::XML::Node const * _node, tchar const * _attrName)
{
	if (_attrName == nullptr)
	{
		return setup_with_string(_node->get_text());
	}
	else if (_node->has_attribute(_attrName))
	{
		return setup_with_string(_node->get_string_attribute(_attrName));
	}
	else
	{
		return true;
	}
}

bool LibraryName::load_from_xml(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	return setup_with_string_loading(_attr->get_value(), _lc);
}

bool LibraryName::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	return setup_with_string_loading(_node->get_text(), _lc);
}

bool LibraryName::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, LibraryLoadingContext & _lc)
{
	if (IO::XML::Node const * childNode = _node->first_child_named(_childNode))
	{
		if (IO::XML::Attribute const * attr = childNode->get_attribute(TXT("name")))
		{
			return setup_with_string_loading(attr->get_value(), _lc);
		}
		else
		{
			return setup_with_string_loading(childNode->get_text(), _lc);
		}
	}
	else
	{
		return true;
	}
}

bool LibraryName::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _attrName, LibraryLoadingContext & _lc)
{
	if (IO::XML::Node const * childNode = _node->first_child_named(_childNode))
	{
		if (tstrlen(_attrName) > 0)
		{
			return setup_with_string_loading(childNode->get_string_attribute(_attrName), _lc);
		}
		else
		{
			return setup_with_string_loading(childNode->get_text(), _lc);
		}
	}
	else
	{
		return true;
	}
}

bool LibraryName::setup_with_string(String const & _libraryName, LibraryLoadingContext & _lc)
{
	return setup_with_string_loading(_libraryName, _lc);
}

bool LibraryName::setup_with_string_loading(String const & _name, LibraryLoadingContext & _lc)
{
	return setup_with_string(_name, _lc.get_current_group());
}

bool LibraryName::setup_with_string(String const & _name, Name _group)
{
	int separatorAt = _name.find_last_of(separator()[0]);
	if (separatorAt >= 0 && separatorAt < _name.get_length())
	{
		group = Name(_name.get_sub(0, separatorAt));
		if (separatorAt < _name.get_length() - 1)
		{
			name = Name(_name.get_sub(separatorAt + 1));
		}
		else
		{
			name = Name::invalid();
			check_if_valid_characters_used();
			return false;
		}
	}
	else
	{
		if (!_name.is_empty())
		{
			group = _group;
			name = Name(_name);
		}
		else
		{
			group = Name::invalid();
			name = Name::invalid();
		}
	}
	return check_if_valid_characters_used();
}

bool LibraryName::check_if_valid_characters_used() const
{
	bool result = true;
	if (group.is_valid())
	{
		tchar const *ch = group.to_char();
		while (*ch != 0)
		{
			if (
#ifndef AN_CLANG
				*ch < 0 ||
#endif
				*ch > 255)
			{
				error(TXT("LibraryName %S has invalid character in group"), to_string().to_char());
				result = false;
				break;
			}
			++ch;
		}
	}
	if (name.is_valid())
	{
		tchar const *ch = name.to_char();
		while (*ch != 0)
		{
			if (
#ifndef AN_CLANG
				*ch < 0 ||
#endif
				*ch > 255)
			{
				error(TXT("LibraryName %S has invalid character in name"), to_string().to_char());
				result = false;
				break;
			}
			++ch;
		}
	}
	return result;
}

bool LibraryName::serialise(Serialiser & _serialiser)
{
	bool result = true;
	
	if (_serialiser.is_using_name_library())
	{
		// much more light weight
		result &= serialise_data(_serialiser, group);
		result &= serialise_data(_serialiser, name);
	}
	else
	{
		// note: work as with pure ascii chars - to reduce size
		if (_serialiser.is_writing())
		{
			String const & groupAsString = group.to_string();
			String const & nameAsString = name.to_string();
			int dataSize = 0;
			ARRAY_STACK(char, asChars, max(groupAsString.get_data().get_size(), nameAsString.get_data().get_size()));

			dataSize = groupAsString.get_data().get_size() - 1; // skip 0
			for_every(ch, groupAsString.get_data())
			{
				asChars.push_back((char)*ch);
			}
			result &= serialise_data(_serialiser, dataSize);
			result &= serialise_plain_data(_serialiser, asChars.get_data(), dataSize);

			asChars.clear();
			dataSize = nameAsString.get_data().get_size() - 1; // skip 0
			for_every(ch, nameAsString.get_data())
			{
				asChars.push_back((char)*ch);
			}
			result &= serialise_data(_serialiser, dataSize);
			result &= serialise_plain_data(_serialiser, asChars.get_data(), dataSize);
		}

		if (_serialiser.is_reading())
		{
			{
				int dataSize = 0;
				result &= serialise_data(_serialiser, dataSize);
				ARRAY_STACK(char, asChars, dataSize);
				asChars.set_size(dataSize);
				result &= serialise_plain_data(_serialiser, asChars.get_data(), dataSize);
				ARRAY_STACK(tchar, asTChars, dataSize + 1);
				for_every(ch, asChars)
				{
					asTChars.push_back((tchar)(*ch));
				}
				asTChars.push_back(0);
				group = Name(asTChars.get_data());
			}

			{
				int dataSize = 0;
				result &= serialise_data(_serialiser, dataSize);
				ARRAY_STACK(char, asChars, dataSize);
				asChars.set_size(dataSize);
				result &= serialise_plain_data(_serialiser, asChars.get_data(), dataSize);
				ARRAY_STACK(tchar, asTChars, dataSize + 1);
				for_every(ch, asChars)
				{
					asTChars.push_back((tchar)(*ch));
				}
				asTChars.push_back(0);
				name = Name(asTChars.get_data());
			}
		}
	}

	return result;
}
