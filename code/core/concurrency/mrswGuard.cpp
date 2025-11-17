#include "mrswGuard.h"

#include "..\debug\debug.h"

using namespace Concurrency;

/**
 *	It's quite naive implementation, but if we would require to use it more and it would appear to be a bottleneck,
 *	it should be investigated to improve.
 */

MRSWGuard::MRSWGuard()
{
}

void MRSWGuard::acquire_read()
{
#ifdef AN_DEVELOPMENT
	an_assert_immediate(activeWriters.get() == 0, TXT("no one should be writing at this point!"));

	activeReaders.increase();
#endif
}

void MRSWGuard::release_read()
{
#ifdef AN_DEVELOPMENT
	activeReaders.decrease();
#endif
}

void MRSWGuard::acquire_write()
{
#ifdef AN_DEVELOPMENT
	an_assert_immediate(activeReaders.get() == 0, TXT("no one should be reading at this point!"));
	an_assert_immediate(activeWriters.get() == 0, TXT("I should be the only one who writes!"));

	activeWriters.increase();
#endif
}

void MRSWGuard::release_write()
{
#ifdef AN_DEVELOPMENT
	activeWriters.decrease();
#endif
}

#ifdef AN_CONCURRENCY_STATS
void MRSWGuard::do_not_report_stats()
{
	reportStats = false;
}
#endif
