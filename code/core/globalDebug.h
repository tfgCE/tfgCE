#pragma once

// changing buildVer.h requires everything to recompile
#include "buildVer.h"

//

// very specific logs, for catching particular bugs, should always be allowed to output/debug
// by default these are off for public releases (defined via batch file), if there's need to enable them, work with lines below
// same if you want to disable them at all

//

// easy to set options (the rest is more like tweaking stuff):
// if you want to investigate the game, enable ALLOW_GLOBAL_DEBUG, disable PROFILE_PERFORMANCE
// if you want to profile performance, just enable PROFILE_PERFORMANCE, disable ALLOW_GLOBAL_DEBUG to avoid messages/warnings (PROFILE_PERFORMANCE auto disables ALLOW_GLOBAL_DEBUG)
// 
// general debug info
/* in general, extensive logs etc -------------------------------------------------------------------- */ /* disabled for testing */ //#define ALLOW_GLOBAL_DEBUG
/* allow detailed debug(as long as debug is allowed in general) -------------------------------------- */ //#define ALLOW_DETAILED_DEBUG
/* allow extra forced debug(special options, always set, etc, frame info, hitches and so on) --------- */ //#define ALLOW_EXTRA_FORCED_DEBUG
// to check performance:
/* we want detailed frame info ----------------------------------------------------------}  one   {--- */ //#define FORCE_FULL_HITCH_FRAME_INFO
/* we want short frame info -------------------------------------------------------------} of two {--- */ #define FORCE_SHORT_HITCH_FRAME_INFO
#ifdef BUILD_NON_RELEASE
/* profile performance (store fpis, draw call infos) (only local development builds!) ---------------- */ //#define PROFILE_PERFORMANCE
/* detailed - includes low-level tasks for rendering, sound, appearance controllers ------------------ */ //#define PROFILE_PERFORMANCE_DETAILED
/* show render hitch frame (requires short hitch frame (or full) ------------------------------------- */ //#define FORCE_RENDER_HITCH_FRAME_INDICATOR
/* if hitch frame indicator is rendered, it is short (less info) ------------------------------------- */ #define SHORT_HITCH_FRAME_INDICATOR
/* force capture performance frames, every (number) of frames, to get idle/random -------------------- */ //#define PROFILE_PERFORMANCE_CAPTURE_EVERY 1000
/* investigate issues with sound system, when sounds start, stop ------------------------------------- */ //#define INVESTIGATE_SOUND_SYSTEM
#endif

//

// switch global debug on
#ifndef AN_RELEASE
	#ifndef ALLOW_GLOBAL_DEBUG
		#define ALLOW_GLOBAL_DEBUG
	#endif
#endif

// auto off for public releases (even previews)
#ifdef BUILD_PREVIEW_RELEASE
	#define AUTO_NO_PERFORMANCE_MEASURE
	//#define AUTO_NO_HITCH_FRAMES
#else
	#ifdef BUILD_PUBLIC_RELEASE
		#define AUTO_NO_GLOBAL_DEBUG
		#define AUTO_NO_PERFORMANCE_MEASURE
		#define AUTO_NO_HITCH_FRAMES
	#else
		#ifdef BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE
			#define AUTO_NO_GLOBAL_DEBUG
			#define AUTO_NO_PERFORMANCE_MEASURE
			#define AUTO_NO_HITCH_FRAMES
		#endif
	#endif
#endif

#ifdef AUTO_NO_GLOBAL_DEBUG
	#ifdef ALLOW_GLOBAL_DEBUG
		#undef ALLOW_GLOBAL_DEBUG
	#endif
#endif
#ifdef AUTO_NO_PERFORMANCE_MEASURE
	#ifdef PROFILE_PERFORMANCE
		#undef PROFILE_PERFORMANCE
	#endif
#endif

// auto off for public release candidate
#ifdef BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE
	#ifdef ALLOW_GLOBAL_DEBUG
		#undef ALLOW_GLOBAL_DEBUG
	#endif
	#ifdef PROFILE_PERFORMANCE
		#undef PROFILE_PERFORMANCE
	#endif
#endif

