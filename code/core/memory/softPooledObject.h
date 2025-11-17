#pragma once

#include "..\concurrency\spinLock.h"

template <class Class> class SoftPool;
template <class Class> class SoftPooledObject;

/**
 *	Doesn't call constructor and destructor of object
 */

template <class Class>
class SoftPool
{
	friend class SoftPooledObject<Class>;

	// TODO add global storage of SoftPools and make it possible to prune all SoftPools
public:
	static void prune(); // deallocates unused

private:
	static SoftPool<Class>* s_pool;
	static Concurrency::SpinLock s_spinLock;
	SoftPooledObject<Class>* firstFree;

	SoftPool();
	~SoftPool();
	inline static SoftPool<Class>* get() { return s_pool? s_pool : construct_soft_pool(); }
	inline Class* get_one();
	inline void release(SoftPooledObject<Class>* _obj);
	static SoftPool<Class>* construct_soft_pool();
	static void destroy_at_exit(); // destroy soft pool
};

template <class Class>
class SoftPooledObject
{
	friend class SoftPool<Class>;

public:
	inline static Class* get_one();
	inline void release();

protected:
	virtual ~SoftPooledObject() {}
	virtual void on_get() {}
	virtual void on_release() {}

private:
	SoftPooledObject* nextInPool;
};

#include "softPooledObject.inl"
