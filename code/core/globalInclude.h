#pragma once

#include "globalSettings.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <new>

#ifdef AN_WINDOWS
	#include <SDKDDKVer.h>
	#include <windows.h>
	#include <intrin.h>	// for debugbreak
	//#include <cstdlib>
    //#include <string.h>
	#include <tchar.h>
#endif

#ifdef AN_LINUX
    #include <cstdlib>
    #include <string.h>
#endif

#ifdef AN_ANDROID
#include <atomic>
#include <jni.h>
#include <android/sensor.h>
#include <android/log.h>
#include <string.h> // to get memory functions (memcpy & co)
#ifdef AN_WIDE_CHAR
	#include <wchar.h>
#endif
#endif
