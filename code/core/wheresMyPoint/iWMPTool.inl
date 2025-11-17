template <typename Class>
Class & Value::access_as()
{
	an_assert(type == type_id<Class>());
	an_assert(RegisteredType::is_plain_data(type));
	an_assert(sizeof(Class) <= MAX_SIZE);
	if (type != type_id<Class>())
	{
		error_stop(TXT("invalid type!"));
	}
	return *((Class*)&valueRaw);
}

template <typename Class>
Class Value::get_as() const
{
	Class result;
	if (type == type_id<Class>())
	{
		an_assert(sizeof(Class) <= MAX_SIZE);
		an_assert(RegisteredType::is_plain_data(type));
		result = *((Class*)&valueRaw);
	}
	else
	{
		set_to_default<Class>(result);
	}
	return result;
}

template <typename Class>
void Value::set(Class const & _value)
{
	an_assert(sizeof(Class) <= MAX_SIZE);
	type = type_id<Class>();
	an_assert(RegisteredType::is_plain_data(type));
	*((Class*)&valueRaw) = _value;
}

//

template <typename Class>
bool ToolSet::update(Class & _value, Context & _context) const
{
	bool result = true;
	Value value;
	value.set<Class>(_value);
	result &= update(value, _context);
	_value = value.get_as<Class>();
	return result;
}

//

template <typename Class>
bool ITool::process_value(OUT_ Class & _value, Class const & _defaultValue, ToolSet const & _toolSet, Context & _context, tchar const * const _errorOnValueType)
{
	bool result = true;

	_value = _defaultValue;
	Value processValue;
	processValue.set(_value);
	if (_toolSet.update(processValue, _context))
	{
		if (processValue.get_type() == type_id<Class>())
		{
			_value = processValue.get_as<Class>();
		}
		else
		{
			if (_errorOnValueType)
			{
				error(_errorOnValueType);
			}
			else
			{
				error(TXT("expected different variable type"));
			}
			result = false;
		}
	}
	else
	{
		result = false;
	}

	return result;
}

