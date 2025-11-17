#include "..\debug\debug.h"
#include "memory.h"
#include "..\concurrency\threadManager.h"
#include "..\debug\detectMemoryLeaks.h"

template <class Class>
SoftPoolMT<Class>* SoftPoolMT<Class>::s_pool = nullptr;

template <class Class>
SoftPoolMT<Class>::SoftPoolMT()
{
	SET_EXTRA_DEBUG_INFO(perThread, TXT("SoftPoolMT.perThread"));

	while (perThread.has_place_left())
	{
		perThread.push_back(PerThread());
	}
	atexit(&destroy_at_exit);
}

template <class Class>
SoftPoolMT<Class>::~SoftPoolMT()
{
}

template <class Class>
SoftPoolMT<Class>* SoftPoolMT<Class>::construct_soft_pool()
{
	static Concurrency::SpinLock pool_constructor;
	pool_constructor.acquire();
	if (!s_pool)
	{
		s_pool = new SoftPoolMT<Class>();
	}
	pool_constructor.release();
	return s_pool;
}

template <class Class>
inline void SoftPoolMT<Class>::destroy_at_exit()
{
	if (s_pool)
	{
		s_pool->prune();
		delete s_pool;
		s_pool = nullptr;
	}
}

template <class Class>
inline void SoftPoolMT<Class>::prune()
{
	SoftPoolMT<Class>& softPool = *SoftPoolMT<Class>::get();

	for_every(pt, softPool.perThread)
	{
		auto& t = *pt;

		auto& firstFreeT = t.firstFree;

		while (firstFreeT)
		{
			SoftPooledObjectMT<Class>* nextFree = firstFreeT->nextInPool;
			delete firstFreeT;
			firstFreeT = nextFree;
		}

		firstFreeT = NULL;
		t.count = 0;
	}
}

template <class Class>
void SoftPoolMT<Class>::redistribute()
{
	SoftPoolMT<Class>& softPool = *SoftPoolMT<Class>::get();

	int threadCount = Concurrency::ThreadManager::get_thread_count();

#ifdef SOFT_POOL_MT_DISTRIBUTION_CHECKS
	softPool.check_uniqueness();
#endif

	int redistributionsLeft = 5;
	while (redistributionsLeft > 0)
	{
		--redistributionsLeft;
		int minIdx = 0;
		int maxIdx = 0;
		int minFF = softPool.perThread[minIdx].count;
		int maxFF = minFF;
		{
			auto* pt = softPool.perThread.begin();
			for_count(int, i, threadCount)
			{
				if (pt->count < minFF)
				{
					minFF = pt->count;
					minIdx = i;
				}
				if (pt->count > maxFF)
				{
					maxFF = pt->count;
					maxIdx = i;
				}
				++pt;
			}
		}

		int acceptableDiff = max(1, (maxFF - minFF) / 10);
		if (maxFF > minFF + acceptableDiff)
		{
			auto& fromPT = softPool.perThread[maxIdx];
			while (maxFF > minFF + acceptableDiff && maxIdx != minIdx)
			{
				auto& toPT = softPool.perThread[minIdx];

				auto* moveOne = fromPT.firstFree;
				an_assert(fromPT.firstFree);
				fromPT.firstFree = fromPT.firstFree->nextInPool;
				--fromPT.count;
				moveOne->nextInPool = toPT.firstFree;
				toPT.firstFree = moveOne;
				++toPT.count;
				--maxFF;
				++minFF;
				{
					auto* pt = softPool.perThread.begin();
					for_count(int, i, threadCount)
					{
						if (pt->count < minFF)
						{
							minFF = pt->count;
							minIdx = i;
						}
						++pt;
					}
				}
			}

			continue;
		}
		else
		{
			break;
		}
	}
#ifdef SOFT_POOL_MT_DISTRIBUTION_CHECKS
	softPool.check_uniqueness();
#endif
}

template <class Class>
inline Class* SoftPoolMT<Class>::get_one()
{
#ifdef SOFT_POOL_MT_EXTENSIVE_CHECKS
	Concurrency::ScopedSpinLock lock(uniquenessLock);
#endif

	auto& t = perThread[Concurrency::ThreadManager::get_current_thread_id()];
	auto& firstFreeT = t.firstFree;

	// check if there is any free
	if (firstFreeT)
	{
		// use firstFreeT
		SoftPooledObjectMT<Class>* found = firstFreeT;
		firstFreeT = firstFreeT->nextInPool;
		found->nextInPool = nullptr;
		--t.count;
		an_assert(t.count >= 0);

		return (Class*)found;
	}
	else
	{
		return new Class();
	}	
}

template <class Class>
inline void SoftPoolMT<Class>::release(SoftPooledObjectMT<Class>* _obj)
{
	{
#ifdef EXTENSIVE_CHECLS
		Concurrency::ScopedSpinLock lock(uniquenessLock);
#endif

		auto& t = perThread[Concurrency::ThreadManager::get_current_thread_id()];
		auto& firstFreeT = t.firstFree;

		((SoftPooledObjectMT<Class>*)_obj)->nextInPool = firstFreeT;
		firstFreeT = ((SoftPooledObjectMT<Class>*)_obj);
		++t.count;
	}

#ifdef SOFT_POOL_MT_EXTENSIVE_CHECKS
	check_uniqueness();
#endif
}

#ifdef AN_DEVELOPMENT
template <class Class>
inline void SoftPoolMT<Class>::check_uniqueness()
{
#ifdef SOFT_POOL_MT_EXTENSIVE_CHECKS
	Concurrency::ScopedSpinLock lock(uniquenessLock);
#endif
	
	check_uniqueness_no_lock();
}
#endif

template <class Class>
inline void SoftPoolMT<Class>::check_uniqueness_no_lock()
{
	int threadCount = Concurrency::ThreadManager::get_thread_count();

	for_count(int, checkThreadIdx, threadCount)
	{
		auto* o = perThread[checkThreadIdx].firstFree;
		int oIdx = 0;
		while (o)
		{
			// check other pools
			for_count(int, checkOtherThreadIdx, threadCount)
			{
				auto* other = perThread[checkOtherThreadIdx].firstFree;
				int otherIdx = 0;
				while (other)
				{
					an_assert_immediate(o != other || (oIdx == otherIdx && checkThreadIdx == checkOtherThreadIdx));
					other = other->nextInPool;
					++otherIdx;
				}
			}
			o = o->nextInPool;
			++oIdx;
		}
	}
}

//

template <class Class>
inline Class* SoftPooledObjectMT<Class>::get_one()
{
	Class* one = SoftPoolMT<Class>::get()->get_one();
#ifdef AN_DEVELOPMENT
	an_assert_immediate(one->softPooledObjectMTReleased);
	one->softPooledObjectMTReleased = false;
	++one->softPooledObjectMTUseCount;
#endif
	one->on_get();
	return one;
}

template <class Class>
inline void SoftPooledObjectMT<Class>::release()
{
#ifdef AN_DEVELOPMENT
	an_assert_immediate(!softPooledObjectMTReleased);
	softPooledObjectMTReleased = true;
#endif
	on_release();
	SoftPoolMT<Class>::get()->release(this);
}
