template <typename Class>
Class const & Param::get_as() const
{
	an_assert(sizeof(Class) <= DATASIZE);
	an_assert(type == type_id<Class>());
	
	return *(plain_cast<Class>(&data));
}

template <typename Class>
Class & Param::access_as()
{
	an_assert(sizeof(Class) <= DATASIZE);
	an_assert(type == type_id_none() ||
		   type == type_id<Class>());
	if (type == type_id_none())
	{
		type = type_id<Class>();
		void* ptr = data;
		RegisteredType::construct(type, ptr);
		if (RegisteredType::is_plain_data(type))
		{
			if (void const * initialValue = RegisteredType::get_initial_value(type))
			{
				memory_copy(ptr, initialValue, RegisteredType::get_size_of(type));
			}
			else
			{
				memory_zero(ptr, RegisteredType::get_size_of(type));
			}
		}
		else
		{
			if (void const * initialValue = RegisteredType::get_initial_value(type))
			{
				RegisteredType::copy(type, ptr, initialValue);
			}
			else
			{
				// constructor should handle this
			}
		}
	}
	return *(plain_cast<Class>(&data));
}
