#include "memory.h"

#pragma push_macro("new")
#undef new

template <typename Class>
void ObjectHelpers<Class>::call_constructor_on(Class* ptr)
{
	new (ptr)Class();
}

template <typename Class>
void ObjectHelpers<Class>::call_destructor_on(Class* ptr)
{
	ptr->~Class();
}

template <typename Class>
void ObjectHelpers<Class>::call_copy_constructor_on(Class* dest, Class const* from)
{
	new (dest)Class(*from);
}

template <typename Class>
void ObjectHelpers<Class>::copy(Class* dest, Class const* from)
{
	*dest = *from;
}

template <typename Class>
void ObjectHelpers<Class>::copy(Class* dest, Class const* from, Class const* until)
{
	for (Class const* src = from; src != until; ++ dest, ++ src)
	{
		*dest = *src;
	}
}

template <typename Class>
void ObjectHelpers<Class>::copy_reverse(Class* dest, Class const* from, Class const* untilInclusive)
{
	for (Class const* src = from; ; -- dest, -- src)
	{
		*dest = *src;
		if (src == untilInclusive)
		{
			break;
		}
	}
}

template <typename Class>
void ObjectHelpers<Class>::call_constructor_on(Class* from, Class* until)
{
	for (Class* i = from; i != until; ++ i)
	{
		new (i) Class();
	}
}

template <typename Class>
void ObjectHelpers<Class>::call_copy_constructor_on(Class* dest, Class const* from, Class const* until)
{
	for (Class const* src = from; src != until; ++ dest, ++ src)
	{
		new (dest) Class(*src);
	}
}

template <typename Class>
void ObjectHelpers<Class>::call_destructor_on(Class* from, Class* until)
{
	for (Class* i = from; i != until; ++ i)
	{
		i->~Class();
	}
}

#pragma pop_macro("new")
