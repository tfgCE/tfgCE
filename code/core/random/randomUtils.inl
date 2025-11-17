
template <typename ContainerClass, typename ContainedObject>
int ChooseFromContainer<ContainerClass, ContainedObject>::choose(int _usingRandomNumber, ContainerClass const & _container, GetChance _get_chance, IsValid _is_valid, bool _noneIfAllInvalid)
{
	if (_container.is_empty())
	{
		return 0;
	}

	struct Ref
	{
		int idx;
		float chance;
	};

	ARRAY_PREALLOC_SIZE(Ref, refs, _container.get_size());

	float sumOfChances = 0.0f;
	for_every(object, _container)
	{
		if (!_is_valid || _is_valid(*object))
		{
			float chance = _get_chance(*object);
			if (chance > 0.0f)
			{
				Ref ref;
				ref.idx = for_everys_index(object);
				ref.chance = chance;
				refs.push_back(ref);
				sumOfChances += ref.chance;
			}
		}
	}

	if (!refs.is_empty())
	{
		if (sumOfChances > 0.0f)
		{
			float chosenChance = ((float)(mod(_usingRandomNumber, RAND_INT + 1)) / RAND_FLOAT) * sumOfChances;
			for_every(ref, refs)
			{
				chosenChance -= ref->chance;
				if (chosenChance <= 0.0f)
				{
					return ref->idx;
				}
			}
			return refs.get_last().idx;
		}
		else
		{
			return refs[mod(_usingRandomNumber, refs.get_size())].idx;
		}
	}
	else if (_noneIfAllInvalid)
	{
		return NONE;
	}
	else
	{
		return mod(_usingRandomNumber, _container.get_size());
	}
}

template <typename ContainerClass, typename ContainedObject>
int ChooseFromContainer<ContainerClass, ContainedObject>::choose(Random::Generator & _usingGenerator, ContainerClass const & _container, GetChance _get_chance, IsValid _is_valid, bool _noneIfAllInvalid)
{
	return choose(_usingGenerator.get_int(), _container, _get_chance, _is_valid, _noneIfAllInvalid);
}
