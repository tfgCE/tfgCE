#pragma once

#include "performanceBlock.h"
#include "performanceSystem.h"

namespace Performance
{
	class ScopedMeasureBlock
	{
	public:
		ScopedMeasureBlock(BlockTag const & _tag, bool _lazy = false);
		~ScopedMeasureBlock();

		void invalidate() { an_assert(lazy, TXT("can invalidate only lazy blocks")); lazy = false; } // this will make block not register at all
	private:
		BlockTag tag;
		BlockToken token;
		Timer timer;
		bool lazy;
	};

};
