#include "libraryLoadSummary.h"

#include "..\..\core\concurrency\scopedSpinLock.h"

//

using namespace Framework;

//

void LibraryLoadSummary::loaded(LibraryStored* _loaded)
{
	Concurrency::ScopedSpinLock lockNow(lock);
	loadedObjects.push_back(_loaded);
}