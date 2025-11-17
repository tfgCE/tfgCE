#ifdef AN_WINDOWS
#include <Windows.h>
#endif

int32 Concurrency::Counter::increase()
{
#ifdef AN_WINDOWS
	return InterlockedIncrement((uint32*)&counter);
#else
#ifdef AN_LINUX_OR_ANDROID
	return ++ counter;
#else
#error implement
#endif
#endif
}

int32 Concurrency::Counter::decrease()
{
#ifdef AN_WINDOWS
	return InterlockedDecrement((uint32*)&counter);
#else
#ifdef AN_LINUX_OR_ANDROID
	return -- counter;
#else
#error implement
#endif
#endif
}

Concurrency::Counter& Concurrency::Counter::operator = (int32 _counter)
{
	counter = _counter;
	return *this;
}

Concurrency::Counter& Concurrency::Counter::operator = (Counter const & _other)
{
#ifdef AN_CLANG
	counter = _other.counter.load(std::memory_order_relaxed);
#else
	counter = _other.counter;
#endif
	return *this;
}

void Concurrency::Counter::wait_till(int32 _counter)
{
#ifdef AN_CONCURRENCY_STATS
	bool waited = false;
#endif
	while (counter != _counter)
	{
#ifdef AN_CONCURRENCY_STATS
		waited = true;
#endif
	}
#ifdef AN_CONCURRENCY_STATS
	Concurrency::Stats::waited_in_counter(waited);
#endif
}

