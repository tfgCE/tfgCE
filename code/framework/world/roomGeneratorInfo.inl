#pragma once

template <typename Class>
Class * Framework::RoomGeneratorInfo::find(SimpleVariableStorage const & _variables, MeshGenParam<LibraryName> const & _value)
{
	if (Framework::LibraryName const *libraryName = _value.find(_variables))
	{
		return fast_cast<Class>(Framework::Library::get_current()->find_stored(*libraryName, Class::library_type()));
	}
	else
	{
		return nullptr;
	}
}

template <typename Class>
Class * Framework::RoomGeneratorInfo::find(SimpleVariableStorage const & _variables, MeshGenParam<LibraryName> const & _valueSpecific, MeshGenParam<LibraryName> const & _valueGeneral)
{
	if (Framework::LibraryName const *libraryName = _valueSpecific.find(_variables, _valueGeneral))
	{
		return fast_cast<Class>(Framework::Library::get_current()->find_stored(*libraryName, Class::library_type()));
	}
	else
	{
		return nullptr;
	}
}

template <typename Class>
Class * Framework::RoomGeneratorInfo::find(SimpleVariableStorage const & _variables, Name const & _variableName, Class* _fallback)
{
	Framework::LibraryName libraryName = _variables.get_value(_variableName, Framework::LibraryName::invalid());
	Class * found = fast_cast<Class>(Framework::Library::get_current()->find_stored(libraryName, Class::library_type()));
	if (!found) found = _fallback;
	return found;
}

template <typename Class>
Class* Framework::RoomGeneratorInfo::find(SimpleVariableStorage const & _variables, Name const & _variableNameSpecific, Name const & _variableNameGeneral, Class* _fallbackSpecific, Class* _fallbackGeneral)
{
	Framework::LibraryName specificLibraryName = _variables.get_value(_variableNameSpecific, Framework::LibraryName::invalid());
	Framework::LibraryName generalLibraryName = _variables.get_value(_variableNameGeneral, Framework::LibraryName::invalid());
	Class * found = fast_cast<Class>(Framework::Library::get_current()->find_stored(specificLibraryName, Class::library_type()));
	if (!found) found = fast_cast<Class>(Framework::Library::get_current()->find_stored(generalLibraryName, Class::library_type()));
	if (!found) found = _fallbackSpecific;
	if (!found) found = _fallbackGeneral;
	return found;
}
