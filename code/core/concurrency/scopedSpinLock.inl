ScopedSpinLock::ScopedSpinLock(SpinLock & _lock, bool _allowLockWithin, tchar const* _reason)
: lock( _lock )
{
	lockedWithin = _allowLockWithin && lock.is_locked_on_this_thread();
	if (!lockedWithin)
	{
		lock.acquire(_reason);
	}
}

ScopedSpinLock::ScopedSpinLock(SpinLock & _lock, tchar const* _reason)
: lock( _lock )
{
	lock.acquire(_reason);
}

ScopedSpinLock::~ScopedSpinLock()
{
	if (!lockedWithin)
	{
		lock.release();
	}
}