// one of two
#ifdef FORCE_FULL_HITCH_FRAME_INFO
	#ifdef FORCE_SHORT_HITCH_FRAME_INFO
		#pragma message("FORCE_FULL_HITCH_FRAME_INFO disables FORCE_SHORT_HITCH_FRAME_INFO")
		#undef FORCE_SHORT_HITCH_FRAME_INFO
	#endif
	#ifdef FORCE_RENDER_HITCH_FRAME_INDICATOR
		#pragma message("FORCE_FULL_HITCH_FRAME_INFO disables FORCE_RENDER_HITCH_FRAME_INDICATOR")
		#undef FORCE_RENDER_HITCH_FRAME_INDICATOR
	#endif
#endif

// to show render hitch frame
#ifdef FORCE_RENDER_HITCH_FRAME_INDICATOR
	#ifndef RENDER_HITCH_FRAME_INDICATOR
		#define RENDER_HITCH_FRAME_INDICATOR
	#endif
	#ifndef FORCE_FULL_HITCH_FRAME_INFO
		#ifndef FORCE_SHORT_HITCH_FRAME_INFO
			#define FORCE_SHORT_HITCH_FRAME_INFO
		#endif
	#endif
#endif

// detailed options for preview builds
#ifndef BUILD_PUBLIC_RELEASE
	// storing every frame seriously hurts performance, for quest we auto disable it
	#define WITH_RECENT_CAPTURE
	#define RECENT_CAPTURE_ONLY_FULL_GAME
	#ifndef AN_ANDROID
		#define RECENT_CAPTURE_STORE_PIXEL_DATA_EVERY_FRAME
	#endif
#endif

// profile performance
#ifdef PROFILE_PERFORMANCE
	#ifdef WITH_RECENT_CAPTURE
		#undef WITH_RECENT_CAPTURE
	#endif
	#define AN_PERFORMANCE_MEASURE
	#ifdef AN_PERFORMANCE_MEASURE
		// check limits and measure them
		//#define AN_PERFORMANCE_MEASURE_GUARD_LIMITS
		// as long as performance is measured
		#define AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH
		// story only if game exceeds
		#define AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH_GAME_ONLY
		// story only very long, > x2.5 expected
		//#define AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH_LONG_ONLY
		// ignore long render times when storing fpi
		//#define AUTO_SAVE_FRAME_PERFORMANCE_INFO_ON_HITCH_IGNORE_RENDER
		// force low rendering settings
		//#define FORCE_RENDERING_LOW
		//#define FORCE_RENDERING_ULTRA_LOW
		// while profiling performance, switch off global debug to not interfere
		#ifdef ALLOW_GLOBAL_DEBUG
			#pragma message("can't do ALLOW_GLOBAL_DEBUG with PROFILE_PERFORMANCE, auto disabling")
			#undef ALLOW_GLOBAL_DEBUG
		#endif
		// unlock to see all elements, otherwise it is high level info only
		#ifdef PROFILE_PERFORMANCE_DETAILED
			#define MEASURE_PERFORMANCE_RENDERING_DETAILED
			#define MEASURE_PERFORMANCE_SOUND_DETAILED
			#define MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
		#endif
		// we need concurrency stats to check for spin locks
		#define AN_CONCURRENCY_STATS
		// get draw calls
		#define AN_SYSTEM_INFO
		#define AN_SYSTEM_INFO_DIRECT_GL_CALLS
		#define AN_SYSTEM_INFO_DRAW_DETAILS
		// to measure if particular things (guarded, ordered to be measured) do not take too much time
		/* use with care as may hog performance due to output */ #define AN_PERFORMANCE_MEASURE_CHECK_LIMITS
		// will show letters/symbols on why we had hitch frame
		#define RENDER_HITCH_FRAME_INDICATOR
	#endif
#endif

