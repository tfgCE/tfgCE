template <typename From, typename To>
bool RemapValue<From, To>::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;
	
	if (!_node)
	{
		return result;
	}

	for_every(node, _node->children_named(TXT("map")))
	{
		Entry e;
		RegisteredType::parse_value<From>(node->get_string_attribute(TXT("from")), e.from);
		RegisteredType::parse_value<To>(node->get_string_attribute(TXT("to")), e.to);
		{
			entries.push_back(e);
		}
	}

	sort(entries);

	return result;
}

template <typename From, typename To>
To RemapValue<From, To>::remap(From const& _value) const
{
	if (entries.is_empty())
	{
		return zero<To>();
	}

	for_count(int, i, entries.get_size() - 1)
	{
		Entry const& c = entries[i];
		Entry const& n = entries[i + 1];

		if (_value <= c.from)
		{
			return c.to;
		}

		if (_value <= n.from)
		{
			float pt = (float)(_value - c.from) / (float)(n.from - c.from);
			return c.to * (1.0f - pt) + n.to * pt;
		}
	}

	return entries.get_last().to;
}

template <typename From, typename To>
void RemapValue<From, To>::add(From const& _from, To const& _to)
{
	Entry e;
	e.from = _from;
	e.to = _to;
	entries.push_back(e);
	sort(entries);
}

template <typename From, typename To>
To RemapValue<From, To>::get_last_to() const
{
	return entries.get_last().to;
}
