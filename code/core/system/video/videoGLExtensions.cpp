#include "videoGLExtensions.h"

#ifdef AN_WINDOWS
#define glGetProcAddress wglGetProcAddress
#endif

#ifdef AN_ANDROID
#define glGetProcAddress eglGetProcAddress
#endif

//

using namespace System;

//

VideoGLExtensions VideoGLExtensions::s_videoGLExtensions;

void VideoGLExtensions::init_extensions()
{
	const char* allExtensions = (const char*)glGetString(GL_EXTENSIONS);
#ifdef BUILD_NON_RELEASE
	{
		String allExtensionsStr = String::from_char8(allExtensions);
		output(TXT("all gl extensions: %S"), allExtensionsStr.to_char());
	}
#endif
#ifdef AN_GL
	if (allExtensions != NULL)
	{
		NVX_gpu_memory_info = strstr(allExtensions, "GL_NVX_gpu_memory_info");
	}
#endif
#ifdef AN_GLES
	if (allExtensions != NULL)
	{
		EXT_texture_border_clamp = strstr(allExtensions, "GL_EXT_texture_border_clamp") ||
								   strstr(allExtensions, "GL_OES_texture_border_clamp");
		EXT_sRGB_write_control = strstr(allExtensions, "GL_EXT_sRGB_write_control");
		EXT_discard_framebuffer = strstr(allExtensions, "GL_EXT_discard_framebuffer");
	}
	if (!glRenderbufferStorageMultisampleEXT)
	{
		glRenderbufferStorageMultisampleEXT = (VideoGLExtensions::PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)glGetProcAddress("glRenderbufferStorageMultisampleEXT");
	}
	if (!glFramebufferTexture2DMultisampleEXT)
	{
		glFramebufferTexture2DMultisampleEXT = (VideoGLExtensions::PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)glGetProcAddress("glFramebufferTexture2DMultisampleEXT");
	}
	if (!glDiscardFramebufferEXT)
	{
		glDiscardFramebufferEXT = (VideoGLExtensions::PFNGLDISCARDFRAMEBUFFEREXTPROC)glGetProcAddress("glDiscardFramebufferEXT");
	}
#endif
	if (allExtensions != NULL)
	{
		QCOM_texture_foveated = strstr(allExtensions, "GL_QCOM_texture_foveated");
		QCOM_framebuffer_foveated = strstr(allExtensions, "GL_QCOM_framebuffer_foveated");
	}
	if (!glTextureFoveationParametersQCOM)
	{
		glTextureFoveationParametersQCOM = (VideoGLExtensions::PFNGLTEXTUREFOVEATIONPARAMETERSQCOMPROC)glGetProcAddress("glTextureFoveationParametersQCOM");
	}
	if (!glFramebufferFoveationConfigQCOM)
	{
		glFramebufferFoveationConfigQCOM = (VideoGLExtensions::PFNGLFRAMEBUFFERFOVEATIONCONFIGQCOMPROC)glGetProcAddress("glFramebufferFoveationConfigQCOM");
	}
	if (!glFramebufferFoveationParametersQCOM)
	{
		glFramebufferFoveationParametersQCOM = (VideoGLExtensions::PFNGLFRAMEBUFFERFOVEATIONPARAMETERSQCOMPROC)glGetProcAddress("glFramebufferFoveationParametersQCOM");
	}
#ifdef AN_GL
	if (!glDebugMessageInsert)
	{
		glDebugMessageInsert = glDebugMessageInsertARB;
	}
#endif
	if (!glDebugMessageInsert)
	{
		glDebugMessageInsert = (VideoGLExtensions::PFNGLDEBUGMESSAGEINSERTPROC)glGetProcAddress("glDebugMessageInsert");
	}
	if (!glDebugMessageCallback)
	{
		glDebugMessageCallback = (VideoGLExtensions::PFNGLDEBUGMESSAGECALLBACKPROC)glGetProcAddress("glDebugMessageCallback");
	}

	output(TXT("gl extensions:"));
#ifdef AN_GLES
	output(TXT(" %c EXT_texture_border_clamp"), EXT_texture_border_clamp? '+' : '-');
	output(TXT(" %c EXT_sRGB_write_control"), EXT_sRGB_write_control ? '+' : '-');
	output(TXT(" %c EXT_discard_framebuffer"), EXT_discard_framebuffer ? '+' : '-');
#endif
	output(TXT(" %c QCOM_texture_foveated"), QCOM_texture_foveated ? '+' : '-');
	output(TXT(" %c QCOM_framebuffer_foveated"), QCOM_framebuffer_foveated ? '+' : '-');
#ifdef AN_GLES
	output(TXT(" %c glRenderbufferStorageMultisampleEXT"), glRenderbufferStorageMultisampleEXT ? '+' : '-');
	output(TXT(" %c glFramebufferTexture2DMultisampleEXT"), glFramebufferTexture2DMultisampleEXT ? '+' : '-');
#endif
	output(TXT(" %c glTextureFoveationParametersQCOM"), glTextureFoveationParametersQCOM ? '+' : '-');
	output(TXT(" %c glFramebufferFoveationConfigQCOM"), glFramebufferFoveationConfigQCOM ? '+' : '-');
	output(TXT(" %c glFramebufferFoveationParametersQCOM"), glFramebufferFoveationParametersQCOM ? '+' : '-');
	output(TXT(" %c glDebugMessageInsert"), glDebugMessageInsert ? '+' : '-');
}

