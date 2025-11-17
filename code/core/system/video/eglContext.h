#pragma once

#ifdef AN_ANDROID

#include "..\..\globalInclude.h"
#include "..\video\video.h"

namespace System
{
	struct EGLContext
	{
		EGLint majorVersion = 0;
		EGLint minorVersion = 0;
		EGLDisplay display = 0;
		EGLConfig config = 0;
		EGLSurface tinySurface = nullptr;
		::EGLContext context = nullptr;

		EGLContext();
		~EGLContext();

		void make_current();
	};
};

#endif
