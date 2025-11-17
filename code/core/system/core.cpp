#include "core.h"
#include "input.h"
#include "sound\soundSystem.h"
#ifdef BUILD_NON_RELEASE
#include "video\renderTargetSave.h"
#endif
#include "video\video2d.h"
#include "video\video3d.h"

#include "..\gameEnvironment\iGameEnvironment.h"

#include "..\mainConfig.h"
#include "..\mainSettings.h"

#include "..\concurrency\scopedSpinLock.h"
#include "..\concurrency\threadManager.h"
#include "..\concurrency\threadSystemUtils.h"

#include "..\network\network.h"

#include "..\performance\performanceUtils.h"
#include "..\splash\splashLogos.h"

#include "..\vr\iVR.h"

#include <time.h>

#include "../timeUtils.h"

#include "android\androidApp.h"

#ifdef AN_LINUX_OR_ANDROID
#include <unistd.h>
#include <sys/system_properties.h>
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define FILL_TIME_LONG
//#define FILL_TIME_FULL_HHMMSS

#ifndef OUTPUT_WITH_THREAD_ID
	//#define OUTPUT_WITH_THREAD_ID
#endif
#ifndef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
	//#define OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
#endif

//

using namespace System;

//

#define USE_OTHER_DELTA_TIMES

tchar s_extraRequestedSystemTags[1024] = TXT("");
ArrayStatic<Name, 16> s_loadedExtraRequestedSystemTags; // we will store extra requested system tags here to compare with new requested to see which have changed
ArrayStatic<Name, 16> s_setExtraRequestedSystemTags; // this is what we set to
ArrayStatic<Name, 16> s_changedExtraRequestedSystemTags; // these are all changed options

bool Core::restartRequired = false;

void Core::clear_extra_requested_system_tags()
{
	set_extra_requested_system_tags(TXT(""));
}

void Core::set_extra_requested_system_tags(tchar const * _tags, bool _add)
{
	if (_add)
	{
		int at = (int)tstrlen(s_extraRequestedSystemTags);
		if (at > 0)
		{
#ifdef AN_CLANG
			tsprintf(&s_extraRequestedSystemTags[at], 511 - at, TXT(" %S"), _tags);
#else
			tsprintf(&s_extraRequestedSystemTags[at], 511 - at, TXT(" %s"), _tags);
#endif
			return;
		}
	}
	tsprintf(s_extraRequestedSystemTags, 511, _tags);
}

void Core::loaded_extra_requested_system_tags()
{
	s_loadedExtraRequestedSystemTags.clear();

	Tags t;
	t.load_from_string(String(s_extraRequestedSystemTags));
	t.do_for_every_tag([](Tag const& _tag)
		{
			if (_tag.get_relevance() > 0.0f)
			{
				s_loadedExtraRequestedSystemTags.push_back(_tag.get_tag());
			}
		});
}

void Core::changed_extra_requested_system_tags()
{
	s_setExtraRequestedSystemTags.clear();

	Tags t;
	t.load_from_string(String(s_extraRequestedSystemTags));
	t.do_for_every_tag([](Tag const& _tag)
		{
			if (_tag.get_relevance() > 0.0f)
			{
				s_setExtraRequestedSystemTags.push_back(_tag.get_tag());
			}
		});

	s_changedExtraRequestedSystemTags.clear();
	for_every(t, s_loadedExtraRequestedSystemTags)
	{
		if (! s_setExtraRequestedSystemTags.does_contain(*t))
		{
			s_changedExtraRequestedSystemTags.push_back_unique(*t);
		}
	}
	for_every(t, s_setExtraRequestedSystemTags)
	{
		if (! s_loadedExtraRequestedSystemTags.does_contain(*t))
		{
			s_changedExtraRequestedSystemTags.push_back_unique(*t);
		}
	}
}

bool Core::has_extra_requested_system_tag_changed(Name const& _tag)
{
	return s_changedExtraRequestedSystemTags.does_contain(_tag);
}

String Core::get_extra_requested_system_tags()
{
	return String(s_extraRequestedSystemTags);
}

Core* Core::s_current = nullptr;

bool Core::s_sdlInitialised = false;

#ifdef TIME_MANIPULATIONS_DEV
static int oneFrameOnly = 0;
static float oneFrameLength = 0.0f;
static float timeMultiplierDev = 1.0f;
#endif
static float timeMultiplier = 1.0f;
static float timePauser = 1.0f;
static int timePauseVR = 0;
static int renderingPause = 0;

static bool isPerformingQuickExit = false;

float time_pauser()
{
	return timePauser * (timePauseVR ? 0.000001f : 1.0f); // we want to advance a little bit
}

#ifdef AN_DEVELOPMENT
tm g_currentTimeInfo;
Concurrency::SpinLock g_currentTimeInfoLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

void update_current_time_info()
{
	time_t currentTime = time(0);

	Concurrency::ScopedSpinLock lock(g_currentTimeInfoLock);
#ifdef AN_WINDOWS
	localtime_s(&g_currentTimeInfo, &currentTime);
#else
#ifdef AN_LINUX_OR_ANDROID
	if (auto * tm = localtime(&currentTime))
	{
		g_currentTimeInfo = *tm;
	}
#else
#error implement
#endif
#endif
}
#endif

tchar s_appName[1025] = TXT("void room");

