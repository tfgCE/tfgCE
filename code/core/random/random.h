#pragma once

#include "..\globalDefinitions.h"

#include "..\defaults.h"

#include "..\math\mathUtils.h"

#include "..\other\registeredType.h"

// one of:

// good, but there is currently a problem with getting/setting current state, todo: maybe it's worth just dumping memory straight into an int?
//#define RANDOM_MERSENNE

// gets quite bad results, when trying to get random number 0..15 gets only 0,1,9,10 (or something like that), also responsible for avoiding get_bool()
//#define RANDOM_PRESHING

// works really nice
#define RANDOM_XOR_SHIFT_64

// craziness released, with better seed setting this can be amazing for vast universes, for time being 64-bit is enough
//#define RANDOM_XOR_SHIFT_1024


#define RAND_INT 0x7fff
#define RAND_FLOAT (((float)(RAND_INT) + 1.0f))

#define RAND_UINT 0xffff
#define RAND_UFLOAT (((float)(RAND_UINT) + 1.0f))

class Serialiser;

struct Range;
struct RangeInt;
struct String;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Random
{
	inline float adjust_chance(float _chance, float _adjustBy, bool _keepNoChance = false); // _adjustBy goes from -1.0 to 1.0 (where -1.0 ends with chance==0.0 and 1.0 ends with chance==1.0) (keepNoChance == true? if chance is 0, keep it being 0
	inline float get_float(float _min, float _maxInclusive); // inclusive min max
	inline int get_int_from_range(int _min, int _maxInclusive); // inclusive min max
	inline int get_int(int _max);
	inline bool get_bool();
	inline bool get_chance(float _chance);
	inline int next_int();
	inline uint next_uint();
	int get(RangeInt const & _range);
	float get(Range const & _range);

	class Generator
	{
	public:
		Generator copy() const { return *this; }
		Generator& temp_ref() { return *this; }

		static inline Generator & get_universal() { return s_universal; }

		Generator();
		explicit inline Generator(uint _seedBase, uint _seedOffset);

		inline Generator spawn();

		inline void set_seed(void const* _ptr);
		inline void set_seed(uint _seedBase, uint _seedOffset);
		inline void advance_seed(uint _seedBase, uint _seedOffset);
		inline void advance_seed(REF_ Generator & _generator);
		inline void advance_seed(void const* _ptr);
		
		inline float get_float(float _min, float _maxInclusive);
		float get_float(Range const & _range);
		inline int get_int_from_range(int _min, int _maxInclusive);
		inline int get_int();
		inline int get_int(int _max);
		inline bool get_bool();
		inline bool get_chance(float _chance);
		int get(RangeInt const & _range);
		float get(Range const & _range);

#ifdef RANDOM_MERSENNE
		bool operator == (Generator const & _other) const { return gen._Equals(_other.gen); }
#endif
#ifdef RANDOM_PRESHING
		bool operator == (Generator const & _other) const { return index == _other.index && intermediateOffset == _other.intermediateOffset; }
#endif
#ifdef RANDOM_XOR_SHIFT_64
		bool operator == (Generator const & _other) const { return state == _other.state; }
#endif
#ifdef RANDOM_XOR_SHIFT_1024
		bool operator == (Generator const & _other) const { return memory_compare(state, _other.state, sizeof(uint64_t) * 16) && index == _other.index; }
		uint64_t state[16];
		int index;
#endif
		bool operator != (Generator const & _other) const { return !operator==(_other); }

		bool is_zero_seed() const;
		Generator & be_zero_seed();

		String get_seed_string() const;
		String get_seed_string_as_numbers() const;
		uint get_seed_hi_number() const;
		uint get_seed_lo_number() const;

		bool load_from_xml(IO::XML::Node const* _node);
		bool save_to_xml(IO::XML::Node* _node) const;

	public:
		bool serialise(Serialiser& _serialiser);

	private:
		static Generator s_universal;

#ifdef RANDOM_MERSENNE
		std::mt19937 gen;
		uint seedIndex;
		uint seedIntermediateOffset;
#endif
#ifdef RANDOM_PRESHING
		uint index;
		uint intermediateOffset;
#endif
#ifdef RANDOM_XOR_SHIFT_64
		uint64 state;
#endif
#ifdef RANDOM_XOR_SHIFT_1024
		uint64_t s[16];
		int p;
#endif

		inline float next_float();
		inline int next_int();
		inline uint next_uint();

#ifdef RANDOM_PRESHING
		inline static uint permute_qpr(uint x);
#endif
#ifdef RANDOM_XOR_SHIFT_64
		uint state_hi() const { return state >> 32; }
		uint state_lo() const { return state & 0xffffffff; }
#endif
	};

	#include "random.inl"
};

DECLARE_REGISTERED_TYPE(Random::Generator);

template <>
inline void set_to_default<Random::Generator>(Random::Generator& _object)
{
	_object = Random::Generator(0, 0);
}
