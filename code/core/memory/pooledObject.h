#pragma once

#include "..\concurrency\spinLock.h"
#include "registeredPool.h"

#ifdef AN_CHECK_MEMORY_LEAKS
#define NAME_POOLED_OBJECT(Class) Pool<Class>::provide_pooled_object_name(TXT(#Class));
#define NAME_POOLED_OBJECT_IF_NOT_NAMED(Class) Pool<Class>::provide_pooled_object_name(TXT(#Class), true);
#else
#define NAME_POOLED_OBJECT(name)
#define NAME_POOLED_OBJECT_IF_NOT_NAMED(Class)
#endif

template <class Class> class Pool;
template <class Class> class PooledObject;

template <class Class>
class Pool
: public RegisteredPool
{
	friend class PooledObject<Class>;

public:
	override_ void prune(); // deallocates unused

#ifdef AN_CHECK_MEMORY_LEAKS
	inline static void provide_pooled_object_name(tchar const* _name, bool _onlyIfNotNamed = false);
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	static int get_total_count() { return s_pool? s_pool->objectNum : 0; }
	static int get_used_count() { return s_pool? s_pool->existingNum : 0; }
#endif

protected:
	override_ void destroy(); // prunes and destroys pool (only pool!)

private:
	static Pool<Class>* s_pool;
	static Concurrency::SpinLock s_spinLock;
	PooledObject<Class>* firstFree;
#ifdef AN_CHECK_MEMORY_LEAKS
	tchar pooledObjectName[128];
	bool pooledObjectNameDefaultName = true;
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	int objectNum; // total
	int existingNum; // used
#endif

	Pool();
	virtual ~Pool();
	inline static Pool<Class>* get() { return s_pool? s_pool : construct_pool(); }
	inline void* on_new(size_t _size);
	inline void on_delete(void* _ptr);
	static Pool<Class>* construct_pool();
};

#pragma push_macro("new")
#undef new

template <class Class>
class PooledObject // has to be first in the base class list, because of deletion.
{
	friend class Pool<Class>;

public:
	inline static void* operator new(size_t _size);
	inline static void operator delete(void* _ptr);

	virtual ~PooledObject() {} // to catch deletes

private:
#ifdef AN_DEVELOPMENT
	int pooledObjectGuardian = 0x00caca00;
#endif
	PooledObject* nextInPool;
};

#pragma pop_macro("new")

#include "pooledObject.inl"
