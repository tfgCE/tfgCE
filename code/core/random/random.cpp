#include "random.h"

#include "..\math\math.h"

#include "..\other\parserUtils.h"

#include "..\serialisation\serialiser.h"

#include <time.h>

//

int Random::get(RangeInt const& _range)
{
	if (_range.min != INF_INT && _range.max != INF_INT)
	{
		return get_int_from_range(_range.min, _range.max);
	}
	else if (_range.min == INF_INT)
	{
		if (_range.max == INF_INT)
		{
			return 0;
		}
		else
		{
			return _range.max;
		}
	}
	else
	{
		return _range.min;
	}
}

float Random::get(Range const& _range)
{
	return get_float(_range.min, _range.max);
}

//

Random::Generator Random::Generator::s_universal;

Random::Generator::Generator()
{
	do
	{
		time_t currentTime = time(0);

		set_seed((uint)(currentTime >> 32), (uint)currentTime);
	}
	while (is_zero_seed());
}

bool Random::Generator::is_zero_seed() const
{
#ifdef RANDOM_MERSENNE
	return seedIndex == 0 && seedIntermediateOffset == 0;
#endif
#ifdef RANDOM_PRESHING
	return index == 0 && intermediateOffset == 0;
#endif
#ifdef RANDOM_XOR_SHIFT_64
	return state_hi() == 0 && state_lo() == 0;
#endif	
#ifdef RANDOM_XOR_SHIFT_1024
	for_count(int, i, 16)
	{
		if (state[i] != 0)
		{
			return false;
		}
	}
	return true;
#endif
}

Random::Generator & Random::Generator::be_zero_seed()
{
#ifdef RANDOM_MERSENNE
	seedIndex = 0;
	seedIntermediateOffset = 0;
#endif
#ifdef RANDOM_PRESHING
	index = 0;
	intermediateOffset = 0;
#endif
#ifdef RANDOM_XOR_SHIFT_64
	state = 0;
#endif	
#ifdef RANDOM_XOR_SHIFT_1024
	for_count(int, i, 16)
	{
		state[i] = 0;
	}
#endif
	return *this;
}

String Random::Generator::get_seed_string() const
{
#ifdef RANDOM_MERSENNE
	return String::printf(TXT("%S:%S"), ParserUtils::uint_to_hex(seedIndex).to_char(), ParserUtils::uint_to_hex(seedIntermediateOffset).to_char());
#endif
#ifdef RANDOM_PRESHING
	return String::printf(TXT("%S:%S"), ParserUtils::uint_to_hex(index).to_char(), ParserUtils::uint_to_hex(intermediateOffset).to_char());
#endif
#ifdef RANDOM_XOR_SHIFT_64
	return String::printf(TXT("%S:%S"), ParserUtils::uint_to_hex(state_hi()).to_char(), ParserUtils::uint_to_hex(state_lo()).to_char());
#endif	
#ifdef RANDOM_XOR_SHIFT_1024
	String result;
	for_count(int, i, 16)
	{
		if (i > 0)
		{
			result += TXT(":");
		}
		result += ParserUtils::uint64_to_hex(state[i]).to_char();
	}
	result += String::printf(TXT(":%S"), ParserUtils::single_to_hex(index).to_char());
	return result;
#endif
}

String Random::Generator::get_seed_string_as_numbers() const
{
#ifdef RANDOM_MERSENNE
	return String::printf(TXT("%u, %u"), seedIndex, seedIntermediateOffset);
#endif
#ifdef RANDOM_PRESHING
	return String::printf(TXT("%u, %u"), index, intermediateOffset);
#endif
#ifdef RANDOM_XOR_SHIFT_64
	return String::printf(TXT("%u, %u"), state_hi(), state_lo());
#endif
#ifdef RANDOM_XOR_SHIFT_1024
	String result;
	for_count(int, i, 16)
	{
		if (i > 0)
		{
			result += TXT(", ");
		}
		result += String::printf(TXT("%llu"), state[i]);
	}
	result += String::printf(TXT(", %i"), index);
	return result;
#endif
}

uint Random::Generator::get_seed_hi_number() const
{
#ifdef RANDOM_MERSENNE
	return seedIndex;
#endif
#ifdef RANDOM_PRESHING
	return index;
#endif
#ifdef RANDOM_XOR_SHIFT_64
	return state_hi();
#endif
#ifdef RANDOM_XOR_SHIFT_1024
	an_assert(false, TXT("not possible"));
	return 0;
#endif
}

uint Random::Generator::get_seed_lo_number() const
{
#ifdef RANDOM_MERSENNE
	return seedIntermediateOffset;
#endif
#ifdef RANDOM_PRESHING
	return intermediateOffset;
#endif
#ifdef RANDOM_XOR_SHIFT_64
	return state_lo();
#endif
#ifdef RANDOM_XOR_SHIFT_1024
	return index;
#endif
}

bool Random::Generator::serialise(Serialiser& _serialiser)
{
	bool result = true;

#ifdef RANDOM_MERSENNE
	result &= serialise_data(_serialiser, seedIndex);
	result &= serialise_data(_serialiser, seedIntermediateOffset);
	if (_serialiser.is_reading())
	{
		set_seed(seedIndex, seedIntermediateOffset);
	}
#endif
#ifdef RANDOM_PRESHING
	result &= serialise_data(_serialiser, index);
	result &= serialise_data(_serialiser, intermediateOffset);
#endif
#ifdef RANDOM_XOR_SHIFT_64
	uint stateA = state_hi();
	uint stateB = state_lo();
	result &= serialise_data(_serialiser, stateA);
	result &= serialise_data(_serialiser, stateB);
	if (_serialiser.is_reading())
	{
		set_seed(stateA, stateB);
	}
#endif
#ifdef RANDOM_XOR_SHIFT_1024
	for_count(int, i, 16)
	{
		result &= serialise_data(_serialiser, state[i]);
	}
	result &= serialise_data(_serialiser, index);
#endif

	return result;
}

bool Random::Generator::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (_node)
	{
		auto* baseAttr = _node->get_attribute(TXT("base"));
		auto* offsetAttr = _node->get_attribute(TXT("offset"));
		if (baseAttr || offsetAttr)
		{
			set_seed(baseAttr ? baseAttr->get_as_int() : 0, offsetAttr ? offsetAttr->get_as_int() : 0);
		}
	}

	return result;
}

bool Random::Generator::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	_node->set_int_attribute(TXT("base"), (int)get_seed_hi_number());
	_node->set_int_attribute(TXT("offset"), (int)get_seed_lo_number());

	return result;
}

float Random::Generator::get_float(Range const& _range)
{
	return _range.is_empty() ? 0.0f : get_float(_range.min, _range.max);
}

float Random::Generator::get(Range const& _range)
{
	return get_float(_range.min, _range.max);
}

int Random::Generator::get(RangeInt const& _range)
{
	if (_range.min != INF_INT && _range.max != INF_INT)
	{
		return get_int_from_range(_range.min, _range.max);
	}
	else if (_range.min == INF_INT)
	{
		if (_range.max == INF_INT)
		{
			return 0;
		}
		else
		{
			return _range.max;
		}
	}
	else
	{
		return _range.min;
	}
}

