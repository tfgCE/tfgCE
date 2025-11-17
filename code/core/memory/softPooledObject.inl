#include "..\debug\debug.h"
#include "memory.h"
#include "..\debug\detectMemoryLeaks.h"

template <class Class>
SoftPool<Class>* SoftPool<Class>::s_pool = nullptr;

template <class Class>
Concurrency::SpinLock SoftPool<Class>::s_spinLock = Concurrency::SpinLock(TXT("SoftPool"));

template <class Class>
SoftPool<Class>::SoftPool()
:	firstFree( nullptr )
{
	atexit(&destroy_at_exit);
}

template <class Class>
SoftPool<Class>::~SoftPool()
{
}

template <class Class>
SoftPool<Class>* SoftPool<Class>::construct_soft_pool()
{
	static Concurrency::SpinLock pool_constructor;
	pool_constructor.acquire();
	if (!s_pool)
	{
		s_pool = new SoftPool<Class>();
	}
	pool_constructor.release();
	return s_pool;
}

template <class Class>
inline void SoftPool<Class>::destroy_at_exit()
{
	if (s_pool)
	{
		s_pool->prune();
		delete s_pool;
		s_pool = nullptr;
	}
}

template <class Class>
inline void SoftPool<Class>::prune()
{
	SoftPool<Class>& softPool = *SoftPool<Class>::get();

	softPool.s_spinLock.acquire(TXT("prune"));
	// critical section

	while (softPool.firstFree)
	{
		SoftPooledObject<Class>* nextFree = softPool.firstFree->nextInPool;
		delete softPool.firstFree;
		softPool.firstFree = nextFree;
	}

	softPool.firstFree = NULL;

	softPool.s_spinLock.release();
}

template <class Class>
inline Class* SoftPool<Class>::get_one()
{
	SoftPooledObject<Class>* one = nullptr;

	if (!firstFree)
	{
		// don't even bother with locking
		one = new Class();
		one->nextInPool = nullptr;
	}
	else
	{
		s_spinLock.acquire(type_as_char<Class>());
		// critical section

		// check if there is any free
		if (!firstFree)
		{
			s_spinLock.release(); // we still need to create a new one, no need to keep the lock acquired
			one = new Class();
			one->nextInPool = nullptr;
		}
		else
		{
			// use firstFree
			one = firstFree;
			firstFree = firstFree->nextInPool;

			s_spinLock.release();
		}
	}

	return (Class*)one;
}

template <class Class>
inline void SoftPool<Class>::release(SoftPooledObject<Class>* _obj)
{
	s_spinLock.acquire(TXT("release"));
#ifdef AN_DEVELOPMENT_SLOW
	auto* obj = firstFree;
	while (obj)
	{
		an_assert_immediate(obj != _obj, TXT("already released!"));
		obj = obj->nextInPool;
	}
#endif
	// critical section
	((SoftPooledObject<Class>*)_obj)->nextInPool = firstFree;
	firstFree = ((SoftPooledObject<Class>*)_obj);
	s_spinLock.release();
}

template <class Class>
inline Class* SoftPooledObject<Class>::get_one()
{
	Class* one = SoftPool<Class>::get()->get_one();
	one->on_get();
	return one;
}

template <class Class>
inline void SoftPooledObject<Class>::release()
{
	on_release();
	SoftPool<Class>::get()->release(this);
}

