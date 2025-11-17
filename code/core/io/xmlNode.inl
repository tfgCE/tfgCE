template <typename Class>
inline Class Node::get(Class const & _defVal) const
{
	Class value = _defVal;
	value.load_from_xml(this);
	return value;
}

template <typename Class>
inline Class Node::get() const
{
	Class value;
	value.load_from_xml(this);
	return value;
}

//
//	specialisations for parsing
//

template <>
inline int Node::get(int const & _defVal) const
{
	return get_int(_defVal);
}

template <>
inline float Node::get(float const & _defVal) const
{
	return get_float(_defVal);
}

template <>
inline int Node::get() const
{
	return get_int();
}

template <>
inline float Node::get() const
{
	return get_float();
}

