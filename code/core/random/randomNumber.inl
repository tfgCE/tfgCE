
template<typename Class, typename ClassUtils>
NumberBase<Class, ClassUtils>::NumberBase(Class const & _singleValue)
{
	sections.push_back(Section().value(_singleValue));
}

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::get() const
{
	Random::Generator generator;
	return get(generator);
}

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::get(Generator & _generator, bool _nonRandomised) const
{
	if (sections.is_empty())
	{
		return ClassUtils::zero();
	}

	int sectionIdx = 0;
	if (_nonRandomised)
	{
		sectionIdx = NONE;
		float bestChance = 0.0f;
		for_every(section, sections)
		{
			if (section->chance > bestChance)
			{
				sectionIdx = for_everys_index(section);
				bestChance = section->chance;
			}
		}
		sectionIdx = clamp(sectionIdx, 0, sections.get_size() - 1);
	}
	else
	{
		float sumChances = 0.0f;
		for_every(section, sections)
		{
			sumChances += section->chance;
		}
		float chance = _generator.get_float(0.0f, sumChances);
		for_every(section, sections)
		{
			if (section->chance > 0.0f)
			{
				chance -= section->chance;
				if (chance <= 0.0f)
				{
					break;
				}
			}
			++sectionIdx;
		}
		sectionIdx = clamp(sectionIdx, 0, sections.get_size() - 1);
	}

	return sections[sectionIdx].get(_generator, _nonRandomised);
}

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::get_min() const
{
	Class currentMin = ClassUtils::zero();
	for_every(section, sections)
	{
		if (for_everys_index(section) == 0)
		{
			currentMin = section->get_min();
		}
		else
		{
			Class newMin = section->get_min();
			if (newMin < currentMin)
			{
				currentMin = newMin;
			}
		}
	}
	return currentMin;
}

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::get_max() const
{
	Class currentMax = ClassUtils::zero();
	for_every(section, sections)
	{
		if (for_everys_index(section) == 0)
		{
			currentMax = section->get_max();
		}
		else
		{
			Class newMax = section->get_max();
			if (currentMax < newMax)
			{
				currentMax = newMax;
			}
		}
	}
	return currentMax;
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return true;
	}

	bool result = true;

	sections.clear();

	if (_node->has_attribute(TXT("range")) ||
		_node->has_attribute(TXT("value")) ||
		!_node->has_children())
	{
		Section section;
		if (section.load_from_xml(_node))
		{
			sections.push_back(section);
		}
	}
	for_every(node, _node->children_named(TXT("choose")))
	{
		Section section;
		if (section.load_from_xml(node))
		{
			sections.push_back(section);
		}
	}

	return result;
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::load_from_xml(IO::XML::Attribute const * _attr)
{
	if (!_attr)
	{
		return true;
	}

	bool result = true;

	sections.clear();
	Section section;
	Section::load_range(_attr, REF_ section.valueRangeMin, REF_ section.valueRangeMax);
	sections.push_back(section);

	return result;
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::load_from_xml(IO::XML::Node const * _node, tchar const * _attrOrChildName)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(_attrOrChildName))
	{
		return load_from_xml(attr);
	}
	else if (IO::XML::Node const * node = _node->first_child_named(_attrOrChildName))
	{
		return load_from_xml(node);
	}
	else
	{
		return true;
	}
}

template<typename Class, typename ClassUtils>
void NumberBase<Class, ClassUtils>::add(Class const& _value)
{
	add(Section().value(_value));
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, sections);

	return result;
}

template<typename Class, typename ClassUtils>
int NumberBase<Class, ClassUtils>::estimate_options_count() const
{
	int options = 0;
	for_every(s, sections)
	{
		Class d = s->valueRangeMax - s->valueRangeMin;
		if (!ClassUtils::is_zero(s->round))
		{
			options += 1 + max(0, d / s->round);
		}
		else
		{
			options += 1 + max(0, (int)d);
		}
	}
	return options;
}

//

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::Section::get(Generator & _generator, bool _nonRandomised) const
{
	if (_nonRandomised)
	{
		Class rangeLength = valueRangeMax - valueRangeMin;
		Class value = valueRangeMin + ClassUtils::half_of(rangeLength);
		value = process(value);
		return value;
	}
	else
	{
		Class value = ClassUtils::get_random(_generator, valueRangeMin, valueRangeMax);
		value = process(value);
		return value;
	}
}

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::Section::get_min() const
{
	Class value = valueRangeMin;
	value = process(value);
	return value;
}

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::Section::get_max() const
{
	Class value = valueRangeMax;
	value = process(value);
	return value;
}

