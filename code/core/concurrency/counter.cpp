#include "counter.h"

using namespace Concurrency;

Counter::Counter()
: counter( 0 )
{
}

Counter::Counter(int32 _counter)
: counter( _counter )
{
}

Counter::Counter(Counter const & _counter)
{
	counter = _counter;
}



