#pragma once

#include "videoGL.h"

#include "..\video\video.h"

#ifdef AN_WINDOWS
	#include <wingdi.h>
	#include <gl\glext.h>
	#ifndef GL_APIENTRY
	#define GL_APIENTRY APIENTRY
	#endif
#endif

namespace System
{
	struct VideoGLExtensions
	{
		static VideoGLExtensions& get() { return s_videoGLExtensions; }

#ifdef AN_GL
		bool NVX_gpu_memory_info = false;
#endif
#ifdef AN_GLES
		bool EXT_texture_border_clamp = false;		// GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
		bool EXT_sRGB_write_control = false;		// GL_EXT_sRGB_write_control
		bool EXT_discard_framebuffer = false;		// GL_EXT_discard_framebuffer
		typedef void (GL_APIENTRY* PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
		typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
		typedef void (GL_APIENTRY* PFNGLDISCARDFRAMEBUFFEREXTPROC) (GLenum target, GLuint attachmentCount, GLenum const* attachments);
		PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT = nullptr;
		PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT = nullptr;
		PFNGLDISCARDFRAMEBUFFEREXTPROC glDiscardFramebufferEXT = nullptr;
#endif
		bool QCOM_texture_foveated = false;			// GL_QCOM_texture_foveated
		bool QCOM_framebuffer_foveated = false;		// GL_QCOM_framebuffer_foveated

		typedef void (GL_APIENTRY* PFNGLTEXTUREFOVEATIONPARAMETERSQCOMPROC) (GLint texture, GLint layer, GLint focalPoint, GLfloat focalX, GLfloat focalY, GLfloat gainX, GLfloat gainY, GLfloat foveaArea);
		typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERFOVEATIONCONFIGQCOMPROC) (GLuint fbo, GLuint numLayers, GLuint focalPointsPerLayer, GLuint requestedFeatures, GLuint* providedFeatures);
		typedef void (GL_APIENTRY* PFNGLFRAMEBUFFERFOVEATIONPARAMETERSQCOMPROC) (GLuint fbo, GLuint layer, GLuint focalPoint, GLfloat focalX, GLfloat focalY, GLfloat gainX, GLfloat gainY, GLfloat foveaArea);
		typedef void (GL_APIENTRY* PFNGLDEBUGMESSAGEINSERTPROC) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message);
		typedef void (GL_APIENTRY* PFNGLDEBUGPROC) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
		typedef void (GL_APIENTRY* PFNGLDEBUGMESSAGECALLBACKPROC) (PFNGLDEBUGPROC callback, const void* userParam);
		PFNGLTEXTUREFOVEATIONPARAMETERSQCOMPROC glTextureFoveationParametersQCOM = nullptr;
		PFNGLFRAMEBUFFERFOVEATIONCONFIGQCOMPROC glFramebufferFoveationConfigQCOM = nullptr;
		PFNGLFRAMEBUFFERFOVEATIONPARAMETERSQCOMPROC glFramebufferFoveationParametersQCOM = nullptr;
		PFNGLDEBUGMESSAGEINSERTPROC glDebugMessageInsert = nullptr;
		PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = nullptr;

		//

		void init_extensions();

	private:
		static VideoGLExtensions s_videoGLExtensions;
	};
};
