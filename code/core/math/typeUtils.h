#pragma once

namespace Random
{
	class Generator;
};

struct String;

class floatUtils
{
public:
	static float zero() { return 0.0f; }
	static float half_of(float _value) { return _value * 0.5f; }
	static bool is_zero(float const & _value) { return _value == 0.0f; }
	static float get_random(Random::Generator & _generator, float const & _min, float const & _max);
	static float parse_from(String const & _string, float const & _defValue);
};

class intUtils
{
public:
	static int zero() { return 0; }
	static int half_of(int _value) { return _value / 2; }
	static bool is_zero(int const & _value) { return _value == 0; }
	static int get_random(Random::Generator & _generator, int const & _min, int const & _max);
	static int parse_from(String const & _string, int const & _defValue);
};
