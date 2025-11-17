#pragma once

#include "..\..\globalSettings.h"

#ifdef AN_WINDOWS
#ifdef AN_GL
#include <gl\glew.h>
#include <gl\GL.h>
#endif
#ifdef AN_GLES
#include <gl\glew.h>
#error no gles for windows!
#endif
#endif

#ifdef AN_LINUX
#ifdef AN_GL
#include <gl\glew.h>
#include <gl\GL.h>
#endif
#ifdef AN_GLES
#include <gl\glew.h>
#error no gles for linux!
#endif
#endif

#ifdef AN_ANDROID
#ifdef AN_GL
#error no gl for android!
#endif
#ifdef AN_GLES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#endif
#endif


// taken from https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_clip_cull_distance.txt
#ifndef GL_MAX_CLIP_DISTANCES_EXT
#define GL_MAX_CLIP_DISTANCES_EXT	0x0D32
#endif

#ifndef GL_CLIP_DISTANCE0_EXT
#define GL_CLIP_DISTANCE0_EXT		0x3000
#endif

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB			0x8DB9
#endif

#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER			0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR		0x1004
#endif

#ifndef GL_FOVEATION_ENABLE_BIT_QCOM
#define GL_FOVEATION_ENABLE_BIT_QCOM					0x01
#endif

#ifndef GL_FOVEATION_SCALED_BIN_METHOD_BIT_QCOM
#define GL_FOVEATION_SCALED_BIN_METHOD_BIT_QCOM			0x02
#endif

#ifndef GL_TEXTURE_FOVEATED_FEATURE_BITS_QCOM
#define GL_TEXTURE_FOVEATED_FEATURE_BITS_QCOM			0x8BFB
#endif

#ifndef GL_TEXTURE_FOVEATED_FEATURE_QUERY_QCOM
#define GL_TEXTURE_FOVEATED_FEATURE_QUERY_QCOM			0x8BFD
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_FOVEATION_QCOM
#define GL_FRAMEBUFFER_INCOMPLETE_FOVEATION_QCOM		0x8BFF
#endif
 
#ifndef GL_TEXTURE_FOVEATED_MIN_PIXEL_DENSITY_QCOM
#define GL_TEXTURE_FOVEATED_MIN_PIXEL_DENSITY_QCOM		0x8BFC
#endif
 
#ifndef GL_TEXTURE_FOVEATED_NUM_FOCAL_POINTS_QUERY_QCOM
#define GL_TEXTURE_FOVEATED_NUM_FOCAL_POINTS_QUERY_QCOM	0x8BFE
#endif

#ifndef GL_DEBUG_SOURCE_APPLICATION
#define GL_DEBUG_SOURCE_APPLICATION	0x824A
#endif

#ifndef GL_DEBUG_TYPE_OTHER
#define GL_DEBUG_TYPE_OTHER	0x8251
#endif

#ifndef GL_DEBUG_TYPE_MARKER
#define GL_DEBUG_TYPE_MARKER	0x8268
#endif

#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION	0x826B
#endif

#ifndef GL_TEXTURE_2D_MULTISAMPLE
#define GL_TEXTURE_2D_MULTISAMPLE	0x9100
#endif

#ifndef GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX	        0x9047
#endif

#ifndef GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX      0x9048
#endif

#ifndef GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX	0x9049
#endif

#ifndef GPU_MEMORY_INFO_EVICTION_COUNT_NVX
#define GPU_MEMORY_INFO_EVICTION_COUNT_NVX	            0x904A
#endif

#ifndef GPU_MEMORY_INFO_EVICTED_MEMORY_NVX
#define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX	            0x904B
#endif

#ifndef GL_DEBUG_TYPE_ERROR
#define GL_DEBUG_TYPE_ERROR	            0x824C
#endif

#ifndef GL_DEBUG_OUTPUT 
#define GL_DEBUG_OUTPUT					0x92E0
#endif

 
//