tchar const* Core::get_app_name()
{
	return s_appName;
}

void Core::set_app_name(tchar const* _name)
{
	tsprintf(s_appName, 1024, _name);
}

void Core::fill_time_info(tchar * buf, int bufSize)
{
#ifdef AN_DEVELOPMENT
	Concurrency::ScopedSpinLock lock(g_currentTimeInfoLock);
	tm & currentTimeInfo = g_currentTimeInfo;
	tm* pcurrentTimeInfo = &currentTimeInfo;
#else
	time_t currentTime = time(0);
	tm currentTimeInfo;
	tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
	pcurrentTimeInfo = localtime(&currentTime);
#else
	localtime_s(&currentTimeInfo, &currentTime);
#endif
#endif
#ifdef FILL_TIME_LONG
	#ifdef OUTPUT_WITH_THREAD_ID
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
		tsprintf(buf, bufSize, TXT("%04i-%02i-%02i %02i:%02i:%02i [%06i : %012.6f] (%07X=%02i) "),
	#else
		tsprintf(buf, bufSize, TXT("%04i-%02i-%02i %02i:%02i:%02i [%06i : %012.6f] (%07X) "),
	#endif
	#else
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
		tsprintf(buf, bufSize, TXT("%04i-%02i-%02i %02i:%02i:%02i [%06i : %012.6f] (%02i) "),
	#else
		tsprintf(buf, bufSize, TXT("%04i-%02i-%02i %02i:%02i:%02i [%06i : %012.6f] "),
	#endif
	#endif
			pcurrentTimeInfo->tm_year + 1900, pcurrentTimeInfo->tm_mon + 1, pcurrentTimeInfo->tm_mday,
			pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec, s_current? get_frame() : 0, s_current? get_time_since_core_started() : 0.0f
	#ifdef OUTPUT_WITH_THREAD_ID
			, Concurrency::ThreadSystemUtils::get_current_thread_system_id()
	#endif
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
			, Concurrency::ThreadManager::get()? Concurrency::ThreadManager::get()->get_current_thread_id(true) : NONE
	#endif
		);
#else
#ifdef FILL_TIME_FULL_HHMMSS
	#ifdef OUTPUT_WITH_THREAD_ID
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
		tsprintf(buf, bufSize, TXT("%02i:%02i:%02i [%06i : %012.6f] (%07X=%02i) "),
	#else
		tsprintf(buf, bufSize, TXT("%02i:%02i:%02i [%06i : %012.6f] (%07X) "),
	#endif
	#else
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
		tsprintf(buf, bufSize, TXT("%02i:%02i:%02i [%06i : %012.6f] (%02i) "),
	#else
		tsprintf(buf, bufSize, TXT("%02i:%02i:%02i [%06i : %012.6f] "),
	#endif
	#endif
			pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec, s_current? get_frame() : 0, s_current? get_time_since_core_started() : 0.0f
	#ifdef OUTPUT_WITH_THREAD_ID
			, Concurrency::ThreadSystemUtils::get_current_thread_system_id()
	#endif
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
			, Concurrency::ThreadManager::get()? Concurrency::ThreadManager::get()->get_current_thread_id(true) : NONE
	#endif
		);
#else
	#ifdef OUTPUT_WITH_THREAD_ID
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
		tsprintf(buf, bufSize, TXT("%06i:%08.3f] (%07X=%02i) "),
	#else
		tsprintf(buf, bufSize, TXT("%06i:%08.3f] (%07X) "),
	#endif
	#else
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
		tsprintf(buf, bufSize, TXT("%06i:%08.3f] (%02i) "),
	#else
		tsprintf(buf, bufSize, TXT("%06i:%08.3f] "),
	#endif
	#endif
			s_current ? get_frame() : 0, s_current ? get_time_since_core_started() : 0.0f
	#ifdef OUTPUT_WITH_THREAD_ID
			, Concurrency::ThreadSystemUtils::get_current_thread_system_id()
	#endif
	#ifdef OUTPUT_WITH_THREAD_ID_VIA_THREAD_MANAGER
			, Concurrency::ThreadManager::get() ? Concurrency::ThreadManager::get()->get_current_thread_id(true) : NONE
	#endif
		);
#endif
#endif
}

Tags & Core::access_system_tags()
{
	if (s_current)
	{
		return s_current->systemTags;
	}
	else
	{
		static Tags tags;
		an_assert(false, TXT("don't call until initialised"));
		return tags;
	}
}

Tags const& Core::get_system_tags()
{
	if (s_current)
	{
		return s_current->systemTags;
	}
	else
	{
		return access_system_tags();
	}
}

String const & Core::get_system_name()
{
	if (s_current)
	{
		return s_current->systemName;
	}
	else
	{
		an_assert(false, TXT("don't call until initialised"));
		return String::empty();
	}
}

String const & Core::get_system_precise_name()
{
	if (s_current)
	{
		return s_current->systemPreciseName;
	}
	else
	{
		an_assert(false, TXT("don't call until initialised"));
		return String::empty();
	}
}

void Core::initialise_static()
{
#ifdef AN_DEVELOPMENT
	update_current_time_info();
#endif
}

float Core::get_time_speed()
{
	float result = time_pauser();
#ifdef TIME_MANIPULATIONS_DEV
	result *= timeMultiplierDev;
#endif
	result *= timeMultiplier;
	return result;
}

void Core::close_static(bool _willRestart)
{
#ifdef AN_SDL
	if (s_sdlInitialised)
	{
		output(TXT("closing SDL..."));
		SDL_Quit();
		output(TXT("closed SDL"));
	}
	s_sdlInitialised = false;
#endif
}

int Core::get_frame()
{
	an_assert(s_current);
	return s_current->frame;
}

void Core::reset_timer()
{
	an_assert(s_current);
	s_current->timerLong = 0.0f;
	s_current->timerPendulum = 0.0f;
	s_current->timerRawLong = 0.0f;
	s_current->timer = 0.0f;
	s_current->timerRaw = 0.0f;
	s_current->timerPendulum = 0.0f;
	s_current->timerPendulumResult = 0.0f;
}

float Core::get_timer()
{
	an_assert(s_current);
	return s_current->timer;
}

float Core::get_timer_mod(float _mod)
{
	an_assert(s_current);
	return (float)mod(s_current->timerLong, (double)_mod);
}

float Core::get_timed_pulse(float _period, Optional<Name> const& _curve)
{
	float pulsePt = get_timer_mod(_period) / _period;
	float power = pulsePt * 2.0f;
	an_assert(power >= 0.0f && power <= 2.0f);
	if (power > 1.0f)
	{
		power = 2.0f - power;
	}
	return BlendCurve::apply(_curve.get(Name::invalid()), power);
}

double Core::get_timer_as_double()
{
	an_assert(s_current);
	return s_current->timerLong;
}

float Core::get_timer_raw()
{
	an_assert(s_current);
	return s_current->timerRaw;
}

float Core::get_timer_pendulum()
{
	an_assert(s_current);
	return s_current->timerPendulumResult;
}

float Core::get_delta_time()
{
	an_assert(s_current);
#ifdef TIME_MANIPULATIONS_DEV
	if (oneFrameOnly)
	{
		return oneFrameLength * timeMultiplier * timeMultiplierDev;
	}
	return s_current->clampedDeltaTime * timeMultiplier * timeMultiplierDev * time_pauser();
#else
	return s_current->clampedDeltaTime * timeMultiplier * time_pauser();
#endif
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
bool Core::is_advancing_one_frame_only()
{
	return oneFrameOnly;
}
#endif

void Core::store_display_delta_time(float _deltaTime)
{
	an_assert(s_current);
	s_current->displayDeltaTime = _deltaTime;
	s_current->displayDeltaTimeProvided = true;
}

void Core::store_vr_delta_time(float _deltaTime)
{
	an_assert(s_current);
	s_current->vrDeltaTime = _deltaTime;
	s_current->vrDeltaTimeProvided = true;
}

void Core::force_vr_sync_delta_time(float _deltaTime)
{
	an_assert(s_current);
	// we store the value here
	s_current->vrSyncDeltaTime = _deltaTime;
	s_current->vrSyncDeltaTimeProvided = true;
	s_current->set_delta_time(_deltaTime);
}

void Core::pause_vr(int _flag)
{
	set_flag(timePauseVR, _flag);
}

void Core::unpause_vr(int _flag)
{
	clear_flag(timePauseVR, _flag);
}

bool Core::is_vr_paused()
{
	return timePauseVR != 0;
}

void Core::pause_rendering(int _flag)
{
	set_flag(renderingPause, _flag);
}

void Core::unpause_rendering(int _flag)
{
	clear_flag(renderingPause, _flag);
}

bool Core::is_rendering_paused()
{
	return renderingPause != 0;
}

void Core::pause(bool _pause)
{
	timePauser = _pause? 0.0f : 1.0f;
}

bool Core::is_paused()
{
	return time_pauser() == 0.0f;
}

#ifdef TIME_MANIPULATIONS_DEV
void Core::slow_down_time_pace()
{
	if (timePauser == 0.0f)
	{
		timeMultiplierDev = 0.125f;
		timePauser = 1.0f;
	}
	else
	{
		if (timeMultiplierDev > 1.0f && timeMultiplierDev * 0.5f <= 1.0f)
		{
			timeMultiplierDev = 1.0f;
		}
		else
		{
			timeMultiplierDev *= 0.5f;
		}
	}
}

void Core::normal_time_pace()
{
	timeMultiplierDev = 1.0f;
	timePauser = 1.0f;
}

void Core::speed_up_time_pace()
{
	if (timePauser == 0.0f)
	{
		timeMultiplierDev = 0.125f;
		timePauser = 1.0f;
	}
	else
	{
		if (timeMultiplierDev < 1.0f && timeMultiplierDev * 2.0f >= 1.0f)
		{
			timeMultiplierDev = 1.0f;
		}
		else
		{
			timeMultiplierDev *= 2.0f;
		}
	}
}

void Core::one_frame_time_pace(int _frames, float _fps)
{
	if (timePauser == 0.0f)
	{
		oneFrameOnly = _frames;
		oneFrameLength = 1.0f / _fps;
	}
	else
	{
		timePauser = 0.0f;
	}
}

float Core::get_time_pace()
{
	return time_pauser() * timeMultiplier * timeMultiplierDev;
}
#endif

void Core::set_time_multiplier(float _timeMultiplier)
{
	timeMultiplier = _timeMultiplier;
}

float Core::get_raw_delta_time()
{
	an_assert(s_current);
	return s_current->rawDeltaTime;
}

void Core::create()
{
	an_assert(s_current == nullptr);
	s_current = new Core();
}

void Core::terminate()
{
	if (s_current)
	{
		Input::terminate();
		Sound::terminate();
		Video2D::terminate();
		Video3D::terminate();
	}
	delete_and_clear(s_current);
}

Core::Core()
: deltaTime(0.0f)
, clampedDeltaTime(0.0f)
, lastTimePoint(zero_time_point())
, timer(0.0f)
{
	coreStartedTimeStamp.reset();
	update_on_system_change_internal();
#ifdef AN_SDL
	if (!s_sdlInitialised)
	{
		s_sdlInitialised = true;
		SDL_SetMainReady();
		SDL_Init(0);
		SDL_InitSubSystem(SDL_INIT_TIMER);
		SDL_InitSubSystem(SDL_INIT_VIDEO);
		SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER); // gamecontroller inits joystick too
		Splash::Logos::add(TXT("sdl"), SPLASH_LOGO_SYSTEM);
	}
#endif
	Network::Network::create();
}

Core::~Core()
{
	Network::Network::terminate();
}

void Core::update_on_system_change()
{
	an_assert(s_current);
	s_current->update_on_system_change_internal();
}

void Core::update_on_system_change_internal()
{
	systemTags.clear();
	{
#ifdef AN_ANDROID
		systemName = TXT("androidHeadset");
		systemPreciseName = TXT("androidHeadset");
	#ifdef AN_QUEST
		systemName = TXT("quest");
		systemPreciseName = TXT("quest");
	#endif
	#ifdef AN_VIVE
		systemName = TXT("vive");
		systemPreciseName = TXT("vive");
	#endif
	#ifdef AN_PICO
		systemName = TXT("pico");
		systemPreciseName = TXT("pico");
	#endif

		{
			char model[PROP_VALUE_MAX]; // PROP_VALUE_MAX from <sys/system_properties.h>.
			int len = __system_property_get("ro.product.model", model);
			if (len > 0)
			{
				String modelId = String::from_char8(model);
				actualSystemName = modelId;
				if (modelId == TXT("Quest"))
				{
					systemName = TXT("quest");
					systemPreciseName = TXT("quest");
				}
				if (modelId == TXT("Quest 2"))
				{
					systemName = TXT("quest");
					systemPreciseName = TXT("quest2");
				}
				if (modelId == TXT("Quest 3"))
				{
					systemName = TXT("quest");
					systemPreciseName = TXT("quest3");
				}
				if (modelId == TXT("Quest Pro"))
				{
					systemName = TXT("quest");
					systemPreciseName = TXT("questPro");
				}
				if (modelId == TXT("VIVE Focus 3"))
				{
					systemName = TXT("vive");
					systemPreciseName = TXT("focus3");
				}
			}
		}
#else
		systemName = TXT("pc");
		systemPreciseName = TXT("pc");
	#ifdef AN_WINDOWS
		{
			actualSystemName = TXT("Windows, PC");

			#define INFO_BUFFER_SIZE 32767
			tchar infoBuf[INFO_BUFFER_SIZE];
			DWORD bufCharCount = ExpandEnvironmentStrings(TXT("%OS%"), infoBuf, INFO_BUFFER_SIZE);
			if (bufCharCount < INFO_BUFFER_SIZE && bufCharCount > 0)
			{
				actualSystemName = infoBuf;
			}
		}
	#endif
#endif

		if (!MainConfig::global().get_forced_system().is_empty())
		{
			String forcedSystem = MainConfig::global().get_forced_system();
			if (forcedSystem == TXT("quest"))
			{
				systemName = TXT("quest");
				systemPreciseName = TXT("quest");
			}
			else if (forcedSystem == TXT("quest2"))
			{
				systemName = TXT("quest");
				systemPreciseName = TXT("quest2");
			}
			else if (forcedSystem == TXT("quest3"))
			{
				systemName = TXT("quest");
				systemPreciseName = TXT("quest3");
			}
			else if (forcedSystem == TXT("questPro"))
			{
				systemName = TXT("quest");
				systemPreciseName = TXT("questPro");
			}
			else if (forcedSystem == TXT("vive"))
			{
				systemName = TXT("vive");
				systemPreciseName = TXT("vive");
			}
			else if (forcedSystem == TXT("pico"))
			{
				systemName = TXT("pico");
				systemPreciseName = TXT("pico");
			}
			else if (forcedSystem == TXT("pc"))
			{
				systemName = TXT("pc");
				systemPreciseName = TXT("pc");
			}
			else if (forcedSystem == TXT("androidHeadset"))
			{
				systemName = TXT("androidHeadset");
				systemPreciseName = TXT("androidHeadset");
			}
			else
			{
				error(TXT("unknown forced system \"%S\""), forcedSystem.to_char());
			}
		}

		if (systemName == TXT("quest"))
		{
			systemTags.set_tag(Name(TXT("android")));
			systemTags.set_tag(Name(TXT("androidHeadset")));
			systemTags.set_tag(Name(TXT("lowGraphics")));
			systemTags.set_tag(Name(TXT("quest")));
			Splash::Logos::add(TXT("quest"), SPLASH_LOGO_IMPORTANT);
			if (systemPreciseName == TXT("quest2"))
			{
				systemTags.set_tag(Name(TXT("quest2")));
			}
			if (systemPreciseName == TXT("quest3"))
			{
				systemTags.set_tag(Name(TXT("quest3")));
			}
			if (systemPreciseName == TXT("questPro"))
			{
				systemTags.set_tag(Name(TXT("questPro")));
			}
		}
		if (systemName == TXT("vive"))
		{
			systemTags.set_tag(Name(TXT("android")));
			systemTags.set_tag(Name(TXT("androidHeadset")));
			systemTags.set_tag(Name(TXT("lowGraphics")));
			systemTags.set_tag(Name(TXT("vive")));
			Splash::Logos::add(TXT("vive"), SPLASH_LOGO_IMPORTANT);
			if (systemPreciseName == TXT("focus3"))
			{
				systemTags.set_tag(Name(TXT("focus3")));
			}
		}
		if (systemName == TXT("pico"))
		{
			systemTags.set_tag(Name(TXT("android")));
			systemTags.set_tag(Name(TXT("androidHeadset")));
			systemTags.set_tag(Name(TXT("lowGraphics")));
			systemTags.set_tag(Name(TXT("pico")));
			Splash::Logos::add(TXT("pico"), SPLASH_LOGO_IMPORTANT);
		}
		if (systemName == TXT("androidHeadset"))
		{
			systemTags.set_tag(Name(TXT("android")));
			systemTags.set_tag(Name(TXT("androidHeadset")));
			systemTags.set_tag(Name(TXT("lowGraphics")));
			Splash::Logos::add(TXT("android"), SPLASH_LOGO_IMPORTANT);
		}
		if (systemName == TXT("pc"))
		{
			systemTags.set_tag(Name(TXT("pc")));
			systemTags.set_tag(Name(TXT("windows")));
			Splash::Logos::add(TXT("windows"), SPLASH_LOGO_IMPORTANT);
		}

		an_assert(!systemName.is_empty(), TXT("could not determine system name"));
	}
/*
// this is hack to downgrade version to pass submission as VRC.PC.Performance.5 is not required but the reviewer was forcing it to be required.
#ifdef AN_WINDOWS
#ifdef AN_OCULUS_ONLY
	systemTags.set_tag(Name(TXT("lowGraphics")));
	systemTags.set_tag(Name(TXT("forceLowShaderSettings")));
#endif
#endif
*/
#ifdef AN_GL
	systemTags.set_tag(Name(TXT("gl")));
#endif
#ifdef AN_GLES
	systemTags.set_tag(Name(TXT("gles")));
#endif
	systemTags.load_from_string(String(s_extraRequestedSystemTags));

	output_system_info_internal();
}

void Core::output_system_info()
{
	an_assert(s_current);
	s_current->output_system_info_internal();
}

void Core::output_system_info_internal()
{
	output(TXT("system: %S"), actualSystemName.to_char());
#ifndef AN_PICO
	if (actualSystemName == TXT("A8110"))
	{
		output(TXT("non PICO build on PICO?"));
	}
#endif
	output(TXT("system name: %S"), systemName.to_char());
	output(TXT("system precise name: %S"), systemPreciseName.to_char());
	output(TXT("system tags: %S"), systemTags.to_string().to_char());
	auto* lang = get_system_language();
	output(TXT("system language: %S"), lang? lang : TXT("unavailable"));
	output(TXT("shader options: %S"), ::MainSettings::global().get_shader_options().to_string().to_char());
}

bool Core::has_focus()
{
	an_assert(s_current != nullptr);
	return s_current->hasFocus;
}

void Core::advance()
{
#ifdef AN_DEVELOPMENT
	update_current_time_info();
#endif

	an_assert(s_current != nullptr);
#ifdef AN_SDL
	uint64 newTimePoint = SDL_GetPerformanceCounter();
	uint64 tickFrequency = SDL_GetPerformanceFrequency();
	s_current->rawDeltaTime = (float)(newTimePoint - s_current->lastTimePoint) / (float)tickFrequency;
#else
#ifdef AN_LINUX_OR_ANDROID
	TimePoint newTimePoint;
	clock_gettime(CLOCK_MONOTONIC, &newTimePoint);
	if (is_time_point_zero(s_current->lastTimePoint))
	{
		s_current->lastTimePoint = newTimePoint;
	}
	// in seconds
	s_current->rawDeltaTime = float(double(newTimePoint.tv_nsec - s_current->lastTimePoint.tv_nsec) * 0.000000001) +
							 (float(newTimePoint.tv_sec - s_current->lastTimePoint.tv_sec));
#else
	#error implement
#endif
#endif
#ifndef BUILD_PUBLIC_RELEASE
	if (s_current->rawDeltaTime > 0.1f)
	{
		warn_dev_ignore(TXT("excessive raw delta time : %.3f"), s_current->rawDeltaTime);
	}
#endif
	s_current->rawDeltaTime = clamp(s_current->rawDeltaTime, 0.0f, 1.0f); // 1s is enough for "raw"
	s_current->deltaTime = s_current->rawDeltaTime;
	s_current->lastTimePoint = newTimePoint;

#ifdef USE_OTHER_DELTA_TIMES
	if (s_current->vrSyncDeltaTimeProvided)
	{
		// this is forced value, although it should be reset after we do vr sync
		s_current->deltaTime = s_current->vrSyncDeltaTime;
	}
	else if (s_current->vrDeltaTimeProvided)
	{
		// sometimes vr system may decide to slows us down to half frames to have consistent behaviour
		s_current->deltaTime = s_current->vrDeltaTime != 0.0f? max(1.0f, round(s_current->deltaTime / s_current->vrDeltaTime)) * s_current->vrDeltaTime : 0.0f;
	}
	else if (s_current->displayDeltaTimeProvided)
	{
		s_current->deltaTime = s_current->displayDeltaTime;
	}
	s_current->vrSyncDeltaTimeProvided = false;
	s_current->vrDeltaTimeProvided = false;
	s_current->displayDeltaTimeProvided = false;
#endif

	// clear it now, this is new for this frame!
	s_current->deltaTimeUsedSoFar = 0.0f;
	s_current->rawDeltaTimeUsedSoFar = 0.0f;

	s_current->set_delta_time(s_current->deltaTime);

	++s_current->frame;

	Input* input = Input::get();

	{
		MEASURE_PERFORMANCE_COLOURED(advanceSystem_input, Colour(0.2f, 0.2f, 0.2f));
		if (input)
		{
			input->prepare_for_events();
			input->advance(s_current->clampedDeltaTime);
		}
	}

#ifdef AN_STANDARD_INPUT
#ifdef AN_DEBUG_KEYS
	if (::System::Input::get())
	{
		if (::System::Input::get()->has_key_been_pressed(::System::Key::F9))
		{
			output_recent_memory_allocations(100);
		}
#ifdef BUILD_NON_RELEASE
		if (::System::Input::get()->has_key_been_pressed(::System::Key::F11))
		{
			auto& rtsso = ::System::RenderTargetSaveSpecialOptions::access();
			rtsso.transparent = !rtsso.transparent;
		}
#endif
	}
#endif
#endif

	// update focus if we should have it but we've lost it for some reason
	bool newFocus = false;
	Video* video = nullptr;
	if (Video2D::get())
	{
		video = Video2D::get();
	}
	if (Video3D::get())
	{
		if (auto* v3d = Video3D::get())
		{
			v3d->destroy_pending(1);
			video = v3d;
		}
	}
	if (video)
	{
#ifdef AN_SDL
		if (video->is_foreground_window() && SDL_GetKeyboardFocus() == video->get_window() && SDL_GetMouseFocus() == video->get_window())
		{
			newFocus = true;
		}
		// if we should be within window but we aren't, move mouse inside window
		if (newFocus && s_current->hasFocus && Input::get() && Input::get()->is_grabbed())
		{
			int w, h;
			int x, y;
			SDL_GetWindowSize(video->get_window(), &w, &h);
			SDL_GetWindowPosition(video->get_window(), &x, &y);

			int mx, my;
			SDL_GetGlobalMouseState(&mx, &my);

			if (mx < x || my < y || mx > x + w || my > y + h)
			{
				SDL_WarpMouseGlobal(x + w / 2, y + h / 2);
				SDL_SetWindowGrab(video->get_window(), to_sdl_bool(true));
				SDL_CaptureMouse(to_sdl_bool(true));
			}
		}
#endif
	}

	if (s_current->hasFocus != newFocus)
	{
		s_current->hasFocus = newFocus;
		if (input)
		{
			input->update_grab();
		}
	}

	// handle events
#ifdef AN_SDL
	{
		MEASURE_PERFORMANCE_COLOURED(advanceSystem_handleEvents, Colour(0.3f, 0.1f, 0.1f));
		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent) != 0)
		{
			MEASURE_PERFORMANCE_COLOURED(advanceSystem_handleEvent, Colour(0.1f, 0.3f, 0.1f));
			if (sdlEvent.type == SDL_QUIT)
			{
				output(TXT("quit"));
				quick_exit();
			}
			else if (sdlEvent.type == SDL_WINDOWEVENT)
			{
				if (sdlEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				{
					if (auto * v = System::Video2D::get())
					{
						v->set_focus(true);
					}
					if (auto * v = System::Video3D::get())
					{
						v->set_focus(true);
					}
					s_current->hasFocus = true;
				}
				else if (sdlEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
				{
					if (auto * v = System::Video2D::get())
					{
						v->set_focus(false);
					}
					if (auto * v = System::Video3D::get())
					{
						v->set_focus(false);
					}
					s_current->hasFocus = false;
				}
				if (input)
				{
					input->update_grab();
				}
			}
#ifdef AN_STANDARD_INPUT
			else if (sdlEvent.type == SDL_MOUSEMOTION)
			{
				if (input && has_focus())
				{
					input->accumulate_mouse_motion(Vector2((float)sdlEvent.motion.xrel, -(float)sdlEvent.motion.yrel)); // invert Y because sdl gives as 0,0 as TOP left corner and we have 0,0 in BOTTOM left corner
					VectorInt2 windowSize = VectorInt2::zero;
					if (auto * v2d = System::Video2D::get())
					{
						windowSize = v2d->get_screen_size();
					}
					if (auto * v3d = System::Video3D::get())
					{
						windowSize = v3d->get_screen_size();
					}
					input->store_mouse_window_location(Vector2((float)sdlEvent.motion.x, (float)(windowSize.y - sdlEvent.motion.y)));
				}
			}
			else if (sdlEvent.type == SDL_MOUSEBUTTONDOWN)
			{
				if (input && has_focus())
				{
					input->store_mouse_button(sdlEvent.button.button - 1, true);
				}
			}
			else if (sdlEvent.type == SDL_MOUSEBUTTONUP)
			{
				if (input && has_focus())
				{
					input->store_mouse_button(sdlEvent.button.button - 1, false);
				}
			}
			else if (sdlEvent.type == SDL_MOUSEWHEEL)
			{
				if (input && has_focus())
				{
					input->accumulate_mouse_wheel_motion(VectorInt2(sdlEvent.wheel.x, sdlEvent.wheel.y));
				}
			}
			else if (sdlEvent.type == SDL_CONTROLLERDEVICEADDED)
			{
				if (input)
				{
					Gamepad::create(sdlEvent.cdevice.which, input);
				}
			}
			else if (sdlEvent.type == SDL_CONTROLLERDEVICEREMOVED)
			{
				if (input)
				{
					if (Gamepad* gamepad = input->get_gamepad_by_system_id(sdlEvent.cdevice.which))
					{
						delete gamepad;
					}
				}
			}
			else if (sdlEvent.type == SDL_CONTROLLERBUTTONDOWN)
			{
				if (input)
				{
					if (Gamepad* gamepad = input->get_gamepad_by_system_id(sdlEvent.cdevice.which))
					{
						gamepad->store_button(sdlEvent.cbutton.button, true);
					}
				}
			}
			else if (sdlEvent.type == SDL_CONTROLLERBUTTONUP)
			{
				if (input)
				{
					if (Gamepad* gamepad = input->get_gamepad_by_system_id(sdlEvent.cdevice.which))
					{
						gamepad->store_button(sdlEvent.cbutton.button, false);
					}
				}
			}
			else if (sdlEvent.type == SDL_CONTROLLERAXISMOTION)
			{
				if (input)
				{
					if (Gamepad* gamepad = input->get_gamepad_by_system_id(sdlEvent.caxis.which))
					{
						gamepad->store_axis(sdlEvent.caxis.axis, clamp((float)sdlEvent.caxis.value / 32767.0f, -1.0f, 1.0f));
					}
				}
			}
#endif
		}
	}
#else
#ifdef AN_ANDROID
	// this is done through Android::App::get().process_events(); through VR System's advance_events
#else
	#error implement for non sdl
#endif
#endif
	if (input)
	{
		MEASURE_PERFORMANCE_COLOURED(advanceSystem_inputPost, Colour(0.2f, 0.2f, 0.2f));
		input->post_events();
		input->update_gamepad_is_active();
	}
#ifdef TIME_MANIPULATIONS_DEV
	if (oneFrameOnly > 0)
	{
		-- oneFrameOnly;
		timePauser = 0.0f;
	}
#ifdef AN_DEBUG_KEYS
	if (Input::get())
	{
#ifdef AN_STANDARD_INPUT
		if (Input::get()->has_key_been_pressed(Key::F3))
		{
			float fps = 90.0f;
			if (Input::get()->is_key_pressed(Key::LeftShift) ||
				Input::get()->is_key_pressed(Key::RightShift))
			{
				fps = 30.0f;
			}
			one_frame_time_pace(1, fps);
		}
		if (Input::get()->has_key_been_pressed(Key::F4) &&
			!Input::get()->is_key_pressed(Key::LeftAlt) &&
			!Input::get()->is_key_pressed(Key::RightAlt))
		{
			pause(! is_paused());
		}
		if (Input::get()->has_key_been_pressed(Key::F5))
		{
			slow_down_time_pace();
		}
		if (Input::get()->has_key_been_pressed(Key::F6))
		{
			normal_time_pace();
		}
		if (Input::get()->has_key_been_pressed(Key::F7))
		{
			speed_up_time_pace();
		}
#endif
	}
#endif
#endif

	for_every(oa, s_current->onAdvance)
	{
		if (oa->perform &&
			oa->blocked <= 0)
		{
			oa->perform();
		}
	}
}

void Core::sleep(float _seconds)
{
#ifdef AN_LINUX_OR_ANDROID
	timespec ts = prepare_timespec(_seconds);
	nanosleep(&ts, nullptr);
#else
#ifdef AN_SDL
	SDL_Delay(max(1, (int)(_seconds * 1000.0f)));
#else
#if AN_WINDOWS
	Sleep(max(1, (int)(_seconds * 1000.0f)));
#else
	#error implement
#endif
#endif
#endif
}

void Core::sleep_u(int _microSeconds)
{
	sleep_n(_microSeconds * 1000);
}

void Core::sleep_n(int _nanoSeconds)
{
#ifdef AN_LINUX_OR_ANDROID
	timespec ts = prepare_timespec_nano_clamped(_nanoSeconds);
	nanosleep(&ts, nullptr);
#else
	sleep((float)_nanoSeconds * 1000000000.0f);
#endif
}

void Core::block_on_advance(Name const& _what)
{
	an_assert(s_current);

	for_every(oe, s_current->onAdvance)
	{
		if (oe->what == _what)
		{
			++ oe->blocked;
			return;
		}
	}

	s_current->onAdvance.push_back(OnAdvance(_what, nullptr));
	++s_current->onAdvance.get_last().blocked;
}

void Core::allow_on_advance(Name const& _what)
{
	an_assert(s_current);

	for_every(oe, s_current->onAdvance)
	{
		if (oe->what == _what)
		{
			--oe->blocked;
			return;
		}
	}

	// we may require negative values!
	s_current->onAdvance.push_back(OnAdvance(_what, nullptr));
	--s_current->onAdvance.get_last().blocked;
}

void Core::on_advance(Name const& _what, std::function<void()> _do)
{
	an_assert(s_current);

	for_every(oe, s_current->onAdvance)
	{
		if (oe->what == _what)
		{
			oe->perform = _do;
			return;
		}
	}

	s_current->onAdvance.push_back(OnAdvance(_what, _do));
}

void Core::on_advance_no_longer(Name const& _what)
{
	an_assert(s_current);

	for_every(oe, s_current->onAdvance)
	{
		if (oe->what == _what)
		{
			// don't remove, just clear
			oe->perform = nullptr;
			return;
		}
	}
}

void Core::on_quick_exit(tchar const * _what, std::function<void()> _do)
{
	an_assert(s_current);

	for_every(oe, s_current->onQuickExit)
	{
		if (String::compare_icase(oe->what, _what))
		{
			oe->perform = _do;
			return;
		}
	}

	s_current->onQuickExit.push_back(OnQuickExit(_what, _do));
}

void Core::on_quick_exit_no_longer(tchar const * _what)
{
	an_assert(s_current);

	for_every(oe, s_current->onQuickExit)
	{
		if (String::compare_icase(oe->what, _what))
		{
			s_current->onQuickExit.remove_at(for_everys_index(oe));
			return;
		}
	}
}

void Core::quick_exit(bool _immediate)
{
	if (s_current && s_current->quickExitHandler)
	{
		if (_immediate)
		{
			s_current->quickExitHandler = nullptr;
		}
		else
		{
			s_current->quickExitHandler();
			return;
		}
	}
	if (isPerformingQuickExit)
	{
		return;
	}
	output(TXT("start exit"));
	isPerformingQuickExit = true;
	while (s_current && !s_current->onQuickExit.is_empty())
	{
		output(s_current->onQuickExit.get_last().what);
		std::function<void()> perform = s_current->onQuickExit.get_last().perform;
		s_current->onQuickExit.pop_back();
		if (perform)
		{
			perform();
		}
	}
	output(TXT("exit now"));
	close_output();
	std::quick_exit(0);
}

bool Core::is_performing_quick_exit()
{
	return isPerformingQuickExit;
}

void Core::set_quick_exit_handler(std::function<void()> _quickExitHandler)
{
	if (s_current)
	{
		s_current->quickExitHandler = _quickExitHandler;
	}
}

void Core::set_title(String const& _name)
{
#ifdef AN_SDL
	if (auto * v = Video3D::get())
	{
		v->set_window_title(_name);
	}
#endif
#ifdef AN_ANDROID
	Android::App::get().set_title(_name);
#endif
}

void Core::post_init_all_subsystems()
{
	if (auto* vr = VR::IVR::get())
	{
		vr->I_am_perf_thread_main();
	}
}

float Core::get_time_since_core_started()
{
	an_assert(s_current);
	return s_current->coreStartedTimeStamp.get_time_since();
}

void Core::set_delta_time(float _deltaTime, Optional<float> const & _rawDeltaTime)
{
	deltaTime = _deltaTime;
	clampedDeltaTime = clamp(deltaTime, 0.0f, 0.1f);
	rawDeltaTime = _rawDeltaTime.get(rawDeltaTime);

	// use diffs (against used so far) for timers
	float deltaTimeDiff = deltaTime - deltaTimeUsedSoFar;
	float rawDeltaTimeDiff = rawDeltaTime - rawDeltaTimeUsedSoFar;

#ifdef TIME_MANIPULATIONS_DEV
	timerLong += (double)(deltaTimeDiff * timeMultiplier * timeMultiplierDev * time_pauser());
	timerPendulum += deltaTimeDiff * timeMultiplier * timeMultiplierDev * time_pauser();
#else
	timerLong += (double)(deltaTimeDiff * timeMultiplier * time_pauser());
	timerPendulum += deltaTimeDiff * timeMultiplier * time_pauser();
#endif
	timerRawLong += (double)(rawDeltaTimeDiff);
	timer = (float)timerLong;
	timerRaw = (float)timerRawLong;
	timerPendulum = mod(timerPendulum, timerPendulumLoop);
	timerPendulumResult = 0.5f * timerPendulumLoop * sin_deg(360.0f * timerPendulum / timerPendulumLoop);

	// store if we change it
	deltaTimeUsedSoFar = deltaTime;
	rawDeltaTimeUsedSoFar = rawDeltaTime;
}

tchar const* Core::get_system_language()
{
	if (auto* ge = GameEnvironment::IGameEnvironment::get())
	{
		if (auto* lang = ge->get_language())
		{
			return lang;
		}
	}
#ifdef AN_WINDOWS
	// these values should not change
	static bool languageRead = false;
	static int const languageIDLength = 256;
	static tchar languageID[languageIDLength + 1];
	if (!languageRead)
	{
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, languageID, sizeof(tchar) * languageIDLength);
		languageRead = true;
	}
	return languageID;
#else
#ifdef AN_ANDROID
	return ::System::Android::App::get_system_language_id();
#else
	todo_note(TXT("implement measurement"));
#endif
#endif
	return nullptr;
}
