#pragma once

#include "..\globalDefinitions.h"

#include "mrswGuard.h"

#ifdef AN_DEVELOPMENT
#define MRSW_GUARD_READ_SCOPE(name) ::Concurrency::ScopedMRSWGuardRead temp_variable_named(guardReadScope)(name)
#define MRSW_GUARD_WRITE_SCOPE(name) ::Concurrency::ScopedMRSWGuardWrite temp_variable_named(guardWriteScope)(name)
#else
#define MRSW_GUARD_READ_SCOPE(...)
#define MRSW_GUARD_WRITE_SCOPE(...)
#endif

namespace Concurrency
{
	class MRSWGuard;

	class ScopedMRSWGuardRead
	{
	public:
#ifdef AN_DEVELOPMENT
		inline ScopedMRSWGuardRead(MRSWGuard & _guard) : guard(_guard) { guard.acquire_read(); }
		inline ~ScopedMRSWGuardRead() { guard.release_read(); }
#else
		inline ScopedMRSWGuardRead(MRSWGuard & _guard) {}
		inline ~ScopedMRSWGuardRead() {}
#endif

	private:
#ifdef AN_DEVELOPMENT
		MRSWGuard & guard;
#else
#ifndef AN_CLANG
		int temp;
#endif
#endif
	};

	class ScopedMRSWGuardWrite
	{
	public:
#ifdef AN_DEVELOPMENT
		inline ScopedMRSWGuardWrite(MRSWGuard & _guard) : guard(_guard) { guard.acquire_write(); }
		inline ~ScopedMRSWGuardWrite() { guard.release_write(); }
#else
		inline ScopedMRSWGuardWrite(MRSWGuard & _guard) {}
		inline ~ScopedMRSWGuardWrite() {}
#endif

	private:
#ifdef AN_DEVELOPMENT
		MRSWGuard & guard;
#else
#ifndef AN_CLANG
		int temp;
#endif
#endif
	};

};
