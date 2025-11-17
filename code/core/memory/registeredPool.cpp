#include "registeredPool.h"
#include "..\concurrency\spinLock.h"
#include <stdlib.h>
#include "..\globalDefinitions.h"

RegisteredPool* RegisteredPool::s_registeredPools = nullptr;

RegisteredPool::RegisteredPool()
: nextPool(nullptr)
{
	static Concurrency::SpinLock pool_constructor = Concurrency::SpinLock(TXT("RegisteredPool.RegisteredPool.pool_constructor"));
	pool_constructor.acquire();
	nextPool = s_registeredPools;
	s_registeredPools = this;
	static bool registeredAtExit = false;
	if (!registeredAtExit)
	{
		registeredAtExit = true;
		atexit(&destroy_at_exit);
	}
	pool_constructor.release();
}

RegisteredPool::~RegisteredPool()
{
}

void RegisteredPool::destroy_at_exit()
{
	RegisteredPool* pool = s_registeredPools;
	while (pool)
	{
		RegisteredPool* next = pool->nextPool;
		pool->destroy();
		pool = next;
	}
	s_registeredPools = nullptr;
}

void RegisteredPool::prune_all()
{
	RegisteredPool* pool = s_registeredPools;
	while (pool)
	{
		pool->prune();
		pool = pool->nextPool;
	}
}
