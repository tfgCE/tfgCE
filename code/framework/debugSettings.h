#pragma once

#include "..\core\globalDebug.h"
#include "..\core\globalDefinitions.h"

#include "..\core\debugSettings.h"

//

// setup AN_USE_FRAME_INFO for various cases
#ifdef FORCE_USE_FRAME_INFO
	#define AN_USE_FRAME_INFO
#else
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		#define AN_USE_FRAME_INFO
	#endif
#endif
#ifdef SHORT_HITCH_FRAME_INFO
	#ifndef AN_USE_FRAME_INFO
		#define AN_USE_FRAME_INFO
	#endif
#endif

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		//#define AN_LOG_POST_PROCESS_ALL_FRAMES
		#define AN_LOG_POST_PROCESS
		#define AN_LOG_POST_PROCESS_RT
		#define AN_LOG_POST_PROCESS_RT_DETAILED
	#endif
#endif

#ifndef AN_ANDROID
	#ifdef AN_OUTPUT_TO_FILE
		//#ifdef AN_DEVELOPMENT
		#ifdef AN_DEVELOPMENT_OR_PROFILER
			#ifndef PROFILE_PERFORMANCE
				// disabled to measure true performance
				#define AN_USE_AI_LOG
				#define AN_USE_AI_LOG_TO_FILE
			#endif
		#endif
	#endif
#endif

#define AN_HANDLE_GENERATION_ISSUES

#ifdef AN_DEVELOPMENT
	#define AN_DEVELOPMENT_OR_GENERATION_ISSUES
#else
	#ifdef AN_HANDLE_GENERATION_ISSUES
		#define AN_DEVELOPMENT_OR_GENERATION_ISSUES
	#endif
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	// to test without frame skipping
	//#define AN_ADVANCE_ALL_FRAMES
#endif

//#define AN_DEBUG_LOAD_LIBRARY


// this is used to override skipping, uncommented makes spawn all objects
//#define AN_OVERRIDE_DEV_SKIPPING_DELAYED_OBJECT_CREATIONS


#ifdef BUILD_NON_RELEASE
#define OUTPUT_LOAD_AND_PREPARE_TIMES
//#define OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
#endif


#ifdef AN_DEVELOPMENT
#define WITH_HISTORY_FOR_OBJECTS
#endif


#ifdef AN_DEVELOPMENT_OR_PROFILER
//#define TEST_NOT_SLIDING_LOCOMOTION
#endif

