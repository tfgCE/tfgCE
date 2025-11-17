#pragma once

#include "name.h"
#include "..\math\math.h"

#include "..\values.h"

template <>
inline Name zero<Name>()
{
	return Name::invalid();
}

template <>
inline bool load_value_from_xml<Name>(REF_ Name & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a = _node->get_name_attribute(_valueName, _a);
			return true;
		}
	}
	return false;
}
