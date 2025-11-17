#pragma once

#include "videoGL.h"

#include "videoConfig.h"

#ifdef AN_SDL
#include "SDL.h"
#endif

namespace System
{
	typedef GLuint TextureID;
	typedef GLuint RenderBufferID;
	typedef GLuint FrameBufferObjectID;
	typedef GLuint ShaderProgramID;
	typedef GLint ShaderParamID;
	typedef GLuint BufferObjectID;
	typedef GLuint VertexArrayObjectID;

	class Core;
	class Input;

	inline TextureID texture_id_none() { return 0; }

	struct VideoInitInfo
	{
		bool openGL;

		VideoInitInfo()
		:	openGL( false )
		{}
	};

	class Video
	{
	public:
		bool is_ok() const { return isOk; }

		bool init(VideoConfig const & _vc, VideoInitInfo const & _vii);
		void close();

		static int get_display_count();
		static VectorInt2 get_desktop_size(int _displayIndex = NONE);
		static void enumerate_resolutions(int _displayIndex, OUT_ Array<VectorInt2> & _resolutions);
		float get_expected_frame_time(int _displayIndex = NONE) const { return get_expected_frame_time(_displayIndex, false); }
		float get_ideal_expected_frame_time(int _displayIndex = NONE) const { return get_expected_frame_time(_displayIndex, true); }

		inline VectorInt2 get_screen_size();
		
#ifdef AN_SDL
		SDL_Window* get_window() { return window; }
#endif
		void set_window_title(String const& _name);

		bool is_foreground_window() const;

#ifdef AN_WINDOWS
		void get_hwnd_and_hdc(OUT_ HWND & _outHwnd, OUT_ HDC & _outHdc);
#endif

	private: friend class Input;
		void update_grab_input(bool _grabInput); // this is called solely by Input class

	private: friend class Core;
		void take_focus();
		void set_focus(bool _focus);

	protected:
		VideoConfig config;
		VectorInt2 screenSize;
		String windowTitle = String(TXT("void room"));
		bool isOk = false;
#ifdef AN_SDL
		SDL_Window* window;
#endif
#ifdef AN_WINDOWS
		HWND windowHWND;
#endif

		Video();
		~Video();

		float get_expected_frame_time(int _displayIndex, bool _onlyIdeal) const;

	};

	#include "video.inl"

};


