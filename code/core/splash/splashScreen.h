#pragma once

#include "..\globalDefinitions.h"

namespace Splash
{
	class ScreenImpl;

	class Screen
	{
	public:
		static void show(tchar const * _fileName = TXT("system\\splash"));
		static void close();

		static ScreenImpl* impl;
	};

};
