#pragma once

#include "globalDebug.h"

// definitions to be defined at project settings/makefile

// options

#define AN_WIDE_CHAR

// info about build
//
//	bit
// AN_32 (auto)
// AN_64
//
//	build type
// AN_DEBUG
// AN_DEV
// AN_PROFILER
// AN_RELEASE
//
//	platform
// AN_WINDOWS
// AN_LINUX very limited support right now
// AN_ANDROID
//   AN_QUEST sub platform
//   AN_VIVE sub platform
//   AN_PICO sub platform
//
//  game systems
// AN_STEAM
// AN_OCULUS
// AN_VIVEPORT
// AN_PICO
//
//	endian
// AN_LITTLE_ENDIAN
// AN_BIG_ENDIAN
//
//	subsystems (in project files)
// AN_SDL
// AN_FMOD
// AN_VRAPI
// AN_GL
// AN_GLES
// AN_STANDARD_INPUT (keyboard, mouse, cursor, etc)
//
//	vr systems
// AN_QUEST ~
// AN_VIVE ~
// AN_PICO ~
// AN_OCULUS_ONLY
// AN_VIVE_ONLY
// AN_PICO_ONLY
//

#ifndef AN_32
#ifndef AN_64
#define AN_32
#endif
#endif

#ifdef AN_SDL
#define AN_SDL_NETWORK
#endif

// QUEST -> ANDROID
#ifndef AN_ANDROID
#ifdef AN_QUEST
#define AN_ANDROID
#endif
#endif

// VIVE -> ANDROID
#ifndef AN_ANDROID
#ifdef AN_VIVE
#define AN_ANDROID
#endif
#endif

// PICO -> ANDROID
#ifndef AN_ANDROID
#ifdef AN_PICO
#define AN_ANDROID
#endif
#endif

#ifndef AN_WINDOWS
#ifndef AN_ANDROID
#ifndef AN_LINUX
#error Platform not supported/missing
#endif
#endif
#endif

// one platform at a time
#ifdef AN_WINDOWS
	#ifdef AN_LINUX
		#error One platform at a time
	#endif
	#ifdef AN_ANDROID
		#error One platform at a time
	#endif
#endif
#ifdef AN_LINUX
	#ifdef AN_ANDROID
		#error One platform at a time
	#endif
#endif

#ifdef AN_LINUX
	#define AN_LINUX_OR_ANDROID
#else
	#ifdef AN_ANDROID
		#define AN_LINUX_OR_ANDROID
	#endif
#endif

// endianness
#ifndef AN_LITTLE_ENDIAN
	#ifndef AN_BIG_ENDIAN
		#error Endian not defined
	#endif
#endif

// no use of more threads
#define THREAD_LIMIT 16

// main settings
#ifdef AN_DEBUG
	#define AN_INSPECT_MEMORY_LEAKS
	//#define AN_DETECT_MEMORY_LEAKS
	//
	//#define AN_LOG_LOADING_AND_PREPARING
	//
	#define AN_ASSERT
	#define AN_ASSERT_SLOW
	#define AN_DEBUG_RENDERER
	#define AN_RENDERER_STATS
	//
	#define AN_DEVELOPMENT
	#define AN_DEVELOPMENT_GL
	#define AN_DEVELOPMENT_SLOW
	//#define AN_DEVELOPMENT_SUPER_SLOW
	//
	#define AN_TODO
	//#define AN_TODO_NOTE
	//#define AN_TODO_FUTURE
	//
	#define AN_STORE_CALL_STACK_INFO
	//
	#define AN_CONCURRENCY_STATS
	#define AN_OUTPUT_TO_LOG
	#define AN_OUTPUT_TO_CONSOLE
	#define AN_OUTPUT_TO_FILE
	//
	#define AN_NAT_VIS
	//
	#define AN_TWEAKS
	#define AN_ALLOW_BULLSHOTS
	#define AN_NO_RARE_ADVANCE_AVAILABLE
	//
	#define AN_DEBUG_KEYS
	//
	#define AN_ALLOW_EXTENDED_DEBUG
	//
	#define AN_INSPECT_LATENT
	#define AN_MEASURE_LATENT
