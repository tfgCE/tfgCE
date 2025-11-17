#pragma once

#include "..\globalSettings.h"

namespace Concurrency
{
	struct Stats
	{
		static void next_frame();
		static Stats const & get_last_frame();

		static void waited_in_counter(bool _waited);
		static void waited_in_spin_lock(bool _waited);
		static void waited_in_semaphore(bool _waited);
		static void waited_in_mrsw_lock_read(bool _waited);
		static void waited_in_mrsw_lock_write(bool _waited);

		int wasInCounter = 0;
		int wasInSpinLock = 0;
		int wasInSemaphore = 0;
		int wasInMRSWLockRead = 0;
		int wasInMRSWLockWrite = 0;
		int waitedInCounter = 0;
		int waitedInSpinLock = 0;
		int waitedInSemaphore = 0;
		int waitedInMRSWLockRead = 0;
		int waitedInMRSWLockWrite = 0;
	};
};
