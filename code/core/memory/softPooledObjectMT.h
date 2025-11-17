#pragma once

#include "..\concurrency\spinLock.h"

template <class Class> class SoftPoolMT;
template <class Class> class SoftPooledObjectMT;

#ifdef AN_DEVELOPMENT
#define SOFT_POOL_MT_DISTRIBUTION_CHECKS
//#define SOFT_POOL_MT_EXTENSIVE_CHECKS
#endif

/**
 *	Doesn't call constructor and destructor of object
 *
 *	Similar to SoftPool but stores per thread (where it is released, there it goes)
 *	It's good to redistribute pooled objects among threads.
 *
 *	Should be used only if there is a heavy use per frame and in the very defined time-frames.
 */

template <class Class>
class SoftPoolMT
{
	friend class SoftPooledObjectMT<Class>;

public:
	static void prune(); // deallocates unused
	static void redistribute(); // redistribute among available threads - should be called when we're sure nothing is accessed!

private:
	static SoftPoolMT<Class>* s_pool;
	struct PerThread
	{
		SoftPooledObjectMT<Class>* firstFree = nullptr;
		int count = 0;
	};
	ArrayStatic<PerThread, 64> perThread;

	SoftPoolMT();
	~SoftPoolMT();
	inline static SoftPoolMT<Class>* get() { return s_pool? s_pool : construct_soft_pool(); }
	inline Class* get_one();
	inline void release(SoftPooledObjectMT<Class>* _obj);
	static SoftPoolMT<Class>* construct_soft_pool();
	static void destroy_at_exit(); // destroy soft pool

#ifdef AN_DEVELOPMENT
	Concurrency::SpinLock uniquenessLock = Concurrency::SpinLock(TXT("SoftPoolMT.uniquenessLock"));
	void check_uniqueness();
#endif
	void check_uniqueness_no_lock();
};

template <class Class>
class SoftPooledObjectMT
{
	friend class SoftPoolMT<Class>;

public:
	inline static Class* get_one();
	inline void release();

protected:
	virtual ~SoftPooledObjectMT() {}
	virtual void on_get() {}
	virtual void on_release() {}

private:
	SoftPooledObjectMT* nextInPool = nullptr;
#ifdef AN_DEVELOPMENT
	volatile int softPooledObjectMTUseCount = 0;
	volatile bool softPooledObjectMTReleased = true;
#endif
};

#include "softPooledObjectMT.inl"
