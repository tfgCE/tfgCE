#pragma once

#include "globalSettings.h"

//
// general options

#ifdef BUILD_NON_RELEASE
	#define ALLOW_SELECTIVE_RENDERING
#endif

//
// actual profiling settings

//#define NO_TEMPORARY_OBJECTS_ON_DEATH
//#define NO_TEMPORARY_OBJECTS_ON_PROJECTILE_HIT
//#define NO_LIGHT_SOURCES

//#define DONT_ADD_TEMPORARY_OBJECTS_TO_RENDER_SCENE
// will add, process but won't render
//#define DONT_RENDER_TEMPORARY_OBJECTS

//
// utility structures

#ifdef ALLOW_SELECTIVE_RENDERING
struct SelectiveRendering
{
	static bool renderNow;
};
#endif
