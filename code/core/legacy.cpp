#include <stdio.h>

// legacy for switching from v120 to v140

#ifndef AN_CLANG
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}
#endif
