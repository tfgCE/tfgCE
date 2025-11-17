#include "androidApp.h"

#ifdef AN_ANDROID

#include "..\video\video3d.h"
#include "..\..\utils.h"
#include "..\..\types\unicode.h"

#include <unistd.h>
#include <android/window.h>
#include <android/native_activity.h>
#include <sys/prctl.h>

//

using namespace System;
using namespace Android;

//

bool g_android_systemLanguageIDProvided = false;
tchar g_android_systemLanguageID[256];

// created with:
//	javah -o vm.h com.voidroom.LanguageUtil
// called from runtime\ARM64\ReleaseOculus\Package\bin\classes
// "C" to keep function names as they are
extern "C"
{
	JNIEXPORT void JNICALL Java_com_voidroom_LanguageUtil_store_1system_1language(JNIEnv* _env, jclass, jstring _languageId)
	{
		const char * lId = _env->GetStringUTFChars(_languageId, 0);

		tchar* r = g_android_systemLanguageID;
		char const* p = lId;
		while (*p != 0)
		{
			if (*p < 0 || *p > 127)
			{
				if (*(p + 1) != 0)
				{
					tchar t[5];
#ifdef AN_WINDOWS
					MultiByteToWideChar(CP_UTF8, 0, p, 2, t, 1);
#else
#ifdef AN_LINUX_OR_ANDROID
					mbtowc(t, p, 1);
#else
#error TODO implement utf8 to wide char
#endif
#endif
					*r = Unicode::unicode_to_tchar(t[0]); // not true unicode but we have to encode as tchar
					++r;
					++p;
					++p;
					continue;
				}
			}
			tchar t = *p;
			*r = t;
			++r;
			++p;
		}
		*r = 0;
		g_android_systemLanguageIDProvided = (r != g_android_systemLanguageID);
		_env->ReleaseStringUTFChars(_languageId, lId);
	}
}

App* App::s_app = nullptr;

void App::start_static(AppInfo* _appInfo)
{
	an_assert(s_app == nullptr);
	s_app = new App(_appInfo);
}

void App::end_static()
{
	an_assert(s_app);
	delete_and_clear(s_app);
}

tchar const* App::get_system_language_id()
{
	return g_android_systemLanguageIDProvided? g_android_systemLanguageID : nullptr;
}

App::App(AppInfo* _appInfo)
: appInfo(_appInfo)
{
	clear();

	ANativeActivity_setWindowFlags(_appInfo->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	activityVM = _appInfo->activity->vm;
	(*activityVM).AttachCurrentThread(&activityEnv, nullptr);
	activityObject = _appInfo->activity->clazz;

	set_title(_appInfo->name);
}

App::~App()
{
	(*activityVM).DetachCurrentThread();
}

void App::clear()
{
	activityVM = nullptr;
	activityEnv = nullptr;
	activityObject = nullptr;
	nativeWindow = nullptr;
	paused = false;
}

void App::set_title(String const & _name)
{
	appInfo->name = _name;
	// Note that AttachCurrentThread will reset the thread name.
	prctl(PR_SET_NAME, _name.to_char8_array().get_data(), 0, 0, 0);
}

void App::process_events(bool _waitInfinitely)
{
	if (appInfo &&
		appInfo->process_events)
	{
		appInfo->process_events(_waitInfinitely);
	}
}

#endif
