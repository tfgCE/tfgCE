#pragma once

#include "debug\debug.h"

// plain_cast<Type>(pointer) -> Type*
// cast_to_value<Type>(pointer) -> Type&
// slow_cast<Type>(pointer) uses dynamic cast to get among the class hierarchy
// up_cast<Type>(pointer) to cast higher in the class hierarchy (base)
// cast_to_nonconst<Type>(pointer)
// cast_to_const<Type>(pointer)
//
// there's also
// fast_cast<Type>(pointer)

template <typename ClassObject, typename FromClassObject>
inline ClassObject * plain_cast(FromClassObject * _object)
{
	return reinterpret_cast<ClassObject *>(_object);
}

template <typename ClassObject, typename FromClassObject>
inline ClassObject const * plain_cast(FromClassObject const * _object)
{
	return reinterpret_cast<ClassObject const *>(_object);
}

template <typename ClassObject, typename FromClassObject>
inline ClassObject & cast_to_value(FromClassObject * _object)
{
	return *plain_cast<ClassObject>(_object);
}

template <typename ClassObject, typename FromClassObject>
inline ClassObject const & cast_to_value(FromClassObject const * _object)
{
	return *plain_cast<ClassObject>(_object);
}

template <typename ClassObject, typename FromClassObject>
inline ClassObject * slow_cast(FromClassObject * _object)
{
	return dynamic_cast<ClassObject *>(_object);
}

template <typename ClassObject, typename FromClassObject>
inline ClassObject const * slow_cast(FromClassObject const * _object)
{
	return dynamic_cast<ClassObject const *>(_object);
}

template <typename ClassObject, typename FromClassObject>
inline ClassObject * up_cast(FromClassObject * _object)
{
#ifndef AN_CLANG
	an_assert(dynamic_cast<ClassObject *>(_object) != nullptr);
#endif
	return static_cast<ClassObject *>(_object);
}

template <typename ClassObject, typename FromClassObject>
inline ClassObject const * up_cast(FromClassObject const * _object)
{
#ifndef AN_CLANG
	an_assert(dynamic_cast<ClassObject const *>(_object) != nullptr);
#endif
	return static_cast<ClassObject const *>(_object);
}

template <typename ClassObject>
inline ClassObject * cast_to_nonconst(ClassObject const * _object)
{
	return const_cast<ClassObject *>(_object);
}

template <typename ClassObject>
inline ClassObject const * cast_to_const(ClassObject * _object)
{
	return const_cast<ClassObject *>(_object);
}
