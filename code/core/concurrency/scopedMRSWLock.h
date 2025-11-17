#pragma once

#include "..\globalDefinitions.h"

#include "mrswLock.h"

namespace Concurrency
{
	class MRSWLock;

	class ScopedMRSWLockRead
	{
	public:
		inline ScopedMRSWLockRead(MRSWLock& _lock, bool _allowLockWithinWrite, tchar const* _reason = nullptr);
		inline ScopedMRSWLockRead(MRSWLock& _lock, tchar const* _reason = nullptr);
		inline ~ScopedMRSWLockRead();

	private:
		MRSWLock & lock;
		bool lockedWithin = false;
	};

	class ScopedMRSWLockWrite
	{
	public:
		inline ScopedMRSWLockWrite(MRSWLock& _lock, bool _allowLockWriteOnThisThread, tchar const* _reason = nullptr);
		inline ScopedMRSWLockWrite(MRSWLock& _lock, tchar const* _reason = nullptr);
		inline ~ScopedMRSWLockWrite();

	private:
		MRSWLock & lock;
		bool lockedWithin = false;
	};

	#include "scopedMRSWLock.inl"

};
