#include "eglContext.h"

#ifdef AN_ANDROID

//

using namespace System;

//

::System::EGLContext::EGLContext()
{
	display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, &majorVersion, &minorVersion);
	// Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
	// flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
	// settings, and that is completely wasted for our warp target.
	const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;
	if (eglGetConfigs(display, configs, MAX_CONFIGS, &numConfigs) == EGL_FALSE)
	{
		error(TXT("eglGetConfigs() failed: %i"), eglGetError());
		return;
	}
	const EGLint configAttribs[] =
	{
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE,		8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE,		8, // need alpha for the multi-pass timewarp compositor
		EGL_DEPTH_SIZE,		0,
		EGL_STENCIL_SIZE,	0,
		EGL_SAMPLES,		0,
		EGL_NONE
	};
	config = 0;
	for (int i = 0; i < numConfigs; i++)
	{
		EGLint value = 0;

		eglGetConfigAttrib(display, configs[i], EGL_RENDERABLE_TYPE, &value);
		if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR)
		{
			continue;
		}

		// The pbuffer config also needs to be compatible with normal window rendering
		// so it can share textures with the window context.
		eglGetConfigAttrib(display, configs[i], EGL_SURFACE_TYPE, &value);
		if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT))
		{
			continue;
		}

		int	j = 0;
		for (; configAttribs[j] != EGL_NONE; j += 2)
		{
			eglGetConfigAttrib(display, configs[i], configAttribs[j], &value);
			if (value != configAttribs[j + 1])
			{
				break;
			}
		}
		if (configAttribs[j] == EGL_NONE)
		{
			config = configs[i];
			break;
		}
	}
	if (config == 0)
	{
		error(TXT("eglChooseConfig() failed: %i"), eglGetError());
		return;
	}
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};
	context = eglCreateContext(display, config, nullptr, contextAttribs);
	if (context == nullptr)
	{
		error(TXT("eglCreateContext() failed: %i"), eglGetError());
		return;
	}
	const EGLint surfaceAttribs[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	tinySurface = eglCreatePbufferSurface(display, config, surfaceAttribs);
	if (tinySurface == nullptr)
	{
		error(TXT("eglCreatePbufferSurface() failed: %i"), eglGetError());
		eglDestroyContext(display, context);
		context = nullptr;
		return;
	}
	output_colour_system();
	output(TXT("egl version: %i.%i"), majorVersion, minorVersion);
	//output(TXT("    display: %p"), display);
	//output(TXT("    config : %p"), config);
	//output(TXT("    context: %p"), context);
	output_colour();
}

::System::EGLContext::~EGLContext()
{
	if (display != 0)
	{
		if (eglMakeCurrent(display, nullptr, nullptr, nullptr) == EGL_FALSE)
		{
			error(TXT("eglMakeCurrent() failed: %i"), eglGetError());
		}
	}
	if (context != nullptr)
	{
		if (eglDestroyContext(display, context) == EGL_FALSE)
		{
			error(TXT("eglDestroyContext() failed: %i"), eglGetError());
		}
		context = nullptr;
	}
	if (tinySurface != nullptr)
	{
		if (eglDestroySurface(display, tinySurface) == EGL_FALSE)
		{
			error(TXT("eglDestroySurface() failed: %i"), eglGetError());
		}
		tinySurface = nullptr;
	}
	if (display != 0)
	{
		if (eglTerminate(display) == EGL_FALSE)
		{
			error(TXT("eglTerminate() failed: %i"), eglGetError());
		}
		display = 0;
	}
}

void ::System::EGLContext::make_current()
{
	if (eglMakeCurrent(display, tinySurface, tinySurface, context) == EGL_FALSE)
	{
		error(TXT("eglMakeCurrent() failed: %i"), eglGetError());
	}
}

#endif
