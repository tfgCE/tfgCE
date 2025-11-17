#pragma once

#include "..\containers\array.h"
#include "..\globalDefinitions.h"
#include "..\tags\tag.h"

#include "timePoint.h"
#include "timeStamp.h"

#ifdef AN_SDL
#include "SDL.h"
#endif

#include <functional>

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define TIME_MANIPULATIONS_DEV
#endif

namespace System
{
#ifdef AN_SDL
	inline SDL_bool to_sdl_bool(bool _a) { return _a? SDL_TRUE : SDL_FALSE; }
#endif

	class Core
	{
	public:
		static bool restartRequired;

		static Core* get() { return s_current; }
		
		static void clear_extra_requested_system_tags();
		static void set_extra_requested_system_tags(tchar const* _tags, bool _add = false);
		static String get_extra_requested_system_tags();
		static void loaded_extra_requested_system_tags(); // right after loaded
		static void changed_extra_requested_system_tags(); // right after changed
		static bool has_extra_requested_system_tag_changed(Name const & _tag);

		static tchar const * get_app_name();
		static void set_app_name(tchar const * _name);

		static Tags const& get_system_tags();
		static Tags& access_system_tags();
		static String const & get_system_name();
		static String const & get_system_precise_name();

		static void quick_exit(bool _immediate = false); // if not immediate, will allow quick exit handler to do its stuff
		static void set_quick_exit_handler(std::function<void()> _quickExitHandler);

		static void fill_time_info(tchar * buf, int bufSize);

		static void initialise_static();
		static void close_static(bool _willRestart = false);

		static void create();
		static void terminate();
		static void post_init_all_subsystems();

		static void update_on_system_change();
		static void output_system_info();

		static void set_title(String const& _name);

		static void block_on_advance(Name const & _what);
		static void allow_on_advance(Name const& _what);
		static void on_advance(Name const& _what, std::function<void()> _do);
		static void on_advance_no_longer(Name const& _what);

		static void advance();
		static float get_delta_time();
		static float get_raw_delta_time();
		static float get_time_speed();
#ifdef AN_DEVELOPMENT_OR_PROFILER
		static bool is_advancing_one_frame_only();
#endif

		static void store_display_delta_time(float _deltaTime);
		static void store_vr_delta_time(float _deltaTime);
		static void force_vr_sync_delta_time(float _deltaTime); // will be used immediately, this should be synced via vr post core::advance, ideally from game thread (not the main!)

		static void reset_timer();
		//
		static int get_frame();
		static float get_timer();
		static float get_timer_mod(float _mod);
		static float get_timed_pulse(float _period, Optional<Name> const & _curve = NP); // 0 to 1
		static double get_timer_as_double();
		static float get_timer_raw();
		static float get_timer_pendulum();

		static bool is_ok() { return s_current != nullptr; }

		static bool has_focus();

		static void sleep(float _seconds); // if 0, sleeps for at least 1 ms
		static void sleep_u(int _microseconds); // if 0, sleeps for at least 1 us, no longer than a second!
		static void sleep_n(int _nanoseconds); // if 0, sleeps for at least 1 ns, no longer than a second!

		static bool is_paused_at_all() { return is_paused() || is_vr_paused(); }

		static void pause(bool _pause = true);
		static bool is_paused();

		// pause due to vr
		static void pause_vr(int _flag);
		static void unpause_vr(int _flag);
		static bool is_vr_paused();

		// pause rendering - no need to?
		static void pause_rendering(int _flag);
		static void unpause_rendering(int _flag);
		static bool is_rendering_paused();

		// if exiting quickly, do these. core has to be initialised
		static void on_quick_exit(tchar const * _what, std::function<void()> _do);
		static void on_quick_exit_no_longer(tchar const * _what);
		static bool is_performing_quick_exit();

		static void set_time_multiplier(float _timeMultiplier = 1.0f);

#ifdef TIME_MANIPULATIONS_DEV
		static void one_frame_time_pace(int _frames = 1, float _fps = 90.0f);
		static void slow_down_time_pace();
		static void normal_time_pace();
		static void speed_up_time_pace();
		static float get_time_pace();
#endif

		static float get_time_since_core_started();

		static tchar const* get_system_language(); // returns language identifier "en"

	private:
		static Core* s_current;
		static bool s_sdlInitialised;

		String systemName; // pc|quest
		String systemPreciseName; // pc|quest|quest2
		Tags systemTags;
		String actualSystemName; // read from system

		TimeStamp coreStartedTimeStamp;

		float deltaTime = 0.0f; // either raw or from display
		float clampedDeltaTime = 0.0f; // to be used by game

		float deltaTimeUsedSoFar = 0.0f; // this is to counter delta time that was already set if we forced it afterwards
		float rawDeltaTimeUsedSoFar = 0.0f; // this is to counter delta time that was already set if we forced it afterwards

		// from most important to least important: vr sync, vr, display, raw
		float rawDeltaTime = 0.0f;
		float displayDeltaTime = 0.0f;
		bool displayDeltaTimeProvided = false;
		float vrDeltaTime = 0.0f;
		bool vrDeltaTimeProvided = false;
		float vrSyncDeltaTime = 0.0f;
		bool vrSyncDeltaTimeProvided = false;

		TimePoint lastTimePoint;
		int frame = 0;
		double timerLong = 0.0f; // timerLong -> timer, this way we should avoid issues with float's precision
		double timerRawLong = 0.0f; // timerLong -> timer, this way we should avoid issues with float's precision
		float timer = 0.0f;
		float timerRaw = 0.0f;
		float timerPendulum = 0.0f;
		float timerPendulumLoop = 128.0f;
		float timerPendulumResult = 0.0f;

		bool hasFocus = false;

		struct OnAdvance
		{
			Name what;
			std::function<void()> perform;
			int blocked = 0;

			OnAdvance() {}
			OnAdvance(Name const& _what, std::function<void()> _perform) : what(_what), perform(_perform) {}
		};
		Array<OnAdvance> onAdvance;

		std::function<void()> quickExitHandler;

		struct OnQuickExit
		{
			tchar const* what;
			std::function<void()> perform;

			OnQuickExit() {}
			OnQuickExit(tchar const* _what, std::function<void()> _perform) : what(_what), perform(_perform) {}
		};
		Array<OnQuickExit> onQuickExit;

		Core();
		~Core();

		void update_on_system_change_internal();
		void output_system_info_internal();

		void set_delta_time(float _newDeltaTime, Optional<float> const& _rawDeltaTime = NP);
	};
};
