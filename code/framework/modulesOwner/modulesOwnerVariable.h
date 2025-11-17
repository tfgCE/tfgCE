#pragma once

#include "..\..\core\types\optional.h"
#include "..\..\core\other\simpleVariableStorage.h"

class SimpleVariableStorage;

namespace Framework
{
	struct LibraryName;
	struct LibraryLoadingContext;
	interface_class IModulesOwner;

	/**
	 *	Stores value or gets one from ModulesOwner's variables
	 */
	template<typename Class>
	struct ModulesOwnerVariable
	{
		ModulesOwnerVariable() {}
		ModulesOwnerVariable(Class const & _defValue) { value = _defValue; }

		void clear();

		Class get(IModulesOwner const * _imo) const;
		Class get_with_default(IModulesOwner const * _imo, Class const & _default) const;
		void set(Class const & _value) { value = _value; }
		void set_if_not_set(Class const & _value) { if (! value.is_set()) value = _value; }

		bool is_value_set() const { return value.is_set(); }
		Class const & get_value() const { an_assert(is_value_set()); return value.get(); }

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr); // will auto add *Param to _valueAttr (bone -> boneParam)
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr, tchar const * _variableNameAttr);
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChildAttr); // will auto add *Param to _valueChild
		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChild, tchar const * _variableNameAttr);

		bool is_set() const { return value.is_set() || variableName.is_set(); }

		Optional<Name> const & get_value_param() const { return variableName; }

	protected:
		Optional<Class> value;
		Optional<Name> variableName;

	};

	bool load_group_into(ModulesOwnerVariable<LibraryName> & _libraryName, LibraryLoadingContext & _lc);

};

#include "modulesOwnerVariable.inl"
