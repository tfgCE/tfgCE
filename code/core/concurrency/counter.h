#pragma once

#include "..\globalDefinitions.h"

#include "concurrencyStats.h"

namespace Concurrency
{
	class Counter
	{
	public:
		Counter();
		Counter(int32 _counter);
		Counter(Counter const & _other);

		inline void wait_till(int32 _counter);

		// returns incremented or decreased value
		inline int32 increase();
		inline int32 decrease();

		inline int32 get() const { return counter; }

		inline operator int32() const { return counter; }
		inline Counter& operator = (int32 _counter);
		inline Counter& operator = (Counter const & _other);

		inline bool operator == (int _counter) const { return counter == _counter; }
		inline bool operator != (int _counter) const { return counter != _counter; }

	private:
#ifdef AN_WINDOWS
		volatile int32 counter;
#else
#ifdef AN_LINUX_OR_ANDROID
		std::atomic<int32> counter;
#else
#error implement
#endif
#endif
	};

};

#include "counter.inl"

