ScopedMRSWLockRead::ScopedMRSWLockRead(MRSWLock& _lock, bool _allowLockWithinWrite, tchar const* _reason)
: lock(_lock)
{
	lockedWithin = _allowLockWithinWrite && lock.is_acquired_write_on_this_thread();
	if (!lockedWithin)
	{
		lock.acquire_read(_reason);
	}
}

ScopedMRSWLockRead::ScopedMRSWLockRead(MRSWLock& _lock, tchar const* _reason)
: lock(_lock)
{
	lock.acquire_read(_reason);
}

ScopedMRSWLockRead::~ScopedMRSWLockRead()
{
	if (!lockedWithin)
	{
		lock.release_read();
	}
}

//

ScopedMRSWLockWrite::ScopedMRSWLockWrite(MRSWLock& _lock, bool _allowLockWriteOnThisThread, tchar const* _reason)
: lock(_lock)
{
	lockedWithin = _allowLockWriteOnThisThread && lock.is_acquired_write_on_this_thread();
	if (!lockedWithin)
	{
		lock.acquire_write(_reason);
	}
}

ScopedMRSWLockWrite::ScopedMRSWLockWrite(MRSWLock& _lock, tchar const* _reason)
: lock(_lock)
{
	lock.acquire_write(_reason);
}

ScopedMRSWLockWrite::~ScopedMRSWLockWrite()
{
	if (!lockedWithin)
	{
		lock.release_write();
	}
}

