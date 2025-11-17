template <typename Class>
void TeaForGodEmperor::WeaponPartStatInfo<Class>::set_auto_positive_is_better()
{
	if (value.is_set())
	{
		if (is_positive(value.get()))
		{
			how = WeaponStatAffection::IncBetter;
		}
		else if (is_negative(value.get()))
		{
			how = WeaponStatAffection::DecWorse;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartStatInfo<Class>::set_auto_negative_is_better()
{
	if (value.is_set())
	{
		if (is_positive(value.get()))
		{
			how = WeaponStatAffection::IncWorse;
		}
		else if (is_negative(value.get()))
		{
			how = WeaponStatAffection::DecBetter;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartStatInfo<Class>::set_auto_coef_more_than_1_is_better()
{
	if (value.is_set())
	{
		if (is_more_than_1(value.get()))
		{
			how = WeaponStatAffection::IncBetter;
		}
		else if (is_less_than_1(value.get()))
		{
			how = WeaponStatAffection::DecWorse;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartStatInfo<Class>::set_auto_coef_less_than_1_is_better()
{
	if (value.is_set())
	{
		if (is_more_than_1(value.get()))
		{
			how = WeaponStatAffection::IncWorse;
		}
		else if (is_less_than_1(value.get()))
		{
			how = WeaponStatAffection::DecBetter;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartStatInfo<Class>::set_auto_special()
{
	if (value.is_set())
	{
		how = WeaponStatAffection::Special;
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartStatInfo<Class>::set_auto_set()
{
	if (value.is_set())
	{
		how = WeaponStatAffection::Set;
	}
}

//--

template <typename Class>
void TeaForGodEmperor::WeaponPartTypeStatInfo<Class>::set_auto_positive_is_better()
{
	if (value.is_set())
	{
		if (is_positive(value.get_min()))
		{
			how = WeaponStatAffection::IncBetter;
		}
		else if (is_negative(value.get_max()))
		{
			how = WeaponStatAffection::DecWorse;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartTypeStatInfo<Class>::set_auto_negative_is_better()
{
	if (value.is_set())
	{
		if (is_positive(value.get_min()))
		{
			how = WeaponStatAffection::IncWorse;
		}
		else if (is_negative(value.get_max()))
		{
			how = WeaponStatAffection::DecBetter;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartTypeStatInfo<Class>::set_auto_coef_more_than_1_is_better()
{
	if (value.is_set())
	{
		if (is_more_than_1(value.get_min()))
		{
			how = WeaponStatAffection::IncBetter;
		}
		else if (is_less_than_1(value.get_max()))
		{
			how = WeaponStatAffection::DecWorse;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartTypeStatInfo<Class>::set_auto_coef_less_than_1_is_better()
{
	if (value.is_set())
	{
		if (is_more_than_1(value.get_min()))
		{
			how = WeaponStatAffection::IncWorse;
		}
		else if (is_less_than_1(value.get_max()))
		{
			how = WeaponStatAffection::DecBetter;
		}
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartTypeStatInfo<Class>::set_auto_special()
{
	if (value.is_set())
	{
		how = WeaponStatAffection::Special;
	}
}

template <typename Class>
void TeaForGodEmperor::WeaponPartTypeStatInfo<Class>::set_auto_set()
{
	if (value.is_set())
	{
		how = WeaponStatAffection::Set;
	}
}

//--

template <typename Class>
bool TeaForGodEmperor::WeaponPartStatInfo<Class>::load_from_xml(IO::XML::Node const* _node, tchar const* _value)
{
	bool result = true;

	if (_node->has_attribute(_value))
	{
		warn_loading_xml(_node, TXT("do not use attributes, use child nodes"));
	}
	value.load_from_xml(_node, _value);

	if (auto* n = _node->first_child_named(_value))
	{
		if (auto* a = n->get_attribute(TXT("affects")))
		{
			how = WeaponStatAffection::parse(a->get_as_string());
		}
	}

	return result;
}

//--

template <typename Class>
void TeaForGodEmperor::WeaponStatInfo<Class>::reset()
{
	value = Class();
	affectedBy.clear();
	notAffectedBy.clear();
}

template <typename Class>
void TeaForGodEmperor::WeaponStatInfo<Class>::clear_affection()
{
	affectedBy.clear();
	notAffectedBy.clear();
}

template <typename Class>
void TeaForGodEmperor::WeaponStatInfo<Class>::move_to_not_affected_by()
{
	for_every(a, affectedBy)
	{
		if (notAffectedBy.has_place_left())
		{
			notAffectedBy.push_back_unique(*a);
		}
	}
	affectedBy.clear();
}

template <typename Class>
void TeaForGodEmperor::WeaponStatInfo<Class>::add_affection(WeaponPart const* _part, WeaponStatAffection::Type _how)
{
	if (affectedBy.has_place_left())
	{
		WeaponsStatAffectedByPart w;
		w.weaponPart = _part;
		w.how = _how;
		affectedBy.push_back_unique(w);
	}
}
