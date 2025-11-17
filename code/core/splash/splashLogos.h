#pragma once

#include "..\globalDefinitions.h"
#include "..\containers\array.h"
#include "..\containers\arrayStatic.h"

namespace System
{
	class Texture;
};

#define SPLASH_LOGO_IMPORTANT 20
#define SPLASH_LOGO_DEVICE 10
#define SPLASH_LOGO_SYSTEM 0

namespace Splash
{
	class Logos
	{
	public:
		static void add(tchar const* _fileName, int _priority = 0, bool _other = false);

		static void load_logos(REF_ Array<::System::Texture*>& _splashTextures, REF_ Array<::System::Texture*>& _otherSplashTextures, bool _randomisedOrder = true);
		static void unload_logos(REF_ Array<::System::Texture*>& _splashTextures, REF_ Array<::System::Texture*>& _otherSplashTextures);
	};

};

