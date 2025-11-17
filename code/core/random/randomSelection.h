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
	 *	Chooses one of the values depending on chance.
	 *	This is basically just a wrap up for functionality to make it easier to use.
	 */
	template<typename Class>
	class Selection
	{
	private:
		struct Section;

	public:
		Selection() {}

		Class get(Generator & _generator) const;
		Class get() const;

		void clear() { values.clear(); }
		void add(Class const & _value) { values.push_back(Value(_value, 1.0f)); }
		void add(float _probCoef, Class const & _value) { values.push_back(Value(_value, _probCoef)); }

	private:
		struct Value
		{
			Class value;
			float probCoef;

			Value(): probCoef(0.0f) {}
			Value(Class const& _value, float _probCoef) : value(_value), probCoef(_probCoef) {}
		};
		Array<Value> values;
	};

	#include "randomSelection.inl"
};
