#pragma once

#include "..\globalDefinitions.h"

#include "spinLock.h"

namespace Concurrency
{
	class SpinLock;

	class ScopedSpinLock
	{
	public:
		inline ScopedSpinLock(SpinLock & _lock, bool _allowLockWithin, tchar const * _reason = nullptr);
		inline ScopedSpinLock(SpinLock & _lock, tchar const * _reason = nullptr);
		inline ~ScopedSpinLock();

	private:
		SpinLock & lock;
		bool lockedWithin = false;
	};

	#include "scopedSpinLock.inl"

};
