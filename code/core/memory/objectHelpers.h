#pragma once

template <typename Class>
class ObjectHelpers
{
	/**
	 *	Set of functions to allow different operations on objects
	 */
public:
	inline static void call_constructor_on(Class* ptr);
	inline static void call_copy_constructor_on(Class* dest, Class const* from);
	inline static void call_destructor_on(Class* ptr);
	inline static void copy(Class* dest, Class const* from); // although that is just a copy
	inline static void copy(Class* dest, Class const* from, Class const* until);
	inline static void copy_reverse(Class* dest, Class const* from, Class const* untilInclusive);
	inline static void call_constructor_on(Class* from, Class* until);
	inline static void call_copy_constructor_on(Class* dest, Class const* from, Class const* until);
	inline static void call_destructor_on(Class* from, Class* until);

	// working on void pointers
	inline static void call_constructor_on(void* ptr) { call_constructor_on((Class*)ptr); }
	inline static void call_destructor_on(void* ptr) { call_destructor_on((Class*)ptr); }
	static void static_call_destructor_on(void* ptr) { call_destructor_on(ptr); }
	inline static void call_copy_on(void* ptr, void const* from) { copy((Class*)ptr, (Class*)from); }
	inline static void call_copy_constructor_on(void* dest, void const* from) { call_copy_constructor_on((Class*)dest, (Class const*)from); }
};

#include "objectHelpers.inl"