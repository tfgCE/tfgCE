#include "videoClipPlaneStack.h"

#include "video3d.h"
#include "viewFrustum.h"

#include "videoClipPlaneStackImpl.inl"

//

using namespace System;

//

void VideoClipPlaneStack::open_clip_planes()
{
	GLint clipPlanesLimit;
#ifdef AN_GL
	DIRECT_GL_CALL_ glGetIntegerv(GL_MAX_CLIP_PLANES, &clipPlanesLimit); AN_GL_CHECK_FOR_ERROR
#else
#ifdef AN_GLES
	DIRECT_GL_CALL_ glGetIntegerv(GL_MAX_CLIP_DISTANCES_EXT, &clipPlanesLimit); AN_GL_CHECK_FOR_ERROR
#else
#error implement
#endif
#endif
	an_assert(clipPlanesLimit >= PLANE_SET_PLANE_LIMIT, TXT("not enough clip planes supported"));
#ifndef AN_OPEN_CLOSE_CLIP_PLANES
	while (clipPlanesEnabled < clipPlanesLimit && clipPlanesEnabled < PLANE_SET_PLANE_LIMIT)
	{
		DIRECT_GL_CALL_ glEnable(get_gl_clip_distance(clipPlanesEnabled)); AN_GL_CHECK_FOR_ERROR
		++clipPlanesEnabled;
	}
#endif
	isReadyForRendering = false;
}

void VideoClipPlaneStack::close_clip_planes()
{
#ifndef AN_OPEN_CLOSE_CLIP_PLANES
	int idx = 0;
	while (idx < clipPlanesEnabled)
	{
		DIRECT_GL_CALL_ glDisable(get_gl_clip_distance(idx)); AN_GL_CHECK_FOR_ERROR
		++idx;
	}
#endif
	clipPlanesEnabled = 0;
	isReadyForRendering = false;
}

void VideoClipPlaneStack::open_close_clip_planes_for_rendering()
{
#ifdef AN_OPEN_CLOSE_CLIP_PLANES
	while (clipPlanesEnabled > setReadyForRendering.planes.get_size())
	{
		DIRECT_GL_CALL_ glDisable(get_gl_clip_distance(clipPlanesEnabled - 1)); AN_GL_CHECK_FOR_ERROR
		-- clipPlanesEnabled;
	}
	while (clipPlanesEnabled < setReadyForRendering.planes.get_size())
	{
		DIRECT_GL_CALL_ glEnable(get_gl_clip_distance(clipPlanesEnabled)); AN_GL_CHECK_FOR_ERROR
		++ clipPlanesEnabled;
	}
#endif
}
