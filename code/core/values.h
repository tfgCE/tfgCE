#pragma once

#include "globalSettings.h"

#include "io\xml.h"

template <typename Type>
inline Type zero()
{
	return (Type)0;
}

template <typename Type>
inline Type one()
{
	return (Type)1;
}

template <typename Type>
inline bool is_positive(Type const& _v)
{
	return _v > zero<Type>();
}

template <typename Type>
inline bool is_negative(Type const& _v)
{
	return _v < zero<Type>();
}

template <typename Type>
inline bool is_more_than_1(Type const& _v)
{
	return _v > one<Type>();
}

template <typename Type>
inline bool is_less_than_1(Type const& _v)
{
	return _v < one<Type>();
}

template <typename Type>
inline bool is_zero(Type const& _v)
{
	return _v == zero<Type>();
}

template <typename Type>
inline Type default_value()
{
	return zero<Type>();
}

// returns true if loaded something, false on error or if not loaded anything - makes it convenient to use multiple types for one value
// may load from node or from value, if you want to be able to load from attribute, override this
template <typename Type>
inline bool load_value_from_xml(REF_ Type & _a, IO::XML::Node const * _node, tchar const * _valueName)
{
	if (_node)
	{
		if (auto * node = _node->first_child_named(_valueName))
		{
			return _a.load_from_xml(node);
		}
	}
	return false;
}

#include "types\nameSpecialisations.h"
#include "types\stringSpecialisations.h"

