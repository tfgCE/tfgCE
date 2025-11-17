#include "objectHelpers.h"

// specializations

// simple type - plain memory copy, no constructors, no destructors
#define OBJECT_HELPERS_FOR_SIMPLE_TYPE(Class) \
template <> void ObjectHelpers<Class>::copy(Class* dest, Class const* from, Class const* until) { memory_copy(dest, from, sizeof(Class) * (until - from)); } \
template <> void ObjectHelpers<Class>::call_constructor_on(Class* from, Class* until) {} \
template <> void ObjectHelpers<Class>::call_copy_constructor_on(Class* dest, Class const* from, Class const* until) { memory_copy(dest, from, sizeof(Class) * (until - from)); } \
template <> void ObjectHelpers<Class>::call_destructor_on(Class* from, Class* until) {} \

OBJECT_HELPERS_FOR_SIMPLE_TYPE(char)

OBJECT_HELPERS_FOR_SIMPLE_TYPE(int64)
OBJECT_HELPERS_FOR_SIMPLE_TYPE(int32)
OBJECT_HELPERS_FOR_SIMPLE_TYPE(int16)
// OBJECT_HELPERS_FOR_SIMPLE_TYPE(int8) as char

OBJECT_HELPERS_FOR_SIMPLE_TYPE(uint64)
OBJECT_HELPERS_FOR_SIMPLE_TYPE(uint32)
OBJECT_HELPERS_FOR_SIMPLE_TYPE(uint16)
OBJECT_HELPERS_FOR_SIMPLE_TYPE(uint8)

// OBJECT_HELPERS_FOR_SIMPLE_TYPE(byte) as uint8

// OBJECT_HELPERS_FOR_SIMPLE_TYPE(uint) as uint32
