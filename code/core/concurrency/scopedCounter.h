#pragma once

#include "..\globalDefinitions.h"

#include "counter.h"

namespace Concurrency
{
	class ScopedCounter
	{
	public:
		inline ScopedCounter(Counter* _counter);
		inline ScopedCounter(Counter& _counter);
		inline ~ScopedCounter();

	private:
		Counter * counter;
	};

	#include "scopedCounter.inl"

};
