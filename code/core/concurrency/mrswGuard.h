#pragma once

#include "..\globalDefinitions.h"

#include "counter.h"

// should be used with macros

#ifdef AN_DEVELOPMENT
#define MRSW_GUARD(name) ::Concurrency::MRSWGuard name
#define MRSW_GUARD_MUTABLE(name) mutable ::Concurrency::MRSWGuard name
#define MRSW_GUARD_READ_START(name) name.acquire_read()
#define MRSW_GUARD_READ_END(name) name.release_read()
#define MRSW_GUARD_WRITE_START(name) name.acquire_write()
#define MRSW_GUARD_WRITE_END(name) name.release_write()
#else
#define MRSW_GUARD(...)
#define MRSW_GUARD_MUTABLE(...)
#define MRSW_GUARD_READ_START(...)
#define MRSW_GUARD_READ_END(...)
#define MRSW_GUARD_WRITE_START(...)
#define MRSW_GUARD_WRITE_END(...)
#endif

namespace Concurrency
{

	class MRSWGuard // multi read single write
	{
	public:
		MRSWGuard();

		void acquire_read();
		void release_read();

		void acquire_write();
		void release_write();

#ifdef AN_CONCURRENCY_STATS
		void do_not_report_stats();
#endif

	private:
#ifdef AN_DEVELOPMENT
		Counter activeReaders;
		Counter activeWriters;
#else
#ifndef AN_CLANG
		int temp;
#endif
#endif
#ifdef AN_CONCURRENCY_STATS
		bool reportStats = true;
#endif
	};

};

