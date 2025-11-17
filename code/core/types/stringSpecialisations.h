#pragma once

#include "string.h"
#include "..\math\math.h"

#include "..\values.h"

template <>
inline String zero<String>()
{
	return String::empty();
}

template <>
inline bool load_value_from_xml<String>(REF_ String& _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto attr = _node->get_attribute(_valueName))
		{
			_a = _node->get_string_attribute(_valueName, _a);
			return true;
		}
	}
	return false;
}
