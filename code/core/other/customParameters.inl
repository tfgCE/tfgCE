#pragma once

inline void CustomParameter::set(String const & _value)
{
	if (!is_not_set()) // ignore for s_notSet
	{
		if (asString != _value)
		{
			asString = _value;
			clear_cached_values();
		}
		asParamName = Name::invalid();
	}
}

inline void CustomParameter::set_as_param_name(Name const & _param)
{
	if (!is_not_set()) // ignore for s_notSet
	{
		asParamName = _param;
		clear_cached_values();
	}
}

template <typename Class>
inline Class CustomParameter::get_as() const
{
	if (!is_not_set()) // ignore for s_notSet
	{
		Class result;
		get_cached_or_parse<Class>(REF_ result);
		return result;
	}
	return Class();
}

template <typename Class>
Class CustomParameter::get_as(Class const & _defValue) const
{
	Class result = _defValue;
	if (!is_not_set()) // ignore for s_notSet
	{
		get_cached_or_parse<Class>(REF_ result);
	}
	return result;
}

template <typename Class>
void CustomParameter::parse_into_as(REF_ Class & _into) const
{
	if (! is_not_set()) // ignore for s_notSet
	{
		an_assert(!asParamName.is_valid());
		::RegisteredType::parse_value<Class>(asString, _into);
	}
}

template <typename Class>
void CustomParameter::get_cached_or_parse(REF_ Class & _into) const
{
	lock.acquire_read();
	an_assert(!asParamName.is_valid());
	int typedClass = type_id<Class>();
	if (CustomParameterCachedValueBase** cachedValue = cachedValues.get_existing(typedClass))
	{
		_into = plain_cast<CustomParameterCachedValue<Class>>(*cachedValue)->value;
		lock.release_read();
	}
	else
	{
		lock.release_read();
		::RegisteredType::parse_value<Class>(asString, _into);
		lock.acquire_write();
		if (!cachedValues.get_existing(typedClass)) // make sure it was not added in meantime
		{
			cachedValues[typedClass] = new CustomParameterCachedValue<Class>(_into);
		}
		lock.release_write();
	}
}

// specialisations
// to solve "non initialised value returned"

template <>
inline float CustomParameter::get_as<float>() const
{
	float result = 0.0f;
	if (!is_not_set()) // ignore for s_notSet
	{
		get_cached_or_parse(REF_ result);
	}
	return result;
}

template <>
inline void CustomParameter::get_cached_or_parse<bool>(REF_ bool & _into) const
{
	lock.acquire_read();
	int typedClass = type_id<bool>();
	if (CustomParameterCachedValueBase** cachedValue = cachedValues.get_existing(typedClass))
	{
		_into = plain_cast<CustomParameterCachedValue<bool>>(*cachedValue)->value;
		lock.release_read();
	}
	else
	{
		lock.release_read();
		if (asString.is_empty())
		{
			// parameter exists, that's enough to consider bool true
			_into = true;
		}
		else
		{
			::RegisteredType::parse_value<bool>(asString, _into);
		}
		lock.acquire_write();
		if (!cachedValues.get_existing(typedClass)) // make sure it was not added in meantime
		{
			cachedValues[typedClass] = new CustomParameterCachedValue<bool>(_into);
		}
		lock.release_write();
	}
}