#else
#ifdef AN_DEV
	#define AN_INSPECT_MEMORY_LEAKS
	//#define AN_DETECT_MEMORY_LEAKS
	//
	//#define AN_LOG_LOADING_AND_PREPARING
	//
	#define AN_ASSERT
	#define AN_ASSERT_SLOW
	#define AN_DEBUG_RENDERER
	#define AN_RENDERER_STATS
	//
	#define AN_DEVELOPMENT
	#define AN_DEVELOPMENT_GL
	//#define AN_DEVELOPMENT_SLOW
	//#define AN_DEVELOPMENT_SUPER_SLOW
	//
	#define AN_TODO // to catch them
	//#define AN_TODO_NOTE
	//#define AN_TODO_FUTURE
	//
	#define AN_STORE_CALL_STACK_INFO
	//
	#define AN_CONCURRENCY_STATS
	#define AN_OUTPUT_TO_LOG
	#define AN_OUTPUT_TO_CONSOLE
	#define AN_OUTPUT_TO_FILE
	//
	#define AN_NAT_VIS
	//
	#define AN_TWEAKS
	#define AN_ALLOW_BULLSHOTS
	#define AN_NO_RARE_ADVANCE_AVAILABLE
	//
	#define AN_DEBUG_KEYS
	//
	#define AN_ALLOW_EXTENDED_DEBUG
	//
	#define AN_INSPECT_LATENT
	#define AN_MEASURE_LATENT
#else
#ifdef AN_PROFILER
	//#define AN_INSPECT_MEMORY_LEAKS
	//#define AN_DETECT_MEMORY_LEAKS
	//
	//#define AN_LOG_LOADING_AND_PREPARING
	//
	//#define AN_ASSERT
	//#define AN_ASSERT_SLOW
	#define AN_DEBUG_RENDERER
	#define AN_RENDERER_STATS
	//
	//#define AN_DEVELOPMENT
	//#define AN_DEVELOPMENT_GL
	//#define AN_DEVELOPMENT_SLOW
	//#define AN_DEVELOPMENT_SUPER_SLOW
	//
	//#define AN_TODO
	//#define AN_TODO_NOTE
	//#define AN_TODO_FUTURE
	//
	#define AN_STORE_CALL_STACK_INFO
	//
	#define AN_CONCURRENCY_STATS
	#define AN_OUTPUT_TO_LOG
	#define AN_OUTPUT_TO_CONSOLE
	#define AN_OUTPUT_TO_FILE
	//
	//#define AN_NAT_VIS
	//
	//#define AN_TWEAKS
	#define AN_ALLOW_BULLSHOTS
	#define AN_NO_RARE_ADVANCE_AVAILABLE
	//
	#define AN_DEBUG_KEYS
	//
	//#define AN_ALLOW_EXTENDED_DEBUG
	//
	//#define AN_INSPECT_LATENT
	#define AN_MEASURE_LATENT
#else
#ifdef AN_RELEASE
	//#define AN_INSPECT_MEMORY_LEAKS
	//#define AN_DETECT_MEMORY_LEAKS
	//
	//#define AN_LOG_LOADING_AND_PREPARING
	//
	//#define AN_ASSERT
	//#define AN_ASSERT_SLOW
	//#define AN_DEBUG_RENDERER
	//#define AN_RENDERER_STATS
	//
	//#define AN_DEVELOPMENT
	//#define AN_DEVELOPMENT_GL
	//#define AN_DEVELOPMENT_SLOW
	//#define AN_DEVELOPMENT_SUPER_SLOW
	//
	//#define AN_TODO
	//#define AN_TODO_NOTE
	//#define AN_TODO_FUTURE
	//
	#define AN_STORE_CALL_STACK_INFO
	//
	//#define AN_CONCURRENCY_STATS
	//#define AN_OUTPUT_TO_LOG
	//#define AN_OUTPUT_TO_CONSOLE
	#define AN_OUTPUT_TO_FILE
	//
	//#define AN_NAT_VIS
	//
	//#define AN_TWEAKS
	#define AN_ALLOW_BULLSHOTS
	#define AN_NO_RARE_ADVANCE_AVAILABLE
	//
	//#define AN_DEBUG_KEYS
	//
	//#define AN_ALLOW_EXTENDED_DEBUG
	//
	//#define AN_INSPECT_LATENT
	//#define AN_MEASURE_LATENT
