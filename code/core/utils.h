#pragma once

template <typename Type>
inline Type const & gibberish_value()
{
	static Type gibberishValue;
	return gibberishValue;
}

template <typename Type>
inline void const* void_const_ptr(Type const& _obj)
{
	return &_obj;
}

template <typename Type>
inline Type const* const_ptr(Type const& _obj)
{
	return &_obj;
}

template <typename Type>
inline tchar const* type_as_char()
{
	return TXT("<type_as_char>");
}

#define TYPE_AS_CHAR(Type) \
template <> \
inline tchar const* type_as_char<Type>() { return TXT(#Type); }

#define safe_tchar_ptr(ptr) (ptr? ptr : TXT(""))

#define assign_value_to_ptr(ptr, value) \
if (ptr) { *ptr = (value); }

#define assign_optional_out_param(outParam, value) \
if (outParam) { *outParam = (value); }

#define assign_if_not_assigned(to, value) \
if (! to) { to = (value); }

#define delete_and_clear(pointer) \
if (pointer) { delete pointer; pointer = nullptr; }

#define release_and_clear(pointer) \
if (pointer) { pointer->release(); pointer = nullptr; }

#define release_ref_and_clear(pointer) \
if (pointer) { pointer->release_ref(); pointer = nullptr; }

#define for_range_step(rangeType, rangeVariable, startRange, endRange, step) \
	for (rangeType rangeVariable = startRange, end_##rangeVariable = endRange; rangeVariable <= end_##rangeVariable; rangeVariable += step)

#define for_range(rangeType, rangeVariable, startRange, endRange) \
	for (rangeType rangeVariable = startRange, end_##rangeVariable = endRange; rangeVariable <= end_##rangeVariable; ++rangeVariable)

#define for_range_reverse(rangeType, rangeVariable, startRange, endRange) \
	for (rangeType rangeVariable = startRange, end_##rangeVariable = endRange; rangeVariable >= end_##rangeVariable; --rangeVariable)

#define for_count(countType, countVariable, countValue) \
	for (countType countVariable = 0, end_##countVariable = countValue; countVariable < end_##countVariable; ++countVariable)

#define for_count_reverse(countType, countVariable, countValue) \
	for (countType countVariable = countValue - 1; countVariable >= 0; --countVariable)

// use normal with object held by container
// use ptr with pointer to object held by container

// by the rule, they work on any class that provides:
// begin() and end() functions returning iterators
// iterators should be structs offering */-> operator and pre++ or pre-- operators

#ifdef AN_CLANG
	#define temp_variable_named_join_again(A, B) tempvariable_ ## A ## _ ## B
	#define temp_variable_named_join(A, B) temp_variable_named_join_again(A, B)
	#define temp_variable_named(A) temp_variable_named_join(A, __LINE__)
#else
	#define temp_variable_name_helper(line) tempvariable_##line 
	#define temp_variable_name(line) temp_variable_name_helper(line)

	#define temp_variable_named(name) temp_variable_name(__LINE__)_##name
#endif

#define temp_container_named_join_again(A, B) tempcontainer_ ## A ## _ ## B
#define temp_container_named_join(A, B) temp_container_named_join_again(A, B)
#define temp_container_named(A) temp_container_named_join(A, __LINE__)

#define temp_container_named_two_join_again(A, B, C) tempcontainer_ ## A ## _ ## B ## _ ## C
#define temp_container_named_two_join(A, B, C) temp_container_named_two_join_again(A, B, C)
#define temp_container_named_two(A, B) temp_container_named_two_join(A, B, __LINE__)

#define temp_container(name) tempcontainer_##name

#define temp_function_named_join_again(A, B) tempfunction_ ## A ## _ ## B
#define temp_function_named_join(A, B) temp_function_named_join_again(A, B)
#define temp_function_named(A) temp_function_named_join(A, __LINE__)

#define temp_function(name) tempfunction_##name

#define for_every_iter(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto variableName = (container_##variableName)->begin(), end##variableName = (container_##variableName)->end(); variableName != end##variableName; ++variableName)

#define iter_end(variableName) end##variableName

// continue with iter after this one
#define for_every_iter_continuing_after(variableName, continueAfterVariableName) \
	auto variableName = continueAfterVariableName; \
	++ variableName; \
	for (; variableName != end##continueAfterVariableName; ++ variableName) 

//

// to access for_every's iterator
#define for_everys_iterator(variableName) iterator_##variableName

// key of map iterator
#define for_everys_map_key(variableName) ((iterator_##variableName).get_key())

#define for_everys_index(variableName) ((container_##variableName)->iterators_index(iterator_##variableName))

#define for_every_continues_index(variableName, continueAfterVariableName) ((container_##continueAfterVariableName)->iterators_index(iterator_##variableName))

//
#ifdef AN_CLANG
#define PREFETCH(ptr) __builtin_prefetch(ptr)
#else
#define PREFETCH(ptr) _m_prefetch(ptr)
#endif

#define for_every_prefetch(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin(), end##variableName = (container_##variableName)->end(); \
		PREFETCH(&(*container_##variableName->next_of(iterator_##variableName))), \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto * const variableName = &(*iterator_##variableName))

#define for_every_ptr_prefetch(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin(), end##variableName = (container_##variableName)->end(); \
		PREFETCH(*container_##variableName->next_of(iterator_##variableName)), \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto * const variableName = *iterator_##variableName)

#define for_every_ref_prefetch(variableName, containerObject) \
	auto const & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin(), end##variableName = (container_##variableName)->end(); \
		PREFETCH((&(*container_##variableName->next_of(iterator_##variableName)))->get()), \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto * const variableName = (&(*iterator_##variableName))->get())

#define for_every_const_prefetch(variableName, containerObject) \
	auto const & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin_const(), end##variableName = (container_##variableName)->end_const(); \
		PREFETCH(&(*container_##variableName->next_of(iterator_##variableName))), \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto const * const variableName = &(*iterator_##variableName))

#define for_every_const_ptr_prefetch(variableName, containerObject) \
	auto const & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin_const(), end##variableName = (container_##variableName)->end_const(); \
		PREFETCH(*container_##variableName->next_of(iterator_##variableName)), \
		iterator_##variableName != end##variableName; ++ iterator_##variableName) \
	if (auto const * const variableName = *iterator_##variableName)

//

#define for_every(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin(), end##variableName = (container_##variableName)->end(); \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto * const variableName = &(*iterator_##variableName))

#define for_every_ptr(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin(), end##variableName = (container_##variableName)->end(); \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto * const variableName = *iterator_##variableName)

#define for_every_ref(variableName, containerObject) \
	auto const & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin(), end##variableName = (container_##variableName)->end(); \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto * const variableName = (&(*iterator_##variableName))->get())

#define for_every_const(variableName, containerObject) \
	auto const & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin_const(), end##variableName = (container_##variableName)->end_const(); \
		iterator_##variableName != end##variableName; ++iterator_##variableName) \
	if (auto const * const variableName = &(*iterator_##variableName))

#define for_every_const_ptr(variableName, containerObject) \
	auto const & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->begin_const(), end##variableName = (container_##variableName)->end_const(); \
		iterator_##variableName != end##variableName; ++ iterator_##variableName) \
	if (auto const * const variableName = *iterator_##variableName)

//

#define for_every_continue_after(variableName, continueAfterVariableName) \
	auto iterator_##variableName = iterator_##continueAfterVariableName; \
	++iterator_##variableName; \
	for (; iterator_##variableName != end##continueAfterVariableName; ++iterator_##variableName) \
	if (auto * const variableName = &(*iterator_##variableName))

#define for_every_ptr_continue_after(variableName, continueAfterVariableName) \
	auto iterator_##variableName = iterator_##continueAfterVariableName; \
	++iterator_##variableName; \
	for (; iterator_##variableName != end##continueAfterVariableName; ++iterator_##variableName) \
	if (auto * const variableName = *iterator_##variableName)

#define for_every_const_continue_after(variableName, continueAfterVariableName) \
	auto iterator_##variableName = iterator_##continueAfterVariableName; \
	++iterator_##variableName; \
	for (; iterator_##variableName != end##continueAfterVariableName; ++iterator_##variableName) \
	if (auto const * const variableName = &(*iterator_##variableName))

#define for_every_const_ptr_continue_after(variableName, continueAfterVariableName) \
	auto iterator_##variableName = iterator_##continueAfterVariableName; \
	++iterator_##variableName; \
	for (; iterator_##variableName != end##continueAfterVariableName; ++iterator_##variableName) \
	if (auto const * const variableName = *iterator_##variableName)

#define for_every_ref_continue_after(variableName, continueAfterVariableName) \
	auto iterator_##variableName = iterator_##continueAfterVariableName; \
	++iterator_##variableName; \
	for (; iterator_##variableName != end##continueAfterVariableName; ++iterator_##variableName) \
	if (auto * const variableName = (&(*iterator_##variableName))->get())

// reverse

#define for_every_reverse(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->end(), end##variableName = (container_##variableName)->begin(); iterator_##variableName != end##variableName;) \
	if (auto * const variableName = &(*(--iterator_##variableName)))

#define for_every_reverse_ptr(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->end(), end##variableName = (container_##variableName)->begin(); iterator_##variableName != end##variableName;) \
	if (auto * const variableName = (*(--iterator_##variableName)))

#define for_every_reverse_const(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->end(), end##variableName = (container_##variableName)->begin(); iterator_##variableName != end##variableName;) \
	if (auto const * const variableName = &(*(--iterator_##variableName)))

#define for_every_reverse_const_ptr(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->end(), end##variableName = (container_##variableName)->begin(); iterator_##variableName != end##variableName;) \
	if (auto const * const variableName = (*(--iterator_##variableName)))

#define for_every_reverse_ref(variableName, containerObject) \
	auto & temp_container_named(variableName) = containerObject; \
	if (auto container_##variableName = &temp_container_named(variableName)) \
	for (auto iterator_##variableName = (container_##variableName)->end(), end##variableName = (container_##variableName)->begin(); iterator_##variableName != end##variableName;) \
	if (auto * const variableName = (&(*(--iterator_##variableName)))->get())

#define for_every_reverse_ptr_continue_after(variableName, continueAfterVariableName) \
	auto iterator_##variableName = iterator_##continueAfterVariableName; \
	for (; iterator_##variableName != end##continueAfterVariableName;) \
	if (auto * const variableName = (*(--iterator_##variableName)))

#define replace_if_not_null(a, b) \
	if (auto* v = b) \
	{ \
		a = v; \
	}

