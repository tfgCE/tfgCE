#include "wmp_uniqueInt.h"

#include "..\concurrency\scopedSpinLock.h"

//

using namespace WheresMyPoint;

//

Concurrency::SpinLock g_uniqueIntLock;
int g_uniqueInt = 0;

//

bool UniqueInt::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	{
		Concurrency::ScopedSpinLock lock(g_uniqueIntLock);
		_value.set(g_uniqueInt);
		++g_uniqueInt;
	}

	return result;
}
