#include "performanceBlock.h"

#include "performanceUtils.h"

using namespace Performance;

BlockID Block::s_id(1);
Concurrency::SpinLock Block::s_idLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

BlockID Block::get_next_id()
{
	if (!Performance::is_enabled())
	{
		return 0;
	}
#ifdef AN_CONCURRENCY_STATS
	s_idLock.do_not_report_stats();
#endif
	s_idLock.acquire();
	do
	{
		++s_id;
	}
	while (s_id == 0);
	BlockID result = s_id;
	s_idLock.release();
	return result;
}

Block::Block(BlockTag const & _tag)
: tag(_tag)
, id(get_next_id())
{
}

Block::~Block()
{
	if (!Performance::is_enabled())
	{
		return;
	}
	id = 0;
}

void Block::turn_into_copy()
{
	if (!Performance::is_enabled())
	{
		return;
	}
	id = get_next_id();
}
