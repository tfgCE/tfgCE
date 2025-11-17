template<typename Class>
RefCountObjectPtr<Class>::RefCountObjectPtr()
: value(nullptr)
{
}

template<typename Class>
RefCountObjectPtr<Class>::RefCountObjectPtr(RefCountObjectPtr const & _source)
: value(_source.value)
{
	if (value)
	{
		value->add_ref();
	}
}

template<typename Class>
RefCountObjectPtr<Class>::RefCountObjectPtr(Class const * _value)
: value(cast_to_nonconst(_value))
{
	if (value)
	{
		value->add_ref();
	}
}

template<typename Class>
RefCountObjectPtr<Class>::~RefCountObjectPtr()
{
	if (value)
	{
		value->release_ref();
	}
}

template<typename Class>
RefCountObjectPtr<Class>& RefCountObjectPtr<Class>::operator=(RefCountObjectPtr const & _source)
{
	if (value == _source.value)
	{
		return *this;
	}
	if (value)
	{
		value->release_ref();
	}
	value = _source.value;
	if (value)
	{
		value->add_ref();
	}
	return *this;
}

template<typename Class>
RefCountObjectPtr<Class>& RefCountObjectPtr<Class>::operator=(Class const * _value)
{
	if (value == _value)
	{
		return *this;
	}
	if (value)
	{
		value->release_ref();
	}
	value = cast_to_nonconst(_value);
	if (value)
	{
		value->add_ref();
	}
	return *this;
}
