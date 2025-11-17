
template <typename Class>
Class Param<Class>::get(WheresMyPoint::IOwner* _parametersProvider, ShouldAllowToFail _allowToFailSilently) const
{
	if (valueParam.is_set())
	{
		Class value;
		if (_parametersProvider->restore_value<Class>(valueParam.get(), value))
		{
			return value;
		}
		if (_allowToFailSilently == DisallowToFail)
		{
			error(TXT("could not find wmp param \"%S\"!"), valueParam.get().to_char());
		}
	}
	if (value.is_set())
	{
		return value.get();
	}
	return zero<Class>();
}

template <typename Class>
Class Param<Class>::get_with_default(WheresMyPoint::IOwner* _parametersProvider, Class const & _defaultValue, ShouldAllowToFail _allowToFailSilently) const
{
	if (valueParam.is_set())
	{
		Class value;
		if (_parametersProvider->restore_value<Class>(valueParam.get(), value))
		{
			return value;
		}
		if (_allowToFailSilently == DisallowToFail)
		{
			error(TXT("could not find wmp param \"%S\"!"), valueParam.get().to_char());
		}
	}
	if (value.is_set())
	{
		return value.get();
	}
	return _defaultValue;
}

template <typename Class>
bool Param<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr)
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
bool Param<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * _valueAttr, tchar const * _valueParamAttr)
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
		valueParam.load_from_xml(_node->get_attribute(_valueParamAttr));
	}

	return result;
}
