#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

namespace Axis
{
	enum Type
	{
		None,
		X,
		Y,
		Z,
		Right = X,
		Forward = Y,
		Up = Z,
	};

	inline Type parse_from(String const & _string, Type _default = None)
	{
		if (_string == TXT("x")) return X;
		if (_string == TXT("y")) return Y;
		if (_string == TXT("z")) return Z;
		if (_string == TXT("right")) return Right;
		if (_string == TXT("forward")) return Forward;
		if (_string == TXT("up")) return Up;
		return _default;
	}
};

