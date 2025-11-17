#pragma once

#include "debug\debug.h"

/**
 *	This code is to provide default values to any kind of object
 */

template <typename Class>
inline void set_to_default(Class & _object)
{
	todo_important(TXT("specialise!"));
}

template <>
inline void set_to_default<bool>(bool & _object)
{
	_object = false;
}

template <>
inline void set_to_default<float>(float & _object)
{
	_object = 0.0f;
}

template <>
inline void set_to_default<int>(int & _object)
{
	_object = 0;
}
