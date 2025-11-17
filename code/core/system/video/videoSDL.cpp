#include "videoSDL.h"

using namespace System;

#ifdef AN_SDL
Concurrency::SpinLock VideoSDL::imageLock = Concurrency::SpinLock(TXT("System.VideoSDL.imageLock"));
#endif