template<typename Class, typename ClassUtils>
Class NumberBase<Class, ClassUtils>::Section::process(Class const & _value) const
{
	Class value = _value;
	if (!ClassUtils::is_zero(round))
	{
		// half is used here to round up to closest full value
		Class roundHalf = ClassUtils::half_of(round);
		value = value - (mod((value + roundHalf), round) - roundHalf);
	}
	value = value + offset;
	if (clampRangeIsSet &&
		clampRangeMin <= clampRangeMax)
	{
		value = clamp(value, clampRangeMin, clampRangeMax);
	}
	return value;
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::Section::load_value(IO::XML::Node const * _node, tchar const * _attr, REF_ Class & _value)
{
	if (IO::XML::Attribute const * attr = _node->get_attribute(_attr))
	{
		_value = ClassUtils::parse_from(attr->get_as_string(), _value);
		return true;
	}
	else
	{
		return false;
	}
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::Section::load_range(IO::XML::Node const * _node, tchar const * _attr, REF_ Class & _min, REF_ Class & _max)
{
	if (_attr)
	{
		return load_range(_node->get_attribute(_attr), REF_ _min, REF_ _max);
	}
	else
	{
		return load_range(_node->get_text(), REF_ _min, REF_ _max);
	}
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::Section::load_range(IO::XML::Attribute const * _attr, REF_ Class & _min, REF_ Class & _max)
{
	if (_attr)
	{
		return load_range(_attr->get_as_string(), REF_ _min, REF_ _max);
	}
	else
	{
		return false;
	}
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::Section::load_range(String const & text, REF_ Class & _min, REF_ Class & _max)
{
	List<String> tokens;
	text.split(String::comma(), tokens);
	if (tokens.get_size() == 1)
	{
		tokens.clear();
		text.split(String(TXT("to")), tokens);
		if (tokens.get_size() == 1)
		{
			tokens.clear();
			text.split(String(TXT("+-")), tokens);
			if (tokens.get_size() == 2)
			{
				List<String>::Iterator iToken0 = tokens.begin();
				List<String>::Iterator iToken1 = iToken0; ++iToken1;
				Class base = ClassUtils::parse_from(*iToken0, ClassUtils::zero());
				Class off = ClassUtils::parse_from(*iToken1, ClassUtils::zero());
				_min = base - off;
				_max = base + off;
				return true;
			}
		}
	}
	if (tokens.get_size() == 2)
	{
		List<String>::Iterator iToken0 = tokens.begin();
		List<String>::Iterator iToken1 = iToken0; ++iToken1;
		_min = ClassUtils::parse_from(*iToken0, _min);
		_max = ClassUtils::parse_from(*iToken1, _max);
		return true;
	}
	else
	{
		if (!text.is_empty() && tokens.get_size() == 1)
		{
			// single value
			_min = ClassUtils::parse_from(text, _min);
			_max = _min;
			return true;
		}
	}
	return false;
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::Section::load_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return true;
	}

	bool result = true;

	if (_node->has_attribute(TXT("range")) ||
		_node->has_attribute(TXT("value")))
	{
		chance = _node->get_float_attribute(TXT("chance"), chance);
		chance = _node->get_float_attribute(TXT("probCoef"), chance);

		load_value(_node, TXT("round"), round);
		load_value(_node, TXT("offset"), offset);
		if (_node->has_attribute(TXT("range")))
		{
			load_range(_node, TXT("range"), REF_ valueRangeMin, REF_ valueRangeMax);
		}
		else if (_node->has_attribute(TXT("value")))
		{
			load_range(_node, TXT("value"), REF_ valueRangeMin, REF_ valueRangeMax);
		}
		else if (!_node->has_children())
		{
			result = load_range(_node, nullptr, REF_ valueRangeMin, REF_ valueRangeMax);
		}
		clampRangeIsSet = load_range(_node, TXT("clamp"), REF_ clampRangeMin, REF_ clampRangeMax);
	}
	else
	{
		// empty
		result = false;
	}

	return result;
}

template<typename Class, typename ClassUtils>
bool NumberBase<Class, ClassUtils>::Section::serialise(Serialiser & _serialiser)
{
	bool result = true;

	result &= serialise_data(_serialiser, chance);
	result &= serialise_data(_serialiser, valueRangeMin);
	result &= serialise_data(_serialiser, valueRangeMax);
	result &= serialise_data(_serialiser, clampRangeIsSet);
	result &= serialise_data(_serialiser, clampRangeMin);
	result &= serialise_data(_serialiser, clampRangeMax);
	result &= serialise_data(_serialiser, round);
	result &= serialise_data(_serialiser, offset);

	return result;
}

