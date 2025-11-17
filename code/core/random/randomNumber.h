#pragma once

#include "random.h"

#include "..\containers\array.h"
#include "..\io\xml.h"
#include "..\math\math.h"

class Serialiser;

namespace Random
{
	class Generator;

	/**
		This is a random number generator that bases on sections and gives option to round and offset values
		It should be easier to generate prices or uneven number generation
		If we want values 1-4 be more common, we can have separate section for that
		If we want just to have even numbers, we may set round to 2
		For uneven, we generate even and add offset 1
		If we want all values be offset by something (for example we want prices to be like 1.99, 14.99) we use round (1.00) and offset (-0.01)
	 
		Simple example (choose/sections may be nested too)
	 		<choose value="1" probCoef="0.7"/>
			<choose value="2 to 3" round="0.5" probCoef="0.3"/>
	
	 */
	template<typename Class, typename ClassUtils>
	class NumberBase
	{
	public:
		struct Section;

	public:
		NumberBase() {}
		NumberBase(Class const & _singleValue);

		Class get(Generator & _generator, bool _nonRandomised = false) const;
		Class get() const;
		Class get_min() const;
		Class get_max() const;

		bool is_set() const { return !is_empty(); }
		bool is_empty() const { return sections.is_empty(); } // if empty, it means nothing was set and some other fallback behaviour should be considered as this will return 0

		bool load_from_xml(IO::XML::Node const * _node);
		bool load_from_xml(IO::XML::Attribute const * _attr);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrOrChildName);

		void clear() { sections.clear(); }
		void add(Section const & _section) { sections.push_back(_section); }
		void add(Class const& _value);

		bool serialise(Serialiser & _serialiser);

		int estimate_options_count() const;

	public:
		struct Section
		{
			Class get(Generator & _generator, bool _nonRandomised = false) const;
			Class get_min() const;
			Class get_max() const;
			Class process(Class const & _value) const;

			Section & with_chance(float _chance) { chance = _chance; return *this; }
			Section & value(Class const & _value) { valueRangeMin = _value; valueRangeMax = _value; return *this; }
			Section & value(Class const & _valueRangeMin, Class const & _valueRangeMax) { valueRangeMin = _valueRangeMin; valueRangeMax = _valueRangeMax; return *this; }
			Section & clamp_to(Class const & _clampRangeMin, Class const & _clampRangeMax) { clampRangeMin = _clampRangeMin; clampRangeMax = _clampRangeMax; clampRangeIsSet = true; return *this; }
			Section & dont_clamp() { clampRangeIsSet = false; return *this; }
			Section & with_round(Class const & _round) { round = _round; return *this; }
			Section & with_offset(Class const & _offset) { offset = _offset; return *this; }

			bool serialise(Serialiser & _serialiser);
		private: friend class NumberBase;
			GS_ float chance = 1.0f;
			GS_ Class valueRangeMin = ClassUtils::zero();
			GS_ Class valueRangeMax = ClassUtils::zero();
			GS_ bool clampRangeIsSet = false;
			GS_ Class clampRangeMin = ClassUtils::zero();
			GS_ Class clampRangeMax = ClassUtils::zero();
			GS_ Class round = ClassUtils::zero(); // round up/down to
			GS_ Class offset = ClassUtils::zero();

			bool load_from_xml(IO::XML::Node const * _node);

			static bool load_value(IO::XML::Node const * _node, tchar const * _attr, REF_ Class & _value);
			static bool load_range(IO::XML::Node const * _node, tchar const * _attr, REF_ Class & _min, REF_ Class & _max);
			static bool load_range(IO::XML::Attribute const * _attr, REF_ Class & _min, REF_ Class & _max);
			static bool load_range(::String const & text, REF_ Class & _min, REF_ Class & _max);
		};
	private:
		GS_ Array<Section> sections;
	};

	// helper class to use same class for utils
	template<typename Class>
	class NumberUtilsFor
	: public Class
	{
		/*	required to be implemented
			static Class zero();
			static Class half_of(Class _value);
			static bool is_zero(Class const& _value);
			static Class get_random(Random::Generator& _generator, Class const& _min, Class const& _max);
			static Class parse_from(::String const& _string, Class const& _defValue);
		 */
	};

	// most common case - own classes that have static functions working as utils
	template<typename Class>
	class Number
	: public NumberBase<Class, NumberUtilsFor<Class>>
	{
		typedef NumberBase<Class, NumberUtilsFor<Class>> base;
	public:
		Number(): base() {}
		Number(Class const & _singleValue) : base(_singleValue) {}
	};

	// specialisations
	template<>
	class NumberUtilsFor<int>
	: public intUtils
	{};

	template<>
	class NumberUtilsFor<float>
	: public floatUtils
	{};

	#include "randomNumber.inl"
};

DECLARE_REGISTERED_TYPE(Random::Number<float>);
DECLARE_REGISTERED_TYPE(Random::Number<int>);
