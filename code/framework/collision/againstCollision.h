#pragma once

#include "..\..\core\types\string.h"

namespace AgainstCollision
{
	enum Type
	{
		Movement, // movement collision is used to calculate gradients. due to how it is calculated, it is advised to all shapes to be convex
		Precise, // precise collision should be used only to calculate trace/ray/segment collisions, it might be still advised to divide meshes into smaller parts to speed up things
	};

	inline Type parse(String const & _value, Type _default = Movement)
	{
		if (_value.is_empty()) return _default;
		if (_value == TXT("precise")) return Precise;
		if (_value == TXT("movement")) return Movement;

		error(TXT("not recognised \"against collision\" value of \"%S\""), _value.to_char());
		return Movement;
	}
};
