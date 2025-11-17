#ifdef AN_CLANG
#include "modulesOwner.h"
#endif

template <typename Class>
void Framework::ModulesOwnerVariable<Class>::clear()
{
	value.clear();
	variableName.clear();
}

template <typename Class>
Class Framework::ModulesOwnerVariable<Class>::get(IModulesOwner const * _imo) const
{
	if (variableName.is_set())
	{
		if (Class const * value = _imo->get_variables().get_existing<Class>(variableName.get()))
		{
			return *value;
		}
	}
	if (value.is_set())
	{
		return value.get();
	}
	if (variableName.is_set())
	{
		error(TXT("\"value\" not defined! (param: %S)"), variableName.get().to_char());
	}
	else
	{
		error(TXT("\"value\" not defined!"));
	}
	return zero<Class>();
}

template <typename Class>
Class Framework::ModulesOwnerVariable<Class>::get_with_default(IModulesOwner const * _imo, Class const & _default) const
{
	if (variableName.is_set())
	{
		if (Class const * value = _imo->get_variables().get_existing<Class>(variableName.get()))
		{
			return *value;
		}
	}
	if (value.is_set())
	{
		return value.get();
	}
	return _default;
}

template <typename Class>
bool Framework::ModulesOwnerVariable<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr)
{
	int charCount = tstrlen(_valueAttr) + 8 + 1;
	allocate_stack_var(tchar, variableNameAttr, charCount);
#ifdef AN_CLANG
	tsprintf(variableNameAttr, charCount, TXT("%SVariable"), _valueAttr);
#else
	tsprintf(variableNameAttr, charCount, TXT("%sVariable"), _valueAttr);
#endif
	return load_from_xml(_node, _valueAttr, variableNameAttr);
}

template <typename Class>
bool Framework::ModulesOwnerVariable<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr, tchar const * _variableNameAttr)
{
	bool result = true;

	if (_node)
	{
		// try to use load_value_from_xml that returns true on successful load and false it there was a problem or nothing was loaded
		Class v;
		if (value.is_set())
		{
			v = value.get();
		}
		if (load_value_from_xml(v, _node, _valueAttr))
		{
			value = v;
		}
		variableName.load_from_xml(_node->get_attribute(_variableNameAttr));
	}

	return result;
}

template <typename Class>
bool Framework::ModulesOwnerVariable<Class>::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChildAttr)
{
	int charCount = tstrlen(_valueChildAttr) + 8 + 1;
	allocate_stack_var(tchar, variableNameAttr, charCount);
#ifdef AN_CLANG
	tsprintf(variableNameAttr, charCount, TXT("%SVariable"), _valueChildAttr);
#else
	tsprintf(variableNameAttr, charCount, TXT("%sVariable"), _valueChildAttr);
#endif
	return load_from_xml_child_node(_node, _valueChildAttr, variableNameAttr);
}

template <typename Class>
bool Framework::ModulesOwnerVariable<Class>::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChild, tchar const * _variableNameAttr)
{
	bool result = true;

	if (_node)
	{
		value.load_from_xml(_node, _valueChild);
		variableName.load_from_xml(_node->get_attribute(_variableNameAttr));
	}

	return result;
}
