#pragma once

#include "types\optional.h"

/*
	Usage

	struct SomeMethodParams
	{
		ADD_FUNCTION_PARAM(SomeMethodParams, Colour, colour, with_colour);
		ADD_FUNCTION_PARAM(SomeMethodParams, float, size, with_size);
		ADD_FUNCTION_PARAM_DEF(SomeMethodParams, bool, bigger, be_bigger, true);
	};
	void some_method(SomeMethodParams const & _params);

	...
	some_method(SomeMethodParams().with_colour(Colour::white));
	...

 */
 
#define ADD_FUNCTION_PARAM(structType, type, name, set_function) \
	Optional<type> name; structType & set_function(Optional<type> const & _value) { name = _value; return *this; }

#define ADD_FUNCTION_PARAM_DEF(structType, type, name, set_function, _defValue) \
	Optional<type> name; structType & set_function(Optional<type> const & _value = _defValue) { name = _value; return *this; }

#define ADD_FUNCTION_PARAM_TRUE(structType, name, set_function) \
	ADD_FUNCTION_PARAM_DEF(structType, bool, name, set_function, true);

#define ADD_FUNCTION_PARAM_FALSE(structType, name, set_function) \
	ADD_FUNCTION_PARAM_DEF(structType, bool, name, set_function, false);
	
#define ADD_FUNCTION_PARAM_PTR(structType, type, name, set_function) \
	type * name = nullptr; structType & set_function(type * _value) { name = _value; return *this; }

#define ADD_FUNCTION_PARAM_PLAIN(structType, type, name, set_function) \
	type name; structType & set_function(type const & _value) { name = _value; return *this; }

#define ADD_FUNCTION_PARAM_PLAIN_INITIAL(structType, type, name, set_function, initialValue) \
	type name = initialValue; structType & set_function(type const & _value) { name = _value; return *this; }

#define ADD_FUNCTION_PARAM_PLAIN_DEF(structType, type, name, set_function, defCallValue) \
	type name; structType & set_function(type const & _value = defCallValue) { name = _value; return *this; }

#define ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(structType, type, name, set_function, initialValue, defCallValue) \
	type name = initialValue; structType & set_function(type const & _value = defCallValue) { name = _value; return *this; }
