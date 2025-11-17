#pragma once

#include "video.h"

namespace System
{
	
	class Video2D
	:	public Video
	{
	public:
		static void create(VideoConfig const * _vc = nullptr);
		static void terminate();
		static Video2D* get() { return s_current; }

		bool is_ok();

		bool init(VideoConfig const & _vc);
		void close();

#ifdef AN_SDL
		SDL_Renderer* get_renderer() { return renderer; }
#endif

	private:
		static Video2D* s_current;
		
#ifdef AN_SDL
		SDL_Renderer* renderer;
#endif

		Video2D();
		~Video2D();

	};

	#include "video2d.inl"

};


