#include "video.h"

#include "..\input.h"
#include "..\core.h"

#include "..\..\mainConfig.h"
#include "..\..\splash\splashScreen.h"
#include "..\..\vr\iVR.h"

#ifdef AN_SDL
#include "SDL_syswm.h"
#endif

using namespace System;

Video::Video()
: screenSize( 0, 0 )
{
#ifdef AN_SDL
	window = nullptr;
#endif
}

Video::~Video()
{
	close();
}

bool Video::init(VideoConfig const & _vc, VideoInitInfo const & _vii)
{
#ifdef AN_SDL
	if (window)
#endif
	{
		close();
	}

	config = _vc;
	config.fill_missing();

	// no vsync for vr
#ifdef AN_VR
	if (VR::IVR::can_be_used())
	{
		config.vsync = false;
	}
#endif

	screenSize = config.resolution;
	
	config.displayIndex = clamp(config.displayIndex, 0, get_display_count() - 1);

#ifdef AN_ANDROID
	// handled via EGLContext
#else
	int windowPosX = 0;
	int windowPosY = 0;
#ifdef AN_SDL
	windowPosX = SDL_WINDOWPOS_CENTERED_DISPLAY(config.displayIndex);
	windowPosY = SDL_WINDOWPOS_CENTERED_DISPLAY(config.displayIndex);
#endif

	VectorInt2 desktopSize = get_desktop_size(config.displayIndex);
	if (!desktopSize.is_zero() && config.fullScreen == FullScreen::WindowScaled)
	{
		config.resolutionFull = desktopSize;
	}

	VectorInt2 windowSize = config.resolution;
	if (config.fullScreen == FullScreen::WindowScaled)
	{
		windowSize = config.resolutionFull;
	}

#ifdef AN_DEVELOPMENT
	if (config.fullScreen == FullScreen::No)
	{
		VectorInt2 desktopSize = get_desktop_size(config.displayIndex);
		if (!desktopSize.is_zero())
		{
			// from top left corner
			windowPosX = min(desktopSize.x * 92 / 100, desktopSize.x - 50) - windowSize.x;
			windowPosY = max(desktopSize.y * 3 / 100, 20);
		}
	}
#endif

	// no name yet, will be provided later
#ifdef AN_SDL
	window = SDL_CreateWindow(windowTitle.to_char8_array().get_data(), windowPosX, windowPosY, windowSize.x, windowSize.y,
								(config.fullScreen == FullScreen::Yes? SDL_WINDOW_FULLSCREEN : 0) | SDL_WINDOW_SHOWN | (_vii.openGL ? SDL_WINDOW_OPENGL : 0));
#endif
#endif

	// close splash screen if present just right after window is created
	Splash::Screen::close();

#ifdef AN_SDL
	if (! window)
	{
		// TODO report
		return false;
	}
#endif

#ifdef AN_SDL
	{	// get resolution
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		an_assert(screenSize.x <= w);
		an_assert(screenSize.y <= h);
	}
#endif

#ifdef AN_WINDOWS
	HDC tempHDC;
	get_hwnd_and_hdc(windowHWND, tempHDC);
#endif

	isOk = true;
	return true;
}

void Video::close()
{
#ifdef AN_SDL
	if (window)
	{
		SDL_DestroyWindow(window);
		window = nullptr;
	}
#endif
#ifdef AN_WINDOWS
	windowHWND = nullptr;
#endif
}

void Video::update_grab_input(bool _grabInput)
{
#ifdef AN_SDL
	SDL_SetWindowGrab(window, to_sdl_bool(_grabInput));
	SDL_CaptureMouse(to_sdl_bool(_grabInput));
#endif
}

void Video::set_window_title(String const & _name)
{
	windowTitle = _name;
#ifdef AN_SDL
	SDL_SetWindowTitle(window, _name.to_char8_array().get_data());
#endif
}

