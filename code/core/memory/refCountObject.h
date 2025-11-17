#pragma once

#include "..\globalDefinitions.h"

#include "..\concurrency\counter.h"

#include "..\debug\debug.h"

class RefCountObject
{
public:
    inline RefCountObject(); // requires add_ref after creation
	inline RefCountObject(RefCountObject const & _source) {} // do not copy refCount!
	inline RefCountObject& operator=(RefCountObject const & _source) { return *this; } // do not copy refCount!
	
	inline void add_ref() const;
	inline bool release_ref() const;

	inline int get_ref_count() const { return refCount.get(); }

protected:
	inline virtual ~RefCountObject() { an_assert_immediate(refCount.get() == 0, TXT("ref count not 0, is %i"), refCount.get()); }
    inline virtual void destroy_ref_count_object();

private:
	mutable Concurrency::Counter refCount;

	inline void destroy_ref_count_object() const;
};

#include "refCountObject.inl"

#include "refCountObjectPtr.h"
