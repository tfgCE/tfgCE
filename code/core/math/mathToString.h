#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

#include "..\types\string.h"

template <typename Type>
inline String to_string(Type const & _v)
{
	return _v.to_string();
}

//

template <>
inline String to_string<int>(int const& _a)
{
	return String::printf(TXT("%i"), _a);
}

template <>
inline String to_string<float>(float const& _a)
{
	return String::printf(TXT("%.10f"), _a);
}
