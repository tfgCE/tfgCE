#pragma once

#include "..\debug\debug.h"

template <typename Class>
struct RefCountObjectPtr
{
public:
	inline RefCountObjectPtr();
	inline RefCountObjectPtr(RefCountObjectPtr const & _source);
	inline explicit RefCountObjectPtr(Class const * _value); // well, this is just to get to that pointer
	inline ~RefCountObjectPtr();

	inline RefCountObjectPtr& operator=(RefCountObjectPtr const & _source);
	inline RefCountObjectPtr& operator=(Class const * _value); // can go through const

	inline bool operator==(RefCountObjectPtr const & _other) const { return value == _other.value; }
	inline bool operator!=(RefCountObjectPtr const & _other) const { return value != _other.value; }
	inline bool operator==(Class const * _other) const { return value == _other; }
	inline bool operator!=(Class const * _other) const { return value != _other; }

	inline Class* operator * ()  const { an_assert_immediate(value); return value; }
	inline Class* operator -> () const { an_assert_immediate(value); return value; }

	inline void clear() { operator=(nullptr); }
	inline Class* get() const { return value; }
	inline bool is_set() const { return value != nullptr; }

	inline int get_ref_count() const { return value? value->get_ref_count() : 0; }

private:
	Class* value;
};

#include "refCountObjectPtr.inl"
