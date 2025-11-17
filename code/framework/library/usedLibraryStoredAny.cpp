#include "usedLibraryStoredAny.h"

#include "library.h"
#include "libraryStored.h"

#ifdef AN_DEVELOPMENT
#include "..\debug\previewGame.h"
#endif

using namespace Framework;

UsedLibraryStoredAny::UsedLibraryStoredAny()
: value(nullptr)
{
}

UsedLibraryStoredAny::UsedLibraryStoredAny(UsedLibraryStoredAny const & _source)
: type(_source.type)
, name(_source.name)
, value(_source.value)
{
	if (value)
	{
		value->add_usage();
		type = value->get_library_type();
	}
}

UsedLibraryStoredAny::UsedLibraryStoredAny(LibraryStored* _value)
: value(_value)
{
	if (value)
	{
		value->add_usage();
		type = value->get_library_type();
	}
}

UsedLibraryStoredAny::~UsedLibraryStoredAny()
{
	if (value)
	{
		value->release_usage();
		value = nullptr;
	}
}

UsedLibraryStoredAny& UsedLibraryStoredAny::operator=(UsedLibraryStoredAny const & _source)
{
	set(_source.value);
	name = _source.name;
	type = _source.type;
	return *this;
}

UsedLibraryStoredAny& UsedLibraryStoredAny::operator=(LibraryStored* _value)
{
	set(_value);
	return *this;
}

LibraryName const& UsedLibraryStoredAny::get_name() const
{
	return value ? value->get_name() : name;
}

void UsedLibraryStoredAny::set(LibraryStored* _value)
{
	if (value == _value)
	{
		return;
	}
	if (value)
	{
		value->release_usage();
	}
	value = _value;
	if (value)
	{
		value->add_usage();
		type = value->get_library_type();
		name = value->get_name();
	}
	else
	{
		type = Name::invalid();
		name = LibraryName::invalid();
	}
}

bool UsedLibraryStoredAny::load_from_xml(IO::XML::Node const * _node, tchar const * _attrType, tchar const * _attrName, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_node)
	{
		result &= name.load_from_xml(_node, _attrName, _lc);
		if (_attrType != nullptr)
		{
			type = _node->get_name_attribute(_attrType, type);
		}
		else
		{
			type = Name(_node->get_name());
		}
	}

	return result;
}

bool UsedLibraryStoredAny::load_from_xml(IO::XML::Node const * _node, tchar const * _attrType, tchar const * _attrName)
{
	bool result = true;

	if (_node)
	{
		result &= name.load_from_xml(_node, _attrName);
		if (_attrType != nullptr)
		{
			type = _node->get_name_attribute(_attrType, type);
		}
		else
		{
			type = Name(_node->get_name());
		}
	}

	return result;
}

bool UsedLibraryStoredAny::save_to_xml(IO::XML::Node * _node, tchar const * _attrType, tchar const * _attrName) const
{
	bool result = true;

	if (!type.is_valid() || (!name.is_valid() && !value))
	{
		return true;
	}

	if (_node)
	{
		_node->set_attribute(_attrType, type.to_string());
		if (value)
		{
			// prefer saving actual object's name
			_node->set_attribute(_attrName, value->get_name().to_string());
		}
		else
		{
			_node->set_attribute(_attrName, name.to_string());
		}
	}

	return result;
}

IO::XML::Node* UsedLibraryStoredAny::save_to_xml_child_node(IO::XML::Node * _node, tchar const * _attrType, tchar const * _attrName) const
{
	if (!type.is_valid() || (!name.is_valid() && ! value))
	{
		return nullptr;
	}

	if (_node)
	{
		IO::XML::Node* n;
		if (_attrType != nullptr)
		{
			n = _node->add_node(TXT("libraryStored"));
			n->set_attribute(_attrType, type.to_string());
		}
		else
		{
			n = _node->add_node(type.to_string());
		}
		if (value)
		{
			// prefer saving actual object's name
			n->set_attribute(_attrName, value->get_name().to_string());
		}
		else
		{
			n->set_attribute(_attrName, name.to_string());
		}

		return n;
	}

	return nullptr;
}

bool UsedLibraryStoredAny::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext, bool _mayFail)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (!find(_library))
		{
			if (!_mayFail)
			{
#ifdef AN_DEVELOPMENT
				if (is_preview_game())
				{
					if (does_preview_game_require_info_about_missing_library_stored())
					{
						output(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), type.to_char());
					}
				}
				else
#endif
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (Library::may_ignore_missing_references())
					{
						return result;
					}
#endif
					error(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), type.to_char());
				}
				result = false;
			}
		}
	}
	return result;
}

bool UsedLibraryStoredAny::find(Library* _library)
{
	if (type.is_valid() && name.is_valid())
	{
		set(_library->get_store(type)->find_stored(name));
		return value != nullptr;
	}
	else
	{
		return true;
	}
}

void UsedLibraryStoredAny::set(LibraryName const & _name, Name const & _type)
{
	name = _name;
	type = _type;
	value = nullptr;
}

bool UsedLibraryStoredAny::set_and_find(LibraryName const & _name, Name const& _type, Library* _library)
{
	name = _name;
	type = _type;
	return find(_library);
}

bool UsedLibraryStoredAny::find_or_create(Library* _library)
{
	if (type.is_valid() && name.is_valid())
	{
		set(_library->get_store(type)->find_stored_or_create(name));
		return value != nullptr;
	}
	else
	{
		return true;
	}
}

bool UsedLibraryStoredAny::operator==(LibraryStored const* _value) const
{
	if (value == _value)
	{
		return true;
	}
	if (_value &&
		type == _value->get_library_type() &&
		name == _value->get_name())
	{
		return true;
	}
	if (!_value &&
		!name.is_valid())
	{
		return true;
	}
	return false;
}