#pragma once

#include "..\..\concurrency\spinLock.h"

namespace System
{
#ifdef AN_SDL
	struct VideoSDL
	{
		static Concurrency::SpinLock imageLock;
	};
#endif
};
