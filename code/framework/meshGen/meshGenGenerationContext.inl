template <typename Class>
inline Class const * GenerationContext::get_parameter(Name const & _name) const
{
	an_assert(!stack.is_empty());
	if (Class const * parameter = stack.get_last()->parameters.get_existing<Class>(_name))
	{
		return parameter;
	}
	return nullptr;
}

template <typename Class>
inline Class const* GenerationContext::get_global_parameter(Name const& _name) const
{
	an_assert(!stack.is_empty());
	if (Class const* parameter = stack.get_first()->parameters.get_existing<Class>(_name))
	{
		return parameter;
	}
	return nullptr;
}

template <typename Class>
inline void GenerationContext::set_parameter(Name const & _name, Class const & _value)
{
	an_assert(!stack.is_empty());
	stack.get_last()->parameters.access<Class>(_name) = _value;
}

template <typename Class>
inline void GenerationContext::set_global_parameter(Name const & _name, Class const & _value)
{
	an_assert(!stack.is_empty());
	stack.get_first()->parameters.access<Class>(_name) = _value;
}

template <typename Class>
inline void GenerationContext::invalidate_parameter(Name const & _name)
{
	an_assert(!stack.is_empty());
	stack.get_last()->parameters.invalidate<Class>(_name);
}

inline bool GenerationContext::has_any_parameter(Name const & _name) const
{
	an_assert(!stack.is_empty());
	TypeId id;
	void const * value;
	return stack.get_last()->parameters.get_existing_of_any_type_id(_name, OUT_ id, OUT_ value);
}

inline bool GenerationContext::get_any_parameter(Name const & _name, OUT_ TypeId & _id, void const * & _value) const
{
	an_assert(!stack.is_empty());
	return stack.get_last()->parameters.get_existing_of_any_type_id(_name, OUT_ _id, OUT_ _value);
}

inline bool GenerationContext::get_parameter(Name const & _name, TypeId _id, void * _into) const
{
	if (void const * param = stack.get_last()->parameters.access_existing(_name, _id))
	{
		RegisteredType::copy(_id, _into, param);
		return true;
	}
	else
	{
		return false;
	}
}

inline bool GenerationContext::has_parameter(Name const & _name, TypeId _id) const
{
	if (void const * param = stack.get_last()->parameters.access_existing(_name, _id))
	{
		return true;
	}
	else
	{
		return false;
	}
}

inline void GenerationContext::set_parameter(Name const & _name, TypeId _id, void const * _value, int _ancestor)
{
	if (_id == type_id_none())
	{
		warn(TXT("parameter \"%S\" of unknown type won't be stored"), _name.to_char());
		// nothing to set
		return;
	}
	an_assert(!stack.is_empty());
	SimpleVariableInfo const & info = stack[max(0, stack.get_size() - 1 - _ancestor)]->parameters.find(_name, _id);
	RegisteredType::copy(_id, info.access_raw(), _value);
}

inline void GenerationContext::set_parameters(SimpleVariableStorage const & _parameters)
{
	an_assert(!stack.is_empty());
	stack.get_last()->parameters.set_from(_parameters);
}

inline bool GenerationContext::convert_parameter(Name const& _name, TypeId _id)
{
	an_assert(!stack.is_empty());
	return stack[max(0, stack.get_size() - 1)]->parameters.convert_existing(_name, _id);
}

inline bool GenerationContext::rename_parameter(Name const& _from, Name const& _to)
{
	an_assert(!stack.is_empty());
	return stack[max(0, stack.get_size() - 1)]->parameters.rename_existing(_from, _to);
}

inline bool GenerationContext::get_any_global_parameter(Name const & _name, OUT_ TypeId & _id, void const * & _value) const
{
	an_assert(!stack.is_empty());
	return stack.get_first()->parameters.get_existing_of_any_type_id(_name, OUT_ _id, OUT_ _value);
}

inline void GenerationContext::set_global_parameter(Name const & _name, TypeId _id, void const * _value)
{
	an_assert(!stack.is_empty());
	SimpleVariableInfo const & info = stack.get_first()->parameters.find(_name, _id);
	RegisteredType::copy(_id, info.access_raw(), _value);
}

