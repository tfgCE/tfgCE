#pragma once

#include "pooledObject.h"
#include "refCountObject.h"

/**
 *	SafeObject does not manage memory, it just makes it possible to access object while it exists (is available).
 *	This allows objects to be deleted (or put into pool or just inactivated) in any way and pointer made unavailable.
 *
 *	How it works?
 *
 *	SafeObject <--1--> SafeObjectPoint <--n-- SafePtr
 *	
 *	SafeObject contains pointer to one SafeObjectPoint that allows objects to be available or not (by pointing at SafeObject)
 *	SafePtr::get() checks through SafeObjectPoint (if set) if object is available (if pointer to SafeObject is set)
 *
 *	Code:
 *		remember to call before destruction/on destruction:
 *			make_safe_object_unavailable();
 *
 */
template <typename Class> class SafeObject;
template <typename Class> struct SafeObjectPoint;
template <typename Class> struct SafePtr;

template <typename Class>
struct SafeObjectPoint
: public PooledObject<SafeObjectPoint<Class>>
, public RefCountObject
{
private:
	typedef RefCountObjectPtr<SafeObjectPoint<Class>> Ptr;
#ifndef AN_CLANG
	typedef SafeObject<Class> SafeObject;
	typedef SafePtr<Class> SafePtr;
	friend class SafeObject;
	friend struct SafePtr;
#else
	friend class SafeObject<Class>;
	friend struct SafePtr<Class>;
#endif

private:
#ifndef AN_CLANG
	inline SafeObjectPoint(SafeObject* _safeObject);
#else
	inline SafeObjectPoint(SafeObject<Class>* _safeObject);
#endif
	inline ~SafeObjectPoint();

	inline bool operator==(Class const * _other) const { return get() == _other; }
	inline bool operator!=(Class const * _other) const { return get() != _other; }

	inline void make_safe_object_unavailable();

	inline bool is_set() const { return safeObject != nullptr; }
	inline Class* get() { return safeObject ? safeObject->object : nullptr; }

private:
#ifndef AN_CLANG
	SafeObject* safeObject = nullptr;
#else
	SafeObject<Class>* safeObject = nullptr;
#endif

	// not allowed
	SafeObjectPoint(SafeObjectPoint const & _source);
	SafeObjectPoint& operator=(SafeObjectPoint const & _source);
};

template <typename Class>
class SafeObject
{
private:
#ifndef AN_CLANG
	typedef SafeObjectPoint<Class> SafeObjectPoint;
	typedef SafePtr<Class> SafePtr;

	friend struct SafeObjectPoint;
	friend struct SafePtr;
#else
	friend struct SafeObjectPoint<Class>;
	friend struct SafePtr<Class>;
#endif

public:
    inline SafeObject(Class* _availableAsObject); // sometimes we may not want to make object available through safe pointer right with the constructor, use make_safe_object_available then

	// this should be overriden if multiple SafeObject derived
	inline bool is_available_as_safe_object() const { return safeObjectPoint.is_set(); }

protected:
	// these should be overriden if multiple SafeObject derived
	// and should be called only from derived class (that's why it's protected!)
	inline void make_safe_object_available(Class* _object); // use only if was not made available via constructor
	inline void make_safe_object_unavailable();

protected:
	inline virtual ~SafeObject();

private:
	Class* object = nullptr;
#ifndef AN_CLANG
	typename SafeObjectPoint::Ptr safeObjectPoint;
#else
	typename SafeObjectPoint<Class>::Ptr safeObjectPoint;
#endif

	// not allowed
	SafeObject(SafeObject const & _source);
	SafeObject& operator=(SafeObject const & _source);
};

template <typename Class>
struct SafePtr
{
private:
#ifndef AN_CLANG
	typedef SafeObject<Class> SafeObject;
	typedef SafeObjectPoint<Class> SafeObjectPoint;

	friend class SafeObject;
	friend struct SafeObjectPoint;
#else
	friend class SafeObject<Class>;
	friend struct SafeObjectPoint<Class>;
#endif

public:
	inline SafePtr();
	inline SafePtr(SafePtr const & _source);
	inline explicit SafePtr(Class const * _object);
	inline ~SafePtr();

	inline SafePtr& operator=(SafePtr const & _source);
	inline SafePtr& operator=(Class const * _object);

	inline bool operator==(SafePtr const & _other) const { return point == _other.point; }
	inline bool operator!=(SafePtr const & _other) const { return point != _other.point; }
	inline bool operator==(Class const * _other) const { return get() == _other; }
	inline bool operator!=(Class const * _other) const { return get() != _other; }

	inline Class* operator * ()  const { an_assert(point.is_set() && point->is_set()); return point->get(); }
	inline Class* operator -> () const { an_assert(point.is_set() && point->is_set()); return point->get(); }

	inline void clear() { operator=(nullptr); }
	inline Class* get() const { return point.is_set() ? point->get() : nullptr; }
	inline bool is_set() const { return point.is_set() && point->is_set(); }
	inline bool is_pointing_at_something() const { return point.is_set(); } // to check if we are pointing at something, even it that thing is now lost

private:
#ifndef AN_CLANG
	typename SafeObjectPoint::Ptr point;
#else
	typename SafeObjectPoint<Class>::Ptr point;
#endif
};

#include "safeObject.inl"
