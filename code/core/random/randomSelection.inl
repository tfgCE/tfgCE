template<typename Class>
Class Selection<Class>::get() const
{
	Random::Generator generator;
	return get(generator);
}

template<typename Class>
Class Selection<Class>::get(Generator & _generator) const
{
	if (values.is_empty())
	{
		Value noneValue;
		return noneValue.value;
	}

	struct Ref
	{
		int idx;
		float chance;
	};

	ARRAY_PREALLOC_SIZE(Ref, refs, values.get_size());

	float sumOfChances = 0.0f;
	for_every(v, values)
	{
		float chance = v->probCoef;
		if (chance > 0.0f)
		{
			Ref ref;
			ref.idx = for_everys_index(v);
			ref.chance = chance;
			refs.push_back(ref);
			sumOfChances += ref.chance;
		}
	}

	if (!refs.is_empty())
	{
		if (sumOfChances > 0.0f)
		{
			float chosenChance = ((float)(mod(_generator.get_int(), RAND_INT + 1)) / RAND_FLOAT) * sumOfChances;
			for_every(ref, refs)
			{
				chosenChance -= ref->chance;
				if (chosenChance <= 0.0f)
				{
					return values[ref->idx].value;
				}
			}
			return values[refs.get_last().idx].value;
		}
		else
		{
			return values[refs[_generator.get_int(refs.get_size())].idx].value;
		}
	}
	else
	{
		return values[_generator.get_int(values.get_size())].value;
	}
}
