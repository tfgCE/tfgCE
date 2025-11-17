#include "scopedMeasureBlock.h"

using namespace Performance;

ScopedMeasureBlock::ScopedMeasureBlock(BlockTag const & _tag, bool _lazy)
: tag(_tag)
, lazy(_lazy)
{
	timer.start();
	if (!lazy)
	{
		Performance::System::add_block(tag, timer, OUT_ token);
	}
}

ScopedMeasureBlock::~ScopedMeasureBlock()
{
	if (lazy)
	{
		Performance::System::add_block(tag, timer, OUT_ token);
	}
	if (token.is_valid())
	{
		timer.stop();
		Performance::System::update_block(tag, timer, token);
	}
}
