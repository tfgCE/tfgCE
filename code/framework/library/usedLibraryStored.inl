#pragma once

#ifdef AN_CLANG
#include "library.h"
#include "libraryPrepareContext.h"
#include "..\..\core\serialisation\serialiser.h"
#endif

#ifdef AN_DEVELOPMENT
#include "..\framework.h"
#endif

template<typename Class>
::Framework::UsedLibraryStored<Class>::UsedLibraryStored()
: value(nullptr)
{
}

template<typename Class>
::Framework::UsedLibraryStored<Class>::UsedLibraryStored(UsedLibraryStored const & _source)
: name(_source.name)
, value(_source.value)
, locked(_source.locked)
{
	if (value)
	{
		value->add_usage();
	}
}

template<typename Class>
::Framework::UsedLibraryStored<Class>::UsedLibraryStored(Class* _value)
: value(nullptr)
{
	set(_value);
}

template<typename Class>
::Framework::UsedLibraryStored<Class>::UsedLibraryStored(Class const * _value)
: value(nullptr)
{
	set(cast_to_nonconst(_value));
}

template<typename Class>
::Framework::UsedLibraryStored<Class>::~UsedLibraryStored()
{
	if (value)
	{
		value->release_usage();
		value = nullptr;
	}
}

template<typename Class>
::Framework::UsedLibraryStored<Class>& ::Framework::UsedLibraryStored<Class>::operator=(UsedLibraryStored const & _source)
{
	if (locked)
	{
		return *this;
	}
	set(_source.value);
	name = _source.name;
	return *this;
}

template<typename Class>
::Framework::UsedLibraryStored<Class>& ::Framework::UsedLibraryStored<Class>::operator=(Class* _value)
{
	if (locked)
	{
		return *this;
	}
	set(_value);
	return *this;
}

template<typename Class>
::Framework::UsedLibraryStored<Class>& ::Framework::UsedLibraryStored<Class>::operator=(Class const * _value)
{
	if (locked)
	{
		return *this;
	}
	set(cast_to_nonconst(_value));
	return *this;
}

template<typename Class>
void ::Framework::UsedLibraryStored<Class>::set(Class* _value)
{
	if (locked)
	{
		if (!_value || _value->get_name() != name)
		{
			return;
		}
	}
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
		name = value->get_name();
	}
	else
	{
		name = LibraryName::invalid();
	}
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, tchar const * _attrName)
{
	bool result = true;

	if (_node)
	{
		if (IO::XML::Node const * node = _node->first_child_named(_childName))
		{
			result &= load_from_xml(node, _attrName);
		}
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName, tchar const* _attrName, LibraryLoadingContext& _lc)
{
	bool result = true;

	if (_node)
	{
		if (IO::XML::Node const* node = _node->first_child_named(_childName))
		{
			result &= load_from_xml(node, _attrName, _lc);
		}
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (_node)
	{
		result &= name.load_from_xml(_node);
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::load_from_xml(IO::XML::Node const* _node, tchar const* _attrName)
{
	bool result = true;

	if (_node)
	{
		result &= name.load_from_xml(_node, _attrName);
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * _attrName, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_node)
	{
		result &= name.load_from_xml(_node, _attrName, _lc);
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::load_from_xml(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_attr)
	{
		result &= name.load_from_xml(_attr, _lc);
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_node)
	{
		result &= name.load_from_xml(_node, _lc);
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::save_to_xml(IO::XML::Node * _node, tchar const* _attrName) const
{
	bool result = true;

	if (value)
	{
		// prefer saving actual object's name
		_node->set_attribute(_attrName, value->get_name().to_string());
	}
	else if (name.is_valid())
	{
		_node->set_attribute(_attrName, name.to_string());
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::prepare_for_game_find(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel, Name const & _typeNameOverride)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, _atLevel)
	{
		if (!find(_library, _typeNameOverride))
		{
#ifdef AN_DEVELOPMENT
			if (_pfgContext.should_allow_failing())
			{
				return result;
			}
#endif
#ifdef AN_DEVELOPMENT
			if (is_preview_game())
			{
				if (does_preview_game_require_info_about_missing_library_stored())
				{
					output(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), _library->get_store(Class::library_type())->get_type_name().to_char());
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
				error(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), _library->get_store(Class::library_type())->get_type_name().to_char());
			}
			result = false;
		}
	}
	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::prepare_for_game_find_may_fail(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel, Name const & _typeNameOverride)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, _atLevel)
	{
		if (!find_may_fail(_library, _typeNameOverride))
		{
			result = false;
		}
	}
	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::prepare_for_game_find_or_create(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel, Name const & _typeNameOverride)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, _atLevel)
	{
		if (!find_or_create(_library, _typeNameOverride))
		{
#ifdef AN_DEVELOPMENT
			if (is_preview_game())
			{
				if (does_preview_game_require_info_about_missing_library_stored())
				{
					output(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), _library->get_store(Class::library_type())->get_type_name().to_char());
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
				error(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), _library->get_store(Class::library_type())->get_type_name().to_char());
			}
			result = false;
		}
	}
	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::find_if_required(Library* _library, Name const & _typeNameOverride)
{
	if (value == nullptr)
	{
		return find(_library, _typeNameOverride);
	}
	return true;
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::find(Library* _library, Name const & _typeNameOverride)
{
	set(fast_cast<Class>(_library->get_store(_typeNameOverride.is_valid() ? _typeNameOverride : Class::library_type())->find_stored(name)));
	if (value == nullptr && name.is_valid())
	{
#ifdef AN_DEVELOPMENT
		if (is_preview_game())
		{
			if (does_preview_game_require_info_about_missing_library_stored())
			{
				output(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), _library->get_store(_typeNameOverride.is_valid() ? _typeNameOverride : Class::library_type())->get_type_name().to_char());
			}
		}
		else
#endif
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (!Library::may_ignore_missing_references())
#endif
			error(TXT("couldn't find \"%S\" of type \"%S\""), name.to_string().to_char(), _library->get_store(_typeNameOverride.is_valid() ? _typeNameOverride : Class::library_type())->get_type_name().to_char());
		}
	}
	return value != nullptr || ! name.is_valid();
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::find_may_fail(Library* _library, Name const & _typeNameOverride)
{
	set(fast_cast<Class>(_library->get_store(_typeNameOverride.is_valid() ? _typeNameOverride : Class::library_type())->find_stored(name)));
	return value != nullptr || !name.is_valid();
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::find_or_create(Library* _library, Name const & _typeNameOverride)
{
	set(fast_cast<Class>(_library->get_store(_typeNameOverride.is_valid() ? _typeNameOverride : Class::library_type())->find_stored_or_create(name)));
	return value != nullptr || !name.is_valid();
}

template<typename Class>
bool ::Framework::UsedLibraryStored<Class>::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= name.serialise(_serialiser);

	if (_serialiser.is_reading())
	{
		result &= find(Library::get_current());
	}

	return result;
}
