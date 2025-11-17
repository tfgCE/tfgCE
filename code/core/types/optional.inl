template <typename Element>
Optional<Element>::Optional()
: flags(Flag::None)
{
}

template <typename Element>
Optional<Element>::Optional(NotProvided const & _notProvided)
: flags(Flag::None)
{
}

template <typename Element>
Optional<Element>::Optional(Element const & _element)
: flags(Flag::IsSet | Flag::HasAnyValueStored)
, element(_element)
{
}

template <typename Element>
void Optional<Element>::clear()
{
	clear_flag(flags, Flag::IsSet);
}

template <typename Element>
void Optional<Element>::unset_with(Element const & _element)
{
	clear_flag(flags, Flag::IsSet);
	set_flag(flags, Flag::HasAnyValueStored);
	element = _element;
}

template <typename Element>
void Optional<Element>::set(Element const & _element)
{
	set_flag(flags, Flag::IsSet);
	set_flag(flags, Flag::HasAnyValueStored);
	element = _element;
}

template <typename Element>
void Optional<Element>::set_if_not_set(Element const & _element)
{
	if (!is_flag_set(flags, Flag::IsSet))
	{
		set_flag(flags, Flag::IsSet);
		set_flag(flags, Flag::HasAnyValueStored);
		element = _element;
	}
}

template <typename Element>
void Optional<Element>::set_if_not_set()
{
	if (!is_flag_set(flags, Flag::IsSet))
	{
		set_flag(flags, Flag::IsSet);
		set_flag(flags, Flag::HasAnyValueStored);
	}
}

template <typename Element>
void Optional<Element>::set_if_not_set(Optional<Element> const & _optional)
{
	if (_optional.is_set())
	{
		if (!is_flag_set(flags, Flag::IsSet))
		{
			set_flag(flags, Flag::IsSet);
			set_flag(flags, Flag::HasAnyValueStored);
			element = _optional.get();
		}
	}
}

template <typename Element>
void Optional<Element>::set(Optional<Element> const & _optional)
{
	if (_optional.is_set())
	{
		set(_optional.get());
	}
}

template <typename Element>
Element & Optional<Element>::access()
{
	an_assert(is_flag_set(flags, Flag::IsSet), TXT("element should be set before getting"));
	return element;
}

template <typename Element>
Element const & Optional<Element>::get() const
{
	an_assert(is_flag_set(flags, Flag::IsSet), TXT("element should be set before getting"));
	return element;
}

template <typename Element>
Element const & Optional<Element>::get(Element const & _default) const
{
	return is_flag_set(flags, Flag::IsSet) ? element : _default;
}

template <typename Element>
bool Optional<Element>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		set_to_default(element);
		set_flag(flags, Flag::HasAnyValueStored);
	}
	// should work for all types that implement_ this method, for others - specialise
	if (element.load_from_xml(_node))
	{
		set_flag(flags, Flag::IsSet);
	}
	return true;
}

template <typename Element>
bool Optional<Element>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return false;
	}
	if (!is_flag_set(flags, Flag::HasAnyValueStored))
	{
		set_to_default(element);
		set_flag(flags, Flag::HasAnyValueStored);
	}
	// should work for all types that implement_ this method, for others - specialise
	if (element.load_from_xml(_attr))
	{
		set_flag(flags, Flag::IsSet);
	}
	// this may appear also on compilation!
	//todo_important(TXT("not everything can be loaded from attribute, specialise it like name if it should"));
	return true;
}

template <typename Element>
bool Optional<Element>::load_from_xml(IO::XML::Node const * _node, tchar const * const _attrOrChildName)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(_attrOrChildName))
	{
		return load_from_xml(attr);
	}
	else
	{
		return load_from_xml(_node->first_child_named(_attrOrChildName));
	}
}

template <typename Element>
bool Optional<Element>::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, flags);
	if (flags)
	{
		result &= serialise_data(_serialiser, element);
	}

	return result;
}

template <> bool Optional<bool>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<int>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<float>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<Name>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<bool>::load_from_xml(IO::XML::Attribute const * _attr);
template <> bool Optional<int>::load_from_xml(IO::XML::Attribute const * _attr);
template <> bool Optional<float>::load_from_xml(IO::XML::Attribute const * _attr);
template <> bool Optional<Name>::load_from_xml(IO::XML::Attribute const * _attr);

// Vector2 is defined in vector2.h
// Vector3 is defined in vector3.h
// Range is defined in range.h
// RangeInt is defined in rangeInt.h
// Transform is defined in transform.h
