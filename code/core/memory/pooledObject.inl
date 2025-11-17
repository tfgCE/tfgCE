#include "..\debug\debug.h"
#include "memory.h"
#include "..\debug\detectMemoryLeaks.h"

template <class Class>
Pool<Class>* Pool<Class>::s_pool = nullptr;

template <class Class>
Concurrency::SpinLock Pool<Class>::s_spinLock = Concurrency::SpinLock(TXT("Pool"));

template <class Class>
Pool<Class>::Pool()
: firstFree(nullptr)
#ifdef AN_DEVELOPMENT_OR_PROFILER
, objectNum(0)
, existingNum(0)
#endif
{
#ifdef AN_CONCURRENCY_STATS
	s_spinLock.do_not_report_stats();
#endif
}

template <class Class>
Pool<Class>::~Pool()
{
}

#ifdef AN_CHECK_MEMORY_LEAKS
template <class Class>
void Pool<Class>::provide_pooled_object_name(tchar const* _name, bool _onlyIfNotNamed)
{
	if (!s_pool)
	{
		construct_pool();
	}

	if (_onlyIfNotNamed && !s_pool->pooledObjectNameDefaultName)
	{
		return;
	}
	memory_copy(s_pool->pooledObjectName, _name, sizeof(tchar) * (size_t)(tstrlen(_name) + 1));
	s_pool->pooledObjectNameDefaultName = false;
}
#endif

template <class Class>
Pool<Class>* Pool<Class>::construct_pool()
{
	static Concurrency::SpinLock pool_constructor = Concurrency::SpinLock(TXT("Pool.construct_pool.pool_constructor"));
	pool_constructor.acquire();
	if (!s_pool)
	{
#ifdef AN_CHECK_MEMORY_LEAKS
		supress_memory_leak_detection();
#endif
		s_pool = new Pool<Class>();
#ifdef AN_CHECK_MEMORY_LEAKS
		provide_pooled_object_name(TXT("<unknown pool, name it>"));
		s_pool->pooledObjectNameDefaultName = true;
#endif
#ifdef AN_CHECK_MEMORY_LEAKS
		resume_memory_leak_detection();
#endif
	}
	pool_constructor.release();
	return s_pool;
}

template <class Class>
inline void Pool<Class>::destroy()
{
	prune();
#ifdef AN_DEVELOPMENT
	//an_assert(objectNum == 0 && existingNum == 0, TXT("there are still some objects in use objects:%i existing:%i (check further asserts)"), objectNum, existingNum);
	if (!(objectNum == 0 && existingNum == 0))
	{
		error_dev_ignore(TXT("there are still some objects in use objects:%i existing:%i (check further asserts)"), objectNum, existingNum);
	}
#endif
	s_pool = nullptr;
#ifdef AN_CHECK_MEMORY_LEAKS
	supress_memory_leak_detection();
#endif
	delete this;
#ifdef AN_CHECK_MEMORY_LEAKS
	resume_memory_leak_detection();
#endif
}

template <class Class>
inline void Pool<Class>::prune()
{
	s_spinLock.acquire();
	// critical section

	while (firstFree)
	{
		PooledObject<Class>* nextFree = firstFree->nextInPool;
#ifdef AN_CHECK_MEMORY_LEAKS
		free_memory_for_memory_leaks(firstFree);
#else
		free_memory(firstFree);
#endif
		firstFree = nextFree;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		-- objectNum;
#endif
	}

	firstFree = nullptr;

	s_spinLock.release();
}

template <class Class>
inline void* Pool<Class>::on_new(size_t _size)
{
#ifdef AN_CHECK_MEMORY_LEAKS
	if (pooledObjectNameDefaultName)
	{
		pooledObjectNameDefaultName = true;
	}
#endif
	s_spinLock.acquire();
	// critical section

	PooledObject<Class>* one = nullptr;

	// check if there is any free
	if (! firstFree)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		++objectNum;
		++existingNum;
#endif
		s_spinLock.release(); // we still need to create a new one, no need to keep the lock acquired
#ifdef AN_CHECK_MEMORY_LEAKS
		one = (Class*)allocate_memory_for_memory_leaks(_size);
#else
		one = (Class*)allocate_memory(_size);
#endif
		one->nextInPool = nullptr;
	}
	else
	{
		// use firstFree
		one = firstFree;
		firstFree = firstFree->nextInPool;

		// disconnect from pool
		one->nextInPool = nullptr;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		++existingNum;
#endif
		s_spinLock.release();
	}

#ifdef AN_CHECK_MEMORY_LEAKS
#define MAX_MLI_LENGTH 128
	tchar mli[MAX_MLI_LENGTH];
#ifdef AN_CLANG
	tsprintf(mli, MAX_MLI_LENGTH, TXT("pooled: size:%i class:%S"), _size, pooledObjectName);
#else
	tsprintf_s(mli, TXT("pooled: size:%i class:%s"), (int)_size, pooledObjectName);
#endif
	memory_leak_info(mli);
	on_new_memory(one);
	forget_memory_leak_info;
#undef MAX_MLI_LENGTH
#endif

	return one;
}

template <class Class>
inline void Pool<Class>::on_delete(void* _ptr)
{
	// critical section
	s_spinLock.acquire();
	auto* ptr = ((PooledObject<Class>*)_ptr);
#ifdef AN_DEVELOPMENT
	an_assert(ptr->pooledObjectGuardian == 0x00caca00, TXT("make it first in list of base classes"));
#endif
#ifdef AN_DEVELOPMENT
	PooledObject<Class>* check = firstFree;
	while (check)
	{
		an_assert(check != ptr, TXT("already deleted?"));
		check = check->nextInPool;
	}
#endif
#ifdef AN_CHECK_MEMORY_LEAKS
	on_delete_memory(_ptr);
#endif
	ptr->nextInPool = firstFree;
	firstFree = ptr;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	--existingNum;
#endif
	s_spinLock.release();
}

#pragma push_macro("new")
#undef new
template <class Class>
inline void* PooledObject<Class>::operator new(size_t _size)
{
	return Pool<Class>::get()->on_new(_size);
}
#pragma pop_macro("new")

template <class Class>
inline void PooledObject<Class>::operator delete(void* _ptr)
{
	Pool<Class>::get()->on_delete(_ptr);
}