// detailed options
#ifdef ALLOW_GLOBAL_DEBUG

	// just info when world is being destroyed
	#define DEBUG_DESTROY_WORLD

	#ifndef INVESTIGATE_SOUND_SYSTEM
		#ifndef AN_DEVELOPMENT
			// when objects get created and destroyed
			/*investigate issues*/ #define DEBUG_WORLD_LIFETIME
			/*investigate issues*/ #define DEBUG_WORLD_LIFETIME_INDICATOR
			// actual creation and destruction of objects
			/*investigate issues*/ #define DEBUG_WORLD_CREATION_AND_DESTRUCTION
		#endif

		// lots of logging
		/*investigate issues*/ #define FORCE_EXTENSIVE_LOGS
		/*investigate issues but optional*/ #define ALLOW_EXTENSIVE_LOGS
	#endif

	// more details on hitch
	#ifndef AUTO_NO_HITCH_FRAMES
		#ifndef AUTO_NO_PERFORMANCE_MEASURE
			#ifndef FORCE_USE_FRAME_INFO
				/*investigate performance (light-weight)*/ #define FORCE_USE_FRAME_INFO
			#endif
			/*will show letters/symbols on why we had hitch frame*/ #define RENDER_HITCH_FRAME_INDICATOR
			#define SHORT_HITCH_FRAME_INFO
		#endif
	#endif

	// these might be switched off in debugSettings.h

	// development or profiler (auto disabled otherwise)
	// memory etc
	#define AN_SYSTEM_INFO
	#define AN_SYSTEM_INFO_DIRECT_GL_CALLS
	#define AN_SYSTEM_INFO_DRAW_DETAILS

	// extensive logs
	//#define AN_DEBUG_JOB_SYSTEM

	// profiler or final
	// to check if any immediate job has stopped/takes way too long time
	//#define AN_JOB_SYSTEM_CHECK_FOR_LOCKS

	// every output line will have info about thread
	/*investigate issues*/ //#define OUTPUT_WITH_THREAD_ID
	/*investigate issues*/ //#define OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER

#else

	#ifdef ALLOW_DETAILED_DEBUG
		#undef ALLOW_DETAILED_DEBUG
	#endif

#endif

#ifdef BUILD_NON_RELEASE
	#ifndef ALLOW_CHEAP_CHEATS
		#define ALLOW_CHEAP_CHEATS
	#endif
#endif

//
// 
// this is extra debug
// to check certain issues - ALWAYS
// it still has guards to avoid

#ifndef ALLOW_CHEAP_CHEATS
	#define ALLOW_CHEAP_CHEATS
#endif

#ifdef ALLOW_EXTRA_FORCED_DEBUG

	/*
	commented out, rely on ALLOW_GLOBAL_DEBUG
	#ifndef DEBUG_DESTROY_WORLD
		#define DEBUG_DESTROY_WORLD
	#endif
	#ifndef DEBUG_WORLD_LIFETIME
		#define DEBUG_WORLD_LIFETIME
	#endif
	#ifndef DEBUG_WORLD_CREATION_AND_DESTRUCTION
		#define DEBUG_WORLD_CREATION_AND_DESTRUCTION
	#endif
	*/

	#ifndef AUTO_NO_HITCH_FRAMES
		// log more for hitch frame
		#ifndef FORCE_USE_FRAME_INFO
			#define FORCE_USE_FRAME_INFO
		#endif

		#ifndef AUTO_SWITCH_OFF_SOME_DEBUG
			#ifndef RENDER_HITCH_FRAME_INDICATOR
				#define RENDER_HITCH_FRAME_INDICATOR
			#endif
		#endif

		#ifndef SHORT_HITCH_FRAME_INFO
			#define SHORT_HITCH_FRAME_INFO
		#endif
	#endif

#endif

// to enforce short hitch frame info always
#ifdef FORCE_FULL_HITCH_FRAME_INFO
	#ifndef FORCE_USE_FRAME_INFO
		#define FORCE_USE_FRAME_INFO
	#endif
	#ifdef SHORT_HITCH_FRAME_INFO
		#undef SHORT_HITCH_FRAME_INFO
	#endif
#endif

// to enforce short hitch frame info always
#ifdef FORCE_SHORT_HITCH_FRAME_INFO
	#ifndef SHORT_HITCH_FRAME_INFO
		#define SHORT_HITCH_FRAME_INFO
	#endif
#endif

// always check for locks now
#ifndef BUILD_NON_RELEASE
	#ifndef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		#define AN_JOB_SYSTEM_CHECK_FOR_LOCKS
	#endif
#endif
#ifdef AN_QUEST
	#ifndef AN_JOB_SYSTEM_CHECK_FOR_LOCKS
		#define AN_JOB_SYSTEM_CHECK_FOR_LOCKS
	#endif
#endif

// for time being, always have lifetime indicators
#ifndef DEBUG_WORLD_LIFETIME_INDICATOR
	//#define DEBUG_WORLD_LIFETIME_INDICATOR
#endif

// short info with extra details
#ifdef SHORT_HITCH_FRAME_INFO
	//#define SHORT_HITCH_FRAME_INFO_DETAILS
#endif

//
// extra particular debugs!
//

//#define DEBUG_ELEVATOR_MOVING_ALONE
//#define INVESTIGATE_MISSING_INPUT
