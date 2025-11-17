template<unsigned int Count, typename NameElement>
CooldownsTimeStampBased<Count, NameElement>::CooldownsTimeStampBased()
{
	SET_EXTRA_DEBUG_INFO(cooldowns, TXT("CooldownsTimeStampBased.cooldowns"));
}

template<unsigned int Count, typename NameElement>
bool CooldownsTimeStampBased<Count, NameElement>::is_available(NameElement const & _name) const
{
	for_every(c, cooldowns)
	{
		if (c->name == _name &&
			c->freeAtTS.get_time_since() <= 0.0f)
		{
			return false;
		}
	}
	return true;
}

template<unsigned int Count, typename NameElement>
void CooldownsTimeStampBased<Count, NameElement>::clean_up()
{
	for (int i = 0; i < cooldowns.get_size(); ++i)
	{
		auto& c = cooldowns[i];
		if (c.freeAtTS.get_time_since() > 0.0f)
		{
			cooldowns.remove_fast_at(i);
			--i;
		}
	}
}

template<unsigned int Count, typename NameElement>
void CooldownsTimeStampBased<Count, NameElement>::add(NameElement const & _name, Optional<float> const & _cooldown)
{
	if (!_cooldown.is_set() || _cooldown.get() < 0.0f)
	{
		return;
	}
	if (!make_sure_theres_place_for_new_one(_cooldown.get()))
	{
		return;
	}
	Cooldown cooldown;
	cooldown.name = _name;
	cooldown.freeAtTS.reset(_cooldown.get());
	cooldowns.push_back(cooldown);
}

template<unsigned int Count, typename NameElement>
void CooldownsTimeStampBased<Count, NameElement>::clear(NameElement const & _name)
{
	for_every(cooldown, cooldowns)
	{
		if (cooldown->name == _name)
		{
			cooldowns.remove_at(for_everys_index(cooldown));
			return;
		}
	}
}

template<unsigned int Count, typename NameElement>
void CooldownsTimeStampBased<Count, NameElement>::set(NameElement const & _name, Optional<float> const & _cooldown)
{
	if (!_cooldown.is_set() || _cooldown.get() < 0.0f)
	{
		return;
	}
	for_every(cooldown, cooldowns)
	{
		if (cooldown->name == _name)
		{
			cooldown->freeAtTS.reset(_cooldown.get());
			return;
		}
	}
	if (!make_sure_theres_place_for_new_one(_cooldown.get()))
	{
		return;
	}
	Cooldown cooldown;
	cooldown.name = _name;
	cooldown.freeAtTS.reset(_cooldown.get());
	cooldowns.push_back(cooldown);
}

template<unsigned int Count, typename NameElement>
void CooldownsTimeStampBased<Count, NameElement>::set_if_longer(NameElement const & _name, Optional<float> const & _cooldown)
{
	if (!_cooldown.is_set() || _cooldown.get() < 0.0f)
	{
		return;
	}
	for_every(cooldown, cooldowns)
	{
		if (cooldown->name == _name)
		{
			cooldown->freeAtTS.reset(max(cooldown->freeAtTS.get_time_to_zero(), _cooldown.get()));
			return;
		}
	}
	if (!make_sure_theres_place_for_new_one(_cooldown.get()))
	{
		return;
	}
	Cooldown cooldown;
	cooldown.name = _name;
	cooldown.freeAtTS.reset(_cooldown.get());
	cooldowns.push_back(cooldown);
}

template<unsigned int Count, typename NameElement>
void CooldownsTimeStampBased<Count, NameElement>::log(LogInfoContext & _log) const
{
	for_every(cooldown, cooldowns)
	{
		_log.log(TXT("   %S: %.3f"), as_string(cooldown->name).to_char(), cooldown->freeAtTS.get_time_to_zero());
	}
}

template<unsigned int Count, typename NameElement>
Optional<float> CooldownsTimeStampBased<Count, NameElement>::get(NameElement const & _name) const
{
	for_every(cooldown, cooldowns)
	{
		if (cooldown->name == _name)
		{
			return cooldown->freeAtTS.get_time_to_zero();
		}
	}
	return NP;
}

template<unsigned int Count, typename NameElement>
bool CooldownsTimeStampBased<Count, NameElement>::make_sure_theres_place_for_new_one(float _cooldown)
{
	clean_up();
	if (cooldowns.has_place_left())
	{
		return true;
	}
	int lowestIdx = NONE;
	float lowestCooldown = 0.0f;
	for_every(cooldown, cooldowns)
	{
		float cl = cooldown->freeAtTS.get_time_to_zero();
		if (lowestIdx == NONE || lowestCooldown > cl)
		{
			lowestIdx = for_everys_index(cooldown);
			lowestCooldown = cl;
		}
	}
	if (lowestIdx != NONE && lowestCooldown < _cooldown)
	{
		cooldowns.remove_fast_at(lowestIdx);
		return true;
	}
	else
	{
		return false;
	}
}
