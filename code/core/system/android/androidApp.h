#pragma once

#ifdef AN_ANDROID
#include "..\..\globalInclude.h"
#include "..\..\debug\debug.h"
#include "..\..\types\optional.h"
#include "..\..\types\string.h"

#include <android\native_activity.h>

#include <functional>

struct android_app;

namespace System
{
	namespace Android
	{
		struct AppInfo
		{
			// fill from glue
			// input
			ANativeActivity* activity = nullptr;
			String name = String(TXT("void room"));
			std::function<void(bool _waitInfinitely)> process_events = nullptr;

			// output
		};

		struct App
		{
		public:
			static App& get() { an_assert(s_app); return *s_app; }
			static void start_static(AppInfo* _appInfo);
			static void end_static();
			static tchar const* get_system_language_id();

			void set_title(String const& _name);

			void process_events(bool _waitInfinitely = false);

			ANativeActivity* get_activity() const { return appInfo->activity; }

		public:
			JavaVM* activityVM = nullptr;
			JNIEnv* activityEnv = nullptr;
			jobject activityObject = nullptr;
			ANativeWindow* nativeWindow;
			bool paused;

		private:
			static App* s_app;

			AppInfo* appInfo; // should remain existent for the whole time App is alive

			App(AppInfo* _appInfo);
			~App();

			void clear();
		};
	};
};
#endif
