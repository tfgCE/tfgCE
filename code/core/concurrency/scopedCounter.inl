ScopedCounter::ScopedCounter(Counter* _counter)
: counter(_counter)
{
	if (counter)
	{
		counter->increase();
	}
}

ScopedCounter::ScopedCounter(Counter & _counter)
: counter(&_counter)
{
	if (counter)
	{
		counter->increase();
	}
}

ScopedCounter::~ScopedCounter()
{
	if (counter)
	{
		counter->decrease();
	}
}