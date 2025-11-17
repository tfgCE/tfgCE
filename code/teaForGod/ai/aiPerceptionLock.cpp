#include "aiPerceptionLock.h"

#include "..\..\core\debug\logInfoContext.h"

//

using namespace TeaForGodEmperor;
using namespace AI;

//

PerceptionLock PerceptionLock::s_empty;

PerceptionLock::PerceptionLock()
{
	SET_EXTRA_DEBUG_INFO(lockOverrides, TXT("PerceptionLock.lockOverrides"));
}

PerceptionLock::Type PerceptionLock::get() const
{
	if (!lockOverrides.is_empty())
	{
		Type lockOverriden = Min;
		for_every(lo, lockOverrides)
		{
			lockOverriden = max(lo->lock, lockOverriden);
		}
		return lockOverriden;
	}
	return lock;
}

void PerceptionLock::set_base_lock(Type _lock)
{
	lock = _lock;
}

bool PerceptionLock::is_overriden_lock_set(Name const & _reason) const
{
	for_every(lo, lockOverrides)
	{
		if (lo->reason == _reason)
		{
			return true;
		}
	}
	return false;
}

void PerceptionLock::override_lock(Name const & _reason, Optional<Type> const & _lock)
{
	for_every(lo, lockOverrides)
	{
		if (lo->reason == _reason)
		{
			if (_lock.is_set())
			{
				lo->lock = _lock.get();
			}
			else
			{
				lockOverrides.remove_fast_at(for_everys_index(lo));
			}
			return;
		}
	}
	if (_lock.is_set())
	{
		LockOverride lo;
		lo.reason = _reason;
		lo.lock = _lock.get();
		lockOverrides.push_back(lo);
	}
}

void PerceptionLock::log(LogInfoContext & _context) const
{
	_context.log(TXT("lock state: %S"), to_char(get()));
	_context.increase_indent();
	_context.log(TXT("lock: %S"), to_char(lock));
	for_every(lo, lockOverrides)
	{
		_context.log(TXT("%S: %S"), lo->reason.to_char(), to_char(lo->lock));
	}
	_context.decrease_indent();
}

tchar const* PerceptionLock::to_char(Type _type) const
{
	if (_type == LookForNew) return TXT("look for new");
	if (_type == KeepButAllowToChange) return TXT("keep but allow to change");
	if (_type == Lock) return TXT("lock");
	return TXT("??");
}
