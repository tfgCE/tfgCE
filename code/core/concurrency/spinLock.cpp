#include "spinLock.h"

#include "threadSystemUtils.h"

#include "..\types\name.h"
#include "..\performance\performanceSystem.h"
#include "..\system\android\androidApp.h"

#ifdef AN_DEVELOPMENT
#include "..\debug\debug.h"
#endif

//

using namespace Concurrency;

//

DEFINE_STATIC_NAME(spinLock);

//

#ifdef AN_WINDOWS
uint32 SpinLock::s_locked = 1;
uint32 SpinLock::s_available = 0;
#else
#ifdef AN_LINUX_OR_ANDROID
bool SpinLock::s_locked = true;
bool SpinLock::s_available = false;
#else
#error implement
#endif
#endif

#ifdef AN_CONCURRENCY_STATS
void SpinLock::report_performance_measure_lock(Performance::Timer const & _timer)
{
#ifdef AN_PERFORMANCE_MEASURE
	Performance::BlockToken token;
	Performance::System::add_block(Performance::BlockTag(NAME(spinLock), Performance::BlockType::Lock, Colour::red), _timer, OUT_ token);
#endif
}
#endif			

#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
void SpinLock::report_performance_limit_exceeded(Performance::Timer const& _timer, tchar const* _wasLockedReason, tchar const* _lockReason) const
{
#ifdef AN_ANDROID
	if (!::System::Android::App::get().get_activity())
	{
		return;
	}
#endif
	float timeSince = _timer.get_time_ms();
	warn(TXT("[performance guard] [spin lock] exceeded limit of %.2fms (%.2fms) : %S [%S -> %S]"), waitLimit * 1000.0f, timeSince * 1000.0f,
		safe_tchar_ptr(info), safe_tchar_ptr(_wasLockedReason), safe_tchar_ptr(_lockReason));
}
#endif			

SpinLock::SpinLock()
: lock( s_available )
{
}

SpinLock::SpinLock(bool _systemLock)
: lock(s_available)
#ifdef SPIN_LOCK_SYSTEM_MEMBER
, systemLock(_systemLock)
#endif
{
}

SpinLock::SpinLock(tchar const * _info, float _waitLimit)
: lock(s_available)
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
, info(_info)
, waitLimit(_waitLimit)
#endif
{
}

SpinLock::SpinLock(SpinLock const& _other)
{
#ifdef SPIN_LOCK_SYSTEM_MEMBER
	systemLock = _other.systemLock;
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	info = _other.info;
	waitLimit = _other.waitLimit;
#endif
}

SpinLock& SpinLock::operator=(SpinLock const& _other)
{
#ifdef SPIN_LOCK_SYSTEM_MEMBER
	systemLock = _other.systemLock;
#endif
#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	info = _other.info;
	waitLimit = _other.waitLimit;
#endif
	return *this;
}

int SpinLock::get_current_thread_system_id() const
{
	return ThreadSystemUtils::get_current_thread_system_id();
}

#ifdef AN_DEVELOPMENT
void SpinLock::do_assert(bool _condition, tchar const * _text) const
{
	an_assert_immediate(_condition, _text);
}
#endif