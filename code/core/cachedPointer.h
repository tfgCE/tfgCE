#pragma once

#include "casts.h"
#include "fastCast.h"

struct CachedPointer
{
public:
	CachedPointer()
	: pointer(nullptr)
	{}

	template <typename ToClass, typename FromClass>
	void setup(FromClass* _from)
	{
		pointer = fast_cast<ToClass>(_from);
	}

	template <typename ToClass>
	inline ToClass* get_as() const
	{
		// todo - some check if it is proper pointer? an_assert(slow_cast<ToClass>(plain_cast<ToClass>(pointer)) == pointer);
		return plain_cast<ToClass>(pointer);
	}

private:
	void* pointer;
};

template <typename Class>
struct TypedCachedPointer
: protected CachedPointer
{
public:
	template <typename FromClass>
	void setup(FromClass* _from)
	{
		CachedPointer::setup<Class, FromClass>(_from);
	}

	inline Class * get() const
	{
		return CachedPointer::get_as<Class>();
	}
};