#pragma once

#include "iWMPTool.h"

#include "..\random\random.h"

namespace WheresMyPoint
{
	interface_class IOwner;

	struct Context
	{
	public:
		Context(IOwner * _owner);

		IOwner * get_owner() const { return owner; }

		Value const * get_temp_value(Name const & _byName) const;
		void set_temp_value(Name const & _byName, Value const & _value);

		// values can be accessed from stack and from below
		// but when popping and requring to keep values on stack, it is advised to first call bunch of keep_temp_before_popping for any variables to keep and then pop_temp_stack
		bool push_temp_stack();
		bool keep_temp_before_popping(Name const & _valueName); // will move value if at top of the stack
		bool pop_temp_stack();

		Random::Generator & access_random_generator() { return randomGenerator; }
		Random::Generator const & get_random_generator() const { return randomGenerator; }
		void set_random_generator(Random::Generator const & _randomGenerator) { randomGenerator = _randomGenerator; }

	private:
		IOwner * owner;

		Random::Generator randomGenerator;

		struct ValueEntry
		{
			::Name name;
			Value value;
		};
		Array<ValueEntry> tempStorage; // values may repeat when they are stacked or when we pop them (because we don't care about finding the latest one) - always the latest one is accessed
		Array<int> stacked;
		int stackAt = 0;
	};
};
