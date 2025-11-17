
template <typename Class>
void Framework::MeshGeneration::ValueDef<Class>::clear()
{
	value.clear();
	valueParam.clear();
}

template <typename Class>
Class Framework::MeshGeneration::ValueDef<Class>::get(GenerationContext const & _context) const
{
	if (valueParam.is_set())
	{
		if (Class const * value = _context.get_parameter<Class>(valueParam.get()))
		{
			return *value;
		}
	}
	if (value.is_set())
	{
		return value.get();
	}
	if (valueParam.is_set())
	{
		error(TXT("\"value\" not defined! (param: %S)"), valueParam.get().to_char());
	}
	else
	{
		error(TXT("\"value\" not defined!"));
	}
	return default_value<Class>();
}

template <typename Class>
Class Framework::MeshGeneration::ValueDef<Class>::get_with_default(GenerationContext const & _context, Class const & _default) const
{
	if (valueParam.is_set())
	{
		if (Class const * value = _context.get_parameter<Class>(valueParam.get()))
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
bool Framework::MeshGeneration::ValueDef<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr)
{
	int charCount = (int)tstrlen(_valueAttr) + 5 + 1;
	allocate_stack_var(tchar, valueParamAttr, charCount);
#ifdef AN_CLANG
	tsprintf(valueParamAttr, charCount, TXT("%SParam"), _valueAttr);
#else
	tsprintf(valueParamAttr, charCount, TXT("%sParam"), _valueAttr);
#endif
	return load_from_xml(_node, _valueAttr, valueParamAttr);
}

template <typename Class>
bool Framework::MeshGeneration::ValueDef<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr, tchar const * _valueParamAttr)
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
		else
		{
			v = default_value<Class>();
		}
		if (load_value_from_xml(v, _node, _valueAttr))
		{
			value = v;
		}
		valueParam.load_from_xml(_node->get_attribute(_valueParamAttr));
	}

	return result;
}

template <typename Class>
bool Framework::MeshGeneration::ValueDef<Class>::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChildAttr)
{
	int charCount = (int)tstrlen(_valueChildAttr) + 5 + 1;
	allocate_stack_var(tchar, valueParamAttr, charCount);
#ifdef AN_CLANG
	tsprintf(valueParamAttr, charCount, TXT("%SParam"), _valueChildAttr);
#else
	tsprintf(valueParamAttr, charCount, TXT("%sParam"), _valueChildAttr);
#endif
	return load_from_xml_child_node(_node, _valueChildAttr, valueParamAttr);
}

template <typename Class>
bool Framework::MeshGeneration::ValueDef<Class>::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _valueChild, tchar const * _valueParamAttr)
{
	bool result = true;

	if (_node)
	{
		// value is optional type and will handle loading from child with this method
		value.load_from_xml(_node, _valueChild);
		valueParam.load_from_xml(_node->get_attribute(_valueParamAttr));
	}

	return result;
}