#else
	#error choose debug, release, final or profiler
#endif
#endif
#endif
#endif

#define WITH_EXTRA_DEBUG_INFO
#ifdef WITH_EXTRA_DEBUG_INFO
	#define SET_EXTRA_DEBUG_INFO(where, what) where.set_extra_debug_info(what);
#else
	#define SET_EXTRA_DEBUG_INFO(where, what)
#endif

// development/profiler
#ifdef AN_DEVELOPMENT
	#define AN_DEVELOPMENT_OR_PROFILER
#else
	#ifdef AN_PROFILER
		#define AN_DEVELOPMENT_OR_PROFILER
	#endif
#endif

// profiler/final
#ifdef AN_PROFILER
	#define AN_PROFILER_OR_RELEASE
#else
	#ifdef AN_RELEASE
		#define AN_PROFILER_OR_RELEASE
	#endif
#endif

#ifdef FORCE_EXTENSIVE_LOGS
	#define AN_ALLOW_EXTENSIVE_LOGS
#else // FORCE_EXTENSIVE_LOGS
	#ifdef ALLOW_EXTENSIVE_LOGS
		#ifdef AN_DEVELOPMENT
			#define AN_ALLOW_EXTENSIVE_LOGS
		#endif
		#ifdef AN_PROFILER
			#define AN_ALLOW_EXTENSIVE_LOGS
		#endif
	#endif
#endif

// gles disable some
#ifdef AN_GLES
	#ifdef AN_DEVELOPMENT_GL
		#undef AN_DEVELOPMENT_GL
	#endif
#endif

#ifdef AN_ANDROID
	// no output to a console for android, redirect to log
	#ifdef AN_OUTPUT_TO_CONSOLE
		#ifndef AN_OUTPUT_TO_LOG
			#define AN_OUTPUT_TO_LOG
		#endif
		#undef AN_OUTPUT_TO_CONSOLE
	#endif
	// all output to a file
	#ifdef AN_OUTPUT_TO_LOG
		#ifndef AN_OUTPUT_TO_FILE
			#define AN_OUTPUT_TO_FILE
		#endif
		#undef AN_OUTPUT_TO_LOG
	#endif
	// all output to a file!
	#ifdef AN_OUTPUT_TO_FILE
		#ifdef AN_OUTPUT_TO_LOG
			#undef AN_OUTPUT_TO_LOG
		#endif
	#endif
#endif

// auto output
#ifdef AN_OUTPUT_TO_LOG
	#define AN_OUTPUT
#else
	#ifdef AN_OUTPUT_TO_CONSOLE
		#define AN_OUTPUT
	#else
		#ifdef AN_OUTPUT_TO_FILE
			#define AN_OUTPUT
		#endif
	#endif
#endif

// for android, use detecting memory leaks, no inspection!
#ifdef AN_ANDROID
	#ifdef AN_INSPECT_MEMORY_LEAKS
		#undef AN_INSPECT_MEMORY_LEAKS
		#ifndef AN_DETECT_MEMORY_LEAKS
			#define AN_DETECT_MEMORY_LEAKS
		#endif
	#endif
#endif

// do checks for memory leaks settings
#ifdef AN_DETECT_MEMORY_LEAKS
	#define AN_CHECK_MEMORY_LEAKS
	#ifdef AN_INSPECT_MEMORY_LEAKS
		#error choose inspect or detect memory leaks - not both at the same time!
	#endif
#else
	#ifdef AN_INSPECT_MEMORY_LEAKS
		#define AN_CHECK_MEMORY_LEAKS
	#endif
#endif

#ifndef AN_CHECK_MEMORY_LEAKS
	#define AN_DONT_CHECK_MEMORY_LEAKS
#endif

// check how much of gpu memory is taken/used
#ifdef BUILD_PREVIEW
	#define DEBUG_GPU_MEMORY
	//#define DEBUG_GPU_MEMORY_OUTPUT
