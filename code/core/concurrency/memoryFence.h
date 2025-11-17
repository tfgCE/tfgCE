#pragma once

#include "..\globalDefinitions.h"
#ifdef AN_LINUX_OR_ANDROID
#include "..\debug\debug.h"
#endif

namespace Concurrency
{
	/** used to synchronise memory when dealing with multiple threads. */
	inline void memory_fence()
	{
#ifdef AN_WINDOWS
		MemoryBarrier();
#else
#ifdef AN_LINUX_OR_ANDROID
		todo_note(TXT("implement memory fence"));
#else
#error TODO implement
#endif
#endif
	}
}
