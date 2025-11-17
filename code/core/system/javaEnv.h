#pragma once

#include "..\globalSettings.h"

#ifdef AN_ANDROID
#include <android\native_activity.h>

/*	
	to get function names of com.voidroom.TeaForGodActivity use:
		javah -o file.h com.voidroom.TeaForGodActivity
	called from runtime\ARM64\Release*\Package\bin\classes
	if that files
	"C" to keep function names as they are
	extern "C"
	{
		copied from file.h
	};
 */

namespace System
{
	class JavaEnv
	{
	public:
		static JNIEnv* get_env(); // if required, starts
		static void drop_env();

		static jclass find_class(JNIEnv*, char const * _className);
		static void make_global_ref(JNIEnv*, jclass &);
		static void make_global_ref(JNIEnv*, jobject & );
	};
};
#endif
