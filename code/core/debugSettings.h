#pragma once

#include "globalSettings.h"

// these are switched on in global debug, switch it off here for other builds

#ifndef AN_DEVELOPMENT_OR_PROFILER
	#ifdef AN_SYSTEM_INFO
		#ifndef AN_PERFORMANCE_MEASURE
			#undef AN_SYSTEM_INFO
		#endif
	#endif
	#ifdef AN_SYSTEM_INFO_DIRECT_GL_CALLS
		#ifndef AN_PERFORMANCE_MEASURE
			#undef AN_SYSTEM_INFO_DIRECT_GL_CALLS
		#endif
	#endif
	#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
		#ifndef AN_PERFORMANCE_MEASURE
			#undef AN_SYSTEM_INFO_DRAW_DETAILS
		#endif
	#endif
#endif

#ifndef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef AN_DEBUG_JOB_SYSTEM
		#undef AN_DEBUG_JOB_SYSTEM
	#endif
#endif

#ifndef AN_PROFILER_OR_RELEASE
	#ifdef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		#undef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
	#endif
	#ifdef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		#undef AN_PERFORMANCE_MEASURE_CHECK_LIMITS
	#endif
#endif
