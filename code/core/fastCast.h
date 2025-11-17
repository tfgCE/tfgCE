#pragma once

#include "casts.h"

typedef int FastCastID;

class FastCastRegistry
{
public:
	static FastCastID get_free_id() { while ((++s_fastCastID) == 0) {} return s_fastCastID; }

private:
	static FastCastID s_fastCastID;
};

/**
 *	This is for very fast casting:
 *		FAST_CAST_DECLARE(Class)				declare in class declaration 
 *		FAST_CAST_BASE(BaseClass)				put all base classes like that
 *		FAST_CAST_END()							end of declaration
 *		REGISTER_FOR_FAST_CAST					register in cpp 
 *	It caches casts within object using IDs.
 *	This means that each object is bigger (we have vtable and now that!) but this also means that it will cast much quicker
 */
#define FAST_CAST_DECLARE(Class) \
	public: \
		AN_NOT_CLANG_INLINE static FastCastID fast_cast_id() { return s_fastCastID; } \
		AN_NOT_CLANG_INLINE virtual FastCastID get_fast_cast_id() const { return s_fastCastID; } \
		virtual void const * fast_cast_to(FastCastID _id) const { return fast_cast_to_worker(_id); } \
		virtual void* fast_cast_to(FastCastID _id) { return cast_to_nonconst<void>(fast_cast_to_worker(_id)); } \
	protected: \
		static const FastCastID s_fastCastID; \
		inline void const * fast_cast_to_worker(FastCastID _id) const \
		{ \
			if (Class::fast_cast_id() == _id) return this;

#define FAST_CAST_BASE(Base) \
			if (void const * ret = Base::fast_cast_to_worker(_id)) return ret;

#define FAST_CAST_END() \
			return nullptr; \
		}

#define REGISTER_FOR_FAST_CAST(Class) \
	FastCastID const Class::s_fastCastID = FastCastRegistry::get_free_id();

template <typename ToClass, typename FromClass>
inline ToClass* fast_cast(FromClass* _object)
{
	return _object ? (ToClass*)(_object->fast_cast_to(ToClass::fast_cast_id())) : nullptr;
}

template <typename ToClass, typename FromClass>
inline ToClass const * fast_cast(FromClass const * _object)
{
	return _object ? (ToClass*)(_object->fast_cast_to(ToClass::fast_cast_id())) : nullptr;
}

template <typename ToClass, typename FromClass>
inline bool is_exact_fast_cast(FromClass* _object)
{
	return _object ? _object->get_fast_cast_id() == ToClass::fast_cast_id() : false;
}