void Video::take_focus()
{
#ifdef AN_WINDOWS
	SetForegroundWindow(windowHWND);
	SetFocus(windowHWND);
#endif
#ifdef AN_SDL
	SDL_SetWindowInputFocus(window);
#endif
}

void Video::set_focus(bool _focus)
{
#ifdef AN_WINDOWS
	if (config.fullScreen == FullScreen::WindowScaled ||
		config.fullScreen == FullScreen::WindowFull)
	{
		if (_focus
//#ifndef AN_DEVELOPMENT
			// because we don't want it to be in foreground when debugging
			&& (Input::get() && Input::get()->is_grabbed()) // if input is grabbed, we should be on top to avoid mouse being trapped in a task bar or somewhere else
//#endif
			)
		{
			// get on top of the taskbar
			SetWindowPos(windowHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		else
		{
			// hind behing the taskbar
			SetWindowPos(windowHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
	}
#endif
}

bool Video::is_foreground_window() const
{
#ifdef AN_WINDOWS
	return GetForegroundWindow() == windowHWND;
#else
	return true;
#endif
}

#ifdef AN_WINDOWS
void Video::get_hwnd_and_hdc(OUT_ HWND & _outHwnd, OUT_ HDC & _outHdc)
{
#ifdef AN_SDL
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version); // set version, otherwise it won't work (I wonder why it isn't part of constructor or SLD_GetWindowWMInfo)
	if (SDL_GetWindowWMInfo(window, &wmInfo))
	{
		_outHwnd = wmInfo.info.win.window;
		_outHdc = GetDC(_outHwnd);
	}
	else
	{
		String sdlError = String::from_char8(SDL_GetError());
		error(TXT("error while accessing window info %S"), sdlError.to_char());
		an_assert(false);
		_outHwnd = nullptr;
		_outHdc = nullptr;
	}
#endif
}
#endif

void Video::enumerate_resolutions(int _displayIndex, OUT_ Array<VectorInt2> & _resolutions)
{
#ifdef AN_SDL
	int displayModesCount = SDL_GetNumDisplayModes(_displayIndex);
	for_count(int, modeIndex, displayModesCount)
	{
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(_displayIndex, modeIndex, &mode) == 0)
		{
			_resolutions.push_back_unique(VectorInt2(mode.w, mode.h));
		}
	}
#endif
}

int Video::get_display_count()
{
#ifdef AN_SDL
	return SDL_GetNumVideoDisplays();
#endif
	return 1;
}

VectorInt2 Video::get_desktop_size(int _displayIndex)
{
	if (_displayIndex == NONE)
	{
		_displayIndex = MainConfig::global().get_video().displayIndex;
	}
#ifdef AN_SDL
	SDL_DisplayMode displayMode;
	if (SDL_GetDesktopDisplayMode(_displayIndex, &displayMode) == 0)
	{
		return VectorInt2(displayMode.w, displayMode.h);
	}
	else
#endif
	{
		return VectorInt2::zero;
	}
}

float Video::get_expected_frame_time(int _displayIndex, bool _idealOnly) const
{
	if (_displayIndex == NONE)
	{
		_displayIndex = config.displayIndex;
	}

	if (VR::IVR::can_be_used())
	{
		if (auto* vr = VR::IVR::get())
		{
			float frameTime = _idealOnly? vr->get_ideal_expected_frame_time() : vr->get_expected_frame_time();
			if (frameTime != 0.0f)
			{
				return frameTime;
			}
		}
	}
	if (config.vsync)
	{
#ifdef AN_SDL
		SDL_DisplayMode displayMode;
		if (SDL_GetDesktopDisplayMode(_displayIndex, &displayMode) == 0)
		{
			int refreshRate = displayMode.refresh_rate;
			if (refreshRate > 0)
			{
				return 1.0f / (float)refreshRate;
			}
			else
			{
				warn(TXT("could not get refresh rate for display %i"), _displayIndex);
			}
		}
#endif
	}

	return 0.0f;
}