#endif

// check for gl errors and debug messages
#define WITH_GL_CHECK_FOR_ERROR 
#ifdef BUILD_PREVIEW
	#define WITH_GL_CHECK_FOR_ERROR_ALL
	#ifdef BUILD_NON_RELEASE
		//#define WITH_GL_DEBUG_MESSAGES
	#endif
#endif

// other misc options

//#define AN_MINIGAME_PLATFORMER

#define AN_OUTPUT_IMPORT_ERRORS

// if extensive logs are allowed - until we catch all the bugs
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#define AN_OUTPUT_WORLD_GENERATION
	#define AN_OUTPUT_WORLD_ACTIVATION
	#define AN_OUTPUT_NAV_GENERATION
#endif

// disable specific features on android devices
#ifdef AN_ANDROID
	#ifdef AN_CONCURRENCY_STATS
		#undef AN_CONCURRENCY_STATS
	#endif
	// no purpose in checking tweaks
	#ifdef AN_TWEAKS
		#undef AN_TWEAKS
	#endif
	#ifdef AN_ALLOW_BULLSHOTS
		#undef AN_ALLOW_BULLSHOTS
	#endif
	#ifdef AN_NAT_VIS
		#undef AN_NAT_VIS
	#endif
	//#ifdef AN_DEBUG_RENDERER
	//	#undef AN_DEBUG_RENDERER
	//#endif
#endif

// memory stats
#ifdef AN_ANDROID
	#ifndef AN_RELEASE
		#define AN_MEMORY_STATS
	#endif
#endif
#ifndef AN_ANDROID
	#ifndef AN_RELEASE
		#ifdef AN_INSPECT_MEMORY_LEAKS
			#define AN_MEMORY_STATS
		#endif
	#endif
#endif

// allow optimize off
#ifndef AN_CLANG
	#ifdef AN_DEVELOPMENT
		#define AN_ALLOW_OPTIMIZE_OFF
	#endif
#endif

// this will load render model from vr system (not useful anymore)
//#define AN_USE_VR_RENDER_MODELS

// various system settings
#ifdef AN_ANDROID
	#ifdef AN_ALLOW_BULLSHOTS
		#undef AN_ALLOW_BULLSHOTS
	#endif
	#ifdef AN_NO_RARE_ADVANCE_AVAILABLE
		#undef AN_NO_RARE_ADVANCE_AVAILABLE
	#endif
#endif

// vr
#ifdef AN_VR
	#ifdef AN_WINDOWS
		#ifdef AN_OCULUS_ONLY
			// + openxr
			#define AN_WITH_OCULUS
		#else
			#ifdef AN_VIVE_ONLY
				// + openxr
				#define AN_WITH_OPENVR
			#else
				#ifdef AN_PICO_ONLY
					// + openxr
				#else
					// + openxr
					#define AN_WITH_OCULUS
					#define AN_WITH_OPENVR
				#endif
			#endif
		#endif
	#endif
	#ifdef AN_QUEST
		// + openxr
		#define AN_WITH_OCULUS_MOBILE
	#else
		#ifdef AN_ANDROID
			#ifdef AN_OCULUS_ONLY
				// + openxr
				#define AN_WITH_OCULUS
			#else
				#ifdef AN_VIVE_ONLY
					// + openxr
					// not using WaveVR
					//#define AN_WITH_WAVEVR
				#else
					#ifdef AN_PICO_ONLY
						// + openxr
					#endif
				#endif
			#endif
		#endif
	#endif
	// openxr is always available!
	#define AN_WITH_OPENXR
#endif

#define AN_ANDROID_LOG_TAG "voidroom"

// graphics
//#define AN_USE_BACKGROUND_RENDERING_BUFFER

// force auto report bugs
#define AN_ASSUME_AUTO_REPORT_BUGS

// debug/test

#ifdef AN_DEVELOPMENT
	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		//#define INSPECT_MESH_RENDERING
	#endif
#endif
//#define AN_LOG_NAV_TASKS

#define SOUND_SYSTEM_OWN_FADING
