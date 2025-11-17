#pragma once

#ifdef AN_WINDOWS
	#ifndef AN_STEAM
	#ifndef AN_OCULUS
	#ifndef AN_VIVEPORT
	#ifndef AN_PICO
		// to use FBX you need to get it first to externals
		//#define USE_FBX
	#endif
	#endif
	#endif
	#endif
#endif

// always process as USE_FBX is defined inside fbxLib.h
#ifndef MEMORY_H_INLCUDED
	#undef new
#endif

#ifdef USE_FBX
	// use dll (and that's also to access static member variables :/)
	#define FBXSDK_SHARED

	#ifdef USE_FBX
		#include <fbxsdk.h>

		#pragma comment(lib, "libfbxsdk.lib")
	#endif
#endif

#ifndef MEMORY_H_INLCUDED
	//#define new
#endif
