template <typename Class>
void Frame::add_param(Class const & _value)
{
	access_stack_variables().push_param(_value);
}
