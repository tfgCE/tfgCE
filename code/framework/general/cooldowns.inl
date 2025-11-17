template<unsigned int Count, typename NameElement>
Cooldowns<Count, NameElement>::Cooldowns()
{
	SET_EXTRA_DEBUG_INFO(cooldowns, TXT("Cooldowns.cooldowns"));
}

template<unsigned int Count, typename NameElement>
bool Cooldowns<Count, NameElement>::is_available(NameElement const & _name) const
{
	for_every(c, cooldowns)
	{
		if (c->name == _name)
		{
			return false;
		}
	}
	return true;
}

template<unsigned int Count, typename NameElement>
void Cooldowns<Count, NameElement>::advance(float _deltaTime)
{
	for (int i = 0; i < cooldowns.get_size(); ++i)
	{
		float & cooldownLeft = cooldowns[i].left;
		cooldownLeft -= _deltaTime;
		if (cooldownLeft <= 0.0f)
		{
			cooldowns.remove_fast_at(i);
			--i;
		}
	}
}

template<unsigned int Count, typename NameElement>
void Cooldowns<Count, NameElement>::add(NameElement const & _name, Optional<float> const & _cooldown)
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
	cooldown.left = _cooldown.get();
	cooldowns.push_back(cooldown);
}

template<unsigned int Count, typename NameElement>
void Cooldowns<Count, NameElement>::clear(NameElement const & _name)
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
void Cooldowns<Count, NameElement>::set(NameElement const & _name, Optional<float> const & _cooldown)
{
	if (!_cooldown.is_set() || _cooldown.get() < 0.0f)
	{
		return;
	}
	for_every(cooldown, cooldowns)
	{
		if (cooldown->name == _name)
		{
			cooldown->left = _cooldown.get();
			return;
		}
	}
	if (!make_sure_theres_place_for_new_one(_cooldown.get()))
	{
		return;
	}
	Cooldown cooldown;
	cooldown.name = _name;
	cooldown.left = _cooldown.get();
	cooldowns.push_back(cooldown);
}

template<unsigned int Count, typename NameElement>
void Cooldowns<Count, NameElement>::set_if_longer(NameElement const & _name, Optional<float> const & _cooldown)
{
	if (!_cooldown.is_set() || _cooldown.get() < 0.0f)
	{
		return;
	}
	for_every(cooldown, cooldowns)
	{
		if (cooldown->name == _name)
		{
			cooldown->left = max(cooldown->left, _cooldown.get());
			return;
		}
	}
	if (!make_sure_theres_place_for_new_one(_cooldown.get()))
	{
		return;
	}
	Cooldown cooldown;
	cooldown.name = _name;
	cooldown.left = _cooldown.get();
	cooldowns.push_back(cooldown);
}

template<unsigned int Count, typename NameElement>
void Cooldowns<Count, NameElement>::log(LogInfoContext & _log) const
{
	for_every(cooldown, cooldowns)
	{
		_log.log(TXT("   %S: %.3f"), as_string(cooldown->name).to_char(), cooldown->left);
	}
}

template<unsigned int Count, typename NameElement>
Optional<float> Cooldowns<Count, NameElement>::get(NameElement const & _name) const
{
	for_every(cooldown, cooldowns)
	{
		if (cooldown->name == _name)
		{
			return cooldown->left;
		}
	}
	return NP;
}

template<unsigned int Count, typename NameElement>
bool Cooldowns<Count, NameElement>::make_sure_theres_place_for_new_one(float _cooldown)
{
	if (cooldowns.has_place_left())
	{
		return true;
	}
	int lowestIdx = NONE;
	float lowestCooldown = 0.0f;
	for_every(cooldown, cooldowns)
	{
		if (lowestIdx == NONE || lowestCooldown > cooldown->left)
		{
			lowestIdx = for_everys_index(cooldown);
			lowestCooldown = cooldown->left;
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
