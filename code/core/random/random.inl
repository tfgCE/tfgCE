float adjust_chance(float _chance, float _adjustBy, bool _keepNoChance)
{
	_chance = clamp(_chance, 0.0f, 1.0f);
	if (_adjustBy > 0.0f)
	{
		if (_keepNoChance && _chance == 0.0f)
		{
			return 0.0f;
		}
		_chance = 1.0f - (1.0f - _chance) * clamp(1.0f - _adjustBy, 0.0f, 1.0f);
	}
	else
	{
		_chance *= clamp(1.0f + _adjustBy, 0.0f, 1.0f);
	}
	return _chance;
}

// inclusive min max
float get_float(float _min, float _maxInclusive)
{
	return _maxInclusive > _min ? ((float)(next_uint() % (RAND_INT + 1)) / RAND_FLOAT) * (_maxInclusive - _min) + _min : _min;
}

// inclusive min max
int get_int_from_range(int _min, int _maxInclusive)
{
	return _maxInclusive > _min ? (int)(next_uint() % (_maxInclusive + 1 - _min)) + _min : _min;
}

int get_int(int _max)
{
	return next_uint() % _max;
}

bool get_bool()
{
	return get_float(0.0f, 1.0f) < 0.5f;
}

bool get_chance(float _chance)
{
	return _chance > 0.0f && get_float(0.0f, 1.0f) <= _chance;
}

int next_int()
{
	return Random::Generator::get_universal().get_int();
}

uint next_uint()
{
	return (uint)Random::Generator::get_universal().get_int();
}

Generator::Generator(uint _seedBase, uint _seedOffset)
{
	set_seed(_seedBase, _seedOffset);
}

Generator Generator::spawn()
{
	// strict order!
	int newBase = get_int();
	int newOffset = get_int();
	if (newBase == 0 && newOffset == 0)
	{
		// we don't want to provide any empty random generator
		++ newBase;
	}
	return Generator(newBase, newOffset);
}

void Generator::set_seed(void const* _ptr)
{
	set_seed(static_cast<uint>(reinterpret_cast<uintptr_t>(_ptr)), 0);
}

void Generator::set_seed(uint _seedBase, uint _seedOffset)
{
	// use as they are!
#ifdef RANDOM_MERSENNE
	std::seed_seq seq{_seedBase, _seedOffset};
	gen.seed(seq);
	seedIndex = _seedBase;
	seedIntermediateOffset = _seedOffset;
#endif
#ifdef RANDOM_PRESHING
	index = _seedBase;
	intermediateOffset = _seedOffset;
#endif
#ifdef RANDOM_XOR_SHIFT_64
	state = _seedBase;
	state = state << 32;
	state |= _seedOffset;
#endif
#ifdef RANDOM_XOR_SHIFT_1024
	for_count(int, i, 8)
	{
		state[i * 2 + 0] = _seedBase;
		state[i * 2 + 1] = _seedOffset;
	}
	index = 0;
#endif
}

void Generator::advance_seed(void const* _ptr)
{
	advance_seed(static_cast<uint>(reinterpret_cast<uintptr_t>(_ptr)), 0);
}

void Generator::advance_seed(uint _seedBase, uint _seedOffset)
{
#ifdef RANDOM_MERSENNE
	set_seed(seedIndex + _seedBase, seedIntermediateOffset + _seedOffset);
#endif
#ifdef RANDOM_PRESHING
	index += _seedBase;
	intermediateOffset += _seedOffset;
#endif
#ifdef RANDOM_XOR_SHIFT_64
	set_seed(state_hi() + _seedBase, state_lo() + _seedOffset);
#endif
#ifdef RANDOM_XOR_SHIFT_1024
	for_count(int, i, 8)
	{
		state[i * 2 + 0] += _seedBase;
		state[i * 2 + 1] += _seedOffset;
	}
#endif
}

void Generator::advance_seed(REF_ Generator & _generator)
{
	// order of parameter calls is important (and different across platforms), hence we do it explicitly instead of using function call parameters
	uint advSeedBase = _generator.get_int();
	uint advSeedOffset = _generator.get_int();
	advance_seed(advSeedBase, advSeedOffset);
}

float Generator::get_float(float _min, float _maxInclusive)
{
	return _maxInclusive > _min ? next_float() * (_maxInclusive - _min) + _min : _min;
}

int Generator::get_int()
{
	return next_int();
}

int Generator::get_int_from_range(int _min, int _maxInclusive)
{
	return _maxInclusive > _min ? (int)(next_uint() % (_maxInclusive + 1 - _min)) + _min : _min;
}

int Generator::get_int(int _maxExclusive)
{
	return _maxExclusive > 0? (int)(next_uint() % ((uint32)_maxExclusive)) : 0;
}

bool Generator::get_bool()
{
	return get_float(0.0f, 1.0f) < 0.5f;
}

bool Generator::get_chance(float _chance)
{
	return _chance > 0.0f && get_float(0.0f, 1.0f) <= _chance;
}

float Generator::next_float()
{
	// TODO do it properly
	return ((float)(next_uint() % (RAND_UINT))) / RAND_UFLOAT;
}

int Generator::next_int()
{
#ifdef RANDOM_MERSENNE
	return gen();
#endif
#ifdef RANDOM_PRESHING
	return (int)(permute_qpr((permute_qpr(index++) + intermediateOffset) ^ 0x5bf03635));
#endif
#ifdef RANDOM_XOR_SHIFT_64
	uint64_t x = state;
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c
	state = x;
	return (int)((x * 0x2545F4914F6CDD1D) & 0xffffffff);
#endif
#ifdef RANDOM_XOR_SHIFT_1024
	// based on https://en.wikipedia.org/wiki/Xorshift
	const uint64_t s0 = state[index];
	index = (index + 1) & 15;
	uint64_t s1 = state[index];
	s1 ^= s1 << 31; // a
	state[index] = s1 ^ s0 ^ (s1 >> 11) ^ (s0 >> 30); // b, c
	return (int)(state[index] * UINT64_C(1181783497276652981));
#endif
}

uint Generator::next_uint()
{
	return (uint)next_int();
}

#ifdef RANDOM_PRESHING
uint Generator::permute_qpr(uint x)
{
	// based on: http://preshing.com/20121224/how-to-generate-a-sequence-of-unique-random-integers/
	uint prime = 4294967291u;
	if (x >= prime)
		return x;  // The 5 integers out of range are mapped to themselves.
	uint residue = (uint)(((unsigned long long)x * x) % (unsigned long)prime);
	return (x <= prime / 2) ? residue : prime - residue;
}
#endif
