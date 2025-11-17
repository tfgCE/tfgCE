#include "video2d.h"
#include "..\input.h"

using namespace System;

Video2D* Video2D::s_current = nullptr;

void Video2D::create(VideoConfig const * _vc)
{
	an_assert(s_current == nullptr);
	s_current = new Video2D();
	if (_vc)
	{
		s_current->init(*_vc);
	}
	if (Input::get())
	{
		Input::get()->update_grab();
	}
}

void Video2D::terminate()
{
	delete_and_clear(s_current);
}

Video2D::Video2D()
{
#ifdef AN_SDL
	renderer = nullptr;
#endif
}

Video2D::~Video2D()
{
	close();
}

bool Video2D::init(VideoConfig const & _vc)
{
	close();

	VideoInitInfo vii;

	if (! Video::init(_vc, vii))
	{
		return false;
	}

#ifdef AN_SDL
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#else
	todo_important(TXT("implement"));
#endif
#ifdef AN_SDL
	if (!renderer)
#endif
	{
		// TODO report
		return false;
    }

	// by default, clear to something similar to black colour
#ifdef AN_SDL
	SDL_SetRenderDrawColor(renderer, 23,7,10, 0);
	SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
#else
	todo_important(TXT("implement"));
#endif

	return true;
}

void Video2D::close()
{
#ifdef AN_SDL
	if (renderer)
	{
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
#else
	todo_important(TXT("implement"));
#endif

	Video::close();
}

bool Video2D::is_ok()
{
	return Video::is_ok()
#ifdef AN_SDL
		&& renderer;
#endif
	;
}

