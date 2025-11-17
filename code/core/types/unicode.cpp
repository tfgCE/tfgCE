#include "unicode.h"

#include "string.h"

#include "..\concurrency\scopedSpinLock.h"
#include "..\containers\arrayStatic.h"

#include "..\debug\debug.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_UNICODE

//

// starts at 128
// special characters:
//	160 (hardcoded)
struct UnicodeImpl
{
	ArrayStatic<uint32, 65536> lookUp;
	uint const startIdx = 128;
	uint count = 0;
	Concurrency::SpinLock mapLock;
};
UnicodeImpl unicodeImpl;

//

void Unicode::initialise_static()
{
	unicodeImpl.lookUp.set_size(unicodeImpl.lookUp.get_max_size());
}

void Unicode::close_static()
{
}

tchar Unicode::unicode_to_tchar(uint32 _unicode)
{
	if (_unicode < unicodeImpl.startIdx)
	{
		return (tchar)_unicode;
	}
	else if (_unicode == String::no_break_space_char())
	{
		return (tchar)_unicode;
	}
	else
	{
		an_assert(!unicodeImpl.lookUp.is_empty(), TXT("call Unicode::initialise_static first"));
		{
			// first check without locking
			uint32* u = &unicodeImpl.lookUp[unicodeImpl.startIdx];
			uint stopAt = unicodeImpl.count;
			for (uint i = unicodeImpl.startIdx; i < stopAt; ++i, ++u)
			{
				if (*u == _unicode)
				{
					return (tchar)i;
				}
			}
		}
		{	// second check with lock and adding a character (as most likely we don't have it yet in the array)
			Concurrency::ScopedSpinLock lock(unicodeImpl.mapLock);
			uint32* u = &unicodeImpl.lookUp[unicodeImpl.startIdx];
			unicodeImpl.count = max(unicodeImpl.count, unicodeImpl.startIdx);
			for (uint i = unicodeImpl.startIdx; i < unicodeImpl.count; ++i, ++u)
			{
				if (*u == _unicode)
				{
					return (tchar)i;
				}
			}
			// skip
			if (unicodeImpl.count == String::no_break_space_char())
			{
				++unicodeImpl.count;
			}
			an_assert(unicodeImpl.count < (uint)unicodeImpl.lookUp.get_size());
			unicodeImpl.lookUp[unicodeImpl.count] = _unicode;
			++unicodeImpl.count;
#ifdef DEBUG_UNICODE
			output(TXT("[unicode] added %04i = %8i = %c"), unicodeImpl.count - 1, _unicode, (tchar)(unicodeImpl.count - 1));
#endif
			return unicodeImpl.count - 1;
		}
	}
}

uint32 Unicode::tchar_to_unicode(tchar _ch)
{
	if (_ch < unicodeImpl.startIdx)
	{
		return (uint32)_ch;
	}
	else if (_ch == String::no_break_space_char())
	{
		return (uint32)_ch;
	}
	else
	{
		an_assert(_ch < unicodeImpl.count);
		return unicodeImpl.lookUp[_ch];
	}
}
