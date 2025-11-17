#include <android/log.h>
#include <android/native_window_jni.h>	// for native window JNI
#include "android_native_app_glue.h"

#include "../core/coreInit.h"
#include "../core/system/faultHandler.h"
#include "../core/system/android/androidApp.h"
#include "../core/vr/iVR.h"
#include "../teaForGod/teaForGod.h"
#include "../teaForGod/teaForGodMain.h"

#define DEBUG 1
#include "../core/globalSettings.h"

#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, AN_ANDROID_LOG_TAG, __VA_ARGS__ )
#if DEBUG
#define ALOGV(...) __android_log_print( ANDROID_LOG_INFO, AN_ANDROID_LOG_TAG, __VA_ARGS__ )
#else
#define ALOGV(...)
#endif

static void android_app_handle_cmd(struct android_app* app, int32_t cmd)
{
	System::Android::App* appState = (System::Android::App*)app->userData;

	switch (cmd)
	{
		// There is no APP_CMD_CREATE. The ANativeActivity creates the
		// application thread from onCreate(). The application thread
		// then calls android_main().
		case APP_CMD_START:
		{
			output(TXT("    APP_CMD_START"));
			break;
		}
		case APP_CMD_RESUME:
		{
			output(TXT("    APP_CMD_RESUME"));
			appState->paused = false;
			break;
		}
		case APP_CMD_PAUSE:
		{
			output(TXT("    APP_CMD_PAUSE"));
			appState->paused = true;
			break;
		}
		case APP_CMD_STOP:
		{
			output(TXT("    APP_CMD_STOP"));
			break;
		}
		case APP_CMD_DESTROY:
		{
			output(TXT("    APP_CMD_DESTROY"));
			appState->nativeWindow = NULL;
			break;
		}
		case APP_CMD_INIT_WINDOW:
		{
			output(TXT("    APP_CMD_INIT_WINDOW"));
			appState->nativeWindow = app->window;
			break;
		}
		case APP_CMD_TERM_WINDOW:
		{
			output(TXT("    APP_CMD_TERM_WINDOW"));
			appState->nativeWindow = NULL;
			break;
		}
	}
}

void test_sample_android_main_oculus(struct android_app* app);

void android_main( struct android_app * app )
{
	//test_sample_android_main_oculus(app);
	//return;

	disallow_output(); // can't output anything until we're good with that

	{
#ifdef AN_CHECK_MEMORY_LEAKS
		mark_static_allocations_done_for_memory_leaks();
#endif
		::System::Android::AppInfo appInfo;
		appInfo.activity = app->activity;
		appInfo.name = String(TEA_FOR_GOD_NAME);
		appInfo.process_events = [app](bool _waitInfinitely)
		{
			for (; ; )
			{
				int events;
				struct android_poll_source* source;
				// wait infinitely if ordered, unless we're about to end
				int timeoutMilliseconds = _waitInfinitely && app->destroyRequested == 0? -1 : 0;
				if (ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void**)& source) < 0)
				{
					break;
				}
				_waitInfinitely = false; // wait infinitely just once

				// Process this event.
				if (source != NULL)
				{
					scoped_call_stack_info(TXT("process event through source (external)"));
					source->process(app, source);
				}

				if (auto* vr = VR::IVR::get())
				{
					vr->handle_vr_mode_changes();
				}
			}
		};

		::System::Android::App::start_static(&appInfo);

		app->userData = &System::Android::App::get();
		app->onAppCmd = android_app_handle_cmd;

		// game begin
		TeaForGodEmperor::Main* teaMain = new TeaForGodEmperor::Main();

		teaMain->run();

		delete teaMain;
		// game end

		::System::Android::App::end_static();

#ifdef AN_CHECK_MEMORY_LEAKS
		mark_static_allocations_left_for_memory_leaks();
#endif
	}
}
