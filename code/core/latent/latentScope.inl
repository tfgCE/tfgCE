template <typename Class>
Class & Scope::next_var(tchar const * _name)
{
	StackVariables & stackVariables = frame.access_stack_variables();
	frame.increase_current_variable();
	if (stackVariables.is_end_params(frame.get_current_variable()))
	{
		frame.increase_current_variable();
	}
	int count = stackVariables.make_sure_params_ended();
	while (count)
	{
		frame.increase_current_variable();
		--count;
	}
	return stackVariables.push_or_get_var<Class>(frame.get_current_variable(), _name);
}

template <typename Class>
Class & Scope::next_existing_param(tchar const * _name)
{
	StackVariables & stackVariables = frame.access_stack_variables();
	frame.increase_current_variable();
	if (stackVariables.is_begin_params(frame.get_current_variable()))
	{
		frame.increase_current_variable();
	}
	return stackVariables.get_param<Class>(frame.get_current_variable(), _name);
}

template <typename Class>
Class const * Scope::next_optional_param(tchar const * _name)
{
	StackVariables & stackVariables = frame.access_stack_variables();
	if (stackVariables.is_begin_params(frame.get_current_variable()))
	{
		frame.increase_current_variable();
	}
	if (stackVariables.is_end_params(frame.get_current_variable() + 1) || frame.get_current_variable() + 1 >= stackVariables.get_variable_count())
	{
		return nullptr;
	}
	frame.increase_current_variable();
	return stackVariables.get_param_optional<Class>(frame.get_current_variable(), _name);
}


template <typename Class>
void Scope::next_function_param(Class const & _value)
{
	frame.access_stack_variables().push_param(_value);
}
